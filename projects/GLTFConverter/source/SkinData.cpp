/**
 * スキンデータ.
 */

#include "SkinData.h"

CSkinData::CSkinData ()
{
	clear();
}

void CSkinData::clear ()
{
	name = "";
	skeletonID = -1;
	meshIndex  = -1;
	inverseBindMatrices.clear();
	joints.clear();
}
