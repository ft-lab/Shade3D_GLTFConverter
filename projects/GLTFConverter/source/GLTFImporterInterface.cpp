/**
 * GLTFファイルを読み込むインポータクラス.
 */
#include "GLTFImporterInterface.h"
#include "GLTFLoader.h"
#include "SceneData.h"
#include "Shade3DUtil.h"
#include "MathUtil.h"
#include "StreamCtrl.h"
#include "StringUtil.h"
#include "MotionExternalAccess.h"

enum
{
	dlg_gamma_id = 101,							// ガンマ値.
	dlg_import_animation_id = 102,				// アニメーションの読み込み.
	dlg_convert_color_from_linear = 103,		// 色をリニアから変換.
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
	m_MorphTargetsAccess = NULL;
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
 * 扱うファイル拡張子数.
 */
int CGLTFImporterInterface::get_number_of_file_extensions (void *)
{
	return 4;
}

/**
 * @brief  ファイル拡張子.
 */
const char *CGLTFImporterInterface::get_file_extension (int index, void *)
{
	if (index == 0) return "glb|gltf";
	if (index == 1) return "glb";
	if (index == 2) return "gltf";
	if (index == 3) return "vrm";

	return 0;
}

/**
 * @brief ファイル詳細.
 */
const char *CGLTFImporterInterface::get_file_extension_description (int index, void*)
{
	if (index == 0) return "glTF";
	if (index == 1) return "glTF";
	if (index == 2) return "glTF";
	if (index == 3) return "vrm";

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
	// streamに、ダイアログボックスのパラメータを保存.
	StreamCtrl::saveImportDialogParam(shade, g_importParam);

	// ファイル名を取得.
	std::string fileName = "";
	if (text_stream) {
		fileName = std::string(text_stream->get_name());
	}
	if (fileName == "" && stream) {
		fileName = std::string(stream->get_name());
	}
	if (fileName == "") return;

	// Morph Targets情報アクセスクラス.
	m_MorphTargetsAccess = getMorphTargetsAttributeAccess(shade);

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

	if (sceneData.isVRM) {
		// VRMの場合のライセンス情報などを表示.
		shade.message(std::string("VRM : "));
		shade.message(std::string("    exporter version : ") + sceneData.licenseData.exporterVersion);
		shade.message(std::string("    version : ") + sceneData.licenseData.version);
		shade.message(std::string("    author : ") + sceneData.licenseData.author);
		shade.message(std::string("    contact information : ") + sceneData.licenseData.contactInformation);
		shade.message(std::string("    reference : ") + sceneData.licenseData.reference);
		shade.message(std::string("    title : ") + sceneData.licenseData.title);
		shade.message(std::string("    allowed user name : ") + sceneData.licenseData.allowedUserName);
		shade.message(std::string("    violent ussage name : ") + sceneData.licenseData.violentUssageName);
		shade.message(std::string("    sexual ussage name : ") + sceneData.licenseData.sexualUssageName);
		shade.message(std::string("    commercial ussage name : ") + sceneData.licenseData.commercialUssageName);
		shade.message(std::string("    other permission url : ") + sceneData.licenseData.otherPermissionUrl);
		shade.message(std::string("    license name : ") + sceneData.licenseData.licenseName);
		shade.message(std::string("    other license url : ") + sceneData.licenseData.otherLicenseUrl);

	} else {
		// Oculus Homeでのライセンス情報.
		shade.message(std::string("Asset title : ") + sceneData.assetExtrasTitle);
		shade.message(std::string("Asset author : ") + sceneData.assetExtrasAuthor);
		shade.message(std::string("Asset source : ") + sceneData.assetExtrasSource);
		shade.message(std::string("Asset license : ") + sceneData.assetExtrasLicense);
	}
	shade.message("");

	shade.message(std::string("Meshes : ") + std::to_string(sceneData.meshes.size()));
	shade.message(std::string("Materials : ") + std::to_string(sceneData.materials.size()));
	shade.message(std::string("Images : ") + std::to_string(sceneData.images.size()));
	shade.message(std::string("Skins : ") + std::to_string(sceneData.skins.size()));
	shade.message(std::string("Animations : ") + std::to_string(sceneData.animations.hasAnimation() ? 1 : 0));

	// シーン情報を構築.
	m_createGLTFScene(scene, &sceneData);

	m_MorphTargetsAccess = NULL;
}

/****************************************************************/
/* ダイアログイベント											*/
/****************************************************************/
void CGLTFImporterInterface::initialize_dialog (sxsdk::dialog_interface& dialog, void*)
{
	// streamより、ダイアログボックスのパラメータを取得.
	StreamCtrl::loadImportDialogParam(shade, g_importParam);
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

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_convert_color_from_linear));
		item->set_bool(g_importParam.convertColorFromLinear);
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

	if (id == dlg_convert_color_from_linear) {
		g_importParam.convertColorFromLinear = item.get_bool();
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

	// VRMとしてのライセンス情報をstreamに保持.
	if (sceneData->isVRM) {
		StreamCtrl::saveLicenseData(rootPart, sceneData->licenseData);
	}
}

