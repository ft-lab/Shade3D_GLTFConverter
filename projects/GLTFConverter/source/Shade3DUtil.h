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
	 * 頂点カラーのレイヤを持つか.
	 */
	bool hasVertexColor (sxsdk::shape_class* shape);

	/**
	 * 表面材質のマッピングレイヤとして、頂点カラー0の層を乗算合成で追加.
	 */
	void setVertexColorSurfaceLayer (sxsdk::master_surface_class* pMasterSurface);

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

	/**
	 * 指定の形状の階層の深さを取得.
	 */
	int getShapeHierarchyDepth (sxsdk::shape_class* pShape);

	/**
	 * 形状のモーションとして、指定のキーフレーム位置の要素が存在するか調べる.
	 * @param[in] motion      形状が持つmotionクラスの参照.
	 * @param[in] keyFrameV   キーフレーム値.
	 * @return モーションポイントのインデックス。見つからなければ-1.
	 */
	int findMotionPoint (sxsdk::motion_interface* motion, const float keyFrameV);

	/**
	 * 指定のマッピングレイヤがOcclusion用のレイヤかどうか.
	 */
	bool isOcclusionMappingLayer (sxsdk::mapping_layer_class* mappingLayer);

	/**
	 * 選択形状(active_shape)での、Occlusion用のmapping_layer_classを取得.
	 */
	sxsdk::mapping_layer_class* getActiveShapeOcclusionMappingLayer (sxsdk::scene_interface* scene);

	/**
	 * 指定のimageにアルファ値を乗算したimageを複製.
	 */
	compointer<sxsdk::image_interface> duplicateImageWithAlpha (sxsdk::image_interface* image, const float alpha);

	/**
	 * 指定のマスターイメージがAlpha情報を持つかどうか.
	 * @param[in] masterImage  マスターイメージ.
	 * @return アルファ要素が1.0でないものがある場合はtrueを返す.
	 */
	bool hasImageAlpha (sxsdk::master_image_class* masterImage);

	/**
	 * 画像を指定のサイズにリサイズ。アルファも考慮（image->duplicate_imageはアルファを考慮しないため）.
	 * @param[in] image  元の画像.
	 * @param[in] size   変更するサイズ.
	 */
	compointer<sxsdk::image_interface> resizeImageWithAlpha (sxsdk::scene_interface* scene, sxsdk::image_interface* image, const sx::vec<int,2>& size);

	/**
	 * compointerを使用せずにイメージをリサイズ.
	 */
	sxsdk::image_interface* resizeImageWithAlphaNotCom (sxsdk::scene_interface* scene, sxsdk::image_interface* image, const sx::vec<int,2>& size);

}

#endif
