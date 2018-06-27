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
	m_diffuseTexCoord     = 0;
	m_normalTexCoord      = 0;
	m_reflectionTexCoord  = 0;
	m_roughnessTexCoord   = 0;
	m_glowTexCoord        = 0;

	m_diffuseAlphaTrans = false;
	m_normalWeight = 1.0f;

	// Shade3Dでの表面材質のマッピングレイヤごとに、各種イメージを合成.
	m_blendImages(sxsdk::enums::diffuse_mapping);
	m_blendImages(sxsdk::enums::normal_mapping);
	m_blendImages(sxsdk::enums::reflection_mapping);
	m_blendImages(sxsdk::enums::roughness_mapping);
	m_blendImages(sxsdk::enums::glow_mapping);

	// glTF用のイメージに変換.
	m_calcGLTFImages();
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
	int newTexCoord;
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
				newWidth    = width;
				newHeight   = height;
				newRepeatX  = mappingLayer.get_repetition_x();
				newRepeatY  = mappingLayer.get_repetition_y();
				newTexCoord = mappingLayer.get_uv_mapping();
				rgbaLine.resize(newWidth);
				rgbaLine0.resize(newWidth);
			}
			if (newRepeatX != mappingLayer.get_repetition_x() || newRepeatY != mappingLayer.get_repetition_y()) continue;
			if (newTexCoord != mappingLayer.get_uv_mapping()) continue;
			compointer<sxsdk::image_interface> image2(image->create_duplicate_image_interface(&(newImage->get_size())));

			// アルファ値を保持するバッファを作成.
			if (i == diffuseAlphaLayerIndex) {
				alphaBuff.resize(newWidth * newHeight, 255);
			}

			const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
			const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
			const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
			const int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
			const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_average_mode);

			for (int y = 0; y < newHeight; ++y) {
				if (flipV) {
					image2->get_pixels_rgba_float(0, newHeight - y - 1, newWidth, 1, &(rgbaLine[0]));
				} else {
					image2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
				}

				// チャンネルの合成モード により、色を埋める.
				if (useChannelMix) {
					float fVal;
					if (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].alpha;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_red_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].red;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_green_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].green;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_blue_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = rgbaLine[x].blue;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					} else if (channelMix == sxsdk::enums::mapping_grayscale_average_mode) {
						for (int x = 0; x < newWidth; ++x) {
							fVal = (rgbaLine[x].red + rgbaLine[x].green + rgbaLine[x].blue) * 0.3333f;
							rgbaLine[x] = sxsdk::rgba_class(fVal, fVal, fVal, 1.0f);
						}
					}
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
		m_diffuseImage    = newImage;
		m_diffuseRepeat   = sx::vec<int,2>(newRepeatX, newRepeatY);
		m_diffuseTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::normal_mapping) {
		m_normalImage    = newImage;
		m_normalRepeat   = sx::vec<int,2>(newRepeatX, newRepeatY);
		m_normalTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		m_reflectionImage    = newImage;
		m_reflectionRepeat   = sx::vec<int,2>(newRepeatX, newRepeatY);
		m_reflectionTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		m_roughnessImage    = newImage;
		m_roughnessRepeat   = sx::vec<int,2>(newRepeatX, newRepeatY);
		m_roughnessTexCoord = newTexCoord;
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		m_glowImage    = newImage;
		m_glowRepeat   = sx::vec<int,2>(newRepeatX, newRepeatY);
		m_glowTexCoord = newTexCoord;
	}

	return true;
}

/**
 * 各種イメージより、glTFにエクスポートするBaseColor/Roughness/Metallicを復元.
 */
