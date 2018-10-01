/**
 * MeshのDraco圧縮.
 * https://github.com/Microsoft/glTF-Toolkit の GLTFMeshCompressionUtils より.
 * gltf/glbのどちらでもDraco圧縮できるように機能追加.
 */
#include "GLTFMeshCompression.h"
#include "../BinStreamReaderWriter.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>

#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <GLTFSDK/ExtensionsKHR.h>
#include <GLTFSDK/BufferBuilder.h>
#include <GLTFSDK/GLBResourceWriter.h>
#include "AccessorUtils.h"

#pragma warning(push)
#pragma warning(disable: 4018 4081 4244 4267 4389)
#include "draco/draco_features.h"
#include "draco/compression/encode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#pragma warning(pop)

#include "../StringUtil.h"

using namespace Microsoft::glTF;

namespace {
draco::GeometryAttribute::Type GetTypeFromAttributeName (const std::string& name)
{
    if (name == ACCESSOR_POSITION)
    {
        return draco::GeometryAttribute::Type::POSITION;
    }
    if (name == ACCESSOR_NORMAL)
    {
        return draco::GeometryAttribute::Type::NORMAL;
    }
    if (name == ACCESSOR_TEXCOORD_0)
    {
        return draco::GeometryAttribute::Type::TEX_COORD;
    }
    if (name == ACCESSOR_TEXCOORD_1)
    {
        return draco::GeometryAttribute::Type::TEX_COORD;
    }
    if (name == ACCESSOR_COLOR_0)
    {
        return draco::GeometryAttribute::Type::COLOR;
    }
    if (name == ACCESSOR_JOINTS_0)
    {
        return draco::GeometryAttribute::Type::GENERIC;
    }
    if (name == ACCESSOR_WEIGHTS_0)
    {
        return draco::GeometryAttribute::Type::GENERIC;
    }
    if (name == ACCESSOR_TANGENT)
    {
        return draco::GeometryAttribute::Type::GENERIC;
    }
    return draco::GeometryAttribute::Type::GENERIC;
}

draco::DataType GetDataType (const Accessor& accessor)
{
    switch (accessor.componentType)
    {
    case COMPONENT_BYTE: return draco::DataType::DT_INT8;
    case COMPONENT_UNSIGNED_BYTE: return draco::DataType::DT_UINT8;
    case COMPONENT_SHORT: return draco::DataType::DT_INT16;
    case COMPONENT_UNSIGNED_SHORT: return draco::DataType::DT_UINT16;
    case COMPONENT_UNSIGNED_INT: return draco::DataType::DT_UINT32;
    case COMPONENT_FLOAT: return draco::DataType::DT_FLOAT32;
    }
    return draco::DataType::DT_INVALID;
}

template<typename T>
int InitializePointAttribute (draco::Mesh& dracoMesh, const std::string& attributeName, const Document& doc, GLBResourceReader* glbReader, GLTFResourceReader& reader, Accessor& accessor)
{
    auto stride = sizeof(T) * Accessor::GetTypeCount(accessor.type);
    auto numComponents = Accessor::GetTypeCount(accessor.type);
    draco::PointAttribute pointAttr;
    pointAttr.Init(GetTypeFromAttributeName(attributeName), nullptr, numComponents, GetDataType(accessor), accessor.normalized, stride, 0);
    int attId = dracoMesh.AddAttribute(pointAttr, true, static_cast<unsigned int>(accessor.count));
    auto attrActual = dracoMesh.attribute(attId);

    std::vector<T> values = glbReader ? (glbReader->ReadBinaryData<T>(doc, accessor)) : reader.ReadBinaryData<T>(doc, accessor);

    if ((accessor.min.empty() || accessor.max.empty()) && !values.empty())
    {
        auto minmax = glTFToolKit::AccessorUtils::CalculateMinMax(accessor, values);
        accessor.min = minmax.first;
        accessor.max = minmax.second;
    }

    for (draco::PointIndex i(0); i < static_cast<uint32_t>(accessor.count); ++i)
    {
        attrActual->SetAttributeValue(attrActual->mapped_index(i), &values[i.value() * numComponents]);
    }
    if (dracoMesh.num_points() == 0) 
    {
        dracoMesh.set_num_points(static_cast<unsigned int>(accessor.count));
    }
    else if (dracoMesh.num_points() != accessor.count)
    {
        throw GLTFException("Inconsistent points count.");
    }

    return attId;
}

void SetEncoderOptions (draco::Encoder& encoder, const glTFToolKit::CompressionOptions& options)
{
    encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, options.PositionQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, options.TexCoordQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, options.NormalQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, options.ColorQuantizationBits);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, options.GenericQuantizationBits);
    encoder.SetSpeedOptions(options.Speed, options.Speed);
    encoder.SetTrackEncodedProperties(true);
}

}		// namespace.

