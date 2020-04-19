/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#ifndef _IMAGESBLEND_H
#define _IMAGESBLEND_H

#include "GlobalHeader.h"


class CImagesBlend
{
public:
	// ベイク時のエラー.
	enum IMAGE_BAKE_RESULT {
		bake_success = 0,				// ベイク成功.
		bake_error_mixed_uv_layer,		// UVレイヤが混在.
		bake_mixed_repeat,				// 繰り返し回数が混在しているのでまとめた.
	};

private:
	sxsdk::scene_interface* m_pScene;
	sxsdk::surface_class* m_surface;

	// sxsdk::image_interfaceで確保した一時イメージは、明示的にReleaseで解放する必要がある.
	sxsdk::image_interface* m_diffuseImage;				// Diffuseの画像.
	sxsdk::image_interface* m_normalImage;				// Normalの画像.
	sxsdk::image_interface* m_reflectionImage;			// Reflectionの画像.
	sxsdk::image_interface* m_roughnessImage;			// Roughnessの画像.
	sxsdk::image_interface* m_glowImage;				// Glowの画像.
	sxsdk::image_interface* m_occlusionImage;			// Occlusionの画像.
	sxsdk::image_interface* m_transparencyImage;		// Transparencyの画像.
	sxsdk::image_interface* m_opacityMaskImage;			// 不透明マスク相当の画像.

	sxsdk::image_interface* m_glTFMetallicRoughnessImage;		// glTFでのMetallicRoughness画像.

	std::string m_diffuseImageName;								// Diffuseの画像名.
	std::string m_normalImageName;								// Normalの画像名.
	std::string m_reflectionImageName;							// Reflectionの画像名.
	std::string m_roughnessImageName;							// Roughnessの画像名.
	std::string m_glowImageName;								// Glowの画像名.
	std::string m_occlusionImageName;							// Occlusionの画像名.
	std::string m_transparencyImageName;						// Transparencyの画像名.
	std::string m_opacityMaskImageName;							// 不透明マスク相当の画像名.

	bool m_hasDiffuseImage;										// diffuseのイメージを持つか.
	bool m_hasReflectionImage;									// reflectionのイメージを持つか.
	bool m_hasRoughnessImage;									// roughnessのイメージを持つか.
	bool m_hasNormalImage;										// normalのイメージを持つか.
	bool m_hasGlowImage;										// glowのイメージを持つか.
	bool m_hasOcclusionImage;									// Occlusionのイメージを持つか.
	bool m_hasTransparencyImage;								// transparencyのイメージを持つか.
	bool m_hasOpacityMaskImage;									// 不透明マスクのイメージを持つか.

	sx::vec<int,2> m_diffuseRepeat;								// Diffuseの反復回数.
	sx::vec<int,2> m_normalRepeat;								// Normalの反復回数.
	sx::vec<int,2> m_reflectionRepeat;							// Reflectionの反復回数.
	sx::vec<int,2> m_roughnessRepeat;							// Roughnessの反復回数.
	sx::vec<int,2> m_glowRepeat;								// Glowの反復回数.
	sx::vec<int,2> m_occlusionRepeat;							// Occlusionの反復回数.
	sx::vec<int,2> m_transparencyRepeat;						// Transparencyの反復回数.
	sx::vec<int,2> m_opacityMaskRepeat;							// OpacityMaskの反復回数.

	int m_diffuseTexCoord;										// DiffuseのUVレイヤ番号.
	int m_normalTexCoord;										// NormalのUVレイヤ番号.
	int m_reflectionTexCoord;									// ReflectionのUVレイヤ番号.
	int m_roughnessTexCoord;									// RoughnessのUVレイヤ番号.
	int m_glowTexCoord;											// GlowのUVレイヤ番号.
	int m_occlusionTexCoord;									// OcclusionのUVレイヤ番号.
	int m_transparencyTexCoord;									// TransparencyのUVレイヤ番号.
	int m_opacityMaskTexCoord;									// OpacityMaskのUVレイヤ番号.

	bool m_diffuseAlphaTrans;									// Diffuseのアルファ透明を使用しているか.

	int m_diffuseTexturesCount;									// Diffuseテクスチャ数.
	int m_normalTexturesCount;									// 法線マップのテクスチャ数.

	bool m_useDiffuseAlpha;										// Diffuseの「アルファ透明」を使用しているか.

	GLTFConverter::alpha_mode_type m_alphaModeType;				// AlphaModeの種類.
	float m_alphaCutoff;										// AlphaCutoff値は、Diffuseテクスチャの属性より取得.

