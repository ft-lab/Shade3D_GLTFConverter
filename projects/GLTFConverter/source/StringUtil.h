/**
 * 文字列操作関数.
 */

#ifndef _STRINGUTIL_H
#define _STRINGUTIL_H

#include <string>

namespace StringUtil
{
	/**
	 * ファイルパスからファイル名のみを取得.
	 * @param[in] filePath      ファイルフルパス.
	 * @param[in] hasExtension  trueの場合は拡張子も付ける.
	 */
	const std::string getFileName (const std::string& filePath, const bool hasExtension = true);

	/**
	 * ファイル名を除いたディレクトリを取得.
	 * @param[in] filePath      ファイルフルパス.
	 */
	const std::string getFileDir (const std::string& filePath);

	/**
	 * ファイルパスから拡張子を取得 (gltfまたはglb).
	 * @param[in] filePath      ファイルフルパス.
	 */
	const std::string getFileExtension (const std::string& filePath);

}

#endif
