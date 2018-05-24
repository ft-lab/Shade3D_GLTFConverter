/**
 * スキンデータ.
 */

#include "SkinData.h"

CSkinData::CSkinData ()
{
}

void CSkinData::clear ()
{
	name = "";
	skeletonID = -1;
	inverseBindMatrices = -1;
	joints.clear();
}
