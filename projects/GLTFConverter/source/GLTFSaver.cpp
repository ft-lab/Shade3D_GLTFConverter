/**
 * GLTFをファイルに保存する.
 */
#include "GLTFSaver.h"
#include "StringUtil.h"

#include "glTFToolKit/GLTFSDK.h"
#include "glTFToolKit/GLTFMeshCompression.h"

#include <GLTFSDK/Extension.h>
#include <GLTFSDK/ExtensionHandlers.h>
#include <GLTFSDK/ExtensionsKHR.h>
#include <GLTFSDK/GLBResourceWriter.h>
#include <GLTFSDK/IStreamWriter.h>
#include <GLTFSDK/BufferBuilder.h>

#include "BinStreamReaderWriter.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>

using namespace Microsoft::glTF;

// 以下はGLTFSDK/rapidjsonのincludeよりも後に指定しないとビルドエラーになる.
#include "SceneData.h"
#include "MathUtil.h"
#include "Shade3DArray.h"
#include "Shade3DUtil.h"

namespace {
	std::string g_errorMessage = "";		// エラーメッセージの保持用.

	/**
	 * ノード情報を指定.
	 */
	void setNodesData (Document& gltfDoc,  const CSceneData* sceneData) {
		if (sceneData->nodes.empty()) return;
		const float fMin = (float)(1e-4);

		std::vector<int> childNodeIndex;
		const size_t nodesCou = sceneData->nodes.size();
		for (size_t i = 0; i < nodesCou; ++i) {
			const CNodeData& nodeD = sceneData->nodes[i];
			Node gltfNode;
			gltfNode.name = nodeD.name;
			gltfNode.id   = std::to_string(i);

			// nodeDを親とする子ノードリストを取得.
			childNodeIndex.clear();
			for (size_t j = 0; j < nodesCou; ++j) {
				if (i == j) continue;
				const CNodeData& nodeD2 = sceneData->nodes[j];
				if (nodeD2.parentNodeIndex == i) {
					childNodeIndex.push_back(j);
					gltfNode.children.push_back(std::to_string(j));
				}
			}

			// glTFの仕様では、RTSに分解できる要素でないといけないので「せん断」は無視される。.
			if (!MathUtil::isZero(nodeD.translation, fMin)) {
				gltfNode.translation = Vector3(nodeD.translation.x, nodeD.translation.y, nodeD.translation.z);
			}
			if (!MathUtil::isZero(nodeD.scale - sxsdk::vec3(1, 1, 1), fMin)) {
				gltfNode.scale = Vector3(nodeD.scale.x, nodeD.scale.y, nodeD.scale.z);
			}
			if (!MathUtil::isZero(nodeD.rotation - sxsdk::quaternion_class::identity, fMin)) {
				gltfNode.rotation = Quaternion(nodeD.rotation.x, nodeD.rotation.y, nodeD.rotation.z, -nodeD.rotation.w);
			}

			// メッシュ情報を持つ場合.
			if (nodeD.meshIndex >= 0) gltfNode.meshId = std::to_string(nodeD.meshIndex);

			// スキン情報を持つ場合.
			if (nodeD.skinIndex >= 0) gltfNode.skinId = std::to_string(nodeD.skinIndex);

			try {
				gltfDoc.nodes.Append(gltfNode);
			} catch (GLTFException e) { }
		}

		// シーンのルートとして、0番目のノードを指定.
		gltfDoc.defaultSceneId = std::string("0");
	}

	/**
	 * assetのextrasの文字列を作成.
	 */
	std::string getAssetExtrasStr (const CSceneData* sceneData) {
		std::string str = "";

		if (sceneData->assetExtrasTitle != "") {
			str += std::string("\"title\": \"") + StringUtil::convHTMLEncode(sceneData->assetExtrasTitle) + "\"";
		}
		if (sceneData->assetExtrasAuthor != "") {
			if (str != "") str += std::string(",\n"); 
			str += std::string("\"author\": \"") + StringUtil::convHTMLEncode(sceneData->assetExtrasAuthor) + "\"";
		}
		if (sceneData->assetExtrasLicense != "") {
			if (str != "") str += std::string(",\n"); 
			str += std::string("\"license\": \"") + StringUtil::convHTMLEncode(sceneData->assetExtrasLicense) + "\"";
		}
		if (sceneData->assetExtrasSource != "") {
			if (str != "") str += std::string(",\n"); 
			str += std::string("\"source\": \"") + StringUtil::convHTMLEncode(sceneData->assetExtrasSource) + "\"";
		}
		if (str != "") str = std::string("{\n") + str + std::string("\n}");

		return str;
	}

	/**
	 * テクスチャのoffset/scaleの指定を文字列化.
	 */
	std::string getTextureTransformStr (const sxsdk::vec2& offset, const sxsdk::vec2& scale) {
		std::string str = "";

		if (!MathUtil::isZero(offset)) {
			str += "\"offset\": [" + std::to_string(offset.x) + std::string(",") + std::to_string(offset.y) + std::string("]\n");
		}
		if (str != "") str += std::string(",\n");
		if (!MathUtil::isZero(scale - sxsdk::vec2(1, 1))) {
			str += "\"scale\": [" + std::to_string(scale.x) + std::string(",") + std::to_string(scale.y) + std::string("]\n");
		}
		if (str == "") return "";

		str = std::string("{\n") + str + std::string("}\n");
		return str;
	}

	/**
	 * マテリアル情報を指定.
	 */
	void setMaterialsData (Document& gltfDoc,  const CSceneData* sceneData) {
		const size_t mCou = sceneData->materials.size();

		// テクスチャの繰り返しで (1, 1)でないものがあるかチェック.
		bool repeatTex = false;
		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];
			if (materialD.baseColorTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.emissiveTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.normalTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.metallicRoughnessTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.occlusionTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (repeatTex) break;
		}

