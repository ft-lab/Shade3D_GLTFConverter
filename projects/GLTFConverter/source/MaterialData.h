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
										// Metallic(B) / Roughness(G) を使用。これはGLTF2.0の仕様として決まっている.
										// Ambient Occlusionも含む指定の場合は、(R)をAOとして使用することになる.
	int occlusionImageIndex;			// Ambient Occlusionとしての画像番号.

	sxsdk::master_surface_class* shadeMasterSurface;	// Shade3Dでの対応するマスターサーフェスのポインタ.

	float roughnessFactor;				// Roughnessの値.
	float metallicFactor;				// Metallicの値.
	float occlusionStrength;			// Occlusionの強さ.

	int baseColorTexCoord;				// BaseColorのテクスチャで使用するTexCoord(0 or 1).
	int normalTexCoord;					// 法線マップのテクスチャで使用するTexCoord(0 or 1).
	int emissionTexCoord;				// 発光のテクスチャで使用するTexCoord(0 or 1).
	int metallicRoughnessTexCoord;		// Metallic(B) / Roughness(G) のテクスチャで使用するTexCoord(0 or 1).
	int occlusionTexCoord;				// Ambient Occlusionのテクスチャで使用するTexCoord(0 or 1).

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

		this->baseColorTexCoord          = v.baseColorTexCoord;
		this->normalTexCoord             = v.normalTexCoord;
		this->emissionTexCoord           = v.emissionTexCoord;
		this->metallicRoughnessTexCoord  = v.metallicRoughnessTexCoord;
		this->occlusionTexCoord          = v.occlusionTexCoord;

		return (*this);
    }

	void clear ();

	/**
	 * マテリアルが同じかチェック (Shade3DからのGLTFエクスポートで使用).
	 */
	bool isSame (const CMaterialData& v) const;

};

#endif
