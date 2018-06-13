/**
 * GLTFファイルを読み込むインポータクラス.
 */
#include "GLTFImporterInterface.h"
#include "GLTFLoader.h"
#include "SceneData.h"
#include "Shade3DUtil.h"
#include "MathUtil.h"

enum
{
	dlg_gamma_id = 101,							// ガンマ値.
	dlg_import_animation_id = 102,				// アニメーションの読み込み.
	dlg_mesh_import_normals_id = 201,			// 法線の読み込み.
	dlg_mesh_angle_threshold_id = 202,			// 限界角度.
	dlg_mesh_import_vertex_color_id = 203,		// 頂点カラーの読み込み.
};

namespace {
	// importerでは常駐にならないからか、クラス内に情報を持つと次回読み込みでクリアされることになるので.
	// 外部に出した.
	CImportDlgParam g_importParam;			// インポート時のパラメータ.
}

CGLTFImporterInterface::CGLTFImporterInterface (sxsdk::shade_interface &shade) : shade(shade)
{
}

/**
 * アプリケーション終了時に呼ばれる.
 */
void CGLTFImporterInterface::cleanup (void *)
{
	// 作業フォルダを削除.
	// 作業フォルダは、shade.get_temporary_path()で与えた後に削除すると、.
	// アプリ内では再度作業フォルダを作成できない。そのため、removeはアプリ終了時に呼んでいる.
	try {
		m_tempPath = std::string(shade.get_temporary_path("shade3d_temp_gltf"));
		if (m_tempPath != "") {
			shade.remove_directory_and_files(m_tempPath.c_str());
		}
	} catch (...) { }
}

/**
 * 扱うファイル拡張子数3
 */
int CGLTFImporterInterface::get_number_of_file_extensions (void *)
{
	return 2;
}

/**
 * @brief  ファイル拡張子.
 */
const char *CGLTFImporterInterface::get_file_extension (int index, void *)
{
	if (index == 0) return "glb";
	if (index == 1) return "gltf";

	return 0;
}

/**
 * @brief ファイル詳細.
 */
const char *CGLTFImporterInterface::get_file_extension_description (int index, void*)
{
	if (index == 0) return "glTF";
	if (index == 1) return "glTF";

	return 0;
}

/**
 * @brief  前処理(ダイアログを出す場合など).
 */
void CGLTFImporterInterface::do_pre_import (const sxsdk::mat4 &t, void *path)
{
	m_coordinatesMatrix = t;
}

/**
 * インポートを行う.
 */
void CGLTFImporterInterface::do_import (sxsdk::scene_interface *scene, sxsdk::stream_interface *stream, sxsdk::text_stream_interface *text_stream, void *)
{
	// ファイル名を取得.
	std::string fileName = "";
	if (text_stream) {
		fileName = std::string(text_stream->get_name());
	}
	if (fileName == "" && stream) {
		fileName = std::string(stream->get_name());
	}
	if (fileName == "") return;

	// GLTFファイルを読み込み.
	CGLTFLoader gltfLoader;
	CSceneData sceneData;

	shade.message("----- glTF Importer -----");

	if (!gltfLoader.loadGLTF(fileName, &sceneData)) {
		const std::string errorMessage = std::string("Error : ") + gltfLoader.getErrorString();
		shade.message(errorMessage);
		return;
	}

	shade.message(std::string("File : ") + fileName);
	shade.message(std::string("Asset generator : ") + sceneData.assetGenerator);
	shade.message(std::string("Asset version : ") + sceneData.assetVersion);
	shade.message(std::string("Asset copyright : ") + sceneData.assetCopyRight);

	shade.message(std::string("Meshes : ") + std::to_string(sceneData.meshes.size()));
	shade.message(std::string("Materials : ") + std::to_string(sceneData.materials.size()));
	shade.message(std::string("Images : ") + std::to_string(sceneData.images.size()));
	shade.message(std::string("Skins : ") + std::to_string(sceneData.skins.size()));
	shade.message(std::string("Animations : ") + std::to_string(sceneData.animations.hasAnimation() ? 1 : 0));

	// シーン情報を構築.
	m_createGLTFScene(scene, &sceneData);
}

/****************************************************************/
/* ダイアログイベント											*/
/****************************************************************/
void CGLTFImporterInterface::initialize_dialog (sxsdk::dialog_interface& dialog, void*)
{
}