		// unlitの指定があるかチェック.
		bool unlitMaterial = false;
		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];
			if (materialD.unlit) {
				unlitMaterial = true;
				break;
			}
		}

		// 拡張として使用する要素名を追加.
		if (repeatTex) {
			gltfDoc.extensionsUsed.insert("KHR_texture_transform");
		}
		if (unlitMaterial) {
			gltfDoc.extensionsUsed.insert("KHR_materials_unlit");
		}

		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];

			Material material;
			material.id          = std::to_string(i);
			material.name        = materialD.name;
			material.doubleSided = materialD.doubleSided;
			material.alphaMode   = ALPHA_OPAQUE;
			material.emissiveFactor = Color3(materialD.emissiveFactor.red, materialD.emissiveFactor.green, materialD.emissiveFactor.blue);

			material.metallicRoughness.baseColorFactor = Color4(materialD.baseColorFactor.red, materialD.baseColorFactor.green, materialD.baseColorFactor.blue, 1.0f);
			material.metallicRoughness.metallicFactor  = materialD.metallicFactor;
			material.metallicRoughness.roughnessFactor = materialD.roughnessFactor;

			// テクスチャを割り当て.
			if (materialD.baseColorImageIndex >= 0) {
				material.metallicRoughness.baseColorTexture.textureId = std::to_string(materialD.baseColorImageIndex);
				material.metallicRoughness.baseColorTexture.texCoord = (size_t)materialD.baseColorTexCoord;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.baseColorTexScale);
					if (str != "") material.metallicRoughness.baseColorTexture.extensions["KHR_texture_transform"] = str;
				}
			}

			if (materialD.emissiveImageIndex >= 0) {
				material.emissiveTexture.textureId =  std::to_string(materialD.emissiveImageIndex);
				material.emissiveTexture.texCoord  = (size_t)materialD.emissiveTexCoord;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.emissiveTexScale);
					if (str != "") material.emissiveTexture.extensions["KHR_texture_transform"] = str;
				}
			}

			if (materialD.normalImageIndex >= 0) {
				material.normalTexture.textureId = std::to_string(materialD.normalImageIndex);
				material.normalTexture.texCoord  = (size_t)materialD.normalTexCoord;
				material.normalTexture.scale     = materialD.normalStrength;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.normalTexScale);
					if (str != "") material.normalTexture.extensions["KHR_texture_transform"] = str;
				}
			}
			if (materialD.metallicRoughnessImageIndex >= 0) {
				material.metallicRoughness.metallicRoughnessTexture.textureId = std::to_string(materialD.metallicRoughnessImageIndex);
				material.metallicRoughness.metallicRoughnessTexture.texCoord  = (size_t)materialD.metallicRoughnessTexCoord;
			}

			if (materialD.occlusionImageIndex >= 0) {
				material.occlusionTexture.textureId = std::to_string(materialD.occlusionImageIndex);
				material.occlusionTexture.texCoord  = (size_t)materialD.occlusionTexCoord;
				material.occlusionTexture.strength  = materialD.occlusionStrength;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.occlusionTexScale);
					if (str != "") material.occlusionTexture.extensions["KHR_texture_transform"] = str;
				}
			}

			// アルファ合成のパラメータを渡す.
			if (materialD.alphaMode != ALPHA_OPAQUE) {
				if (materialD.alphaMode == ALPHA_BLEND) {
					material.alphaMode   = (AlphaMode)2;		// ALPHA_BLEND.

				} else {
					material.alphaMode   = (AlphaMode)3;		// ALPHA_MASK.
					material.alphaCutoff = materialD.alphaCutOff;
				}
				// 透明度をBaseColorのAlphaに反映.
				material.metallicRoughness.baseColorFactor.a = 1.0f - materialD.transparency;
			}

			// Unlitの指定.
			if (materialD.unlit) {
				material.emissiveFactor = Color3(0.0f, 0.0f, 0.0f);
				material.metallicRoughness.metallicFactor  = 0.0f;
				material.metallicRoughness.roughnessFactor = 1.0f;
				material.extensions["KHR_materials_unlit"] = "{ }";
			}

			gltfDoc.materials.Append(material);
		}
	}

	/**
	 * メッシュ情報を指定.
	 * @return accessorIDの数.
	 */
	int setMeshesData (Document& gltfDoc,  const CSceneData* sceneData) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return 0;

		// Mesh内のPrimitiveの頂点を共有するかどうか.
		bool shareVerticesMesh = sceneData->exportParam.shareVerticesMesh;

		int accessorID = 0;
		for (size_t meshLoop = 0; meshLoop < meshCou; ++meshLoop) {
			const CMeshData& meshD = sceneData->getMeshData(meshLoop);
			const size_t primCou = meshD.primitives.size();

			Mesh mesh;
			mesh.name = meshD.name;
			mesh.id   = std::to_string(meshLoop);

			std::vector<float> weights;		// Morph Targetsのウエイト値.

			// MeshPrimitiveに1つのメッシュの情報を格納。accessorのIDを指定していく.
			// indices                 : 三角形の頂点インデックス.
			// attributes - NORMAL     : 法線.
			// attributes - POSITION   : 頂点位置.
			// attributes - TEXCOORD_0 : テクスチャのUV0.
			// attributes - TEXCOORD_1 : テクスチャのUV0.
			// attributes - COLOR_0    : 頂点カラー.
			// attributes - JOINTS_0   : バインドされているジョイント.
			// attributes - WEIGHTS_0  : スキンのウエイト値.
			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				const CPrimitiveData& primitiveD = meshD.primitives[primLoop];

				MeshPrimitive meshPrimitive;
				if (primitiveD.materialIndex >= 0) {
					meshPrimitive.materialId = std::to_string(primitiveD.materialIndex);
				}

				meshPrimitive.indicesAccessorId = std::to_string(accessorID++);
				if (shareVerticesMesh && primLoop > 0) {	// Mesh内のPrimitiveの頂点情報を共有する.
					const CPrimitiveData& primitiveD0 = meshD.primitives[0];

					if (!primitiveD0.normals.empty()) {
						meshPrimitive.attributes[ACCESSOR_NORMAL]     = mesh.primitives[0].attributes[ACCESSOR_NORMAL];
					}
					if (!primitiveD0.vertices.empty()) {
						meshPrimitive.attributes[ACCESSOR_POSITION]   = mesh.primitives[0].attributes[ACCESSOR_POSITION];
					}
					if (!primitiveD0.uv0.empty()) {
						meshPrimitive.attributes[ACCESSOR_TEXCOORD_0] = mesh.primitives[0].attributes[ACCESSOR_TEXCOORD_0];
					}
					if (!primitiveD0.uv1.empty()) {
						meshPrimitive.attributes[ACCESSOR_TEXCOORD_1] = mesh.primitives[0].attributes[ACCESSOR_TEXCOORD_1];
					}
					if (!primitiveD0.color0.empty()) {
						meshPrimitive.attributes[ACCESSOR_COLOR_0]    = mesh.primitives[0].attributes[ACCESSOR_COLOR_0];
					}
					if (!primitiveD0.skinJoints.empty()) {
						meshPrimitive.attributes[ACCESSOR_JOINTS_0]   = mesh.primitives[0].attributes[ACCESSOR_JOINTS_0];
					}
					if (!primitiveD0.skinWeights.empty()) {
						meshPrimitive.attributes[ACCESSOR_WEIGHTS_0]  = mesh.primitives[0].attributes[ACCESSOR_WEIGHTS_0];
					}

				} else {
					meshPrimitive.attributes[ACCESSOR_NORMAL] = std::to_string(accessorID++);
					meshPrimitive.attributes[ACCESSOR_POSITION] = std::to_string(accessorID++);
					if (!primitiveD.uv0.empty()) {
						meshPrimitive.attributes[ACCESSOR_TEXCOORD_0] = std::to_string(accessorID++);
					}
					if (!primitiveD.uv1.empty()) {
						meshPrimitive.attributes[ACCESSOR_TEXCOORD_1] = std::to_string(accessorID++);

					}
					if (!primitiveD.color0.empty()) {
						meshPrimitive.attributes[ACCESSOR_COLOR_0] = std::to_string(accessorID++);

					}
					if (!primitiveD.skinJoints.empty()) {
						meshPrimitive.attributes[ACCESSOR_JOINTS_0] = std::to_string(accessorID++);
					}
					if (!primitiveD.skinWeights.empty()) {
						meshPrimitive.attributes[ACCESSOR_WEIGHTS_0] = std::to_string(accessorID++);
					}
				}

				if (!primitiveD.morphTargets.morphTargetsData.empty() || (shareVerticesMesh && primLoop > 0)) {
					if (shareVerticesMesh && primLoop > 0) {
						// 1番目のprimitive以降は0番目のtargetを複製.
						const CPrimitiveData& primitiveD0 = meshD.primitives[0];
						if (!primitiveD0.morphTargets.morphTargetsData.empty()) {
							const CMorphTargetsData& morphTargetsD0 = primitiveD0.morphTargets;
							for (size_t tLoop = 0; tLoop < morphTargetsD0.morphTargetsData.size(); ++tLoop) {
								meshPrimitive.targets.push_back(MorphTarget());
								MorphTarget& dstTarget = meshPrimitive.targets.back();
								dstTarget.positionsAccessorId = mesh.primitives[0].targets[tLoop].positionsAccessorId;
								dstTarget.normalsAccessorId   = mesh.primitives[0].targets[tLoop].normalsAccessorId;
							}
						}
					} else {
						const CMorphTargetsData& morphTargetsD = primitiveD.morphTargets;
						for (size_t tLoop = 0; tLoop < morphTargetsD.morphTargetsData.size(); ++tLoop) {
							const COneMorphTargetData& mTargetD = morphTargetsD.morphTargetsData[tLoop];
							meshPrimitive.targets.push_back(MorphTarget());
							MorphTarget& dstTarget = meshPrimitive.targets.back();
							dstTarget.positionsAccessorId = std::to_string(accessorID++);
							if (!mTargetD.normal.empty()) dstTarget.normalsAccessorId = std::to_string(accessorID++);
							//if (!mTargetD.tangent.empty()) dstTarget.tangentsAccessorId = std::to_string(accessorID++);

							// mesh内のすべてのprimitiveは、同一のMorph Targets数でウエイト値も同じという仕様.
							if (primLoop == 0) weights.push_back(mTargetD.weight);
						}
					}
				}

				meshPrimitive.mode = MESH_TRIANGLES;		// (4) 三角形情報として格納.

				mesh.primitives.push_back(meshPrimitive);
			}

			// Morph Targetsのデフォルトウエイト値.
			// 複数のPrimitiveがある場合は、すべてで同じTarget数でないといけない模様.
			if (!weights.empty()) mesh.weights = weights;

			// Mesh情報を追加.
			gltfDoc.meshes.Append(mesh);
		}

		return accessorID;
	}

	/**
	 *   GLB出力向けにバイナリ情報をbufferBuilderに格納.
	 */
	void setBufferData_to_BufferBuilder (Document& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return;

		// Mesh内のPrimitiveの頂点を共有するかどうか.
		bool shareVerticesMesh = sceneData->exportParam.shareVerticesMesh;

		int accessorID = 0;
		size_t byteOffset = 0;
		for (size_t meshLoop = 0; meshLoop < meshCou; ++meshLoop) {
			const CMeshData& meshD = sceneData->getMeshData(meshLoop);
			const size_t primCou = meshD.primitives.size();

			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				const CPrimitiveData& primitiveD = meshD.primitives[primLoop];

				// 頂点のバウンディングボックスを計算.
				sxsdk::vec3 bbMin, bbMax;
				primitiveD.calcBoundingBox(bbMin, bbMax);

				// indicesAccessor.
				{
					// short型で格納.
					const bool storeUShort = (primitiveD.vertices.size() < 65530);

					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_SCALAR;
					acceDesc.componentType = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * primitiveD.triangleIndices.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);

					if (storeUShort) {
						// AddAccessorではalignmentの詰め物を意識する必要なし.
						std::vector<unsigned short> shortData;
						shortData.resize(primitiveD.triangleIndices.size(), 0);
						for (size_t i = 0; i < primitiveD.triangleIndices.size(); ++i) {
							shortData[i] = (unsigned short)(primitiveD.triangleIndices[i]);
						}
						bufferBuilder->AddAccessor(shortData, acceDesc); 
					} else {
						bufferBuilder->AddAccessor(primitiveD.triangleIndices, acceDesc); 
					}

					byteOffset += byteLength;
					accessorID++;
				}

				// normalsAccessor.
				if (primLoop == 0 || !shareVerticesMesh) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC3;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 3) * primitiveD.normals.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(Shade3DArray::convert_vec3_to_float(primitiveD.normals), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// positionsAccessor.
				if (primLoop == 0 || !shareVerticesMesh) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC3;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;
					acceDesc.minValues.push_back(bbMin.x);
					acceDesc.minValues.push_back(bbMin.y);
					acceDesc.minValues.push_back(bbMin.z);
					acceDesc.maxValues.push_back(bbMax.x);
					acceDesc.maxValues.push_back(bbMax.y);
					acceDesc.maxValues.push_back(bbMax.z);

					const size_t byteLength = (sizeof(float) * 3) * primitiveD.vertices.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(Shade3DArray::convert_vec3_to_float(primitiveD.vertices), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// uv0Accessor.
				if (!primitiveD.uv0.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC2;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 2) * primitiveD.uv0.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(Shade3DArray::convert_vec2_to_float(primitiveD.uv0), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// uv1Accessor.
				if (!primitiveD.uv1.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC2;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 2) * primitiveD.uv1.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(Shade3DArray::convert_vec2_to_float(primitiveD.uv1), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// color0Accessor.
				if (!primitiveD.color0.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC4;
					acceDesc.componentType = COMPONENT_UNSIGNED_BYTE;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = true;

					size_t byteLength = (sizeof(unsigned char) * 4) * primitiveD.color0.size();
					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);

					// AddAccessorではalignmentの詰め物を意識する必要なし.
					const std::vector<unsigned char> buff = Shade3DArray::convert_vec4_to_uchar(primitiveD.color0, true);
					bufferBuilder->AddAccessor(buff, acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// SkinのJoints.
				if (!primitiveD.skinJoints.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					// short型で格納.
					const bool storeUShort = (primitiveD.skinJoints.size() < 65530);

					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC4;
					acceDesc.componentType = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * 4 * primitiveD.skinJoints.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);

					if (storeUShort) {
						// AddAccessorではalignmentの詰め物を意識する必要なし.
						std::vector<unsigned short> shortData;
						shortData.resize(primitiveD.skinJoints.size() * 4, 0);
						for (size_t i = 0, iPos = 0; i < primitiveD.skinJoints.size(); ++i, iPos += 4) {
							shortData[iPos + 0] = (unsigned short)(primitiveD.skinJoints[i][0]);
							shortData[iPos + 1] = (unsigned short)(primitiveD.skinJoints[i][1]);
							shortData[iPos + 2] = (unsigned short)(primitiveD.skinJoints[i][2]);
							shortData[iPos + 3] = (unsigned short)(primitiveD.skinJoints[i][3]);
						}
						bufferBuilder->AddAccessor(shortData, acceDesc); 
					} else {
						std::vector<unsigned int> intData;
						intData.resize(primitiveD.skinJoints.size() * 4, 0);
						for (size_t i = 0, iPos = 0; i < primitiveD.skinJoints.size(); ++i, iPos += 4) {
							intData[iPos + 0] = (unsigned int)(primitiveD.skinJoints[i][0]);
							intData[iPos + 1] = (unsigned int)(primitiveD.skinJoints[i][1]);
							intData[iPos + 2] = (unsigned int)(primitiveD.skinJoints[i][2]);
							intData[iPos + 3] = (unsigned int)(primitiveD.skinJoints[i][3]);
						}
						bufferBuilder->AddAccessor(intData, acceDesc); 
					}

					byteOffset += byteLength;
					accessorID++;
				}

				// SkinのWeights.
				if (!primitiveD.skinWeights.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC4;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 4) * primitiveD.skinWeights.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(Shade3DArray::convert_vec4_to_float(primitiveD.skinWeights), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// Morph Targets情報を格納.
				// Targetとして格納する頂点数は、primitiveの頂点数と同じである必要がある.
				if (!primitiveD.morphTargets.morphTargetsData.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					const size_t primVersCou = primitiveD.vertices.size();
					std::vector<sxsdk::vec3> vec3List, vec4List;
					vec3List.resize(primVersCou);
					vec4List.resize(primVersCou);
					const CMorphTargetsData& morphTargetsD = primitiveD.morphTargets;
					for (size_t tLoop = 0; tLoop < morphTargetsD.morphTargetsData.size(); ++tLoop) {
						const COneMorphTargetData& targetD = morphTargetsD.morphTargetsData[tLoop];
						const size_t tvCou = targetD.vIndices.size();

						// 頂点座標の差分を格納.
						{
							for (size_t i = 0; i < primVersCou; ++i) vec3List[i] = sxsdk::vec3(0, 0, 0);
							for (size_t i = 0; i < tvCou; ++i) {
								const int vIndex = targetD.vIndices[i];
								vec3List[vIndex] = targetD.position[i] - primitiveD.vertices[vIndex];
							}

							MathUtil::calcBoundingBox(vec3List, bbMin, bbMax);

							AccessorDesc acceDesc;
							acceDesc.accessorType  = TYPE_VEC3;
							acceDesc.componentType = COMPONENT_FLOAT;
							acceDesc.byteOffset    = byteOffset;
							acceDesc.normalized    = false;
							acceDesc.minValues.push_back(bbMin.x);
							acceDesc.minValues.push_back(bbMin.y);
							acceDesc.minValues.push_back(bbMin.z);
							acceDesc.maxValues.push_back(bbMax.x);
							acceDesc.maxValues.push_back(bbMax.y);
							acceDesc.maxValues.push_back(bbMax.z);
							const size_t byteLength = (sizeof(float) * 3) * primVersCou;

							bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
							bufferBuilder->AddAccessor(Shade3DArray::convert_vec3_to_float(vec3List), acceDesc); 

							byteOffset += byteLength;
							accessorID++;
						}

						// 法線を格納.
						if (!targetD.normal.empty()) {
							for (size_t i = 0; i < primVersCou; ++i) vec3List[i] = primitiveD.normals[i];
							for (size_t i = 0; i < tvCou; ++i) {
								const int vIndex = targetD.vIndices[i];
								vec3List[vIndex] = targetD.normal[i];
							}

							AccessorDesc acceDesc;
							acceDesc.accessorType  = TYPE_VEC3;
							acceDesc.componentType = COMPONENT_FLOAT;
							acceDesc.byteOffset    = byteOffset;
							acceDesc.normalized    = false;
							const size_t byteLength = (sizeof(float) * 3) * primVersCou;

							bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
							bufferBuilder->AddAccessor(Shade3DArray::convert_vec3_to_float(vec3List), acceDesc); 

							byteOffset += byteLength;
							accessorID++;
						}
					}
				}
			}
		}
#if 1
		// スキンの情報を格納.
		{
			const size_t skinsCou = sceneData->skins.size();
			for (size_t loop = 0; loop < skinsCou; ++loop) {
				const CSkinData& skinD = sceneData->skins[loop];
				const size_t jointsCou = skinD.joints.size();
				if (jointsCou == 0) continue;
				if (skinD.joints.empty() || skinD.inverseBindMatrices.empty()) continue;

				AccessorDesc acceDesc;
				acceDesc.accessorType  = TYPE_MAT4;
				acceDesc.componentType = COMPONENT_FLOAT;
				acceDesc.byteOffset    = byteOffset;
				acceDesc.normalized    = false;

				const size_t byteLength = sizeof(float) * 16 * jointsCou;


				// バッファ情報として格納.
				std::vector<float> fData;
				{
					fData.resize(byteLength / sizeof(float), 0.0f);
					int iPos = 0;
					for (size_t i = 0; i < jointsCou; ++i) {
						const sxsdk::mat4& m = skinD.inverseBindMatrices[i];
						for (size_t k = 0; k < 16; ++k) {
							fData[iPos++] = m[k >> 2][k & 3];
						}
					}
				}
				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
				bufferBuilder->AddAccessor(fData, acceDesc); 

				byteOffset += byteLength;
				accessorID++;
			}
		}
#endif

		// アニメーション情報を格納.
		{
			const CAnimationData animD = sceneData->animations;
			const size_t animCou = animD.channelData.size();

			for (size_t loop = 0; loop < animCou; ++loop) {
				const CAnimChannelData& channelD = animD.channelData[loop];
				const CAnimSamplerData& samplerD = animD.samplerData[loop];

				{
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_SCALAR;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					// min/maxの指定は必須.
					const size_t dataCou = samplerD.inputData.size();
					{
						float minV, maxV;
						minV = maxV = samplerD.inputData[0];
						for (size_t i = 0; i < dataCou; ++i) {
							const float v = samplerD.inputData[i];
							minV = std::min(minV, v);
							maxV = std::max(maxV, v);
						}
						acceDesc.minValues.push_back(minV);
						acceDesc.maxValues.push_back(maxV);
					}

					const size_t byteLength = sizeof(float) * dataCou;

					// バッファ情報として格納.
					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(samplerD.inputData, acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}
				{
					AccessorDesc acceDesc;
					acceDesc.accessorType  = (channelD.pathType == CAnimChannelData::path_type_translation || channelD.pathType == CAnimChannelData::path_type_scale) ? TYPE_VEC3 : TYPE_VEC4;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t dataCou = samplerD.outputData.size();
					const size_t byteLength = sizeof(float) * dataCou;

					// バッファ情報として格納.
					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(samplerD.outputData, acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}
			}
		}
	}

	/**
	 *   Accessor情報（メッシュから三角形の頂点インデックス、法線、UVバッファなどをパックしたもの）を格納.
	 *   Accessor → bufferViews → buffers、と経由して情報をバッファに保持する。.
	 *   拡張子gltfの場合、バッファは外部のbinファイル。.
	 *   格納は、格納要素のOffsetごとに4バイト alignmentを考慮（そうしないとエラーになる）.
	 */
	void setBufferData (Document& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return;

		std::string fileName = sceneData->getFileName(false);		// 拡張子を除いたファイル名を取得.
		// Windows環境の場合、.
		// fileNameに日本語ディレクトリなどがあると出力に失敗するので、SJISに置き換える.
	#if _WINDOWS
		StringUtil::convUTF8ToSJIS(fileName, fileName);
	#endif

		// binの出力ファイル名.
		std::string binFileName = sceneData->getFileName(false) + std::string(".bin");
	#if _WINDOWS
		StringUtil::convUTF8ToSJIS(binFileName, binFileName);
	#endif

		// 出力ディレクトリ.
		std::string fileDir = sceneData->getFileDir();
	#if _WINDOWS
		StringUtil::convUTF8ToSJIS(fileDir, fileDir);
	#endif

		// ファイル拡張子 (gltf/glb).
		const std::string fileExtension = sceneData->getFileExtension();

		// binの出力バッファ。これはglTF出力の場合はそのままバイナリ情報として参照する.
		std::shared_ptr<GLTFResourceWriter> binWriter;
		if (fileExtension == "gltf") {
			auto binStreamWriter = std::make_unique<BinStreamWriter>(fileDir, binFileName);
			binWriter.reset(new GLTFResourceWriter(std::move(binStreamWriter)));
		}

		// Mesh内のPrimitiveの頂点を共有するかどうか.
		bool shareVerticesMesh = sceneData->exportParam.shareVerticesMesh;

		int accessorID = 0;
		size_t byteOffset = 0;
		for (size_t meshLoop = 0; meshLoop < meshCou; ++meshLoop) {
			const CMeshData& meshD = sceneData->getMeshData(meshLoop);
			const size_t primCou = meshD.primitives.size();

			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				const CPrimitiveData& primitiveD = meshD.primitives[primLoop];

				// 頂点のバウンディングボックスを計算.
				sxsdk::vec3 bbMin, bbMax;
				primitiveD.calcBoundingBox(bbMin, bbMax);

				// indicesAccessor.
				{
					// short型で格納.
					const bool storeUShort = (primitiveD.vertices.size() < 65530);

					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_SCALAR;
					acce.componentType  = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
					acce.count          = primitiveD.triangleIndices.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;

					// 以下、4バイト alignmentを考慮 (shortの場合は2バイトであるため、4で割り切れない).
					buffV.byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * primitiveD.triangleIndices.size();
					if ((buffV.byteLength & 3) != 0) buffV.byteLength = (buffV.byteLength + 3) & 0xfffffffc;

					buffV.target     = ELEMENT_ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						if (storeUShort) {
							std::vector<unsigned short> shortData;
							shortData.resize(buffV.byteLength / sizeof(unsigned short), 0);		// バイト alignmentを考慮して余分なバッファを確保.
							for (size_t i = 0; i < primitiveD.triangleIndices.size(); ++i) {
								shortData[i] = (unsigned short)(primitiveD.triangleIndices[i]);
							}
							binWriter->Write(gltfDoc.bufferViews[accessorID], &(shortData[0]), gltfDoc.accessors[accessorID]);

						} else {
							binWriter->Write(gltfDoc.bufferViews[accessorID], &(primitiveD.triangleIndices[0]), gltfDoc.accessors[accessorID]);
						}
					}

					byteOffset += buffV.byteLength;
					accessorID++;

				}

				// normalsAccessor.
				if (primLoop == 0 || !shareVerticesMesh) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC3;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = primitiveD.normals.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (sizeof(float) * 3) * primitiveD.normals.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(primitiveD.normals[0]), gltfDoc.accessors[accessorID]);

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// positionsAccessor.
				if (primLoop == 0 || !shareVerticesMesh) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC3;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = primitiveD.vertices.size();
					acce.min.push_back(bbMin.x);
					acce.min.push_back(bbMin.y);
					acce.min.push_back(bbMin.z);
					acce.max.push_back(bbMax.x);
					acce.max.push_back(bbMax.y);
					acce.max.push_back(bbMax.z);
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (sizeof(float) * 3) * primitiveD.vertices.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(primitiveD.vertices[0]), gltfDoc.accessors[accessorID]);

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// uv0Accessor.
				if (!primitiveD.uv0.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC2;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = primitiveD.uv0.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (sizeof(float) * 2) * primitiveD.uv0.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(primitiveD.uv0[0]), gltfDoc.accessors[accessorID]);

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// uv1Accessor.
				if (!primitiveD.uv1.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC2;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = primitiveD.uv1.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (sizeof(float) * 2) * primitiveD.uv1.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(primitiveD.uv1[0]), gltfDoc.accessors[accessorID]);

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// color0Accessor.
				if (!primitiveD.color0.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC4;
					acce.componentType  = COMPONENT_UNSIGNED_BYTE;
					acce.count          = primitiveD.color0.size();
					acce.normalized     = true;
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (sizeof(unsigned char) * 4) * primitiveD.color0.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						std::vector<unsigned char> buff = Shade3DArray::convert_vec4_to_uchar(primitiveD.color0, true);
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(buff[0]), gltfDoc.accessors[accessorID]);
					}
					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// SkinのJoints.
				if (!primitiveD.skinJoints.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					// short型で格納.
					const bool storeUShort = (primitiveD.skinJoints.size() < 65530);

					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC4;
					acce.componentType  = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
					acce.count          = primitiveD.skinJoints.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * 4 * primitiveD.skinJoints.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						if (storeUShort) {
							std::vector<unsigned short> shortData;
							shortData.resize(buffV.byteLength / sizeof(unsigned short), 0);
							for (size_t i = 0, iPos = 0; i < primitiveD.skinJoints.size(); ++i, iPos += 4) {
								shortData[iPos + 0] = (unsigned short)(primitiveD.skinJoints[i][0]);
								shortData[iPos + 1] = (unsigned short)(primitiveD.skinJoints[i][1]);
								shortData[iPos + 2] = (unsigned short)(primitiveD.skinJoints[i][2]);
								shortData[iPos + 3] = (unsigned short)(primitiveD.skinJoints[i][3]);
							}
							binWriter->Write(gltfDoc.bufferViews[accessorID], &(shortData[0]), gltfDoc.accessors[accessorID]);
						} else {
							std::vector<unsigned int> intData;
							intData.resize(buffV.byteLength / sizeof(unsigned int), 0);
							for (size_t i = 0, iPos = 0; i < primitiveD.skinJoints.size(); ++i, iPos += 4) {
								intData[iPos + 0] = (unsigned int)(primitiveD.skinJoints[i][0]);
								intData[iPos + 1] = (unsigned int)(primitiveD.skinJoints[i][1]);
								intData[iPos + 2] = (unsigned int)(primitiveD.skinJoints[i][2]);
								intData[iPos + 3] = (unsigned int)(primitiveD.skinJoints[i][3]);
							}
							binWriter->Write(gltfDoc.bufferViews[accessorID], &(intData[0]), gltfDoc.accessors[accessorID]);
						}
					}

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// SkinのWeights.
				if (!primitiveD.skinWeights.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_VEC4;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = primitiveD.skinWeights.size();
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = sizeof(float) * 4 * primitiveD.skinWeights.size();
					buffV.target     = ARRAY_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						std::vector<float> fData;
						fData.resize(buffV.byteLength / sizeof(float), 0.0f);
						for (size_t i = 0, iPos = 0; i < primitiveD.skinWeights.size(); ++i, iPos += 4) {
							fData[iPos + 0] = primitiveD.skinWeights[i][0];
							fData[iPos + 1] = primitiveD.skinWeights[i][1];
							fData[iPos + 2] = primitiveD.skinWeights[i][2];
							fData[iPos + 3] = primitiveD.skinWeights[i][3];
						}
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(fData[0]), gltfDoc.accessors[accessorID]);
					}
					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// Morph Targets.
				if (!primitiveD.morphTargets.morphTargetsData.empty() && (primLoop == 0 || !shareVerticesMesh)) {
					const size_t primVersCou = primitiveD.vertices.size();
					std::vector<sxsdk::vec3> vec3List, vec4List;
					vec3List.resize(primVersCou);
					vec4List.resize(primVersCou);
					const CMorphTargetsData& morphTargetsD = primitiveD.morphTargets;
					for (size_t tLoop = 0; tLoop < morphTargetsD.morphTargetsData.size(); ++tLoop) {
						const COneMorphTargetData& targetD = morphTargetsD.morphTargetsData[tLoop];
						const size_t tvCou = targetD.vIndices.size();

						// 頂点座標の差分を格納.
						{
							for (size_t i = 0; i < primVersCou; ++i) vec3List[i] = sxsdk::vec3(0, 0, 0);
							for (size_t i = 0; i < tvCou; ++i) {
								const int vIndex = targetD.vIndices[i];
								vec3List[vIndex] = targetD.position[i] - primitiveD.vertices[vIndex];
							}

							MathUtil::calcBoundingBox(vec3List, bbMin, bbMax);

							Accessor acce;
							acce.id             = std::to_string(accessorID);
							acce.bufferViewId   = std::to_string(accessorID);
							acce.type           = TYPE_VEC3;
							acce.componentType  = COMPONENT_FLOAT;
							acce.count          = primVersCou;
							acce.min.push_back(bbMin.x);
							acce.min.push_back(bbMin.y);
							acce.min.push_back(bbMin.z);
							acce.max.push_back(bbMax.x);
							acce.max.push_back(bbMax.y);
							acce.max.push_back(bbMax.z);
							gltfDoc.accessors.Append(acce);

							BufferView buffV;
							buffV.id         = std::to_string(accessorID);
							buffV.bufferId   = std::string("0");
							buffV.byteOffset = byteOffset;
							buffV.byteLength = (sizeof(float) * 3) * primVersCou;
							buffV.target     = ARRAY_BUFFER;
							gltfDoc.bufferViews.Append(buffV);

							// バッファ情報として格納.
							if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(vec3List[0]), gltfDoc.accessors[accessorID]);

							byteOffset += buffV.byteLength;
							accessorID++;
						}

						// 法線を格納.
						if (!targetD.normal.empty()) {
							for (size_t i = 0; i < primVersCou; ++i) vec3List[i] = primitiveD.normals[i];
							for (size_t i = 0; i < tvCou; ++i) {
								const int vIndex = targetD.vIndices[i];
								vec3List[vIndex] = targetD.normal[i];
							}

							Accessor acce;
							acce.id             = std::to_string(accessorID);
							acce.bufferViewId   = std::to_string(accessorID);
							acce.type           = TYPE_VEC3;
							acce.componentType  = COMPONENT_FLOAT;
							acce.count          = primVersCou;
							gltfDoc.accessors.Append(acce);

							BufferView buffV;
							buffV.id         = std::to_string(accessorID);
							buffV.bufferId   = std::string("0");
							buffV.byteOffset = byteOffset;
							buffV.byteLength = (sizeof(float) * 3) * primVersCou;
							buffV.target     = ARRAY_BUFFER;
							gltfDoc.bufferViews.Append(buffV);

							// バッファ情報として格納.
							if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(vec3List[0]), gltfDoc.accessors[accessorID]);

							byteOffset += buffV.byteLength;
							accessorID++;
						}
					}
				}
			}
		}
