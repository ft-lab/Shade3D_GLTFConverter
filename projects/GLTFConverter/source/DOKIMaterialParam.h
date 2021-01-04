/**
 * DOKI for Shade3D( http://www.ft-lab.ne.jp/shade3d/DOKI/ )のカスタムマテリアル情報を取得.
 * ClearCoat/Thin/Transmissionなどのパラメータを読み込み.
 */

#ifndef _DOKI_MATERIAL_PARAM_H
#define _DOKI_MATERIAL_PARAM_H

#include "GlobalHeader.h"

// DOKI for Shade3Dでのカスタムマテリアル情報のUUID.
#define DOKI_CUSTOM_MATERIAL_ATTRIBUTE_INTERFACE_ID sx::uuid_class("174148CE-DB8B-4668-B1BD-3EFC0CBA004B")

// カスタムマテリアルのstreamバージョン.
#define DOKI_CUSTOM_MATERIAL_DATA_STREAM_VERSION 0x100

namespace DOKI
{
	/**
	 * Shade3Dでのマテリアルの種類.
	 */
	enum CustomMaterialType {
		materialType_shade3D = 0,		// Shade3Dの「Shade3Dマテリアル」を使用.
		materialType_PBR,				// OSPRayの"principled"を使用.
		materialType_glass,				// OSPRayの"Glass"を使用.
		materialType_luminous,			// OSPRayの"Luminous"を使用 (発光).
	};

	class CCustomMaterialData
	{
	public:
		DOKI::CustomMaterialType type;				// マテリアルの種類.

		// Shade3Dマテリアルでの追加パラメータ.
		bool shade3D_thin;								// 薄い形状.
		float shade3D_thin_thickness;					// thin = trueの場合の厚み.
		float shade3D_backlight;						// バックライト (0.0 - 2.0).

		float shade3D_transmissionDepth;				// 透明度使用時の減衰距離.

		// Glassのパラメータ.
		bool glass_thin;								// 薄い形状.
		float glass_ior;								// 屈折率.
		sxsdk::rgb_class glass_attenuationColor;		// 減衰の色.
		float glass_attenuationDistance;				// 減衰の距離.
		float glass_thickness;

		sxsdk::rgb_class edgeColor;						// エッジカラー（メタリックのみ）.

		// クリアコートのパラメータ.
		float clearcoat_weight;							// クリアコートの強さ.
		float clearcoat_ior;							// クリアコートの屈折率.
		sxsdk::rgb_class clearcoat_color;				// クリアコートの色.
		float clearcoat_thickness;						// クリアコートの厚み.
		float clearcoat_roughness;						// クリアコートのラフネス.

		// 光沢(sheen)のパラメータ.
		float sheen_weight;								// 光沢の強さ.
		sxsdk::rgb_class sheen_color;					// 光沢の色.
		float sheen_tint;								// 光沢の色合い.
		float sheen_roughness;							// 光沢のラフネス.

		// 発光(luminous)のパラメータ.
		float luminous_intensity;						// 発光の強さ.
		sxsdk::rgb_class luminous_color;				// 発光の色.
		float luminous_transparency;					// 発光の透明度.

	public:
		CCustomMaterialData ();

		CCustomMaterialData (const CCustomMaterialData& v);
		~CCustomMaterialData ();

		CCustomMaterialData& operator = (const CCustomMaterialData &v) {
			this->type  = v.type;

			this->glass_thin                = v.glass_thin;
			this->glass_ior                 = v.glass_ior;
			this->glass_attenuationColor    = v.glass_attenuationColor;
			this->glass_attenuationDistance = v.glass_attenuationDistance;
			this->glass_thickness           = v.glass_thickness;

			this->shade3D_thin      = v.shade3D_thin;
			this->shade3D_thin_thickness = v.shade3D_thin_thickness;
			this->shade3D_backlight = v.shade3D_backlight;

			this->shade3D_transmissionDepth = v.shade3D_transmissionDepth;

			this->edgeColor = v.edgeColor;

			this->clearcoat_weight    = v.clearcoat_weight;
			this->clearcoat_ior       = v.clearcoat_ior;
			this->clearcoat_color     = v.clearcoat_color;
			this->clearcoat_thickness = v.clearcoat_thickness;
			this->clearcoat_roughness = v.clearcoat_roughness;

			this->sheen_weight    = v.sheen_weight;
			this->sheen_color     = v.sheen_color;
			this->sheen_tint      = v.sheen_tint;
			this->sheen_roughness = v.sheen_roughness;

			this->luminous_intensity    = v.luminous_intensity;
			this->luminous_color        = v.luminous_color;
			this->luminous_transparency = v.luminous_transparency;

			return (*this);
		}

		void clear ();

		/**
		 * 同一マテリアル設定か.
		 */
		bool isSame (const CCustomMaterialData& matD) const;
	};

	/**
	 * カスタムマテリアル情報を読み込み.
	 */
	bool loadCustomMaterialData (sxsdk::surface_interface* surface, CCustomMaterialData& data);
	bool loadCustomMaterialData (sxsdk::surface_class* surface, CCustomMaterialData& data);

}

#endif
