/**
 * Export用にShade3Dの表面材質のマッピングレイヤのテクスチャを合成.
 */
#include "ImagesBlend.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"

#include <math.h>

// sxsdk::image_interface* の解放処理.
// 注意点として、compointer<sxsdk::image_interface>で確保した場合は自動で解放されるため、Releaseを呼んではいけない.
#define IMAGE_INTERFACE_RELEASE(image) {if (image) { image->Release(); image = NULL; } }

/*
	Shade3Dの「透明」「不透明マスク」「チャンネル合成のアルファ透明」は、最終的に合成されてすべてOpacityのテクスチャに格納される.
*/

CImagesBlend::CImagesBlend (sxsdk::scene_interface* scene, sxsdk::surface_class* surface) : m_pScene(scene), m_surface(surface)
{
	m_glTFMetallicRoughnessImage = NULL;
	m_diffuseImage      = NULL;
	m_normalImage       = NULL;
	m_reflectionImage   = NULL;
	m_roughnessImage    = NULL;
	m_glowImage         = NULL;
	m_occlusionImage    = NULL;
	m_transparencyImage = NULL;
	m_opacityMaskImage  = NULL;
}

CImagesBlend::~CImagesBlend ()
{
	clear();
}

void CImagesBlend::clear ()
{
	m_exportParam.clear();

	IMAGE_INTERFACE_RELEASE(m_glTFMetallicRoughnessImage);
	IMAGE_INTERFACE_RELEASE(m_diffuseImage);
	IMAGE_INTERFACE_RELEASE(m_normalImage);
	IMAGE_INTERFACE_RELEASE(m_reflectionImage);
	IMAGE_INTERFACE_RELEASE(m_roughnessImage);
	IMAGE_INTERFACE_RELEASE(m_glowImage);
	IMAGE_INTERFACE_RELEASE(m_occlusionImage);
	IMAGE_INTERFACE_RELEASE(m_transparencyImage);
	IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);

	m_diffuseRepeat    = sx::vec<int,2>(1, 1);
	m_normalRepeat     = sx::vec<int,2>(1, 1);
	m_reflectionRepeat = sx::vec<int,2>(1, 1);
	m_roughnessRepeat  = sx::vec<int,2>(1, 1);
	m_glowRepeat       = sx::vec<int,2>(1, 1);
	m_opacityMaskRepeat  = sx::vec<int,2>(1, 1);
	m_transparencyRepeat = sx::vec<int,2>(1, 1);

	m_diffuseTexCoord     = 0;
	m_normalTexCoord      = 0;
	m_reflectionTexCoord  = 0;
	m_roughnessTexCoord   = 0;
	m_glowTexCoord        = 0;
	m_occlusionTexCoord   = 0;
	m_opacityMaskTexCoord = 0;
	m_transparencyTexCoord = 0;

	m_diffuseAlphaTrans = false;

	m_hasDiffuseImage    = false;
	m_hasReflectionImage = false;
	m_hasRoughnessImage  = false;
	m_hasNormalImage     = false;
	m_hasGlowImage       = false;
	m_hasOcclusionImage  = false;
	m_hasOpacityMaskImage  = false;
	m_hasTransparencyImage = false;

	m_diffuseImageName      = "";
	m_normalImageName       = "";
	m_reflectionImageName   = "";
	m_roughnessImageName    = "";
	m_glowImageName         = "";
	m_occlusionImageName    = "";
	m_transparencyImageName = "";
	m_opacityMaskImageName  = "";

	m_diffuseColor  = sxsdk::rgb_class(1.0f, 1.0f, 1.0f);
	m_emissiveColor = sxsdk::rgb_class(0.0f, 0.0f, 0.0f);
	m_metallic  = 0.0f;
	m_roughness = 0.0f;
	m_normalStrength = 1.0f;

	m_diffuseTexturesCount = 0;
	m_normalTexturesCount = 0;
	m_useDiffuseAlpha = false;

	m_alphaModeType = GLTFConverter::alpha_mode_opaque;
	m_alphaCutoff   = 0.5f;
	m_transparency = 0.0f;

	m_useOcclusionInMetallicRoughnessTexture = false;
}

/**
 * 個々のイメージを合成.
 * @param[in] convColorToLinear  色をリニア変換.
 */
CImagesBlend::IMAGE_BAKE_RESULT CImagesBlend::blendImages (const CExportDlgParam& exportParam)
{
	IMAGE_BAKE_RESULT result = CImagesBlend::bake_success;

	clear();
	m_exportParam.clone(exportParam);

	// AlphaModeの情報を取得.
	CAlphaModeMaterialData alphaModeData;
	bool hasAlphaModeData = false;
	if (StreamCtrl::loadAlphaModeMaterialParam(m_surface, alphaModeData)) {
		m_alphaModeType = alphaModeData.alphaModeType;
		m_alphaCutoff   = alphaModeData.alphaCutoff;
		hasAlphaModeData = true;
	}

	// Shade3Dでの表面材質のマッピングレイヤで、加工無しの画像が参照されているかチェック.
	m_checkImage(sxsdk::enums::diffuse_mapping, m_diffuseTexCoord, m_diffuseRepeat, m_hasDiffuseImage);
	m_checkImage(sxsdk::enums::normal_mapping, m_normalTexCoord, m_normalRepeat, m_hasNormalImage);
	m_checkImage(sxsdk::enums::reflection_mapping, m_reflectionTexCoord, m_reflectionRepeat, m_hasReflectionImage);
	m_checkImage(sxsdk::enums::roughness_mapping, m_roughnessTexCoord, m_roughnessRepeat, m_hasRoughnessImage);
	m_checkImage(sxsdk::enums::glow_mapping, m_glowTexCoord, m_glowRepeat, m_hasGlowImage);
	m_checkImage(sxsdk::enums::transparency_mapping, m_transparencyTexCoord, m_transparencyRepeat, m_hasTransparencyImage);
	m_checkImage(MAPPING_TYPE_OPACITY, m_opacityMaskTexCoord, m_opacityMaskRepeat, m_hasOpacityMaskImage);
	m_checkImage(MAPPING_TYPE_GLTF_OCCLUSION, m_occlusionTexCoord, m_occlusionRepeat, m_hasOcclusionImage);

	// 最終的にPBRマテリアルとしてDiffuse/Mettalic/Roughness/Opacityなどで補正するため、.
	// 「UVレイヤが異なる場合」「テクスチャの繰り返し回数が異なる場合」は正しいベイク結果にはならない.
	// 「テクスチャの繰り返し」は、「法線」「発光」以外はすべて同一の繰り返し回数になるようにする.
	{
		const sx::vec<int,2> repeat1(1, 1);
		sx::vec<int,2> dRepeat(0, 0);
		bool chkF = false;

		if (!m_exportParam.bakeWithoutProcessingTextures) {
			chkF = false;
			if (m_hasTransparencyImage) {
				if (dRepeat[0] == 0) dRepeat = m_transparencyRepeat;
				if (dRepeat != m_transparencyRepeat) chkF = true;
			}
			if (m_hasOpacityMaskImage) {
				if (dRepeat[0] == 0) dRepeat = m_opacityMaskRepeat;
				if (dRepeat != m_opacityMaskRepeat) chkF = true;
			}
			if (m_hasDiffuseImage) {
				if (dRepeat[0] == 0) dRepeat = m_diffuseRepeat;
				if (dRepeat != m_diffuseRepeat) chkF = true;
			}
			if (m_hasReflectionImage) {
				if (dRepeat[0] == 0) dRepeat = m_reflectionRepeat;
				if (dRepeat != m_reflectionRepeat) chkF = true;
			}
			if (m_hasRoughnessImage) {
				if (dRepeat[0] == 0) dRepeat = m_roughnessRepeat;
				if (dRepeat != m_roughnessRepeat) chkF = true;
			}
			if (m_hasOcclusionImage) {
				if (m_hasReflectionImage || m_hasRoughnessImage) {
					if (dRepeat[0] == 0) dRepeat = m_occlusionRepeat;
					if (dRepeat != m_occlusionRepeat) chkF = true;
				}
			}
			if (chkF) {
				result = CImagesBlend::bake_mixed_repeat;
				m_transparencyRepeat = repeat1;
				m_opacityMaskRepeat = repeat1;
				m_diffuseRepeat = repeat1;
				m_reflectionRepeat = repeat1;
				m_roughnessRepeat = repeat1;
				m_occlusionRepeat = repeat1;
			}
		} else {
			// テクスチャを加工せずにベイクする場合、.
			// Transparency/Opacity/Diffuse と Reflection/Roughness/Occlusionを分けて考える.
			dRepeat = sx::vec<int,2>(0, 0);
			chkF = false;
			if (m_hasTransparencyImage) {
				if (dRepeat[0] == 0) dRepeat = m_transparencyRepeat;
				if (dRepeat != m_transparencyRepeat) chkF = true;
			}
			if (m_hasOpacityMaskImage) {
				if (dRepeat[0] == 0) dRepeat = m_opacityMaskRepeat;
				if (dRepeat != m_opacityMaskRepeat) chkF = true;
			}
			if (m_hasDiffuseImage) {
				if (dRepeat[0] == 0) dRepeat = m_diffuseRepeat;
				if (dRepeat != m_diffuseRepeat) chkF = true;
			}
			if (chkF) {
				result = CImagesBlend::bake_mixed_repeat;
				m_transparencyRepeat = repeat1;
				m_opacityMaskRepeat = repeat1;
				m_diffuseRepeat = repeat1;
			}

			dRepeat = sx::vec<int,2>(0, 0);
			chkF = false;
			if (m_hasReflectionImage) {
				if (dRepeat[0] == 0) dRepeat = m_reflectionRepeat;
				if (dRepeat != m_reflectionRepeat) chkF = true;
			}
			if (m_hasRoughnessImage) {
				if (dRepeat[0] == 0) dRepeat = m_roughnessRepeat;
				if (dRepeat != m_roughnessRepeat) chkF = true;
			}
			if (m_hasOcclusionImage) {
				if (m_hasReflectionImage || m_hasRoughnessImage) {
					if (dRepeat[0] == 0) dRepeat = m_occlusionRepeat;
					if (dRepeat != m_occlusionRepeat) chkF = true;
				}
			}
			if (chkF) {
				result = CImagesBlend::bake_mixed_repeat;
				m_reflectionRepeat = repeat1;
				m_roughnessRepeat = repeat1;
				m_occlusionRepeat = repeat1;
			}
		}
	}

	// UVレイヤが異なるかチェック.
	// 法線マップ/Occlusionマップ/Glowマップは別UVレイヤでもOK.
	{
		int dUV = -1;
		bool chkF = false;
		if (m_hasTransparencyImage) {
			if (dUV < 0) dUV = m_transparencyTexCoord;
			if (dUV != m_transparencyTexCoord) chkF = true;
		}
		if (m_hasOpacityMaskImage) {
			if (dUV < 0) dUV = m_opacityMaskTexCoord;
			if (dUV != m_opacityMaskTexCoord) chkF = true;
		}
		if (m_hasDiffuseImage) {
			if (dUV < 0) dUV = m_diffuseTexCoord;
			if (dUV != m_diffuseTexCoord) chkF = true;
		}
		if (m_hasReflectionImage) {
			if (dUV < 0) dUV = m_reflectionTexCoord;
			if (dUV != m_reflectionTexCoord) chkF = true;
		}
		if (m_hasRoughnessImage) {
			if (dUV < 0) dUV = m_roughnessTexCoord;
			if (dUV != m_roughnessTexCoord) chkF = true;
		}
		if (chkF) {
			result = CImagesBlend::bake_error_mixed_uv_layer;
		}
	}

	// Shade3Dでの表面材質のマッピングレイヤごとに、各種イメージを合成.
	if (m_hasDiffuseImage) m_blendImages(sxsdk::enums::diffuse_mapping, m_diffuseRepeat);
	if (m_hasNormalImage) m_blendImages(sxsdk::enums::normal_mapping, m_normalRepeat);
	if (m_hasReflectionImage) m_blendImages(sxsdk::enums::reflection_mapping, m_reflectionRepeat);
	if (m_hasRoughnessImage) m_blendImages(sxsdk::enums::roughness_mapping, m_roughnessRepeat);
	if (m_hasGlowImage) m_blendImages(sxsdk::enums::glow_mapping, m_glowRepeat);
	if (m_hasTransparencyImage) m_blendImages(sxsdk::enums::transparency_mapping, m_transparencyRepeat);
	if (m_hasOpacityMaskImage) m_blendImages(MAPPING_TYPE_OPACITY, m_opacityMaskRepeat);
	if (m_hasOcclusionImage) m_blendImages(MAPPING_TYPE_GLTF_OCCLUSION, m_occlusionRepeat);

	// Shade3DのマテリアルからPBRマテリアルに置き換え.
	if (!m_exportParam.bakeWithoutProcessingTextures) {
		m_convShade3DToPBRMaterial();
	} else {
		// 加工せずにテクスチャを格納.
		m_noBakeShade3DToPBRMaterial();
	}

	return result;
}