#if 1
		// スキンの情報を格納.
		{
			const size_t skinsCou = sceneData->skins.size();
			for (size_t loop = 0; loop < skinsCou; ++loop) {
				const CSkinData& skinD = sceneData->skins[loop];
				const size_t jointsCou = skinD.joints.size();
				if (jointsCou == 0) continue;
				if (skinD.joints.empty() || skinD.inverseBindMatrices.empty()) continue;

				Accessor acce;
				acce.id             = std::to_string(accessorID);
				acce.bufferViewId   = std::to_string(accessorID);
				acce.type           = TYPE_MAT4;
				acce.componentType  = COMPONENT_FLOAT;
				acce.count          = jointsCou;
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = sizeof(float) * 16 * jointsCou;
				buffV.target     = UNKNOWN_BUFFER;		// Skinのmatrixの場合は、UNKNOWN_BUFFERを指定しないとエラーになる ?.
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) {
					std::vector<float> fData;
					fData.resize(buffV.byteLength / sizeof(float), 0.0f);
					int iPos = 0;
					for (size_t i = 0; i < jointsCou; ++i) {
						const sxsdk::mat4& m = skinD.inverseBindMatrices[i];
						for (size_t k = 0; k < 16; ++k) {
							fData[iPos++] = m[k >> 2][k & 3];
						}
					}

					binWriter->Write(gltfDoc.bufferViews[accessorID], &(fData[0]), gltfDoc.accessors[accessorID]);
				}

				byteOffset += buffV.byteLength;
				accessorID++;
			}
		}
