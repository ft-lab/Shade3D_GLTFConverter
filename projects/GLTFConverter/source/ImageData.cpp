/**
 * GLTFでのテクスチャ/イメージ情報.
 */

#include "ImageData.h"
#include "MathUtil.h"

CImageData::CImageData ()
{
	clear();
}

CImageData::CImageData (const CImageData& v)
{
	this->name       = v.name;
	this->mimeType   = v.mimeType;
	this->imageDatas = v.imageDatas;
	this->width      = v.width;
	this->height     = v.height;
	this->imageMask  = v.imageMask;
	this->useBaseColorAlpha = v.useBaseColorAlpha;
	this->shadeMasterImage  = v.shadeMasterImage;
	this->imageRGBAData = v.imageRGBAData;
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

	shadeMasterImage = NULL;

	imageRGBAData.clear();
}

/**
 * カスタムイメージの格納(RGBAを格納).
 */
void CImageData::setCustomImage (sxsdk::image_interface* image)
{
	clear();
	try {
		width  = image->get_size().x;
		height = image->get_size().y;
		
		imageRGBAData.resize(width * height * 4);
		std::vector<sx::rgba8_class> colLines;
		colLines.resize(width);

		int iPos = 0;
		for (int y = 0; y < height; ++y) {
			image->get_pixels_rgba(0, y, width, 1, &(colLines[0]));
			for (int x = 0; x < width; ++x) {
				const sx::rgba8_class& col = colLines[x];
				imageRGBAData[iPos + 0] = col.red;
				imageRGBAData[iPos + 1] = col.green;
				imageRGBAData[iPos + 2] = col.blue;
				imageRGBAData[iPos + 3] = col.alpha;
				iPos += 4;
			}
		}

	} catch (...) { }
}

/**
 * イメージが同じかチェック (Shade3DからのGLTFエクスポートで使用).
 */
bool CImageData::isSame (const CImageData &v) const
{
	if ((this->width) == 0 || v.width == 0 || (this->height) == 0 || v.height == 0) return false;
	if ((this->width) != v.width || (this->height) != v.height) return false;

	if ((this->shadeMasterImage) && (v.shadeMasterImage)) {
		if (this->shadeMasterImage->get_handle() == v.shadeMasterImage->get_handle()) return true;
	}
	if (!(this->imageRGBAData.empty()) && !(v.imageRGBAData.empty())) {
		bool sameF = true;
		int iPos = 0;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const unsigned char r0 = this->imageRGBAData[iPos + 0];
				const unsigned char g0 = this->imageRGBAData[iPos + 1];
				const unsigned char b0 = this->imageRGBAData[iPos + 2];
				const unsigned char a0 = this->imageRGBAData[iPos + 3];
				const unsigned char r1 = v.imageRGBAData[iPos + 0];
				const unsigned char g1 = v.imageRGBAData[iPos + 1];
				const unsigned char b1 = v.imageRGBAData[iPos + 2];
				const unsigned char a1 = v.imageRGBAData[iPos + 3];
				iPos += 4;

				if (r0 != r1 || g0 != g1 || b0 != b1 || a0 != a1) {
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

/**
 * 画像を取得.
 */
compointer<sxsdk::image_interface> CImageData::getImage (sxsdk::scene_interface* scene) const
{
	try {
		if (shadeMasterImage) return compointer<sxsdk::image_interface>(shadeMasterImage->get_image());

		if (!imageRGBAData.empty()) {
			compointer<sxsdk::image_interface> image(scene->create_image_interface(sx::vec<int,2>(width, height)));
			if (!image) return compointer<sxsdk::image_interface>();

			std::vector<sx::rgba8_class> colLines;
			colLines.resize(width);
			int iPos = 0;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					colLines[x] = sx::rgba8_class(imageRGBAData[iPos + 0], imageRGBAData[iPos + 1], imageRGBAData[iPos + 2], imageRGBAData[iPos + 3]);
					iPos += 4;
				}
				image->set_pixels_rgba(0, y, width, 1, &(colLines[0]));
			}
			return image;
		}

	} catch (...) { }
	return compointer<sxsdk::image_interface>();
}

