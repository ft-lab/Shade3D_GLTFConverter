/**
 * GLTFでのテクスチャ/イメージ情報.
 */

#include "ImageData.h"

CImageData::CImageData ()
{
	clear();
}

CImageData::~CImageData ()
{
}

void CImageData::clear ()
{
	name = "";
	mimeType = "";
	imageDatas.clear();
	width = height = 0;
	imageMask = CImageData::gltf_image_mask_none;

	m_shadeMasterImage = NULL;
}

/**
 * イメージが同じかチェック (Shade3DからのGLTFエクスポートで使用).
 */
bool CImageData::isSame (const CImageData &v) const
{
	if ((this->width) == 0 || v.width == 0 || (this->height) == 0 || v.height == 0) return false;
	if ((this->width) != v.width || (this->height) != v.height) return false;

	if ((this->m_shadeMasterImage) && (v.m_shadeMasterImage)) {
		if (this->m_shadeMasterImage->get_handle() == v.m_shadeMasterImage->get_handle()) return true;
	}

	return false;
}