void CGLTFImporterInterface::load_dialog_data (sxsdk::dialog_interface &d,void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_gamma_id));
		item->set_selection((int)g_importParam.gamma);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_import_animation_id));
		item->set_bool(g_importParam.importAnimation);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_mesh_import_normals_id));
		item->set_bool(g_importParam.meshImportNormals);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_mesh_angle_threshold_id));
		item->set_float(g_importParam.meshAngleThreshold);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_mesh_import_vertex_color_id));
		item->set_bool(g_importParam.meshImportVertexColor);
	}

	// 「座標変換」を無効化.
	{
		{
			sxsdk::dialog_item_class* item;
			item = &(d.get_dialog_item(10002));
			item->set_enabled(false);
		}
		for (int i = 0; i < 3; ++i) {
			sxsdk::dialog_item_class* item;
			item = &(d.get_dialog_item(10021 + i));
			item->set_enabled(false);
		}
		for (int i = 0; i < 3; ++i) {
			sxsdk::dialog_item_class* item;
			item = &(d.get_dialog_item(10031 + i));
			item->set_enabled(false);
		}
		{
			sxsdk::dialog_item_class* item;
			item = &(d.get_dialog_item(10041));
			item->set_enabled(false);
		}
	}
}

void CGLTFImporterInterface::save_dialog_data (sxsdk::dialog_interface &dialog,void *)
{
}

bool CGLTFImporterInterface::respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();

	if (id == sx::iddefault) {
		g_importParam.clear();
		load_dialog_data(dialog);
		return true;
	}

	if (id == dlg_gamma_id) {
		g_importParam.gamma = (int)item.get_selection();
		return true;
	}

	if (id == dlg_import_animation_id) {
		g_importParam.importAnimation = (int)item.get_bool();
		return true;
	}

	if (id == dlg_mesh_import_normals_id) {
		g_importParam.meshImportNormals = (int)item.get_bool();
		return true;
	}

	if (id == dlg_mesh_angle_threshold_id) {
		g_importParam.meshAngleThreshold = item.get_float();
		return true;
	}

	if (id == dlg_mesh_import_vertex_color_id) {
		g_importParam.meshImportVertexColor = (int)item.get_bool();
		return true;
	}

	return false;
}

/****************************************************************/

/**
 * GLTFを読み込んだシーン情報より、Shade3Dの形状として構築.
 */
void CGLTFImporterInterface::m_createGLTFScene (sxsdk::scene_interface *scene, CSceneData* sceneData)
{
	scene->begin_creating();

	// イメージを作成.
	m_createGLTFImages(scene, sceneData);

	// マテリアル情報から、マスターサーフェスを作成.
	m_createGLTFMaterials(scene, sceneData);

	sxsdk::part_class& rootPart = scene->begin_part(sceneData->getFileName().c_str());

	// シーン階層をたどってノードとメッシュ作成.
	m_createGLTFNodeHierarchy(scene, sceneData, 0);

	scene->end_part();
	scene->end_creating();

	// スキンを割り当て.
	m_setMeshSkins(scene, sceneData);

	// ボーン調整.
	m_adjustBones(&rootPart);

	// アニメーション情報を割り当て.
	if (g_importParam.importAnimation) {
		m_setAnimations(scene, sceneData);
	}
}

/**
 * GLTFを読み込んだシーン情報より、ノードを再帰的にたどって階層構造を構築.
 */
void CGLTFImporterInterface::m_createGLTFNodeHierarchy (sxsdk::scene_interface *scene, CSceneData* sceneData, const int nodeIndex)
{
	CNodeData& nodeD = sceneData->nodes[nodeIndex];

	// パートもしくはボーンとして開始.
	sxsdk::part_class* part = NULL;
	nodeD.pShapeHandle = NULL;
	if (nodeIndex > 0 && nodeD.meshIndex < 0) {
		if (nodeD.isBone) {
			const float bone_r = 10.0f;
			const sxsdk::vec3 bonePos(0, 0, 0);
			const sxsdk::vec3 boneAxisDir(1, 0, 0);
			part = &(scene->begin_bone_joint(bonePos, bone_r, false, boneAxisDir, nodeD.name.c_str()));
		} else {
			part = &(scene->begin_part(nodeD.name.c_str()));
		}
		nodeD.pShapeHandle = part->get_handle();
	}

	if (nodeIndex > 0 && nodeD.meshIndex >= 0) {
		// メッシュを生成.
		sxsdk::mat4 m = (nodeD.childNodeIndex >= 0) ? sxsdk::mat4::identity : (sceneData->getNodeMatrix(nodeIndex));
		m_createGLTFMesh(nodeD.name, scene, sceneData, nodeD.meshIndex, m);
	}

	std::vector<int> childNodeIndexList;
	if (nodeD.childNodeIndex >= 0) {
		int nIndex = nodeD.childNodeIndex;
		while (nIndex >= 0) {
			childNodeIndexList.push_back(nIndex);
			nIndex = sceneData->nodes[nIndex].nextNodeIndex;
		}
	}
	for (size_t i = 0; i < childNodeIndexList.size(); ++i) {
		m_createGLTFNodeHierarchy(scene, sceneData, childNodeIndexList[i]);
	}

	if (nodeIndex > 0 && nodeD.meshIndex < 0) {
		// nodeIndexでの変換行列を取得.
		const sxsdk::mat4 m = sceneData->getNodeMatrix(nodeIndex, true);

		if (nodeD.isBone) {
			// ボーンの行列を指定.
			compointer<sxsdk::bone_joint_interface> bone(part->get_bone_joint_interface());
			bone->set_matrix(m);

			scene->end_bone_joint();

		} else {
			scene->end_part();

			// パートの行列を指定.
			part->set_transformation_matrix(m);
		}
	}
}