/**
 * 指定のテクスチャの種類がベイク不要の1枚のテクスチャであるかチェック.
 * @param[in]  mappingType   マッピングの種類.
 * @param[out] uvTexCoord    UV用の使用テクスチャ層番号を返す.
 * @param[out] texRepeat     繰り返し回数。複数テクスチャ使用で異なる数の場合は(1, 1)が入る.
 * @param[out] hasImage      イメージを持つか (単数または複数).
 */
bool CImagesBlend::m_checkImage (const sxsdk::enums::mapping_type mappingType, 
		int& uvTexCoord, sx::vec<int,2>& texRepeat, bool& hasImage)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();
	int counter = 0;
	texRepeat = sx::vec<int,2>(1, 1);
	hasImage = false;

	float baseV = 1.0f;
	if (mappingType == sxsdk::enums::diffuse_mapping) baseV = m_surface->get_diffuse();
	if (mappingType == sxsdk::enums::reflection_mapping) baseV = m_surface->get_reflection();
	if (mappingType == sxsdk::enums::roughness_mapping) baseV = m_surface->get_roughness();
	if (mappingType == sxsdk::enums::glow_mapping) baseV = m_surface->get_glow();
	if (mappingType == sxsdk::enums::transparency_mapping) baseV = m_surface->get_transparency();

	bool texRepeatSingle = true;

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		m_diffuseTexturesCount = 0;
		m_useDiffuseAlpha = false;
	}
	if (mappingType == sxsdk::enums::normal_mapping) {
		m_normalTexturesCount = 0;
	}

	int uvIndex = 0;
	int occlusionChannelMix = 0;
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			uvIndex = mappingLayer.get_uv_mapping();
		} else {
			if (!Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

			try {
				compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
				COcclusionShaderData occlusionD;
				StreamCtrl::loadOcclusionParam(stream, occlusionD);
				uvIndex = occlusionD.uvIndex;
				occlusionChannelMix = occlusionD.channelMix;
			} catch (...) {
				continue;
			}
		}
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else {
				if (type != MAPPING_TYPE_OPACITY) continue;
			}
		} else {
			if (mappingType == sxsdk::enums::normal_mapping) {
				// バンプマップの場合は法線マップに変換するため.
				if (type != mappingType && type != sxsdk::enums::bump_mapping) continue;
			} else {
				if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
					if (type != mappingType) continue;
				}
			}
		}

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
		const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
		const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
		const bool rotate90  = mappingLayer.get_swap_axes();			// 90度回転.

		int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
		if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) {
			if (occlusionChannelMix == 0) channelMix = sxsdk::enums::mapping_grayscale_red_mode;
			else if (occlusionChannelMix == 1) channelMix = sxsdk::enums::mapping_grayscale_green_mode;
			else if (occlusionChannelMix == 2) channelMix = sxsdk::enums::mapping_grayscale_blue_mode;
		}

		const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
									channelMix == sxsdk::enums::mapping_grayscale_average_mode);

		if (counter == 0) {
			texRepeatSingle = true;
			uvTexCoord  = uvIndex;
			const int repeatX = mappingLayer.get_repetition_x();
			const int repeatY = mappingLayer.get_repetition_y();
			texRepeat = sx::vec<int,2>(repeatX, repeatY);

			// マスターイメージを取得.
			sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
			if (masterImage) {
				const std::string imageName = masterImage->get_name();

				switch (mappingType) {
				case sxsdk::enums::diffuse_mapping:
					m_diffuseImageName = imageName;
					break;

				case sxsdk::enums::normal_mapping:
					m_normalImageName = imageName;
					break;

				case sxsdk::enums::reflection_mapping:
					m_reflectionImageName = imageName;
					break;

				case sxsdk::enums::roughness_mapping:
					m_roughnessImageName = imageName;
					break;

				case sxsdk::enums::glow_mapping:
					m_glowImageName = imageName;
					break;

				case sxsdk::enums::transparency_mapping:
					m_transparencyImageName = imageName;
					break;
				}
				if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) {
					m_occlusionImageName = imageName;
				}
				if (mappingType == MAPPING_TYPE_OPACITY) {
					m_opacityMaskImageName = imageName;
				}
			}
		}
		if (uvIndex != uvTexCoord) continue;

		// 1つ前が「マット」の場合は無条件でベイク対象にする.
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					{
						const int repeatX = prevMappingLayer.get_repetition_x();
						const int repeatY = prevMappingLayer.get_repetition_y();
						if (texRepeat[0] != repeatX || texRepeat[1] != repeatY) texRepeatSingle = false;
					}
					counter++;
				}
			}
		}

		{
			const int repeatX = mappingLayer.get_repetition_x();
			const int repeatY = mappingLayer.get_repetition_y();
			if (texRepeat[0] != repeatX || texRepeat[1] != repeatY) texRepeatSingle = false;
		}

		if (mappingType == sxsdk::enums::diffuse_mapping) {
			if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
				m_useDiffuseAlpha = true;
			}
			m_diffuseTexturesCount++;
		}
		if (mappingType == sxsdk::enums::normal_mapping) {
			m_normalTexturesCount++;
		}

		counter++;
	}

	if (!texRepeatSingle) texRepeat = sx::vec<int,2>(1, 1);
	if (counter >= 1) hasImage = true;
	if (counter == 0 || counter > 1) {
		switch (mappingType) {
		case sxsdk::enums::diffuse_mapping:
			m_diffuseImageName = "diffuse";
			break;

		case sxsdk::enums::normal_mapping:
			m_normalImageName = "normal";
			break;

		case sxsdk::enums::reflection_mapping:
			m_reflectionImageName = "reflection";
			break;

		case sxsdk::enums::roughness_mapping:
			m_roughnessImageName = "roughness";
			break;

		case sxsdk::enums::glow_mapping:
			m_glowImageName = "glow";
			break;

		case sxsdk::enums::transparency_mapping:
			m_transparencyImageName = "transparency";
			break;
		}
		if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) {
			m_occlusionImageName = "occlusion";
		}
		if (mappingType == MAPPING_TYPE_OPACITY) {
			m_opacityMaskImageName = "opacity";
		}
	}

	return true;
}

/**
 * 指定のマッピングの種類でのテクスチャサイズの最大を取得.
 * 異なるサイズのテクスチャが混在する場合、一番大きいサイズのテクスチャに合わせる.
 * @param[in]  mappingType   マッピングの種類.
 */
sx::vec<int,2> CImagesBlend::m_getMaxMappingImageSize (const sxsdk::enums::mapping_type mappingType)
{
	sx::vec<int,2> texSize(0, 0);

	int uvTexCoord = -1;
	int uvIndex = 0;
	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			uvIndex = mappingLayer.get_uv_mapping();
		} else {
			if (!Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

			try {
				compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
				COcclusionShaderData occlusionD;
				StreamCtrl::loadOcclusionParam(stream, occlusionD);
				uvIndex = occlusionD.uvIndex;
			} catch (...) {
				continue;
			}
		}
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else {
				if (type != MAPPING_TYPE_OPACITY) continue;
			}
		} else {
			if (mappingType == sxsdk::enums::normal_mapping) {
				// バンプマップの場合は法線マップに変換するため.
				if (type != mappingType && type != sxsdk::enums::bump_mapping) continue;
			} else {
				if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
					if (type != mappingType) continue;
				}
			}
		}

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;
		sx::vec<int,2> size = image->get_size();

		if (uvTexCoord < 0) uvTexCoord = uvIndex;
		if (uvTexCoord != uvIndex) continue;

		// 1つ前が「マット」の場合.
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					if (uvTexCoord == prevMappingLayer.get_uv_mapping()) {
						compointer<sxsdk::image_interface> image2(prevMappingLayer.get_image_interface());
						if (image2 && (image2->has_image()) && (image2->get_size().x) > 0 && (image2->get_size().y) > 0) {
							const sx::vec<int,2> size2 = image2->get_size();
							size.x = std::max(size.x, size2.x);
							size.y = std::max(size.y, size2.y);
						}
					}
				}
			}
		}

		texSize.x = std::max(texSize.x, size.x);
		texSize.y = std::max(texSize.y, size.y);
	}
	return texSize;
}

