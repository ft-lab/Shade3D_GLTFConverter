﻿/**
 * 数値演算系関数.
 */
#ifndef _MATHUTIL_H
#define _MATHUTIL_H

#include "GlobalHeader.h"

namespace MathUtil {
	/**
	 * ゼロチェック.
	 */
	bool IsZero (const sxsdk::vec2& v, const float fMin = (float)(1e-3));
	bool IsZero (const sxsdk::vec3& v, const float fMin = (float)(1e-3));
	bool IsZero (const sxsdk::quaternion_class& v, const float fMin = (float)(1e-3));
	bool IsZero (const float v, const float fMin = (float)(1e-3));

	/**
	 * 三角形の法線を計算.
	 */
	sxsdk::vec3 CalcTriangleNormal (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3);

}

#endif