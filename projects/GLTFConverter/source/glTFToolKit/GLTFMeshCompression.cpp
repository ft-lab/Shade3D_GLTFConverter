/**
 * MeshのDraco圧縮.
 * https://github.com/Microsoft/glTF-Toolkit の GLTFMeshCompressionUtils より.
 * gltf/glbのどちらでもDraco圧縮できるように機能追加.
 */
#include "pch.h"
#include "GLTFMeshCompression.h"

#include "GLTFSDK/MeshPrimitiveUtils.h"
#include "GLTFSDK/ExtensionsKHR.h"
#include "GLTFSDK/BufferBuilder.h"
#include "AccessorUtils.h"

#pragma warning(push)
#pragma warning(disable: 4018 4081 4244 4267 4389)
#include "draco/draco_features.h"
#include "draco/compression/encode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#pragma warning(pop)

using namespace Microsoft::glTF;

std::wstring PathConcat (const std::wstring& part1, const std::wstring& part2)
{
    wchar_t uriAbsoluteRaw[MAX_PATH];
    // Note: PathCchCombine will return the last argument if it's an absolute path
    if (FAILED(::PathCchCombine(uriAbsoluteRaw, ARRAYSIZE(uriAbsoluteRaw), part1.c_str(), part2.c_str())))
    {
        auto msg = L"Could not combine the path names: " + part1 + L" and " + part2;
        throw std::invalid_argument(std::string(msg.begin(), msg.end()));
    }

    return uriAbsoluteRaw;
}

std::string PathConcat (const std::string& part1, const std::string& part2)
{
    std::wstring part1W = std::wstring(part1.begin(), part1.end());
    std::wstring part2W = std::wstring(part2.begin(), part2.end());

    auto pathW = PathConcat(part1W, part2W);
    return std::string(pathW.begin(), pathW.end());
}

class FilepathStreamWriter : public IStreamWriter
{
public:
    FilepathStreamWriter(std::string uriBase) : m_uriBase(uriBase) {}

