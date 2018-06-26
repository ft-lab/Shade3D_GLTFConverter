/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#include "ImagesBlend.h"
#include "MathUtil.h"

CImagesBlend::CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface) : m_pScene(scene), m_surface(surface)
{
}

/**
 * 個々のイメージを合成.
 */
void CImagesBlend::blendImages ()
{
	m_diffuseRepeat    = sx::vec<int,2>(1, 1);
	m_normalRepeat     = sx::vec<int,2>(1, 1);
	m_reflectionRepeat = sx::vec<int,2>(1, 1);
	m_roughnessRepeat  = sx::vec<int,2>(1, 1);
	m_glowRepeat       = sx::vec<int,2>(1, 1);
	m_diffuseAlphaTrans = false;
	m_normalWeight = 1.0f;

	m_blendImages(sxsdk::enums::diffuse_mapping);
	m_blendImages(sxsdk::enums::normal_mapping);
	m_blendImages(sxsdk::enums::reflection_mapping);
	m_blendImages(sxsdk::enums::roughness_mapping);
	m_blendImages(sxsdk::enums::glow_mapping);
}

/**
 * 指定のテクスチャの合成処理.
 */
bool CImagesBlend::m_blendImages (const sxsdk::enums::mapping_type mappingType)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();

	compointer<sxsdk::image_interface> newImage;
	int newWidth, newHeight;
	int newRepeatX, newRepeatY;
	int counter = 0;
	std::vector<sxsdk::rgba_class> rgbaLine0, rgbaLine;
	sxsdk::rgba_class col, whiteCol;
	whiteCol = sxsdk::rgba_class(1, 1, 1, 1);

	// Diffuse時にアルファ透明を使用する場合はそのレイヤ番号を取得.
	int diffuseAlphaLayerIndex = -1;
	if (mappingType == sxsdk::enums::diffuse_mapping) {
		for (int i = 0; i < layersCou; ++i) {
			sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
			if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
				diffuseAlphaLayerIndex = i;
				m_diffuseAlphaTrans = true;
				break;
			}
		}
	}
	std::vector<unsigned char> alphaBuff;

	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
		if (mappingLayer.get_type() != mappingType) continue;
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight  = (mappingType == sxsdk::enums::normal_mapping) ? 1.0f : (std::min(std::max(mappingLayer.get_weight(), 0.0f), 1.0f));
		const float weight2 = 1.0f - weight;
		if (MathUtil::isZero(weight)) continue;
		if (mappingLayer.get_uv_mapping() != 0) continue;		// UV0のみ採用.

		if (mappingType == sxsdk::enums::normal_mapping) {
			m_normalWeight = mappingLayer.get_weight();
		}

		const int blendMode = mappingLayer.get_blend_mode();
		try {
			compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
			if (!image) continue;
			const int width  = image->get_size().x;
			const int height = image->get_size().y;
			if (width <= 1 || height <= 1) continue;
			if (!newImage) {
				newImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
				newWidth  = width;
				newHeight = height;
				newRepeatX = mappingLayer.get_repetition_x();
				newRepeatY = mappingLayer.get_repetition_y();
				rgbaLine.resize(newWidth);
				rgbaLine0.resize(newWidth);
			}
			if (newRepeatX != mappingLayer.get_repetition_x() || newRepeatY != mappingLayer.get_repetition_y()) continue;
			compointer<sxsdk::image_interface> image2(image->create_duplicate_image_interface(&(newImage->get_size())));

			// アルファ値を保持するバッファを作成.
			if (i == diffuseAlphaLayerIndex) {
				alphaBuff.resize(newWidth * newHeight, 255);
			}

			const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
			const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
			const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.

			for (int y = 0; y < newHeight; ++y) {
				if (flipV) {
					image2->get_pixels_rgba_float(0, newHeight - y - 1, newWidth, 1, &(rgbaLine[0]));
				} else {
					image2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
				}
				if (flipH) {
					const int newWidthH = newWidth >> 1;
					for (int x = 0; x < newWidthH; ++x) {
						std::swap(rgbaLine[x], rgbaLine[newWidth - x - 1]);
					}
				}

				if (flipColor) {
					for (int x = 0; x < newWidth; ++x) {
						rgbaLine[x].red   = 1.0f - rgbaLine[x].red;
						rgbaLine[x].green = 1.0f - rgbaLine[x].green;
						rgbaLine[x].blue  = 1.0f - rgbaLine[x].blue;
					}
				}

				// アルファ値を保持.
				if (i == diffuseAlphaLayerIndex) {
					const int iPos = y * newWidth;
					for (int x = 0; x < newWidth; ++x) {
						alphaBuff[x + iPos] = (unsigned char)std::min((int)(rgbaLine[x].alpha * 255.0f), 255);
					}
				}

				if (counter == 0) {
					for (int x = 0; x < newWidth; ++x) {
						rgbaLine[x] = rgbaLine[x] * weight + whiteCol * weight2;
					}
				} else {
					newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine0[0]));

					if (blendMode == sxsdk::enums::mapping_blend_mode) {		// 「通常」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine[x] * weight + rgbaLine0[x] * weight2;
						}
					} else if (blendMode == sxsdk::enums::mapping_mul_mode) {	// 「乗算 (レガシー)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine[x] * rgbaLine0[x] * weight;
						}
					} else if (blendMode == 7) {							// 「乗算」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = (whiteCol * weight2 + rgbaLine[x] * weight) * rgbaLine0[x];
						}
					} else if (blendMode == sxsdk::enums::mapping_add_mode) {		// 「加算」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine0[x] + (rgbaLine[x] * weight);
							rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
							rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
							rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
							rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
						}
					} else if (blendMode == sxsdk::enums::mapping_sub_mode) {		// 「減算」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x] = rgbaLine0[x] - (rgbaLine[x] * weight);
							rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
							rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
							rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
							rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
						}
					} else if (blendMode == sxsdk::enums::mapping_min_mode) {		// 「比較(暗)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x].red   = std::min(rgbaLine0[x].red,   rgbaLine[x].red   * weight);
							rgbaLine[x].green = std::min(rgbaLine0[x].green, rgbaLine[x].green * weight);
							rgbaLine[x].blue  = std::min(rgbaLine0[x].blue,  rgbaLine[x].blue  * weight);
						}
					} else if (blendMode == sxsdk::enums::mapping_max_mode) {		// 「比較(明)」合成.
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x].red   = std::max(rgbaLine0[x].red,   rgbaLine[x].red   * weight);
							rgbaLine[x].green = std::max(rgbaLine0[x].green, rgbaLine[x].green * weight);
							rgbaLine[x].blue  = std::max(rgbaLine0[x].blue,  rgbaLine[x].blue  * weight);
						}
					}
				}
				newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			}
			counter++;

			// 法線マップは1枚のみを採用.
			if (mappingType == sxsdk::enums::normal_mapping) break;

		} catch (...) { }
	}
	if (!newImage) return false;

	// アルファ透明のアルファ値で、アルファ値を上書き.
	if (!alphaBuff.empty()) {
		for (int y = 0, iPos = 0; y < newHeight; ++y, iPos += newWidth) {
			newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			for (int x = 0; x < newWidth; ++x) {
				rgbaLine[x].alpha = (float)alphaBuff[x + iPos] / 255.0f;
			}
			newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
		}
	}

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		m_diffuseImage = newImage;
		m_diffuseRepeat = sx::vec<int,2>(newRepeatX, newRepeatY);
	}
	if (mappingType == sxsdk::enums::normal_mapping) {
		m_normalImage = newImage;
		m_normalRepeat = sx::vec<int,2>(newRepeatX, newRepeatY);
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		m_reflectionImage = newImage;
		m_reflectionRepeat = sx::vec<int,2>(newRepeatX, newRepeatY);
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		m_roughnessImage = newImage;
		m_roughnessRepeat = sx::vec<int,2>(newRepeatX, newRepeatY);
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		m_glowImage = newImage;
		m_glowRepeat = sx::vec<int,2>(newRepeatX, newRepeatY);
	}

	return true;
}