/**
 * マッピングレイヤで「マット」を持つか.
 * @param[in]  mappingType   マッピングの種類.
 */
 bool CImagesBlend::m_hasWeightTexture (const sxsdk::enums::mapping_type mappingType)
 {
	bool hasF = false;

	int uvIndex = 0;
	int uvTexCoord = -1;
	const int layersCou = m_surface->get_number_of_mapping_layers();
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			uvIndex = mappingLayer.get_uv_mapping();
		} else {
			if (!Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

			try {
				compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
				COcclusionShaderData occlusionD;
				StreamCtrl::loadOcclusionParam(stream, occlusionD);
				uvIndex = occlusionD.uvIndex;
			} catch (...) {
				continue;
			}
		}
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
			} else {
				if (type != MAPPING_TYPE_OPACITY) continue;
			}
		} else {
			if (mappingType == sxsdk::enums::normal_mapping) {
				// バンプマップの場合は法線マップに変換するため.
				if (type != mappingType && type != sxsdk::enums::bump_mapping) continue;
			} else {
				if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
					if (type != mappingType) continue;
				}
			}
		}

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		if (uvTexCoord < 0) uvTexCoord = uvIndex;
		if (uvTexCoord != uvIndex) continue;

		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					if (uvTexCoord == prevMappingLayer.get_uv_mapping()) {
						compointer<sxsdk::image_interface> image2(prevMappingLayer.get_image_interface());
						if (image2 && (image2->has_image()) && (image2->get_size().x) > 0 && (image2->get_size().y) > 0) {
							hasF = true;
							break;
						}
					}
				}
			}
		}
	}
	return hasF;
 }

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
compointer<sxsdk::image_interface> CImagesBlend::m_duplicateImage (sxsdk::image_interface* image, const sx::vec<int,2>& dstSize, const bool flipColor, const bool flipH, const bool flipV, const bool rotate90, const int repeatU, const int repeatV)
{
	compointer<sxsdk::image_interface> image2, dstImage;
	const int width  = dstSize.x;
	const int height = dstSize.y;
	if (width == 0 || height == 0) return dstImage;

	std::vector<sx::rgba8_class> lineCols, lineCols2;
	lineCols.resize(width);
	lineCols2.resize(width);

	if (repeatU > 1 || repeatV > 1) {
		dstImage = m_pScene->create_image_interface(dstSize);

		// gc->draw_imageでは、アルファ値も合成してしまうためアルファは分けて考える.
		if (Shade3DUtil::hasImageAlpha(image)) {
			const sx::vec<int,2> imgSize = image->get_size();
			const int imgWidth  = imgSize.x;
			const int imgHeight = imgSize.y;

			// アルファだけを取得し、Redに一時的に入れる.
			compointer<sxsdk::image_interface> alphaImage(m_pScene->create_image_interface(imgSize));
			{
				std::vector<sxsdk::rgba_class> lineD, lineD2;
				lineD.resize(imgWidth);
				lineD2.resize(imgWidth, sxsdk::rgba_class(0, 0, 0, 1));
				for (int y = 0; y < imgHeight; ++y) {
					image->get_pixels_rgba_float(0, y, imgWidth, 1, &(lineD[0]));
					for (int x = 0; x < imgWidth; ++x) lineD2[x].red = lineD[x].alpha;
					alphaImage->set_pixels_rgba_float(0, y, imgWidth, 1, &(lineD2[0]));
				}
			}

			// イメージのAlphaを1にする.
			compointer<sxsdk::image_interface> image2(m_pScene->create_image_interface(imgSize));
			{
				std::vector<sxsdk::rgba_class> lineD;
				lineD.resize(imgWidth);
				for (int y = 0; y < imgHeight; ++y) {
					image->get_pixels_rgba_float(0, y, imgWidth, 1, &(lineD[0]));
					for (int x = 0; x < imgWidth; ++x) lineD[x].alpha = 1.0f;
					image2->set_pixels_rgba_float(0, y, imgWidth, 1, &(lineD[0]));
				}
			}

			// RGBをタイリング.
			try {
				compointer<sxsdk::graphic_context_interface> gc(dstImage->get_graphic_context_interface());
				gc->set_color(sxsdk::rgb_class(1, 1, 1));
				const float dx = (float)width / (float)repeatU;
				const float dy = (float)height / (float)repeatV;

				sx::rectangle_class rect;
				for (float y = 0.0f; y < (float)(height - 1); y += dy) {
					for (float x = 0.0f; x < (float)(width - 1); x += dx) {
						rect.min = sx::vec<int,2>((int)x, (int)y);
						rect.max = sx::vec<int,2>((int)(x + dx), (int)(y + dy));
						rect.min.x = std::max(0, rect.min.x);
						rect.min.y = std::max(0, rect.min.y);
						rect.max.x = std::min(width - 1, rect.max.x);
						rect.max.y = std::min(height - 1, rect.max.y);
						gc->draw_image(image2, rect);
					}
				}
				gc->restore_color();
			} catch (...) { }

			// Aをタイリング.
			compointer<sxsdk::image_interface> dstImageA(m_pScene->create_image_interface(dstSize));
			try {
				compointer<sxsdk::graphic_context_interface> gc(dstImageA->get_graphic_context_interface());
				gc->set_color(sxsdk::rgb_class(1, 1, 1));
				const float dx = (float)width / (float)repeatU;
				const float dy = (float)height / (float)repeatV;

				sx::rectangle_class rect;
				for (float y = 0.0f; y < (float)(height - 1); y += dy) {
					for (float x = 0.0f; x < (float)(width - 1); x += dx) {
						rect.min = sx::vec<int,2>((int)x, (int)y);
						rect.max = sx::vec<int,2>((int)(x + dx), (int)(y + dy));
						rect.min.x = std::max(0, rect.min.x);
						rect.min.y = std::max(0, rect.min.y);
						rect.max.x = std::min(width - 1, rect.max.x);
						rect.max.y = std::min(height - 1, rect.max.y);
						gc->draw_image(alphaImage, rect);
					}
				}
				gc->restore_color();
			} catch (...) { }

			// RGB + AをdstImageに格納.
			try {
				std::vector<sxsdk::rgba_class> lineD, lineD2;
				lineD.resize(width);
				lineD2.resize(width);

				for (int y = 0; y < height; ++y) {
					dstImage->get_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
					dstImageA->get_pixels_rgba_float(0, y, width, 1, &(lineD2[0]));

					for (int x = 0; x < width; ++x) {
						lineD[x].alpha = lineD2[x].red;
					}
					dstImage->set_pixels_rgba_float(0, y, width, 1, &(lineD[0]));
				}

			} catch (...) { }

		} else {
			try {
				compointer<sxsdk::graphic_context_interface> gc(dstImage->get_graphic_context_interface());
				gc->set_color(sxsdk::rgb_class(1, 1, 1));
				const float dx = (float)width / (float)repeatU;
				const float dy = (float)height / (float)repeatV;

				sx::rectangle_class rect;
				for (float y = 0.0f; y < (float)(height - 1); y += dy) {
					for (float x = 0.0f; x < (float)(width - 1); x += dx) {
						rect.min = sx::vec<int,2>((int)x, (int)y);
						rect.max = sx::vec<int,2>((int)(x + dx), (int)(y + dy));
						rect.min.x = std::max(0, rect.min.x);
						rect.min.y = std::max(0, rect.min.y);
						rect.max.x = std::min(width - 1, rect.max.x);
						rect.max.y = std::min(height - 1, rect.max.y);
						gc->draw_image(image, rect);
					}
				}
				gc->restore_color();
			} catch (...) { }
		}

	} else {
		dstImage = Shade3DUtil::resizeImageWithAlpha(m_pScene, image, dstSize);
	}

	if (flipColor) {
		for (int y = 0; y < height; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			for (int x = 0; x < width; ++x) {
				lineCols[x].red   = 255 - lineCols[x].red;
				lineCols[x].green = 255 - lineCols[x].green;
				lineCols[x].blue  = 255 - lineCols[x].blue;
			}
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
		}
	}
	if (flipH) {
		for (int y = 0; y < height; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols2[0]));
			for (int x = 0; x < width; ++x) {
				lineCols[x] = lineCols2[width - x - 1];
			}
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
		}
	}

	if (flipV) {
		for (int y = 0; y < height / 2; ++y) {
			dstImage->get_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			dstImage->get_pixels_rgba(0, height - y - 1, width, 1, &(lineCols2[0]));
			dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols2[0]));
			dstImage->set_pixels_rgba(0, height - y - 1, width, 1, &(lineCols[0]));
		}
	}

	if (rotate90) {
		try {
			compointer<sxsdk::image_interface> image2 = Shade3DUtil::resizeImageWithAlpha(m_pScene, dstImage, sx::vec<int,2>(height, width));

			for (int y = 0; y < height; ++y) {
				image2->get_pixels_rgba(y, 0, 1, width, &(lineCols2[0]));
				for (int x = 0; x < width; ++x) {
					lineCols[x] = lineCols2[width - x - 1];

				}
				dstImage->set_pixels_rgba(0, y, width, 1, &(lineCols[0]));
			}
		} catch (...) { }
	}

	return dstImage;
 }

/**
 * 指定のテクスチャの合成処理.
 * @param[in] mappingType  マッピングの種類.
 * @param[in] repeatTex    繰り返し回数.
 */
