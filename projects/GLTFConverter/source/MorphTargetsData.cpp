/**
 * Morph Targets情報.
 */
#include "MorphTargetsData.h"

//-------------------------------------------------.
COneMorphTargetData::COneMorphTargetData ()
{
	clear();
}

void COneMorphTargetData::clear ()
{
	position.clear();
	normal.clear();
	tangent.clear();
	vIndices.clear();
	weight = 0.0f;
}

//-------------------------------------------------.

CMorphTargetsData::CMorphTargetsData ()
{
	clear();
}

void CMorphTargetsData::clear()
{
	morphTargetsData.clear();
}
