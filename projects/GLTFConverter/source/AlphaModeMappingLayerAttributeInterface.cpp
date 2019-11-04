/**
 * マッピングレイヤでAlphaModeを指定するインターフェース派生クラス.
 */
#include "AlphaModeMappingLayerAttributeInterface.h"
#include "StreamCtrl.h"

enum {
	dlg_alpha_mode_id = 101,		// AlphaMode.
	dlg_alpha_cutoff_id = 102,		// AlphaCutoff.
};

CAlphaModeMappingLayerInterface::CAlphaModeMappingLayerInterface (sxsdk::shade_interface& shade) : shade(shade)
{
}

CAlphaModeMappingLayerInterface::~CAlphaModeMappingLayerInterface ()
{
}

bool CAlphaModeMappingLayerInterface::ask_mapping (sxsdk::mapping_layer_class &mapping, void *)
{
	compointer<sxsdk::dialog_interface> dlg(shade.create_dialog_interface_with_uuid(ALPHA_MODE_INTERFACE_ID));
	dlg->set_resource_name("alpha_mode_dlg");

	dlg->set_responder(this);
	this->AddRef();

	StreamCtrl::loadAlphaModeMappingLayerParam(mapping, m_data);
	if (dlg->ask()) {
		StreamCtrl::saveAlphaModeMappingLayerParam(mapping, m_data);
		return true;
	}
	return false;	
}

/**
 * ダイアログの初期化.
 */
void CAlphaModeMappingLayerInterface::initialize_dialog (sxsdk::dialog_interface &d, void *)
{
}

/** 
 * ダイアログのイベントを受け取る.
 */
bool CAlphaModeMappingLayerInterface::respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == dlg_alpha_mode_id) {
		m_data.alphaModeType = (GLTFConverter::alpha_mode_type)item.get_selection();
		return true;
	}
	if (id == dlg_alpha_cutoff_id) {
		m_data.alphaCutoff = item.get_float();
		return true;
	}

	return false;
}

/**
 * ダイアログのデータを設定する.
 */
void CAlphaModeMappingLayerInterface::load_dialog_data (sxsdk::dialog_interface &d, void *)
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
	}
}

