/**
 * 数値演算系関数.
 */
#include "MathUtil.h"

/**
 * ゼロチェック.
 */
bool MathUtil::IsZero (const sxsdk::vec2& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin);
}
bool MathUtil::IsZero (const sxsdk::vec3& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin && std::abs(v.z) < fMin);
}
bool MathUtil::IsZero (const sxsdk::quaternion_class& v, const float fMin)
{
	return (std::abs(v.x) < fMin && std::abs(v.y) < fMin && std::abs(v.z) < fMin && std::abs(v.w) < fMin);
}

bool MathUtil::IsZero (const float v, const float fMin)
{
	return (std::abs(v) < fMin);
}

/**
 * 三角形の法線を計算.
 */
sxsdk::vec3 MathUtil::CalcTriangleNormal (const sxsdk::vec3& v1, const sxsdk::vec3& v2, const sxsdk::vec3& v3)
{
	const sxsdk::vec3 ev1 = v2 - v1;
	const sxsdk::vec3 ev2 = v3 - v2;
	return normalize(sx::product(ev1, ev2));
}
