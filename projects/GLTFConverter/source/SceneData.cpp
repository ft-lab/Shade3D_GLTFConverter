/**
 * GLTFのシーンデータ.
 */

#include "SceneData.h"
#include "StringUtil.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

//---------------------------------------------.
CNodeData::CNodeData ()
{
	clear();
}
CNodeData::~CNodeData ()
{
}

void CNodeData::clear ()
{
	name = "";
	meshIndex = -1;
	skinIndex = -1;
	prevNodeIndex = -1;
	nextNodeIndex = -1;
	childNodeIndex = -1;
	parentNodeIndex = -1;
	isBone = false;
	pShapeHandle = NULL;

	translation = sxsdk::vec3(0, 0, 0);
	scale       = sxsdk::vec3(1, 1, 1);
	rotation    = sxsdk::quaternion_class();
}

/**
 * 変換行列を取得.
 */
sxsdk::mat4 CNodeData::getMatrix () const
{
	sxsdk::mat4 m = sxsdk::mat4::identity;

	sxsdk::vec3 rotE;
	rotation.get_euler(rotE);
	m = sxsdk::mat4::scale(scale) * sxsdk::mat4::rotate(rotE) * sxsdk::mat4::translate(translation);
	return m;
}

//---------------------------------------------.
CSceneData::CSceneData ()
{
	clear();
}

void CSceneData::clear()
{
	assetVersion    = "";
	assetGenerator  = "";
	assetCopyRight  = "";
	filePath        = "";

	nodes.clear();
	materials.clear();
	meshes.clear();
	images.clear();
	skins.clear();
	m_nodeStack.clear();
	animations.clear();
}

/**
 * ファイル名を除いたディレクトリを取得.
 */
const std::string CSceneData::getFileDir () const
{
	return StringUtil::getFileDir(filePath);
}

/**
 * ファイルパスからファイル名のみを取得.
 * @param[in] hasExtension  trueの場合は拡張子も付ける.
 */
const std::string CSceneData::getFileName (const bool hasExtension) const
{
	return StringUtil::getFileName(filePath, hasExtension);
}

/**
 * ファイルパスから拡張子を取得 (gltfまたはglb).
 */
const std::string CSceneData::getFileExtension () const
{
	return StringUtil::getFileExtension(filePath);
}

/**
 * 作業フォルダに画像ファイルを出力.
 * @param[in] imageIndex  画像番号.
 * @param[in] tempPath    作業用フォルダ.
 * @return 出力ファイル名. 
 */