Document glTFToolKit::GLTFMeshCompressionUtils::CompressMesh (
	GLBResourceReader* glbReader, 
    std::shared_ptr<IStreamReader> streamReader,
    const Document & doc,
    CompressionOptions options,
    const Mesh & mesh,
    BufferBuilder* builder,
    std::unordered_set<std::string>& bufferViewsToRemove)
{
    GLTFResourceReader reader(streamReader);
    Document resultDocument(doc);
    draco::Encoder encoder;
    SetEncoderOptions(encoder, options);

    Mesh resultMesh(mesh);
    resultMesh.primitives.clear();
    for (const auto& primitive : mesh.primitives)
    {
        if (primitive.HasExtension<KHR::MeshPrimitives::DracoMeshCompression>())
        {
            resultMesh.primitives.emplace_back(primitive);
            continue;
        }
        auto dracoExtension = std::make_unique<KHR::MeshPrimitives::DracoMeshCompression>();
        draco::Mesh dracoMesh;
        Accessor indiciesAccessor(doc.accessors[primitive.indicesAccessorId]);
        std::vector<uint32_t> indices;
		if (glbReader) {
			if (indiciesAccessor.componentType == COMPONENT_UNSIGNED_SHORT) {
				auto indices2 = glbReader->ReadBinaryData<uint16_t>(doc, indiciesAccessor);
				indices.resize(indices2.size());
				for (size_t i = 0; i < indices.size(); ++i) indices[i] = (uint32_t)indices2[i];

			} else if (indiciesAccessor.componentType == COMPONENT_SHORT) {
				auto indices2 = glbReader->ReadBinaryData<int16_t>(doc, indiciesAccessor);
				indices.resize(indices2.size());
				for (size_t i = 0; i < indices.size(); ++i) indices[i] = (uint32_t)indices2[i];

			} else if (indiciesAccessor.componentType == COMPONENT_UNSIGNED_INT) {
				indices = glbReader->ReadBinaryData<uint32_t>(doc, indiciesAccessor);
			}

		} else {
			indices = MeshPrimitiveUtils::GetIndices32(doc, reader, primitive);
		}
        size_t numFaces = indices.size() / 3;
        dracoMesh.SetNumFaces(numFaces);
		draco::Mesh::Face face;
        for (uint32_t i = 0; i < numFaces; i++)
        {
            face[0] = indices[(i * 3) + 0];
            face[1] = indices[(i * 3) + 1];
            face[2] = indices[(i * 3) + 2];
            dracoMesh.SetFace(draco::FaceIndex(i), face);
        }

        bufferViewsToRemove.emplace(indiciesAccessor.bufferViewId);
        indiciesAccessor.bufferViewId = "";
        indiciesAccessor.byteOffset = 0;
        resultDocument.accessors.Replace(indiciesAccessor);

        for (const auto& attribute : primitive.attributes)
        {
            const auto& accessor = doc.accessors[attribute.second];
            Accessor attributeAccessor(accessor);
            int attId;
            switch (accessor.componentType)
            {
            case COMPONENT_BYTE:           attId = InitializePointAttribute<int8_t>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_BYTE:  attId = InitializePointAttribute<uint8_t>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            case COMPONENT_SHORT:          attId = InitializePointAttribute<int16_t>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_SHORT: attId = InitializePointAttribute<uint16_t>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_INT:   attId = InitializePointAttribute<uint32_t>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            case COMPONENT_FLOAT:          attId = InitializePointAttribute<float>(dracoMesh, attribute.first, doc, glbReader, reader, attributeAccessor); break;
            default: throw GLTFException("Unknown component type.");
            }
            
            bufferViewsToRemove.emplace(accessor.bufferViewId);
            attributeAccessor.bufferViewId = "";
            attributeAccessor.byteOffset = 0;
            resultDocument.accessors.Replace(attributeAccessor);

            dracoExtension->attributes.emplace(attribute.first, dracoMesh.attribute(attId)->unique_id());
        }
        if (primitive.targets.size() > 0)
        {
            // Set sequential encoding to preserve order of vertices.
            encoder.SetEncodingMethod(draco::MESH_SEQUENTIAL_ENCODING);
        }

        dracoMesh.DeduplicateAttributeValues();
        dracoMesh.DeduplicatePointIds();
        draco::EncoderBuffer buffer;
        const draco::Status status = encoder.EncodeMeshToBuffer(dracoMesh, &buffer);
        if (!status.ok()) {
            throw GLTFException(std::string("Failed to encode the mesh: ") + status.error_msg());
        }

        // We must update the original accessors to the encoding out values.
        Accessor encodedIndexAccessor(resultDocument.accessors[primitive.indicesAccessorId]);
        encodedIndexAccessor.count = encoder.num_encoded_faces() * 3;
        resultDocument.accessors.Replace(encodedIndexAccessor);

        for (const auto& dracoAttribute : dracoExtension->attributes)
        {
            auto accessorId = primitive.attributes.at(dracoAttribute.first);
            Accessor encodedAccessor(resultDocument.accessors[accessorId]);
            encodedAccessor.count = encoder.num_encoded_points();
            resultDocument.accessors.Replace(encodedAccessor);
        }

		// 4 byte alignment.
		std::vector<uint8_t> dataBuff;
		size_t dCou = 0;
		{
			dCou = buffer.size();
			dataBuff.resize(dCou + 4, 0);
			for (size_t i = 0; i < dCou; ++i) dataBuff[i] = buffer.data()[i];
			if (dCou & 3) dCou += 4 - (dCou & 3);
		}

        // Finally put the encoded data in place.
        auto bufferView = builder->AddBufferView(&(dataBuff[0]), dCou);
        dracoExtension->bufferViewId = bufferView.id;
        MeshPrimitive resultPrim(primitive);
        resultPrim.SetExtension(std::move(dracoExtension));
		resultMesh.primitives.emplace_back(resultPrim);
    }
    resultDocument.meshes.Replace(resultMesh);

    return resultDocument;
}

