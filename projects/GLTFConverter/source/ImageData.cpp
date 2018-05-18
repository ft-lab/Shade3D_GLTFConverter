/**
 * GLTF�ł̃e�N�X�`��/�C���[�W���.
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

