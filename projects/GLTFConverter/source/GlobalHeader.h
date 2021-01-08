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
#define ALPHA_MODE_INTERFACE_ID sx::uuid_class("F92DECA6-AC2C-4B6C-8C89-A3D2AD0F6A43")
#define LICENSE_DIALOG_INTERFACE_ID sx::uuid_class("DC1B3583-05DE-4AA7-BE76-1B0B1FC599AD")

// streamに保存するstreamのバージョン.
#define GLTF_IMPORTER_DLG_STREAM_VERSION		0x101
#define GLTF_IMPORTER_DLG_STREAM_VERSION_101	0x101
#define GLTF_IMPORTER_DLG_STREAM_VERSION_100	0x100

#define GLTF_EXPORTER_DLG_STREAM_VERSION		0x105
#define GLTF_EXPORTER_DLG_STREAM_VERSION_105	0x105
#define GLTF_EXPORTER_DLG_STREAM_VERSION_104	0x104
#define GLTF_EXPORTER_DLG_STREAM_VERSION_103	0x103
#define GLTF_EXPORTER_DLG_STREAM_VERSION_102	0x102
#define GLTF_EXPORTER_DLG_STREAM_VERSION_101	0x101
#define GLTF_EXPORTER_DLG_STREAM_VERSION_100	0x100

#define OCCLUSION_PARAM_DLG_STREAM_VERSION		0x101
#define OCCLUSION_PARAM_DLG_STREAM_VERSION_101	0x101
#define OCCLUSION_PARAM_DLG_STREAM_VERSION_100	0x100

#define ALPHA_MODE_DLG_STREAM_VERSION			0x100
#define GLTF_LICENSE_STREAM_VERSION				0x100

// 作業ディレクトリ名.
#define GLTF_TEMP_DIR "shade3d_temp_gltf"

#define MAPPING_TYPE_OPACITY  ((sxsdk::enums::mapping_type)24)				// sxsdk::enums::mapping_typeでの「不透明マスク」.
#define MAPPING_TYPE_GLTF_OCCLUSION  ((sxsdk::enums::mapping_type)1001)		// 「オクルージョン」これはglTFで割り当てたカスタムの種類.

namespace GLTFConverter {
	/**
	 * 出力するテクスチャのタイプ.
	 */
	enum export_texture_type {
		export_texture_name = 0,		// マスターサーフェス名の拡張子指定を参照.
		export_texture_png,				// pngの置き換え.
		export_texture_jpeg,			// jpegの置き換え.
	};

	/**
	 * 出力する最大テクスチャサイズ.
	 */
	enum export_max_texture_size {
		export_max_texture_size_undefined = 0,	// 指定なし.
		export_max_texture_size_256,			// 256.
		export_max_texture_size_512,			// 512.
		export_max_texture_size_1024,			// 1024.
		export_max_texture_size_2048,			// 2048.
		export_max_texture_size_4096,			// 4096.
	};

	/**
	 * AlphaModeの種類.
	 */
	enum alpha_mode_type {
		alpha_mode_opaque = 0,					// OPAQUE.
		alpha_mode_mask,						// MASK.
		alpha_mode_blend,						// BLEND.
	};

	/**
	 * エンジンの種類.
	 */
	enum engine_type {
		engine_unity = 0,					// Unity(Standard Shader/URP).
		engine_unity_hdrp,					// Unity (HDRP).
		engine_unigine,						// Unigine.
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
	GLTFConverter::export_max_texture_size maxTextureSize;	// 最大テクスチャサイズ.

	int exportFileFormat;									// 出力ファイル形式 (0:glb / 1:gltf)

	bool outputBonesAndSkins;								// ボーンとスキン情報を出力.
	bool outputVertexColor;									// 頂点カラーを出力.
	bool outputAnimation;									// アニメーションを出力.
	bool dracoCompression;									// Draco圧縮.
	bool shareVerticesMesh;									// Mesh内のPrimitiveの頂点情報を共有.
	bool convertColorToLinear;								// 色をリニアに変換.

