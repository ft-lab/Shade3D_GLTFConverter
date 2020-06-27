/**
 * glTFエクスポート時のWarning用.
 */

#ifndef _WARNINGCHECK_H
#define _WARNINGCHECK_H

#include "GlobalHeader.h"
#include <vector>
#include <string>

class CWarningCheck
{
private:
	std::vector<std::string> m_shearUsedNameList;					// せん断を使っている形状名のリスト.
	std::vector<std::string> m_classicSkinUsedNameList;				// クラシックスキンを使っている形状名のリスト.
	std::vector<std::string> m_unsupportedJointUsedNameList;		// 未サポートのジョイントを使っている形状名のリスト.

public:
	CWarningCheck ();
	~CWarningCheck ();

	void clear ();

	/**
	 * せん断が使用されている形状を追加.
	 */
	void appendShearUsedShape (sxsdk::shape_class* shape);

	/**
	 * クラシックスキンが使用されている形状を追加.
	 */
	void appendClassicSkinUsedShape (sxsdk::shape_class* shape);

	/**
	 * 未サポートのジョイントを使っている形状を追加.
	 */
	void appendUnsupportedJointUsedShape (sxsdk::shape_class* shape);


	/**
	 * 警告メッセージを出力.
	 */
	void outputWarningMessage (sxsdk::shade_interface& shade);
};

#endif
