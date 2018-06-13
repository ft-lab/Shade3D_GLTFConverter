/**
 * Shade3Dのシーン走査のため便利機能など.
 */
#include "Shade3DUtil.h"
#include "MathUtil.h"

namespace {
	/**
	 * 再帰的にボーンのルートを探す.
	 */
	void findBoneRootLoop (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList) {
		if (rootShape->has_son()) {
			sxsdk::shape_class* pS = rootShape->get_son();
			while (pS->has_bro()) {
				pS = pS->get_bro();
				if (!pS) break;
				if (pS->get_type() == sxsdk::enums::part) {
					sxsdk::part_class& part = pS->get_part();
					if (part.get_part_type() == sxsdk::enums::bone_joint || part.get_part_type() == sxsdk::enums::ball_joint) {
						boneRootList.push_back(pS);
					} else {
						findBoneRootLoop(pS, boneRootList);
					}
				}
			}
		}
	}

	/**
	 * ボーンの向きをそろえる再帰.
	 */
	void adjustBonesDirectionLoop (sxsdk::shape_class& shape) {
		if (!Shade3DUtil::isBone(shape)) return;

		const sxsdk::mat4 lwMat = shape.get_transformation() * shape.get_local_to_world_matrix();
		const sxsdk::mat4 wlMat = inv(lwMat);

		bool firstF = true;
		if (shape.has_son()) {
			sxsdk::part_class* pPart   = &shape.get_part();
			sxsdk::shape_class* pShape = shape.get_son();
			while (pShape->has_bro()) {
				pShape = pShape->get_bro();

				if (firstF) {
					firstF = false;
					if (Shade3DUtil::isBone(*pShape)) {
						const sxsdk::vec3 v0 = Shade3DUtil::getBoneCenter(shape, NULL) * wlMat;
						const sxsdk::vec3 v1 = Shade3DUtil::getBoneCenter(*pShape, NULL) * wlMat;

						const sxsdk::vec3 vDir = normalize(v1 - v0);
						try {
							compointer<sxsdk::bone_joint_interface> bone(shape.get_bone_joint_interface());
							if (!(bone->get_auto_direction())) {
								bone->set_axis_dir(vDir);
							}
						} catch (...) { }
					}
				}
				adjustBonesDirectionLoop(*pShape);
			}
		}
	}

	/**
	 * ボーンの大きさを調整する再帰.
	 */
	void resizeBonesLoop (sxsdk::shape_class& shape, sxsdk::shape_class* prevShape) {
		if (!Shade3DUtil::isBone(shape)) return;

		float size = 0.0f;

		sxsdk::vec3 center = Shade3DUtil::getBoneCenter(shape, &size);

		{
			const float scale = 1.0f;
			float dist = 0.0f;
			float sizePrev = size;
			sxsdk::vec3 centerPrev = center;
			if (prevShape) {
				centerPrev = Shade3DUtil::getBoneCenter(*prevShape, &sizePrev);
				dist = sxsdk::distance3(centerPrev, center);
			}
			if (dist > 0.0f) {
				size = (dist / 8.0f) * scale;
				try {
					compointer<sxsdk::bone_joint_interface> bone(shape.get_bone_joint_interface());
					bone->set_size(size);
				} catch (...) { }
				if (prevShape) {
					try {
						compointer<sxsdk::bone_joint_interface> bone(prevShape->get_bone_joint_interface());
						bone->set_size(size);
					} catch (...) { }
				}
			}
		}

		if (shape.has_son()) {
			sxsdk::shape_class* pShape = shape.get_son();
			while (pShape->has_bro()) {
				pShape = pShape->get_bro();
				resizeBonesLoop(*pShape, &shape);
			}
		}
	}
}

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
	if (!image) return NULL;
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

/**
 * ボーンのルートを取得.
 * @param[in]  rootShape     検索を開始するルート.
 * @param[out] bontRootList  ルートボーンが返る.
 * @return ルートボーン数.
 */
int Shade3DUtil::findBoneRoot (sxsdk::shape_class* rootShape, std::vector<sxsdk::shape_class*>& boneRootList)
{
	boneRootList.clear();
	::findBoneRootLoop(rootShape, boneRootList);
	return (int)boneRootList.size();
}

/**
 * 指定の形状がボーンかどうか.
 */
bool Shade3DUtil::isBone (sxsdk::shape_class& shape)
{
	if (shape.get_type() != sxsdk::enums::part) return false;
	sxsdk::part_class& part = shape.get_part();
	if (part.get_part_type() == sxsdk::enums::bone_joint) return true;
	return false;
}

/**
 * ボーンのワールド座標での中心位置とボーンサイズを取得.
 */
sxsdk::vec3 Shade3DUtil::getBoneCenter (sxsdk::shape_class& shape, float *size)
{
	if (size) *size = 0.0f;
	if (!Shade3DUtil::isBone(shape)) return sxsdk::vec3(0, 0, 0);

	// シーケンスOff時の中心位置を取得する.
	// この場合は、bone->get_matrix()を使用.
	// shape.get_transformation() を取得すると、これはシーケンスOn時の変換行列になる.
	try {
		compointer<sxsdk::bone_joint_interface> bone(shape.get_bone_joint_interface());
		const sxsdk::mat4 m = bone->get_matrix();

		const sxsdk::mat4 lwMat = shape.get_local_to_world_matrix();
		const sxsdk::vec3 center = sxsdk::vec3(0, 0, 0) * m * lwMat;

		if (size) *size = bone->get_size();

		return center;
	} catch (...) { }

	return sxsdk::vec3(0, 0, 0);

}

