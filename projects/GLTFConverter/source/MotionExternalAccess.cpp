/**
 * 外部アクセス用.
 */
#include "MotionExternalAccess.h"

/**
 * CBoneAttributeAccessクラスの参照を取得.
 */
CBoneAttributeAccess* getBoneAttributeAccess (sxsdk::shade_interface& shade)
{
	try {
		sxsdk::plugin_interface* pluginI = shade.find_plugin_interface_with_uuid(HIDDEN_BONE_UTIL_INTERFACE_ID);
		if (pluginI) {
			CBoneAttributeAccess* pA = (CBoneAttributeAccess *)pluginI;
			return pA;
		}
	} catch (...) { }
	return NULL;
}

/**
 * CMorphTargetsAccessクラスの参照を取得.
 */
CMorphTargetsAccess* getMorphTargetsAttributeAccess (sxsdk::shade_interface& shade)
{
	try {
		sxsdk::plugin_interface* pluginI = shade.find_plugin_interface_with_uuid(HIDDEN_MORPH_TARGETS_INTERFACE_ID);
		if (pluginI) {
			CMorphTargetsAccess* pA = (CMorphTargetsAccess *)pluginI;
			return pA;
		}
	} catch (...) { }
	return NULL;
}

