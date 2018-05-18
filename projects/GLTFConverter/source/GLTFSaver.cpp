/**
 * GLTFをファイルに保存する.
 */
#include "GLTFSaver.h"

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
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
			for (size_t j = 0; j < nodesCou; ++j) {
				if (i == j) continue;
				const CNodeData& nodeD2 = sceneData->nodes[j];
				if (nodeD2.parentNodeIndex == i) {
					gltfNode.children.push_back(std::to_string(j));
				}
			}
			if (!MathUtil::IsZero(nodeD.translation, fMin)) {
				gltfNode.translation = Vector3(nodeD.translation.x, nodeD.translation.y, nodeD.translation.z);
			}
			if (!MathUtil::IsZero(nodeD.scale - sxsdk::vec3(1, 1, 1), fMin)) {
				gltfNode.scale = Vector3(nodeD.scale.x, nodeD.scale.y, nodeD.scale.z);
			}
			if (!MathUtil::IsZero(nodeD.rotation - sxsdk::quaternion_class::identity, fMin)) {
				gltfNode.rotation = Quaternion(nodeD.rotation.x, nodeD.rotation.y, nodeD.rotation.z, nodeD.rotation.w);
			}

			// メッシュ情報を持つ場合.
			if (nodeD.meshIndex >= 0) {
				gltfNode.meshId = std::to_string(nodeD.meshIndex);
			}

			try {
				gltfDoc.nodes.Append(gltfNode);
			} catch (GLTFException e) { }
		}

		// シーンのルートとして、0番目のノードを指定.
		gltfDoc.defaultSceneId = std::string("0");
	}

	/**
	 * マテリアル情報を指定.
	 */
	void setMaterialsData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
		// dummy.
		Material material;
		material.id          = std::to_string(0);
		material.name        = "default";
		material.doubleSided = false;
		material.alphaMode   = ALPHA_OPAQUE;
		material.emissiveFactor = Color3(0.0f, 0.0f, 0.0f);

		material.metallicRoughness.baseColorFactor = Color4(194.0f / 255.0f, 92.0f / 255.0f, 38.0f / 255.0f, 1.0f);
		material.metallicRoughness.metallicFactor  = 0.0f;
		material.metallicRoughness.roughnessFactor = 0.5f;

		gltfDoc.materials.Append(material);
	}

	/**
	 * メッシュ情報を指定.
	 */
	void setMeshesData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return;

		int accessorID = 0;
		for (size_t meshLoop = 0; meshLoop < meshCou; ++meshLoop) {
			const CMeshData& meshD = sceneData->meshes[meshLoop];
			Mesh mesh;

			mesh.name = meshD.name;
			mesh.id   = std::to_string(meshLoop);

			// MeshPrimitiveに1つのメッシュの情報を格納。accessorのIDを指定していく.
			// indices                 : 三角形の頂点インデックス.
			// attributes - NORMAL     : 法線.
			// attributes - POSITION   : 頂点位置.
			// attributes - TEXCOORD_0 : テクスチャのUV0.
			// attributes - TEXCOORD_1 : テクスチャのUV0.

			MeshPrimitive meshPrimitive;
			//if (meshD.materialIndex >= 0) {
			//	meshPrimitive.materialId = std::to_string(meshD.materialIndex);
			//}
			meshPrimitive.materialId = std::string("0");

			meshPrimitive.indicesAccessorId = std::to_string(accessorID++);
			meshPrimitive.normalsAccessorId   = std::to_string(accessorID++);
			meshPrimitive.positionsAccessorId = std::to_string(accessorID++);
			if (!meshD.uv0.empty()) {
				meshPrimitive.uv0AccessorId = std::to_string(accessorID++);
			}
			if (!meshD.uv1.empty()) {
				meshPrimitive.uv1AccessorId = std::to_string(accessorID++);
			}

			meshPrimitive.mode = MESH_TRIANGLES;		// (4) 三角形情報として格納.

			mesh.primitives.push_back(meshPrimitive);

			// Mesh情報を追加.
			gltfDoc.meshes.Append(mesh);
		}
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
			const CMeshData& meshD = sceneData->meshes[meshLoop];

			// 頂点のバウンディングボックスを計算.
			sxsdk::vec3 bbMin, bbMax;
			meshD.calcBoundingBox(bbMin, bbMax);

			// indicesAccessor.
			{
				// short型で格納.
				const bool storeUShort = (meshD.vertices.size() < 65530);

				AccessorDesc acceDesc;
				acceDesc.accessorType  = TYPE_SCALAR;
				acceDesc.componentType = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
				acceDesc.byteOffset    = byteOffset;
				acceDesc.normalized    = false;

				const size_t byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * meshD.triangleIndices.size();

				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);

				if (storeUShort) {
					std::vector<unsigned short> shortData;
					shortData.resize(meshD.triangleIndices.size());
					for (size_t i = 0; i < meshD.triangleIndices.size(); ++i) {
						shortData[i] = (unsigned short)(meshD.triangleIndices[i]);
					}
					bufferBuilder->AddAccessor(shortData, acceDesc); 
				} else {
					bufferBuilder->AddAccessor(meshD.triangleIndices, acceDesc); 
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

				const size_t byteLength = (sizeof(float) * 3) * meshD.normals.size();

				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
				bufferBuilder->AddAccessor(convert_vec3_to_float(meshD.normals), acceDesc); 

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

				const size_t byteLength = (sizeof(float) * 3) * meshD.vertices.size();

				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
				bufferBuilder->AddAccessor(convert_vec3_to_float(meshD.vertices), acceDesc); 

				byteOffset += byteLength;
				accessorID++;
			}

			// uv0Accessor.
			if (!meshD.uv0.empty()) {
				AccessorDesc acceDesc;
				acceDesc.accessorType  = TYPE_VEC2;
				acceDesc.componentType = COMPONENT_FLOAT;
				acceDesc.byteOffset    = byteOffset;
				acceDesc.normalized    = false;

				const size_t byteLength = (sizeof(float) * 2) * meshD.uv0.size();

				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
				bufferBuilder->AddAccessor(convert_vec2_to_float(meshD.uv0), acceDesc); 

				byteOffset += byteLength;
				accessorID++;
			}

			// uv1Accessor.
			if (!meshD.uv1.empty()) {
				AccessorDesc acceDesc;
				acceDesc.accessorType  = TYPE_VEC2;
				acceDesc.componentType = COMPONENT_FLOAT;
				acceDesc.byteOffset    = byteOffset;
				acceDesc.normalized    = false;

				const size_t byteLength = (sizeof(float) * 2) * meshD.uv1.size();

				bufferBuilder->AddBufferView(gltfDoc.bufferViews.Get(accessorID).target);
				bufferBuilder->AddAccessor(convert_vec2_to_float(meshD.uv1), acceDesc); 

				byteOffset += byteLength;
				accessorID++;
			}
		}
	}

	/**
	 *   Accessor情報（メッシュから三角形の頂点インデックス、法線、UVバッファなどをパックしたもの）を格納.
	 *   Accessor → bufferViews → buffers、と経由して情報をバッファに保持する。.
	 *   拡張子gltfの場合、バッファは外部のbinファイル。.
	 */
	void setBufferData (GLTFDocument& gltfDoc,  const CSceneData* sceneData, std::unique_ptr<BufferBuilder>& bufferBuilder) {
		const size_t meshCou = sceneData->meshes.size();
		if (meshCou == 0) return;

		const std::string fileName = sceneData->getFileName(false);		// 拡張子を除いたファイル名を取得.

		// binの出力ファイル名.
		const std::string binFileName = sceneData->getFileName(false) + std::string(".bin");

		// 出力ディレクトリ.
		const std::string fileDir = sceneData->getFileDir();

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
			const CMeshData& meshD = sceneData->meshes[meshLoop];

			// 頂点のバウンディングボックスを計算.
			sxsdk::vec3 bbMin, bbMax;
			meshD.calcBoundingBox(bbMin, bbMax);

			// indicesAccessor.
			{
				// short型で格納.
				const bool storeUShort = (meshD.vertices.size() < 65530);

				Accessor acce;
				acce.id             = std::to_string(accessorID);
				acce.bufferViewId   = std::to_string(accessorID);
				acce.type           = TYPE_SCALAR;
				acce.componentType  = storeUShort ? COMPONENT_UNSIGNED_SHORT : COMPONENT_UNSIGNED_INT;
				acce.count          = meshD.triangleIndices.size();
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = (storeUShort ? sizeof(unsigned short) : sizeof(int)) * meshD.triangleIndices.size();
				buffV.target     = ELEMENT_ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) {
					if (storeUShort) {
						std::vector<unsigned short> shortData;
						shortData.resize(meshD.triangleIndices.size());
						for (size_t i = 0; i < meshD.triangleIndices.size(); ++i) {
							shortData[i] = (unsigned short)(meshD.triangleIndices[i]);
						}
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(shortData[0]), gltfDoc.accessors[accessorID]);

					} else {
						binWriter->Write(gltfDoc.bufferViews[accessorID], &(meshD.triangleIndices[0]), gltfDoc.accessors[accessorID]);
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
				acce.count          = meshD.normals.size();
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = (sizeof(float) * 3) * meshD.normals.size();
				buffV.target     = ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(meshD.normals[0]), gltfDoc.accessors[accessorID]);

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
				acce.count          = meshD.vertices.size();
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
				buffV.byteLength = (sizeof(float) * 3) * meshD.vertices.size();
				buffV.target     = ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(meshD.vertices[0]), gltfDoc.accessors[accessorID]);

				byteOffset += buffV.byteLength;
				accessorID++;
			}

			// uv0Accessor.
			if (!meshD.uv0.empty()) {
				Accessor acce;
				acce.id             = std::to_string(accessorID);
				acce.bufferViewId   = std::to_string(accessorID);
				acce.type           = TYPE_VEC2;
				acce.componentType  = COMPONENT_FLOAT;
				acce.count          = meshD.uv0.size();
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = (sizeof(float) * 2) * meshD.uv0.size();
				buffV.target     = ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(meshD.uv0[0]), gltfDoc.accessors[accessorID]);

				byteOffset += buffV.byteLength;
				accessorID++;
			}

			// uv1Accessor.
			if (!meshD.uv1.empty()) {
				Accessor acce;
				acce.id             = std::to_string(accessorID);
				acce.bufferViewId   = std::to_string(accessorID);
				acce.type           = TYPE_VEC2;
				acce.componentType  = COMPONENT_FLOAT;
				acce.count          = meshD.uv1.size();
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = (sizeof(float) * 2) * meshD.uv1.size();
				buffV.target     = ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

				// バッファ情報として格納.
				if (binWriter) binWriter->Write(gltfDoc.bufferViews[accessorID], &(meshD.uv1[0]), gltfDoc.accessors[accessorID]);

				byteOffset += buffV.byteLength;
				accessorID++;
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
}

