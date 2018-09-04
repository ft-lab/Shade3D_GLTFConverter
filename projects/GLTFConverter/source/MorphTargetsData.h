/**
 * Morph Targets情報.
 * Morph TargetsはPrimitiveごとに保持される.
 * glTFの場合は、1Primitiveの頂点数分のNormnal/Position/Tangentを保持できる.
 */
#ifndef _MORPHTARGETSDATA_H
#define _MORPHTARGETSDATA_H

#include "GlobalHeader.h"
#include <vector>

//-------------------------------------------------.
/**
 * MorphTarget 1つ分の情報.
 * Primitiveの頂点数分の情報を持つ.
 */
class COneMorphTargetData
{
public:
	std::vector<sxsdk::vec3> position;	// 位置.
	std::vector<sxsdk::vec3> normal;	// 法線.
	std::vector<sxsdk::vec3> tangent;	// Tangent.

	float weight;						// ウエイト値.

	std::vector<int> vIndices;			// 参照する頂点番号 (Shade3Dに渡す際に、最適化として使用).

	std::string name;					// 名前 (VRM拡張).

public:
	COneMorphTargetData ();

    COneMorphTargetData& operator = (const COneMorphTargetData &v) {
		this->position = v.position;
		this->normal   = v.normal;
		this->tangent  = v.tangent;
		this->weight   = v.weight;
		this->vIndices = v.vIndices;

		this->name     = v.name;

		return (*this);
	}

	void clear ();
};

//-------------------------------------------------.
/**
 * MorphTargets情報.
 */
class CMorphTargetsData
{
public:
	std::vector<COneMorphTargetData> morphTargetsData;		// Targetのリスト.

public:
	CMorphTargetsData ();
    CMorphTargetsData& operator = (const CMorphTargetsData &v) {
		this->morphTargetsData = v.morphTargetsData;
		return (*this);
	}

	void clear();
};

#endif
