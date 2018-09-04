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
	std::string name;					// 名前.
	int skeletonID;						// スケルトンID.
	int meshIndex;						// 対応するメッシュ番号.
	std::vector<int> joints;			// 対応するノード番号.
	std::vector<sxsdk::mat4> inverseBindMatrices;	// ジョイントごとの逆行列.

public:
	CSkinData ();
	CSkinData (const CSkinData& v);
	~CSkinData ();

    CSkinData& operator = (const CSkinData &v) {
		this->name                = v.name;
		this->skeletonID          = v.skeletonID;
		this->meshIndex           = v.meshIndex;
		this->inverseBindMatrices = v.inverseBindMatrices;
		this->joints              = v.joints;
		return (*this);
    }

	void clear ();
};

#endif