/**
 * 指定のaccessorIDの情報を取得し、builderに移し替え。この際に、accessorで参照しているbufferViewのIDを入れ替えることになる.
 *  float型のみの対応.
 * @param[in]     glbReader            オリジナルのglbリソース情報の読み込み用.
 * @param[in]     reader               オリジナルのbinリソース情報の読み込み用.
 * @param[in/out] doc                  glTF document.
 * @param[in]     accessorIDStr        対象のaccessorID.
 * @param[out]    builder              バッファ情報の格納クラス.
 * @param[in]     bufferViewsToRemove  削除するBufferViewのID.
 */
void restoreBuffersFloat (GLBResourceReader* glbReader, GLTFResourceReader& reader, Document& doc, const std::string& accessorIDStr, BufferBuilder* builder) {
	if (accessorIDStr == "") return;

	const int accessorID = std::stoi(accessorIDStr);
	const Accessor& accessor = doc.accessors[accessorID];
	if (accessor.bufferViewId == "") return;
	const std::vector<float> values = glbReader ? (glbReader->ReadBinaryData<float>(doc, (doc, accessor))) : reader.ReadBinaryData<float>(doc, accessor);
	const auto stride = sizeof(float) * Accessor::GetTypeCount(accessor.type);
	const auto bufferView = builder->AddBufferView(values, stride);
	Accessor accessor2(accessor);
	accessor2.bufferViewId = bufferView.id;
	doc.accessors.Replace(accessor2);
}

/**
 * bufferBuilderに圧縮しない情報を格納し、accessorのIDを置き換え.
 * @param[in]     glbReader            オリジナルのglbリソース情報の読み込み用.
 * @param[in]     streamReader         オリジナルのbinリソース情報の読み込み用.
 * @param[in/out] doc                  glTF document.
 * @param[out]    builder              バッファ情報の格納クラス.
 */
