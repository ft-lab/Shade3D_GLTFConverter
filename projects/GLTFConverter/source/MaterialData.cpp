/**
 * マテリアル情報.
 */

#include "MaterialData.h"
#include "MathUtil.h"

CMaterialData::CMaterialData ()
{
	clear();
}

CMaterialData::CMaterialData (const CMaterialData& v)
{
	this->name = v.name;
	this->alphaCutOff       = v.alphaCutOff;
	this->alphaMode         = v.alphaMode;
	this->doubleSided       = v.doubleSided;
	this->unlit             = v.unlit;
	this->hasVertexColor    = v.hasVertexColor;
	this->baseColorFactor   = v.baseColorFactor;
	this->emissiveFactor    = v.emissiveFactor;
	this->occlusionStrength = v.occlusionStrength;
	this->normalStrength    = v.normalStrength;
	this->baseColorImageIndex         = v.baseColorImageIndex;
	this->normalImageIndex            = v.normalImageIndex;
	this->emissiveImageIndex          = v.emissiveImageIndex;
	this->metallicRoughnessImageIndex = v.metallicRoughnessImageIndex;
	this->occlusionImageIndex         = v.occlusionImageIndex;

	this->roughnessFactor       = v.roughnessFactor;
	this->metallicFactor        = v.metallicFactor;

	this->shadeMasterSurface    = v.shadeMasterSurface;

	this->baseColorTexCoord          = v.baseColorTexCoord;
	this->normalTexCoord             = v.normalTexCoord;
	this->emissiveTexCoord           = v.emissiveTexCoord;
	this->metallicRoughnessTexCoord  = v.metallicRoughnessTexCoord;
	this->occlusionTexCoord          = v.occlusionTexCoord;

	this->baseColorTexScale         = v.baseColorTexScale;
	this->normalTexScale            = v.normalTexScale;
	this->emissiveTexScale          = v.emissiveTexScale;
	this->metallicRoughnessTexScale = v.metallicRoughnessTexScale;
	this->occlusionTexScale         = v.occlusionTexScale;

	this->textureTransformOffset = v.textureTransformOffset;
	this->textureTransformScale  = v.textureTransformScale;

	this->transparency = v.transparency;

	this->pbrSpecularGlossiness_use                = v.pbrSpecularGlossiness_use;
	this->pbrSpecularGlossiness_diffuseFactor      = v.pbrSpecularGlossiness_diffuseFactor;
	this->pbrSpecularGlossiness_specularFactor     = v.pbrSpecularGlossiness_specularFactor;
	this->pbrSpecularGlossiness_diffuseImageIndex  = v.pbrSpecularGlossiness_diffuseImageIndex;
	this->pbrSpecularGlossiness_specularGlossinessImageIndex = v.pbrSpecularGlossiness_specularGlossinessImageIndex;
	this->pbrSpecularGlossiness_glossinessFactor   = v.pbrSpecularGlossiness_glossinessFactor;
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
	unlit       = false;
	hasVertexColor = false;
	baseColorFactor = sxsdk::rgb_class(1, 1, 1);
	emissiveFactor  = sxsdk::rgb_class(0, 0, 0);
	roughnessFactor  = 1.0f;
	metallicFactor   = 0.0f;
	occlusionStrength = 1.0f;
	normalStrength    = 1.0f;

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

	textureTransformOffset = sxsdk::vec2(0, 0);
	textureTransformScale  = sxsdk::vec2(1, 1);

	transparency = 0.0f;

	pbrSpecularGlossiness_use = false;
	pbrSpecularGlossiness_diffuseFactor  = sxsdk::rgba_class(1, 1, 1 ,1);
	pbrSpecularGlossiness_specularFactor = sxsdk::rgb_class(1, 1, 1);
	pbrSpecularGlossiness_diffuseImageIndex  = -1;
	pbrSpecularGlossiness_specularGlossinessImageIndex = -1;
	pbrSpecularGlossiness_glossinessFactor   = 1.0f;
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
	if ((this->unlit) != v.unlit) return false;
	if ((this->hasVertexColor) != v.hasVertexColor) return false;
	if (!MathUtil::isZero((this->baseColorFactor) - v.baseColorFactor)) return false;
	if (!MathUtil::isZero((this->emissiveFactor) - v.emissiveFactor)) return false;
	if (!MathUtil::isZero((this->roughnessFactor) - v.roughnessFactor)) return false;
	if (!MathUtil::isZero((this->metallicFactor) - v.metallicFactor)) return false;
	if (!MathUtil::isZero((this->occlusionStrength) - v.occlusionStrength)) return false;
	if (!MathUtil::isZero((this->normalStrength) - v.normalStrength)) return false;
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

	if (!MathUtil::isZero((this->transparency) - v.transparency)) return false;

	return true;
}
