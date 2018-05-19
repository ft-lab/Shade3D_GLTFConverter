/**
 * マテリアル情報.
 */

#include "MaterialData.h"
#include "MathUtil.h"

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
	alphaMode   = 1;
	doubleSided = false;
	baseColorFactor = sxsdk::rgb_class(1, 1, 1);
	emissionFactor  = sxsdk::rgb_class(0, 0, 0);
	roughnessFactor  = 1.0f;
	metallicFactor   = 0.0f;
	occlusionStrength = 1.0f;

	baseColorImageIndex         = -1;
	normalImageIndex            = -1;
	emissionImageIndex          = -1;
	metallicRoughnessImageIndex = -1;
	occlusionImageIndex         = -1;

	shadeMasterSurface = NULL;
}

/**
 * マテリアルが同じかチェック (Shade3DからのGLTFエクスポートで使用).
 */
bool CMaterialData::isSame (const CMaterialData& v) const
{
	if ((this->shadeMasterSurface) && v.shadeMasterSurface) {
		if ((this->shadeMasterSurface->get_handle()) == v.shadeMasterSurface->get_handle()) return true;
	}

	if (!MathUtil::IsZero((this->alphaCutOff) - v.alphaCutOff)) return false;
	if ((this->alphaMode) != v.alphaMode) return false;
	if ((this->doubleSided) != v.doubleSided) return false;
	if (!MathUtil::IsZero((this->baseColorFactor) - v.baseColorFactor)) return false;
	if (!MathUtil::IsZero((this->emissionFactor) - v.emissionFactor)) return false;
	if (!MathUtil::IsZero((this->roughnessFactor) - v.roughnessFactor)) return false;
	if (!MathUtil::IsZero((this->metallicFactor) - v.metallicFactor)) return false;
	if (!MathUtil::IsZero((this->occlusionStrength) - v.occlusionStrength)) return false;
	if ((this->baseColorImageIndex) != v.baseColorImageIndex) return false;
	if ((this->normalImageIndex) != v.normalImageIndex) return false;
	if ((this->emissionImageIndex) != v.emissionImageIndex) return false;
	if ((this->metallicRoughnessImageIndex) != v.metallicRoughnessImageIndex) return false;
	if ((this->occlusionImageIndex) != v.occlusionImageIndex) return false;

	return true;
}
