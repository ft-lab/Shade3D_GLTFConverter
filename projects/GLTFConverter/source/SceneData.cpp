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
