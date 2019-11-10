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
	bool isZero (const sxsdk::vec2& v, const float fMin = (float)(1e-3));
	bool isZero (const sxsdk::vec3& v, const float fMin = (float)(1e-3));
	bool isZero (const sxsdk::rgb_class& v, const float fMin = (float)(1e-3));
	bool isZero (const sxsdk::rgba_class& v, const float fMin = (float)(1e-3));
	bool isZero (const sxsdk::quaternion_class& v, const float fMin = (float)(1e-3));
	bool isZero (const float v, const float fMin = (float)(1e-3));

	/**
	 * 三角形の法線を計算.
	 */
	sxsdk::vec3 calcTriangleNormal (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3);

	/**
	 * 三角形の面積を計算.
	 */
	double calcTriangleArea (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3);

	/**
	 * バウンディングボックスの最小最大を計算.
	 */
	void calcBoundingBox (const std::vector<sxsdk::vec3>& vers, sxsdk::vec3& bbMin, sxsdk::vec3& bbMax);

	/**
	 * 色情報を逆ガンマ2.2し、リニア化.
	 */
	void convColorLinear (float& vRed, float& vGreen, float& vBlue);

}

#endif