#endif
		// アニメーション情報を格納.
		{
			const CAnimationData animD = sceneData->animations;
			const size_t animCou = animD.channelData.size();

			for (size_t loop = 0; loop < animCou; ++loop) {
				const CAnimChannelData& channelD = animD.channelData[loop];
				const CAnimSamplerData& samplerD = animD.samplerData[loop];

				{
					const size_t dataCou = samplerD.inputData.size();

					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = TYPE_SCALAR;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = dataCou;

					// min/maxの指定は必須.
					{
						float minV, maxV;
						minV = maxV = samplerD.inputData[0];
						for (size_t i = 0; i < dataCou; ++i) {
							const float v = samplerD.inputData[i];
							minV = std::min(minV, v);
							maxV = std::max(maxV, v);
						}
						acce.min.push_back(minV);
						acce.max.push_back(maxV);
					}

					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = sizeof(float) * dataCou;
					buffV.target     = UNKNOWN_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(samplerD.inputData[0]), gltfDoc.accessors[accessorID]);
					}

					byteOffset += buffV.byteLength;
					accessorID++;
				}
				{
					const size_t dataCou = samplerD.outputData.size();

					const int eCou = (channelD.pathType == CAnimChannelData::path_type_translation || channelD.pathType == CAnimChannelData::path_type_scale) ? 3 : 4;
					Accessor acce;
					acce.id             = std::to_string(accessorID);
					acce.bufferViewId   = std::to_string(accessorID);
					acce.type           = (channelD.pathType == CAnimChannelData::path_type_translation || channelD.pathType == CAnimChannelData::path_type_scale) ? TYPE_VEC3 : TYPE_VEC4;
					acce.componentType  = COMPONENT_FLOAT;
					acce.count          = dataCou / eCou;
					gltfDoc.accessors.Append(acce);

					BufferView buffV;
					buffV.id         = std::to_string(accessorID);
					buffV.bufferId   = std::string("0");
					buffV.byteOffset = byteOffset;
					buffV.byteLength = sizeof(float) * dataCou;
					buffV.target     = UNKNOWN_BUFFER;
					gltfDoc.bufferViews.Append(buffV);

					// バッファ情報として格納.
					if (binWriter) {
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(samplerD.outputData[0]), gltfDoc.accessors[accessorID]);
					}

					byteOffset += buffV.byteLength;
					accessorID++;
				}
			}
		}

		// buffers.
		{
			Buffer buff;
			buff.id         = std::string("0");
			buff.byteLength = byteOffset;
			if (fileExtension == "gltf") {
				buff.uri = binFileName;
			}
			gltfDoc.buffers.Append(buff);
		}

		// GLB出力のバッファを確保.
		if (bufferBuilder) {
			setBufferData_to_BufferBuilder(gltfDoc, sceneData, bufferBuilder);
		}
	}

	/**
	 *   Image/Textures情報を格納.
	 */
	void setImagesData (Document& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder, sxsdk::shade_interface* shade) {
		const size_t imagesCou = sceneData->images.size();

		std::vector<std::string> imageFileNameList;
		imageFileNameList.resize(imagesCou, "");

		// 最大テクスチャサイズ.
		const int maxTexSize = sceneData->exportParam.getMaxTextureSize();

		compointer<sxsdk::scene_interface> scene(shade->get_scene_interface());

		for (size_t i = 0; i < imagesCou; ++i) {
			const CImageData& imageD = sceneData->images[i];
			if (!imageD.shadeMasterImage && !imageD.shadeImage) continue;
			std::string fileName = StringUtil::getFileName(imageD.name);
			if (fileName == "") fileName = std::string("image_") + std::to_string(i);

			// 拡張子を取得.
			std::string extStr = StringUtil::getFileExtension(fileName);
			if ((sceneData->exportParam.outputTexture) == GLTFConverter::export_texture_name) {
				if (extStr == "jpeg") extStr = "jpg";
				if (extStr != "jpg" && extStr != "png") {
					extStr = "png";
				}
			} else if ((sceneData->exportParam.outputTexture) == GLTFConverter::export_texture_png) {
				extStr = "png";
			} else if ((sceneData->exportParam.outputTexture) == GLTFConverter::export_texture_jpeg) {
				extStr = "jpg";
			}
			// アルファ透明を使用する場合は、pngにする.
			if (imageD.useBaseColorAlpha) extStr = "png";

			// ファイル名を再構成.
			fileName = StringUtil::getFileName(imageD.name, false) + std::string(".") + extStr;

			try {
				// 画像ファイルを指定の拡張子(jpg/png)で保存.
				sxsdk::image_interface *image = (imageD.shadeMasterImage) ? (imageD.shadeMasterImage->get_image()) : imageD.shadeImage;
				if (!image) continue;

				// テクスチャをリサイズする場合 (ver.0.2.0.2 - ).
				compointer<sxsdk::image_interface> image2;
				if (sceneData->exportParam.maxTextureSize != GLTFConverter::export_max_texture_size_undefined) {
					const int srcWidth  = image->get_size().x;
					const int srcHeight = image->get_size().y;
					if (srcWidth > maxTexSize || srcHeight > maxTexSize) {
						sx::vec<int,2> newSize(srcWidth, srcHeight);
						if (srcWidth > maxTexSize && srcHeight <= maxTexSize) {
							newSize.x = maxTexSize;
							newSize.y = maxTexSize * srcHeight / srcWidth;
						} else if (srcHeight > maxTexSize && srcWidth <= maxTexSize) {
							newSize.y = maxTexSize;
							newSize.x = maxTexSize * srcWidth / srcHeight;
						} else if (srcWidth > maxTexSize && srcHeight > maxTexSize) {
							if (srcWidth > srcHeight) {
								newSize.x = maxTexSize;
								newSize.y = maxTexSize * srcHeight / srcWidth;
							} else {
								newSize.y = maxTexSize;
								newSize.x = maxTexSize * srcWidth / srcHeight;
							}
						}
						if (newSize.x != srcWidth || newSize.y != srcHeight) {
							image2 = Shade3DUtil::resizeImageWithAlpha(scene, image, newSize);
						}
					}
				}

				const std::string fileFullPath = (sceneData->getFileDir()) + std::string("/") + fileName;
				if (image2 && image2->has_image()) {
					image2->save(fileFullPath.c_str());
				} else {
					image->save(fileFullPath.c_str());
				}

				imageFileNameList[i] = fileFullPath;

				// GLTFにImage要素を追加.
				Image img;
				img.id       = std::to_string(i);
				img.uri      = fileName;
				gltfDoc.images.Append(img);

				// GLTFにTexture要素を追加.
				Texture tex;
				tex.id        = std::to_string(i);
				tex.imageId   = std::to_string(i);
				tex.samplerId = std::to_string(i);
				gltfDoc.textures.Append(tex);

				// GLTFにSampler要素を追加.
				Sampler sampl;
				sampl.id        = std::to_string(i);
				sampl.minFilter = MinFilter_LINEAR;
				sampl.magFilter = MagFilter_LINEAR;
				sampl.wrapS     = Wrap_REPEAT;
				sampl.wrapT     = Wrap_REPEAT;
				gltfDoc.samplers.Append(sampl);

			} catch (...) {
				continue;
			}
		}

		// glb出力の場合.
		if (bufferBuilder) {
			for (size_t i = 0; i < imagesCou; ++i) {
				// ファイル出力した画像をいったん読み込み、bufferBuilderに格納する.
				std::string fileName = imageFileNameList[i];
#if _WINDOWS
				StringUtil::convUTF8ToSJIS(fileName, fileName);
#endif

				try {
					std::shared_ptr<BinStreamReader> binStreamReader;
					std::shared_ptr<GLTFResourceReader> binReader;

					binStreamReader.reset(new BinStreamReader(StringUtil::getFileDir(fileName)));
					binReader.reset(new GLTFResourceReader(binStreamReader));

					// uriはSJISに変換.
					Image image2(gltfDoc.images[i]);
#if _WINDOWS
					StringUtil::convUTF8ToSJIS(image2.uri, image2.uri);
#endif

					auto data = binReader->ReadBinaryData(gltfDoc, image2);
					auto imageBufferView = bufferBuilder->AddBufferView(data);

					Image newImage(gltfDoc.images[i]);
					newImage.bufferViewId = imageBufferView.id;

					// mimeの指定 (image/jpeg , image/png).
					const std::string extStr = StringUtil::getFileExtension(newImage.uri);
					newImage.mimeType = std::string("image/");
					if (extStr == "jpg" || extStr == "jpeg") newImage.mimeType += "jpeg";
					else newImage.mimeType += "png";

					newImage.name = newImage.uri;
					newImage.uri.clear();
					gltfDoc.images.Replace(newImage);

				} catch (...) {
					break;
				}
			}

			// 出力したイメージファイルを削除.
			for (size_t i = 0; i < imagesCou; ++i) {
				try {
					const std::string fileName = imageFileNameList[i];
					shade->delete_file(fileName.c_str());
				} catch (...) { }
			}
		}
	}

	/**
	 * スキン情報を格納.
	 */
	int setSkinData (Document& gltfDoc,  const CSceneData* sceneData, const int meshAccessorIDCount) {
		const size_t skinsCou = sceneData->skins.size();
		if (skinsCou == 0) return meshAccessorIDCount;

		int accessorID = meshAccessorIDCount;
		for (size_t loop = 0; loop < skinsCou; ++loop) {
			const CSkinData& skinD = sceneData->skins[loop];
			const int meshIndex = skinD.meshIndex;
			if (meshIndex < 0) continue;

			const size_t joinsCou = skinD.joints.size();
			Skin dstSkin;
			dstSkin.name = skinD.name;
			dstSkin.id   = std::to_string(loop);
#if 1
			// 各ジョイントの変換行列の逆数の保持用.
			if (!skinD.joints.empty() && !skinD.inverseBindMatrices.empty()) {
				dstSkin.inverseBindMatricesAccessorId = std::to_string(accessorID++);
			}

#endif
			if (skinD.skeletonID >= 0) dstSkin.skeletonId = std::to_string(skinD.skeletonID);
			for (size_t j = 0; j < joinsCou; ++j) dstSkin.jointIds.push_back(std::to_string(skinD.joints[j]));
			gltfDoc.skins.Append(dstSkin);
		}

		return accessorID;
	}

	/**
	 * アニメーション情報を格納.
	 */
	int setAnimationData (Document& gltfDoc,  const CSceneData* sceneData, const int skinAccessorIDCount) {
		const CAnimationData animD = sceneData->animations;
		const size_t animCou = animD.channelData.size();
		if (animCou == 0) return skinAccessorIDCount;

		Animation anim;
		anim.id   = std::to_string(0);
		anim.name = animD.name;

		int accessorID = skinAccessorIDCount;
		for (size_t loop = 0; loop < animCou; ++loop) {
			const CAnimChannelData& channelD = animD.channelData[loop];

			// Channel情報を格納.
			{
				AnimationChannel animChannel;
				animChannel.id            = std::to_string(loop);
				animChannel.samplerId     = std::to_string(channelD.samplerIndex);
				animChannel.target.nodeId = std::to_string(channelD.targetNodeIndex);
				animChannel.target.path   = (channelD.pathType == CAnimChannelData::path_type_translation) ? TARGET_TRANSLATION : TARGET_ROTATION;
				anim.channels.Append(animChannel);
			}

			// Sampler情報を格納.
			{
				AnimationSampler animSampler;
				animSampler.id               = std::to_string(loop);
				animSampler.inputAccessorId  = std::to_string(accessorID++);
				animSampler.interpolation    = INTERPOLATION_LINEAR;
				animSampler.outputAccessorId = std::to_string(accessorID++);
				anim.samplers.Append(animSampler);
			}
		}

		gltfDoc.animations.Append(anim);

		return accessorID;
	}

}