void adjustmentBuffers (GLBResourceReader* glbReader, std::shared_ptr<IStreamReader> streamReader, Document& doc, BufferBuilder* builder) {
    GLTFResourceReader reader(streamReader);

	// draco(2.2)で圧縮対象にならないものが参照しているaccessorでのbufferViewIDがある場合、.
	// 参照しているbufferViewIDの更新とbufferBuilderへの情報追加を行う.
	// images / skins / animations / primitivesのMorph Targetsが対象.

	// images (glbの場合はbufferViewに画像が格納されている。gltfの場合は外部ファイルなので不要).
	if (glbReader) {
		const size_t imagesCou = doc.images.Size();
		for (size_t i = 0; i < imagesCou; ++i) {
			const Image& image = doc.images[i];
			if (image.bufferViewId != "") {
				std::vector<uint8_t> values = glbReader->ReadBinaryData(doc, image);
				// 4 byte alignment.
				if (values.size() & 3) {
					const int cou = 4 - (int)(values.size() & 3);
					for (int j = 0; j < cou; ++j) values.push_back(0);
				}

				const auto bufferView2 = builder->AddBufferView(values);
				Image image2(image);
				image2.bufferViewId = bufferView2.id;
				doc.images.Replace(image2);
			}
		}
	}

	// Skins.
	{
		const size_t skinsCou = doc.skins.Size();
		for (size_t i = 0; i < skinsCou; ++i) {
			const Skin& skin = doc.skins[i];
			restoreBuffersFloat(glbReader, reader, doc, skin.inverseBindMatricesAccessorId, builder);
		}
	}

	// Mesh - Primitive - Target.
	{
		const size_t meshesCou = doc.meshes.Size();
		for (size_t mLoop = 0; mLoop < meshesCou; ++mLoop) {
			const Mesh& srcMesh = doc.meshes[mLoop];
			const size_t primCou = srcMesh.primitives.size();
			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				const MeshPrimitive& srcPrimitive = srcMesh.primitives[primLoop];
				const size_t targetsCou = srcPrimitive.targets.size();
				for (size_t i = 0; i < targetsCou; ++i) {
					const MorphTarget& morphTarget = srcPrimitive.targets[i];
					restoreBuffersFloat(glbReader, reader, doc, morphTarget.positionsAccessorId, builder);
					restoreBuffersFloat(glbReader, reader, doc, morphTarget.tangentsAccessorId, builder);
					restoreBuffersFloat(glbReader, reader, doc, morphTarget.normalsAccessorId, builder);
				}
			}
		}
	}

	// Animations.
	{
		const size_t animCou = doc.animations.Size();
		for (size_t aLoop = 0; aLoop < animCou; ++aLoop) {
			const Animation& animD = doc.animations[aLoop];
			const size_t sampCou = animD.samplers.Size();
			for (size_t i = 0; i < sampCou; ++i) {
				const AnimationSampler& sampD = animD.samplers[i];
				restoreBuffersFloat(glbReader, reader, doc, sampD.inputAccessorId, builder);
				restoreBuffersFloat(glbReader, reader, doc, sampD.outputAccessorId, builder);
			}
		}
	}
}
	 
