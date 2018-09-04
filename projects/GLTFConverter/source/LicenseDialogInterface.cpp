/**
 * glTF(glb/vrm)のライセンス情報を表示するダイアログボックス.
 */

#include "LicenseDialogInterface.h"
#include "StreamCtrl.h"

enum {
	dlg_exporter_version_id = 101,
	dlg_version_id = 102,
	dlg_author_id = 103,
	dlg_contact_information_id = 104,
	dlg_reference_id = 105,
	dlg_title_id = 106,
	dlg_allowed_user_name_id = 107,
	dlg_violent_ussage_name_id = 108,
	dlg_sexual_ussage_name_id = 109,
	dlg_commercial_ussage_name_id = 110,
	dlg_other_permission_url_id = 111,
	dlg_license_name_id = 112,
	dlg_other_license_url_id = 113,
};

CLicenseDialogInterface::CLicenseDialogInterface (sxsdk::shade_interface& shade) : shade(shade)
{
}

CLicenseDialogInterface::~CLicenseDialogInterface ()
{
}

/**
 * ダイアログボックスを表示.
 */
bool CLicenseDialogInterface::showDialog (sxsdk::shape_class& shape)
{
	compointer<sxsdk::dialog_interface> dlg(shade.create_dialog_interface_with_uuid(LICENSE_DIALOG_INTERFACE_ID));
	dlg->set_resource_name("license_dlg");

	dlg->set_responder(this);
	this->AddRef();

	StreamCtrl::loadLicenseData(shape, m_data);
	if (dlg->ask()) {
		StreamCtrl::saveLicenseData(shape, m_data);
		return true;
	}
	return false;	
}

bool CLicenseDialogInterface::ask_shape (sxsdk::shape_class &shape, void *)
{
	return showDialog(shape);
}

//--------------------------------------------------.
//  ダイアログのイベント処理用.
//--------------------------------------------------.
/**
 * ダイアログの初期化.
 */
void CLicenseDialogInterface::initialize_dialog (sxsdk::dialog_interface &d, void *)
{
}

/** 
 * ダイアログのイベントを受け取る.
 */
bool CLicenseDialogInterface::respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == dlg_exporter_version_id) {
		m_data.exporterVersion = std::string(item.get_string());
		return true;
	}
	if (id == dlg_version_id) {
		m_data.version = std::string(item.get_string());
		return true;
	}
	if (id == dlg_author_id) {
		m_data.author = std::string(item.get_string());
		return true;
	}
	if (id == dlg_contact_information_id) {
		m_data.contactInformation = std::string(item.get_string());
		return true;
	}
	if (id == dlg_reference_id) {
		m_data.reference = std::string(item.get_string());
		return true;
	}
	if (id == dlg_title_id) {
		m_data.title = std::string(item.get_string());
		return true;
	}
	if (id == dlg_allowed_user_name_id) {
		m_data.allowedUserName = std::string(item.get_string());
		return true;
	}
	if (id == dlg_violent_ussage_name_id) {
		m_data.violentUssageName = std::string(item.get_string());
		return true;
	}
	if (id == dlg_sexual_ussage_name_id) {
		m_data.sexualUssageName = std::string(item.get_string());
		return true;
	}
	if (id == dlg_commercial_ussage_name_id) {
		m_data.commercialUssageName = std::string(item.get_string());
		return true;
	}
	if (id == dlg_other_permission_url_id) {
		m_data.otherPermissionUrl = std::string(item.get_string());
		return true;
	}
	if (id == dlg_license_name_id) {
		m_data.licenseName = std::string(item.get_string());
		return true;
	}
	if (id == dlg_other_license_url_id) {
		m_data.otherLicenseUrl = std::string(item.get_string());
		return true;
	}

	return false;
}

/**
 * ダイアログのデータを設定する.
 */
void CLicenseDialogInterface::load_dialog_data (sxsdk::dialog_interface &d, void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_exporter_version_id));
		item->set_string(m_data.exporterVersion.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_version_id));
		item->set_string(m_data.version.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_author_id));
		item->set_string(m_data.author.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_contact_information_id));
		item->set_string(m_data.contactInformation.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_reference_id));
		item->set_string(m_data.reference.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_title_id));
		item->set_string(m_data.title.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_allowed_user_name_id));
		item->set_string(m_data.allowedUserName.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_violent_ussage_name_id));
		item->set_string(m_data.violentUssageName.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_sexual_ussage_name_id));
		item->set_string(m_data.sexualUssageName.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_commercial_ussage_name_id));
		item->set_string(m_data.commercialUssageName.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_other_permission_url_id));
		item->set_string(m_data.otherPermissionUrl.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_license_name_id));
		item->set_string(m_data.licenseName.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_other_license_url_id));
		item->set_string(m_data.otherLicenseUrl.c_str());
	}
}
