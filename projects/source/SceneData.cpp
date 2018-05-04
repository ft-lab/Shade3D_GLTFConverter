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
	fileName        = "";

	materials.clear();
	meshes.clear();
	images.clear();
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