/**
 * 指定のメッシュを生成.
 */
bool CGLTFImporterInterface::m_createGLTFMesh (const std::string& name, sxsdk::scene_interface *scene, CSceneData* sceneData, const int meshIndex, const sxsdk::mat4& matrix)
{
	CMeshData& meshD = sceneData->getMeshData(meshIndex);
	meshD.pMeshHandle = NULL;
	const size_t primitivesCou = meshD.primitives.size();

	// プリミティブを1つにマージする.
	CTempMeshData newMeshData;
	if (!meshD.mergePrimitives(newMeshData)) return false;
	newMeshData.name = name;

	const int faceGroupCou = (int)newMeshData.faceGroupMaterialIndex.size();

	bool errF = false;
	{
		// ポリゴンメッシュ形状を作成.
		sxsdk::polygon_mesh_class& pMesh = scene->begin_polygon_mesh(newMeshData.name.c_str());
		meshD.pMeshHandle = pMesh.get_handle();

		// 頂点座標を格納。.
		// GLTFではメートル単位であるので、Shade3Dのミリメートルに変換して渡している.
		const size_t versCou = newMeshData.vertices.size();
		const size_t triCou  = newMeshData.triangleIndices.size() / 3;
		{
			const float scale = 1000.0f;
			for (size_t i = 0; i < versCou; ++i) {
				const sxsdk::vec3 v =  (newMeshData.vertices[i] * scale) * matrix;
				scene->append_polygon_mesh_point(v);
			}
		}

		// 三角形の頂点インデックスを格納.
		{
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				const int i0 = newMeshData.triangleIndices[iPos + 0];
				const int i1 = newMeshData.triangleIndices[iPos + 1];
				const int i2 = newMeshData.triangleIndices[iPos + 2];
				if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= versCou || i1 >= versCou || i2 >= versCou) {
					errF = true;
					break;
				}

				scene->append_polygon_mesh_face(i0, i1, i2);
			}
		}

		scene->end_polygon_mesh();
		if (errF) return false;

		// 法線を読み込み（ベイク）.
		if (g_importParam.meshImportNormals) {
			sxsdk::vec3 normals[4];
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				sxsdk::face_class& f = pMesh.face(i);
				const int vCou = f.get_number_of_vertices();
				if (vCou != 3) continue;
				for (int j = 0; j < 3; ++j) normals[j] = newMeshData.triangleNormals[iPos + j];
				f.set_normals(vCou, normals);
			}
		}

		// UV0を格納.
		if (!newMeshData.triangleUV0.empty()) {
			const int uvIndex = pMesh.append_uv_layer();

			int triIndices[3];
			for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				sxsdk::face_class& f = pMesh.face(i);
				for (int j = 0; j < 3; ++j) {
					f.set_face_uv(uvIndex, j, newMeshData.triangleUV0[iPos + j]);
				}
			}
		}

		// UV1を格納.
		if (!newMeshData.triangleUV1.empty()) {
			const int uvIndex = pMesh.append_uv_layer();

			int triIndices[3];
			for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				sxsdk::face_class& f = pMesh.face(i);
				for (int j = 0; j < 3; ++j) {
					f.set_face_uv(uvIndex, j, newMeshData.triangleUV1[iPos + j]);
				}
			}
		}

		// 頂点カラーを格納.
		bool hasVertexColor = false;
		if (g_importParam.meshImportVertexColor) {
			if (!newMeshData.triangleColor0.empty()) {
				hasVertexColor = true;
				const int vLayerIndex = pMesh.append_vertex_color_layer();;
				sxsdk::vec4 col;
				for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
					sxsdk::face_class& f = pMesh.face(i);
					for (int j = 0; j < 3; ++j) {
						col = newMeshData.triangleColor0[iPos + j];
						f.set_vertex_color(vLayerIndex, j, sxsdk::rgba_class(col.x, col.y, col.z, col.w));
					}
				}
			}
		}

		// マテリアルを割り当て.
		if (faceGroupCou == 1) {
			const CMaterialData& materialD = sceneData->materials[newMeshData.faceGroupMaterialIndex[0]];
			if (materialD.shadeMasterSurface) {
				if (hasVertexColor) {		// 頂点カラーのマッピングレイヤを追加.
					Shade3DUtil::setVertexColorSurfaceLayer(materialD.shadeMasterSurface);
				}

				pMesh.set_master_surface(materialD.shadeMasterSurface);
			}
		} else {
			// フェイスグループ情報を追加.
			for (int i = 0; i < faceGroupCou; ++i) {
				const int facegroupIndex = pMesh.append_face_group();
				const CMaterialData& materialD = sceneData->materials[newMeshData.faceGroupMaterialIndex[i]];
				if (materialD.shadeMasterSurface) {
					if (hasVertexColor) {		// 頂点カラーのマッピングレイヤを追加.
						Shade3DUtil::setVertexColorSurfaceLayer(materialD.shadeMasterSurface);
					}

					pMesh.set_face_group_surface(facegroupIndex, materialD.shadeMasterSurface);
				}
			}

			// 面ごとにフェイスグループを割り当て.
			for (int i = 0; i < triCou; ++i) {
				pMesh.set_face_group_index(i, newMeshData.triangleFaceGroupIndex[i]);
			}
		}

		if (newMeshData.skinJoints.empty() && newMeshData.skinWeights.empty()) {
			// 重複頂点のマージ.
			pMesh.cleanup_redundant_vertices();

			// 稜線を生成.
			pMesh.make_edges();
		}

		// 限界角度の指定.
		pMesh.set_threshold(g_importParam.meshAngleThreshold);

		return true;
	}
}

