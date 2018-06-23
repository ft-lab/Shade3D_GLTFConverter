/**
 * GLTFをファイルに保存する.
 */
#include "GLTFSaver.h"
#include "StringUtil.h"

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLTFResourceWriter2.h>
#include <GLTFSDK/GLBResourceWriter2.h>
#include <GLTFSDK/IStreamWriter.h>
#include <GLTFSDK/IStreamFactory.h>
#include <GLTFSDK/BufferBuilder.h>

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

namespace {
	std::string g_errorMessage = "";		// エラーメッセージの保持用.

	/**
	 * sxsdk::vec4をfoatの配列に置き換え.
	 * @param[in] vList     vec4の配列.
	 * @param[in] useAlpha  Alpha情報を格納するか.
	 */
	std::vector<float> convert_vec4_to_float (const std::vector<sxsdk::vec4>& vList, const bool useAlpha = true)
	{
		const int eCou = useAlpha ? 4 : 3;
		std::vector<float> newData = std::vector<float>(vList.size() * eCou);
		for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += eCou) {
			newData[iPos + 0] = vList[i].x;
			newData[iPos + 1] = vList[i].y;
			newData[iPos + 2] = vList[i].z;
			if (useAlpha) newData[iPos + 3] = vList[i].w;
		}
		return newData;
	}

	/**
	 * sxsdk::vec4をunsigned charの配列に置き換え.
	 * @param[in] vList     vec4の配列.
	 * @param[in] useAlpha  Alpha情報を格納するか.
	 */
	std::vector<unsigned char> convert_vec4_to_uchar (const std::vector<sxsdk::vec4>& vList, const bool useAlpha = true)
	{
		const int eCou = useAlpha ? 4 : 3;
		std::vector<unsigned char> newData = std::vector<unsigned char>(vList.size() * eCou);
		for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += eCou) {
			newData[iPos + 0] = (unsigned char)std::min((int)(vList[i].x * 255.0f), 255);
			newData[iPos + 1] = (unsigned char)std::min((int)(vList[i].y * 255.0f), 255);
			newData[iPos + 2] = (unsigned char)std::min((int)(vList[i].z * 255.0f), 255);
			if (useAlpha) newData[iPos + 3] = (unsigned char)std::min((int)(vList[i].w * 255.0f), 255);
		}
		return newData;
	}

	/**
	 * sxsdk::vec3をfoatの配列に置き換え.
	 */
	std::vector<float> convert_vec3_to_float (const std::vector<sxsdk::vec3>& vList)
	{
		std::vector<float> newData = std::vector<float>(vList.size() * 3);
		for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += 3) {
			newData[iPos + 0] = vList[i].x;
			newData[iPos + 1] = vList[i].y;
			newData[iPos + 2] = vList[i].z;
		}
		return newData;
	}

	/**
	 * sxsdk::vec2をfoatの配列に置き換え.
	 */
	std::vector<float> convert_vec2_to_float (const std::vector<sxsdk::vec2>& vList)
	{
		std::vector<float> newData = std::vector<float>(vList.size() * 2);
		for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += 2) {
			newData[iPos + 0] = vList[i].x;
			newData[iPos + 1] = vList[i].y;
		}
		return newData;
	}

	/**
	 * バイナリ読み込み用.
	 */
	class BinStreamReader : public IStreamReader
	{
	private:
		std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.

	public:
		BinStreamReader (std::string basePath) : m_basePath(basePath)
		{
		}

		virtual std::shared_ptr<std::istream> GetInputStream(const std::string& filename) const override
		{
			const std::string path = m_basePath + std::string("/") + filename;
			return std::make_shared<std::ifstream>(path, std::ios::binary);
		}
	};

	/**
	 * GLTFのバイナリ出力用.
	 */
	class BinStreamWriter : public IStreamWriter
	{
	private:
		std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.
		std::string m_fileName;		// 出力ファイル名.

	public:
		BinStreamWriter (std::string basePath, std::string fileName) : m_basePath(basePath), m_fileName(fileName)
		{
		}
		virtual ~BinStreamWriter () {
		}

		virtual std::shared_ptr<std::ostream> GetOutputStream (const std::string& filename) const override
		{
			const std::string path = m_basePath + std::string("/") + m_fileName;
			return std::make_shared<std::ofstream>(path, std::ios::binary | std::ios::out);
		}
	};

	/**
	 * GLBのバイナリ出力用.
	 */
	class OutputGLBStreamFactory : public IStreamFactory
	{
	private:
		std::string m_basePath;		// glbファイルの絶対パスのディレクトリ.
		std::string m_fileName;		// 出力ファイル名.

	public:
		OutputGLBStreamFactory (std::string basePath, std::string fileName) : 
			m_basePath(basePath), m_fileName(fileName),
			m_stream(std::make_shared<std::stringstream>(std::ios_base::app | std::ios_base::binary | std::ios_base::in | std::ios_base::out))
	
		{
		}

		virtual ~OutputGLBStreamFactory () {
		}

		virtual std::shared_ptr<std::iostream> GetTemporaryStream (const std::string& uri) const override
		{
			return std::dynamic_pointer_cast<std::iostream>(m_stream);
		}

		virtual std::shared_ptr<std::ostream> GetOutputStream (const std::string& filename) const override
		{
			const std::string path = m_basePath + std::string("/") + m_fileName;
			return std::make_shared<std::ofstream>(path, std::ios::binary | std::ios::out);
		}

		virtual std::shared_ptr<std::istream> GetInputStream(const std::string&) const override
		{
			return std::dynamic_pointer_cast<std::istream>(m_stream);
		}

	private:
		std::shared_ptr<std::stringstream> m_stream;
	};

	/**
	 * ノード情報を指定.
	 */
	void setNodesData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
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
	void setMaterialsData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
		const size_t mCou = sceneData->materials.size();

		// テクスチャの繰り返しで (1, 1)でないものがあるかチェック.
		bool repeatTex = false;
		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];
			if (materialD.baseColorTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.emissionTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.normalTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.metallicRoughnessTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (materialD.occlusionTexScale != sxsdk::vec2(1, 1)) repeatTex = true;
			if (repeatTex) break;
		}

		// 拡張として使用する要素名を追加.
		if (repeatTex) {
			gltfDoc.extensionsUsed.insert("KHR_texture_transform");
		}

		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& materialD = sceneData->materials[i];

			Material material;
			material.id          = std::to_string(i);
			material.name        = materialD.name;
			material.doubleSided = materialD.doubleSided;
			material.alphaMode   = ALPHA_OPAQUE;
			material.emissiveFactor = Color3(materialD.emissionFactor.red, materialD.emissionFactor.green, materialD.emissionFactor.blue);

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

			if (materialD.emissionImageIndex >= 0) {
				material.emissiveTexture.textureId =  std::to_string(materialD.emissionImageIndex);
				material.emissiveTexture.texCoord  = (size_t)materialD.emissionTexCoord;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.emissionTexScale);
					if (str != "") material.emissiveTexture.extensions["KHR_texture_transform"] = str;
				}
			}

			if (materialD.normalImageIndex >= 0) {
				material.normalTexture.textureId = std::to_string(materialD.normalImageIndex);
				material.normalTexture.texCoord  = (size_t)materialD.normalTexCoord;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.normalTexScale);
					if (str != "") material.normalTexture.extensions["KHR_texture_transform"] = str;
				}
			}
			if (materialD.metallicRoughnessImageIndex >= 0) {		// TODO : まだ途中.
				material.metallicRoughness.metallicRoughnessTexture.texCoord  = (size_t)materialD.metallicRoughnessTexCoord;
			}
			if (materialD.occlusionImageIndex >= 0) {				// TODO : まだ途中.
				material.occlusionTexture.textureId = std::to_string(materialD.occlusionImageIndex);
				material.occlusionTexture.texCoord  = (size_t)materialD.occlusionTexCoord;

				// テクスチャのTiling情報を指定.
				{
					const std::string str = getTextureTransformStr(sxsdk::vec2(0, 0), materialD.occlusionTexScale);
					if (str != "") material.occlusionTexture.extensions["KHR_texture_transform"] = str;
				}
			}

			// アルファ合成のパラメータを渡す.
			if (materialD.alphaMode != ALPHA_OPAQUE) {
				material.alphaMode   = (AlphaMode)3;		// ALPHA_MASK.
				material.alphaCutoff = materialD.alphaCutOff;
			}

			gltfDoc.materials.Append(material);
		}
	}

	/**
	 * メッシュ情報を指定.
	 * @return accessorIDの数.
	 */
	int setMeshesData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return 0;

		int accessorID = 0;
		for (size_t meshLoop = 0; meshLoop < meshCou; ++meshLoop) {
			const CMeshData& meshD = sceneData->getMeshData(meshLoop);
			const size_t primCou = meshD.primitives.size();

			Mesh mesh;
			mesh.name = meshD.name;
			mesh.id   = std::to_string(meshLoop);

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
				meshPrimitive.normalsAccessorId   = std::to_string(accessorID++);
				meshPrimitive.positionsAccessorId = std::to_string(accessorID++);
				if (!primitiveD.uv0.empty()) {
					meshPrimitive.uv0AccessorId = std::to_string(accessorID++);
				}
				if (!primitiveD.uv1.empty()) {
					meshPrimitive.uv1AccessorId = std::to_string(accessorID++);
				}
				if (!primitiveD.color0.empty()) {
					meshPrimitive.color0AccessorId = std::to_string(accessorID++);
				}
				if (!primitiveD.skinJoints.empty()) {
					meshPrimitive.joints0AccessorId = std::to_string(accessorID++);
				}
				if (!primitiveD.skinWeights.empty()) {
					meshPrimitive.weights0AccessorId = std::to_string(accessorID++);
				}

				meshPrimitive.mode = MESH_TRIANGLES;		// (4) 三角形情報として格納.

				mesh.primitives.push_back(meshPrimitive);
			}

			// Mesh情報を追加.
			gltfDoc.meshes.Append(mesh);
		}

		return accessorID;
	}

	/**
	 *   GLB出力向けにバイナリ情報をbufferBuilderに格納.
	 */
	void setBufferData_to_BufferBuilder (GLTFDocument& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return;

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
				{
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC3;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 3) * primitiveD.normals.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(convert_vec3_to_float(primitiveD.normals), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// positionsAccessor.
				{
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
					bufferBuilder->AddAccessor(convert_vec3_to_float(primitiveD.vertices), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// uv0Accessor.
				if (!primitiveD.uv0.empty()) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC2;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 2) * primitiveD.uv0.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(convert_vec2_to_float(primitiveD.uv0), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// uv1Accessor.
				if (!primitiveD.uv1.empty()) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC2;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 2) * primitiveD.uv1.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(convert_vec2_to_float(primitiveD.uv1), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// color0Accessor.
				if (!primitiveD.color0.empty()) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC4;
					acceDesc.componentType = COMPONENT_UNSIGNED_BYTE;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = true;

					size_t byteLength = (sizeof(unsigned char) * 4) * primitiveD.color0.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					const std::vector<unsigned char> buff = convert_vec4_to_uchar(primitiveD.color0, true);
					bufferBuilder->AddAccessor(buff, acceDesc); 

					byteOffset += byteLength;
					accessorID++;
				}

				// SkinのJoints.
				if (!primitiveD.skinJoints.empty()) {
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
				if (!primitiveD.skinWeights.empty()) {
					AccessorDesc acceDesc;
					acceDesc.accessorType  = TYPE_VEC4;
					acceDesc.componentType = COMPONENT_FLOAT;
					acceDesc.byteOffset    = byteOffset;
					acceDesc.normalized    = false;

					const size_t byteLength = (sizeof(float) * 4) * primitiveD.skinWeights.size();

					bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
					bufferBuilder->AddAccessor(convert_vec4_to_float(primitiveD.skinWeights), acceDesc); 

					byteOffset += byteLength;
					accessorID++;
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
	void setBufferData (GLTFDocument& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder) {
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

		// binの出力バッファ.
		std::shared_ptr<GLTFResourceWriter2> binWriter;
		if (fileExtension == "gltf") {
			auto binStreamWriter = std::make_unique<BinStreamWriter>(fileDir, binFileName);
			binWriter.reset(new GLTFResourceWriter2(std::move(binStreamWriter), binFileName));
		}

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
				{
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
				{
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
				if (!primitiveD.uv0.empty()) {
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
				if (!primitiveD.uv1.empty()) {
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
				if (!primitiveD.color0.empty()) {
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
						std::vector<unsigned char> buff = convert_vec4_to_uchar(primitiveD.color0, true);
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(buff[0]), gltfDoc.accessors[accessorID]);
					}

					byteOffset += buffV.byteLength;
					accessorID++;
				}

				// SkinのJoints.
				if (!primitiveD.skinJoints.empty()) {
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
				if (!primitiveD.skinWeights.empty()) {
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
	void setImagesData (GLTFDocument& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder, sxsdk::shade_interface* shade) {
		const size_t imagesCou = sceneData->images.size();

		std::vector<std::string> imageFileNameList;
		imageFileNameList.resize(imagesCou, "");

		for (size_t i = 0; i < imagesCou; ++i) {
			const CImageData& imageD = sceneData->images[i];
			if (!imageD.m_shadeMasterImage) continue;
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
				sxsdk::image_interface *image = imageD.m_shadeMasterImage->get_image();
				if (!image) continue;
				const std::string fileFullPath = (sceneData->getFileDir()) + std::string("/") + fileName;
				image->save(fileFullPath.c_str());

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
					binReader.reset(new GLTFResourceReader(*binStreamReader));

					auto data = binReader->ReadBinaryData(gltfDoc, gltfDoc.images[i]);
					auto imageBufferView = bufferBuilder->AddBufferView(data);

					Image newImage(gltfDoc.images[i]);
					newImage.bufferViewId = imageBufferView.id;

					// mimeの指定 (image/jpeg , image/png).
					const std::string extStr = StringUtil::getFileExtension(newImage.uri);
					newImage.mimeType = std::string("image/");
					if (extStr == "jpg" || extStr == "jpeg") newImage.mimeType += "jpeg";
					else newImage.mimeType += "png";

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
	int setSkinData (GLTFDocument& gltfDoc,  const CSceneData* sceneData, const int meshAccessorIDCount) {
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
	int setAnimationData (GLTFDocument& gltfDoc,  const CSceneData* sceneData, const int skinAccessorIDCount) {
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
				anim.channels.push_back(animChannel);
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
		GLTFDocument gltfDoc;
		gltfDoc.images.Clear();
	    gltfDoc.buffers.Clear();
		gltfDoc.bufferViews.Clear();
		gltfDoc.accessors.Clear();

		// GLB出力用.
		auto glbStreamFactory = std::make_unique<OutputGLBStreamFactory>(fileDir2, fileName2);

		std::unique_ptr<BufferBuilder> glbBuilder;
		if (sceneData->getFileExtension() == "glb") {
			auto glbWriter = std::make_unique<GLBResourceWriter2>(std::move(glbStreamFactory), filePath2);
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
		    auto outputWriter = dynamic_cast<GLBResourceWriter2 *>(&glbBuilder->GetResourceWriter());
			if (outputWriter) outputWriter->Flush(manifest, std::string(""));

		} else {
			// gltfファイルを出力.
			std::string gltfJson = Serialize(gltfDoc, SerializeFlags::Pretty);
			std::ofstream outStream(filePath2.c_str(), std::ios::trunc | std::ios::out);
			outStream << gltfJson;
			outStream.flush();
		}

		return true;

	} catch (GLTFException e) {
		const std::string errorStr(e.what());
		g_errorMessage = errorStr;
	}

	return false;
}

