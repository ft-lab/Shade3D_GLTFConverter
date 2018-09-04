/**
 * GLTF用のメッシュデータ.
 */
#ifndef _MESHDATA_H
#define _MESHDATA_H

#include "GlobalHeader.h"
#include "MorphTargetsData.h"

#include <vector>
#include <string>

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (Shade3Dでの構成).
 */
class CTempMeshData
{
public:
	std::string name;						// 形状名.

	std::vector<sxsdk::vec3> vertices;			// 頂点座標.
	std::vector<sxsdk::vec4> skinWeights;		// 頂点ごとのスキンのウエイト値.
	std::vector< sx::vec<int,4> > skinJoints;	// 頂点ごとのスキンのジョイントインデックス値 (Import時に使用).
	std::vector< sx::vec<void *,4> > skinJointsHandle;	// 頂点ごとのスキンのジョイントのハンドル (Export時に使用).

	std::vector<int> triangleIndices;				// 三角形の頂点インデックス.
	std::vector<int> triangleFaceGroupIndex;		// 三角形ごとのフェイスグループ番号.
	std::vector<sxsdk::vec3> triangleNormals;		// 三角形ごとの法線.
	std::vector<sxsdk::vec2> triangleUV0;			// 三角形ごとのUV0.
	std::vector<sxsdk::vec2> triangleUV1;			// 三角形ごとのUV1.
	std::vector<sxsdk::vec4> triangleColor0;		// 三角形ごとの頂点カラー0.

	int materialIndex;							// 対応するマテリアル番号.
	std::vector<int> faceGroupMaterialIndex;	// フェイスグループごとのマテリアル番号リスト.

	CMorphTargetsData morphTargets;				// Morph Targetsの情報.

public:
	CTempMeshData ();
	CTempMeshData (const CTempMeshData& v);
	~CTempMeshData ();

    CTempMeshData& operator = (const CTempMeshData &v) {
		this->name            = v.name;
		this->vertices        = v.vertices;
		this->skinWeights     = v.skinWeights;
		this->skinJoints      = v.skinJoints;
		this->skinJointsHandle = v.skinJointsHandle;
		this->triangleIndices = v.triangleIndices;
		this->triangleFaceGroupIndex  = v.triangleFaceGroupIndex;
		this->triangleNormals = v.triangleNormals;
		this->triangleUV0     = v.triangleUV0;
		this->triangleUV1     = v.triangleUV1;
		this->triangleColor0  = v.triangleColor0;

		this->materialIndex   = v.materialIndex;
		this->faceGroupMaterialIndex = v.faceGroupMaterialIndex;

		this->morphTargets = v.morphTargets;

		return (*this);
    }

	void clear ();

	/**
	 * 最適化 (不要頂点の除去など).
	 */
	void optimize ();
};

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (GLTFでのPrimitiveの構成).
 */
class CPrimitiveData
{
public:
	std::string name;						// 形状名.

	// vertices/normals/uv0/uv1/skinWeights/skinJoints の要素数は同じ。.
	std::vector<sxsdk::vec3> vertices;		// 頂点座標.
	std::vector<sxsdk::vec3> normals;		// 頂点ごとの法線.
	std::vector<sxsdk::vec2> uv0;			// 頂点ごとのUV0.
	std::vector<sxsdk::vec2> uv1;			// 頂点ごとのUV1.
	std::vector<sxsdk::vec4> color0;		// 頂点ごとのColor0.
	std::vector<sxsdk::vec4> skinWeights;		// 頂点ごとのスキン時のウエイト (最大4つ分).
	std::vector< sx::vec<int,4> > skinJoints;	// 頂点ごとのスキン時に参照するジョイントインデックスリスト (最大4つ分).
	std::vector< sx::vec<void *,4> > skinJointsHandle;	// 頂点ごとのスキンのジョイントのハンドル (Export時に使用).

	std::vector<int> triangleIndices;		// 三角形の頂点インデックス.

	int materialIndex;						// 対応するマテリアル番号.

	CMorphTargetsData morphTargets;			// Morph Targetsの情報.

public:
	CPrimitiveData ();
	CPrimitiveData (const CPrimitiveData& v);
	~CPrimitiveData ();

    CPrimitiveData& operator = (const CPrimitiveData &v) {
		this->name            = v.name;
		this->vertices        = v.vertices;
		this->normals         = v.normals;
		this->uv0             = v.uv0;
		this->uv1             = v.uv1;
		this->color0          = v.color0;
		this->triangleIndices = v.triangleIndices;
		this->materialIndex   = v.materialIndex;
		this->skinWeights     = v.skinWeights;
		this->skinJoints      = v.skinJoints;
		this->skinJointsHandle = v.skinJointsHandle;
		this->morphTargets     = v.morphTargets;

		return (*this);
    }

	void clear ();

	/**
	 * CTempMeshDataからコンバート.
	 */
	void convert (const CTempMeshData& tempMeshData);

	/**
	 * CTempMeshDataからコンバート(フェイスグループを考慮して分離).
	 * @param[in]  tempMeshData        作業用のメッシュ情報.
     * @param[out] primitivesData      プリミティブ情報が返る.
	 * @param[out] faceGroupIndexList  プリミティブごとのフェイスグループ番号が返る.
	 * @return メッシュの数.
	 */
	static int convert (const CTempMeshData& tempMeshData, std::vector<CPrimitiveData>& primitivesData, std::vector<int>& faceGroupIndexList);

	/**
	 * 頂点座標のバウンディングボックスを計算.
	 */
	void calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const;

	/**
	 * 頂点カラー情報で、Alphaを出力する必要があるか.
	 */
	bool hasNeedVertexColorAlpha () const;
};

/**
 * 複数のメッシュを格納 (GLTFでは、Meshの中に複数のPrimitiveを配列で持つ構造になる).
 */
class CMeshData
{
public:
	std::string name;							// Mesh名.
	std::vector<CPrimitiveData> primitives;		// 複数のメッシュ(プリミティブ)情報保持用.

	void* pMeshHandle;							// Shade3Dのポリゴンメッシュクラスのハンドル (Import時に一時使用).

public:
	CMeshData ();
	CMeshData (const CMeshData& v);
	~CMeshData ();

    CMeshData& operator = (const CMeshData &v) {
		this->name        = v.name;
		this->primitives  = v.primitives;
		this->pMeshHandle = v.pMeshHandle;
		return (*this);
    }

	void clear ();

	/**
	 * 複数のPrimitiveを1つのメッシュにまとめる (Import時に使用).
	 * Shade3Dのフェイスグループを使用する1つのポリゴンメッシュにする.
	 * @param[out] tempMeshData  まとめたメッシュ情報を格納.
	 */
	bool mergePrimitives (CTempMeshData& tempMeshData) const;
};

#endif
