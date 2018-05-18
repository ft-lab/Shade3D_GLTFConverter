﻿/**
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
private:
	std::vector<int> m_nodeStack;			// ノードを格納する際の階層構造のためのスタック.

public:
	std::string assetVersion;				// Assetバージョン.
	std::string assetGenerator;				// 作成ツール.
	std::string assetCopyRight;				// Copy Right.

	std::string filePath;					// ファイルフルパス.

	std::vector<CNodeData> nodes;			// ノード情報.

	std::vector<CMaterialData> materials;	// マテリアル情報の配列.
	std::vector<CMeshData> meshes;			// メッシュ情報の配列.
	std::vector<CImageData> images;			// 画像情報の配列.

private:


public:
	CSceneData ();

	void clear();

	/**
	 * ファイルパスからファイル名のみを取得.
	 * @param[in] hasExtension  trueの場合は拡張子も付ける.
	 */
	const std::string getFileName (const bool hasExtension = true) const;

	/**
	 * ファイル名を除いたディレクトリを取得.
	 */
	const std::string getFileDir () const;

	/**
	 * ファイルパスから拡張子を取得 (gltfまたはglb).
	 */
	const std::string getFileExtension () const;

	/**
	 * ノードの変換行列を取得.
	 * @param[in] nodeIndex  ノード番号.
	 * @param[in] unitMtoMM  メートルからミリメートルに単位変換する場合はtrue.
	 * @return 変換行列.
	 */
	sxsdk::mat4 getNodeMatrix (const int nodeIndex, const bool unitMtoMM = true);

	/**
	 * 作業フォルダに画像ファイルを出力.
	 * @param[in] imageIndex  画像番号.
	 * @param[in] tempPath    作業用フォルダ.
	 * @return 出力ファイル名. 
	 */
	std::string outputTempImage (const int imageIndex, const std::string& tempPath);

	/* **************************************************************************** */
	/*   エクスポート時の、データ格納用.											*/
	/* **************************************************************************** */
	/**
	 * ノードの格納開始.
	 * @param[in] nodeName  ノード名.
	 * @param[in] m         変換行列.
	 * @return ノード番号.
	 */
	int beginNode (const std::string& nodeName, const sxsdk::mat4 m = sxsdk::mat4::identity);

	/**
	 * ノードの格納終了.
	 */
	void endNode ();

	/**
	 * 現在処理中のカレントノード番号を取得.
	 */
	int getCurrentNodeIndex () const;
};

#endif
