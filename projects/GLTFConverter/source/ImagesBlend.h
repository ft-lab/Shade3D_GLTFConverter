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

	sx::vec<int,2> m_diffuseRepeat;								// Diffuseの反復回数.
	sx::vec<int,2> m_normalRepeat;								// Normalの反復回数.
	sx::vec<int,2> m_reflectionRepeat;							// Reflectionの反復回数.
	sx::vec<int,2> m_roughnessRepeat;							// Roughnessの反復回数.
	sx::vec<int,2> m_glowRepeat;								// Glowの反復回数.

	bool m_diffuseAlphaTrans;									// Diffuseのアルファ透明を使用しているか.
	float m_normalWeight;										// Normalのウエイト値.

private:
	/**
	 * 指定のテクスチャの合成処理.
	 */
	bool m_blendImages (const sxsdk::enums::mapping_type mappingType);


public:
	CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface);

	/**
	 * 個々のイメージを合成.
	 */
	void blendImages ();

	/**
	 * 各種イメージを持つか.
	 */
	bool hasDiffuseImage () const { return m_diffuseImage != NULL; }
	bool hasNormalImage () const { return m_normalImage != NULL; }
	bool hasReflectionImage () const { return m_reflectionImage != NULL; }
	bool hasRoughnessImage () const { return m_roughnessImage != NULL; }
	bool hasGlowImage () const { return m_glowImage != NULL; }

	/**
	 * イメージを取得.
	 */
	compointer<sxsdk::image_interface> getDiffuseImage () { return m_diffuseImage; }
	compointer<sxsdk::image_interface> getNormalImage () { return m_normalImage; }
	compointer<sxsdk::image_interface> getReflectionImage () { return m_reflectionImage; }
	compointer<sxsdk::image_interface> getRoughnessImage () { return m_roughnessImage; }
	compointer<sxsdk::image_interface> getGlowImage () { return m_glowImage; }

	/**
	 * イメージの反復回数を取得.
	 */
	sx::vec<int,2> getDiffuseRepeat () { return m_diffuseRepeat; }
	sx::vec<int,2> getNormalRepeat () { return m_normalRepeat; }
	sx::vec<int,2> getReflectionRepeat () { return m_reflectionRepeat; }
	sx::vec<int,2> getRoughnessRepeat () { return m_roughnessRepeat; }
	sx::vec<int,2> getGlowRepeat () { return m_glowRepeat; }

	/**
	 * アルファ透明を使用しているか.
	 */
	bool getDiffuseAlphaTrans () { return m_diffuseAlphaTrans; }

	/**
	 * Normalのウエイト値を取得.
	 */
	float getNormalWeight () { return m_normalWeight; }

};

#endif
