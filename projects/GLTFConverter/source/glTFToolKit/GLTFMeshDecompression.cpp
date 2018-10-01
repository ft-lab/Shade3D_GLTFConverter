/**
 * gltf/glbのDraco圧縮された構造を展開.
 */
#include "GLTFMeshDecompression.h"
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
#include "draco/compression/decode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/mesh_io.h"
#include "draco/io/point_cloud_io.h"
#include "draco/mesh/mesh.h"
#pragma warning(pop)

#include "../StringUtil.h"

using namespace Microsoft::glTF;

//----------------------------------------------------------.
glTFToolKit::DecompressMeshData::DecompressMeshData ()
{
	clear();
}
glTFToolKit::DecompressMeshData::~DecompressMeshData ()
{
}
glTFToolKit::DecompressMeshData::DecompressMeshData (const glTFToolKit::DecompressMeshData& v)
{
	this->meshIndex = v.meshIndex;
	this->primitiveIndex = v.primitiveIndex;
	this->pointsCou = v.pointsCou;
	this->indices  = v.indices;
	this->vertices = v.vertices;
	this->normals  = v.normals;
	this->uvs0     = v.uvs0;
	this->uvs1     = v.uvs1;
	this->colors0  = v.colors0;
	this->joints   = v.joints;
	this->weights  = v.weights;
	this->tangents = v.tangents;
}

void glTFToolKit::DecompressMeshData::clear()
{
	meshIndex = -1;
	primitiveIndex = -1;
	pointsCou = 0;
	indices.clear();
	vertices.clear();
	normals.clear();
	uvs0.clear();
	uvs1.clear();
	colors0.clear();
	joints.clear();
	weights.clear();
	tangents.clear();
}

//----------------------------------------------------------.
/**
 * meshDataListのうち、指定のmeshIndex - primitiveIndexの要素番号を取得.
 */
int glTFToolKit::findMeshPrimitiveIndex (const std::vector<glTFToolKit::DecompressMeshData>& meshDataList, const int meshIndex, const int primitiveIndex)
{
	int index = -1;
	for(size_t i = 0; i < meshDataList.size(); ++i) {
		const glTFToolKit::DecompressMeshData& meshD = meshDataList[i];
		if (meshD.meshIndex == meshIndex && meshD.primitiveIndex == primitiveIndex) {
			index = (int)i;
			break;
		}
	}
	return index;
}

