/**
 * streamへの入出力.
 */
#include "StreamCtrl.h"

#include <string>
#include <algorithm>
#include <iostream>

/**
 * Importダイアログボックスの情報を保存.
 */
void StreamCtrl::saveImportDialogParam (sxsdk::shade_interface& shade, const CImportDlgParam& data)
{
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->create_attribute_stream_interface_with_uuid(GLTF_IMPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iDat;
		int iVersion = GLTF_IMPORTER_DLG_STREAM_VERSION;
		stream->write_int(iVersion);

		stream->write_int(data.gamma);
		{
			iDat = data.meshImportNormals ? 1 : 0;
			stream->write_int(iDat);
		}
		stream->write_float(data.meshAngleThreshold);
		{
			iDat = data.meshImportVertexColor ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.importAnimation ? 1 : 0;
			stream->write_int(iDat);
		}
	} catch (...) { }
}

/**
 * Importダイアログボックスの情報を読み込み.
 */
void StreamCtrl::loadImportDialogParam (sxsdk::shade_interface& shade, CImportDlgParam& data)
{
	data.clear();
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->get_attribute_stream_interface_with_uuid(GLTF_IMPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);

		int iDat;
		int iVersion = GLTF_IMPORTER_DLG_STREAM_VERSION;
		stream->read_int(iVersion);

		stream->read_int(data.gamma);
		{
			stream->read_int(iDat);
			data.meshImportNormals = iDat ? true : false;
		}
		stream->read_float(data.meshAngleThreshold);
		{
			stream->read_int(iDat);
			data.meshImportVertexColor = iDat ? true : false;
		}
		{
			stream->read_int(iDat);
			data.importAnimation = iDat ? true : false;
		}
	} catch (...) { }
}

/**
 * Exportダイアログボックスの情報を保存.
 */
void StreamCtrl::saveExportDialogParam (sxsdk::shade_interface& shade, const CExportDlgParam& data)
{
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->create_attribute_stream_interface_with_uuid(GLTF_EXPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iDat;
		int iVersion = GLTF_EXPORTER_DLG_STREAM_VERSION;
		char szStr[200];
		stream->write_int(iVersion);

		{
			iDat = (int)data.outputTexture;
			stream->write_int(iDat);
		}
		{
			iDat = data.outputBonesAndSkins ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.outputVertexColor ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = data.outputAnimation ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.assetExtrasTitle[0]), std::min((int)data.assetExtrasTitle.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.assetExtrasAuthor[0]), std::min((int)data.assetExtrasAuthor.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.assetExtrasLicense[0]), std::min((int)data.assetExtrasLicense.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.assetExtrasSource[0]), std::min((int)data.assetExtrasSource.length(), 190));
			stream->write(192, szStr);
		}

		// ver.0.2.0.2 - 
		{
			iDat = data.dracoCompression ? 1 : 0;
			stream->write_int(iDat);
		}
		{
			iDat = (int)data.maxTextureSize;
			stream->write_int(iDat);
		}

		// ver.0.2.0.3 - 
		{
			iDat = data.shareVerticesMesh ? 1 : 0;
			stream->write_int(iDat);
		}

		// ver.0.2.0.7 - 
		{
			stream->write_int(data.exportFileFormat);
		}

	} catch (...) { }
}

/**
 * Exportダイアログボックスの情報を読み込み.
 */
