/**
 * Shade3Dのsxsdk::vec3の配列をfloat配列に変換など.
 */
#include "Shade3DArray.h"

/**
 * sxsdk::vec4をfoatの配列に置き換え.
 * @param[in] vList     vec4の配列.
 * @param[in] useAlpha  Alpha情報を格納するか.
 */
std::vector<float> Shade3DArray::convert_vec4_to_float (const std::vector<sxsdk::vec4>& vList, const bool useAlpha)
{
	const int eCou = useAlpha ? 4 : 3;
	std::vector<float> newData = std::vector<float>(vList.size() * eCou);
	for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += eCou) {
		newData[iPos + 0] = vList[i].x;
		newData[iPos + 1] = vList[i].y;
		newData[iPos + 2] = vList[i].z;
		if (useAlpha) newData[iPos + 3] = vList[i].w;
	}
	return newData;
}

/**
 * sxsdk::vec4をunsigned charの配列に置き換え.
 * @param[in] vList     vec4の配列.
 * @param[in] useAlpha  Alpha情報を格納するか.
 */
std::vector<unsigned char> Shade3DArray::convert_vec4_to_uchar (const std::vector<sxsdk::vec4>& vList, const bool useAlpha)
{
	const int eCou = useAlpha ? 4 : 3;
	std::vector<unsigned char> newData = std::vector<unsigned char>(vList.size() * eCou);
	for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += eCou) {
		newData[iPos + 0] = (unsigned char)std::min((int)(vList[i].x * 255.0f), 255);
		newData[iPos + 1] = (unsigned char)std::min((int)(vList[i].y * 255.0f), 255);
		newData[iPos + 2] = (unsigned char)std::min((int)(vList[i].z * 255.0f), 255);
		if (useAlpha) newData[iPos + 3] = (unsigned char)std::min((int)(vList[i].w * 255.0f), 255);
	}
	return newData;
}

/**
 * sxsdk::vec3をfoatの配列に置き換え.
 */
std::vector<float> Shade3DArray::convert_vec3_to_float (const std::vector<sxsdk::vec3>& vList)
{
	std::vector<float> newData = std::vector<float>(vList.size() * 3);
	for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += 3) {
		newData[iPos + 0] = vList[i].x;
		newData[iPos + 1] = vList[i].y;
		newData[iPos + 2] = vList[i].z;
	}
	return newData;
}

/**
 * sxsdk::vec2をfoatの配列に置き換え.
 */
std::vector<float> Shade3DArray::convert_vec2_to_float (const std::vector<sxsdk::vec2>& vList)
{
	std::vector<float> newData = std::vector<float>(vList.size() * 2);
	for (size_t i = 0, iPos = 0; i < vList.size(); ++i, iPos += 2) {
		newData[iPos + 0] = vList[i].x;
		newData[iPos + 1] = vList[i].y;
	}
	return newData;
}
