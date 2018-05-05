/**
 * GLTFのシーンデータ.
 */
#ifndef _SCENEDATA_H
#define _SCENEDATA_H

#include "GlobalHeader.h"
#include "MeshData.h"
#include "MaterialData.h"
#include "ImageData.h"

#include <vector>
#include <string>

//---------------------------------------------.
/**
 * シーン階層を表現するノード情報.
 */
class CNodeData
{
public:
	std::string name;		// 名前.
	int meshIndex;			// 参照するメッシュ番号.

	int prevNodeIndex;			// 1つ前のノード番号.
	int nextNodeIndex;			// 1つ次のノード番号.
	int childNodeIndex;			// 子のノード番号.
	int parentNodeIndex;		// 親のノード番号.

	sxsdk::vec3 translation;			// 位置.
	sxsdk::vec3 scale;					// スケール.
	sxsdk::quaternion_class rotation;	// 回転.

public:
	CNodeData ();
	~CNodeData ();

    CNodeData& operator = (const CNodeData &v) {
		this->name            = v.name;
		this->meshIndex       = v.meshIndex;
		this->prevNodeIndex   = v.prevNodeIndex;
		this->nextNodeIndex   = v.nextNodeIndex;
		this->childNodeIndex  = v.childNodeIndex;
		this->parentNodeIndex = v.parentNodeIndex;
		this->translation     = v.translation;
		this->scale           = v.scale;
		this->rotation        = v.rotation;

		return (*this);
    }

	void clear ();
};

//---------------------------------------------.
/**
 * シーン情報の保持用.
 */
class CSceneData
{
public:
	std::string assetVersion;				// Assetバージョン.
	std::string assetGenerator;				// 作成ツール.
	std::string assetCopyRight;				// Copy Right.

	std::string filePath;					// ファイルフルパス.
	std::string fileName;					// ファイル名.

	std::vector<CNodeData> nodes;			// ノード情報.

	std::vector<CMaterialData> materials;	// マテリアル情報の配列.
	std::vector<CMeshData> meshes;			// メッシュ情報の配列.
	std::vector<CImageData> images;			// 画像情報の配列.

public:
	CSceneData ();

	void clear();

	/**
	 * ファイル名を除いたディレクトリを取得.
	 */
	const std::string getFileDir () const;

	/**
	 * 作業フォルダに画像ファイルを出力.
	 * @param[in] imageIndex  画像番号.
	 * @param[in] tempPath    作業用フォルダ.
	 * @return 出力ファイル名. 
	 */
	std::string outputTempImage (const int imageIndex, const std::string& tempPath);
};

#endif
