/**
 * streamへの入出力.
 */

#ifndef _STREAMCTRL_H
#define _STREAMCTRL_H

#include "GlobalHeader.h"

namespace StreamCtrl
{
	/**
	 * Importダイアログボックスの情報を保存.
	 */
	void saveImportDialogParam (sxsdk::shade_interface& shade, const CImportDlgParam& data);

	/**
	 * Importダイアログボックスの情報を読み込み.
	 */
	void loadImportDialogParam (sxsdk::shade_interface& shade, CImportDlgParam& data);

	/**
	 * Exportダイアログボックスの情報を保存.
	 */
	void saveExportDialogParam (sxsdk::shade_interface& shade, const CExportDlgParam& data);

	/**
	 * Exportダイアログボックスの情報を読み込み.
	 */
	void loadExportDialogParam (sxsdk::shade_interface& shade, CExportDlgParam& data);
}

#endif