/**
 * GLTFを読み込んだシーン情報より、マスターイメージを作成.
 */
void CGLTFImporterInterface::m_createGLTFImages (sxsdk::scene_interface *scene, CSceneData* sceneData)
{
	// 画像はいったん作業用フォルダ(m_tempPath)に出力してから、それを読み込むものとする.
	m_tempPath = std::string(shade.get_temporary_path("shade3d_temp_gltf"));

	try {
		std::vector<std::string> filesList;

		const size_t imagesCou = sceneData->images.size();
		for (size_t i = 0; i < imagesCou; ++i) {
			CImageData& imageD = sceneData->images[i];
			if (imageD.imageDatas.empty()) continue;

			const std::string name = (imageD.name == "") ? (std::string("image_") + std::to_string(i)) : imageD.name;

			// 画像ファイルを作業フォルダに出力.
			const std::string fileName = sceneData->outputTempImage(i, m_tempPath);
			if (fileName == "") continue;
			filesList.push_back(fileName);

			// Shade3Dのマスターサーフェスに画像を割り当て.
			sxsdk::master_image_class& masterImage = scene->create_master_image(name.c_str());
			masterImage.load_image(fileName.c_str());
			sxsdk::image_interface* image = masterImage.get_image();

			imageD.m_shadeMasterImage = &masterImage;
			imageD.width  = image->get_size().x;
			imageD.height = image->get_size().y;

			// ガンマの指定 (法線以外).
			if (!(imageD.imageMask & CImageData::gltf_image_mask_normal)) {
				if (g_importParam.gamma == 1) {
					masterImage.set_gamma(1.0f / 2.2f);
				}
			}
		}

		// 作業用に出力したファイルを削除.
		for (size_t i = 0; i < filesList.size(); ++i) {
			try {
				shade.delete_file(filesList[i].c_str());
			} catch (...) { }
		}

	} catch (...) { }
}

/**
 * GLTFを読み込んだシーン情報より、マスターサーフェスとしてマテリアルを作成.
 */
