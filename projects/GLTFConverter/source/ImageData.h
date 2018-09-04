/**
 * GLTFでのテクスチャ/イメージ情報.
 */

#ifndef _IMAGEDATA_H
#define _IMAGEDATA_H

#include "GlobalHeader.h"

#include <vector>
#include <string>


class CImageData
{
public:
	/**
	 * イメージとしてRGBAに格納されている情報.
	 */
	enum GLTF_IMAGE_MASK
	{
		gltf_image_mask_none = 0,
		gltf_image_mask_base_color = 0x0001,		// BaseColor (RGBA).
		gltf_image_mask_normal = 0x0002,			// Normal (RGB).
		gltf_image_mask_roughness = 0x0004,			// Roughness.
		gltf_image_mask_metallic = 0x0008,			// Metallic.
		gltf_image_mask_occlusion = 0x0010,			// Occlusion.
		gltf_image_mask_emissive = 0x0020,			// Emissive (RGB).
	};

public:
	std::string name;							// テクスチャ名.
	std::string mimeType;						// 画像としての種類 ("image/jpg" など ).
	std::vector<unsigned char> imageDatas;		// 画像情報。これは、jpeg/pngのフォーマットそのままに入る.
	int width, height;							// 画像サイズ.

	sxsdk::master_image_class* shadeMasterImage;	// Shade3Dでのマスターイメージクラス.
	compointer<sxsdk::image_interface> shadeImage;	// Shade3Dでのイメージ (合成したもの)。エクスポータで使用.

	int imageMask;						// テクスチャとして使用している情報.
	bool useBaseColorAlpha;				// BaseColorとしてAlphaを使用するか.

public:
	CImageData ();
	CImageData (const CImageData& v);
	~CImageData ();

    CImageData& operator = (const CImageData &v) {
		this->name       = v.name;
		this->mimeType   = v.mimeType;
		this->imageDatas = v.imageDatas;
		this->width      = v.width;
		this->height     = v.height;
		this->imageMask  = v.imageMask;
		this->useBaseColorAlpha = v.useBaseColorAlpha;
		this->shadeMasterImage  = v.shadeMasterImage;
		this->shadeImage = v.shadeImage;

		return (*this);
    }

	void clear ();

	/**
	 * イメージが同じかチェック (Shade3DからのGLTFエクスポートで使用).
	 */
	bool isSame (const CImageData &v) const;
};

#endif