	bool bakeWithoutProcessingTextures;						// テクスチャを加工せずにベイク.
	bool separateOpacityAndTransmission;					// 「不透明(Opacity)」と「透明(Transmission)」を分ける.

	std::string assetExtrasTitle;							// タイトル.
	std::string assetExtrasAuthor;							// 作成者.
	std::string assetExtrasLicense;							// ライセンス.
	std::string assetExtrasSource;							// オリジナルモデルの参照先.

	bool outputAdditionalTextures;							// エンジン別にテクスチャを別途出力.
	GLTFConverter::engine_type engineType;					// エンジンの種類.

public:
	CExportDlgParam () {
		clear();
	}

	void clone (const CExportDlgParam& v) {
		this->outputTexture         = v.outputTexture;
		this->maxTextureSize        = v.maxTextureSize;
		this->exportFileFormat      = v.exportFileFormat;
		this->outputBonesAndSkins   = v.outputBonesAndSkins;
		this->outputVertexColor     = v.outputVertexColor;
		this->outputAnimation       = v.outputAnimation;
		this->dracoCompression      = v.dracoCompression;
		this->shareVerticesMesh     = v.shareVerticesMesh;
		this->convertColorToLinear  = v.convertColorToLinear;
		this->bakeWithoutProcessingTextures  = v.bakeWithoutProcessingTextures;
		this->separateOpacityAndTransmission = v.separateOpacityAndTransmission;
		this->assetExtrasTitle   = v.assetExtrasTitle;
		this->assetExtrasAuthor  = v.assetExtrasAuthor;
		this->assetExtrasLicense = v.assetExtrasLicense;
		this->assetExtrasSource  = v.assetExtrasSource;
		this->outputAdditionalTextures = v.outputAdditionalTextures;
		this->engineType               = v.engineType;
	}

	void clear () {
		exportFileFormat = 0;
		outputTexture       = GLTFConverter::export_texture_name;
		maxTextureSize      = GLTFConverter::export_max_texture_size_undefined;

		outputBonesAndSkins = true;
		outputVertexColor   = true;
		outputAnimation     = true;
		dracoCompression    = false;
		shareVerticesMesh   = true;
		convertColorToLinear = true;

		bakeWithoutProcessingTextures = false;
		separateOpacityAndTransmission = false;

		assetExtrasTitle   = "";
		assetExtrasAuthor  = "";
		assetExtrasLicense = "All rights reserved";
		assetExtrasSource  = "";

		outputAdditionalTextures = false;
		engineType = GLTFConverter::engine_unity;
	}

	/**
	 * 最大テクスチャサイズを取得.
	 */
	int getMaxTextureSize () const {
		if (maxTextureSize == GLTFConverter::export_max_texture_size_256) return 256;
		if (maxTextureSize == GLTFConverter::export_max_texture_size_512) return 512;
		if (maxTextureSize == GLTFConverter::export_max_texture_size_1024) return 1024;
		if (maxTextureSize == GLTFConverter::export_max_texture_size_2048) return 2048;
		if (maxTextureSize == GLTFConverter::export_max_texture_size_4096) return 4096;
		return 99999;
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
	bool convertColorFromLinear;	// 色をリニアから変換.

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
		convertColorFromLinear = true;
	}
};

/**
 * OcclusionのShader情報.
 */
class COcclusionShaderData
{
public:
	int uvIndex;			// UV層番号 (0 or 1).
	int channelMix;			// チャンネル合成（0:Red、1:Green、2:Blue、3:Alpha）.

public:
	COcclusionShaderData () {
		clear();
	}

	void clear () {
		uvIndex = 0;
		channelMix = 0;
	}
};

/**
 * AlphaModeの情報.
 * これは、表面材質の属性として指定.
 */
class CAlphaModeMaterialData
{
public:
	GLTFConverter::alpha_mode_type alphaModeType;		// AlphaModeの種類.
	float alphaCutoff;

	CAlphaModeMaterialData () {
		clear();
	}

	void clear () {
		alphaModeType = GLTFConverter::alpha_mode_opaque;
		alphaCutoff   = 0.5f;
	}
};

#endif
