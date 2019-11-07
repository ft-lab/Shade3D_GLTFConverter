/**
 * 表面材質でAlphaModeを指定するインターフェース派生クラス.
 */
#include "AlphaModeMaterialAttributeInterface.h"
#include "StreamCtrl.h"

enum {
	dlg_alpha_mode_id = 101,			// AlphaMode.
	dlg_alpha_cutoff_id = 102,			// AlphaCutoff.
	dlg_set_diffuse_RtoA_id = 103,		// 拡散反射テクスチャの(1.0-Red)値をAlpha値に反映.
};

CAlphaModeMaterialInterface::CAlphaModeMaterialInterface (sxsdk::shade_interface& shade) : shade(shade)
{
}

CAlphaModeMaterialInterface::~CAlphaModeMaterialInterface ()
{
}

bool CAlphaModeMaterialInterface::ask_surface (sxsdk::surface_interface *surface, void *)
{
	compointer<sxsdk::dialog_interface> dlg(shade.create_dialog_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
	dlg->set_resource_name("alpha_mode_dlg");

	dlg->set_responder(this);
	this->AddRef();

	StreamCtrl::loadAlphaModeMaterialParam(surface, m_data);
	if (dlg->ask()) {
		StreamCtrl::saveAlphaModeMaterialParam(surface, m_data);
		return true;
	}
	return false;	
}

/**
 * ダイアログの初期化.
 */
void CAlphaModeMaterialInterface::initialize_dialog (sxsdk::dialog_interface &d, void *)
{
}

/** 
 * ダイアログのイベントを受け取る.
 */
bool CAlphaModeMaterialInterface::respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == sx::iddefault) {
		m_data.clear();
		load_dialog_data(d);
		return true;
	}
	if (id == dlg_alpha_mode_id) {
		m_data.alphaModeType = (GLTFConverter::alpha_mode_type)item.get_selection();
		load_dialog_data(d);
		return true;
	}
	if (id == dlg_alpha_cutoff_id) {
		m_data.alphaCutoff = item.get_float();
		return true;
	}
	if (id == dlg_set_diffuse_RtoA_id) {
		m_data.setDiffuseRtoA = item.get_bool();
		return true;
	}

	return false;
}

/**
 * ダイアログのデータを設定する.
 */
void CAlphaModeMaterialInterface::load_dialog_data (sxsdk::dialog_interface &d, void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_alpha_mode_id));
		item->set_selection((int)m_data.alphaModeType);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_alpha_cutoff_id));
		item->set_float(m_data.alphaCutoff);
		item->set_enabled(m_data.alphaModeType == GLTFConverter::alpha_mode_mask);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_set_diffuse_RtoA_id));
		item->set_bool(m_data.setDiffuseRtoA);
	}
}
