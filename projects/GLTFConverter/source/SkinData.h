/**
 * スキンデータ.
 */

#ifndef _SKINDATA_H
#define _SKINDATA_H

#include "GlobalHeader.h"

#include <vector>

class CSkinData
{
public:
	int inverseBindMatrices;			// バインド情報（accessorsの参照インデックス）.
	std::string name;					// 名前.
	int skeletonID;						// スケルトンID.
	std::vector<int> joints;			// 対応するノード番号.

public:
	CSkinData ();

    CSkinData& operator = (const CSkinData &v) {
		this->name                = v.name;
		this->skeletonID          = v.skeletonID;
		this->inverseBindMatrices = v.inverseBindMatrices;
		this->joints              = v.joints;
		return (*this);
    }

	void clear ();
};

#endif