Document glTFToolKit::GLTFMeshCompressionUtils::CompressMeshes (Microsoft::glTF::GLBResourceReader* glbReader, std::shared_ptr<IStreamReader> streamReader, const Document& doc, CompressionOptions options,
	const std::string& outputDirectory, const bool isGLB,
	std::shared_ptr<Microsoft::glTF::BufferBuilder>& bufferBuilder)
{
    Document resultDocument(doc);
	if (doc.buffers.Size() == 0) return resultDocument;

	// オリジナルのbinファイル名 (gltfファイルの場合).
	const std::string orgBinFileName = doc.buffers[0].uri;

	// 出力用のbinファイル名.
	const std::string tmpBinFileName = orgBinFileName + std::string("_tmp_bin");

	// xxx.binファイルの出力用.
	if (!bufferBuilder) {
		auto binStreamWriter = std::make_shared<const BinStreamWriter>(outputDirectory, tmpBinFileName);
		auto binWriter = std::make_unique<GLTFResourceWriter>(binStreamWriter);
		auto builder = std::make_unique<BufferBuilder>(std::move(binWriter));
		builder->AddBuffer(GLB_BUFFER_ID);
		bufferBuilder.reset(new BufferBuilder(std::move(*builder)));
	}

	// bufferBuilderに圧縮しない情報を格納し、accessor内のbufferViewIDを置き換え.
	adjustmentBuffers(glbReader, streamReader, resultDocument, bufferBuilder.get());

	// メッシュ情報を圧縮し、bufferBuilderに格納.
	std::unordered_set<std::string> bufferViewsToRemove;
	for (const auto& mesh : doc.meshes.Elements()) {
        resultDocument = CompressMesh(glbReader, streamReader, resultDocument, options, mesh, bufferBuilder.get(), bufferViewsToRemove);
    }

	// bufferBuilder内のbufferViews/buffersの情報を、Documentに反映.
	resultDocument.bufferViews.Clear();
	resultDocument.buffers.Clear();
	bufferBuilder->Output(resultDocument);

	if (!isGLB) {
		// 名前変更などのファイル操作を有効にするために、解放.
		bufferBuilder.reset();

		// binファイル名を変更 (元の名前に入れ替え).
		try {
#if _WINDOWS
			const std::string fileSep("\\");
#else
			const std::string fileSep("/");
#endif

			const std::string srcFileName = outputDirectory + fileSep + orgBinFileName;
			const std::string dstFileName = outputDirectory + fileSep + tmpBinFileName;
#if _WINDOWS
			if (_access_s(srcFileName.c_str(), 0) == 0) {
#else
			if (access(srcFileName.c_str(), F_OK) == 0) {
#endif
				std::remove(srcFileName.c_str());
			}

			struct stat buf;
			if (stat(srcFileName.c_str(), &buf) != 0) {
				std::rename(dstFileName.c_str(), srcFileName.c_str());
			}
		} catch (...) { }

		const Buffer& buffer = resultDocument.buffers[0];
		Buffer buffer2(buffer);
		buffer2.uri = orgBinFileName;
		resultDocument.buffers.Replace(buffer2);
	}

	resultDocument.extensionsUsed.emplace(KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME);
    resultDocument.extensionsRequired.emplace(KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME);

    return resultDocument;
}

/**
 * 指定のgltf/glbファイルを読み込み、Draco圧縮を行う.
 */
bool glTFToolKit::GLTFMeshCompressionUtils::doDracoCompress (const std::string gltfFileName)
{
	const std::string fileDir2  = StringUtil::getFileDir(gltfFileName);
	const std::string fileName2 = StringUtil::getFileName(gltfFileName);

	// fileNameがglb(vrm)ファイルかどうか.
	const std::string fileExtStr = StringUtil::getFileExtension(gltfFileName);
	const bool glbFile = (fileExtStr == std::string("glb") || fileExtStr == std::string("vrm"));

	Document gltfDoc;
	Document compressDoc;
	std::shared_ptr<Microsoft::glTF::BufferBuilder> dstBufferBuilder;
	{
		// gltfファイルを読み込み.
		std::shared_ptr<GLBResourceReader> glbReader;

		std::string errStr = "";
		try {
			std::string jsonStr = "";

			if (glbFile) {
				// glb/vrmファイルを読み込み.
				auto glbStream = std::make_shared<std::ifstream>(gltfFileName, std::ios::binary);
				std::shared_ptr<BinStreamReader> binStreamReader;
				binStreamReader.reset(new BinStreamReader(""));
				glbReader.reset(new GLBResourceReader(binStreamReader, glbStream));

				// glbファイルからjson部を取得.
				jsonStr = glbReader->GetJson();

			} else {
				// gltfファイルを読み込み.
				std::ifstream gltfStream(gltfFileName);
				if (!gltfStream) return false;

				// json部を取得.
				jsonStr = "";
				{
					std::string str;
					while (!gltfStream.eof()) {
						std::getline(gltfStream, str);
						jsonStr += str + std::string("\n");
					}
				}
			}
			if (jsonStr == "") return false;

			// jsonデータをパース.
			gltfDoc = Deserialize(jsonStr);

		} catch (GLTFException e) {
			errStr = std::string(e.what());
			return false;
		} catch (...) {
			errStr = std::string("glb file could not be loaded.");
			return false;
		}
		if (gltfDoc.buffers.Size() == 0) return false;

		if (glbFile) {
			auto binStreamWriter = std::make_shared<const BinStreamWriter>(fileDir2, fileName2);
			auto glbWriter = std::make_unique<GLBResourceWriter>(binStreamWriter);
			dstBufferBuilder = std::make_unique<BufferBuilder>(std::move(glbWriter));
			dstBufferBuilder->AddBuffer(GLB_BUFFER_ID);
		}

		// Draco圧縮を行い、再度ファイル出力.
		glTFToolKit::CompressionOptions compressOptions;
		std::shared_ptr<BinStreamReader> binStreamReader;
		binStreamReader.reset(new BinStreamReader(fileDir2));

		compressDoc = CompressMeshes(glbReader.get(), binStreamReader, gltfDoc, compressOptions, fileDir2, glbFile, dstBufferBuilder);
	}

	if (glbFile) {
		// glbファイルを出力.
		const auto extensionSerializer = KHR::GetKHRExtensionSerializer();
		auto manifest     = Serialize(compressDoc, extensionSerializer);
		auto outputWriter = dynamic_cast<GLBResourceWriter *>(&dstBufferBuilder->GetResourceWriter());
		if (outputWriter) outputWriter->Flush(manifest, gltfFileName);

	} else {
		// gltfファイルを出力.
		const auto extensionSerializer = KHR::GetKHRExtensionSerializer();
		std::string gltfJson = Serialize(compressDoc, extensionSerializer, SerializeFlags::Pretty);
		std::ofstream outStream(gltfFileName.c_str(), std::ios::trunc | std::ios::out);
		outStream << gltfJson;
		outStream.flush();
	}

	return true;
}