void CGLTFImporterInterface::m_createGLTFMaterials (sxsdk::scene_interface *scene, CSceneData* sceneData)
{
	try {
		const size_t materialsCou = sceneData->materials.size();
		for (size_t i = 0; i < materialsCou; ++i) {
			CMaterialData& materialD = sceneData->materials[i];
			const std::string name = (materialD.name == "") ? (std::string("material_") + std::to_string(i)) : materialD.name;

			sxsdk::master_surface_class& masterSurface = scene->create_master_surface(name.c_str());
			masterSurface.set_name(name.c_str());
			materialD.shadeMasterSurface = &masterSurface;

			sxsdk::surface_class* surface = masterSurface.get_surface();
			surface->set_diffuse_color(materialD.baseColorFactor);

			// 光沢を調整.
			surface->set_highlight(std::min(materialD.metallicFactor, 0.3f));
			surface->set_highlight_size(0.7f);

			// ALPHA_BLEND/ALPHA_MASK : アルファを考慮.
			const bool alphaBlend = (materialD.alphaMode == 2 || materialD.alphaMode == 3);

			// BaseColorを拡散反射のマッピングレイヤとして追加.
			if (materialD.baseColorImageIndex >= 0) {
				surface->append_mapping_layer();
				const int layerIndex = surface->get_number_of_mapping_layers() - 1;
				sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
				mLayer.set_pattern(sxsdk::enums::image_pattern);
				mLayer.set_type(sxsdk::enums::diffuse_mapping);

				// テクスチャ画像を割り当て.
				if (sceneData->images[materialD.baseColorImageIndex].m_shadeMasterImage) {
					compointer<sxsdk::image_interface> image(sceneData->images[materialD.baseColorImageIndex].m_shadeMasterImage->get_image());
					mLayer.set_image_interface(image);
				}

				mLayer.set_blur(true);
				mLayer.set_uv_mapping(materialD.baseColorTexCoord);
				mLayer.set_repetition_x(std::max(1, (int)materialD.baseColorTexScale.x));
				mLayer.set_repetition_y(std::max(1, (int)materialD.baseColorTexScale.y));

				// DiffuseのマッピングをAlpha透過にする.
				if (alphaBlend) {
					mLayer.set_channel_mix(sxsdk::enums::mapping_transparent_alpha_mode);
				}
			}

			// 法線マップをマッピングレイヤとして追加.
			if (materialD.normalImageIndex >= 0) {
				surface->append_mapping_layer();
				const int layerIndex = surface->get_number_of_mapping_layers() - 1;
				sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
				mLayer.set_pattern(sxsdk::enums::image_pattern);
				mLayer.set_type(sxsdk::enums::normal_mapping);

				// テクスチャ画像を割り当て.
				if (sceneData->images[materialD.normalImageIndex].m_shadeMasterImage) {
					compointer<sxsdk::image_interface> image(sceneData->images[materialD.normalImageIndex].m_shadeMasterImage->get_image());
					mLayer.set_image_interface(image);
				}

				mLayer.set_blur(true);
				mLayer.set_uv_mapping(materialD.normalTexCoord);
				mLayer.set_repetition_x(std::max(1, (int)materialD.normalTexScale.x));
				mLayer.set_repetition_y(std::max(1, (int)materialD.normalTexScale.y));
			}

			// 発光をマッピングレイヤとして追加.
			if (materialD.emissionImageIndex >= 0) {
				surface->set_glow_color(materialD.emissionFactor);
				surface->set_glow(1.0f);

				surface->append_mapping_layer();
				const int layerIndex = surface->get_number_of_mapping_layers() - 1;
				sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
				mLayer.set_pattern(sxsdk::enums::image_pattern);
				mLayer.set_type(sxsdk::enums::glow_mapping);

				// テクスチャ画像を割り当て.
				if (sceneData->images[materialD.emissionImageIndex].m_shadeMasterImage) {
					compointer<sxsdk::image_interface> image(sceneData->images[materialD.emissionImageIndex].m_shadeMasterImage->get_image());
					mLayer.set_image_interface(image);
				}

				mLayer.set_blur(true);
				mLayer.set_uv_mapping(materialD.emissionTexCoord);
				mLayer.set_repetition_x(std::max(1, (int)materialD.emissionTexScale.x));
				mLayer.set_repetition_y(std::max(1, (int)materialD.emissionTexScale.y));

			} else {
				if (MathUtil::isZero(materialD.emissionFactor)) {
					surface->set_glow(0.0f);
				} else {
					surface->set_glow_color(materialD.emissionFactor);
					surface->set_glow(1.0f);
				}
			}

			{
				surface->set_reflection(materialD.metallicFactor);
				surface->set_roughness(materialD.roughnessFactor);

				if (materialD.metallicRoughnessImageIndex >= 0) {
					const CImageData& imageD = sceneData->images[materialD.metallicRoughnessImageIndex];

					// 拡散反射にMetallicの反転を乗算.
					{
						surface->append_mapping_layer();
						const int layerIndex = surface->get_number_of_mapping_layers() - 1;
						sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
						mLayer.set_pattern(sxsdk::enums::image_pattern);
						mLayer.set_type(sxsdk::enums::diffuse_mapping);

						// テクスチャ画像を割り当て.
						if (imageD.m_shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.m_shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							// Metallicは[B]の要素を参照.
							mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_blue_mode);

							mLayer.set_blend_mode(7);		// 乗算合成.
							mLayer.set_flip_color(true);
							mLayer.set_weight(0.5f);
						}
						mLayer.set_blur(true);
						mLayer.set_uv_mapping(materialD.metallicRoughnessTexCoord);
						mLayer.set_repetition_x(std::max(1, (int)materialD.metallicRoughnessTexScale.x));
						mLayer.set_repetition_y(std::max(1, (int)materialD.metallicRoughnessTexScale.y));
					}

					// Metallicを「反射」要素としてマッピングレイヤに追加.
					{
						surface->append_mapping_layer();
						const int layerIndex = surface->get_number_of_mapping_layers() - 1;
						sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
						mLayer.set_pattern(sxsdk::enums::image_pattern);
						mLayer.set_type(sxsdk::enums::reflection_mapping);

						// テクスチャ画像を割り当て.
						if (imageD.m_shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.m_shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							// Metallicは[B]の要素を参照.
							mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_blue_mode);
						}
						mLayer.set_blur(true);
						mLayer.set_uv_mapping(materialD.metallicRoughnessTexCoord);
						mLayer.set_repetition_x(std::max(1, (int)materialD.metallicRoughnessTexScale.x));
						mLayer.set_repetition_y(std::max(1, (int)materialD.metallicRoughnessTexScale.y));
					}

					// BaseColorを「反射」に乗算.
					{
						surface->append_mapping_layer();
						const int layerIndex = surface->get_number_of_mapping_layers() - 1;
						sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
						mLayer.set_pattern(sxsdk::enums::image_pattern);
						mLayer.set_type(sxsdk::enums::reflection_mapping);

						const CImageData& imageBaseColorD = sceneData->images[materialD.baseColorImageIndex];

						// テクスチャ画像を割り当て.
						if (imageBaseColorD.m_shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageBaseColorD.m_shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							mLayer.set_blend_mode(7);		// 乗算合成.
							mLayer.set_weight(1.0f);
						}
						mLayer.set_blur(true);
						mLayer.set_uv_mapping(materialD.baseColorTexCoord);
						mLayer.set_repetition_x(std::max(1, (int)materialD.baseColorTexScale.x));
						mLayer.set_repetition_y(std::max(1, (int)materialD.baseColorTexScale.y));
					}

					// Roughnessを「荒さ」要素としてマッピングレイヤに追加.
					{
						surface->append_mapping_layer();
						const int layerIndex = surface->get_number_of_mapping_layers() - 1;
						sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
						mLayer.set_pattern(sxsdk::enums::image_pattern);
						mLayer.set_type(sxsdk::enums::roughness_mapping);

						// テクスチャ画像を割り当て.
						if (imageD.m_shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.m_shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							// Roughnessは[G]の要素を参照.
							mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_green_mode);
							mLayer.set_flip_color(true);		// RoughnessはShade3Dのマッピングレイヤでは、黒に近づくにつれて粗くなる.
						}
						mLayer.set_blur(true);
						mLayer.set_uv_mapping(materialD.metallicRoughnessTexCoord);
						mLayer.set_repetition_x(std::max(1, (int)materialD.metallicRoughnessTexScale.x));
						mLayer.set_repetition_y(std::max(1, (int)materialD.metallicRoughnessTexScale.y));
					}

					// Occlusionを「拡散反射」の乗算としてマッピングレイヤに追加.
					if (imageD.imageMask & CImageData::gltf_image_mask_occlusion) {
						surface->append_mapping_layer();
						const int layerIndex = surface->get_number_of_mapping_layers() - 1;
						sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
						mLayer.set_pattern(sxsdk::enums::image_pattern);
						mLayer.set_type(sxsdk::enums::diffuse_mapping);

						// テクスチャ画像を割り当て.
						if (imageD.m_shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.m_shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							// Occlusionは[R]の要素を参照.
							mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_red_mode);

							mLayer.set_blend_mode(7);		// 乗算合成.
						}
						mLayer.set_blur(true);
						mLayer.set_weight(materialD.occlusionStrength);
						mLayer.set_uv_mapping(materialD.metallicRoughnessTexCoord);
						mLayer.set_repetition_x(std::max(1, (int)materialD.metallicRoughnessTexScale.x));
						mLayer.set_repetition_y(std::max(1, (int)materialD.metallicRoughnessTexScale.y));
					}
				}
			}

			// Occlusionをマッピングレイヤとして追加.
			if (materialD.occlusionImageIndex >= 0) {
				if (materialD.occlusionImageIndex != materialD.metallicRoughnessImageIndex) {
					surface->append_mapping_layer();
					const int layerIndex = surface->get_number_of_mapping_layers() - 1;
					sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
					mLayer.set_pattern(sxsdk::enums::image_pattern);
					mLayer.set_type(sxsdk::enums::diffuse_mapping);

					// テクスチャ画像を割り当て.
					if (sceneData->images[materialD.occlusionImageIndex].m_shadeMasterImage) {
						compointer<sxsdk::image_interface> image(sceneData->images[materialD.occlusionImageIndex].m_shadeMasterImage->get_image());
						mLayer.set_image_interface(image);

						mLayer.set_blend_mode(7);		// 乗算合成.
					}

					mLayer.set_blur(true);
					mLayer.set_weight(materialD.occlusionStrength);
					mLayer.set_uv_mapping(materialD.occlusionTexCoord);
					mLayer.set_repetition_x(std::max(1, (int)materialD.occlusionTexScale.x));
					mLayer.set_repetition_y(std::max(1, (int)materialD.occlusionTexScale.y));
				}
			}

			masterSurface.update();
		}

	} catch (...) { }
}

