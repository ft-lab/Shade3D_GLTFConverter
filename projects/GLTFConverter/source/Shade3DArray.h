/**
 * Shade3Dのsxsdk::vec3の配列をfloat配列に変換など.
 */
#ifndef _SHADE3DARRAY_H_
#define _SHADE3DARRAY_H_

#include "GlobalHeader.h"

#include <vector>

namespace Shade3DArray
{
	/**
	 * sxsdk::vec4をfoatの配列に置き換え.
	 * @param[in] vList     vec4の配列.
	 * @param[in] useAlpha  Alpha情報を格納するか.
	 */
	std::vector<float> convert_vec4_to_float (const std::vector<sxsdk::vec4>& vList, const bool useAlpha = true);

	/**
	 * sxsdk::vec4をunsigned charの配列に置き換え.
	 * @param[in] vList     vec4の配列.
	 * @param[in] useAlpha  Alpha情報を格納するか.
	 */
	std::vector<unsigned char> convert_vec4_to_uchar (const std::vector<sxsdk::vec4>& vList, const bool useAlpha = true);

	/**
	 * sxsdk::vec3をfoatの配列に置き換え.
	 */
	std::vector<float> convert_vec3_to_float (const std::vector<sxsdk::vec3>& vList);

	/**
	 * sxsdk::vec2をfoatの配列に置き換え.
	 */
	std::vector<float> convert_vec2_to_float (const std::vector<sxsdk::vec2>& vList);
}

#endif
