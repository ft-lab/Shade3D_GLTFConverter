/**
 *  @file   main.cpp
 *  @brief  glTFインポータ・エクスポータ.
 */

#include "GlobalHeader.h"
#include "GLTFImporterInterface.h"
#include "GLTFExporterInterface.h"
#include "OcclusionShaderInterface.h"
#include "LicenseDialogInterface.h"

//**************************************************//
//	グローバル関数									//
//**************************************************//
/**
 * プラグインインターフェースの生成.
 */
extern "C" SXSDKEXPORT void STDCALL create_interface (const IID &iid, int i, void **p, sxsdk::shade_interface *shade, void *) {
	unknown_interface *u = NULL;
	
	if (iid == attribute_iid) {
		if (i == 0) {
			u = new CLicenseDialogInterface(*shade);
		}
	}
	if (iid == importer_iid) {
		if (i == 0) {
			u = new CGLTFImporterInterface(*shade);
		}
	}
	if (iid == exporter_iid) {
		if (i == 0) {
			u = new CGLTFExporterInterface(*shade);
		}
	}
	if (iid == shader_iid) {
		if (i == 0) {
			u = new COcclusionTextureShaderInterface(*shade);
		}
	}

	if (u) {
		u->AddRef();
		*p = (void *)u;
	}
}

/**
 * インターフェースの数を返す.
 */
extern "C" SXSDKEXPORT int STDCALL has_interface (const IID &iid, sxsdk::shade_interface *shade) {

	if (iid == attribute_iid) return 1;
	if (iid == importer_iid) return 1;
	if (iid == exporter_iid) return 1;
	if (iid == shader_iid) return 1;

	return 0;
}

/**
 * インターフェース名を返す.
 */
extern "C" SXSDKEXPORT const char * STDCALL get_name (const IID &iid, int i, sxsdk::shade_interface *shade, void *) {
	if (iid == attribute_iid) {
		if (i == 0) {
			return CLicenseDialogInterface::name(shade);
		}
	}
	if (iid == importer_iid) {
		if (i == 0) {
			return CGLTFImporterInterface::name(shade);
		}
	}
	if (iid == exporter_iid) {
		if (i == 0) {
			return CGLTFExporterInterface::name(shade);
		}
	}
	if (iid == shader_iid) {
		if (i == 0) {
			return COcclusionTextureShaderInterface::name(shade);
		}
	}

	return 0;
}

/**
 * プラグインのUUIDを返す.
 */
extern "C" SXSDKEXPORT sx::uuid_class STDCALL get_uuid (const IID &iid, int i, void *) {
	if (iid == attribute_iid) {
		if (i == 0) {
			return LICENSE_DIALOG_INTERFACE_ID;
		}
	}
	if (iid == importer_iid) {
		if (i == 0) {
			return GLTF_IMPORTER_INTERFACE_ID;
		}
	}
	if (iid == exporter_iid) {
		if (i == 0) {
			return GLTF_EXPORTER_INTERFACE_ID;
		}
	}
	if (iid == shader_iid) {
		if (i == 0) {
			return OCCLUSION_SHADER_INTERFACE_ID;
		}
	}

	return sx::uuid_class(0, 0, 0, 0);
}

/**
 * バージョン情報.
 */
extern "C" SXSDKEXPORT void STDCALL get_info (sxsdk::shade_plugin_info &info, sxsdk::shade_interface *shade, void *) {
	info.sdk_version = SHADE_BUILD_NUMBER;
	info.recommended_shade_version = 491000;
	info.major_version = 0;
	info.minor_version = 1;
	info.micro_version = 0;
	info.build_number  = 15;
}

/**
 * 常駐プラグイン.
 */
extern "C" SXSDKEXPORT bool STDCALL is_resident (const IID &iid, int i, void *) {
	return true;
}