/**
 * 指定のMesh形状に対して、スキン情報を割り当て.
 */
void CGLTFImporterInterface::m_setMeshSkins (sxsdk::scene_interface *scene, CSceneData* sceneData)
{
	const size_t skinsCou = sceneData->skins.size();
	const size_t meshsCou = sceneData->meshes.size();
	if (skinsCou == 0) return;

	for (size_t meshLoop = 0; meshLoop < meshsCou; ++meshLoop) {
		const CMeshData& meshD = sceneData->meshes[meshLoop];
		if (!meshD.pMeshHandle) continue;
		sxsdk::shape_class* meshShape = scene->get_shape_by_handle(meshD.pMeshHandle);
		if (!meshShape) continue;
		if (meshShape->get_type() != sxsdk::enums::polygon_mesh) continue;
		sxsdk::polygon_mesh_class& pMesh = meshShape->get_polygon_mesh();

		// meshLoopに対応するノード番号を取得.
		const int nodeIndex = m_findNodeFromMeshIndex(sceneData, (int)meshLoop);
		if (nodeIndex < 0) continue;

		const CNodeData& nodeD = sceneData->nodes[nodeIndex];
		if (nodeD.skinIndex < 0) continue;
		const CSkinData& skinD = sceneData->skins[nodeD.skinIndex];

		// meshD.pMeshのポリゴンメッシュで使用しているスキンを登録.
		pMesh.set_skin_type(1);		// 頂点ブレンドの指定.
		std::vector<sxsdk::shape_class *> jointParts;
		std::vector<std::string> jointNames;
		jointParts.resize(skinD.joints.size(), NULL);
		jointNames.resize(skinD.joints.size(), "");
		for (size_t i = 0; i < skinD.joints.size(); ++i) {
			const int jointNodeIndex = skinD.joints[i];
			const CNodeData& jointNodeD = sceneData->nodes[jointNodeIndex + 1];		// ノードの0番目はルートとして新たに追加した要素なので、+1している.
			if (jointNodeD.pShapeHandle) {
				jointParts[i] = scene->get_shape_by_handle(jointNodeD.pShapeHandle);
				jointNames[i] = jointParts[i]->get_name();
				pMesh.append_skin(jointParts[i]->get_part());
			}
		}

		// プリミティブを1つにマージする.
		CTempMeshData newMeshData;
		if (!meshD.mergePrimitives(newMeshData)) continue;
		if (newMeshData.skinJoints.empty() || newMeshData.skinWeights.empty()) continue;

		// meshD.pMeshのポリゴンメッシュに対してスキンのバインドとウエイト値を指定.
		const size_t versCou = newMeshData.vertices.size();
		for (size_t i = 0; i < versCou; ++i) {
			const sxsdk::vec4& skinWeight   = newMeshData.skinWeights[i];
			const sx::vec<int,4>& skinJoint = newMeshData.skinJoints[i];

			sxsdk::vertex_class& verC = pMesh.vertex(i);
			sxsdk::skin_class& skinC  = verC.get_skin();
			int sCou = 0;
			for (int j = 3; j >= 0; --j) {
				if (skinWeight[j] > 0.0f) {
					sCou = j + 1;
					break;
				}
			}
			for (int j = 0; j < sCou; ++j) {
				skinC.append_bind();
				sxsdk::skin_bind_class& skinBindC = skinC.get_bind(j);
				skinBindC.set_shape(jointParts[ skinJoint[j] ]);
				skinBindC.set_weight(skinWeight[j]);
			}
		}

		// 重複頂点のマージ.
		pMesh.cleanup_redundant_vertices();

		// 稜線を生成.
		pMesh.make_edges();

		pMesh.update_skin_bindings();
	}
}

