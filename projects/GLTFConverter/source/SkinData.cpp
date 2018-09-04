/**
 * スキンデータ.
 */

#include "SkinData.h"

CSkinData::CSkinData ()
{
	clear();
}
CSkinData::CSkinData (const CSkinData& v)
{
	this->name                = v.name;
	this->skeletonID          = v.skeletonID;
	this->meshIndex           = v.meshIndex;
	this->inverseBindMatrices = v.inverseBindMatrices;
	this->joints              = v.joints;
}

CSkinData::~CSkinData ()
{
}

void CSkinData::clear ()
{
	name = "";
	skeletonID = -1;
	meshIndex  = -1;
	inverseBindMatrices.clear();
	joints.clear();
}