bool CImagesBlend::m_blendImages (const sxsdk::enums::mapping_type mappingType, const sx::vec<int,2>& repeatTex)
{
	const int layersCou = m_surface->get_number_of_mapping_layers();
	sxsdk::image_interface* newImage = NULL;
	int newWidth, newHeight;
	int newRepeatX, newRepeatY;
	int newTexCoord;
	sxsdk::master_image_class* pNewMasterImage = NULL;
	std::string newTexName;
	int counter = 0;
	std::vector<sxsdk::rgba_class> rgbaLine0, rgbaLine, rgbaWeightLine;
	sxsdk::rgba_class col, whiteCol;
	whiteCol = sxsdk::rgba_class(1, 1, 1, 1);
	bool singleSimpleMapping = true;				// 1枚のテクスチャのみの参照で、色反転や左右反転/上下反転などがない場合は.
													// そのときのマスターイメージを採用 (ベイクする必要なしの場合、true).

	sxsdk::rgba_class baseCol(1, 1, 1, 1);
	if (mappingType == sxsdk::enums::diffuse_mapping) {
		// baseColorは別途乗算するため、ここでは基本設定としての拡散反射色を指定しない (以下はコメントアウト).
		//baseCol = sxsdk::rgba_class(m_surface->get_diffuse_color());
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		baseCol = sxsdk::rgba_class(m_surface->get_glow_color());
	}

	// 法線の中立値.
	const sxsdk::rgb_class normalDefaultCol(0.5f, 0.5f, 1.0f);
	const sxsdk::vec3 normalDefault = MathUtil::convRGBToNormal(normalDefaultCol);
	if (mappingType == sxsdk::enums::normal_mapping) {
		whiteCol = sxsdk::rgba_class(normalDefaultCol.red, normalDefaultCol.green, normalDefaultCol.blue, 1.0f);
		baseCol = whiteCol;
	}

	// 合成するテクスチャサイズ.
	const sx::vec<int,2> dstTexSize = m_getMaxMappingImageSize(mappingType);
	if (dstTexSize.x == 0 || dstTexSize.y == 0) return false;

	// 「マット」を持つか.
	const bool hasWeightTex = m_hasWeightTexture(mappingType);

	int uvIndex = 0;
	int occlusionChannelMix = 0;
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			uvIndex = mappingLayer.get_uv_mapping();
		} else {
			if (!Shade3DUtil::isOcclusionMappingLayer(&mappingLayer)) continue;

			try {
				compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
				COcclusionShaderData occlusionD;
				StreamCtrl::loadOcclusionParam(stream, occlusionD);
				uvIndex = occlusionD.uvIndex;
				occlusionChannelMix = occlusionD.channelMix;
			} catch (...) {
				continue;
			}
		}
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.

		float weight  = std::min(std::max(mappingLayer.get_weight(), 0.0f), 1.0f);
		const int type = mappingLayer.get_type();
		if (mappingType == sxsdk::enums::normal_mapping && (type == sxsdk::enums::normal_mapping || type == sxsdk::enums::bump_mapping)) {
			if (m_normalTexturesCount == 1) {
				m_normalStrength = mappingLayer.get_weight();
				weight = 1.0f;
			}
		}

		const float weight2 = 1.0f - weight;
		if (MathUtil::isZero(weight)) continue;

		bool alphaTrans = false;
		if (mappingType == MAPPING_TYPE_OPACITY) {
			if (type == sxsdk::enums::diffuse_mapping) {
				if (mappingLayer.get_channel_mix() != sxsdk::enums::mapping_transparent_alpha_mode) continue;
				alphaTrans = true;
			} else {
				if (type != MAPPING_TYPE_OPACITY) continue;
			}
		} else {
			if (mappingType == sxsdk::enums::normal_mapping) {
				// バンプマップの場合は法線マップに変換するため.
				if (type != mappingType && type != sxsdk::enums::bump_mapping) continue;
			} else {
				if (mappingType != MAPPING_TYPE_GLTF_OCCLUSION) {
					if (type != mappingType) continue;
				}
			}
		}

		float repeatU = (float)mappingLayer.get_repetition_X();
		float repeatV = (float)mappingLayer.get_repetition_Y();
		if (repeatTex[0] != 1 || repeatTex[1] != 1) {
			repeatU = 1;
			repeatV = 1;
		}

		// 1つ前が「マット」の場合.
		compointer<sxsdk::image_interface> weightImage;
		int weightWidth  = 0;
		int weightHeight = 0;
		bool weightFlipColor = false;
		bool weightFlipH     = false;
		bool weightFlipV     = false;
		bool weightRotate90  = false;
		int weightRepeatU = 1;
		int weightRepeatV = 1;
		if (i > 0) {
			sxsdk::mapping_layer_class& prevMappingLayer = m_surface->mapping_layer(i - 1);
			if (prevMappingLayer.get_pattern() == sxsdk::enums::image_pattern) {
				if (prevMappingLayer.get_type() == sxsdk::enums::weight_mapping) {
					if (newTexCoord == prevMappingLayer.get_uv_mapping()) {
						weightFlipColor = prevMappingLayer.get_flip_color();
						weightFlipH     = prevMappingLayer.get_horizontal_flip();
						weightFlipV     = prevMappingLayer.get_vertical_flip();
						weightRotate90  = prevMappingLayer.get_swap_axes();
						weightRepeatU   = prevMappingLayer.get_repetition_X();
						weightRepeatV   = prevMappingLayer.get_repetition_Y();
						if (repeatTex[0] != 1 || repeatTex[1] != 1) {
							weightRepeatU = 1;
							weightRepeatV = 1;
						}

						try {
							weightImage = prevMappingLayer.get_image_interface();
							if (weightImage && (weightImage->has_image())) {
								weightWidth  = weightImage->get_size().x;
								weightHeight = weightImage->get_size().y;
								if (weightWidth <= 1 || weightHeight <= 1) {
									weightWidth = weightHeight = 0;
								}
							}
						} catch (...) {
							weightWidth = weightHeight = 0;
						}
					}
				}
			}
		}

		const int blendMode = mappingLayer.get_blend_mode();
		try {
			compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
			if (!image || !(image->has_image())) continue;
			const int width  = image->get_size().x;
			const int height = image->get_size().y;
			if (width <= 1 || height <= 1) continue;

			// Diffuseテクスチャの場合はAlpha要素をRGBに入れる.
			if (mappingType == MAPPING_TYPE_OPACITY) {
				if (type == sxsdk::enums::diffuse_mapping) {
					compointer<sxsdk::image_interface> image2(image->duplicate_image());

					std::vector<sxsdk::rgba_class> colA;
					float fA;
					colA.resize(width);
					for (int y = 0; y < height; ++y) {
						image2->get_pixels_rgba_float(0, y, width, 1, &(colA[0]));
						for (int x = 0; x < width; ++x) {
							fA = colA[x].alpha;
							colA[x] = sxsdk::rgba_class(fA, fA, fA, 1.0f);
						}
						image2->set_pixels_rgba_float(0, y, width, 1, &(colA[0]));
					}
					image = image2;
				}
			}

			const bool flipColor = mappingLayer.get_flip_color();			// 色反転.
			const bool flipH     = mappingLayer.get_horizontal_flip();		// 左右反転.
			const bool flipV     = mappingLayer.get_vertical_flip();		// 上下反転.
			const bool rotate90  = mappingLayer.get_swap_axes();			// 90度回転.
			int channelMix = mappingLayer.get_channel_mix();			// イメージのチャンネル.
			if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) {
				if (occlusionChannelMix == 0) channelMix = sxsdk::enums::mapping_grayscale_red_mode;
				else if (occlusionChannelMix == 1) channelMix = sxsdk::enums::mapping_grayscale_green_mode;
				else if (occlusionChannelMix == 2) channelMix = sxsdk::enums::mapping_grayscale_blue_mode;
			}
			const bool useChannelMix = (channelMix == sxsdk::enums::mapping_grayscale_alpha_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_red_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_green_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_blue_mode ||
										channelMix == sxsdk::enums::mapping_grayscale_average_mode);

			if (!newImage) {
				newImage = m_pScene->create_image_interface(dstTexSize);
				newWidth    = dstTexSize.x;
				newHeight   = dstTexSize.y;
				newRepeatX  = repeatU;
				newRepeatY  = repeatV;
				newTexCoord = uvIndex;
				rgbaLine.resize(newWidth);
				rgbaLine0.resize(newWidth);
				if (hasWeightTex) rgbaWeightLine.resize(newWidth);

				// マスターイメージを持つか調べる.
				pNewMasterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);

				if (pNewMasterImage) {
					// そのまま画像を採用する可能性があるかどうか.
					if (singleSimpleMapping) {
						singleSimpleMapping = (!flipColor && !flipH && !flipV && !rotate90 && !useChannelMix);
					}
					if (type == sxsdk::enums::bump_mapping) singleSimpleMapping = false;
					newTexName = pNewMasterImage->get_name();
				}
			}
			if (uvIndex != newTexCoord) continue;

			sx::vec<int,2> newImgSize = newImage->get_size();
			compointer<sxsdk::image_interface> image2 = m_duplicateImage(image, newImgSize, flipColor, flipH, flipV, rotate90, repeatU, repeatV);

			compointer<sxsdk::image_interface> weightImage2;
			if (weightWidth > 0) {
				weightImage2 = m_duplicateImage(weightImage, newImgSize, weightFlipColor, weightFlipH, weightFlipV, weightRotate90, weightRepeatU, weightRepeatV);
			}

			// バンプの場合は法線マップに置き換え.
			if (mappingType == sxsdk::enums::normal_mapping) {
				if (type == sxsdk::enums::bump_mapping) image2->convert_bump_to_normalmap(1.0f);
			}

			for (int y = 0; y < newHeight; ++y) {
				image2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));

				if (weightWidth > 0) {
					weightImage2->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaWeightLine[0]));
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

				// 「アルファ透明」でない場合.
				if (mappingType == sxsdk::enums::diffuse_mapping) {
					if (channelMix != sxsdk::enums::mapping_transparent_alpha_mode) {
						for (int x = 0; x < newWidth; ++x) rgbaLine[x].alpha = 1.0f;
					}
				}

				// Roughnessの場合、Shade3Dはテクスチャの濃淡は逆転している.
				// 「テクスチャを加工せずにベイク」の場合はそのまま採用するため、この処理は行わない.
				if (!m_exportParam.bakeWithoutProcessingTextures) {
					if (mappingType == sxsdk::enums::roughness_mapping) {
						for (int x = 0; x < newWidth; ++x) {
							rgbaLine[x].red   = 1.0f - rgbaLine[x].red;
							rgbaLine[x].green = 1.0f - rgbaLine[x].green;
							rgbaLine[x].blue  = 1.0f - rgbaLine[x].blue;
						}
					}
				}

				if (counter == 0) {
					float aV;
					if (blendMode == 7 && mappingType != sxsdk::enums::normal_mapping) {				// 「乗算」合成.
						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							rgbaLine[x] = (baseCol * w2 + rgbaLine[x] * w) * baseCol;
						}

					} else {										// 「通常」合成.
						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							rgbaLine[x] = rgbaLine[x] * w + baseCol * w2;
						}
					}

				} else {
					newImage->get_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine0[0]));

					if (mappingType == sxsdk::enums::normal_mapping) {
						sxsdk::vec3 n, n2;
						sxsdk::rgb_class col;
						for (int x = 0; x < newWidth; ++x) {
							const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
							const float w2 = 1.0f - w;
							n  = MathUtil::convRGBToNormal(sxsdk::rgb_class(rgbaLine[x].red, rgbaLine[x].green, rgbaLine[x].blue));
							n2 = MathUtil::convRGBToNormal(sxsdk::rgb_class(rgbaLine0[x].red, rgbaLine0[x].green, rgbaLine0[x].blue));

							n = n * w + n2 * w2;
							col = MathUtil::convNormalToRGB(n);
							rgbaLine[x] = sxsdk::rgba_class(col.red, col.green, col.blue, 1.0f);
						}

					} else {
						if (blendMode == sxsdk::enums::mapping_blend_mode) {		// 「通常」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;
								rgbaLine[x] = rgbaLine[x] * w + rgbaLine0[x] * w2;
							}

						} else if (blendMode == sxsdk::enums::mapping_mul_mode) {	// 「乗算 (レガシー)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;
								rgbaLine[x] = rgbaLine[x] * rgbaLine0[x] * w;
							}
						} else if (blendMode == 7) {							// 「乗算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								const float w2 = 1.0f - w;

								rgbaLine[x] = (whiteCol * w2 + rgbaLine[x] * w) * rgbaLine0[x];
							}

						} else if (blendMode == sxsdk::enums::mapping_add_mode) {		// 「加算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x] = rgbaLine0[x] + (rgbaLine[x] * w);
								rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
								rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
								rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
								rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
							}
						} else if (blendMode == sxsdk::enums::mapping_sub_mode) {		// 「減算」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x] = rgbaLine0[x] - (rgbaLine[x] * w);
								rgbaLine[x].red   = std::min(std::max(0.0f, rgbaLine[x].red), 1.0f);
								rgbaLine[x].green = std::min(std::max(0.0f, rgbaLine[x].green), 1.0f);
								rgbaLine[x].blue  = std::min(std::max(0.0f, rgbaLine[x].blue), 1.0f);
								rgbaLine[x].alpha = std::min(std::max(0.0f, rgbaLine[x].alpha), 1.0f);
							}
						} else if (blendMode == sxsdk::enums::mapping_min_mode) {		// 「比較(暗)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x].red   = std::min(rgbaLine0[x].red,   rgbaLine[x].red   * w);
								rgbaLine[x].green = std::min(rgbaLine0[x].green, rgbaLine[x].green * w);
								rgbaLine[x].blue  = std::min(rgbaLine0[x].blue,  rgbaLine[x].blue  * w);
							}
						} else if (blendMode == sxsdk::enums::mapping_max_mode) {		// 「比較(明)」合成.
							for (int x = 0; x < newWidth; ++x) {
								const float w  = alphaTrans ? 1.0f : ((weightWidth > 0) ? rgbaWeightLine[x].red * weight : weight);
								rgbaLine[x].red   = std::max(rgbaLine0[x].red,   rgbaLine[x].red   * w);
								rgbaLine[x].green = std::max(rgbaLine0[x].green, rgbaLine[x].green * w);
								rgbaLine[x].blue  = std::max(rgbaLine0[x].blue,  rgbaLine[x].blue  * w);
							}
						}
					}
				}
				newImage->set_pixels_rgba_float(0, y, newWidth, 1, &(rgbaLine[0]));
			}
			counter++;

		} catch (...) { }
	}
	if (!newImage) return false;

	// 複数のテクスチャをベイクした場合.
	if (counter >= 2) {
		if (singleSimpleMapping) singleSimpleMapping = false;
	}

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		m_diffuseImage    = newImage;
		m_diffuseRepeat   = repeatTex;
		m_diffuseTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == sxsdk::enums::normal_mapping) {
		m_normalImage    = newImage;
		m_normalRepeat   = repeatTex;
		m_normalTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		m_reflectionImage    = newImage;
		m_reflectionRepeat   = repeatTex;
		m_reflectionTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		m_roughnessImage    = newImage;
		m_roughnessRepeat   = repeatTex;
		m_roughnessTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		m_glowImage    = newImage;
		m_glowRepeat   = repeatTex;
		m_glowTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == sxsdk::enums::transparency_mapping) {
		m_transparencyImage    = newImage;
		m_transparencyRepeat   = repeatTex;
		m_transparencyTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == MAPPING_TYPE_OPACITY) {
		m_opacityMaskImage    = newImage;
		m_opacityMaskRepeat   = repeatTex;
		m_opacityMaskTexCoord = newTexCoord;
		return true;
	}
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) {
		m_occlusionImage    = newImage;
		m_occlusionRepeat   = repeatTex;
		m_occlusionTexCoord = newTexCoord;
		return true;
	}
	IMAGE_INTERFACE_RELEASE(newImage);

	return false;
}

