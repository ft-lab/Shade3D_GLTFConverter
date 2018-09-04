/**
 * Morph Targets情報.
 */
#include "MorphTargetsData.h"

//-------------------------------------------------.
COneMorphTargetData::COneMorphTargetData ()
{
	clear();
}
COneMorphTargetData::COneMorphTargetData (const COneMorphTargetData& v)
{
	this->position = v.position;
	this->normal   = v.normal;
	this->tangent  = v.tangent;
	this->weight   = v.weight;
	this->vIndices = v.vIndices;

	this->name     = v.name;
}

COneMorphTargetData::~COneMorphTargetData ()
{
}

void COneMorphTargetData::clear ()
{
	position.clear();
	normal.clear();
	tangent.clear();
	vIndices.clear();
	weight = 0.0f;
	name = "";
}

//-------------------------------------------------.

CMorphTargetsData::CMorphTargetsData ()
{
	clear();
}

CMorphTargetsData::CMorphTargetsData (const CMorphTargetsData& v)
{
	this->morphTargetsData = v.morphTargetsData;
}

CMorphTargetsData::~CMorphTargetsData ()
{
}

void CMorphTargetsData::clear()
{
	morphTargetsData.clear();
}
