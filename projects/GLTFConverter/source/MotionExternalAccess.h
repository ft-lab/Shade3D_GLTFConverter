/**
 * 外部アクセス用の宣言.
 */
#ifndef _EXTERNALACCESS_H
#define _EXTERNALACCESS_H

#include "sxsdk.cxx"

#include <vector>

/*
	関数の引数としてstd::stringやstd::vector<>を使うと、Release/Debugの混在でうまく連携できない模様.
	そのため、char *、ポインタ参照、などで渡す.
	また、新しい関数の追加がある場合は末尾に追加していくこと。.
	引数の変更の場合は、呼び出し元と呼び出す側で全体入れ替えが必要.
*/

/**
 * プラグインインターフェイス派生クラスのID.
 */
// CHiddenMorphTargetsInterfaceクラス.
#define HIDDEN_MORPH_TARGETS_INTERFACE_ID sx::uuid_class("B66C31FF-3886-438C-85A3-7F916A465360")

// CHiddenBoneUtilInterfaceクラス.
#define HIDDEN_BONE_UTIL_INTERFACE_ID sx::uuid_class("0AB5E6B6-F3DE-4C86-B6C0-F0246FF87330")

/**
 * ベジェ曲線のポイント情報.
 */
class CBezierPoint2D {
public:
	sxsdk::vec2 point;			// ポイント.
	sxsdk::vec2 inHandle;		// inハンドル.
	sxsdk::vec2 outHandle;		// outハンドル.

public:
	CBezierPoint2D () {
		point = inHandle = outHandle = sxsdk::vec2(0, 0);
	}
};

//----------------------------------------------------------------------.
/**
 * ボーン機能へのアクセス.
 */
class CBoneAttributeAccess : public sxsdk::attribute_interface
{
public:
	virtual ~CBoneAttributeAccess() { }

	/**
	 * 指定の形状がボーンかどうか.
	 */
	virtual bool isBone (sxsdk::shape_class& shape) = 0;

	/**
	 * ボーンのワールド座標での中心位置とボーンサイズを取得.
	 */
	virtual sxsdk::vec3 getBoneCenter (sxsdk::shape_class& shape, float *size) = 0;

	/**
	 * ボーンの向きをそろえる.
	 * @param[in] boneRoot  対象のボーンルート.
	 */
	virtual void adjustBonesDirection (sxsdk::shape_class* boneRoot) = 0;

	/**
	 * ボーンサイズの自動調整.
	 * @param[in] boneRoot  対象のボーンルート.
	 */
	virtual void resizeBones (sxsdk::shape_class* boneRoot) = 0;
};

//----------------------------------------------------------------------.
/**
 * Morph Targets機能へのアクセス.
 */
class CMorphTargetsAccess : public sxsdk::attribute_interface
{
public:
	virtual ~CMorphTargetsAccess () { }

	//---------------------------------------------------------------.
	// Stream保存/読み込み用.
	//---------------------------------------------------------------.
	/**
	 * Morph Targets情報を持つか.
	 */
	virtual bool hasMorphTargets (sxsdk::shape_class& shape) = 0;

	/**
	 * 現在のMorph Targets情報をstreamに保存.
	 */
	virtual void writeMorphTargetsData () = 0;

	/**
	 * Morph Targets情報をstreamから読み込み.
	 */
	virtual bool readMorphTargetsData (sxsdk::shape_class& shape) = 0;

	//---------------------------------------------------------------.
	// 登録/編集用.
	//---------------------------------------------------------------.
	/**
	 * 対象形状のポインタを取得.
	 */
	virtual sxsdk::shape_class* getTargetShape () = 0;

	/**
	 * オリジナルの頂点座標数を取得.
	 */
	virtual int getOrgVerticesCount () const = 0;

	/**
	 * オリジナルの頂点座標を取得.
	 */
	virtual int getOrgVertices (sxsdk::vec3* vertices) const = 0;

	/**
	 * 対象のポリゴンメッシュ形状クラスを渡す.
	 * これは変形前のもので、これを呼び出した後に位置移動した選択頂点をtargetとして登録していく.
	 * @param[in] pShaoe   対象形状.
	 */
	virtual bool setupShape (sxsdk::shape_class* pShape) = 0;