std::string CSceneData::outputTempImage (const int imageIndex, const std::string& tempPath)
{
	if (imageIndex < 0 || imageIndex >= images.size()) return "";
	const CImageData& imageD = images[imageIndex];

	// ファイル名.
	std::string name = (imageD.name == "") ? (std::string("image_") + std::to_string(imageIndex)) : imageD.name;
	if (imageD.mimeType.find("/png") > 0) {
		name += ".png";
	} else {
		name += ".jpg";
	}

	// フルパスファイル名.
	std::string fileName = tempPath + std::string("/") + name;

	try {
		std::ofstream outStream(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
		outStream.write((char *)&(imageD.imageDatas[0]), imageD.imageDatas.size());
		outStream.flush();
	} catch (...) {
		return "";
	}

	return fileName;
}

/**
 * ノードの変換行列を取得.
 * @param[in] nodeIndex  ノード番号.
 * @param[in] unitMtoMM  メートルからミリメートルに単位変換する場合はtrue.
 * @return 変換行列.
 */
sxsdk::mat4 CSceneData::getNodeMatrix (const int nodeIndex, const bool unitMtoMM)
{
	const CNodeData& nodeD = nodes[nodeIndex];

	const sxsdk::mat4 mTrans = sxsdk::mat4::translate(nodeD.translation * (unitMtoMM ? 1000.0f : 1.0f));
	const sxsdk::mat4 mScale = sxsdk::mat4::scale(nodeD.scale);
	const sxsdk::mat4 mRot   = sxsdk::mat4(nodeD.rotation);
	
	return (mScale * mRot * mTrans);
}

/**
 * 新しいメッシュデータを追加.
 * @return メッシュ番号.
 */
int CSceneData::appendNewMeshData ()
{
	const int index = (int)meshes.size();
	meshes.push_back(CMeshData());
	return index;
}

/**
 * メッシュデータを取得.
 * @param[in] meshIndex  メッシュ番号.
 * @return メッシュデータクラス.
 */
CMeshData& CSceneData::getMeshData (const int meshIndex)
{
	return meshes[meshIndex];
}

const CMeshData& CSceneData::getMeshData (const int meshIndex) const
{
	return meshes[meshIndex];
}

/* **************************************************************************** */
/*   エクスポート時の、データ格納用.											*/
/* **************************************************************************** */
/**
 * ノードの格納開始.
 * @param[in] nodeName  ノード名.
 * @param[in] m         変換行列.
 * @return ノード番号.
 */
int CSceneData::beginNode (const std::string& nodeName, const sxsdk::mat4 m)
{
	const int nodeIndex = (int)nodes.size();
	nodes.push_back(CNodeData());
	CNodeData& nodeD = nodes.back();
	nodeD.name = nodeName;

	// 親のノード番号.
	nodeD.parentNodeIndex = m_nodeStack.empty() ? -1 : (int)m_nodeStack.back();

	// 変換行列の格納.
	{
		sxsdk::vec3 scale ,shear, rotate, trans;
		m.unmatrix(scale ,shear, rotate, trans);
		nodeD.translation = trans * 0.001f;			// mm ==> m 変換.
		nodeD.scale       = scale;
		nodeD.rotation    = sxsdk::quaternion_class(rotate);
	}

	int prevNodeIndex = -1;

	// 親ノードから、子の末尾のノード番号を取得.
	if (nodeD.parentNodeIndex >= 0) {
		CNodeData& parentNodeD = nodes[nodeD.parentNodeIndex];

		if (parentNodeD.childNodeIndex > 0) {
			prevNodeIndex = parentNodeD.childNodeIndex;
			CNodeData* pNode = &(nodes[prevNodeIndex]);
			while ((pNode->nextNodeIndex) > 0) {
				prevNodeIndex = pNode->nextNodeIndex;
				pNode = &(nodes[prevNodeIndex]);
			}
		} else {
			parentNodeD.childNodeIndex = nodeIndex;
		}

		nodeD.prevNodeIndex = prevNodeIndex;
	}

	m_nodeStack.push_back(nodeIndex);

	return nodeIndex;
}

/**
 * ノードの格納終了.
 * @param[in] removeF  ノードを削除する場合はtrue.
 */
void CSceneData::endNode (const bool removeF)
{
	if (m_nodeStack.empty()) return;
	m_nodeStack.pop_back();

	if (removeF) {
		const int nodeIndex = (int)nodes.size() - 1;
		const int parentNodeIndex = nodes[nodeIndex].parentNodeIndex;
		const int prevNodeIndex   = nodes[nodeIndex].prevNodeIndex;

		if (parentNodeIndex >= 0) {
			if (nodes[parentNodeIndex].childNodeIndex == nodeIndex) {
				nodes[parentNodeIndex].childNodeIndex = -1;
			}
		}
		if (prevNodeIndex >= 0) {
			if (nodes[prevNodeIndex].nextNodeIndex == nodeIndex) {
				nodes[prevNodeIndex].nextNodeIndex = -1;
			}
		}
		nodes.pop_back();
	}
}

/**
 * 現在処理中のカレントノード番号を取得.
 */
int CSceneData::getCurrentNodeIndex () const
{
	if (m_nodeStack.empty()) return -1;
	return m_nodeStack.back();
}

/**
 * 末尾の2つのメッシュを1つにマージする.
 */
void CSceneData::mergeLastTwoMeshes ()
{
	const size_t meshesCou = meshes.size();
	if (meshesCou <= 1) return;

	CPrimitiveData& mesh1 = meshes[meshesCou - 2].primitives[0];
	const CPrimitiveData& mesh2 = meshes[meshesCou - 1].primitives[0];

	const size_t vOffset = mesh1.vertices.size();
	const size_t vCou    = mesh2.vertices.size();
	const size_t triVCou = mesh2.triangleIndices.size();

	for (size_t i = 0; i < vCou; ++i) {
		mesh1.vertices.push_back(mesh2.vertices[i]);
	}
	if (mesh2.normals.size() == vCou) {
		for (size_t i = 0; i < vCou; ++i) {
			mesh1.normals.push_back(mesh2.normals[i]);
		}
	}
	if (mesh2.uv0.size() == vCou) {
		if (mesh1.uv0.size() != vOffset) {
			for (size_t i = mesh1.uv0.size(); i < vOffset; ++i) {
				mesh1.uv0.push_back(sxsdk::vec2(0, 0));
			}
		}
		for (size_t i = 0; i < vCou; ++i) {
			mesh1.uv0.push_back(mesh2.uv0[i]);
		}
	} else {
		if (!mesh1.uv0.empty()) {
			if (mesh1.uv0.size() != vOffset + vCou) {
				for (size_t i = mesh1.uv0.size(); i < vOffset + vCou; ++i) {
					mesh1.uv0.push_back(sxsdk::vec2(0, 0));
				}
			}
		}
	}

	if (mesh2.uv1.size() == vCou) {
		if (mesh1.uv1.size() != vOffset) {
			for (size_t i = mesh1.uv1.size(); i < vOffset; ++i) {
				mesh1.uv1.push_back(sxsdk::vec2(0, 0));
			}
		}

		for (size_t i = 0; i < vCou; ++i) {
			mesh1.uv1.push_back(mesh2.uv1[i]);
		}
	} else {
		if (!mesh1.uv1.empty()) {
			if (mesh1.uv1.size() != vOffset + vCou) {
				for (size_t i = mesh1.uv1.size(); i < vOffset + vCou; ++i) {
					mesh1.uv1.push_back(sxsdk::vec2(0, 0));
				}
			}
		}
	}

	for (size_t i = 0; i < triVCou; ++i) {
		mesh1.triangleIndices.push_back(mesh2.triangleIndices[i] + vOffset);
	}

	meshes.pop_back();
}

/**
 * 同一のマテリアルがあるか調べる.
 * @param[in] materialData  マテリアル情報.
 * @return 同一のマテリアルがある場合はマテリアルのインデックスを返す。.
 *         存在しない場合は-1を返す.
 */
int CSceneData::findSameMaterial (const CMaterialData& materialData)
{
	const size_t mCou = materials.size();
	int index = -1;
	for (size_t i = 0; i < mCou; ++i) {
		if (materials[i].isSame(materialData)) {
			index = (int)i;
			break;
		}
	}
	return index;
}

/**
 * 同一のイメージがあるか調べる.
 * @param[in] imageData  イメージ情報.
 * @return 同一のイメージがある場合はイメージのインデックスを返す。.
 *         存在しない場合は-1を返す.
 */
int CSceneData::findSameImage (const CImageData& imageData)
{
	const size_t iCou = images.size();
	int index = -1;
	for (size_t i = 0; i < iCou; ++i) {
		if (images[i].isSame(imageData)) {
			index = (int)i;
			break;
		}
	}
	return index;
}

/**
 * 他とかぶらないユニークなマテリアル名を取得.
 * @param[in] name  格納したいマテリアル名.
 * @return 他とはかぶらないマテリアル名.
 */
std::string CSceneData::getUniqueMaterialName (const std::string& name)
{
	const size_t mCou = materials.size();

	bool findF = false;
	for (size_t i = 0; i < mCou; ++i) {
		const CMaterialData& matD = materials[i];
		if (matD.name == name) {
			findF = true;
			break;
		}
	}
	if (!findF) return name;

	std::string newName = "";
	int iCou = 1;
	while (true) {
		findF = false;
		const std::string name2 = name + std::string("_") + std::to_string(iCou);
		for (size_t i = 0; i < mCou; ++i) {
			const CMaterialData& matD = materials[i];
			if (matD.name == name2) {
				findF = true;
				break;
			}
		}
		if (!findF) {
			newName = name2;
			break;
		}
		iCou++;
	}

	return newName;
}

/**
 * ローカル座標からワールド座標の変換行列を取得
 * @param[in] nodeIndex  ノード番号.
 * @return 変換行列.
 */
sxsdk::mat4 CSceneData::getLocalToWorldMatrix (const int nodeIndex) const
{
	sxsdk::mat4 lwMat = sxsdk::mat4::identity;

	int nodeIndex2 = nodeIndex;
	while (nodeIndex2 >= 0) {
		const CNodeData& nodeD = nodes[nodeIndex2];
		lwMat = lwMat * nodeD.getMatrix();
		nodeIndex2 = nodeD.parentNodeIndex;
	}
	return lwMat;
}
