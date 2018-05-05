/**
 * GLTF用のメッシュデータ.
 */
#ifndef _MESHDATA_H
#define _MESHDATA_H

#include "GlobalHeader.h"

#include <vector>
#include <string>

/**
 * 1つのメッシュ情報.
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
};

#endif