/**
 * メッシュ番号を参照しているノードを取得.
 * @return ノード番号.
 */
int CGLTFImporterInterface::m_findNodeFromMeshIndex (CSceneData* sceneData, const int meshIndex)
{
	int nodeIndex = -1;
	const size_t nodesCou = sceneData->nodes.size();
	for (size_t i = 0; i < nodesCou; ++i) {
		const CNodeData& nodeD = sceneData->nodes[i];
		if (nodeD.meshIndex == meshIndex) {
			nodeIndex = (int)i;
			break;
		}
	}
	return nodeIndex;
}

/**
 * 読み込んだ形状のルートから調べて、ボーンの場合に向きとボーンサイズを自動調整.
 */
void CGLTFImporterInterface::m_adjustBones (sxsdk::shape_class* shape)
{
	// ルートボーンを取得.
	std::vector<sxsdk::shape_class *> rootBones;
	const int rootShapesCou = Shade3DUtil::findBoneRoot(shape, rootBones);

	for (int i = 0; i < rootShapesCou; ++i) {
		// ボーンの向きをそろえる.
		Shade3DUtil::adjustBonesDirection(rootBones[i]);

		// ボーンサイズを自動調整.
		Shade3DUtil::resizeBones(rootBones[i]);
	}
}

/**
 * アニメーション情報を割り当て.
 */
