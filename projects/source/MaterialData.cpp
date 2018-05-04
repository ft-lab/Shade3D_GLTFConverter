/**
 * マテリアル情報.
 */

#include "MaterialData.h"

CMaterialData::CMaterialData ()
{
	clear();
}

CMaterialData::~CMaterialData ()
{
}

void CMaterialData::clear ()
{
	name = "";
	alphaCutOff = 0.5f;
	alphaMode   = 0;
	doubleSided = false;
	baseColorFactor = sxsdk::rgb_class(1, 1, 1);
	emissionFactor  = sxsdk::rgb_class(1, 1, 1);
	roughnessFactor  = 0.5f;
	metallicFactor   = 0.0f;
	occlusionStrength = 1.0f;

	baseColorImageIndex         = -1;
	normalImageIndex            = -1;
	emissionImageIndex          = -1;
	metallicRoughnessImageIndex = -1;
	occlusionImageIndex         = -1;

	shadeMasterSurface = NULL;
}