	bool m_useOcclusionInMetallicRoughnessTexture;				// OcclusionテクスチャをMetallic-Roughnessテクスチャにまとめている場合.

	// テクスチャを使用しない場合のパラメータ.
	sxsdk::rgb_class m_diffuseColor;							// Diffuse色.
	sxsdk::rgb_class m_emissiveColor;							// Emmisive色.
	float m_metallic;											// Metallic値.
	float m_roughness;											// Roughness値.
	float m_normalStrength;										// 法線の強さ.
	float m_transparency;										// 透明度.

private:
	/**
	 * 指定のテクスチャの合成処理.
	 * @param[in] mappingType  マッピングの種類.
	 * @param[in] repeatTex    繰り返し回数.
	 */
	bool m_blendImages (const sxsdk::enums::mapping_type mappingType, const sx::vec<int,2>& repeatTex);

	/**
	 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * @param[in]  mappingType   マッピングの種類.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkImage (const sxsdk::enums::mapping_type mappingType,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

	/**
	 * 指定のマッピングの種類でのテクスチャサイズの最大を取得.
	 * 異なるサイズのテクスチャが混在する場合、一番大きいサイズのテクスチャに合わせる.
	 * @param[in]  mappingType   マッピングの種類.
	 */
	sx::vec<int,2> m_getMaxMappingImageSize (const sxsdk::enums::mapping_type mappingType);

	/**
	 * マッピングレイヤで「マット」を持つか.
	 * @param[in]  mappingType   マッピングの種類.
	 */
	 bool m_hasWeightTexture (const sxsdk::enums::mapping_type mappingType);

	 /**
	  * テクスチャを複製する場合に、反転や繰り返し回数を考慮.
	  * @param[in] image       イメージクラス.
	  * @param[in] dstSize     複製後のサイズ.
	  * @param[in] flipColor   色反転.
	  * @param[in] flipH       左右反転.
	  * @param[in] flipV       上下反転.
	  * @param[in] rotate90    90度反転.
	  * @param[in] repeatU     繰り返し回数U.
	  * @param[in] repeatV     繰り返し回数V.
	  */
	 compointer<sxsdk::image_interface> m_duplicateImage (sxsdk::image_interface* image, const sx::vec<int,2>& dstSize,
		 const bool flipColor = false, const bool flipH = false, const bool flipV = false, const bool rotate90 = false,
		 const int repeatU = 1, const int repeatV = 1);

	/**
	 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
	 */
	void m_convShade3DToPBRMaterial ();

	/**
	 * glTFのMetallic-Roughnessのパック処理 (Occlusion(R)/Roughness(G)/Metallic(B)).
	 */
	void m_packOcclusionRoughnessMetallicImage ();

public:
	CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface);
	~CImagesBlend ();

	void clear ();

	/**
	 * 個々のイメージを合成.
	 * @return ベイクの結果.
	 */
	IMAGE_BAKE_RESULT blendImages ();

	/**
	 * 各種イメージを持つか (単一または複数).
	 */
	bool hasImage (const sxsdk::enums::mapping_type mappingType) const;

	/**
	 * イメージを取得.
	 */
	sxsdk::image_interface* getImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージ名を取得.
	 */
	std::string getImageName (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージのUV層番号を取得.
	 */
	int getTexCoord (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの反復回数を取得.
	 */
	sx::vec<int,2> getImageRepeat (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの強度を色として取得.
	 */
	sxsdk::rgb_class getImageFactor (const sxsdk::enums::mapping_type mappingType);

	/**
	 * アルファ透明を使用しているか.
	 */
	bool getDiffuseAlphaTrans () const { return m_diffuseAlphaTrans; }

	/**
	 * 法線マップの強さを取得.
	 */
	float getNormalStrength () const { return m_normalStrength; }

	/**
	 * glTFのMetallic-Roughnessイメージを取得.
	 */
	sxsdk::image_interface* getMetallicRoughnessImage ();

	/**
	 * AlphaModeの種類を取得.
	 */
	GLTFConverter::alpha_mode_type getAlphaModeType () const { return m_alphaModeType; }

	/**
	 * AlphaModeのCutoff値を取得.
	 */
	float getAlphaCutoff () const { return m_alphaCutoff; }

	/**
	 * 透明度を取得.
	 */
	float getTransparency () const { return m_transparency; }

	/**
	 * OcclusionテクスチャがMetallicRoughnessに内包されるか.
	 */
	bool getUseOcclusionInMetallicRoughnessTexture () const { return m_useOcclusionInMetallicRoughnessTexture; }

};

#endif
