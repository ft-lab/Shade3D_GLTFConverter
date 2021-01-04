/**
 * DOKI for Shade3D( http://www.ft-lab.ne.jp/shade3d/DOKI/ )のカスタムマテリアル情報を取得.
 * ClearCoat/Thin/Transmissionなどのパラメータを読み込み.
 */
#include "DOKIMaterialParam.h"
#include "MathUtil.h"

using namespace DOKI;

CCustomMaterialData::CCustomMaterialData ()
{
	clear();
}


CCustomMaterialData::CCustomMaterialData (const CCustomMaterialData& v)
{
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
}

CCustomMaterialData::~CCustomMaterialData ()
{
}

void CCustomMaterialData::clear ()
{
	type = CustomMaterialType::materialType_shade3D;

	glass_thin = false;
	glass_ior  = 1.5f;
	glass_attenuationColor    = sxsdk::rgb_class(1, 1, 1);
	glass_attenuationDistance = 1000.0f;
	glass_thickness           = 10.0f;

	shade3D_thin      = false;
	shade3D_thin_thickness = 10.0f;
	shade3D_backlight = 0.0f;

	shade3D_transmissionDepth = 100.0f;

	edgeColor = sxsdk::rgb_class(1, 1, 1);
	clearcoat_weight    = 0.0f;
	clearcoat_ior       = 1.5f;
	clearcoat_color     = sxsdk::rgb_class(1, 1, 1);
	clearcoat_thickness = 10.0f;
	clearcoat_roughness = 0.0f;

	sheen_weight    = 0.0f;
	sheen_color     = sxsdk::rgb_class(1, 1, 1);
	sheen_tint      = 0.0f;
	sheen_roughness = 0.2f;

	luminous_intensity    = 0.0f;
	luminous_color        = sxsdk::rgb_class(1, 1, 1);
	luminous_transparency = 1.0f;
}

/**
 * 同一マテリアル設定か.
 */
bool CCustomMaterialData::isSame (const CCustomMaterialData& matD) const
{
	if (type != matD.type) return false;
	if (glass_thin != matD.glass_thin) return false;
	if (!MathUtil::isZero(glass_ior - matD.glass_ior)) return false;
	if (!MathUtil::isZero(glass_attenuationColor - matD.glass_attenuationColor)) return false;
	if (!MathUtil::isZero(glass_attenuationDistance - matD.glass_attenuationDistance)) return false;
	if (!MathUtil::isZero(glass_thickness - matD.glass_thickness)) return false;
	if (shade3D_thin != matD.shade3D_thin) return false;
	if (!MathUtil::isZero(shade3D_thin_thickness - matD.shade3D_thin_thickness)) return false;
	if (!MathUtil::isZero(shade3D_backlight - matD.shade3D_backlight)) return false;
	if (!MathUtil::isZero(shade3D_transmissionDepth - matD.shade3D_transmissionDepth)) return false;
	if (!MathUtil::isZero(edgeColor - matD.edgeColor)) return false;

	if (!MathUtil::isZero(clearcoat_weight - matD.clearcoat_weight)) return false;
	if (!MathUtil::isZero(clearcoat_ior - matD.clearcoat_ior)) return false;
	if (!MathUtil::isZero(clearcoat_color - matD.clearcoat_color)) return false;
	if (!MathUtil::isZero(clearcoat_thickness - matD.clearcoat_thickness)) return false;
	if (!MathUtil::isZero(clearcoat_roughness - matD.clearcoat_roughness)) return false;

	if (!MathUtil::isZero(sheen_weight - matD.sheen_weight)) return false;
	if (!MathUtil::isZero(sheen_color - matD.sheen_color)) return false;
	if (!MathUtil::isZero(sheen_tint - matD.sheen_tint)) return false;
	if (!MathUtil::isZero(sheen_roughness - matD.sheen_roughness)) return false;

	if (!MathUtil::isZero(luminous_intensity - matD.luminous_intensity)) return false;
	if (!MathUtil::isZero(luminous_color - matD.luminous_color)) return false;
	if (!MathUtil::isZero(luminous_transparency - matD.luminous_transparency)) return false;

	return true;
}

/**
 * カスタムマテリアル情報を読み込み.
 */

namespace {
	/**
	 * カスタムマテリアル情報を読み込み.
	 */
	bool g_loadCustomMaterialData (sxsdk::stream_interface* stream, CCustomMaterialData& data) {
		data.clear();

		try {
			if (!stream) return false;

			stream->set_pointer(0);

			int iDat;
			int iVersion;
			stream->read_int(iVersion);

			iDat = (int)data.type;
			stream->read_int(iDat);
			data.type = (DOKI::CustomMaterialType)iDat;

			stream->read_int(iDat);
			data.shade3D_thin = iDat ? true : false;
			stream->read_float(data.shade3D_backlight);

			stream->read_int(iDat);
			data.glass_thin = iDat ? true : false;

			stream->read_float(data.glass_ior);
			stream->read_float(data.glass_attenuationColor.red);
			stream->read_float(data.glass_attenuationColor.green);
			stream->read_float(data.glass_attenuationColor.blue);
			stream->read_float(data.glass_attenuationDistance);
			stream->read_float(data.glass_thickness);

			stream->read_float(data.shade3D_transmissionDepth);

			try {
				stream->read_float(data.edgeColor.red);
				stream->read_float(data.edgeColor.green);
				stream->read_float(data.edgeColor.blue);
			} catch (...) { }

			try {
				stream->read_float(data.clearcoat_weight);
				stream->read_float(data.clearcoat_ior);
				stream->read_float(data.clearcoat_color.red);
				stream->read_float(data.clearcoat_color.green);
				stream->read_float(data.clearcoat_color.blue);
				stream->read_float(data.clearcoat_thickness);
				stream->read_float(data.clearcoat_roughness);
			} catch (...) { }

			try {
				stream->read_float(data.sheen_weight);
				stream->read_float(data.sheen_color.red);
				stream->read_float(data.sheen_color.green);
				stream->read_float(data.sheen_color.blue);
				stream->read_float(data.sheen_tint);
				stream->read_float(data.sheen_roughness);
			} catch (...) { }

			try {
				stream->read_float(data.shade3D_thin_thickness);
			} catch (...) { }

			try {
				stream->read_float(data.luminous_intensity);
				stream->read_float(data.luminous_color.red);
				stream->read_float(data.luminous_color.green);
				stream->read_float(data.luminous_color.blue);
				stream->read_float(data.luminous_transparency);
			} catch (...) { }

			return true;
		} catch (...) { }

		return false;
	}
};

bool DOKI::loadCustomMaterialData (sxsdk::surface_interface* surface, DOKI::CCustomMaterialData& data)
{
	data.clear();

	try {
		compointer<sxsdk::stream_interface> stream(surface->get_attribute_stream_interface_with_uuid(DOKI_CUSTOM_MATERIAL_ATTRIBUTE_INTERFACE_ID));
		if (!stream) return false;
		return ::g_loadCustomMaterialData(stream, data);
	} catch (...) { }

	return false;

}

bool DOKI::loadCustomMaterialData (sxsdk::surface_class* surface, DOKI::CCustomMaterialData& data)
{
	data.clear();

	try {
		compointer<sxsdk::stream_interface> stream(surface->get_attribute_stream_interface_with_uuid(DOKI_CUSTOM_MATERIAL_ATTRIBUTE_INTERFACE_ID));
		if (!stream) return false;
		return ::g_loadCustomMaterialData(stream, data);
	} catch (...) { }

	return false;
}

