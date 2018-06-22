/**
 * GLTFのシーンデータ.
 */
#ifndef _SCENEDATA_H
#define _SCENEDATA_H

#include "GlobalHeader.h"
#include "MeshData.h"
#include "MaterialData.h"
#include "ImageData.h"
#include "SkinData.h"
#include "AnimationData.h"

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
	int skinIndex;			// 参照するスキン番号.

	int prevNodeIndex;			// 1つ前のノード番号.
	int nextNodeIndex;			// 1つ次のノード番号.
	int childNodeIndex;			// 子のノード番号.
	int parentNodeIndex;		// 親のノード番号.

	sxsdk::vec3 translation;			// 位置.
	sxsdk::vec3 scale;					// スケール.
	sxsdk::quaternion_class rotation;	// 回転.

	bool isBone;					// ボーンノードとして使用している場合はtrue (skinsのjoints要素で参照されているかで判別).
	void* pShapeHandle;				// Import/Export時のShade3Dでの形状のhandleの参照 (パートまたはボーン).

public:
	CNodeData ();
	~CNodeData ();

    CNodeData& operator = (const CNodeData &v) {
		this->name            = v.name;
		this->meshIndex       = v.meshIndex;
		this->skinIndex       = v.skinIndex;
		this->prevNodeIndex   = v.prevNodeIndex;
		this->nextNodeIndex   = v.nextNodeIndex;
		this->childNodeIndex  = v.childNodeIndex;
		this->parentNodeIndex = v.parentNodeIndex;
		this->translation     = v.translation;
		this->scale           = v.scale;
		this->rotation        = v.rotation;
		this->isBone          = v.isBone;
		this->pShapeHandle    = v.pShapeHandle;

		return (*this);
    }

	void clear ();

	/**
	 * 変換行列を取得.
	 */
	sxsdk::mat4 getMatrix () const;
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

	// 以下、Oculus Homeで指定されているもの.
	std::string assetExtrasTitle;			// オブジェクト名.
	std::string assetExtrasAuthor;			// 作者.
	std::string assetExtrasLicense;			// ライセンス.
	std::string assetExtrasSource;			// 原型モデルの参照先.


	std::string filePath;					// ファイルフルパス.

	std::vector<CNodeData> nodes;			// ノード情報.

	std::vector<CMaterialData> materials;	// マテリアル情報の配列.
	std::vector<CMeshData> meshes;			// メッシュ情報の配列 (GLTFのMesh).
	std::vector<CImageData> images;			// 画像情報の配列.
	std::vector<CSkinData> skins;			// スキン情報の配列.

	CAnimationData animations;				// アニメーション情報クラス.

	CExportDlgParam exportParam;			// エクスポート時のパラメータ.

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

	/**
	 * 新しいメッシュを追加.
	 * @return メッシュ番号.
	 */
	int appendNewMeshData ();

	/**
	 * メッシュデータを取得.
	 * @param[in] meshIndex  メッシュ番号.
	 * @return メッシュデータクラス.
	 */
	CMeshData& getMeshData (const int meshIndex);
	const CMeshData& getMeshData (const int meshIndex) const;

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
	 * @param[in] removeF  ノードを削除する場合はtrue.
	 */
	void endNode (const bool removeF = false);

	/**
	 * 末尾の2つのメッシュを1つにマージする.
	 */
	void mergeLastTwoMeshes ();

	/**
	 * 現在処理中のカレントノード番号を取得.
	 */
	int getCurrentNodeIndex () const;

	/**
	 * 同一のマテリアルがあるか調べる.
	 * @param[in] materialData  マテリアル情報.
	 * @return 同一のマテリアルがある場合はマテリアルのインデックスを返す。.
	 *         存在しない場合は-1を返す.
	 */
	int findSameMaterial (const CMaterialData& materialData);

	/**
	 * 同一のイメージがあるか調べる.
	 * @param[in] imageData  イメージ情報.
	 * @return 同一のイメージがある場合はイメージのインデックスを返す。.
	 *         存在しない場合は-1を返す.
	 */
	int findSameImage (const CImageData& imageData);

	/**
	 * 他とかぶらないユニークなマテリアル名を取得.
	 * @param[in] name  格納したいマテリアル名.
	 * @return 他とはかぶらないマテリアル名.
	 */
	std::string getUniqueMaterialName (const std::string& name);

	/**
	 * ローカル座標からワールド座標の変換行列を取得
	 * @param[in] nodeIndex  ノード番号.
	 * @return 変換行列.
	 */
	sxsdk::mat4 getLocalToWorldMatrix (const int nodeIndex) const;

	/**
	 * ノード番号に対応するSkinのInverseBindMatrixを取得.
	 * @param[in]  nodeIndex          ノード番号.
	 * @param[out] inverseBindMatrix  変換行列.
	 */
	bool getSkinsInverseBindMatrix (const int nodeIndex, sxsdk::mat4& inverseBindMatrix) const;

	/**
	 * glTFでの変換行列をShade3Dのミリメートル単位に変換.
	 */
	sxsdk::mat4 convMatrixToShade3D (const sxsdk::mat4& m) const;
};

#endif