/**
 * ボーンの向きをそろえる.
 * @param[in] boneRoot  対象のボーンルート.
 */
void Shade3DUtil::adjustBonesDirection (sxsdk::shape_class* boneRoot)
{
	::adjustBonesDirectionLoop(*boneRoot);
}

/**
 * ボーンサイズの自動調整.
 * @param[in] boneRoot  対象のボーンルート.
 */
void Shade3DUtil::resizeBones (sxsdk::shape_class* boneRoot)
{
	::resizeBonesLoop(*boneRoot, NULL);
}

/**
 * 指定の形状の階層の深さを取得.
 */
int Shade3DUtil::getShapeHierarchyDepth (sxsdk::shape_class* pShape)
{
	int depth = 0;
	sxsdk::shape_class* pS = pShape;
	while (pS->has_dad()) {
		depth++;
		pS = pS->get_dad();
	}
	return depth;
}

/**
 * 頂点カラーのレイヤを持つか.
 */
bool Shade3DUtil::hasVertexColor (sxsdk::shape_class* shape)
{
	if (shape->get_type() != sxsdk::enums::polygon_mesh) return false;
	sxsdk::shape_class* shape2 = Shade3DUtil::getHasSurfaceParentShape(shape);
	if (!shape2->get_has_surface_attributes()) return false;

	// 頂点カラーのマッピングレイヤを持つか.
	try {
		sxsdk::surface_class* surface = shape2->get_surface();
		const int layersCou = surface->get_number_of_mapping_layers();
		if (layersCou <= 0) return false;
		bool hasVertexColorMappingLayer = false;
		for (int i = 0; i < layersCou; ++i) {
			const sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(i);
			if (mappingLayer.get_pattern() != sxsdk::enums::vertex_color_pattern) continue;
			if (mappingLayer.get_vertex_color_layer() == 0) {
				hasVertexColorMappingLayer = true;
				break;
			}
		}
		if (!hasVertexColorMappingLayer) return false;
	} catch (...) {
		return false;
	}

	// ポリゴンメッシュの頂点カラー情報があるか.
	try {
		sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
		if (pMesh.get_number_of_vertex_color_layers() >= 1)  return true;
	} catch (...) {
		return false;
	}

	return false;
}

/**
 * 表面材質のマッピングレイヤとして、頂点カラー0の層を乗算合成で追加.
 */
void Shade3DUtil::setVertexColorSurfaceLayer (sxsdk::master_surface_class* pMasterSurface)
{
	if (!pMasterSurface->get_has_surface_attributes()) {
		pMasterSurface->set_has_surface_attributes(true);
	}
	sxsdk::surface_class* surface = pMasterSurface->get_surface();
	const int layersCou = surface->get_number_of_mapping_layers();

	bool hasVertexColorMapping = false;
	bool hasDiffuseMapping = false;
	for (int i = 0; i < layersCou; ++i) {
		sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(i);
		const int pattern = mappingLayer.get_pattern();
		if (pattern == sxsdk::enums::vertex_color_pattern) {
			if (mappingLayer.get_vertex_color_layer() == 0) {
				hasVertexColorMapping = true;
				break;
			}
		}
		if (mappingLayer.get_type() == sxsdk::enums::diffuse_mapping) {
			hasDiffuseMapping = true;
		}
	}
	if (hasVertexColorMapping) return;

	// 頂点カラー(頂点カラー0)のマッピングレイヤを追加.
	{
		surface->append_mapping_layer();
		const int layerIndex = layersCou;
		sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(layerIndex);
		mappingLayer.set_pattern(sxsdk::enums::vertex_color_pattern);
		mappingLayer.set_type(sxsdk::enums::diffuse_mapping);
		mappingLayer.set_vertex_color_layer(0);

		if (hasDiffuseMapping) {				// 乗算合成にする.
			mappingLayer.set_blend_mode(7);		// 乗算合成.
		}
	}
}

/**
 * 形状のモーションとして、指定のキーフレーム位置の要素が存在するか調べる.
 * @param[in] motion      形状が持つmotionクラスの参照.
 * @param[in] keyFrameV   キーフレーム値.
 * @return モーションポイントのインデックス。見つからなければ-1.
 */
int Shade3DUtil::findMotionPoint (sxsdk::motion_interface* motion, const float keyFrameV)
{
	int index = -1;
	try {
		const int mCou = motion->get_number_of_motion_points();
		for (int i = 0; i < mCou; ++i) {
			compointer<sxsdk::motion_point_interface> motionP(motion->get_motion_point_interface(i));
			if (MathUtil::isZero((motionP->get_sequence()) - keyFrameV)) {
				index = i;
				break;
			}
		}
		return index;
	} catch (...) { }

	return -1;
}
