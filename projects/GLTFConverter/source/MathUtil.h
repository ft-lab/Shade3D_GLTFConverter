/**
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

	/**
	 * RGBをHSVに変換.
	 */
	sxsdk::vec3 rgb_to_hsv (const sxsdk::rgb_class& col);

	/**
	 * HSVをRGBに変換.
	 */
	sxsdk::rgb_class hsv_to_rgb (const sxsdk::vec3& hsv);

	/**
	 * RGBをグレイスケールに変換.
	 */
	float rgb_to_grayscale (const sxsdk::rgb_class& col);

	/**
	 * 法線マップのRGB( (0.5f, 0.5f, 1.0f)が中立 )を+Z向きの法線に変換.
	 */
	sxsdk::vec3 convRGBToNormal (const sxsdk::rgb_class& col);

	/**
	 * +Z向きの法線を法線マップのRGBに変換.
	 */
	sxsdk::rgb_class convNormalToRGB (const sxsdk::vec3& n, const bool normalizeF = true);
}

#endif