void StreamCtrl::loadExportDialogParam (sxsdk::shade_interface& shade, CExportDlgParam& data)
{
	data.clear();
	try {
		compointer<sxsdk::preference_interface> preference(shade.get_preference_interface());
		if (!preference) return;
		compointer<sxsdk::stream_interface> stream(preference->get_attribute_stream_interface_with_uuid(GLTF_EXPORTER_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);

		int iDat;
		int iVersion;
		char szStr[200];
		stream->read_int(iVersion);

		{
			stream->read_int(iDat);
			data.outputTexture = (GLTFConverter::export_texture_type)iDat;
		}
		{
			stream->read_int(iDat);
			data.outputBonesAndSkins = iDat ? true : false;
		}
		{
			stream->read_int(iDat);
			data.outputVertexColor = iDat ? true : false;
		}
		{
			stream->read_int(iDat);
			data.outputAnimation = iDat ? true : false;
		}

		{
			stream->read(192, szStr);
			data.assetExtrasTitle = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.assetExtrasAuthor = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.assetExtrasLicense = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.assetExtrasSource = std::string(szStr);
		}

		// ver.0.2.0.2 - 
		if (iVersion >= GLTF_EXPORTER_DLG_STREAM_VERSION_101) {
			{
				stream->read_int(iDat);
				data.dracoCompression = iDat ? true : false;
			}
			{
				stream->read_int(iDat);
				data.maxTextureSize = (GLTFConverter::export_max_texture_size)iDat;
			}
		}

		// ver.0.2.0.2 - 
		if (iVersion >= GLTF_EXPORTER_DLG_STREAM_VERSION_102) {
			{
				stream->read_int(iDat);
				data.shareVerticesMesh = iDat ? true : false;
			}
		}

		// ver.0.2.0.7 - 
		if (iVersion >= GLTF_EXPORTER_DLG_STREAM_VERSION_103) {
			stream->read_int(data.exportFileFormat);
		}

	} catch (...) { }
}


/**
 * 形状ごとにライセンス情報を保存.
 */
void StreamCtrl::saveLicenseData (sxsdk::shape_class& shape, const CLicenseData& data)
{
	try {
		compointer<sxsdk::stream_interface> stream(shape.create_attribute_stream_interface_with_uuid(LICENSE_DIALOG_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iDat;
		int iVersion = GLTF_LICENSE_STREAM_VERSION;
		char szStr[200];
		stream->write_int(iVersion);
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.exporterVersion[0]), std::min((int)data.exporterVersion.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.version[0]), std::min((int)data.version.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.author[0]), std::min((int)data.author.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.contactInformation[0]), std::min((int)data.contactInformation.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.reference[0]), std::min((int)data.reference.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.title[0]), std::min((int)data.title.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.allowedUserName[0]), std::min((int)data.allowedUserName.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.violentUssageName[0]), std::min((int)data.violentUssageName.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.sexualUssageName[0]), std::min((int)data.sexualUssageName.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.commercialUssageName[0]), std::min((int)data.commercialUssageName.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.otherPermissionUrl[0]), std::min((int)data.otherPermissionUrl.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.licenseName[0]), std::min((int)data.licenseName.length(), 190));
			stream->write(192, szStr);
		}
		{
			memset(szStr, 0, 192);
			memcpy(szStr, &(data.otherLicenseUrl[0]), std::min((int)data.otherLicenseUrl.length(), 190));
			stream->write(192, szStr);
		}
	} catch (...) { }

}

/**
 * 形状ごとのライセンス情報を読み込み.
 */
void StreamCtrl::loadLicenseData (sxsdk::shape_class& shape, CLicenseData& data)
{
	data.clear();
	try {
		compointer<sxsdk::stream_interface> stream(shape.get_attribute_stream_interface_with_uuid(LICENSE_DIALOG_INTERFACE_ID));
		if (!stream) return;
		stream->set_pointer(0);

		int iDat;
		int iVersion;
		char szStr[200];
		stream->read_int(iVersion);

		{
			stream->read(192, szStr);
			data.exporterVersion = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.version = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.author = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.contactInformation = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.reference = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.title = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.allowedUserName = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.violentUssageName = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.sexualUssageName = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.commercialUssageName = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.otherPermissionUrl = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.licenseName = std::string(szStr);
		}
		{
			stream->read(192, szStr);
			data.otherLicenseUrl = std::string(szStr);
		}
	} catch (...) { }
}

/**
 * Occlusion Shader(マッピングレイヤ)情報を保存.
 */
void StreamCtrl::saveOcclusionParam (sxsdk::stream_interface* stream, const COcclusionShaderData& data)
{
	try {
		if (!stream) return;
		stream->set_pointer(0);
		stream->set_size(0);

		int iVersion = OCCLUSION_PARAM_DLG_STREAM_VERSION;
		stream->write_int(iVersion);

		stream->write_int(data.uvIndex);

	} catch (...) { }
}

void StreamCtrl::saveOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, const COcclusionShaderData& data)
{
	try {
		compointer<sxsdk::stream_interface> stream(mappingLayer.create_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
		if (!stream) return;
		saveOcclusionParam(stream, data);
	} catch (...) { }
}

/**
 * Occlusion Shader(マッピングレイヤ)情報を取得.
 */
bool StreamCtrl::loadOcclusionParam (sxsdk::stream_interface* stream, COcclusionShaderData& data)
{
	data.clear();
	try {
		if (!stream) return false;
		stream->set_pointer(0);

		int iVersion;
		stream->read_int(iVersion);

		stream->read_int(data.uvIndex);

		return true;
	} catch (...) { }
	return false;
}

bool StreamCtrl::loadOcclusionParam (sxsdk::mapping_layer_class& mappingLayer, COcclusionShaderData& data)
{
	data.clear();
	try {
		compointer<sxsdk::stream_interface> stream(mappingLayer.get_attribute_stream_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
		if (!stream) return false;
		return loadOcclusionParam(stream, data);
	} catch (...) { }
	return false;
}
