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
	inverseBindMatrices = -1;
	joints.clear();
}
