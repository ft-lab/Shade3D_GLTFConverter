/**
 * glTFエクスポート時のWarning用.
 */

#include "WarningCheck.h"

CWarningCheck::CWarningCheck ()
{
	clear();
}

CWarningCheck::~CWarningCheck ()
{
}


void CWarningCheck::clear ()
{
	m_shearUsedNameList.clear();
	m_classicSkinUsedNameList.clear();
	m_unsupportedJointUsedNameList.clear();
}

/**
 * せん断が使用されている形状を追加.
 */
void CWarningCheck::appendShearUsedShape (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();

	int index = -1;
	for (size_t i = 0; i < m_shearUsedNameList.size(); ++i) {
		if (name == m_shearUsedNameList[i]) {
			index = (int)i;
			break;
		}
	}
	if (index >= 0) return;
	m_shearUsedNameList.push_back(name);
}

/**
 * クラシックスキンが使用されている形状を追加.
 */
void CWarningCheck::appendClassicSkinUsedShape (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();

	int index = -1;
	for (size_t i = 0; i < m_classicSkinUsedNameList.size(); ++i) {
		if (name == m_classicSkinUsedNameList[i]) {
			index = (int)i;
			break;
		}
	}
	if (index >= 0) return;
	m_classicSkinUsedNameList.push_back(name);
}

/**
 * 未サポートのジョイントを使っている形状を追加.
 */
void CWarningCheck::appendUnsupportedJointUsedShape (sxsdk::shape_class* shape)
{
	const std::string name = shape->get_name();

	int index = -1;
	for (size_t i = 0; i < m_unsupportedJointUsedNameList.size(); ++i) {
		if (name == m_unsupportedJointUsedNameList[i]) {
			index = (int)i;
			break;
		}
	}
	if (index >= 0) return;
	m_unsupportedJointUsedNameList.push_back(name);
}

/**
 * 警告メッセージを出力.
 */
void CWarningCheck::outputWarningMessage (sxsdk::shade_interface& shade)
{
	if (!m_shearUsedNameList.empty()) {
		std::string msgStr = shade.gettext("export_msg_used_shear");
		shade.message(msgStr);

		msgStr = "  ";
		for (size_t i = 0; i < m_shearUsedNameList.size(); ++i) {
			if (i != 0) msgStr += ",";
			msgStr += m_shearUsedNameList[i];
		}
		shade.message(msgStr);

		shade.message("");
	}

	if (!m_classicSkinUsedNameList.empty()) {
		std::string msgStr = shade.gettext("export_msg_used_classic_skin");
		shade.message(msgStr);

		msgStr = "  ";
		for (size_t i = 0; i < m_classicSkinUsedNameList.size(); ++i) {
			if (i != 0) msgStr += ",";
			msgStr += m_classicSkinUsedNameList[i];
		}
		shade.message(msgStr);

		shade.message("");
	}

	if (!m_unsupportedJointUsedNameList.empty()) {
		std::string msgStr = shade.gettext("export_msg_used_unsupport_joint");
		shade.message(msgStr);

		msgStr = "  ";
		for (size_t i = 0; i < m_unsupportedJointUsedNameList.size(); ++i) {
			if (i != 0) msgStr += ",";
			msgStr += m_unsupportedJointUsedNameList[i];
		}
		shade.message(msgStr);

		shade.message("");
	}
}