    virtual ~FilepathStreamWriter() override {}
    virtual std::shared_ptr<std::ostream> GetOutputStream(const std::string& filename) const override
    {
        return std::make_shared<std::ofstream>(PathConcat(m_uriBase, filename), std::ios::binary);
    }
private:
    const std::string m_uriBase;
};

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
int InitializePointAttribute (draco::Mesh& dracoMesh, const std::string& attributeName, const Document& doc, GLTFResourceReader& reader, Accessor& accessor)
{
    auto stride = sizeof(T) * Accessor::GetTypeCount(accessor.type);
    auto numComponents = Accessor::GetTypeCount(accessor.type);
    draco::PointAttribute pointAttr;
    pointAttr.Init(GetTypeFromAttributeName(attributeName), nullptr, numComponents, GetDataType(accessor), accessor.normalized, stride, 0);
    int attId = dracoMesh.AddAttribute(pointAttr, true, static_cast<unsigned int>(accessor.count));
    auto attrActual = dracoMesh.attribute(attId);

    std::vector<T> values = reader.ReadBinaryData<T>(doc, accessor);

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

Document glTFToolKit::GLTFMeshCompressionUtils::CompressMesh (
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
        auto indices = MeshPrimitiveUtils::GetIndices32(doc, reader, primitive);
        size_t numFaces = indices.size() / 3;
        dracoMesh.SetNumFaces(numFaces);
        for (uint32_t i = 0; i < numFaces; i++)
        {
            draco::Mesh::Face face;
            face[0] = indices[(i * 3) + 0];
            face[1] = indices[(i * 3) + 1];
            face[2] = indices[(i * 3) + 2];
            dracoMesh.SetFace(draco::FaceIndex(i), face);
        }

        Accessor indiciesAccessor(doc.accessors[primitive.indicesAccessorId]);
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
            case COMPONENT_BYTE:           attId = InitializePointAttribute<int8_t>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_BYTE:  attId = InitializePointAttribute<uint8_t>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
            case COMPONENT_SHORT:          attId = InitializePointAttribute<int16_t>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_SHORT: attId = InitializePointAttribute<uint16_t>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
            case COMPONENT_UNSIGNED_INT:   attId = InitializePointAttribute<uint32_t>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
            case COMPONENT_FLOAT:          attId = InitializePointAttribute<float>(dracoMesh, attribute.first, doc, reader, attributeAccessor); break;
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

        // Finally put the encoded data in place.
        auto bufferView = builder->AddBufferView(buffer.data(), buffer.size());
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
 * @param[in]     reader               オリジナルのglTF情報の読み込み用.
 * @param[in/out] doc                  glTF document.
 * @param[in]     accessorIDStr        対象のaccessorID.
 * @param[out]    builder              バッファ情報の格納クラス.
 * @param[in]     bufferViewsToRemove  削除するBufferViewのID.
 */
void restoreBuffersFloat (GLTFResourceReader& reader, Document& doc, const std::string& accessorIDStr, BufferBuilder* builder) {
	if (accessorIDStr == "") return;

	const int accessorID = std::stoi(accessorIDStr);
	const Accessor& accessor = doc.accessors[accessorID];
	if (accessor.bufferViewId == "") return;
	const std::vector<float> values = reader.ReadBinaryData<float>(doc, accessor);
	const auto stride = sizeof(float) * Accessor::GetTypeCount(accessor.type);
	const auto bufferView = builder->AddBufferView(values, stride);
	Accessor accessor2(accessor);
	accessor2.bufferViewId = bufferView.id;
	doc.accessors.Replace(accessor2);
}

/**
 * bufferBuilderに圧縮しない情報を格納し、accessorのIDを置き換え.
 * @param[in]     streamReader         オリジナルのglTF情報の読み込み用.
 * @param[in/out] doc                  glTF document.
 * @param[out]    builder              バッファ情報の格納クラス.
 */
void adjustmentBuffers (std::shared_ptr<IStreamReader> streamReader, Document& doc, BufferBuilder* builder) {
    GLTFResourceReader reader(streamReader);

	// draco(2.2)で圧縮対象にならないものが参照しているaccessorでのbufferViewIDがある場合、.
	// 参照しているbufferViewIDの更新とbufferBuilderへの情報追加を行う.
	// skins / animations / primitivesのMorph Targetsが対象.

	// Skins.
	{
		const size_t skinsCou = doc.skins.Size();
		for (size_t i = 0; i < skinsCou; ++i) {
			const Skin& skin = doc.skins[i];
			restoreBuffersFloat(reader, doc, skin.inverseBindMatricesAccessorId, builder);
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
					restoreBuffersFloat(reader, doc, morphTarget.positionsAccessorId, builder);
					restoreBuffersFloat(reader, doc, morphTarget.tangentsAccessorId, builder);
					restoreBuffersFloat(reader, doc, morphTarget.normalsAccessorId, builder);
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
				restoreBuffersFloat(reader, doc, sampD.inputAccessorId, builder);
				restoreBuffersFloat(reader, doc, sampD.outputAccessorId, builder);
			}
		}
	}
}
	 
Document glTFToolKit::GLTFMeshCompressionUtils::CompressMeshes (std::shared_ptr<IStreamReader> streamReader, const Document& doc, CompressionOptions options,
	const std::string& outputDirectory, const bool isGLB,
	std::shared_ptr<Microsoft::glTF::BufferBuilder>& bufferBuilder)
{
    Document resultDocument(doc);
	if (doc.buffers.Size() == 0) return resultDocument;

	// オリジナルのbinファイル名 (glbのバイナリでも一時的に格納している).
	const std::string orgBinFileName = doc.buffers[0].uri;

	// xxx.binファイルの出力用.
	if (!bufferBuilder) {
		auto writerStream = std::make_shared<FilepathStreamWriter>(outputDirectory);
		auto writer = std::make_unique<GLTFResourceWriter>(writerStream);
		writer->SetUriPrefix(PathConcat(outputDirectory, "MeshCompression"));
		std::unique_ptr<BufferBuilder> builder = std::make_unique<BufferBuilder>(std::move(writer),
			[&doc](const BufferBuilder& builder) { return std::to_string(doc.buffers.Size() + builder.GetBufferCount()); },
			[&doc](const BufferBuilder& builder) { return std::to_string(doc.bufferViews.Size() + builder.GetBufferViewCount()); },
			[&doc](const BufferBuilder& builder) { return std::to_string(doc.accessors.Size() + builder.GetAccessorCount()); });
		auto buffer = builder->AddBuffer();
		bufferBuilder.reset(new BufferBuilder(std::move(*builder)));
	}

	std::unordered_set<std::string> bufferViewsToRemove;

	// bufferBuilderに圧縮しない情報を格納し、accessorのIDを置き換え.
	adjustmentBuffers(streamReader, resultDocument, bufferBuilder.get());

	// メッシュ情報を圧縮し、bufferBuilderに格納.
    for (const auto& mesh : doc.meshes.Elements()) {
        resultDocument = CompressMesh(streamReader, resultDocument, options, mesh, bufferBuilder.get(), bufferViewsToRemove);
    }

	// mesh - primitiveでbufferViewから不要な要素を削除.
	{
		const size_t bufferViewCou = doc.bufferViews.Size();
		for (const auto& bufferViewId : bufferViewsToRemove) {
			if (resultDocument.bufferViews.Has(bufferViewId)) {
				resultDocument.bufferViews.Remove(bufferViewId);
			}
		}
	}

	// bufferBuilder内のbufferViews/buffersの情報を、Documentに反映.
	resultDocument.bufferViews.Clear();
	resultDocument.buffers.Clear();
	bufferBuilder->Output(resultDocument);

    resultDocument.extensionsUsed.emplace(KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME);
    resultDocument.extensionsRequired.emplace(KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME);

    return resultDocument;
}