/**
 * 「不透明」と「透明」のテクスチャを分離もしくは合成して再格納.
 * @return 不透明度のテクスチャを格納.
 */
sxsdk::image_interface* CImagesBlend::m_storeOpasicyTransparencyTexture ()
{
	// TransparencyとOpacityMaskを持つ場合、合成してdstOpacityImageに入れる.
	// 最終的にTransparencyテクスチャは削除し、m_opacityMaskImageに格納される.
	sxsdk::image_interface* dstOpacityImage = NULL;
	if (m_exportParam.separateOpacityAndTransmission) {
		// 「「不透明(Opacity)」と「透明(Transmission)」を分ける」がOnの場合.
		if (m_opacityMaskImage) {
			const int width  = m_opacityMaskImage->get_size().x;
			const int height = m_opacityMaskImage->get_size().y;

			std::vector<sxsdk::rgba_class> col1A;
			col1A.resize(width);

			dstOpacityImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
			for (int y = 0; y < height; ++y) {
				m_opacityMaskImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				dstOpacityImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
			}
		}

	} else {
		if (m_transparencyImage || m_opacityMaskImage) {
			if (m_transparencyImage) m_transparency = 0.0f;
			if (m_transparencyImage) {
				const int width  = m_transparencyImage->get_size().x;
				const int height = m_transparencyImage->get_size().y;

				std::vector<sxsdk::rgba_class> col1A;
				col1A.resize(width);

				// transparencyを逆転して、Opacityにする.
				dstOpacityImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
				for (int y = 0; y < height; ++y) {
					m_transparencyImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
					for (int x = 0; x < width; ++x) {
						const float fV = 1.0f - col1A[x].red;
						col1A[x] = sxsdk::rgba_class(fV, fV, fV, 1.0f);
					}
					dstOpacityImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				}
			} else if (m_opacityMaskImage) {
				const int width  = m_opacityMaskImage->get_size().x;
				const int height = m_opacityMaskImage->get_size().y;

				std::vector<sxsdk::rgba_class> col1A;
				col1A.resize(width);

				dstOpacityImage = m_pScene->create_image_interface(sx::vec<int,2>(width, height));
				for (int y = 0; y < height; ++y) {
					m_opacityMaskImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
					dstOpacityImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				}
			}

			// transparencyとopacityMaskを合成 ==> dstOpacityImageに出力.
			if (dstOpacityImage) {
				if (m_transparencyImage && m_opacityMaskImage) {
					if (m_transparencyTexCoord == m_opacityMaskTexCoord && m_transparencyRepeat == m_opacityMaskRepeat) {
						const int width  = dstOpacityImage->get_size().x;
						const int height = dstOpacityImage->get_size().y;
						const int width2  = m_opacityMaskImage->get_size().x;
						const int height2 = m_opacityMaskImage->get_size().y;

						std::vector<sxsdk::rgba_class> col1A, col2A;
						col1A.resize(width);
						col2A.resize(width);
						if (width != width2 || height != height2) {
							sxsdk::image_interface* image = Shade3DUtil::resizeImageWithAlphaNotCom(m_pScene, m_opacityMaskImage, sx::vec<int,2>(width, height));
							IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);
							m_opacityMaskImage = image;
						}
						for (int y = 0; y < height; ++y) {
							dstOpacityImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
							m_opacityMaskImage->get_pixels_rgba_float(0, y, width, 1, &(col2A[0]));

							for (int x = 0; x < width; ++x) {
								const float fV = col1A[x].red * col2A[x].red;
								col1A[x] = sxsdk::rgba_class(fV, fV, fV, 1.0f);
							}
							dstOpacityImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
						}
					}
				}
			}
		}
	}
	return dstOpacityImage;
}

/**
 * 透明度テクスチャがある場合に、BaseColorと透明色を考慮してglTF用に補正する.
 */
void CImagesBlend::m_convTransparencyData ()
{
	if (!m_exportParam.separateOpacityAndTransmission) return;
	if (m_transparency <= 0.0f) return;
	const float trans2 = 1.0f - m_transparency;

	const sxsdk::rgb_class transCol = m_surface->get_transparency_color();
	if (!m_transparencyImage) {
		// DiffuseColorは、透明の色と合成される.
		m_diffuseColor = transCol * m_transparency + m_diffuseColor * trans2;
		return;
	}

	const int transWid = m_transparencyImage->get_size().x;
	const int transHei = m_transparencyImage->get_size().y;

	// 透明度テクスチャの値と透明色、拡散反射色より、拡散反射テクスチャを作成.
	if (!m_diffuseImage) {
		m_diffuseImage = m_pScene->create_image_interface(sx::vec<int,2>(transWid, transHei));

		std::vector<sxsdk::rgba_class> lineCols;
		lineCols.resize(transWid);
		float vR;
		for (int y = 0; y < transHei; ++y) {
			m_transparencyImage->get_pixels_rgba_float(0, y, transWid, 1, &(lineCols[0]));
			for (int x = 0; x < transWid; ++x) {
				vR = lineCols[x].red * m_transparency;
				lineCols[x].red   = transCol.red   * vR + m_diffuseColor.red   * (1.0f - vR);
				lineCols[x].green = transCol.green * vR + m_diffuseColor.green * (1.0f - vR);
				lineCols[x].blue  = transCol.blue  * vR + m_diffuseColor.blue  * (1.0f - vR);
				lineCols[x].alpha = 1.0f;
			}
			m_diffuseImage->set_pixels_rgba_float(0, y, transWid, 1, &(lineCols[0]));
		}
		m_diffuseRepeat   = m_transparencyRepeat;
		m_diffuseTexCoord = m_transparencyTexCoord;

		m_diffuseColor = sxsdk::rgb_class(1, 1, 1);
		m_diffuseTexturesCount = 1;
		m_hasDiffuseImage   = true;
		m_diffuseAlphaTrans = false;
		m_useDiffuseAlpha   = false;
		return;
	}

	// 透明テクスチャと拡散反射テクスチャが存在する場合、拡散反射テクスチャに対して透明度を合成.
	{
		const int diffuseWid = m_diffuseImage->get_size().x;
		const int diffuseHei = m_diffuseImage->get_size().y;

		// UVレイヤが異なる場合は処理できないためreturn.
		if (m_diffuseTexCoord != m_transparencyTexCoord) return;

		if (m_diffuseRepeat == m_transparencyRepeat) {
			compointer<sxsdk::image_interface> tImg = Shade3DUtil::resizeImageWithAlpha(m_pScene, m_transparencyImage, sx::vec<int,2>(diffuseWid, diffuseHei));

			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(diffuseWid);
			lineCols2.resize(diffuseWid);
			float vR;
			sxsdk::rgba_class col;
			for (int y = 0; y < diffuseHei; ++y) {
				tImg->get_pixels_rgba_float(0, y, diffuseWid, 1, &(lineCols[0]));
				m_diffuseImage->get_pixels_rgba_float(0, y, diffuseWid, 1, &(lineCols2[0]));
				for (int x = 0; x < diffuseWid; ++x) {
					vR  = lineCols[x].red * m_transparency;
					col = lineCols2[x] * sxsdk::rgba_class(m_diffuseColor, 1.0f);
					lineCols[x].red   = transCol.red   * vR + col.red   * (1.0f - vR);
					lineCols[x].green = transCol.green * vR + col.green * (1.0f - vR);
					lineCols[x].blue  = transCol.blue  * vR + col.blue  * (1.0f - vR);
					lineCols[x].alpha = col.alpha;
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, diffuseWid, 1, &(lineCols[0]));
			}

			m_diffuseColor = sxsdk::rgb_class(1, 1, 1);
			m_diffuseTexturesCount = 1;
			m_hasDiffuseImage   = true;
			m_diffuseAlphaTrans = false;
			m_useDiffuseAlpha   = false;
			return;
		} else {
			// ここに来ることはないはず (あらかじめ、同一の反復回数でベイク済みのため).
		}
	}
}

/**
 * diffuse/roughness/metallicのテクスチャを、Shade3Dの状態からPBRマテリアルに変換.
 */
