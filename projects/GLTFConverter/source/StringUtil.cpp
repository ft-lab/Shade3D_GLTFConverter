/**
 * 文字列操作関数.
 */
#include "StringUtil.h"

#include <string>
#include <sstream>
#include <algorithm>

/**
 * ファイルパスからファイル名のみを取得.
 * @param[in] filePath      ファイルフルパス.
 * @param[in] hasExtension  trueの場合は拡張子も付ける.
 */
const std::string StringUtil::getFileName (const std::string& filePath, const bool hasExtension)
{
	// ファイルパスからファイル名を取得.
	std::string fileNameS = filePath;
	int iPos  = filePath.find_last_of("/");
	int iPos2 = filePath.find_last_of("\\");
	if (iPos != std::string::npos || iPos2 != std::string::npos) {
		if (iPos != std::string::npos && iPos2 != std::string::npos) {
			iPos = std::max(iPos, iPos2);
		} else if (iPos == std::string::npos) iPos = iPos2;
		if (iPos != std::string::npos) fileNameS = fileNameS.substr(iPos + 1);
	}
	if (hasExtension) return fileNameS;

	iPos = fileNameS.find_last_of(".");
	if (iPos != std::string::npos) {
		fileNameS = fileNameS.substr(0, iPos);
	}
	return fileNameS;
}

/**
 * ファイル名を除いたディレクトリを取得.
 * @param[in] filePath      ファイルフルパス.
 */
const std::string StringUtil::getFileDir (const std::string& filePath)
{
	int iPos  = filePath.find_last_of("/");
	int iPos2 = filePath.find_last_of("\\");
	if (iPos == std::string::npos && iPos2 == std::string::npos) return filePath;
	if (iPos != std::string::npos && iPos2 != std::string::npos) {
		iPos = std::max(iPos, iPos2);
	} else if (iPos == std::string::npos) iPos = iPos2;
	if (iPos == std::string::npos) return filePath;

	return filePath.substr(0, iPos);
}

/**
 * ファイルパスから拡張子を取得 (gltfまたはglb).
 * @param[in] filePath      ファイルフルパス.
 */
const std::string StringUtil::getFileExtension (const std::string& filePath)
{
	std::string fileNameS = getFileName(filePath);
	const int iPos = fileNameS.find_last_of(".");
	if (iPos == std::string::npos) return "";

	std::transform(fileNameS.begin(), fileNameS.end(), fileNameS.begin(), ::tolower);

	return fileNameS.substr(iPos + 1);
}

/**
 * 文字列内の全置換.
 * @param[in] targetStr   対象の文字列.
 * @param[in] srcStr      置き換え前の文字列.
 * @param[in] dstStr      置き換え後の文字列.
 * @return 変換された文字列.
 */
std::string StringUtil::replaceString (const std::string& targetStr, const std::string& srcStr, const std::string& dstStr)
{
	 std::string str = targetStr;

	 std::string::size_type pos = 0;
	 while ((pos = str.find(srcStr, pos)) != std::string::npos) {
		str.replace(pos, srcStr.length(), dstStr);
		pos += dstStr.length();
	 }
	 return str;
}


/**
 * テキストをHTML用に変換.
 *  & ==> &amp;  < ==> &lt; など.
 */
std::string StringUtil::convHTMLEncode (const std::string& str)
{
	 std::string str2 = str;
	 str2 = replaceString(str2, "&", "&amp;");
	 str2 = replaceString(str2, "\"", "&quot;");
	 str2 = replaceString(str2, "<", "&lt;");
	 str2 = replaceString(str2, ">", "&gt;");
	 return str2;
}

/**
 * HTML用のテキストを元に戻す.
 *  &amp; ==> &   &lt; ==> < など.
 */
std::string StringUtil::convHTMLDecode (const std::string& str)
{
	 std::string str2 = str;
	 str2 = replaceString(str2, "&gt;", ">");
	 str2 = replaceString(str2, "&lt;", "<");
	 str2 = replaceString(str2, "&quot;", "\"");
	 str2 = replaceString(str2, "&amp;", "&");
	 return str2;
}
