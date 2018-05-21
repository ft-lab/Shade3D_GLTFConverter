/**
 * GLTF用のメッシュデータ.
 */
#ifndef _MESHDATA_H
#define _MESHDATA_H

#include "GlobalHeader.h"

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

	std::vector<sxsdk::vec3> vertices;		// 頂点座標.

	std::vector<int> triangleIndices;				// 三角形の頂点インデックス.
	std::vector<int> faceGroupIndex;				// 三角形ごとのフェイスグループ番号.
	std::vector<sxsdk::vec3> triangleNormals;		// 三角形ごとの法線.
	std::vector<sxsdk::vec2> triangleUV0;			// 三角形ごとのUV0.
	std::vector<sxsdk::vec2> triangleUV1;			// 三角形ごとのUV1.

	int materialIndex;						// 対応するマテリアル番号.

public:
	CTempMeshData ();
	~CTempMeshData ();

    CTempMeshData& operator = (const CTempMeshData &v) {
		this->name            = v.name;
		this->vertices        = v.vertices;
		this->triangleIndices = v.triangleIndices;
		this->faceGroupIndex  = v.faceGroupIndex;
		this->triangleNormals = v.triangleNormals;
		this->triangleUV0     = v.triangleUV0;
		this->triangleUV1     = v.triangleUV1;

		this->materialIndex   = v.materialIndex;

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
 * 1つのメッシュ情報 (GLTFでの構成).
 */
class CMeshData
{
public:
	std::string name;						// 形状名.

	// vertices/normals/uv0/uv1 の要素数は同じ。.
	std::vector<sxsdk::vec3> vertices;		// 頂点座標.
	std::vector<sxsdk::vec3> normals;		// 頂点ごとの法線.
	std::vector<sxsdk::vec2> uv0;			// 頂点ごとのUV0.
	std::vector<sxsdk::vec2> uv1;			// 頂点ごとのUV1.

	std::vector<int> triangleIndices;		// 三角形の頂点インデックス.

	int materialIndex;						// 対応するマテリアル番号.

public:
	CMeshData ();
	~CMeshData ();

    CMeshData& operator = (const CMeshData &v) {
		this->name            = v.name;
		this->vertices        = v.vertices;
		this->normals         = v.normals;
		this->uv0             = v.uv0;
		this->uv1             = v.uv1;
		this->triangleIndices = v.triangleIndices;
		this->materialIndex   = v.materialIndex;

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
	 * @param[out] meshData            メッシュ情報が返る.
	 * @param[out] faceGroupIndexList  メッシュごとのフェイスグループ番号が返る.
	 * @return メッシュの数.
	 */
	static int convert (const CTempMeshData& tempMeshData, std::vector<CMeshData>& meshData, std::vector<int>& faceGroupIndexList);

	/**
	 * 頂点座標のバウンディングボックスを計算.
	 */
	void calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const;
};

#endif