/**
 * GLTFを読み込んだシーン情報より、ノードを再帰的にたどって階層構造を構築.
 */
void CGLTFImporterInterface::m_createGLTFNodeHierarchy (sxsdk::scene_interface *scene, CSceneData* sceneData, const int nodeIndex)
{
	CNodeData& nodeD = sceneData->nodes[nodeIndex];
	const float bone_r = 10.0f;

	// パートもしくはボーンとして開始.
	sxsdk::part_class* part = NULL;
	nodeD.pShapeHandle = NULL;
	if (nodeIndex > 0 && nodeD.meshIndex < 0) {
		if (nodeD.isBone) {
			const sxsdk::vec3 bonePos(0, 0, 0);
			const sxsdk::vec3 boneAxisDir(1, 0, 0);
			part = &(scene->begin_bone_joint(bonePos, bone_r, false, boneAxisDir, nodeD.name.c_str()));

		} else if (nodeD.hasAnimation) {
			// 単独のTransform animationの場合、変換行列に回転を入れる場合もある.
			// このとき、もしボールジョイントを使用する場合は回転要素は無効にする処理が行われている.
			// そのため、ここではボーンを使用した.
			const sxsdk::vec3 bonePos(0, 0, 0);
			const sxsdk::vec3 boneAxisDir(1, 0, 0);
			part = &(scene->begin_bone_joint(bonePos, bone_r, false, boneAxisDir, nodeD.name.c_str()));

		} else {
			part = &(scene->begin_part(nodeD.name.c_str()));
		}
		nodeD.pShapeHandle = part->get_handle();
	}

	if (nodeIndex > 0 && nodeD.meshIndex >= 0) {
		sxsdk::part_class* part2 = NULL;
		if (nodeD.hasAnimation) {		// 単一メッシュでアニメーションを持つ場合、ボーンを作成.
			const sxsdk::vec3 bonePos(0, 0, 0);
			const sxsdk::vec3 boneAxisDir(1, 0, 0);
			part2 = &(scene->begin_bone_joint(bonePos, bone_r, false, boneAxisDir, nodeD.name.c_str()));
		}

		// メッシュを生成.
		sxsdk::mat4 m = (nodeD.childNodeIndex >= 0) ? sxsdk::mat4::identity : (sceneData->getNodeMatrix(nodeIndex));
		m_createGLTFMesh(nodeD.name, scene, sceneData, nodeD.meshIndex, m);

		if (part2) {
			scene->end_bone_joint();
			nodeD.pShapeHandle = part2->get_handle();
		}
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

		} else if (nodeD.hasAnimation) {
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
		std::vector<int> triIndexList;
		triIndexList.resize(triCou, -1);
		{
			int index = 0;
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				const int i0 = newMeshData.triangleIndices[iPos + 0];
				const int i1 = newMeshData.triangleIndices[iPos + 1];
				const int i2 = newMeshData.triangleIndices[iPos + 2];
				if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= versCou || i1 >= versCou || i2 >= versCou) {
					errF = true;
					break;
				}
				if (i0 == i1 || i1 == i2 || i0 == i2) continue;
				triIndexList[i] = index;

				scene->append_polygon_mesh_face(i0, i1, i2);
				index++;
			}
		}
		scene->end_polygon_mesh();

		if (errF) return false;

		// 法線を読み込み（ベイク）.
		if (g_importParam.meshImportNormals) {
			sxsdk::vec3 normals[4];
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				if (triIndexList[i] < 0) continue;
				try {
					sxsdk::face_class& f = pMesh.face(triIndexList[i]);
					const int vCou = f.get_number_of_vertices();
					if (vCou != 3) continue;
					for (int j = 0; j < 3; ++j) normals[j] = newMeshData.triangleNormals[iPos + j];
					f.set_normals(vCou, normals);
				} catch (...) { }
			}
		}

		// UV0を格納.
		if (!newMeshData.triangleUV0.empty()) {
			const int uvIndex = pMesh.append_uv_layer();

			int triIndices[3];
			for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				if (triIndexList[i] < 0) continue;
				try {
					sxsdk::face_class& f = pMesh.face(triIndexList[i]);
					for (int j = 0; j < 3; ++j) {
						f.set_face_uv(uvIndex, j, newMeshData.triangleUV0[iPos + j]);
					}
				} catch (...) { }
			}
		}

		// UV1を格納.
		if (!newMeshData.triangleUV1.empty()) {
			const int uvIndex = pMesh.append_uv_layer();

			int triIndices[3];
			for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				if (triIndexList[i] < 0) continue;
				try {
					sxsdk::face_class& f = pMesh.face(triIndexList[i]);
					for (int j = 0; j < 3; ++j) {
						f.set_face_uv(uvIndex, j, newMeshData.triangleUV1[iPos + j]);
					}
				} catch (...) { }
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
					if (triIndexList[i] < 0) continue;
					try {
						sxsdk::face_class& f = pMesh.face(triIndexList[i]);
						for (int j = 0; j < 3; ++j) {
							col = newMeshData.triangleColor0[iPos + j];
							// ノンリニアに変換.
							if (g_importParam.convertColorFromLinear) {
								MathUtil::convColorNonLinear(col.x, col.y, col.z);
							}
							f.set_vertex_color(vLayerIndex, j, sxsdk::rgba_class(col.x, col.y, col.z, col.w));
						}
					} catch (...) { }
				}
			}
		}

		// マテリアルを割り当て.
		if (faceGroupCou == 1) {
			if (newMeshData.faceGroupMaterialIndex[0] >= 0 && newMeshData.faceGroupMaterialIndex[0] < sceneData->materials.size()) {
				const CMaterialData& materialD = sceneData->materials[newMeshData.faceGroupMaterialIndex[0]];
				if (materialD.shadeMasterSurface) {
					if (hasVertexColor) {		// 頂点カラーのマッピングレイヤを追加.
						Shade3DUtil::setVertexColorSurfaceLayer(materialD.shadeMasterSurface);
					}

					pMesh.set_master_surface(materialD.shadeMasterSurface);
				}
			}
		} else {
			// フェイスグループ情報を追加.
			for (int i = 0; i < faceGroupCou; ++i) {
				const int facegroupIndex = pMesh.append_face_group();
				if (newMeshData.faceGroupMaterialIndex[i] >= 0 && newMeshData.faceGroupMaterialIndex[i] < sceneData->materials.size()) {
					const CMaterialData& materialD = sceneData->materials[newMeshData.faceGroupMaterialIndex[i]];
					if (materialD.shadeMasterSurface) {
						if (hasVertexColor) {		// 頂点カラーのマッピングレイヤを追加.
							Shade3DUtil::setVertexColorSurfaceLayer(materialD.shadeMasterSurface);
						}

						pMesh.set_face_group_surface(facegroupIndex, materialD.shadeMasterSurface);
					}
				}
			}

			// 面ごとにフェイスグループを割り当て.
			for (int i = 0; i < triCou; ++i) {
				if (triIndexList[i] < 0) continue;
				pMesh.set_face_group_index(triIndexList[i], newMeshData.triangleFaceGroupIndex[i]);
			}
		}

		// Morph Targets情報を割り当て.
		// これは、「MotionUtil」プラグインに情報を渡している.
		if (!newMeshData.morphTargets.morphTargetsData.empty() && m_MorphTargetsAccess) {
			const int versCou = pMesh.get_total_number_of_control_points();

			// 指定の形状を初期値として渡す.
			m_MorphTargetsAccess->setupShape(&pMesh);

			sxsdk::polygon_mesh_saver_class* pMeshSaver = pMesh.get_polygon_mesh_saver();

			// Targetごとの頂点番号/頂点座標を渡す.
			const float scale = 1000.0f;
			const int targetsCou = (int)newMeshData.morphTargets.morphTargetsData.size();
			for (int i = 0; i < targetsCou; ++i) {
				const COneMorphTargetData& morphD = newMeshData.morphTargets.morphTargetsData[i];
				const std::string name = (morphD.name == "") ? std::string("target ") + std::to_string(i) : morphD.name;
				std::vector<sxsdk::vec3> posList;
				std::vector<int> vIndices;

				for (size_t j = 0; j < morphD.vIndices.size(); ++j) {
					const int vIndex = morphD.vIndices[j];
					if (vIndex >= 0 && vIndex < versCou) {
						posList.push_back((morphD.position[j] * scale) + (pMeshSaver->get_point(vIndex)));
						vIndices.push_back(vIndex);
					}
				}
				if (!vIndices.empty()) {
					const int tIndex = m_MorphTargetsAccess->appendTargetVertices(name.c_str(), (int)vIndices.size(), &(vIndices[0]), &(posList[0]));
					m_MorphTargetsAccess->setTargetWeight(tIndex, morphD.weight);
				}
			}
			pMeshSaver->release();

			// Morph Targets情報をstreamに保存.
			m_MorphTargetsAccess->writeMorphTargetsData();
			m_MorphTargetsAccess->updateMesh();
		}

		// スキン時の重複頂点のマージはスキンの処理を行うm_setMeshSkin関数で行うため、
		// ここではスキップ.
		if (newMeshData.skinJoints.empty() && newMeshData.skinWeights.empty()) {
			// 重複頂点のマージ.
			m_cleanupRedundantVertices(pMesh, &newMeshData);
		}
		// 稜線を生成.
		pMesh.make_edges();

		// 限界角度の指定.
		pMesh.set_threshold(g_importParam.meshAngleThreshold);

		return true;
	}
}