void CImagesBlend::m_convShade3DToPBRMaterial ()
{
	// 反射が大きい場合にbaseColorを黒にするとglTFとして見たときは黒くなるため、reflectionも考慮.
	// なお、ここでの「色」はテクスチャに乗算するものなので、リニア変換は行わない.
	// 「拡散反射色」「発光色」については、テクスチャ作成時にすでに考慮しているのでここでは入れない.
	const float diffuseV = std::min(1.0f, std::max(0.0f, m_surface->get_diffuse()));
	const sxsdk::rgb_class col0 = (m_diffuseImage && m_diffuseTexturesCount >= 2) ? sxsdk::rgb_class(1, 1, 1) : (m_surface->get_diffuse_color());
	sxsdk::rgb_class col = col0 * diffuseV;
	sxsdk::rgb_class reflectionCol = m_surface->get_reflection_color();
	const float reflectionV  = std::max(std::min(1.0f, m_surface->get_reflection()), 0.0f);
	const float reflectionV2 = 1.0f - reflectionV;
	col = col * reflectionV2 + reflectionCol * reflectionV;
	m_diffuseColor.red   = std::min(col0.red, col.red);
	m_diffuseColor.green = std::min(col0.green, col.green);
	m_diffuseColor.blue  = std::min(col0.blue, col.blue);

	m_metallic     = reflectionV;
	m_roughness    = m_surface->get_roughness();
	m_transparency = m_surface->get_transparency();

	const float emissiveV = std::min(1.0f, std::max(0.0f, m_surface->get_glow()));
	const sxsdk::rgb_class emCol0 = m_glowImage ? sxsdk::rgb_class(1, 1, 1) : (m_surface->get_glow_color());
	sxsdk::rgb_class emCol = emCol0 * emissiveV;
	m_emissiveColor = emCol;

	// 光沢(Specular)値により、Roughnessを調整.
	if (!m_roughnessImage) {
		if (m_metallic < 0.3f) {
			if (m_surface->get_has_specular_1()) {
				float specularV = (m_surface->get_highlight_color().red + m_surface->get_highlight_color().green + m_surface->get_highlight_color().blue) * 0.3333f;
				specularV *= m_surface->get_highlight();
				float specularPower = std::max(0.0001f, m_surface->get_highlight_size());

				const float sV = std::pow(specularV, 1.0f + specularPower * 2.0f);

				m_roughness = std::max((1.0f - sV) * 0.5f, m_roughness);
				if (MathUtil::isZero(sV)) m_roughness = 1.0f;

			} else {
				m_roughness = std::max(0.5f, m_roughness);
			}
		}
	}

	// 「不透明」と「透明」のテクスチャを分離もしくは合成して再格納.
	sxsdk::image_interface* dstOpacityImage = m_storeOpasicyTransparencyTexture();

	// DiffuseとOpacityの両方が存在する場合、DiffuseのA要素としてOpacityを格納.
	if (m_diffuseImage && dstOpacityImage) {
		if (m_diffuseTexCoord == m_opacityMaskTexCoord && m_diffuseRepeat == m_opacityMaskRepeat) {
			const int width  = m_diffuseImage->get_size().x;
			const int height = m_diffuseImage->get_size().y;
			compointer<sxsdk::image_interface> optImage = Shade3DUtil::resizeImageWithAlpha(m_pScene, dstOpacityImage, sx::vec<int,2>(width, height));
			
			std::vector<sxsdk::rgba_class> col1A;
			std::vector<sxsdk::rgba_class> col2A;
			col1A.resize(width);
			col2A.resize(width);
			for (int y = 0; y < height; ++y) {
				m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				optImage->get_pixels_rgba_float(0, y, width, 1, &(col2A[0]));
				for (int x = 0; x < width; ++x) {
					col1A[x].alpha = col2A[x].red;
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
			}
			m_diffuseAlphaTrans = true;
		}
	}

	// DiffuseとMetallicの両方のテクスチャが存在する場合.
	if (m_diffuseImage && m_reflectionImage) {
		const int width  = m_diffuseImage->get_size().x;
		const int height = m_diffuseImage->get_size().y;
		const int widthM  = m_reflectionImage->get_size().x;
		const int heightM = m_reflectionImage->get_size().y;

		// m_diffuseImageとm_reflectionImageが異なるサイズの場合、大きい方にリサイズ.
		int widthS  = width;
		int heightS = height;
		if (width != widthM || height != heightM) {
			widthS  = std::max(width, widthM);
			heightS = std::max(height, heightM);

			if (width != widthS || height != heightS) {
				sxsdk::image_interface* image = Shade3DUtil::resizeImageWithAlphaNotCom(m_pScene, m_diffuseImage, sx::vec<int,2>(widthS, heightS));
				IMAGE_INTERFACE_RELEASE(m_diffuseImage);
				m_diffuseImage = image;
			}
			if (widthM != widthS || heightM != heightS) {
				sxsdk::image_interface* image = Shade3DUtil::resizeImageWithAlphaNotCom(m_pScene, m_reflectionImage, sx::vec<int,2>(widthS, heightS));
				IMAGE_INTERFACE_RELEASE(m_reflectionImage);
				m_reflectionImage = image;
			}
		}

		{
			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(widthS);
			lineCols2.resize(widthS);

			sxsdk::rgba_class col;
			sxsdk::rgb_class baseColorCol;
			float rV, d1, d2, prevRV;
			sxsdk::vec3 hsv;

			for (int y = 0; y < heightS; ++y) {
				m_diffuseImage->get_pixels_rgba_float(0, y, widthS, 1, &(lineCols[0]));
				m_reflectionImage->get_pixels_rgba_float(0, y, widthS, 1, &(lineCols2[0]));
				for (int x = 0; x < widthS; ++x) {
					// Shade3Dのdiffuse/reflection値より、PBRマテリアルでのdiffuse/metallicに変換.
					col = lineCols[x];
					prevRV = lineCols2[x].red;
					rV  = prevRV * reflectionV;

					baseColorCol.red   = std::max(std::min(col.red   + rV, 1.0f), 0.0f);
					baseColorCol.green = std::max(std::min(col.green + rV, 1.0f), 0.0f);
					baseColorCol.blue  = std::max(std::min(col.blue  + rV, 1.0f), 0.0f);
					const float baseV = MathUtil::rgb_to_grayscale(baseColorCol);

					// Metallicの計算.
					float metallicV = 0.0f;
					if (baseV > 0.0f) {
						d1 = 0.7f;
						d2 = 1.0f - d1;
						metallicV = std::min(rV / (baseV * d1 + d2), 1.0f);
					}

					baseColorCol = col;
					if (m_metallic > 0.3f && metallicV > 0.0f) {
						// RGB ==> HSVに変換.
						hsv = MathUtil::rgb_to_hsv(col);
						const float orgS = hsv.y;		// 彩度.
						const float orgV = hsv.z;		// 明度.

						// Diffuse色の明度をMetallicで調整.
						const float dd = (1.0f - hsv.y) + diffuseV + 1.0f;
						hsv.z = std::max(0.0f, std::min(1.0f, hsv.z * dd));
						hsv.z = hsv.z * (1.0f - (1.0f - diffuseV) * 0.2f);
						hsv.z = orgV * (1.0f - metallicV) + hsv.z * metallicV;

						// Metallic成分が強い場合、Diffuse成分が弱い場合は、彩度をわずかに下げる.
						d1 = (1.0f - diffuseV) * 0.8f;
						d2 = 1.0f - d1;

						const float m1 = metallicV * 0.6f;
						float m2 = std::max((1.0f - m1) * d2, 0.5f);
						hsv.y = hsv.y * m2;

						baseColorCol = MathUtil::hsv_to_rgb(hsv);

						const float blendV = metallicV * 0.5f;
						baseColorCol.red   = baseColorCol.red   * blendV + col.red   * (1.0f - blendV);
						baseColorCol.green = baseColorCol.green * blendV + col.green * (1.0f - blendV);
						baseColorCol.blue  = baseColorCol.blue  * blendV + col.blue  * (1.0f - blendV);
					}

					lineCols[x].red   = baseColorCol.red;
					lineCols[x].green = baseColorCol.green;
					lineCols[x].blue  = baseColorCol.blue;

					if (reflectionV > 0.0f) {
						metallicV /= reflectionV;
						metallicV = std::max(std::min(metallicV, 1.0f), 0.0f);
					} else {
						metallicV = prevRV;
					}
					lineCols2[x] = sxsdk::rgba_class(metallicV, metallicV, metallicV, 1.0f);
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, widthS, 1, &(lineCols[0]));
				m_reflectionImage->set_pixels_rgba_float(0, y, widthS, 1, &(lineCols2[0]));
			}
		}

		if (widthS != width || heightS != height) {
			sxsdk::image_interface* image = Shade3DUtil::resizeImageWithAlphaNotCom(m_pScene, m_diffuseImage, sx::vec<int,2>(width, height));
			IMAGE_INTERFACE_RELEASE(m_diffuseImage);
			m_diffuseImage = image;
		}
		if (widthS != widthM || heightS != heightM) {
			sxsdk::image_interface* image = Shade3DUtil::resizeImageWithAlphaNotCom(m_pScene, m_reflectionImage, sx::vec<int,2>(widthM, heightM));
			IMAGE_INTERFACE_RELEASE(m_reflectionImage);
			m_reflectionImage = image;
		}
	}

	// Diffuseのテクスチャが存在する場合.
	// m_metallicの値により、Diffuseテクスチャの調整を行う.
	if (m_diffuseImage && !m_reflectionImage) {
		const int width  = m_diffuseImage->get_size().x;
		const int height = m_diffuseImage->get_size().y;

		std::vector<sxsdk::rgba_class> lineCols;
		lineCols.resize(width);

		sxsdk::rgba_class col;
		sxsdk::rgb_class baseColorCol;
		float rV, d1, d2;
		sxsdk::vec3 hsv;

		for (int y = 0; y < height; ++y) {
			m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
			for (int x = 0; x < width; ++x) {
				// Shade3Dのdiffuse/reflection値より、PBRマテリアルでのdiffuse/metallicに変換.
				col = lineCols[x];
				rV  = m_metallic;

				baseColorCol.red   = std::max(std::min(col.red   + rV, 1.0f), 0.0f);
				baseColorCol.green = std::max(std::min(col.green + rV, 1.0f), 0.0f);
				baseColorCol.blue  = std::max(std::min(col.blue  + rV, 1.0f), 0.0f);
				const float baseV = MathUtil::rgb_to_grayscale(baseColorCol);

				// Metallicの計算.
				float metallicV = 0.0f;
				if (baseV > 0.0f) {
					d1 = 0.7f;
					d2 = 1.0f - d1;
					metallicV = std::min(rV / (baseV * d1 + d2), 1.0f);
				}

				baseColorCol = col;
				if (m_metallic > 0.3f && metallicV > 0.0f) {
					// RGB ==> HSVに変換.
					hsv = MathUtil::rgb_to_hsv(col);
					const float orgS = hsv.y;		// 彩度.
					const float orgV = hsv.z;		// 明度.

					// Diffuse色の明度をMetallicで調整.
					const float dd = (1.0f - hsv.y) + diffuseV + 1.0f;
					hsv.z = std::max(0.0f, std::min(1.0f, hsv.z * dd));
					hsv.z = hsv.z * (1.0f - (1.0f - diffuseV) * 0.2f);
					hsv.z = orgV * (1.0f - metallicV) + hsv.z * metallicV;

					// Metallic成分が強い場合、Diffuse成分が弱い場合は、彩度をわずかに下げる.
					d1 = (1.0f - diffuseV) * 0.8f;
					d2 = 1.0f - d1;

					const float m1 = metallicV * 0.6f;
					float m2 = std::max((1.0f - m1) * d2, 0.5f);
					hsv.y = hsv.y * m2;

					baseColorCol = MathUtil::hsv_to_rgb(hsv);

					const float blendV = metallicV * 0.5f;
					baseColorCol.red   = baseColorCol.red   * blendV + col.red   * (1.0f - blendV);
					baseColorCol.green = baseColorCol.green * blendV + col.green * (1.0f - blendV);
					baseColorCol.blue  = baseColorCol.blue  * blendV + col.blue  * (1.0f - blendV);
				}

				lineCols[x].red   = baseColorCol.red;
				lineCols[x].green = baseColorCol.green;
				lineCols[x].blue  = baseColorCol.blue;
			}
			m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(lineCols[0]));
		}
	}

	// m_opacityMaskImageを参照し、完全に透明な箇所はMetallic = 0.0/Roughness = 1.0を入れる.
	if (m_opacityMaskImage) {
		const int oWidth  = m_opacityMaskImage->get_size().x;
		const int oHeight = m_opacityMaskImage->get_size().y;

		if (m_metallic > 0.0f) {
			if (!m_reflectionImage) {		// Metallicテクスチャがない場合は作成.
				m_reflectionImage = m_pScene->create_image_interface(sx::vec<int,2>(oWidth, oHeight));
				std::vector<sxsdk::rgba_class> lineCols, lineCols2;
				lineCols.resize(oWidth);
				lineCols2.resize(oWidth);

				sxsdk::rgba_class col;
				const sxsdk::rgba_class whiteCol(1, 1, 1, 1);
				const sxsdk::rgba_class blackCol(0, 0, 0, 1);

				for (int y = 0; y < oHeight; ++y) {
					m_opacityMaskImage->get_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));
					for (int x = 0; x < oWidth; ++x) {
						col = lineCols[x];
						lineCols2[x] = MathUtil::isZero(col.red) ? blackCol : whiteCol;
					}
					m_reflectionImage->set_pixels_rgba_float(0, y, oWidth, 1, &(lineCols2[0]));
				}
				m_hasReflectionImage = true;
				m_reflectionRepeat   = m_opacityMaskRepeat;
				m_reflectionTexCoord = m_opacityMaskTexCoord;
			} else {
				const int rWidth  = m_reflectionImage->get_size().x;
				const int rHeight = m_reflectionImage->get_size().y;
				if (oWidth != rWidth || oHeight != rHeight) {
					compointer<sxsdk::image_interface> soImage = Shade3DUtil::resizeImageWithAlpha(m_pScene, m_opacityMaskImage, sx::vec<int,2>(rWidth, rHeight));

					std::vector<sxsdk::rgba_class> lineCols, lineCols2;
					lineCols.resize(rWidth);
					lineCols2.resize(rWidth);

					sxsdk::rgba_class col;

					for (int y = 0; y < rHeight; ++y) {
						soImage->get_pixels_rgba_float(0, y, rWidth, 1, &(lineCols[0]));
						m_reflectionImage->get_pixels_rgba_float(0, y, rWidth, 1, &(lineCols2[0]));
						for (int x = 0; x < rWidth; ++x) {
							const float fV = lineCols[x].red * lineCols2[x].red;
							lineCols2[x] = sxsdk::rgba_class(fV, fV, fV, 1.0f);
						}
						m_reflectionImage->set_pixels_rgba_float(0, y, rWidth, 1, &(lineCols2[0]));
					}
				}
			}
		}

		if (m_roughness < 1.0f) {
			if (!m_roughnessImage) {		// Roughnessテクスチャがない場合は作成.
				m_roughnessImage = m_pScene->create_image_interface(sx::vec<int,2>(oWidth, oHeight));
				std::vector<sxsdk::rgba_class> lineCols, lineCols2;
				lineCols.resize(oWidth);
				lineCols2.resize(oWidth);

				sxsdk::rgba_class col;
				const sxsdk::rgba_class whiteCol(1, 1, 1, 1);
				const sxsdk::rgba_class blackCol(0, 0, 0, 1);

				for (int y = 0; y < oHeight; ++y) {
					m_opacityMaskImage->get_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));
					for (int x = 0; x < oWidth; ++x) {
						const float r1 = lineCols[x].red;
						const float r2 = 1.0f - r1;
						const float fV = r1 * m_roughness + r2;

						lineCols2[x] = sxsdk::rgba_class(fV, fV, fV, 1.0f);
					}
					m_roughnessImage->set_pixels_rgba_float(0, y, oWidth, 1, &(lineCols2[0]));
				}
				m_hasRoughnessImage = true;
				m_roughnessRepeat   = m_opacityMaskRepeat;
				m_roughnessTexCoord = m_opacityMaskTexCoord;
			} else {
				const int rWidth  = m_roughnessImage->get_size().x;
				const int rHeight = m_roughnessImage->get_size().y;
				if (oWidth != rWidth || oHeight != rHeight) {
					compointer<sxsdk::image_interface> soImage = Shade3DUtil::resizeImageWithAlpha(m_pScene, m_opacityMaskImage, sx::vec<int,2>(rWidth, rHeight));

					std::vector<sxsdk::rgba_class> lineCols, lineCols2;
					lineCols.resize(rWidth);
					lineCols2.resize(rWidth);

					sxsdk::rgba_class col;

					for (int y = 0; y < rHeight; ++y) {
						soImage->get_pixels_rgba_float(0, y, rWidth, 1, &(lineCols[0]));
						m_roughnessImage->get_pixels_rgba_float(0, y, rWidth, 1, &(lineCols2[0]));
						for (int x = 0; x < rWidth; ++x) {
							const float r1 = lineCols[x].red;
							const float r2 = 1.0f - r1;

							const float fV = r1 * (lineCols2[x].red * m_roughness) + r2;
							lineCols2[x] = sxsdk::rgba_class(fV, fV, fV, 1.0f);
						}
						m_roughnessImage->set_pixels_rgba_float(0, y, rWidth, 1, &(lineCols2[0]));
					}
				}
			}
			m_roughness = 1.0f;
		}
	}

	// Diffuseテクスチャが存在せず、Opacityイメージが存在する場合.
	if (!m_diffuseImage && dstOpacityImage) {
		const int oWidth  = dstOpacityImage->get_size().x;
		const int oHeight = dstOpacityImage->get_size().y;
		m_diffuseImage = m_pScene->create_image_interface(sx::vec<int,2>(oWidth, oHeight));

		std::vector<sxsdk::rgba_class> lineCols;
		lineCols.resize(oWidth);

		for (int y = 0; y < oHeight; ++y) {
			dstOpacityImage->get_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));

			for (int x = 0; x < oWidth; ++x) {
				lineCols[x] = sxsdk::rgba_class(1.0f, 1.0f, 1.0f, lineCols[x].red);
			}
			m_diffuseImage->set_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));
		}

		m_hasDiffuseImage = true;
		if (m_opacityMaskImage) {
			m_diffuseRepeat   = m_opacityMaskRepeat;
			m_diffuseTexCoord = m_opacityMaskTexCoord;
		} else if (m_transparencyImage) {
			m_diffuseRepeat   = m_transparencyRepeat;
			m_diffuseTexCoord = m_transparencyTexCoord;
		}

		m_diffuseAlphaTrans = true;
	}

	// OpacityMaskにdstOpacityImageの内容を入れる.
	if (!m_exportParam.separateOpacityAndTransmission) {
		if (dstOpacityImage) {
			if (m_transparencyImage) {
				IMAGE_INTERFACE_RELEASE(m_transparencyImage);
				m_hasTransparencyImage = false;
			}
			IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);
			m_opacityMaskImage = dstOpacityImage;
			m_hasOpacityMaskImage = true;
			dstOpacityImage = NULL;
		}

		if (m_diffuseAlphaTrans) {
			IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);
			m_hasOpacityMaskImage = false;
		}
	}
	IMAGE_INTERFACE_RELEASE(dstOpacityImage);

	// 透明度による変換処理.
	m_convTransparencyData();

	// Metallic/Roughnessテクスチャのパック処理.
	m_packOcclusionRoughnessMetallicImage();
}

