/**
 * GLTFをファイルに保存する.
 */
#include "GLTFSaver.h"

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/Serialize.h>
#include <GLTFSDK/GLTFResourceWriter.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/GLTFResourceReader.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace Microsoft::glTF;

// 以下はGLTFSDK/rapidjsonのincludeよりも後に指定しないとビルドエラーになる.
#include "SceneData.h"
#include "MathUtil.h"

namespace {
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
			//meshPrimitive.materialId = std::to_string();
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
	 * Accessor情報（メッシュから三角形の頂点インデックス、法線、UVバッファなどをパックしたもの）を格納.
	 *   Accessor → bufferViews → buffers、と経由して情報をバッファに保持する。.
	 *  バッファは外部のbinファイル.
	 */
	void setAccessorBufferData (GLTFDocument& gltfDoc,  const CSceneData* sceneData) {
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
				Accessor acce;
				acce.id             = std::to_string(accessorID);
				acce.bufferViewId   = std::to_string(accessorID);
				acce.type           = TYPE_SCALAR;
				acce.componentType  = COMPONENT_UNSIGNED_INT;
				acce.count          = meshD.triangleIndices.size();
				gltfDoc.accessors.Append(acce);

				BufferView buffV;
				buffV.id         = std::to_string(accessorID);
				buffV.bufferId   = std::string("0");
				buffV.byteOffset = byteOffset;
				buffV.byteLength = sizeof(int) * meshD.triangleIndices.size();
				buffV.target     = ELEMENT_ARRAY_BUFFER;
				gltfDoc.bufferViews.Append(buffV);

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

				byteOffset += buffV.byteLength;
				accessorID++;
			}
		}

		// buffers.
		{
			Buffer buff;
			buff.id         = std::string("0");
			buff.byteLength = byteOffset;
			gltfDoc.buffers.Append(buff);
		}
	}
}

CGLTFSaver::CGLTFSaver ()
{
}

/**
 * 指定のGLTFファイルを出力.
 * @param[in]  fileName    出力ファイル名 (glb).
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

		// ノード情報を指定.
		::setNodesData(gltfDoc, sceneData);

		// メッシュ情報を指定.
		::setMeshesData(gltfDoc, sceneData);

		// バッファ情報を指定.
		::setAccessorBufferData(gltfDoc, sceneData);

		std::string gltfJson = Serialize(gltfDoc, SerializeFlags::Pretty);
		std::ofstream outStream(fileName.c_str(), std::ios::trunc | std::ios::out);
		outStream << gltfJson;
		outStream.flush();
		return true;

	} catch (...) { }

	return false;
}

