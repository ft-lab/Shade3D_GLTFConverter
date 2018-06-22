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
	} catch (...) { }
}

/**
 * Exportダイアログボックスの情報を読み込み.
 */
void StreamCtrl::loadExportDialogParam (sxsdk::shade_interface& shade, CExportDlgParam& data)
{
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
	} catch (...) { }
}

