/**
 * glTF(glb/vrm)のライセンス情報を表示するダイアログボックス.
 */

#ifndef _LICENSEDIALOGINTERFACE_H
#define _LICENSEDIALOGINTERFACE_H

#include "GlobalHeader.h"

#include <string>

struct CLicenseDialogInterface : public sxsdk::attribute_interface
{
private:
	sxsdk::shade_interface& shade;

	CLicenseData m_data;		// 表示するデータ.

private:

	/**
	 * SDKのビルド番号を指定（これは固定で変更ナシ）。.
	 * ※ これはプラグインインターフェースごとに必ず必要。.
	 */
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }

	/**
	 * UUIDの指定（独自に定義したGUIDを指定）.
	 * ※ これはプラグインインターフェースごとに必ず必要。.
	 */
	virtual sx::uuid_class get_uuid (void * = 0) { return LICENSE_DIALOG_INTERFACE_ID; }

	/**
	 * メニューに表示しない.
	 */
	virtual void accepts_shape (bool &accept, void *aux=0) { accept = true; }

	virtual bool ask_shape (sxsdk::shape_class &shape, void *aux=0);

	//--------------------------------------------------.
	//  ダイアログのイベント処理用.
	//--------------------------------------------------.
	/**
	 * ダイアログの初期化.
	 */
	virtual void initialize_dialog (sxsdk::dialog_interface &d, void * = 0);

	/** 
	 * ダイアログのイベントを受け取る.
	 */
	virtual bool respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void * = 0);

	/**
	 * ダイアログのデータを設定する.
	 */
	virtual void load_dialog_data (sxsdk::dialog_interface &d, void * = 0);

public:
	CLicenseDialogInterface (sxsdk::shade_interface& shade);
	virtual ~CLicenseDialogInterface ();

	/**
	 * プラグイン名をSXUL(text.sxul)より取得.
	 */
	static const char *name (sxsdk::shade_interface *shade) { return shade->gettext("license_dialog_title"); }

	/**
	 * ダイアログボックスを表示.
	 */
	bool showDialog (sxsdk::shape_class& shape);
};

#endif
