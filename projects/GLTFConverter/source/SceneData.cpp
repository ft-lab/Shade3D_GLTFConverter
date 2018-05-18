/**
 * GLTFのシーンデータ.
 */

#include "SceneData.h"

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
	prevNodeIndex = -1;
	nextNodeIndex = -1;
	childNodeIndex = -1;
	parentNodeIndex = -1;

	translation = sxsdk::vec3(0, 0, 0);
	scale       = sxsdk::vec3(1, 1, 1);
	rotation    = sxsdk::quaternion_class();
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
	fileName        = "";
	filePath        = "";

	nodes.clear();
	materials.clear();
	meshes.clear();
	images.clear();
	m_nodeStack.clear();
}

/**
 * ファイル名を除いたディレクトリを取得.
 */
const std::string CSceneData::getFileDir () const
{
	int iPos  = filePath.find_last_of("/");
	int iPos2 = filePath.find_last_of("\\");
	if (iPos == std::string::npos && iPos2 == std::string::npos) return filePath;
	if (iPos != std::string::npos && iPos2 != std::string::npos) {
		iPos = std::max(iPos, iPos2);
	} else if (iPos == std::string::npos) iPos = iPos2;
	if (iPos == std::string::npos) return filePath;

	return filePath.substr(0, iPos);
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
		nodeD.translation = trans;
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
 */
void CSceneData::endNode ()
{
	if (m_nodeStack.empty()) return;
	m_nodeStack.pop_back();
}

/**
 * 現在処理中のカレントノード番号を取得.
 */
int CSceneData::getCurrentNodeIndex () const
{
	if (m_nodeStack.empty()) return -1;
	return m_nodeStack.back();
}