	/**
	 * baseの頂点座標を格納。streamからの読み込み時に呼ばれる.
	 * @param[in] vCou      頂点数.
	 * @param[in] vertices  登録する頂点座標.
	 */
	virtual void setOrgVertices (const int vCou, const sxsdk::vec3* vertices) = 0;

	/**
	 * 選択頂点座標をMorphTargetsの頂点として追加.
	 * @param[in] name      target名.
	 * @param[in] vCou      頂点数.
	 * @param[in] indices   登録する頂点インデックス (選択された頂点).
	 * @param[in] vertices  登録する頂点座標.
	 * @return Morph Targets番号.
	 */
	virtual int appendTargetVertices (const char* name, const int vCou, const int* indices, const sxsdk::vec3* vertices) = 0;

	/**
	 * Morph Targetsの数.
	 */
	virtual int getTargetsCount () const = 0;

	/**
	 * Morph Targetの名前を取得.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @param[out] name      名前が入る.
	 */
	virtual bool getTargetName (const int tIndex, char* name) const = 0;

	/**
	 * Morph Targetの名前を指定.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @param[in]  name      名前.
	 */
	virtual void setTargetName (const int tIndex, const char* name) = 0;

	/**
	 * Morph Targetsの頂点数を取得.
	 * @param[in]  tIndex    Morph Targets番号.
	 */
	virtual int getTargetVerticesCount (const int tIndex) = 0;

	/**
	 * Morph Targetsの頂点座標を取得.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @param[out] indices   頂点インデックスが返る.
	 * @param[out] vertices  頂点座標が返る.
	 */
	virtual bool getTargetVertices (const int tIndex, int* indices, sxsdk::vec3* vertices) = 0;

	/**
	 * Morph Targetsのウエイト値を指定.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @param[in]  weight    ウエイト値(0.0 - 1.0).
	 */
	virtual bool setTargetWeight (const int tIndex, const float weight) = 0;

	/**
	 * Morph Targetsのウエイト値を取得.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @return ウエイト値.
	 */
	virtual float getTargetWeight (const int tIndex) const = 0;

	/**
	 * 指定のMorph Target情報を削除.
	 * @param[in]  tIndex    Morph Targets番号.
	 */
	virtual bool removeTarget (const int tIndex) = 0;

	/**
	 * Morph Targets情報を削除して、元の頂点に戻す.
	 * @param[in] restoreVertices  頂点を元に戻す場合はtrue.
	 */
	virtual void removeMorphTargets (const bool restoreVertices = true) = 0;

	/**
	 * すべてのウエイト値を0にする。.
	 * この後にupdateMeshを呼ぶと、ウエイト前の頂点となる.
	 */
	virtual void setZeroAllWeight () = 0;

	/**
	 * 重複頂点をマージする.
	 * ポリゴンメッシュの「sxsdk::polygon_mesh_class::cleanup_redundant_vertices」と同等で、Morph Targetsも考慮したもの.
	 */
	virtual bool cleanupRedundantVertices (sxsdk::shape_class& shape) = 0;

	/**
	 * Morph Targetsの情報より、m_pTargetShapeのポリゴンメッシュを更新.
	 */
	virtual void updateMesh () = 0;

	/**
	 * シーンのすべての形状で、Morph Targets情報を持つ形状のウエイト値を一時保持.
	 * (いったんすべてのウエイト値を0にして戻す、という操作で使用).
	 */
	virtual void pushAllWeight (sxsdk::scene_interface* scene, const bool setZeroWeight = false) = 0;

	/**
	 * シーンのすべての形状のMorph Targets情報のウエイト値を戻す.
	 */
	virtual void popAllWeight (sxsdk::scene_interface* scene) = 0;

	/**
	 * 指定の形状の(Cacheでの)カレントウエイト値を取得.
	 * @param[in]  tIndex    Morph Targets番号.
	 * @param[out] weights   targetごとのウエイト値が返る.
	 */
	virtual bool getShapeCurrentWeights (const sxsdk::shape_class* shape, float* weights) = 0;

};

//----------------------------------------------------------------------.
/**
 * CBoneAttributeAccessクラスの参照を取得.
 */
CBoneAttributeAccess* getBoneAttributeAccess (sxsdk::shade_interface& shade);

/**
 * CMorphTargetsAccessクラスの参照を取得.
 */
CMorphTargetsAccess* getMorphTargetsAttributeAccess (sxsdk::shade_interface& shade);

#endif
