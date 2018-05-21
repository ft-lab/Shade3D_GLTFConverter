/**
 * Shade3Dのシーン走査のため便利機能など.
 */
#include "Shade3DUtil.h"

/**
 * サーフェス情報を持つ親形状までたどる.
 * @param[in] shape  対象の形状.
 * @return サーフェス情報を持つ親形状。なければshapeを返す.
 */
sxsdk::shape_class* Shade3DUtil::getHasSurfaceParentShape (sxsdk::shape_class* shape)
{
	// サーフェスを持つ親までたどる.
	sxsdk::shape_class* shape2 = shape;
	while (!shape2->get_has_surface_attributes()) {
		if (!shape2->has_dad()) {
			shape2 = shape;
			break;
		}
		shape2 = shape2->get_dad();
	}

	return shape2;
}

/**
 * 面を反転するか。親をたどって調べる.
 */
bool Shade3DUtil::isFaceFlip (sxsdk::shape_class* shape)
{
	bool flipF = shape->get_flip_face();
	sxsdk::shape_class* shape2 = shape;
	while (shape2->has_dad()) {
		shape2 = shape2->get_dad();
		flipF = (shape2->get_flip_face()) ? !flipF : flipF;
	}
	return flipF;
}

/**
 * マスターイメージパートを取得.
 */
sxsdk::shape_class* Shade3DUtil::findMasteImagePart (sxsdk::scene_interface* scene)
{
	try {
		sxsdk::shape_class& rootShape = scene->get_shape();
		sxsdk::shape_class* pImagePartS = NULL;
		if (rootShape.has_son()) {
			sxsdk::shape_class* pS = rootShape.get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				if (!pS) break;
				if (pS->get_type() == sxsdk::enums::part) {
					sxsdk::part_class& part = pS->get_part();
					if (part.get_part_type() == sxsdk::enums::master_image_part) {
						pImagePartS = pS;
						break;
					}
				}
			}
			return pImagePartS;
		}
	} catch (...) { }

	return NULL;
}

/**
 * 指定のイメージに対応するマスターイメージを取得.
 * @param[in] scene  シーンクラス.
 * @param[in] image  対象のイメージ.
 * @return マスターイメージが存在する場合はそのポインタ.
 */
sxsdk::master_image_class* Shade3DUtil::getMasterImageFromImage (sxsdk::scene_interface* scene, sxsdk::image_interface* image)
{
	if (!image->has_image()) return NULL;
	const int width  = image->get_size().x;
	const int height = image->get_size().y;

	sxsdk::master_image_class* masterImageC = NULL;

	// マスターイメージパートを取得.
	sxsdk::shape_class* pMasterImagePart = findMasteImagePart(scene);
	if (!pMasterImagePart || !(pMasterImagePart->has_son())) return NULL;

	try {
		sxsdk::shape_class* pS = pMasterImagePart->get_son();
		while (pS->has_bro()) {
			pS = pS->get_bro();
			if (!pS) break;
			if (pS->get_type() != sxsdk::enums::master_image) continue;
			sxsdk::master_image_class& masterImage = pS->get_master_image();
			sxsdk::image_interface* image2 = masterImage.get_image();
			if (!image2->has_image()) continue;

			const int width2  = image2->get_size().x;
			const int height2 = image2->get_size().y;
			if (width != width2 || height != height2) continue;

			if (image->is_same_as(image2)) {
				masterImageC = &masterImage;
				break;
			}
		}
		return masterImageC;
	} catch (...) { }

	return NULL;
}
