/**
 * 数値演算系関数.
 */
#include "MathUtil.h"

/**
 * ゼロチェック.
 */
bool MathUtil::isZero (const sxsdk::vec2& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin);
}
bool MathUtil::isZero (const sxsdk::vec3& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin && std::abs(v.z) < fMin);
}
bool MathUtil::isZero (const sxsdk::rgb_class& v, const float fMin)
{
	return (std::abs(v.red) < fMin && std::abs(v.green) < fMin && std::abs(v.blue) < fMin);
}
bool MathUtil::isZero (const sxsdk::rgba_class& v, const float fMin)
{
	return (std::abs(v.red) < fMin && std::abs(v.green) < fMin && std::abs(v.blue) < fMin  && std::abs(v.alpha) < fMin);
}

bool MathUtil::isZero (const sxsdk::quaternion_class& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin && std::abs(v.z) < fMin && std::abs(v.w) < fMin);
}

bool MathUtil::isZero (const float v, const float fMin)
{
	return (std::abs(v) < fMin);
}

/**
 * 三角形の法線を計算.
 */
sxsdk::vec3 MathUtil::calcTriangleNormal (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3)
{
	const sxsdk::vec3 ev1 = v2 - v1;
	const sxsdk::vec3 ev2 = v3 - v2;
	return normalize(sx::product(ev1, ev2));
}

/**
 * 三角形の面積を計算.
 */
double MathUtil::calcTriangleArea (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3)
{
	double ax, ay, az, bx, by, bz;
	double fDat1, fDat2, fDat3;
	double S;

	ax = (double)(v2.x - v1.x);
	ay = (double)(v2.y - v1.y);
	az = (double)(v2.z - v1.z);
	bx = (double)(v3.x - v1.x);
	by = (double)(v3.y - v1.y);
	bz = (double)(v3.z - v1.z);

	fDat1 = ax * ax + ay * ay + az * az;
	fDat2 = bx * bx + by * by + bz * bz;
	fDat3 = ax * bx + ay * by + az * bz;
	fDat3 *= fDat3;

	fDat1 = fDat1 * fDat2 - fDat3;
	if (fDat1 < (1e-8)) return 0.0;
	S = std::sqrt(fDat1) * 0.5;
	return S;
}

/**
 * バウンディングボックスの最小最大を計算.
 */
void MathUtil::calcBoundingBox (const std::vector<sxsdk::vec3>& vers, sxsdk::vec3& bbMin, sxsdk::vec3& bbMax)
{
	bbMin = bbMax = sxsdk::vec3(0, 0, 0);
	if (vers.empty()) return;
	bbMin = bbMax = vers[0];
	for (size_t i = 1; i < vers.size(); ++i) {
		const sxsdk::vec3& v = vers[i];
		bbMin.x = std::min(bbMin.x, v.x);
		bbMin.y = std::min(bbMin.y, v.y);
		bbMin.z = std::min(bbMin.z, v.z);
		bbMax.x = std::max(bbMax.x, v.x);
		bbMax.y = std::max(bbMax.y, v.y);
		bbMax.z = std::max(bbMax.z, v.z);
	}
}

/**
 * 色情報を逆ガンマ2.2し、リニア化.
 */
void MathUtil::convColorLinear (float& vRed, float& vGreen, float& vBlue)
{
	const float gamma = 2.2f;
	if (vRed < 1.0f) {
		vRed   = powf(vRed, gamma);
	}
	if (vGreen < 1.0f) {
		vGreen = powf(vGreen, gamma);
	}
	if (vBlue < 1.0f) {
		vBlue  = powf(vBlue, gamma);
	}
}

/**
 * 色情報をガンマ2.2し、リニアからノンリニア化.
 */
void MathUtil::convColorNonLinear (float& vRed, float& vGreen, float& vBlue)
{
	const float gamma = 1.0f / 2.2f;
	if (vRed < 1.0f) {
		vRed   = powf(vRed, gamma);
	}
	if (vGreen < 1.0f) {
		vGreen = powf(vGreen, gamma);
	}
	if (vBlue < 1.0f) {
		vBlue  = powf(vBlue, gamma);
	}
}