CGLTFSaver::CGLTFSaver (sxsdk::shade_interface* shade) : shade(shade)
{
}

/**
 * エラー時の文字列取得.
 */
std::string CGLTFSaver::getErrorString () const
{
	return g_errorMessage;
}

/**
 * 指定のGLTFファイルを出力.
 * @param[in]  fileName    出力ファイル名 (gltf/glb).
 * @param[in]  sceneData   GLTFのシーン情報.
 */
bool CGLTFSaver::saveGLTF (const std::string& fileName, const CSceneData* sceneData)
{
	std::string json = "";
	g_errorMessage = "";

	// Windows環境の場合、fileNameに日本語ディレクトリなどがあると出力に失敗するので、.
	// SJISに置き換える.
	std::string filePath2 = fileName;
#if _WINDOWS
	StringUtil::convUTF8ToSJIS(fileName, filePath2);
#endif
	const std::string fileDir2  = StringUtil::getFileDir(filePath2);
	const std::string fileName2 = StringUtil::getFileName(filePath2);

	try {
		Document gltfDoc;
		gltfDoc.images.Clear();
	    gltfDoc.buffers.Clear();
		gltfDoc.bufferViews.Clear();
		gltfDoc.accessors.Clear();

		std::unique_ptr<BufferBuilder> glbBuilder;
		bool writeGLB = false; 
		if (sceneData->getFileExtension() == "glb") {
			writeGLB = true;
			auto binStreamWriter = std::make_shared<const BinStreamWriter>(fileDir2, fileName2);
			auto glbWriter = std::make_unique<GLBResourceWriter>(binStreamWriter);
			glbBuilder = std::make_unique<BufferBuilder>(std::move(glbWriter));
			glbBuilder->AddBuffer(GLB_BUFFER_ID);
		}

		// ヘッダ部を指定.
		gltfDoc.asset.generator = sceneData->assetGenerator;
		gltfDoc.asset.version   = sceneData->assetVersion;		// GLTFバージョン.
		gltfDoc.asset.copyright = sceneData->assetCopyRight;

		// ヘッダ拡張部を指定.
		gltfDoc.asset.extras = ::getAssetExtrasStr(sceneData);

		// シーン情報を指定.
		{
			Scene gltfScene;
			gltfScene.name = "Scene";
			gltfScene.id   = "0";
			if (!sceneData->nodes.empty()) {
				gltfScene.nodes.push_back("0");			// ルートノード.
			}

			gltfDoc.scenes.Append(gltfScene);
		}

		// マテリアル情報を指定.
		::setMaterialsData(gltfDoc, sceneData);

		// ノード情報を指定.
		::setNodesData(gltfDoc, sceneData);

		// メッシュ情報を指定.
		const int meshAccessorIDCount = ::setMeshesData(gltfDoc, sceneData);

		// スキン情報を指定.
		const int skinAccessorIDCount = ::setSkinData(gltfDoc, sceneData, meshAccessorIDCount);

		// アニメーション情報を指定.
		::setAnimationData(gltfDoc, sceneData, skinAccessorIDCount);

		// バッファ情報を指定.
		// 拡張子がgltfの場合、binファイルもここで出力.
		// 拡張子がglbの場合、glbBuilderにバッファ情報を格納.
		::setBufferData(gltfDoc, sceneData, glbBuilder);

		// 画像情報を格納.
		::setImagesData(gltfDoc, sceneData, glbBuilder, shade);

		if (glbBuilder) {
			gltfDoc.buffers.Clear();
			gltfDoc.bufferViews.Clear();
			gltfDoc.accessors.Clear();

			glbBuilder->Output(gltfDoc);	// glbBuilderの情報をgltfDocに反映.

			// glbファイルを出力.
			auto manifest     = Serialize(gltfDoc);
			auto outputWriter = dynamic_cast<GLBResourceWriter *>(&glbBuilder->GetResourceWriter());
			if (outputWriter) outputWriter->Flush(manifest, filePath2);

		} else {
			// gltfファイルを出力.
			std::string gltfJson = Serialize(gltfDoc, SerializeFlags::Pretty);
			std::ofstream outStream(filePath2.c_str(), std::ios::trunc | std::ios::out);
			outStream << gltfJson;
			outStream.flush();
		}

	} catch (GLTFException e) {
		const std::string errorStr(e.what());
		g_errorMessage = errorStr;
		return false;
	}

	// Draco圧縮を行う.
	if (sceneData->exportParam.dracoCompression) {
		glTFToolKit::GLTFMeshCompressionUtils::doDracoCompress(filePath2);
	}

	return true;
}