//----------------------------------------------------------.
namespace {
	/**
	 * メッシュ情報を展開.
	 */
	bool decompressMeshes (GLBResourceReader* glbReader, std::shared_ptr<IStreamReader> streamReader, Document& doc, std::vector<glTFToolKit::DecompressMeshData>& meshDataList) {
		meshDataList.clear();
		GLTFResourceReader reader(streamReader);

		const size_t meshesCou = doc.meshes.Size();
		for (size_t mLoop = 0; mLoop < meshesCou; ++mLoop) {
			const Mesh& srcMesh = doc.meshes[mLoop];
			const size_t primCou = srcMesh.primitives.size();
			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				const MeshPrimitive& srcPrimitive = srcMesh.primitives[primLoop];
				if (!srcPrimitive.HasExtension<KHR::MeshPrimitives::DracoMeshCompression>()) continue;
				const KHR::MeshPrimitives::DracoMeshCompression& dracoMeshComp = srcPrimitive.GetExtension<KHR::MeshPrimitives::DracoMeshCompression>();
				if (dracoMeshComp.bufferViewId == "") continue;

				// glTFとしてのprimitive - attributesのkeyリスト(POSITION/NORMAL/TEXCOORD_0 など)を取得.
				// 参照IDの小さい順に並び替える.
				std::vector<std::string> attrKeyNames;
				{
					std::vector<int> attrIDList;
					for (auto attribute = srcPrimitive.attributes.begin(); attribute != srcPrimitive.attributes.end(); ++attribute) {
						const std::string v1 = attribute->first;
						const std::string v2 = attribute->second;
						attrKeyNames.push_back(v1);
						attrIDList.push_back(std::stoi(v2));
					}
					const size_t sCou = attrKeyNames.size();
					for(size_t i = 0; i < sCou; ++i) {
						for(size_t j = i + 1; j < sCou; ++j) {
							if (attrIDList[i] > attrIDList[j]) {
								std::swap(attrKeyNames[i], attrKeyNames[j]);
								std::swap(attrIDList[i], attrIDList[j]);
							}
						}
					}
				}

				// bufferViewからバイナリ情報を取得.
				const int bufferViewID = std::stoi(dracoMeshComp.bufferViewId);
				const BufferView& bufferView = doc.bufferViews[bufferViewID];
				std::vector<uint8_t> data = (glbReader) ? (glbReader->ReadBinaryData<uint8_t>(doc, bufferView)) : (reader.ReadBinaryData<uint8_t>(doc, bufferView));

				draco::Decoder decoder;
				draco::DecoderBuffer buffer;
				buffer.Init((char *)&(data[0]), data.size());
				const draco::EncodedGeometryType geom_type = draco::Decoder::GetEncodedGeometryType(&buffer).value();
				if (geom_type != draco::TRIANGULAR_MESH) continue;

				// メッシュ情報を展開.
				auto meshBuffer = decoder.DecodeMeshFromBuffer(&buffer);
				if (!meshBuffer.ok()) return false;
				std::unique_ptr<draco::Mesh> in_mesh = std::move(meshBuffer).value();

				std::unique_ptr<draco::PointCloud> pc;
				draco::Mesh *mesh = NULL;
				if (in_mesh) {
					mesh = in_mesh.get();
					pc = std::move(in_mesh);
				}
				if (pc == NULL) return false;

				// 展開されたメッシュ情報をmeshDataListに格納.
				meshDataList.push_back(glTFToolKit::DecompressMeshData());
				glTFToolKit::DecompressMeshData& dstMeshData = meshDataList.back();
				dstMeshData.meshIndex      = (int)mLoop;
				dstMeshData.primitiveIndex = (int)primLoop;

				const int facesCou = mesh->num_faces();
				const int attrCou  = mesh->num_attributes();
				if (attrCou != attrKeyNames.size()) continue;

				dstMeshData.indices.resize(facesCou * 3);
				draco::Mesh::Face face;
				for (int i = 0, iPos = 0; i < facesCou; ++i, iPos += 3) {
					face = mesh->face(draco::FaceIndex(i));
					dstMeshData.indices[iPos + 0] = face[0].value();
					dstMeshData.indices[iPos + 1] = face[1].value();
					dstMeshData.indices[iPos + 2] = face[2].value();
				}

				const int pointsCou = mesh->num_points();
				dstMeshData.pointsCou = pointsCou;

				bool errF = false;
				for (int aLoop = 0; aLoop < attrCou; ++aLoop) {
					const draco::PointAttribute* attr = mesh->GetAttributeByUniqueId(aLoop);
					if (attr == NULL || (attr->size() == 0)) continue;

					const draco::GeometryAttribute::Type type = attr->attribute_type();
					const draco::DataType dataType = attr->data_type();
					const size_t size = attr->size();
					const std::string& attrKeyName = attrKeyNames[aLoop];

					const size_t indicesMapSize = attr->indices_map_size();

					if (type == draco::GeometryAttribute::Type::POSITION) {
						if (dataType != draco::DataType::DT_FLOAT32 || attrKeyName != ACCESSOR_POSITION) {
							errF = true;
							break;
						}
						dstMeshData.vertices.resize(indicesMapSize * 3);
						std::array<float, 3> value;
						for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 3) {
							const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
							if (!attr->ConvertValue<float, 3>(index, &value[0])) continue;
							dstMeshData.vertices[iPos + 0] = value[0];
							dstMeshData.vertices[iPos + 1] = value[1];
							dstMeshData.vertices[iPos + 2] = value[2];
						}
						continue;
					}
					if (type == draco::GeometryAttribute::Type::NORMAL) {
						if (dataType != draco::DataType::DT_FLOAT32 || attrKeyName != ACCESSOR_NORMAL) {
							errF = true;
							break;
						}
						dstMeshData.normals.resize(indicesMapSize * 3);
						std::array<float, 3> value;
						for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 3) {
							const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
							if (!attr->ConvertValue<float, 3>(index, &value[0])) continue;
							dstMeshData.normals[iPos + 0] = value[0];
							dstMeshData.normals[iPos + 1] = value[1];
							dstMeshData.normals[iPos + 2] = value[2];
						}
						continue;
					}
					if (type == draco::GeometryAttribute::Type::TEX_COORD) {
						if (dataType != draco::DataType::DT_FLOAT32 || (attrKeyName != ACCESSOR_TEXCOORD_0 && attrKeyName != ACCESSOR_TEXCOORD_1)) {
							errF = true;
							break;
						}

						if (attrKeyName == ACCESSOR_TEXCOORD_0) {
							dstMeshData.uvs0.resize(indicesMapSize * 2);
							std::array<float, 2> value;
							for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 2) {
								const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
								if (!attr->ConvertValue<float, 2>(index, &value[0])) continue;
								dstMeshData.uvs0[iPos + 0] = value[0];
								dstMeshData.uvs0[iPos + 1] = value[1];
							}
						}
						if (attrKeyName == ACCESSOR_TEXCOORD_1) {
							dstMeshData.uvs1.resize(indicesMapSize * 2);
							std::array<float, 2> value;
							for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 2) {
								const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
								if (!attr->ConvertValue<float, 2>(index, &value[0])) continue;
								dstMeshData.uvs1[iPos + 0] = value[0];
								dstMeshData.uvs1[iPos + 1] = value[1];
							}
						}
						continue;
					}
					if (type == draco::GeometryAttribute::Type::COLOR) {
						if ((dataType != draco::DataType::DT_FLOAT32 && dataType != draco::DataType::DT_UINT8) || attrKeyName != ACCESSOR_COLOR_0) {
							errF = true;
							break;
						}
						
						if (dataType == draco::DataType::DT_UINT8) {
							dstMeshData.colors0.resize(indicesMapSize * 4);
							std::array<uint8_t, 4> value;
							for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
								const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
								if (!attr->ConvertValue<uint8_t, 4>(index, &value[0])) continue;
								dstMeshData.colors0[iPos + 0] = (float)value[0] / 255.0f;
								dstMeshData.colors0[iPos + 1] = (float)value[1] / 255.0f;
								dstMeshData.colors0[iPos + 2] = (float)value[2] / 255.0f;
								dstMeshData.colors0[iPos + 3] = (float)value[3] / 255.0f;
							}
						} else if (dataType == draco::DataType::DT_FLOAT32) {
							dstMeshData.colors0.resize(indicesMapSize * 4);
							std::array<float, 4> value;
							for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
								const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
								if (!attr->ConvertValue<float, 4>(index, &value[0])) continue;
								dstMeshData.colors0[iPos + 0] = value[0];
								dstMeshData.colors0[iPos + 1] = value[1];
								dstMeshData.colors0[iPos + 2] = value[2];
								dstMeshData.colors0[iPos + 3] = value[3];
							}
						}
						continue;
					}

					// JOINTS_0 / WEIGHTS_0 / TANGENT.
					if (type == draco::GeometryAttribute::Type::GENERIC) {
						if (attrKeyName != ACCESSOR_JOINTS_0 && attrKeyName != ACCESSOR_WEIGHTS_0 && attrKeyName != ACCESSOR_TANGENT) {
							errF = true;
							break;
						}
						if (attrKeyName == ACCESSOR_JOINTS_0) {
							if (dataType == draco::DataType::DT_UINT16) {
								dstMeshData.joints.resize(indicesMapSize * 4);
								std::array<unsigned short, 4> value;
								for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
									const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
									if (!attr->ConvertValue<unsigned short, 4>(index, &value[0])) continue;
									dstMeshData.joints[iPos + 0] = (int)value[0];
									dstMeshData.joints[iPos + 1] = (int)value[1];
									dstMeshData.joints[iPos + 2] = (int)value[2];
									dstMeshData.joints[iPos + 3] = (int)value[3];
								}
							} else if (dataType == draco::DataType::DT_UINT32) {
								dstMeshData.joints.resize(indicesMapSize * 4);
								std::array<int, 4> value;
								for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
									const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
									if (!attr->ConvertValue<int, 4>(index, &value[0])) continue;
									dstMeshData.joints[iPos + 0] = value[0];
									dstMeshData.joints[iPos + 1] = value[1];
									dstMeshData.joints[iPos + 2] = value[2];
									dstMeshData.joints[iPos + 3] = value[3];
								}
							} else if (dataType == draco::DataType::DT_FLOAT32) {
								dstMeshData.joints.resize(indicesMapSize * 4);
								std::array<float, 4> value;
								for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
									const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
									if (!attr->ConvertValue<float, 4>(index, &value[0])) continue;
									dstMeshData.joints[iPos + 0] = (int)value[0];
									dstMeshData.joints[iPos + 1] = (int)value[1];
									dstMeshData.joints[iPos + 2] = (int)value[2];
									dstMeshData.joints[iPos + 3] = (int)value[3];
								}
							}
						} else if (attrKeyName == ACCESSOR_WEIGHTS_0) {
							if (dataType == draco::DataType::DT_FLOAT32) {
								dstMeshData.weights.resize(indicesMapSize * 4);
								std::array<float, 4> value;
								for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
									const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
									if (!attr->ConvertValue<float, 4>(index, &value[0])) continue;
									dstMeshData.weights[iPos + 0] = value[0];
									dstMeshData.weights[iPos + 1] = value[1];
									dstMeshData.weights[iPos + 2] = value[2];
									dstMeshData.weights[iPos + 3] = value[3];
								}
							}
						} else if (attrKeyName == ACCESSOR_TANGENT) {
							if (dataType == draco::DataType::DT_FLOAT32) {
								dstMeshData.weights.resize(indicesMapSize * 3);
								std::array<float, 3> value;
								for (size_t i = 0, iPos = 0; i < indicesMapSize; ++i, iPos += 4) {
									const draco::AttributeValueIndex index = attr->mapped_index(draco::PointIndex(i));
									if (!attr->ConvertValue<float, 3>(index, &value[0])) continue;
									dstMeshData.tangents[iPos + 0] = value[0];
									dstMeshData.tangents[iPos + 1] = value[1];
									dstMeshData.tangents[iPos + 2] = value[2];
								}
							}
						}

						continue;
					}
				}
				if (errF) return false;
			}
		}
		return true;
	}
}

