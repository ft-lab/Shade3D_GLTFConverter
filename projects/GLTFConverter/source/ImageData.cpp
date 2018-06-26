/**
 * GLTFでのテクスチャ/イメージ情報.
 */

#include "ImageData.h"
#include "MathUtil.h"

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
	useBaseColorAlpha = false;
	imageMask = CImageData::gltf_image_mask_none;

	m_shadeMasterImage = NULL;

	if (shadeImage) shadeImage->Release();
	shadeImage = NULL;
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
	if ((this->shadeImage) && v.shadeImage) {
		const int width  = this->shadeImage->get_size().x;
		const int height = this->shadeImage->get_size().y;
		if (width != (v.shadeImage->get_size().x) || height != (v.shadeImage->get_size().y)) return false;

		bool sameF = true;
		std::vector<sxsdk::rgba_class> line0, line1;
		line0.resize(width);
		line1.resize(width);
		for (int y = 0; y < height; ++y) {
			this->shadeImage->get_pixels_rgba_float(0, y, width, 1, &(line0[0]));
			v.shadeImage->get_pixels_rgba_float(0, y, width, 1, &(line1[0]));
			for (int x = 0; x < width; ++x) {
				const sxsdk::rgba_class& col0 = line0[x];
				const sxsdk::rgba_class& col1 = line1[x];
				if (!MathUtil::isZero(col0 - col1)) {
					sameF = false;
					break;
				}
			}
			if (!sameF) break;
		}
		return sameF;
	}

	return false;
}

