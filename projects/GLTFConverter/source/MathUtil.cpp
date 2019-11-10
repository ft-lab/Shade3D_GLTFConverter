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
	if (fDat1 < (1e-5)) return 0.0;
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