/**
 * 加工せずにそのままPBRマテリアルとして格納.
 */
void CImagesBlend::m_noBakeShade3DToPBRMaterial ()
{
	m_diffuseColor = (m_surface->get_diffuse_color()) * (m_surface->get_diffuse());
	if (m_diffuseImage && m_diffuseTexturesCount >= 2) {
		m_diffuseColor = sxsdk::rgb_class(1, 1, 1);
	}

	m_metallic     = m_surface->get_reflection();
	m_roughness    = m_surface->get_roughness();
	m_transparency = m_surface->get_transparency();

	m_emissiveColor = (m_surface->get_glow_color()) * (m_surface->get_glow());
	if (m_glowImage) m_emissiveColor = sxsdk::rgb_class(1, 1, 1);

	// 「不透明」と「透明」のテクスチャを分離もしくは合成して再格納.
	sxsdk::image_interface* dstOpacityImage = m_storeOpasicyTransparencyTexture();

	// DiffuseとOpacityの両方が存在する場合、DiffuseのA要素としてOpacityを格納.
	if (m_diffuseImage && dstOpacityImage) {
		if (m_diffuseTexCoord == m_opacityMaskTexCoord && m_diffuseRepeat == m_opacityMaskRepeat) {
			const int width  = m_diffuseImage->get_size().x;
			const int height = m_diffuseImage->get_size().y;
			compointer<sxsdk::image_interface> optImage = Shade3DUtil::resizeImageWithAlpha(m_pScene, dstOpacityImage, sx::vec<int,2>(width, height));
			
			std::vector<sxsdk::rgba_class> col1A;
			std::vector<sxsdk::rgba_class> col2A;
			col1A.resize(width);
			col2A.resize(width);
			for (int y = 0; y < height; ++y) {
				m_diffuseImage->get_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
				optImage->get_pixels_rgba_float(0, y, width, 1, &(col2A[0]));
				for (int x = 0; x < width; ++x) {
					col1A[x].alpha = col2A[x].red;
				}
				m_diffuseImage->set_pixels_rgba_float(0, y, width, 1, &(col1A[0]));
			}
			m_diffuseAlphaTrans = true;
		}
	}

	// Diffuseテクスチャが存在せず、Opacityイメージが存在する場合.
	if (!m_diffuseImage && dstOpacityImage) {
		const int oWidth  = dstOpacityImage->get_size().x;
		const int oHeight = dstOpacityImage->get_size().y;
		m_diffuseImage = m_pScene->create_image_interface(sx::vec<int,2>(oWidth, oHeight));

		std::vector<sxsdk::rgba_class> lineCols;
		lineCols.resize(oWidth);

		for (int y = 0; y < oHeight; ++y) {
			dstOpacityImage->get_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));

			for (int x = 0; x < oWidth; ++x) {
				lineCols[x] = sxsdk::rgba_class(1.0f, 1.0f, 1.0f, lineCols[x].red);
			}
			m_diffuseImage->set_pixels_rgba_float(0, y, oWidth, 1, &(lineCols[0]));
		}

		m_hasDiffuseImage = true;
		if (m_opacityMaskImage) {
			m_diffuseRepeat   = m_opacityMaskRepeat;
			m_diffuseTexCoord = m_opacityMaskTexCoord;
		} else if (m_transparencyImage) {
			m_diffuseRepeat   = m_transparencyRepeat;
			m_diffuseTexCoord = m_transparencyTexCoord;
		}

		m_diffuseAlphaTrans = true;
	}

	// OpacityMaskにdstOpacityImageの内容を入れる.
	if (!m_exportParam.separateOpacityAndTransmission) {
		if (dstOpacityImage) {
			if (m_transparencyImage) {
				IMAGE_INTERFACE_RELEASE(m_transparencyImage);
				m_hasTransparencyImage = false;
			}
			IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);
			m_opacityMaskImage = dstOpacityImage;
			m_hasOpacityMaskImage = true;
			dstOpacityImage = NULL;
		}

		if (m_diffuseAlphaTrans) {
			IMAGE_INTERFACE_RELEASE(m_opacityMaskImage);
			m_hasOpacityMaskImage = false;
		}
	}
	IMAGE_INTERFACE_RELEASE(dstOpacityImage);

	// 透明度による変換処理.
	m_convTransparencyData();

	// Metallic/Roughnessテクスチャのパック処理.
	m_packOcclusionRoughnessMetallicImage();
}

