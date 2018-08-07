/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#ifndef _IMAGESBLEND_H
#define _IMAGESBLEND_H

#include "GlobalHeader.h"

class CImagesBlend
{
private:
	sxsdk::scene_interface* m_pScene;
	sxsdk::surface_class* m_surface;

	compointer<sxsdk::image_interface> m_diffuseImage;			// Diffuseの画像.
	compointer<sxsdk::image_interface> m_normalImage;			// Normalの画像.
	compointer<sxsdk::image_interface> m_reflectionImage;		// Reflectionの画像.
	compointer<sxsdk::image_interface> m_roughnessImage;		// Roughnessの画像.
	compointer<sxsdk::image_interface> m_glowImage;				// Glowの画像.
	compointer<sxsdk::image_interface> m_occlusionImage;		// Occlusionの画像.

	compointer<sxsdk::image_interface> m_gltfBaseColorImage;			// glTFとしてエクスポートするBaseColorの画像.
	compointer<sxsdk::image_interface> m_gltfMetallicRoughnessImage;	// glTFとしてエクスポートするMetallic/Roughnessの画像.

	bool m_hasDiffuseImage;										// diffuseのイメージを持つか.
	bool m_hasReflectionImage;									// reflectionのイメージを持つか.
	bool m_hasRoughnessImage;									// roughnessのイメージを持つか.
	bool m_hasNormalImage;										// normalのイメージを持つか.
	bool m_hasGlowImage;										// glowのイメージを持つか.
	bool m_hasOcclusionImage;									// Occlusionのイメージを持つか.

	// 以下、マッピングレイヤで1枚のみの加工不要のテクスチャである場合のマスターサーフェスクラスの参照.
	sxsdk::master_image_class* m_diffuseMasterImage;			// Diffuseのマスターイメージクラス.
	sxsdk::master_image_class* m_reflectionMasterImage;			// Reflectionのマスターイメージクラス.
	sxsdk::master_image_class* m_roughnessMasterImage;			// Roughnessのマスターイメージクラス.
	sxsdk::master_image_class* m_normalMasterImage;				// Normalのマスターイメージクラス.
	sxsdk::master_image_class* m_glowMasterImage;				// Glowのマスターイメージクラス.
	sxsdk::master_image_class* m_occlusionMasterImage;			// Occlusionのマスターイメージクラス.

	sx::vec<int,2> m_diffuseRepeat;								// Diffuseの反復回数.
	sx::vec<int,2> m_normalRepeat;								// Normalの反復回数.
	sx::vec<int,2> m_reflectionRepeat;							// Reflectionの反復回数.
	sx::vec<int,2> m_roughnessRepeat;							// Roughnessの反復回数.
	sx::vec<int,2> m_glowRepeat;								// Glowの反復回数.
	sx::vec<int,2> m_occlusionRepeat;							// Occlusionの反復回数.

	int m_diffuseTexCoord;										// DiffuseのUVレイヤ番号.
	int m_normalTexCoord;										// NormalのUVレイヤ番号.
	int m_reflectionTexCoord;									// ReflectionのUVレイヤ番号.
	int m_roughnessTexCoord;									// RoughnessのUVレイヤ番号.
	int m_glowTexCoord;											// GlowのUVレイヤ番号.
	int m_occlusionTexCoord;									// OcclusionのUVレイヤ番号.

	bool m_diffuseAlphaTrans;									// Diffuseのアルファ透明を使用しているか.
	float m_normalWeight;										// Normalのウエイト値.
	float m_occlusionWeight;									// Occlusionのウエイト値.

private:
	/**
	 * 指定のテクスチャの合成処理.
	 * @param[in] mappingType  マッピングの種類.
	 */
	bool m_blendImages (const sxsdk::enums::mapping_type mappingType);

	/**
	 * Diffuseのアルファ透明を使用しているかチェック.
	 */
	bool m_checkDiffuseAlphaTrans ();

	/**
	 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * @param[in]  mappingType   マッピングの種類.
	 * @param[out] ppMasterImage master imageの参照を返す.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkSingleImage (const sxsdk::enums::mapping_type mappingType,
		sxsdk::master_image_class** ppMasterImage,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

	/**
	 * Occlusionのテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
	 * ※ OcclusionレイヤはShade3D ver.17/18段階では存在しないため COcclusionTextureShaderInterface クラスで与えている.
	 * @param[out] ppMasterImage master imageの参照を返す.
	 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
	 * @param[out] texRepeat     繰り返し回数.
	 * @param[out] hasImage      イメージを持つか (単数または複数).
	 */
	bool m_checkOcclusionSingleImage (sxsdk::master_image_class** ppMasterImage,
		int& uvTexCoord,
		sx::vec<int,2>& texRepeat,
		bool& hasImage);

	/**
	 * 法線マップの強さを取得.
	 */
	float m_getNormalWeight ();

public:
	CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface);

	/**
	 * 個々のイメージを合成.
	 */
	void blendImages ();

	/**
	 * 各種イメージより、glTFにエクスポートするBaseColor/Roughness/Metallicを復元.
	 */
	bool calcGLTFImages ();

	/**
	 * 各種イメージを持つか (単一または複数).
	 */
	bool hasImage (const sxsdk::enums::mapping_type mappingType) const;

	/**
	 * 単一テクスチャを参照する場合のマスターイメージクラスを取得.
	 */
	sxsdk::master_image_class* getSingleMasterImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージを取得.
	 */
	compointer<sxsdk::image_interface> getImage (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージのUV層番号を取得.
	 */
	int getTexCoord (const sxsdk::enums::mapping_type mappingType);

	/**
	 * イメージの反復回数を取得.
	 */
	sx::vec<int,2> getImageRepeat (const sxsdk::enums::mapping_type mappingType);

	/**
	 * アルファ透明を使用しているか.
	 */
	bool getDiffuseAlphaTrans () { return m_diffuseAlphaTrans; }

	/**
	 * Normalのウエイト値を取得.
	 */
	float getNormalWeight () { return m_normalWeight; }

	/**
	 * Occlusionのウエイト値を取得.
	 */
	float getOcclusionWeight () { return m_occlusionWeight; }

	/**
	 * Occlusionのマスターイメージを取得.
	 */
	sxsdk::master_image_class* getOcclusionMasterImage () { return m_occlusionMasterImage; }

	/**
	 * OcclusionのUV層番号を取得.
	 */
	int getOcclusionTexCoord () { return m_occlusionTexCoord; }

	/**
	 * glTFとしてのイメージを取得.
	 */
	compointer<sxsdk::image_interface> getGLTFBaseColorImage () { return m_gltfBaseColorImage; }
	compointer<sxsdk::image_interface> getGLTFMetallicRoughnessImage () { return m_gltfMetallicRoughnessImage; }
};

#endif