/**
 * 重複頂点をマージする.
 * Morph Targetsの頂点もマージ対象.
 */
void CGLTFImporterInterface::m_cleanupRedundantVertices (sxsdk::polygon_mesh_class& pMesh, const CTempMeshData* tempMeshData)
{
	// Morph Targetsの情報も更新.
	if (m_MorphTargetsAccess) {
		if (!tempMeshData->morphTargets.morphTargetsData.empty()) {
			m_MorphTargetsAccess->cleanupRedundantVertices(pMesh);
			m_MorphTargetsAccess->writeMorphTargetsData();
			return;
		}
	}

	pMesh.cleanup_redundant_vertices();
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

			imageD.shadeMasterImage = &masterImage;
			imageD.width  = image->get_size().x;
			imageD.height = image->get_size().y;

			// ガンマの指定 (BaseColor/Emissiveのみ).
			if ((imageD.imageMask & CImageData::gltf_image_mask_base_color) || (imageD.imageMask & CImageData::gltf_image_mask_emissive)) {
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

			const std::string materialName = materialD.name;

			std::string name = (materialName == "") ? (std::string("material_") + std::to_string(i)) : materialName;
			if (materialD.doubleSided) {	// doubleSidedの場合は、マスターサーフェス名に「doubleSided」をつける.
				std::string name2 = name;
				std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);
				const int iPos = name2.find("doublesided");
				if (iPos == std::string::npos) {
					name += "_doubleSided";
				}
			}

			sxsdk::master_surface_class& masterSurface = scene->create_master_surface(name.c_str());
			masterSurface.set_name(name.c_str());
			materialD.shadeMasterSurface = &masterSurface;

			sxsdk::surface_class* surface = masterSurface.get_surface();

			// 光沢を調整.
			float highlightV = std::min(materialD.metallicFactor, 0.3f);
			if (materialD.roughnessFactor < 1.0f) {
				highlightV = std::max(materialD.roughnessFactor * 0.5f, highlightV);
			}
			surface->set_highlight(highlightV);
			surface->set_highlight_size(0.7f);

			// ALPHA_MASK : アルファを考慮.
			const bool alphaMask = (materialD.alphaMode == 3);

			// ALPHA_BLEND : 透明度を考慮.
			const bool alphaBlend = (materialD.alphaMode == 2);

			// AlphaMask情報をStreamに保持.
			{
				CAlphaModeMaterialData aData;
				aData.alphaCutoff   = materialD.alphaCutOff;
				if (alphaMask) aData.alphaModeType = GLTFConverter::alpha_mode_mask;
				if (alphaBlend) aData.alphaModeType = GLTFConverter::alpha_mode_blend;
				StreamCtrl::saveAlphaModeMaterialParam(surface, aData);
			}

			bool needAlpha = false;
			const float transparency = alphaBlend ? materialD.transparency : 0.0f;
			surface->set_transparency(transparency);

			// 陰影付けなしの指定.
			if (materialD.unlit) {
				surface->set_no_shading(true);
			}

			// KHR_materials_pbrSpecularGlossinessの読み込み.
			if (materialD.pbrSpecularGlossiness_use) {
				{
					sxsdk::rgb_class col;
					col.red   = materialD.pbrSpecularGlossiness_diffuseFactor.red;
					col.green = materialD.pbrSpecularGlossiness_diffuseFactor.green;
					col.blue  = materialD.pbrSpecularGlossiness_diffuseFactor.blue;

					// ノンリニアに変換.
					if (g_importParam.convertColorFromLinear) {
						MathUtil::convColorNonLinear(col.red, col.green, col.blue);
					}
					surface->set_diffuse_color(col);
				}

				{
					surface->set_roughness(1.0f - materialD.pbrSpecularGlossiness_glossinessFactor);
				}

				if (materialD.pbrSpecularGlossiness_diffuseImageIndex >= 0) {
					surface->append_mapping_layer();
					const int layerIndex = surface->get_number_of_mapping_layers() - 1;
					sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
					mLayer.set_pattern(sxsdk::enums::image_pattern);
					mLayer.set_type(sxsdk::enums::diffuse_mapping);

					// テクスチャ画像を割り当て.
					if (sceneData->images[materialD.pbrSpecularGlossiness_diffuseImageIndex].shadeMasterImage) {
						compointer<sxsdk::image_interface> image(sceneData->images[materialD.pbrSpecularGlossiness_diffuseImageIndex].shadeMasterImage->get_image());
						mLayer.set_image_interface(image);

						// ALPHA_BLENDのときに、イメージのAlpha要素で透過がある場合.
						if (alphaBlend) {
							if (Shade3DUtil::hasImageAlpha(sceneData->images[materialD.pbrSpecularGlossiness_diffuseImageIndex].shadeMasterImage)) {
								needAlpha = true;
							}
						}
					}

					mLayer.set_blend_mode(7);		// 乗算合成.
					mLayer.set_blur(true);

					// DiffuseのマッピングをAlpha透過にする.
					if (alphaMask || needAlpha) {
						mLayer.set_channel_mix(sxsdk::enums::mapping_transparent_alpha_mode);
					}
				}
			}

			// BaseColorを拡散反射のマッピングレイヤとして追加.
			if (!materialD.pbrSpecularGlossiness_use) {
				if (materialD.baseColorImageIndex >= 0) {
					surface->append_mapping_layer();
					const int layerIndex = surface->get_number_of_mapping_layers() - 1;
					sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
					mLayer.set_pattern(sxsdk::enums::image_pattern);
					mLayer.set_type(sxsdk::enums::diffuse_mapping);

					// テクスチャ画像を割り当て.
					if (sceneData->images[materialD.baseColorImageIndex].shadeMasterImage) {
						compointer<sxsdk::image_interface> image(sceneData->images[materialD.baseColorImageIndex].shadeMasterImage->get_image());
						mLayer.set_image_interface(image);

						// ALPHA_BLENDのときに、イメージのAlpha要素で透過がある場合.
						if (alphaBlend) {
							if (Shade3DUtil::hasImageAlpha(sceneData->images[materialD.baseColorImageIndex].shadeMasterImage)) {
								needAlpha = true;
							}
						}
					}

					mLayer.set_blend_mode(7);		// 乗算合成.
					mLayer.set_blur(true);
					mLayer.set_uv_mapping(materialD.baseColorTexCoord);
					mLayer.set_repetition_x(std::max(1, (int)materialD.baseColorTexScale.x));
					mLayer.set_repetition_y(std::max(1, (int)materialD.baseColorTexScale.y));

					// DiffuseのマッピングをAlpha透過にする.
					if (alphaMask || needAlpha) {
						mLayer.set_channel_mix(sxsdk::enums::mapping_transparent_alpha_mode);
					}
				}
			
				// Shade3DでのDiffuseを黒にしないと反射に透明感が出ないので補正.
				{
					const sxsdk::rgb_class whiteCol(1, 1, 1);
					sxsdk::rgb_class col = materialD.baseColorFactor;

					// ノンリニアに変換.
					if (g_importParam.convertColorFromLinear) {
						MathUtil::convColorNonLinear(col.red, col.green, col.blue);
					}

					const float metallicV  = materialD.metallicFactor;
					const float metallicV2 = 1.0f - metallicV;
					const sxsdk::rgb_class reflectionCol = col * metallicV + whiteCol * metallicV2;

					surface->set_diffuse_color(col);
					if (materialD.metallicRoughnessImageIndex < 0) {		// MetallicRoughnessのイメージを持たない場合.
						surface->set_diffuse(metallicV2);
						surface->set_reflection_color(reflectionCol);
					}
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
				if (sceneData->images[materialD.normalImageIndex].shadeMasterImage) {
					compointer<sxsdk::image_interface> image(sceneData->images[materialD.normalImageIndex].shadeMasterImage->get_image());
					mLayer.set_image_interface(image);
				}

				mLayer.set_blur(true);
				mLayer.set_uv_mapping(materialD.normalTexCoord);
				mLayer.set_repetition_x(std::max(1, (int)materialD.normalTexScale.x));
				mLayer.set_repetition_y(std::max(1, (int)materialD.normalTexScale.y));
				mLayer.set_weight(materialD.normalStrength);
			}

			// 発光をマッピングレイヤとして追加.
			if (materialD.emissiveImageIndex >= 0) {
				// ノンリニアに変換.
				sxsdk::rgb_class eCol = materialD.emissiveFactor;
				if (g_importParam.convertColorFromLinear) {
					MathUtil::convColorNonLinear(eCol.red, eCol.green, eCol.blue);
				}
				surface->set_glow_color(eCol);
				surface->set_glow(1.0f);

				surface->append_mapping_layer();
				const int layerIndex = surface->get_number_of_mapping_layers() - 1;
				sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
				mLayer.set_pattern(sxsdk::enums::image_pattern);
				mLayer.set_type(sxsdk::enums::glow_mapping);

				// テクスチャ画像を割り当て.
				if (sceneData->images[materialD.emissiveImageIndex].shadeMasterImage) {
					compointer<sxsdk::image_interface> image(sceneData->images[materialD.emissiveImageIndex].shadeMasterImage->get_image());
					mLayer.set_image_interface(image);
				}

				mLayer.set_blur(true);
				mLayer.set_uv_mapping(materialD.emissiveTexCoord);
				mLayer.set_repetition_x(std::max(1, (int)materialD.emissiveTexScale.x));
				mLayer.set_repetition_y(std::max(1, (int)materialD.emissiveTexScale.y));

			} else {
				if (MathUtil::isZero(materialD.emissiveFactor)) {
					surface->set_glow(0.0f);
				} else {
					// ノンリニアに変換.
					sxsdk::rgb_class eCol = materialD.emissiveFactor;
					if (g_importParam.convertColorFromLinear) {
						MathUtil::convColorNonLinear(eCol.red, eCol.green, eCol.blue);
					}

					surface->set_glow_color(eCol);
					surface->set_glow(1.0f);
				}
			}

			if (!materialD.pbrSpecularGlossiness_use) {
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
						if (imageD.shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.shadeMasterImage->get_image());
							mLayer.set_image_interface(image);

							// Metallicは[B]の要素を参照.
							mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_blue_mode);

							mLayer.set_blend_mode(7);		// 乗算合成.
							mLayer.set_flip_color(true);
							mLayer.set_weight(materialD.metallicFactor);
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
						if (imageD.shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.shadeMasterImage->get_image());
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
						if (imageBaseColorD.shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageBaseColorD.shadeMasterImage->get_image());
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
						if (imageD.shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.shadeMasterImage->get_image());
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
						if (imageD.shadeMasterImage) {
							compointer<sxsdk::image_interface> image(imageD.shadeMasterImage->get_image());
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
			// COcclusionTextureShaderInterfaceで作成したOcclusionのマッピングレイヤに、乗算合成で割り当てるとする.
			if (materialD.occlusionImageIndex >= 0) {
				//if (materialD.occlusionImageIndex != materialD.metallicRoughnessImageIndex) {
					surface->append_mapping_layer();
					const int layerIndex = surface->get_number_of_mapping_layers() - 1;
					sxsdk::mapping_layer_class& mLayer = surface->mapping_layer(layerIndex);
					mLayer.set_pattern_uuid(OCCLUSION_SHADER_INTERFACE_ID);		// Occlusionのレイヤ.
					mLayer.set_type(sxsdk::enums::diffuse_mapping);

					// テクスチャ画像を割り当て.
					if (sceneData->images[materialD.occlusionImageIndex].shadeMasterImage) {
						compointer<sxsdk::image_interface> image(sceneData->images[materialD.occlusionImageIndex].shadeMasterImage->get_image());
						mLayer.set_image_interface(image);

						mLayer.set_blend_mode(7);		// 乗算合成.

						// Occlusionは[R]の要素を参照.
						mLayer.set_channel_mix(sxsdk::enums::mapping_grayscale_red_mode);
					}

					mLayer.set_blur(true);
					mLayer.set_weight(materialD.occlusionStrength);
					
					// shader_interfaceのマッピングレイヤではUV層の指定ができないため、mapping_layerのstreamに保持.
					//mLayer.set_uv_mapping(materialD.occlusionTexCoord);
					{
						COcclusionShaderData data;
						data.uvIndex = materialD.occlusionTexCoord;
						data.channelMix = 0;	// Red.
						StreamCtrl::saveOcclusionParam(mLayer, data);
					}

					mLayer.set_repetition_x(std::max(1, (int)materialD.occlusionTexScale.x));
					mLayer.set_repetition_y(std::max(1, (int)materialD.occlusionTexScale.y));
				//}
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
		m_setMeshSkin(scene, sceneData, (int)meshLoop);
	}
}

void CGLTFImporterInterface::m_setMeshSkin (sxsdk::scene_interface *scene, CSceneData* sceneData, const int meshIndex)
{
	const size_t skinsCou = sceneData->skins.size();
	if (skinsCou == 0) return;

	const CMeshData& meshD = sceneData->meshes[meshIndex];
	if (!meshD.pMeshHandle) return;
	sxsdk::shape_class* meshShape = scene->get_shape_by_handle(meshD.pMeshHandle);
	if (!meshShape) return;
	if (meshShape->get_type() != sxsdk::enums::polygon_mesh) return;
	sxsdk::polygon_mesh_class& pMesh = meshShape->get_polygon_mesh();

	// meshLoopに対応するノード番号を取得.
	const int nodeIndex = m_findNodeFromMeshIndex(sceneData, meshIndex);
	if (nodeIndex < 0) return;

	const CNodeData& nodeD = sceneData->nodes[nodeIndex];
	if (nodeD.skinIndex < 0) return;
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
	if (!meshD.mergePrimitives(newMeshData)) return;
	if (newMeshData.skinJoints.empty() || newMeshData.skinWeights.empty()) return;
	if (newMeshData.skinJoints.size() != newMeshData.skinWeights.size()) return;

	// meshD.pMeshのポリゴンメッシュに対してスキンのバインドとウエイト値を指定.
	const size_t versCou = newMeshData.vertices.size();
	if ((int)versCou == pMesh.get_total_number_of_control_points()) {
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

		if (skinD.skeletonID >= 0) {
			const int rootNodeIndex = skinD.skeletonID + 1;

			// 格納されたskinD.inverseBindMatrices (ルートボーンのワールドローカル変換行列)、ルートのローカルワールド変換行列より、.
			// 差分の行列を計算.
			const sxsdk::mat4 bindM = inv(skinD.inverseBindMatrices[0]);
			const sxsdk::mat4 lwM   = sceneData->getLocalToWorldMatrix(rootNodeIndex);
			const sxsdk::mat4 m     = lwM * inv(bindM);

			const int versCou = pMesh.get_number_of_skin_points();
			if (versCou > 0 && versCou == pMesh.get_total_number_of_control_points()) {
				for (int i = 0; i < versCou; ++i) {
					sxsdk::vec3 v = pMesh.vertex(i).get_position() * m;		// ボーンルートからのローカル座標系に変換.
					pMesh.vertex(i).set_position(v);
				}
			}
		}

		// 重複頂点のマージ.
		m_cleanupRedundantVertices(pMesh, &newMeshData);

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
	if (animCou == 0) return;
	std::vector<bool> chkAnimA(animCou, false);

	const int frameRate = scene->get_frame_rate();

	// アニメーションはtranslationとrotationをセットにして配列に保持し、Shade3Dのモーションポイントとして格納していく.
	int targetNodeIndex = 0;
	while (targetNodeIndex >= 0) {
		targetNodeIndex = -1;
		for (size_t loop = 0; loop < animCou; ++loop) {
			if (chkAnimA[loop]) continue;
			const CAnimChannelData& channelD = sceneData->animations.channelData[loop];
			if (channelD.samplerIndex < 0 || channelD.targetNodeIndex < 0) {
				chkAnimA[loop] = true;
				continue;
			}
			if (channelD.pathType != CAnimChannelData::path_type_translation && channelD.pathType != CAnimChannelData::path_type_rotation) {
				chkAnimA[loop] = true;
				continue;
			}
			targetNodeIndex = sceneData->animations.channelData[loop].targetNodeIndex;
			break;
		}
		if (targetNodeIndex < 0) break;

		std::vector<float> tmpKeyFrames;
		std::vector<sxsdk::vec3> tmpOffsets;
		std::vector<sxsdk::quaternion_class> tmpRotations;
		std::vector<bool> tmpLinears;

		// キーフレーム位置を取得.
		for (size_t loop = 0; loop < animCou; ++loop) {
			if (chkAnimA[loop]) continue;
			const CAnimChannelData& channelD = sceneData->animations.channelData[loop];
			if (channelD.targetNodeIndex != targetNodeIndex) continue;
			const CAnimSamplerData& samplerD = sceneData->animations.samplerData[channelD.samplerIndex];
			const size_t sCou = samplerD.inputData.size();
			for (size_t i = 0; i < sCou; ++i) {
				const float fPos = samplerD.inputData[i];		// 秒単位のキーフレーム位置.
				const float keyFramePos = fPos * (float)frameRate;

				int index = -1;
				for (size_t j = 0; j < tmpKeyFrames.size(); ++j) {
					if (MathUtil::isZero(tmpKeyFrames[j] - keyFramePos)) {
						index = (int)j;
						break;
					}
				}
				if (index < 0) {
					tmpKeyFrames.push_back(keyFramePos);
				}
			}
		}
		if (tmpKeyFrames.empty()) {
			for (size_t loop = 0; loop < animCou; ++loop) {
				if (chkAnimA[loop]) continue;
				const CAnimChannelData& channelD = sceneData->animations.channelData[loop];
				if (channelD.targetNodeIndex != targetNodeIndex) continue;
				chkAnimA[loop] = true;
			}
			continue;
		}

		const size_t framesCou = tmpKeyFrames.size();
		tmpOffsets.resize(framesCou, sxsdk::vec3(0, 0, 0));
		tmpRotations.resize(framesCou, sxsdk::quaternion_class::identity);
		tmpLinears.resize(framesCou, false);

		// キーフレームに対応するOffset/Rotationを一時的に格納.
		for (size_t loop = 0; loop < animCou; ++loop) {
			if (chkAnimA[loop]) continue;
			const CAnimChannelData& channelD = sceneData->animations.channelData[loop];
			if (channelD.targetNodeIndex != targetNodeIndex) continue;
			chkAnimA[loop] = true;

			const CAnimSamplerData& samplerD = sceneData->animations.samplerData[channelD.samplerIndex];
			const size_t sCou = samplerD.inputData.size();
			int iPos = 0;

			for (size_t i = 0; i < sCou; ++i) {
				const float fPos = samplerD.inputData[i];		// 秒単位のキーフレーム位置.
				const float keyFramePos = fPos * (float)frameRate;

				int index = -1;
				for (size_t j = 0; j < tmpKeyFrames.size(); ++j) {
					if (MathUtil::isZero(tmpKeyFrames[j] - keyFramePos)) {
						index = (int)j;
						break;
					}
				}
				if (index < 0) continue;

				if (samplerD.interpolationType == CAnimSamplerData::interpolation_type_linear) {
					tmpLinears[index] = true;
				}

				if (channelD.pathType == CAnimChannelData::path_type_translation) {
					// オフセット値を取得し、メートルからミリメートルに変換.
					tmpOffsets[index] = sxsdk::vec3(samplerD.outputData[iPos + 0], samplerD.outputData[iPos + 1], samplerD.outputData[iPos + 2]) * 1000.0f;
					iPos += 3;

				} else if (channelD.pathType == CAnimChannelData::path_type_rotation) {
					tmpRotations[index] = sxsdk::quaternion_class(-samplerD.outputData[iPos + 3], samplerD.outputData[iPos + 0], samplerD.outputData[iPos + 1], samplerD.outputData[iPos + 2]);
					iPos += 4;
				}
			}
		}

		// Shade3Dでの対象形状を取得.
		const CNodeData& nodeD = sceneData->nodes[targetNodeIndex + 1];		// ルートノードは別途追加したものなので+1している.
		if (!nodeD.pShapeHandle) continue;
		sxsdk::shape_class* shape = scene->get_shape_by_handle(nodeD.pShapeHandle);
		if (!shape) continue;
		if (!shape->has_motion()) continue;

		compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());

		// ローカル変換行列.
		sxsdk::mat4 localM = shape->get_transformation();
		if (Shade3DUtil::isBone(*shape)) {
			compointer<sxsdk::bone_joint_interface> bone(shape->get_bone_joint_interface());
			localM = bone->get_matrix();
		}

		// キーフレーム情報として格納.
		for (size_t i = 0; i < framesCou; ++i) {
			const float keyFramePos = tmpKeyFrames[i];
			int motionPointIndex = motion->create_motion_point(keyFramePos);

			compointer<sxsdk::motion_point_interface> motionP(motion->get_motion_point_interface(motionPointIndex));

			// glTFでのoffset + rotationは、ローカル座標での変換行列を表す.
			sxsdk::vec3 offsetV       = tmpOffsets[i];
			sxsdk::quaternion_class q = tmpRotations[i];
			const sxsdk::mat4 rotM = sxsdk::mat4::rotate(sxsdk::vec3(0, 0, 0), q);

			offsetV = offsetV - sxsdk::vec3(localM[3][0], localM[3][1], localM[3][2]);
			sxsdk::mat4 m = rotM * inv(localM);
			sxsdk::vec3 scale, shear, rotate, trans;
			m.unmatrix(scale, shear, rotate, trans);

			motionP->set_offset(offsetV);
			motionP->set_rotation(sxsdk::quaternion_class(rotate));

			// モーションをリニアにする。これは「コーナー」をOnにする.
			if (tmpLinears[i]) {
				motionP->set_corner(true);
			}
		}
	}
}

