/**
 *  @file   GlobalHeader.h
 *  @brief  共通して使用する変数など.
 */

#ifndef _GLOBALHEADER_H
#define _GLOBALHEADER_H

#include "sxsdk.cxx"

/**
 * プラグインID.
 */
#define GLTF_IMPORTER_INTERFACE_ID sx::uuid_class("29C48EA8-1851-4703-AF06-9DEB5A17FF49")
#define GLTF_EXPORTER_INTERFACE_ID sx::uuid_class("40385ADE-D20F-4694-A817-27CE6B8A1016")
#define OCCLUSION_SHADER_INTERFACE_ID sx::uuid_class("509D92F5-D9F9-4335-B070-0FBDEE179523")
#define LICENSE_DIALOG_INTERFACE_ID sx::uuid_class("DC1B3583-05DE-4AA7-BE76-1B0B1FC599AD")

// streamに保存するstreamのバージョン.
#define GLTF_IMPORTER_DLG_STREAM_VERSION		0x100
#define GLTF_EXPORTER_DLG_STREAM_VERSION		0x100
#define OCCLUSION_PARAM_DLG_STREAM_VERSION		0x100
#define GLTF_LICENSE_STREAM_VERSION				0x100

// 作業ディレクトリ名.
#define GLTF_TEMP_DIR "shade3d_temp_gltf"

namespace GLTFConverter {
	/**
	 * 出力するテクスチャのタイプ.
	 */
	enum export_texture_type {
		export_texture_name = 0,		// マスターサーフェス名の拡張子指定を参照.
		export_texture_png,				// pngの置き換え.
		export_texture_jpeg,			// jpegの置き換え.
	};
}

/**
 * ライセンス情報 (VRMで使用する).
 */
class CLicenseData
{
public:
	std::string exporterVersion;
	std::string version;
	std::string author;
	std::string contactInformation;
	std::string reference;
	std::string title;
	std::string allowedUserName;
	std::string violentUssageName;
	std::string sexualUssageName;
	std::string commercialUssageName;
	std::string otherPermissionUrl;
	std::string licenseName;
	std::string otherLicenseUrl;

public:
	CLicenseData () {
		clear();
	}

	void clear () {
		exporterVersion      = "";
		version              = "";
		author               = "";
		contactInformation   = "";
		reference            = "";
		title                = "";
		allowedUserName      = "";
		violentUssageName    = "";
		sexualUssageName     = "";
		commercialUssageName = "";
		otherPermissionUrl   = "";
		licenseName          = "";
		otherLicenseUrl      = "";
	}
};

/**
 * エクスポートダイアログボックスのパラメータ.
 */
class CExportDlgParam
{
public:
	GLTFConverter::export_texture_type outputTexture;		// エクスポート時のテクスチャの種類.
	bool outputBonesAndSkins;								// ボーンとスキン情報を出力.
	bool outputVertexColor;									// 頂点カラーを出力.
	bool outputAnimation;									// アニメーションを出力.
	bool dracoCompression;									// Draco圧縮.

	std::string assetExtrasTitle;							// タイトル.
	std::string assetExtrasAuthor;							// 作成者.
	std::string assetExtrasLicense;							// ライセンス.
	std::string assetExtrasSource;							// オリジナルモデルの参照先.

public:
	CExportDlgParam () {
		clear();
	}

	void clear () {
		outputTexture       = GLTFConverter::export_texture_name;
		outputBonesAndSkins = true;
		outputVertexColor   = true;
		outputAnimation     = true;
		dracoCompression    = false;

		assetExtrasTitle   = "";
		assetExtrasAuthor  = "";
		assetExtrasLicense = "All rights reserved";
		assetExtrasSource  = "";
	}
};

/**
 * インポートダイアログボックスのパラメータ.
 */
class CImportDlgParam
{
public:
	int gamma;						// ガンマ補正値 : 0 ... 1.0 , 1 ... 1.0/2.2
	bool meshImportNormals;			// 法線の読み込み.
	float meshAngleThreshold;		// 限界角度.
	bool meshImportVertexColor;		// 頂点カラーの読み込み.
	bool importAnimation;			// アニメーションの読み込み.

public:
	CImportDlgParam () {
		clear();
	}

	void clear () {
		gamma = 0;
		meshImportNormals     = false;
		meshAngleThreshold    = 50.0f;
		meshImportVertexColor = true;
		importAnimation       = true;
	}
};

/**
 * OcclusionのShader情報.
 */
class COcclusionShaderData
{
public:
	int uvIndex;			// UV層番号 (0 or 1).

public:
	COcclusionShaderData () {
		clear();
	}

	void clear () {
		uvIndex = 0;
	}
};

#endif