CGLTFSaver::CGLTFSaver ()
{
}

/**
 * 指定のGLTFファイルを出力.
 * @param[in]  fileName    出力ファイル名 (gltf/glb).
 * @param[in]  sceneData   GLTFのシーン情報.
 */
bool CGLTFSaver::saveGLTF (const std::string& fileName, const CSceneData* sceneData)
{
	std::string json = "";

	try {
		GLTFDocument gltfDoc;
		gltfDoc.images.Clear();
	    gltfDoc.buffers.Clear();
		gltfDoc.bufferViews.Clear();
		gltfDoc.accessors.Clear();

		// GLB出力用.
		auto glbStreamFactory = std::make_unique<OutputGLBStreamFactory>(sceneData->getFileDir(), sceneData->getFileName());

		std::unique_ptr<BufferBuilder> glbBuilder;
		if (sceneData->getFileExtension() == "glb") {
			auto glbWriter = std::make_unique<GLBResourceWriter2>(std::move(glbStreamFactory), sceneData->filePath);
			glbBuilder = std::make_unique<BufferBuilder>(std::move(glbWriter));
			glbBuilder->AddBuffer(GLB_BUFFER_ID);
		}

		// ヘッダ部を指定.
		gltfDoc.asset.generator = sceneData->assetGenerator;
		gltfDoc.asset.version   = sceneData->assetVersion;		// GLTFバージョン.
		gltfDoc.asset.copyright = sceneData->assetCopyRight;

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
		::setMeshesData(gltfDoc, sceneData);

		// バッファ情報を指定.
		// 拡張子がgltfの場合、binファイルもここで出力.
		// 拡張子がglbの場合、glbBuilderにバッファ情報を格納.
		::setBufferData(gltfDoc, sceneData, glbBuilder);

		if (glbBuilder) {
			gltfDoc.buffers.Clear();
			gltfDoc.bufferViews.Clear();
			gltfDoc.accessors.Clear();

			// glbファイルを出力.
			glbBuilder->Output(gltfDoc);	// glbBuilderの情報をgltfDocに反映.

			auto manifest     = Serialize(gltfDoc);
		    auto outputWriter = dynamic_cast<GLBResourceWriter2 *>(&glbBuilder->GetResourceWriter());
			if (outputWriter) outputWriter->Flush(manifest, std::string(""));

		} else {
			// gltfファイルを出力.
			std::string gltfJson = Serialize(gltfDoc, SerializeFlags::Pretty);
			std::ofstream outStream(fileName.c_str(), std::ios::trunc | std::ios::out);
			outStream << gltfJson;
			outStream.flush();
		}

		return true;

	} catch (GLTFException e) { }

	return false;
}