/**
 * glTFのMetallic-Roughnessのパック処理  (Occlusion(R)/Roughness(G)/Metallic(B)).
 * Occlusionは分離できる.
 * TexCoordがMetallic-RoughnessとOcclusionで異なる場合、.
 * テクスチャサイズがMetallic-RoughnessとOcclusionで異なる場合、は、Occlusionテクスチャは独立して使用する.
 */
void CImagesBlend::m_packOcclusionRoughnessMetallicImage ()
{
	if (m_reflectionImage == NULL && m_roughnessImage == NULL) return;

	IMAGE_INTERFACE_RELEASE(m_glTFMetallicRoughnessImage);

	m_useOcclusionInMetallicRoughnessTexture = false;

	// Metallic/Roughnessテクスチャのいずれかがある場合、サイズの大きい方にあわせる.
	int mrTexWidth  = 0;
	int mrTexHeight = 0;
	int mrTexCoord = -1;
	sx::vec<int,2> mrRepeat(0, 0);
	if (m_reflectionImage) {
		mrTexWidth  = std::max(mrTexWidth, m_reflectionImage->get_size().x);
		mrTexHeight = std::max(mrTexHeight, m_reflectionImage->get_size().y);
		mrTexCoord = m_reflectionTexCoord;
		mrRepeat   = m_reflectionRepeat;
	}
	if (m_roughnessImage) {
		mrTexWidth  = std::max(mrTexWidth, m_roughnessImage->get_size().x);
		mrTexHeight = std::max(mrTexHeight, m_roughnessImage->get_size().y);
		mrTexCoord = m_roughnessTexCoord;
		mrRepeat   = m_roughnessRepeat;
	}
	if (mrTexWidth == 0 || mrTexHeight == 0) return;

	try {
		m_glTFMetallicRoughnessImage = m_pScene->create_image_interface(sx::vec<int,2>(mrTexWidth, mrTexHeight));
		if (!m_glTFMetallicRoughnessImage) return;

		std::vector<sxsdk::rgba_class> lineCols;
		lineCols.resize(mrTexWidth);
		for (int x = 0; x < mrTexWidth; ++x) lineCols[x] = sxsdk::rgba_class(1, 1, 1, 1);
		for (int y = 0; y < mrTexHeight; ++y) {
			m_glTFMetallicRoughnessImage->set_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
		}
	} catch (...) { }

	// Metallicを[B]に格納.
	if (m_reflectionImage) {
		const int width  = m_reflectionImage->get_size().x;
		const int height = m_reflectionImage->get_size().y;

		try {
			sxsdk::image_interface* image = NULL;
			if (width == mrTexWidth && height == mrTexHeight) image = m_reflectionImage;
			else {
				sx::vec<int,2> tSize(mrTexWidth, mrTexHeight);
				image = m_reflectionImage->duplicate_image(&tSize);
			}
			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(mrTexWidth);
			lineCols2.resize(mrTexWidth);

			for (int y = 0; y < mrTexHeight; ++y) {
				m_glTFMetallicRoughnessImage->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
				image->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols2[0]));
				for (int x = 0; x < mrTexWidth; ++x) lineCols[x].blue = lineCols2[x].red;
				m_glTFMetallicRoughnessImage->set_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
			}
			if (image != m_reflectionImage) IMAGE_INTERFACE_RELEASE(image);

		} catch (...) { }
	}

	// Roughnessを[G]に格納.
	if (m_roughnessImage) {
		const int width  = m_roughnessImage->get_size().x;
		const int height = m_roughnessImage->get_size().y;

		try {
			sxsdk::image_interface* image = NULL;
			if (width == mrTexWidth && height == mrTexHeight) image = m_roughnessImage;
			else {
				sx::vec<int,2> tSize(mrTexWidth, mrTexHeight);
				image = m_roughnessImage->duplicate_image(&tSize);
			}
			std::vector<sxsdk::rgba_class> lineCols, lineCols2;
			lineCols.resize(mrTexWidth);
			lineCols2.resize(mrTexWidth);

			for (int y = 0; y < mrTexHeight; ++y) {
				m_glTFMetallicRoughnessImage->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
				image->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols2[0]));
				for (int x = 0; x < mrTexWidth; ++x) lineCols[x].green = lineCols2[x].red;
				m_glTFMetallicRoughnessImage->set_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
			}
			if (image != m_roughnessImage) IMAGE_INTERFACE_RELEASE(image);

		} catch (...) { }
	}

	// OcclusionのTexCoordとサイズがMetallic/Roughnessと同じ場合、[R]にOcclusionを格納.
	if (m_occlusionImage) {
		const int width  = m_occlusionImage->get_size().x;
		const int height = m_occlusionImage->get_size().y;
		if (width == mrTexWidth && height == mrTexHeight && mrTexCoord == m_occlusionTexCoord && mrRepeat == m_occlusionRepeat) {
			try {
				sxsdk::image_interface* image = m_occlusionImage;
				std::vector<sxsdk::rgba_class> lineCols, lineCols2;
				lineCols.resize(mrTexWidth);
				lineCols2.resize(mrTexWidth);

				for (int y = 0; y < mrTexHeight; ++y) {
					m_glTFMetallicRoughnessImage->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
					image->get_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols2[0]));
					for (int x = 0; x < mrTexWidth; ++x) lineCols[x].red = lineCols2[x].red;
					m_glTFMetallicRoughnessImage->set_pixels_rgba_float(0, y, mrTexWidth, 1, &(lineCols[0]));
				}

			} catch (...) { }
			m_useOcclusionInMetallicRoughnessTexture = true;

			IMAGE_INTERFACE_RELEASE(m_occlusionImage);
			m_hasOcclusionImage = false;
		}
	}
}

/**
 * 各種イメージを持つか (単一または複数).
 */
bool CImagesBlend::hasImage (const sxsdk::enums::mapping_type mappingType) const
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_hasDiffuseImage;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_hasReflectionImage;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_hasRoughnessImage;
	if (mappingType == sxsdk::enums::normal_mapping) return m_hasNormalImage;
	if (mappingType == sxsdk::enums::glow_mapping) return m_hasGlowImage;
	if (mappingType == sxsdk::enums::transparency_mapping) return m_hasTransparencyImage;
	if (mappingType == MAPPING_TYPE_OPACITY) return m_hasOpacityMaskImage;
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) return m_hasOcclusionImage;
	return false;
}

/**
 * イメージを取得.
 */
sxsdk::image_interface* CImagesBlend::getImage (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseImage;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionImage;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessImage;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalImage;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowImage;
	if (mappingType == sxsdk::enums::transparency_mapping) return m_transparencyImage;
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityMaskImage;
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) return m_occlusionImage;
	return NULL;
}

/**
 * イメージ名を取得.
 */
std::string CImagesBlend::getImageName (const sxsdk::enums::mapping_type mappingType)
{
	std::string name = "";
	if (mappingType == sxsdk::enums::diffuse_mapping) name = m_diffuseImageName;
	if (mappingType == sxsdk::enums::reflection_mapping) name = m_reflectionImageName;
	if (mappingType == sxsdk::enums::roughness_mapping) name = m_roughnessImageName;
	if (mappingType == sxsdk::enums::normal_mapping) name = m_normalImageName;
	if (mappingType == sxsdk::enums::glow_mapping) name = m_glowImageName;
	if (mappingType == sxsdk::enums::transparency_mapping) name = m_transparencyImageName;
	if (mappingType == MAPPING_TYPE_OPACITY) name = m_opacityMaskImageName;
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) name = m_occlusionImageName;

	if (name == "") name = "image";
	return name;
}

/**
 * イメージのUV層番号を取得.
 */
int CImagesBlend::getTexCoord (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseTexCoord;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionTexCoord;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessTexCoord;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalTexCoord;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowTexCoord;
	if (mappingType == sxsdk::enums::transparency_mapping) return m_transparencyTexCoord;
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityMaskTexCoord;
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) return m_occlusionTexCoord;

	return 0;
}

/**
 * イメージの反復回数を取得.
 */
sx::vec<int,2> CImagesBlend::getImageRepeat (const sxsdk::enums::mapping_type mappingType)
{
	if (mappingType == sxsdk::enums::diffuse_mapping) return m_diffuseRepeat;
	if (mappingType == sxsdk::enums::reflection_mapping) return m_reflectionRepeat;
	if (mappingType == sxsdk::enums::roughness_mapping) return m_roughnessRepeat;
	if (mappingType == sxsdk::enums::normal_mapping) return m_normalRepeat;
	if (mappingType == sxsdk::enums::glow_mapping) return m_glowRepeat;
	if (mappingType == sxsdk::enums::transparency_mapping) return m_transparencyRepeat;
	if (mappingType == MAPPING_TYPE_OPACITY) return m_opacityMaskRepeat;
	if (mappingType == MAPPING_TYPE_GLTF_OCCLUSION) return m_occlusionRepeat;
	return sx::vec<int,2>(1, 1);
}

/**
 * イメージの強度を色として取得.
 */
sxsdk::rgb_class CImagesBlend::getImageFactor (const sxsdk::enums::mapping_type mappingType)
{
	sxsdk::rgb_class retCol(1, 1, 1);

	if (mappingType == sxsdk::enums::diffuse_mapping) {
		retCol = m_diffuseColor;
	}
	if (mappingType == sxsdk::enums::glow_mapping) {
		retCol = m_emissiveColor;
	}
	if (mappingType == sxsdk::enums::roughness_mapping) {
		retCol = sxsdk::rgb_class(m_roughness, m_roughness, m_roughness);
	}
	if (mappingType == sxsdk::enums::reflection_mapping) {
		retCol = sxsdk::rgb_class(m_metallic, m_metallic, m_metallic);
	}

	return retCol;
}

/**
 * glTFのMetallic-Roughnessイメージを取得.
 */
sxsdk::image_interface* CImagesBlend::getMetallicRoughnessImage ()
{
	return m_glTFMetallicRoughnessImage;
}

/**
 * マッピングが「通常」合成かチェック。マッピングが存在しない場合はfalse.
 */
bool CImagesBlend::m_checkDefaultBlendMapping (const sxsdk::enums::mapping_type mappingType)
{
	bool chkF = true;
	const int layersCou = m_surface->get_number_of_mapping_layers();
	int cou = 0;
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = m_surface->mapping_layer(i);
		if (mappingLayer.get_projection() != 3) continue;		// UV投影でない場合.
		if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;

		const float weight = mappingLayer.get_weight();
		if (MathUtil::isZero(weight)) continue;

		const int type = mappingLayer.get_type();
		if (type != mappingType) continue;

		compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
		if (!image || !(image->has_image()) || (image->get_size().x) <= 0 || (image->get_size().y) <= 0) continue;

		if (cou == 0) {
			const int blendMode = mappingLayer.get_blend_mode();
			if (blendMode != 7) {		// 「乗算」合成でない場合.
				chkF = false;
				break;
			}
		}
		cou++;
	}
	if (cou >= 2) chkF = false;

	return chkF;
}
