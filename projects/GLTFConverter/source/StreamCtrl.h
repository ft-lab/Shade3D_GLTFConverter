﻿/**
 * streamへの入出力.
 */

#ifndef _STREAMCTRL_H
#define _STREAMCTRL_H

#include "GlobalHeader.h"

namespace StreamCtrl
{
	/**
	 * Importダイアログボックスの情報を保存.
	 */
	void saveImportDialogParam (sxsdk::shade_interface& shade, const CImportDlgParam& data);

	/**
	 * Importダイアログボックスの情報を読み込み.
	 */
	void loadImportDialogParam (sxsdk::shade_interface& shade, CImportDlgParam& data);

	/**
	 * Exportダイアログボックスの情報を保存.
	 */
	void saveExportDialogParam (sxsdk::shade_interface& shade, const CExportDlgParam& data);

	/**
	 * Exportダイアログボックスの情報を読み込み.
	 */
	void loadExportDialogParam (sxsdk::shade_interface& shade, CExportDlgParam& data);

	/**
	 * 形状ごとにライセンス情報を保存.
	 */
	void saveLicenseData (sxsdk::shape_class& shape, const CLicenseData& data);

	/**
	 * 形状ごとのライセンス情報を読み込み.
	 */
	void loadLicenseData (sxsdk::shape_class& shape, CLicenseData& data);

	/**
	 * Occlusion Shader(マッピングレイヤ)情報を保存.
	 */
	void saveOcclusionParam (sxsdk::stream_interface* stream, const COcclusionShaderData& data);
	void saveOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, const COcclusionShaderData& data);

	/**
	 * Occlusion Shader(マッピングレイヤ)情報を取得.
	 */
	bool loadOcclusionParam (sxsdk::stream_interface* stream, COcclusionShaderData& data);
	bool loadOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, COcclusionShaderData& data);

	/**
	 * AlphaModeのマテリアル情報を保存.
	 */
	void saveAlphaModeMaterialParam (sxsdk::stream_interface* stream, const CAlphaModeMaterialData& data);
	void saveAlphaModeMaterialParam (sxsdk::surface_interface* surface, const CAlphaModeMaterialData& data);
	void saveAlphaModeMaterialParam (sxsdk::surface_class* surface, const CAlphaModeMaterialData& data);

	/**
	 * AlphaModeのマテリアル情報を取得.
	 */
	bool loadAlphaModeMaterialParam (sxsdk::stream_interface* stream, CAlphaModeMaterialData& data);
	bool loadAlphaModeMaterialParam (sxsdk::surface_interface* surface, CAlphaModeMaterialData& data);
	bool loadAlphaModeMaterialParam (sxsdk::surface_class* surface, CAlphaModeMaterialData& data);

}

#endif