void CGLTFImporterInterface::m_setAnimations (sxsdk::scene_interface *scene, CSceneData* sceneData)
{
	const size_t animCou = sceneData->animations.channelData.size();
	for (size_t loop = 0; loop < animCou; ++loop) {
		const CAnimChannelData& channelD = sceneData->animations.channelData[loop];
		if (channelD.samplerIndex < 0 || channelD.targetNodeIndex < 0) continue;
		const CAnimSamplerData& samplerD = sceneData->animations.samplerData[channelD.samplerIndex];

		const int frameRate = scene->get_frame_rate();

		try {
			// Shade3Dでの対象形状を取得.
			const CNodeData& nodeD = sceneData->nodes[channelD.targetNodeIndex + 1];	// ルートノードは別途追加したものなので+1している.
			if (!nodeD.pShapeHandle) continue;
			sxsdk::shape_class* shape = scene->get_shape_by_handle(nodeD.pShapeHandle);
			if (!shape) continue;

			const sxsdk::vec3 boneWCenter = Shade3DUtil::getBoneCenter(*shape, NULL);
			const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
			const sxsdk::vec3 boneLCenter = (boneWCenter * inv(lwMat));

			compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());

			const size_t sCou = samplerD.inputData.size();
			int iPos = 0;
			for (size_t i = 0; i < sCou; ++i) {
				const float fPos = samplerD.inputData[i];		// 秒単位のキーフレーム位置.
				const float keyFramePos = fPos * (float)frameRate;

				// すでにキーフレームが存在するか.
				int motionPointIndex = Shade3DUtil::findMotionPoint(motion, keyFramePos);
				if (motionPointIndex < 0) {
					motionPointIndex = motion->create_motion_point(keyFramePos);
				}

				compointer<sxsdk::motion_point_interface> motionP(motion->get_motion_point_interface(motionPointIndex));

				if (channelD.pathType == CAnimChannelData::path_type_translation) {
					// オフセット値を取得し、メートルからミリメートルに変換.
					sxsdk::vec3 offsetV = sxsdk::vec3(samplerD.outputData[iPos + 0], samplerD.outputData[iPos + 1], samplerD.outputData[iPos + 2]) * 1000.0f;
					offsetV -= boneLCenter;

					motionP->set_offset(offsetV);
					iPos += 3;

				} else if (channelD.pathType == CAnimChannelData::path_type_rotation) {
					sxsdk::quaternion_class q = sxsdk::quaternion_class(-samplerD.outputData[iPos + 3], samplerD.outputData[iPos + 0], samplerD.outputData[iPos + 1], samplerD.outputData[iPos + 2]);
					motionP->set_rotation(q);
					iPos += 4;

				} else {
					break;
				}
			}

		} catch (...) { }
	}
}