bool CImagesBlend::m_calcGLTFImages ()
{
	const sxsdk::rgba_class whiteCol(1, 1, 1, 1);
	const sxsdk::rgba_class blackCol(0, 0, 0, 1);

	if (m_gltfMetallicRoughnessImage) m_gltfMetallicRoughnessImage->Release();
	if (m_gltfBaseColorImage) m_gltfBaseColorImage->Release();
	if (!m_roughnessImage && !m_diffuseImage && !m_reflectionImage) return false;

	// Diffuseのテクスチャだけ持つ場合、そのまま返す.
	if (m_diffuseImage && !m_roughnessImage && !m_reflectionImage) {
		m_gltfBaseColorImage = m_diffuseImage->duplicate_image();
		return true;
	}

	// 画像サイズを計算.
	int width, height;
	width = height = 0;
	if (m_diffuseImage) {
		width  = std::max(width, m_diffuseImage->get_size().x);
		height = std::max(height, m_diffuseImage->get_size().y);
	}
	if (m_reflectionImage) {
		width  = std::max(width, m_reflectionImage->get_size().x);
		height = std::max(height, m_reflectionImage->get_size().y);
	}
	if (m_roughnessImage) {
		width  = std::max(width, m_roughnessImage->get_size().x);
		height = std::max(height, m_roughnessImage->get_size().y);
	}
	if (width <= 0 || height <= 0) return true;

	m_gltfBaseColorImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
	m_gltfMetallicRoughnessImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
	compointer<sxsdk::image_interface> gltfMetallicImage(m_pScene->create_image_interface(sx::vec<int,2>(width, height)));
	compointer<sxsdk::image_interface> gltfRoughnessImage(m_pScene->create_image_interface(sx::vec<int,2>(width, height)));

	// イメージサイズが width x heightとなるようにリサイズ.
	if (m_diffuseImage) {
		const sx::vec<int,2> size = m_diffuseImage->get_size();
		if (size.x != width || size.y != height) {
			compointer<sxsdk::image_interface> image(m_diffuseImage->duplicate_image(&(sx::vec<int,2>(width, height))));
			m_diffuseImage->Release();
			m_diffuseImage = image;
		}
	}
	if (m_reflectionImage) {
		const sx::vec<int,2> size = m_reflectionImage->get_size();
		if (size.x != width || size.y != height) {
			compointer<sxsdk::image_interface> image(m_reflectionImage->duplicate_image(&(sx::vec<int,2>(width, height))));
			m_reflectionImage->Release();
			m_reflectionImage = image;
		}
	}
	if (m_roughnessImage) {
		const sx::vec<int,2> size = m_roughnessImage->get_size();
		if (size.x != width || size.y != height) {
			compointer<sxsdk::image_interface> image(m_roughnessImage->duplicate_image(&(sx::vec<int,2>(width, height))));
			m_roughnessImage->Release();
			m_roughnessImage = image;
		}
	}

	std::vector<sxsdk::rgba_class> lineD, lineD2, baseColorLine, metallicLine;
	sxsdk::rgba_class col1, col, baseColorCol, metallicCol;

	// Roughnessテクスチャの生成.
	lineD.resize(width);
	if (m_roughnessImage) {
		for (int y = 0; y < height; ++y) {
			m_roughnessImage->get_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
			for (int x = 0; x < width; ++x) {
				const float v = 1.0f - lineD[x].red;
				lineD[x] = sxsdk::rgba_class(v, v, v, 1.0f);
			}
			gltfRoughnessImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
		}
	} else {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) lineD[x] = whiteCol;
			gltfRoughnessImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
		}
	}

	// BaseColor/Metallicのテクスチャを白でクリア.
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) lineD[x] = whiteCol;
		m_gltfBaseColorImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
	}
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) lineD[x] = whiteCol;
		gltfMetallicImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
	}

	// Diffuse/Reflectionテクスチャより、glTFのBaseColor/Metallicを計算.
	if (m_diffuseImage || m_reflectionImage) {
		const float fMin = (float)(1e-4);
		float f;
		lineD.resize(width);
		lineD2.resize(width);
		baseColorLine.resize(width);
		metallicLine.resize(width);
		for (int y = 0; y < height; ++y) {
			if (m_diffuseImage) {
				m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
			} else {
				for (int x = 0; x < width; ++x) lineD[x] = whiteCol;
			}
			if (m_reflectionImage) {
				m_reflectionImage->get_pixels_rgba_float(0, y, width, 1, &(lineD2[0]));
			} else {
				for (int x = 0; x < width; ++x) lineD2[x] = whiteCol;
			}
			for (int x = 0; x < width; ++x) {
				col1 = lineD[x];
				const float reflection = lineD2[x].red;

				baseColorCol = blackCol;
				// 計算したb1/b2のどちらかは必ず0になる.
				{
					const float bV = -(col1.red + reflection * 0.5f);
					const float c = std::abs(bV);
					if (c > fMin) {
						const float b1 = (-bV - c) * 0.5f;
						const float b2 = (-bV + c) * 0.5f;
						baseColorCol.red = std::min(std::max(b1, b2), 1.0f);
					}
				}
				{
					const float bV = -(col1.green + reflection * 0.5f);
					const float c = std::abs(bV);
					if (c > fMin) {
						const float b1 = (-bV - c) * 0.5f;
						const float b2 = (-bV + c) * 0.5f;
						baseColorCol.green = std::min(std::max(b1, b2), 1.0f);
					}
				}
				{
					const float bV = -(col1.blue + reflection * 0.5f);
					const float c = std::abs(bV);
					if (c > fMin) {
						const float b1 = (-bV - c) * 0.5f;
						const float b2 = (-bV + c) * 0.5f;
						baseColorCol.blue = std::min(std::max(b1, b2), 1.0f);
					}
				}
				metallicCol = blackCol;
				if (baseColorCol.red   > 0.0f) metallicCol.red   = std::min(reflection / baseColorCol.red, 1.0f);
				if (baseColorCol.green > 0.0f) metallicCol.green = std::min(reflection / baseColorCol.green, 1.0f);
				if (baseColorCol.blue  > 0.0f) metallicCol.blue  = std::min(reflection / baseColorCol.blue, 1.0f);
				const float metallicV = (metallicCol.red + metallicCol.green + metallicCol.blue) * 0.3333f;

				baseColorLine[x] = baseColorCol;
				metallicLine[x]  = sxsdk::rgba_class(metallicV, metallicV, metallicV, 1.0f);
			}

			// Alpha値はDiffuseのものを採用.
			for (int x = 0; x < width; ++x) baseColorLine[x].alpha = lineD[x].alpha;

			m_gltfBaseColorImage->set_pixels_rgba_float(0, y, width, 1, &(baseColorLine[0]));
			gltfMetallicImage->set_pixels_rgba_float(0, y, width, 1, &(metallicLine[0]));
		}
	}

	// Metallic(B)/Roughness(G)のテクスチャを合成.
	for (int y = 0; y < height; ++y) {
		gltfMetallicImage->get_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
		gltfRoughnessImage->get_pixels_rgba_float(0, y, width, 1, &(lineD2[0]));
		for (int x = 0; x < width; ++x) {
			lineD[x] = sxsdk::rgba_class(1.0f, lineD2[x].red, lineD[x].red, 1.0f);
		}
		m_gltfMetallicRoughnessImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
	}

	return true;

}
