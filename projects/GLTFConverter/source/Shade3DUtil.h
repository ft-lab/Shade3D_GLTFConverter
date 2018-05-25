/**
 * Shade3Dのシーン走査のため便利機能など.
 */

#ifndef _SHADE3DUTIL_H
#define _SHADE3DUTIL_H

#include "GlobalHeader.h"

#include <vector>

namespace Shade3DUtil
{
	/**
	 * マスターイメージパートを取得.
	 * @param[in] scene  シーンクラス.
	 */
	sxsdk::shape_class* findMasteImagePart (sxsdk::scene_interface* scene);

	/**
	 * サーフェス情報を持つ親形状までたどる.
	 * @param[in] shape  対象の形状.
	 * @return サーフェス情報を持つ親形状。なければshapeを返す.
	 */
	sxsdk::shape_class* getHasSurfaceParentShape (sxsdk::shape_class* shape);

	/**
	 * 面を反転するか。親をたどって調べる.
	 */
	bool isFaceFlip (sxsdk::shape_class* shape);

	/**
	 * 指定のイメージに対応するマスターイメージを取得.
	 * @param[in] scene  シーンクラス.
	 * @param[in] image  対象のイメージ.
	 * @return マスターイメージが存在する場合はそのポインタ.
	 */
	sxsdk::master_image_class* getMasterImageFromImage (sxsdk::scene_interface* scene, sxsdk::image_interface* image);

	/**
	 * ボーンのルートを取得.
	 * @param[in]  rootShape     検索を開始するルート.
	 * @param[out] bontRootList  ルートボーンが返る.
	 * @return ルートボーン数.
	 */
	int findBoneRoot (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList);

	/**
	 * 指定の形状がボーンかどうか.
	 */
	bool isBone (sxsdk::shape_class& shape);

	/**
	 * ボーンのワールド座標での中心位置とボーンサイズを取得.
	 */
	sxsdk::vec3 getBoneCenter (sxsdk::shape_class& shape, float *size);

	/**
	 * ボーンの向きをそろえる.
	 * @param[in] boneRoot  対象のボーンルート.
	 */
	void adjustBonesDirection (sxsdk::shape_class* boneRoot);

	/**
	 * ボーンサイズの自動調整.
	 * @param[in] boneRoot  対象のボーンルート.
	 */
	void resizeBones (sxsdk::shape_class* boneRoot);
}

#endif
