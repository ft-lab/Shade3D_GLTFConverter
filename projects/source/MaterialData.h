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

	sxsdk::rgb_class baseColorFactor;	// BaseColorの色.
	sxsdk::rgb_class emissionFactor;	// 発光の色.

	// Metallic-Roughness Material modelの場合、.
	// baseColor / Occlusion-Roughness-Metallic / normal がテクスチャの基本要素.
	int baseColorImageIndex;			// BaseColorとしての画像番号.
	int normalImageIndex;				// 法線マップとしての画像番号.
	int emissionImageIndex;				// 発光としての画像番号.
	int metallicRoughnessImageIndex;	// Metallic/Roughnessとしての画像番号.
	int occlusionImageIndex;			// Ambient Occlusionとしての画像番号 (metallicRoughnessImageIndexの画像の要素を参照している模様).

	sxsdk::master_surface_class* shadeMasterSurface;	// Shadeでの対応するマスターサーフェスのポインタ.

	float roughnessFactor;				// Roughnessの値.
	float metallicFactor;				// Metallicの値.
	float occlusionStrength;			// Occlusionの強さ.

public:
	CMaterialData ();
	~CMaterialData ();

    CMaterialData& operator = (const CMaterialData &v) {
		this->name = v.name;
		this->alphaCutOff       = v.alphaCutOff;
		this->alphaMode         = v.alphaMode;
		this->doubleSided       = v.doubleSided;
		this->baseColorFactor   = v.baseColorFactor;
		this->emissionFactor    = v.emissionFactor;
		this->occlusionStrength = v.occlusionStrength;
		this->baseColorImageIndex         = v.baseColorImageIndex;
		this->normalImageIndex            = v.normalImageIndex;
		this->emissionImageIndex          = v.emissionImageIndex;
		this->metallicRoughnessImageIndex = v.metallicRoughnessImageIndex;
		this->occlusionImageIndex         = v.occlusionImageIndex;

		this->roughnessFactor       = v.roughnessFactor;
		this->metallicFactor        = v.metallicFactor;

		this->shadeMasterSurface    = v.shadeMasterSurface;

		return (*this);
    }

	void clear ();
};

#endif