/**
 * 指定のgltf/glbファイルからDraco情報を展開.
 * @param[in]  gltfFileName  gltf/glbのファイル名.
 * @param[out] meshDataList  メッシュ情報が展開されて入る.

 */
bool glTFToolKit::GLTFMeshDecompressionUtils::doDracoDecompress (const std::string gltfFileName, std::vector<DecompressMeshData>& meshDataList)
{
	meshDataList.clear();
	const std::string fileDir2  = StringUtil::getFileDir(gltfFileName);
	const std::string fileName2 = StringUtil::getFileName(gltfFileName);

	// fileNameがglb(vrm)ファイルかどうか.
	const std::string fileExtStr = StringUtil::getFileExtension(gltfFileName);
	const bool glbFile = (fileExtStr == std::string("glb") || fileExtStr == std::string("vrm"));

	Document gltfDoc;
	{
		// gltfファイルを読み込み.
		std::shared_ptr<GLBResourceReader> glbReader;
		std::shared_ptr<BinStreamReader> binStreamReader;

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
			const auto extensionDeserializer = KHR::GetKHRExtensionDeserializer();
			gltfDoc = Deserialize(jsonStr, extensionDeserializer);

			if (gltfDoc.extensionsUsed.size() == 0) return false;
			auto extUsedV = gltfDoc.extensionsUsed.find(KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME);
			if (*extUsedV != KHR::MeshPrimitives::DRACOMESHCOMPRESSION_NAME) return false;

		} catch (GLTFException e) {
			errStr = std::string(e.what());
			return false;
		} catch (...) {
			errStr = std::string("glb file could not be loaded.");
			return false;
		}
		if (gltfDoc.buffers.Size() == 0) return false;

		if (!glbFile) {
			binStreamReader.reset(new BinStreamReader(fileDir2));
		}

		// Mesh情報を取得.
		if (!decompressMeshes(glbReader.get(), binStreamReader, gltfDoc, meshDataList)) return false;
	}

	return true;
}