/**
 * RGBをHSVに変換.
 */
sxsdk::vec3 MathUtil::rgb_to_hsv (const sxsdk::rgb_class& col)
{
	float h_val, s_val, v_val;
	float min_val;
	sxsdk::vec3 rgb;

	rgb = sxsdk::vec3(col.red, col.green, col.blue);
	if (rgb.x < rgb.y) {
		if (rgb.y < rgb.z) v_val = rgb.z;
		else v_val = rgb.y;
	} else {
		if (rgb.x < rgb.z) v_val = rgb.z;
		else v_val = rgb.x;
	}
	if (rgb.x < rgb.y) {
		if (rgb.x < rgb.z) min_val = rgb.x;
		else min_val = rgb.z;
	} else {
		if (rgb.y < rgb.z) min_val = rgb.y;
		else min_val = rgb.z;
	}
	if (sx::zero(v_val)) s_val = 0.0f;
	else s_val = (v_val - min_val) / v_val;

	if (sx::zero(v_val - min_val)) {
		rgb = sxsdk::vec3(0.0f, 0.0f, 0.0f);
	} else {
		rgb.x = (v_val - col.red)   / (v_val - min_val);
		rgb.y = (v_val - col.green) / (v_val - min_val);
		rgb.z = (v_val - col.blue)  / (v_val - min_val);
	}
	if (sx::zero(v_val - col.red)) h_val = 60.0f * (rgb.z - rgb.y);
	else if (sx::zero(v_val - col.green)) h_val = 60.0f * (2.0f + rgb.x - rgb.z);
	else h_val = 60.0f * (4.0f + rgb.y - rgb.x);
	if (h_val < 0.0f) h_val = h_val + 360.0f;

	return sxsdk::vec3(h_val, s_val, v_val);
}

/**
 * HSVをRGBに変換.
 */
sxsdk::rgb_class MathUtil::hsv_to_rgb (const sxsdk::vec3& hsv)
{
	const int i_val = (int)floor(hsv.x / 60.0f);
	float fl = (hsv.x / 60.0f) - (float)i_val;
	if (!(i_val & 1)) fl = 1.0 - fl;

	const float m = hsv.z * (1.0f - hsv.y);
	const float n = hsv.z * (1.0f - hsv.y * fl);

	sxsdk::rgb_class col;
	switch (i_val) {
	case 0:
		col = sxsdk::rgb_class(hsv.z, n, m);
		break;
	case 1:
		col = sxsdk::rgb_class(n, hsv.z, m);
		break;
	case 2:
		col = sxsdk::rgb_class(m, hsv.z, n);
		break;
	case 3:
		col = sxsdk::rgb_class(m, (float)n, hsv.z);
		break;
	case 4:
		col = sxsdk::rgb_class(n, (float)m, hsv.z);
		break;
	case 5:
		col = sxsdk::rgb_class(hsv.z, m, n);
		break;
	}
	return col;
}

/**
 * 法線マップのRGB( (0.5f, 0.5f, 1.0f)が中立 )を+Z向きの法線に変換.
 */
sxsdk::vec3 MathUtil::convRGBToNormal (const sxsdk::rgb_class& col)
{
	sxsdk::vec3 n;
	n.x = (col.red   - 0.5f) * 2.0f;
	n.y = (col.green - 0.5f) * 2.0f;
	n.z = col.blue;
	return sx::normalize(n);
}


/**
 * +Z向きの法線を法線マップのRGBに変換.
 */
sxsdk::rgb_class MathUtil::convNormalToRGB (const sxsdk::vec3& n, const bool normalizeF)
{
	sxsdk::rgb_class col;
	sxsdk::vec3 n2 = n;
	if (normalizeF) n2 = sx::normalize(n);

	col.red   = n2.x * 0.5f + 0.5f;
	col.green = n2.y * 0.5f + 0.5f;
	col.blue  = n2.z;

	return col;
}

/**
 * RGBをグレイスケールに変換.
 */
float MathUtil::rgb_to_grayscale (const sxsdk::rgb_class& col)
{
	return 0.2126f * col.red + 0.7152f * col.green + 0.0722f * col.blue;
}

