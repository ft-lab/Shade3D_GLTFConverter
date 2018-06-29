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
	emissiveFactor  = sxsdk::rgb_class(0, 0, 0);
	roughnessFactor  = 1.0f;
	metallicFactor   = 0.0f;
	occlusionStrength = 1.0f;

	baseColorImageIndex         = -1;
	normalImageIndex            = -1;
	emissiveImageIndex          = -1;
	metallicRoughnessImageIndex = -1;
	occlusionImageIndex         = -1;

	shadeMasterSurface = NULL;

	baseColorTexCoord         = 0;
	normalTexCoord            = 0;
	emissiveTexCoord          = 0;
	metallicRoughnessTexCoord = 0;
	occlusionTexCoord         = 0;

	baseColorTexScale         = sxsdk::vec2(1, 1);
	normalTexScale            = sxsdk::vec2(1, 1);
	emissiveTexScale          = sxsdk::vec2(1, 1);
	metallicRoughnessTexScale = sxsdk::vec2(1, 1);
	occlusionTexScale         = sxsdk::vec2(1, 1);
}

/**
 * マテリアルが同じかチェック (Shade3DからのGLTFエクスポートで使用).
 */
bool CMaterialData::isSame (const CMaterialData& v) const
{
	if ((this->shadeMasterSurface) && v.shadeMasterSurface) {
		if ((this->shadeMasterSurface->get_handle()) == v.shadeMasterSurface->get_handle()) return true;

		// マスターサーフェス同士の場合は、名前が一致しない場合は同じでないとみなす.
		const std::string name1(this->shadeMasterSurface->get_name());
		const std::string name2(v.shadeMasterSurface->get_name());
		if (name1 != name2) return false;
	}

	if (!MathUtil::isZero((this->alphaCutOff) - v.alphaCutOff)) return false;
	if ((this->alphaMode) != v.alphaMode) return false;
	if ((this->doubleSided) != v.doubleSided) return false;
	if (!MathUtil::isZero((this->baseColorFactor) - v.baseColorFactor)) return false;
	if (!MathUtil::isZero((this->emissiveFactor) - v.emissiveFactor)) return false;
	if (!MathUtil::isZero((this->roughnessFactor) - v.roughnessFactor)) return false;
	if (!MathUtil::isZero((this->metallicFactor) - v.metallicFactor)) return false;
	if (!MathUtil::isZero((this->occlusionStrength) - v.occlusionStrength)) return false;
	if ((this->baseColorImageIndex) != v.baseColorImageIndex) return false;
	if ((this->normalImageIndex) != v.normalImageIndex) return false;
	if ((this->emissiveImageIndex) != v.emissiveImageIndex) return false;
	if ((this->metallicRoughnessImageIndex) != v.metallicRoughnessImageIndex) return false;
	if ((this->occlusionImageIndex) != v.occlusionImageIndex) return false;

	if ((this->baseColorTexCoord) != v.baseColorTexCoord) return false;
	if ((this->normalTexCoord) != v.normalTexCoord) return false;
	if ((this->emissiveTexCoord) != v.emissiveTexCoord) return false;
	if ((this->metallicRoughnessTexCoord) != v.metallicRoughnessTexCoord) return false;
	if ((this->occlusionTexCoord) != v.occlusionTexCoord) return false;

	if ((this->baseColorTexScale)         != v.baseColorTexScale) return false;
	if ((this->normalTexScale)            != v.normalTexScale) return false;
	if ((this->emissiveTexScale)          != v.emissiveTexScale) return false;
	if ((this->metallicRoughnessTexScale) != v.metallicRoughnessTexScale) return false;
	if ((this->occlusionTexScale)         != v.occlusionTexScale) return false;

	return true;
}
