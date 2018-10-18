/**
 * マテリアル情報.
 */
#ifndef _MATERIALDATA_H
#define _MATERIALDATA_H

#include "GlobalHeader.h"

#include <vector>
#include <string>

/**
 * 1つのマテリアルデータ.
 */
class CMaterialData
{
public:
	std::string name;					// マテリアル名.
	float alphaCutOff;					// Alphaのカットオフの敷居値.
	int alphaMode;						// 合成モード(0:UNKNOWN , 1:OPAQUE, 2:BLEND, 3:MASK).
	bool doubleSided;					// 両面表示するか.
	bool unlit;							// 陰影付けしない.

	sxsdk::rgb_class baseColorFactor;	// BaseColorの色.
	sxsdk::rgb_class emissiveFactor;	// 発光の色.

	// Metallic-Roughness Material modelの場合、.
	// baseColor / Occlusion-Roughness-Metallic / normal がテクスチャの基本要素.
	int baseColorImageIndex;			// BaseColorとしての画像番号.
	int normalImageIndex;				// 法線マップとしての画像番号.
	int emissiveImageIndex;				// 発光としての画像番号.
	int metallicRoughnessImageIndex;	// Metallic/Roughnessとしての画像番号.
										// Metallic(B) / Roughness(G) を使用。これはGLTF2.0の仕様として決まっている.
										// Ambient Occlusionも含む指定の場合は、(R)をAOとして使用することになる.
	int occlusionImageIndex;			// Ambient Occlusionとしての画像番号.

	sxsdk::master_surface_class* shadeMasterSurface;	// Shade3Dでの対応するマスターサーフェスのポインタ.

	float roughnessFactor;				// Roughnessの値.
	float metallicFactor;				// Metallicの値.
	float occlusionStrength;			// Occlusionの強さ.
	float normalStrength;				// Normalの強さ.

	int baseColorTexCoord;				// BaseColorのテクスチャで使用するTexCoord(0 or 1).
	int normalTexCoord;					// 法線マップのテクスチャで使用するTexCoord(0 or 1).
	int emissiveTexCoord;				// 発光のテクスチャで使用するTexCoord(0 or 1).
	int metallicRoughnessTexCoord;		// Metallic(B) / Roughness(G) のテクスチャで使用するTexCoord(0 or 1).
	int occlusionTexCoord;				// Ambient Occlusionのテクスチャで使用するTexCoord(0 or 1).

	// Tiling用のスケール値 (glTF 2.0ではTextureごとのextensionsにKHR_texture_transform のscaleとして追加).
	sxsdk::vec2 baseColorTexScale;
	sxsdk::vec2 normalTexScale;
	sxsdk::vec2 emissiveTexScale;
	sxsdk::vec2 metallicRoughnessTexScale;
	sxsdk::vec2 occlusionTexScale;

	float transparency;					// Shade3Dでの透明度.

public:
	CMaterialData ();
	CMaterialData (const CMaterialData& v);
	~CMaterialData ();

    CMaterialData& operator = (const CMaterialData &v) {
		this->name = v.name;
		this->alphaCutOff       = v.alphaCutOff;
		this->alphaMode         = v.alphaMode;
		this->doubleSided       = v.doubleSided;
		this->unlit             = v.unlit;
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

		this->transparency = v.transparency;

		return (*this);
    }

	void clear ();

	/**
	 * マテリアルが同じかチェック (Shade3DからのGLTFエクスポートで使用).
	 */
	bool isSame (const CMaterialData& v) const;

};

#endif
