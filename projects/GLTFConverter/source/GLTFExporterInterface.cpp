/**
 * GLTFエクスポータ.
 */
#include "GLTFExporterInterface.h"
#include "GLTFSaver.h"
#include "SceneData.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"
#include "StreamCtrl.h"
#include "StringUtil.h"
#include "ImagesBlend.h"
#include "MotionExternalAccess.h"

#include <iostream>
#include <map>

enum
{
	dlg_output_texture_id = 101,			// テクスチャ出力:|マスターサーフェス名の拡張子指定を参照|pngに置き換え|jpegに置き換え.
	dlg_output_bones_and_skins_id = 102,	// ボーンとスキンを出力.
	dlg_output_vertex_color_id = 103,		// 頂点カラーを出力.
	dlg_output_animation_id = 104,			// アニメーションを出力.
	dlg_output_draco_compression_id = 105,	// Draco圧縮.
	dlg_output_max_texture_size_id = 106,	// 最大テクスチャサイズ.
	dlg_output_share_vertices_mesh_id = 107,	// Mesh内のPrimitiveの頂点情報を共有.
	dlg_output_convert_color_to_linear_id = 108,	// 色をリニアに変換.
	dlg_output_bake_without_processing_textures_id = 109,	// テクスチャを加工せずにベイク.

	dlg_asset_title_id = 201,				// タイトル.
	dlg_asset_author_id = 202,				// 制作者.
	dlg_asset_select_license_id = 203,		// ライセンス選択.
	dlg_asset_license_id = 204,				// ライセンス.
	dlg_asset_license_desc_id = 205,		// ライセンスの説明.
	dlg_asset_source_id = 206,				// 参照先.

	dlg_export_file_format_id = 301,		// 出力形式 (0:glb / 1:gltf).

	dlg_output_textures_id = 401,					// エンジン別にテクスチャを別途出力.
	dlg_output_textures_engine_type_id = 402,		// エンジンの種類.

};

CGLTFExporterInterface::CGLTFExporterInterface (sxsdk::shade_interface& shade) : shade(shade)
{
	m_MorphTargetsAccess = NULL;

	m_dlgOK = false;

	// ライセンスの種類.
	// これはダイアログボックスの「ライセンスの選択」のリストと同じ並び.
	m_licenseTypeList.clear();
	m_licenseTypeList.push_back("All rights reserved");
	m_licenseTypeList.push_back("CC BY-4.0 (https://creativecommons.org/licenses/by/4.0/)");
	m_licenseTypeList.push_back("CC BY-SA-4.0 (https://creativecommons.org/licenses/by-sa/4.0/)");
	m_licenseTypeList.push_back("CC BY-NC-4.0 (https://creativecommons.org/licenses/by-nc/4.0/)");
	m_licenseTypeList.push_back("CC BY-ND-4.0 (https://creativecommons.org/licenses/by-nd/4.0/)");
	m_licenseTypeList.push_back("CC BY-NC-SA-4.0 (https://creativecommons.org/licenses/by-nc-sa/4.0/)");
	m_licenseTypeList.push_back("CC BY-NC-ND-4.0 (https://creativecommons.org/licenses/by-nc-nd/4.0/)");
	m_licenseTypeList.push_back("CC0(Public domain) (https://creativecommons.org/publicdomain/zero/1.0/)");
}

CGLTFExporterInterface::~CGLTFExporterInterface ()
{
}

/**
 * ファイル拡張子.
 */
const char *CGLTFExporterInterface::get_file_extension (void *aux)
{
	return (m_exportParam.exportFileFormat == 0) ? "glb" : "gltf";
}

/**
 * ファイルの説明文.
 */
const char *CGLTFExporterInterface::get_file_description (void *aux)
{
	return "glTF";
}

/**
 * エクスポート処理を行う.
 */
void CGLTFExporterInterface::do_export (sxsdk::plugin_exporter_interface *plugin_exporter, void *)
{
	// Morph Targets情報アクセスクラス.
	m_MorphTargetsAccess = getMorphTargetsAttributeAccess(shade);

	// streamに、ダイアログボックスのパラメータを保存.
	StreamCtrl::saveExportDialogParam(shade, m_exportParam);

	m_shapeStack.clear();
	m_currentDepth = 0;
	m_sceneData.reset(new CSceneData());
	m_warningCheck.clear();

	compointer<sxsdk::scene_interface> scene(shade.get_scene_interface());
	try {
		m_pluginExporter = plugin_exporter;
		m_pluginExporter->AddRef();

		m_stream      = m_pluginExporter->get_stream_interface();
		m_text_stream = m_pluginExporter->get_text_stream_interface();
	} catch(...) { }

	// Asset情報を指定.
	{
		m_sceneData->assetVersion   = "2.0";
		m_sceneData->assetGenerator = "glTF Converter for Shade3D";
		m_sceneData->filePath       = std::string(m_pluginExporter->get_file_path());
	}

	// Assetの拡張情報を指定.
	{
		m_sceneData->assetExtrasTitle   = m_exportParam.assetExtrasTitle;
		m_sceneData->assetExtrasAuthor  = m_exportParam.assetExtrasAuthor;
		m_sceneData->assetExtrasLicense = m_exportParam.assetExtrasLicense;
		m_sceneData->assetExtrasSource  = m_exportParam.assetExtrasSource;
	}

	// エクスポート時のパラメータ.
	m_sceneData->exportParam = m_exportParam;

	m_pScene = scene;

	shade.message("----- glTF Exporter -----");

	// シーケンスモード時は、出力時にシーケンスOffにして出す.
	// シーンの変更フラグは後で元に戻す.
	const bool oldSequenceMode = m_pScene->get_sequence_mode();
	const bool oldDirty = m_pScene->get_dirty();
	if (m_exportParam.outputBonesAndSkins) {
		if (oldSequenceMode) {
			m_pScene->set_sequence_mode(false);
		}
	}

	// Morph Targetsのウエイト値を0にする.
	if (m_MorphTargetsAccess) m_MorphTargetsAccess->pushAllWeight(scene, true);

	// エクスポートを開始.
	plugin_exporter->do_export();

	// Morph Targetsのウエイト値を元に戻す.
	if (m_MorphTargetsAccess) m_MorphTargetsAccess->popAllWeight(scene);

	// 元のシーケンスモードに戻す.
	if (m_exportParam.outputBonesAndSkins) {
		if (oldSequenceMode) {
			m_pScene->set_sequence_mode(oldSequenceMode);
		}
	}

	m_pScene->set_dirty(oldDirty);

	m_MorphTargetsAccess = NULL;
}

/********************************************************************/
/* エクスポートのコールバックとして呼ばれる							*/
/********************************************************************/

/**
 * エクスポートの開始.
 */
void CGLTFExporterInterface::start (void *)
{
}

/**
 * エクスポートの終了.
 */
void CGLTFExporterInterface::finish (void *)
{
}

/**
 * エクスポート処理が完了した後に呼ばれる.
 * ここで、ファイル出力するとstreamとかぶらない.
 */
void CGLTFExporterInterface::clean_up (void *)
{
	if (!m_sceneData || (m_sceneData->filePath) == "") return;

	// ポリゴンメッシュのスキン情報より、スキン情報を格納.
	m_setSkinsFromMeshes();

	// アニメーションを持つ形状はユニーク名になるように補正.
	m_renameUniqueNameInBone();

	// アニメーション情報を格納.
	if (m_exportParam.outputAnimation) {
		m_setAnimations();
	}

	// 警告のメッセージがある場合は表示.
	m_warningCheck.outputWarningMessage(shade);

	CGLTFSaver gltfSaver(&shade);
	if (gltfSaver.saveGLTF(m_sceneData->filePath, &(*m_sceneData))) {
		shade.message("Export success!");
	} else {
		const std::string errorMessage = std::string("Error : ") + gltfSaver.getErrorString();
		shade.message(errorMessage);
	}
}

/**
 * 指定の形状がスキップ対象か.
 */
bool CGLTFExporterInterface::m_checkSkipShape (sxsdk::shape_class* shape)
{
	bool skipF = false;
	if (!shape) return skipF;

	// レンダリング対象でない場合はスキップ.
	const std::string name(shape->get_name());
	if (shape->get_render_flag() == 0) skipF = true;
	if (name != "" && name[0] == '#') skipF = true;

	if (!skipF && shape->get_render_flag() < 0) {		// 継承.
		sxsdk::shape_class* pS = shape;
		while (pS->has_dad()) {
			pS = pS->get_dad();
			if (pS->get_render_flag() == 0) {
				skipF = true;
				break;
			}
			if (pS->get_render_flag() == 1) {
				skipF = false;
				break;
			}
		}
	}
	if (skipF) return true;

	// 形状により、スキップする形状を判断.
	const int type = shape->get_type();
	if (type == sxsdk::enums::part) {
		const int partType = shape->get_part().get_part_type();
		if (partType == sxsdk::enums::master_surface_part || partType == sxsdk::enums::master_image_part) skipF = true;

	} else {
		if (type == sxsdk::enums::master_surface || type == sxsdk::enums::master_image) skipF = true;

		// 光源の場合はスキップ.
		if (type == sxsdk::enums::area_light || type == sxsdk::enums::directional_light || type == sxsdk::enums::point_light || type == sxsdk::enums::spotlight) {
			skipF = true;
		}
		if (type == sxsdk::enums::line) {
			// 面光源/線光源の場合.
			sxsdk::line_class& lineC = shape->get_line();
			if (lineC.get_light_intensity() > 0.0f) skipF = true;
		}
	}

	return skipF;
}

/**
 * カレント形状の処理の開始.
 */
void CGLTFExporterInterface::begin (void *)
{
	m_pCurrentShape = NULL;
	m_skip = false;
	m_meshData.clear();
	m_currentShapeSkinType = false;

	// カレントの形状管理クラスのポインタを取得.
	m_pCurrentShape        = m_pluginExporter->get_current_shape();
	const sxsdk::mat4 gMat = m_pluginExporter->get_transformation();	// local to worldの変換行列.
	const std::string name(m_pCurrentShape->get_name());

	// 形状により、スキップする形状を判断.
	const int type = m_pCurrentShape->get_type();
	m_skip = m_checkSkipShape(m_pCurrentShape);

	// ローカル変換行列.
	sxsdk::mat4 tPreM = m_shapeStack.getLocalToWorldMatrix();
	sxsdk::mat4 lMat  = gMat * inv(tPreM);

	// 掃除引体、回転体の場合.
	if (m_pCurrentShape->get_type() == sxsdk::enums::line) {
		sxsdk::line_class& lineC = m_pCurrentShape->get_line();
		if (lineC.is_extruded() || lineC.is_revolved()) {
			lMat = inv(m_pCurrentShape->get_transformation()) * lMat;
		}
	}

	m_shapeStack.push(m_currentDepth, m_pCurrentShape, lMat);

	// 面の反転フラグ.
	m_flipFace = m_shapeStack.isFlipFace();

	// 掃引体の場合は面が反転しているので、反転.
	if (type == sxsdk::enums::line) {
		sxsdk::line_class& lineC = m_pCurrentShape->get_line();
		if (lineC.is_extruded()) {
			m_flipFace = !m_flipFace;
		}
	}
	if (type == sxsdk::enums::disk) {
		sxsdk::disk_class& diskC = m_pCurrentShape->get_disk();
		if (diskC.is_extruded()) {
			m_flipFace = !m_flipFace;
		}
	}

	m_spMat = sxsdk::mat4::identity;
	m_currentLWMatrix = gMat;
	m_currentDepth++;

	if (!m_skip) {
		const sxsdk::mat4 lwMat = m_pCurrentShape->get_local_to_world_matrix();

		sxsdk::mat4 m = lMat;

		if (type == sxsdk::enums::polygon_mesh) {
			m = sxsdk::mat4::identity;

		} else if (Shade3DUtil::isBallJoint(*m_pCurrentShape)) {
			// ボールジョイントの場合のローカル座標での変換行列.
			m = Shade3DUtil::getBallJointMatrix(*m_pCurrentShape);

		} else {
			// 親がボールジョイントの場合.
			if (m_pCurrentShape->has_dad()) {
				sxsdk::shape_class* parentShape = m_pCurrentShape->get_dad();
				if (Shade3DUtil::isBallJoint(*parentShape)) {
					const sxsdk::mat4 parentLWMat = Shade3DUtil::getBallJointMatrix(*parentShape, true);
					m = m * lwMat;
					m = m * inv(parentLWMat);
				}
			}
		}

		const std::string name = std::string(m_pCurrentShape->get_name());
		const int nodeIndex = m_sceneData->beginNode(name, m);

		// 形状に対応するハンドルを保持 (出力前に、スキンでのノード番号取得で使用).
		m_sceneData->nodes[nodeIndex].pShapeHandle = m_pCurrentShape->get_handle();

		// 未サポートのジョイントを使用している場合.
		if (Shade3DUtil::usedUnsupportedJoint(*m_pCurrentShape)) {
			m_warningCheck.appendUnsupportedJointUsedShape(m_pCurrentShape);
		}

		// 変換行列mがせん断を持つ場合.
		if (Shade3DUtil::hasShearInMatrix(m)) {
			m_warningCheck.appendShearUsedShape(m_pCurrentShape);
		}

		m_currentShapeSkinType = m_pCurrentShape->get_skin_type();
		if (m_currentShapeSkinType == 0) {		// クラシックスキン.
			m_warningCheck.appendClassicSkinUsedShape(m_pCurrentShape);
		}
	}
}

/**
 * カレント形状の処理の終了.
 */
void CGLTFExporterInterface::end (void *)
{
	if (!m_skip && m_pCurrentShape) {
		if (m_meshData.vertices.empty() || m_meshData.triangleIndices.empty()) {
			const int type = m_pCurrentShape->get_type();
			if (type == sxsdk::enums::line) {
				m_sceneData->endNode(true);		// 追加したノードは削除.
				m_skip = true;
			}
			if (type == sxsdk::enums::part) {
				if (m_pCurrentShape->get_part().get_part_type() == sxsdk::enums::surface_part) {	// 自由曲面.
					m_sceneData->endNode(true);		// 追加したノードは削除.
					m_skip = true;
				}
			}
		}
	}

	m_shapeStack.pop();
	m_pCurrentShape = NULL;

	sxsdk::shape_class *pShape;
	sxsdk::mat4 lwMat;
	int depth = 0;

	m_shapeStack.getShape(&pShape, &lwMat, &depth);
	if (pShape && depth > 0) {
		m_pCurrentShape  = pShape;
		m_currentDepth   = depth;
		m_currentLWMatrix = inv(pShape->get_transformation()) * m_shapeStack.getLocalToWorldMatrix();

		// 面の反転フラグ.
		m_flipFace = m_shapeStack.isFlipFace();
	}

	if (!m_skip) {
		m_sceneData->endNode();
	}

	m_skip = m_checkSkipShape(m_pCurrentShape);
}

/**
 * カレント形状が掃引体の上面部分の場合、掃引に相当する変換マトリクスが渡される.
 */
void CGLTFExporterInterface::set_transformation (const sxsdk::mat4 &t, void *)
{
	m_spMat = t;
	m_flipFace = !m_flipFace;	// 掃引体の場合、上ふたの面が反転するため面反転を行う.
}

/**
 * カレント形状が掃引体の上面部分の場合の行列クリア.
 */
void CGLTFExporterInterface::clear_transformation (void *)
{
	m_spMat = sxsdk::mat4::identity;
	m_flipFace = !m_flipFace;	// 面反転を戻す.
}

/**
 * ポリゴンメッシュの開始時に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh (void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = -1;
	m_faceGroupCount = 0;

	m_LWMat = m_spMat * m_currentLWMatrix;
	m_WLMat = inv(m_LWMat);

	m_meshData.clear();
	m_meshData.name = std::string(m_pCurrentShape->get_name());

	// NURBSの場合は面がフェイスグループごとに分割されてしまうため、それを判断する.
	m_isCurrentNurbs = (m_pCurrentShape->get_type() == 13);		// 13はNURBS形状.

	m_currentMeshFaceIndex = 0;
	m_currentMeshFaceUseList.clear();
	m_tempZeroMeshCount = 0;
}

/**
 * ポリゴンメッシュの頂点情報格納時に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh_vertex (int n, void *)
{
	if (m_skip) return;

	m_meshData.vertices.resize(n);
	m_meshData.skinJointsHandle.clear();
	m_meshData.skinWeights.clear();
}

/**
 * 頂点が格納されるときに呼ばれる.
 */
void CGLTFExporterInterface::polymesh_vertex (int i, const sxsdk::vec3 &v, const sxsdk::skin_class *skin)
{
	if (m_skip) return;

	sxsdk::vec3 pos = v;

	// must_transform_skinでfalseを返した場合、スキン変形前の頂点が取得される.
	// そのため、独自変換は不要。.

	pos = pos * m_spMat;
	if (skin) {
		pos = pos * m_currentLWMatrix;		// スキン使用時はワールド座標に変換する.
	}

#if 0
	if (skin) {
		// スキン変換前の座標値を計算.
		if (m_exportParam.outputBonesAndSkins) {
			const sxsdk::mat4 skin_m = skin->get_skin_world_matrix();
			const sxsdk::mat4 skin_m_inv = inv(skin_m);
			sxsdk::vec4 v4 = sxsdk::vec4(pos, 1) * m_LWMat * skin_m_inv;
			pos = sxsdk::vec3(v4.x, v4.y, v4.z);
		}
	}
#endif

	m_meshData.vertices[i] = pos * 0.001f;		// mm ==> m変換.

	if (m_exportParam.outputBonesAndSkins) {
		if (skin && m_pCurrentShape->get_skin_type() == 1) {		// スキンを持っており、頂点ブレンドで格納されている場合.
			const int bindsCou = skin->get_number_of_binds();
			if (bindsCou > 0) {
				// スキンの影響ジョイント(bone/ball)とweight値を格納.
				m_meshData.skinJointsHandle.push_back(sx::vec<void*,4>(NULL, NULL, NULL, NULL));
				m_meshData.skinWeights.push_back(sxsdk::vec4(0, 0, 0, 0));
				for (int j = 0; j < bindsCou && j < 4; ++j) {
					const sxsdk::skin_bind_class& skinBind = skin->get_bind(j);
					m_meshData.skinJointsHandle[i][j] = skinBind.get_shape()->get_handle();
					m_meshData.skinWeights[i][j]      = skinBind.get_weight();
				}

				if (bindsCou < 4) {
					// ボーンの親をたどり、skinのweight 0(スキンの影響なし)として割り当てる.
					// これは、gltf出力時にスキン対象とするボーン(node)を認識しやすくするため.
					std::vector<sxsdk::shape_class *> shapeList;
					const sxsdk::skin_bind_class& skinBind = skin->get_bind(bindsCou - 1);
					sxsdk::shape_class* pS = skinBind.get_shape();
					if (pS->has_dad()) {
						pS = pS->get_dad();
						while ((pS->get_part().get_part_type()) == sxsdk::enums::bone_joint || (pS->get_part().get_part_type()) == sxsdk::enums::ball_joint) {
							shapeList.push_back(pS);
							if (shapeList.size() > 3) break;
							if (!pS->has_dad()) break;
							pS = pS->get_dad();
						}
					}
					for (int j = 0; j + bindsCou < 4 && j < shapeList.size(); ++j) {
						m_meshData.skinJointsHandle[i][j + bindsCou] = shapeList[j];
						m_meshData.skinWeights[i][j + bindsCou]      = 0.0f;
					}
				}

				// 同一のボーンを参照している場合はWeight値を0にする.
				{
					for (int j = 0; j < 4; ++j) {
						void* pV = m_meshData.skinJointsHandle[i][j];
						for (int k = j + 1; k < 4; ++k) {
							void* pV2 = m_meshData.skinJointsHandle[i][k];
							if (pV == pV2) {
								m_meshData.skinWeights[i][k] = 0.0f;
							}
						}
					}
				}


				// skinWeights[]が大きい順に並び替え.
				for (int j = 0; j < 4; ++j) {
					for (int k = j + 1; k < 4; ++k) {
						if (m_meshData.skinWeights[i][j] < m_meshData.skinWeights[i][k]) {
							std::swap(m_meshData.skinWeights[i][j], m_meshData.skinWeights[i][k]);
							std::swap(m_meshData.skinJointsHandle[i][j], m_meshData.skinJointsHandle[i][k]);
						}
					}
				}

				// Weight値の合計が1.0になるように正規化.
				float sumWeight = 0.0f;
				for (int j = 0; j < 4; ++j) sumWeight += m_meshData.skinWeights[i][j];
				if (!MathUtil::isZero(sumWeight)) {
					for (int j = 0; j < 4; ++j) m_meshData.skinWeights[i][j] /= sumWeight;
				}
			}
		}
	}
}

/**
 * ポリゴンメッシュの面情報が格納されるときに呼ばれる.
 */
void CGLTFExporterInterface::polymesh_face_uvs (int n_list, const int list[], const sxsdk::vec3 *normals, const sxsdk::vec4 *plane_equation, const int n_uvs, const sxsdk::vec2 *uvs, void *)
{
	if (m_skip) return;

	// カレントの面番号のリスト（面積0のスキップ確認用）を保持.
	m_currentMeshFaceIndex++;
	m_currentMeshFaceUseList.push_back(true);

	std::vector<int> indicesList;
	std::vector<sxsdk::vec3> normalsList;
	std::vector<sxsdk::vec2> uvs0List, uvs1List;

	indicesList.resize(n_list);
	normalsList.resize(n_list);
	uvs0List.resize(n_list, sxsdk::vec2(0, 0));
	uvs1List.resize(n_list, sxsdk::vec2(0, 0));

	sxsdk::vec4 v4;
	for (int i = 0; i < n_list; ++i) {
		indicesList[i] = list[i];
		v4 = sxsdk::vec4(normals[i], 0) * m_spMat;
		normalsList[i] = normalize(sxsdk::vec3(v4.x, v4.y, v4.z));
	}

	// 法線のゼロチェック.
	if (n_list == 3) {
		bool errorNormal = false;
		for (int i = 0; i < n_list; ++i) {
			sxsdk::vec3 n = normalsList[i];
			if (sxsdk::absolute(n) < 0.9f) {
				errorNormal = true;
				break;
			}
		}

		if (errorNormal) {
			// 頂点座標をmからできるだけ大きめの値に変換 (極小の三角形の場合もありうるため).
			sxsdk::vec3 vA[4];
			const float scaleV = 100000.0f;
			for (int i = 0; i < n_list; ++i) {
				vA[i] = m_meshData.vertices[list[i]] * scaleV;
			}

			// 面積を計算.
			const double areaV = MathUtil::calcTriangleArea(vA[0], vA[1], vA[2]);
			if (areaV <= 0.0) {
				m_currentMeshFaceUseList.back() = false;
				m_tempZeroMeshCount++;

			} else {
				// 法線を計算.
				const sxsdk::vec3 n = MathUtil::calcTriangleNormal(vA[0], vA[1], vA[2]);
				for (int i = 0; i < n_list; ++i) normalsList[i] = n;
			}
		}
	}

	if (n_uvs > 0 && uvs != NULL) {
		for (int i = 0; i < n_uvs && i < 2; ++i) {
			int iPos = i;
			for (int j = 0; j < n_list; ++j) {
				if (i == 0) uvs0List[j] = uvs[iPos];
				else if (i == 1) uvs1List[j] = uvs[iPos];
				iPos += n_uvs;
			}
		}
	}

	if (m_flipFace) {
		std::reverse(indicesList.begin(), indicesList.end());
		std::reverse(normalsList.begin(), normalsList.end());
		std::reverse(uvs0List.begin(), uvs0List.end());
		std::reverse(uvs1List.begin(), uvs1List.end());
		for (size_t i = 0; i < normalsList.size(); ++i) normalsList[i] = -normalsList[i];
	}

	if (n_list == 3) {
		for (int i = 0; i < 3; ++i) {
			m_meshData.triangleIndices.push_back(indicesList[i]);
		}
		for (int i = 0; i < 3; ++i) {
			m_meshData.triangleNormals.push_back(normalsList[i]);
		}
		if (n_uvs >= 1) {
			for (int i = 0; i < 3; ++i) {
				m_meshData.triangleUV0.push_back(uvs0List[i]);
			}
		}
		if (n_uvs >= 2) {
			for (int i = 0; i < 3; ++i) {
				m_meshData.triangleUV1.push_back(uvs1List[i]);
			}
		}
		m_meshData.triangleFaceGroupIndex.push_back(m_currentFaceGroupIndex);
	}
}

/**
 * 頂点カラー情報を受け取る.
 */
void CGLTFExporterInterface::polymesh_face_vertex_colors (int n_list, const int list[], const sxsdk::rgba_class* vertex_colors, int layer_index, int number_of_layers, void*)
{
	if (!m_exportParam.outputVertexColor) return;
	if (n_list != 3) return;

	if (layer_index == 0) {
		for (int i = 0; i < n_list; ++i) {
			sxsdk::rgba_class col = vertex_colors[i];

			// 色をリニアにする.
			if (m_exportParam.convertColorToLinear) {
				MathUtil::convColorLinear(col.red, col.green, col.blue);
			}

			m_meshData.triangleColor0.push_back(sxsdk::vec4(col.red, col.green, col.blue, col.alpha));
		}
	}

	if (layer_index == 1) {
		for (int i = 0; i < n_list; ++i) {
			sxsdk::rgba_class col = vertex_colors[i];

			// 色をリニアにする.
			if (m_exportParam.convertColorToLinear) {
				MathUtil::convColorLinear(col.red, col.green, col.blue);
			}

			m_meshData.triangleColor1.push_back(sxsdk::vec4(col.red, col.green, col.blue, col.alpha));
		}
	}
}

/**
 * ポリゴンメッシュの終了時に呼ばれる.
 */
void CGLTFExporterInterface::end_polymesh (void *)
{
	if (m_skip) return;

	// 頂点カラーを2つ使っている場合は乗算する.
	if (!m_meshData.triangleColor0.empty()) {
		if (!m_meshData.triangleColor1.empty()) {
			if (m_meshData.triangleColor0.size() == m_meshData.triangleColor1.size()) {
				const size_t vCou = m_meshData.triangleColor0.size();
				for (size_t i = 0; i < vCou; ++i) {
					m_meshData.triangleColor0[i] = (m_meshData.triangleColor0[i] + m_meshData.triangleColor1[i]) * 0.5f;
				}
				m_meshData.triangleColor1.clear();
			}
		}
	} else {
		if (!m_meshData.triangleColor1.empty()) {
			m_meshData.triangleColor0 = m_meshData.triangleColor1;
			m_meshData.triangleColor1.clear();
		}
	}

	// 面積ゼロの面がある場合はカットしていく.
	if (m_tempZeroMeshCount > 0) {
		const int iCou = (int)m_currentMeshFaceUseList.size();
		int iPos = (iCou - 1) * 3;
		for (int i = iCou - 1; i >= 0; --i, iPos -= 3) {
			if (!m_currentMeshFaceUseList[i]) {
				for (int j = 2; j >= 0; --j) m_meshData.triangleIndices.erase(m_meshData.triangleIndices.begin() + iPos + j);
				if (!m_meshData.triangleNormals.empty()) {
					for (int j = 2; j >= 0; --j) m_meshData.triangleNormals.erase(m_meshData.triangleNormals.begin() + iPos + j);
				}
				if (!m_meshData.triangleUV0.empty()) {
					for (int j = 2; j >= 0; --j) m_meshData.triangleUV0.erase(m_meshData.triangleUV0.begin() + iPos + j);
				}
				if (!m_meshData.triangleUV1.empty()) {
					for (int j = 2; j >= 0; --j) m_meshData.triangleUV1.erase(m_meshData.triangleUV1.begin() + iPos + j);
				}
				if (!m_meshData.triangleColor0.empty()) {
					for (int j = 2; j >= 0; --j) m_meshData.triangleColor0.erase(m_meshData.triangleColor0.begin() + iPos + j);
				}
				
				if (!m_meshData.triangleFaceGroupIndex.empty()) {
					m_meshData.triangleFaceGroupIndex.erase(m_meshData.triangleFaceGroupIndex.begin() + i);
				}
			}
		}
	}

	m_meshData.optimize();

	// m_meshDataからGLTFの形式にコンバートして格納.
	if (!m_meshData.triangleIndices.empty() && !m_meshData.vertices.empty()) {
		// Morph Targets情報を取得し、m_meshDataに格納.
		m_setMorphTargets(m_pCurrentShape);

		if (m_faceGroupCount == 0) {
			const int curNodeIndex = m_sceneData->getCurrentNodeIndex();
			const int meshIndex = m_sceneData->appendNewMeshData();
			CMeshData& meshD = m_sceneData->getMeshData(meshIndex);
			meshD.name        = m_meshData.name;
			meshD.pMeshHandle = m_pCurrentShape->get_handle();
			meshD.primitives.push_back(CPrimitiveData());
			CPrimitiveData& primitiveD = m_sceneData->getMeshData(meshIndex).primitives[0];
			primitiveD.convert(m_meshData);

			// マテリアル情報を格納.
			primitiveD.materialIndex = m_setMaterialCurrentShape(m_pCurrentShape);

			if (m_sceneData->nodes[curNodeIndex].meshIndex >= 0) {
				// 掃引体は1つで3つのメッシュを生成するため、マージ.
				m_sceneData->mergeLastTwoMeshes();

			} else {
				m_sceneData->nodes[curNodeIndex].meshIndex = meshIndex;
			}

			// マテリアルとして頂点カラーを持たず、メッシュとして頂点カラー情報を持つ場合、頂点カラー情報を削除.
			if (primitiveD.materialIndex >= 0) {
				if (!m_sceneData->hasVertexColorInMaterial(primitiveD.materialIndex)) {
					if (!primitiveD.color0.empty()) primitiveD.color0.clear();
				}
			}

		} else {
			// フェイスグループを使用しているポリゴンメッシュの場合、Primitiveを作成してそれに分けて格納.
			m_storeMeshesWithFaceGroup();
		}
	}
}

/**
 * 形状に割り当てられているMorph Targets情報を格納.
 */
void CGLTFExporterInterface::m_setMorphTargets (sxsdk::shape_class* shape)
{
	if (!m_MorphTargetsAccess) return;
	if (shape->get_type() != sxsdk::enums::polygon_mesh) return;

	// Morph Targetsの情報を読み込み.
	if (!m_MorphTargetsAccess->readMorphTargetsData(*shape)) return;

	sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
	const int orgVersCou = m_MorphTargetsAccess->getOrgVerticesCount();
	const int targetsCou = m_MorphTargetsAccess->getTargetsCount();
	if (orgVersCou == 0 || targetsCou == 0) return;
	if (pMesh.get_total_number_of_control_points() != orgVersCou) return;

	// オリジナルの頂点座標を取得.
	std::vector<sxsdk::vec3> orgVertices;
	orgVertices.resize(orgVersCou);
	m_MorphTargetsAccess->getOrgVertices(&(orgVertices[0]));

	const float scaleInv = 1.0f / 1000.0f;

	if (m_meshData.vertices.size() == orgVersCou) {
		for (int i = 0; i < orgVersCou; ++i) m_meshData.vertices[i] = orgVertices[i] * scaleInv;
	}

	// ウエイト値を取得.
	std::vector<float> weights;
	weights.resize(targetsCou, 0.0f);
	m_MorphTargetsAccess->getShapeCurrentWeights(shape, &(weights[0]));

	char szName[256];
	std::vector<int> indices;
	std::vector<sxsdk::vec3> vertices;
	for (int i = 0; i < targetsCou; ++i) {
		m_MorphTargetsAccess->getTargetName(i, szName);
		m_meshData.morphTargets.morphTargetsData.push_back(COneMorphTargetData());
		COneMorphTargetData& dstMorphTargetD = m_meshData.morphTargets.morphTargetsData.back();
		dstMorphTargetD.name = std::string(szName);
		const int vCou = m_MorphTargetsAccess->getTargetVerticesCount(i);
		if (vCou == 0) continue;
		indices.resize(vCou);
		vertices.resize(vCou);
		m_MorphTargetsAccess->getTargetVertices(i, &(indices[0]), &(vertices[0]));

		dstMorphTargetD.vIndices = indices;

		dstMorphTargetD.position.resize(vCou);
		for (int j = 0; j < vCou; ++j) dstMorphTargetD.position[j] = vertices[j] * scaleInv;

		dstMorphTargetD.weight = weights[i];	//m_MorphTargetsAccess->getTargetWeight(i);
	}
}

/**
 * ポリゴンメッシュのフェイスグループを使用している場合の格納.
 */
void CGLTFExporterInterface::m_storeMeshesWithFaceGroup ()
{
	// フェイスグループごとにメッシュを分離.
	std::vector<CPrimitiveData> primitivesDataList;
	std::vector<int> faceGroupIndexList;
	const int primitivesCou = CPrimitiveData::convert(m_meshData, m_exportParam.shareVerticesMesh, primitivesDataList, faceGroupIndexList);
	if (primitivesCou == 0) return;

	// マテリアル情報を格納.
	for (int i = 0; i < primitivesCou; ++i) {
		const int faceGroupIndex = faceGroupIndexList[i];
		// 形状に割り当てられているマテリアル情報を格納.
		primitivesDataList[i].materialIndex = m_setMaterialCurrentShape(m_pCurrentShape, faceGroupIndex);
	}

	// Mesh内にPrimitiveの配列として格納.
	const int meshIndex = m_sceneData->appendNewMeshData();
	CMeshData& meshD = m_sceneData->getMeshData(meshIndex);
	meshD.name        = m_meshData.name;
	meshD.pMeshHandle = m_pCurrentShape->get_handle();

	for (int i = 0; i < primitivesCou; ++i) {
		CPrimitiveData& primitiveD = primitivesDataList[i];
		primitiveD.materialIndex = primitivesDataList[i].materialIndex;

		// マテリアルとして頂点カラーを持たず、メッシュとして頂点カラー情報を持つ場合、頂点カラー情報を削除.
		if (primitiveD.materialIndex >= 0) {
			if (!m_sceneData->hasVertexColorInMaterial(primitiveD.materialIndex)) {
				if (!primitiveD.color0.empty()) primitiveD.color0.clear();
			}
		}

		meshD.primitives.push_back(primitiveD);
	}

	const int curNodeIndex = m_sceneData->getCurrentNodeIndex();
	m_sceneData->nodes[curNodeIndex].meshIndex = meshIndex;
}

/**
 * 面情報格納前に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh_face2 (int n, int number_of_face_groups, void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = -1;
	m_faceGroupCount = number_of_face_groups;

	// NURBSの場合はフェイスグループごとに分解が起きるので、フェイスグループは使用しないようにする.
	if (m_isCurrentNurbs) m_faceGroupCount = 0;
}

/**
 * フェイスグループごとの面列挙前に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh_face_group (int face_group_index, void *)
{
	if (m_skip) return;

	// NURBSの場合はフェイスグループごとに分解が起きるので、フェイスグループは使用しないようにする.
	if (m_isCurrentNurbs) return;

	m_currentFaceGroupIndex = face_group_index;

	// フェイスグループが「無し」の場合.
	if (m_currentFaceGroupIndex >= 0) {
		if (m_pCurrentShape->get_type() == sxsdk::enums::polygon_mesh) {
			sxsdk::polygon_mesh_class& pMesh = m_pCurrentShape->get_polygon_mesh();
			if (pMesh.get_face_group_surface(m_currentFaceGroupIndex) == NULL) {
				m_currentFaceGroupIndex = -1;
			}
		}
	}
}

/**
 * フェイスグループごとの面列挙後に呼ばれる.
 */
void CGLTFExporterInterface::end_polymesh_face_group (void *)
{
	if (m_skip) return;
}

/****************************************************************/
/* ダイアログイベント											*/
/****************************************************************/
/**
 * ダイアログボックスの「ライセンス選択」で選択された要素より、対応するテキストを取得.
 * その他の場合は "" を返す.
 */
std::string CGLTFExporterInterface::m_getSelectLicenseToString (const int selectionIndex)
{
	const int cou = (int)m_licenseTypeList.size();
	if (selectionIndex < 0 || selectionIndex >= cou) return "";
	return m_licenseTypeList[selectionIndex];
}

/**
 * ダイアログボックスの「ライセンス選択」で選択された要素より、説明文のテキストを取得.
 */
std::string CGLTFExporterInterface::m_getSelectLicenseToDescString (const int selectionIndex)
{
	if (selectionIndex == 0) {
		return shade.gettext("license_All_rights_reserved");
	}
	if (selectionIndex == 1) {
		return shade.gettext("license_CC_BY");
	}
	if (selectionIndex == 2) {
		return shade.gettext("license_CC_BY-SA");
	}
	if (selectionIndex == 3) {
		return shade.gettext("license_CC_BY-NC");
	}
	if (selectionIndex == 4) {
		return shade.gettext("license_CC_BY-ND");
	}
	if (selectionIndex == 5) {
		return shade.gettext("license_CC_BY-NC-SA");
	}
	if (selectionIndex == 6) {
		return shade.gettext("license_CC_BY-NC-ND");
	}
	if (selectionIndex == 7) {
		return shade.gettext("license_public_domain");
	}
	return shade.gettext("license_other");
}

/**
 * ダイアログボックスの「ライセンス」のテキストより、対応する「ライセンス選択」のインデックスを取得.
 */
int CGLTFExporterInterface::m_getLicenseStringToIndex (const std::string& str)
{
	const size_t cou = m_licenseTypeList.size();
	int index = (int)cou;
	for (size_t i = 0; i < cou; ++i) {
		if (m_licenseTypeList[i] == str) {
			index = (int)i;
			break;
		}
	}
	return index;
}

void CGLTFExporterInterface::initialize_dialog (sxsdk::dialog_interface& dialog, void*)
{
	// streamより、ダイアログボックスのパラメータを取得.
	StreamCtrl::loadExportDialogParam(shade, m_exportParam);
}

void CGLTFExporterInterface::load_dialog_data (sxsdk::dialog_interface &d,void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_texture_id));
		item->set_selection((int)m_exportParam.outputTexture);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_bones_and_skins_id));
		item->set_bool(m_exportParam.outputBonesAndSkins);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_vertex_color_id));
		item->set_bool(m_exportParam.outputVertexColor);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_animation_id));
		item->set_bool(m_exportParam.outputAnimation);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_draco_compression_id));
		item->set_bool(m_exportParam.dracoCompression);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_max_texture_size_id));
		item->set_selection((int)m_exportParam.maxTextureSize);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_share_vertices_mesh_id));
		item->set_bool(m_exportParam.shareVerticesMesh);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_convert_color_to_linear_id));
		item->set_bool(m_exportParam.convertColorToLinear);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_bake_without_processing_textures_id));
		item->set_bool(m_exportParam.bakeWithoutProcessingTextures);
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_title_id));
		item->set_string(m_exportParam.assetExtrasTitle.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_author_id));
		item->set_string(m_exportParam.assetExtrasAuthor.c_str());
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_select_license_id));
		const int index = m_getLicenseStringToIndex(m_exportParam.assetExtrasLicense);
		item->set_selection(index);
	}
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_license_id));
		item->set_string(m_exportParam.assetExtrasLicense.c_str());
		const int index = m_getLicenseStringToIndex(m_exportParam.assetExtrasLicense);
		item->set_enabled(index == (int)m_licenseTypeList.size());
	}
	{
		// ライセンスの説明文を表示.
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_license_desc_id));
		const int index = m_getLicenseStringToIndex(m_exportParam.assetExtrasLicense);
		item->set_text(m_getSelectLicenseToDescString(index));
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_asset_source_id));
		item->set_string(m_exportParam.assetExtrasSource.c_str());
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_export_file_format_id));
		item->set_selection(m_exportParam.exportFileFormat);
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_textures_id));
		item->set_bool(m_exportParam.outputAdditionalTextures);
	}

	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_output_textures_engine_type_id));
		item->set_selection((int)m_exportParam.engineType);
		item->set_enabled(m_exportParam.outputAdditionalTextures);
	}
}

void CGLTFExporterInterface::save_dialog_data (sxsdk::dialog_interface &dialog,void *)
{

}

bool CGLTFExporterInterface::respond (sxsdk::dialog_interface &dialog, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();

	if (id == sx::iddefault) {
		m_exportParam.clear();
		load_dialog_data(dialog);
		return true;
	}

	if (id == sx::idok) {
		m_dlgOK = true;
	}

	if (id == dlg_output_texture_id) {
		m_exportParam.outputTexture = (GLTFConverter::export_texture_type)(item.get_selection());
		return true;
	}

	if (id == dlg_output_bones_and_skins_id) {
		m_exportParam.outputBonesAndSkins = item.get_bool();
		return true;
	}

	if (id == dlg_output_vertex_color_id) {
		m_exportParam.outputVertexColor = item.get_bool();
		return true;
	}

	if (id == dlg_output_animation_id) {
		m_exportParam.outputAnimation = item.get_bool();
		return true;
	}

	if (id == dlg_output_draco_compression_id) {
		m_exportParam.dracoCompression = item.get_bool();
		return true;
	}

	if (id == dlg_output_max_texture_size_id) {
		m_exportParam.maxTextureSize = (GLTFConverter::export_max_texture_size)item.get_selection();
		return true;
	}

	if (id == dlg_output_share_vertices_mesh_id) {
		m_exportParam.shareVerticesMesh = item.get_bool();
		return true;
	}
	if (id == dlg_output_convert_color_to_linear_id) {
		m_exportParam.convertColorToLinear = item.get_bool();
		return true;
	}
	if (id == dlg_output_bake_without_processing_textures_id) {
		m_exportParam.bakeWithoutProcessingTextures = item.get_bool();
		return true;
	}

	if (id == dlg_asset_title_id) {
		m_exportParam.assetExtrasTitle = item.get_string();
		return true;
	}
	if (id == dlg_asset_author_id) {
		m_exportParam.assetExtrasAuthor = item.get_string();
		return true;
	}
	if (id == dlg_asset_select_license_id) {
		// 選択要素より、対応するテキストを取得.
		int index = (int)item.get_selection();
		const std::string str = m_getSelectLicenseToString(index);
		m_exportParam.assetExtrasLicense = str;
		load_dialog_data(dialog);
		return true;
	}

	if (id == dlg_asset_license_id) {
		m_exportParam.assetExtrasLicense = item.get_string();
		return true;
	}
	if (id == dlg_asset_source_id) {
		m_exportParam.assetExtrasSource = item.get_string();
		return true;
	}
	if (id == dlg_export_file_format_id) {
		m_exportParam.exportFileFormat = item.get_selection();
		return true;
	}

	if (id == dlg_output_textures_id) {
		m_exportParam.outputAdditionalTextures = item.get_bool();
		load_dialog_data(dialog);
		return true;
	}
	if (id == dlg_output_textures_engine_type_id) {
		m_exportParam.engineType = (GLTFConverter::engine_type)item.get_selection();
		return true;
	}

	return false;
}


/***********************************************************************/

/**
 * Shade3Dのsurface情報をマテリアルとして格納.
 */
bool CGLTFExporterInterface::m_setMaterialData (sxsdk::surface_class* surface, CMaterialData& materialData, const std::string& smName)
{
	materialData.clear();

	// ユニークなマテリアル名を取得.
	materialData.name = m_sceneData->getUniqueMaterialName(smName);

	// diffuse/reflection/roughness/glow/normalのイメージをあらかじめ合成する.
	CImagesBlend imagesBlend(m_pScene, surface);
	CImagesBlend::IMAGE_BAKE_RESULT blendResult = imagesBlend.blendImages(!m_exportParam.bakeWithoutProcessingTextures);		// 複数のマッピングレイヤのイメージを合成.

	if (blendResult == CImagesBlend::bake_error_mixed_uv_layer) {
		const std::string str = std::string("[") + smName + std::string("] ") + std::string(m_pScene->gettext("bake_msg_error_mixed_uv_layer"));
		m_pScene->message(str);
	}
	if (blendResult == CImagesBlend::bake_mixed_repeat) {
		const std::string str = std::string("[") + smName + std::string("] ") + std::string(m_pScene->gettext("bake_msg_mixed_repeat"));
		m_pScene->message(str);
	}

	materialData.baseColorFactor = sxsdk::rgb_class(0.8f, 0.8f, 0.8f);
	materialData.emissiveFactor  = sxsdk::rgb_class(0, 0, 0);
	materialData.metallicFactor  = 0.0f;
	materialData.roughnessFactor = 1.0f;
	materialData.transparency    = 0.0f;
	materialData.unlit           = false;

	switch (imagesBlend.getAlphaModeType()) {
	case GLTFConverter::alpha_mode_opaque:
		materialData.alphaMode = 1;
		break;
	case GLTFConverter::alpha_mode_mask:
		materialData.alphaMode = 3;
		materialData.alphaCutOff = imagesBlend.getAlphaCutoff();
		break;
	case GLTFConverter::alpha_mode_blend:
		materialData.alphaMode = 2;
		break;
	}

	if (surface->get_no_shading()) {
		materialData.unlit = true;
	}

	// 頂点カラーを持つか.
	{
		const int layersCou = surface->get_number_of_mapping_layers();
		for (int i = 0; i < layersCou; ++i) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(i);
			if (mappingLayer.get_pattern() == sxsdk::enums::vertex_color_pattern) {
				materialData.hasVertexColor = true;
				break;
			}
		}
	}

	//----------------------------------------------------.
	// baseColorを格納.
	// 不透明度テクスチャや透明度テクスチャが存在する場合は、baseColorTextureのAlpha要素に格納される.
	//----------------------------------------------------.
	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::diffuse_mapping;
		const sxsdk::rgb_class factor = imagesBlend.getImageFactor(iType);
		if (imagesBlend.hasImage(iType)) {
			materialData.baseColorFactor = factor;

			// 色をリニアにする.
			if (m_exportParam.convertColorToLinear) {
				MathUtil::convColorLinear(materialData.baseColorFactor.red, materialData.baseColorFactor.green, materialData.baseColorFactor.blue);
			}

			const bool useAlphaTrans = imagesBlend.getDiffuseAlphaTrans();		// DiffuseでAlpha成分に不透明度を使用している場合.

			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.baseColorTexScale = sxsdk::vec2(repeatV.x, repeatV.y);
			materialData.baseColorTexCoord = imagesBlend.getTexCoord(iType);

			sxsdk::image_interface* image = imagesBlend.getImage(iType);
			CImageData imageData;
			imageData.setCustomImage(image);
			imageData.name = imagesBlend.getImageName(iType);
			imageData.name = m_sceneData->getUniqueImageName(imageData.name);
			imageData.materialName = materialData.name;
			imageData.imageMask    = CImageData::gltf_image_mask_base_color;

			int imageIndex = m_sceneData->findSameImage(imageData);
			if (imageIndex < 0) {
				imageIndex = (int)m_sceneData->images.size();
				m_sceneData->images.push_back(imageData);
			}

			if (imageIndex >= 0) {
				materialData.baseColorImageIndex = imageIndex;

				if (useAlphaTrans || (materialData.alphaMode == 2 || materialData.alphaMode == 3)) {
					CImageData& imageData = m_sceneData->images[imageIndex];
					imageData.useBaseColorAlpha = true;
					if (materialData.alphaMode == 1) {	// OPAQUEの場合.
						materialData.alphaMode = 3;		// MASKモードに切り替え.
					}
				}
			}
		} else {
			materialData.baseColorFactor = factor;

			// 色をリニアにする.
			if (m_exportParam.convertColorToLinear) {
				MathUtil::convColorLinear(materialData.baseColorFactor.red, materialData.baseColorFactor.green, materialData.baseColorFactor.blue);
			}
		}
	}

	//----------------------------------------------------.
	// 透明度を持つ場合.
	//----------------------------------------------------.
	{
		const sxsdk::enums::mapping_type iType = MAPPING_TYPE_OPACITY;
		const float transparencyV = imagesBlend.getTransparency();
		if (transparencyV > 0.01f) {
			if (materialData.alphaMode == 1) {			// OPAQUEの場合.
				materialData.alphaMode = 2;				// BLENDモードにする.
			}
			materialData.transparency = transparencyV;
		}
	}

	//----------------------------------------------------.
	// 法線マップを格納.
	//----------------------------------------------------.
	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::normal_mapping;
		const sxsdk::rgb_class factor = imagesBlend.getImageFactor(iType);
		materialData.normalStrength = imagesBlend.getNormalStrength();
		if (imagesBlend.hasImage(iType)) {
			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.normalTexScale = sxsdk::vec2(repeatV.x, repeatV.y);
			materialData.normalTexCoord = imagesBlend.getTexCoord(iType);

			sxsdk::image_interface* image = imagesBlend.getImage(iType);
			CImageData imageData;
			imageData.setCustomImage(image);
			imageData.name = imagesBlend.getImageName(iType);
			imageData.name = m_sceneData->getUniqueImageName(imageData.name);
			imageData.materialName = materialData.name;
			imageData.imageMask    = CImageData::gltf_image_mask_normal;

			int imageIndex = m_sceneData->findSameImage(imageData);
			if (imageIndex < 0) {
				imageIndex = (int)m_sceneData->images.size();
				m_sceneData->images.push_back(imageData);
			}

			if (imageIndex >= 0) {
				materialData.normalImageIndex = imageIndex;
			}
		}
	}

	//----------------------------------------------------.
	// 発光を格納.
	//----------------------------------------------------.
	{
		const sxsdk::enums::mapping_type iType = sxsdk::enums::glow_mapping;
		const sxsdk::rgb_class factor = imagesBlend.getImageFactor(iType);
		materialData.emissiveFactor = factor;
		if (imagesBlend.hasImage(iType)) {
			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.emissiveTexScale = sxsdk::vec2(repeatV.x, repeatV.y);
			materialData.emissiveTexCoord = imagesBlend.getTexCoord(iType);

			sxsdk::image_interface* image = imagesBlend.getImage(iType);
			CImageData imageData;
			imageData.setCustomImage(image);
			imageData.name = imagesBlend.getImageName(iType);
			imageData.name = m_sceneData->getUniqueImageName(imageData.name);
			imageData.materialName = materialData.name;
			imageData.imageMask    = CImageData::gltf_image_mask_emissive;

			int imageIndex = m_sceneData->findSameImage(imageData);
			if (imageIndex < 0) {
				imageIndex = (int)m_sceneData->images.size();
				m_sceneData->images.push_back(imageData);
			}

			if (imageIndex >= 0) {
				materialData.emissiveImageIndex = imageIndex;
			}
		} else {

			// 色をリニアにする.
			if (m_exportParam.convertColorToLinear) {
				MathUtil::convColorLinear(materialData.emissiveFactor.red, materialData.emissiveFactor.green, materialData.emissiveFactor.blue);
			}
		}
	}
	//----------------------------------------------------.
	// メタリックとラフネス(オクルージョン)を格納.
	//----------------------------------------------------.
	{
		const sxsdk::rgb_class metallicFactor  = imagesBlend.getImageFactor(sxsdk::enums::reflection_mapping);
		const sxsdk::rgb_class roughnessFactor = imagesBlend.getImageFactor(sxsdk::enums::roughness_mapping);
		materialData.metallicFactor  = metallicFactor.red;
		materialData.roughnessFactor = roughnessFactor.red;

		const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(sxsdk::enums::reflection_mapping);
		materialData.metallicRoughnessTexScale = sxsdk::vec2(repeatV.x, repeatV.y);
		materialData.metallicRoughnessTexCoord = imagesBlend.getTexCoord(sxsdk::enums::reflection_mapping);

		sxsdk::image_interface* metallicRoughnessImg = imagesBlend.getMetallicRoughnessImage();
		if (metallicRoughnessImg) {
			CImageData imageData;
			imageData.setCustomImage(metallicRoughnessImg);
			imageData.name = smName + std::string("_metallicRoughness");
			imageData.name = m_sceneData->getUniqueImageName(imageData.name);
			imageData.materialName = materialData.name;
			imageData.imageMask    = CImageData::gltf_image_mask_metallic | CImageData::gltf_image_mask_roughness;

			if (imagesBlend.getUseOcclusionInMetallicRoughnessTexture()) {
				imageData.imageMask |= CImageData::gltf_image_mask_occlusion;
			}

			int imageIndex = m_sceneData->findSameImage(imageData);
			if (imageIndex < 0) {
				imageIndex = (int)m_sceneData->images.size();
				m_sceneData->images.push_back(imageData);
			}

			if (imageIndex >= 0) {
				materialData.metallicRoughnessImageIndex = imageIndex;
			}

			// OcclusionがmetalliRoughnessテクスチャの[R]に格納される場合.
			if (imagesBlend.getUseOcclusionInMetallicRoughnessTexture()) {
				materialData.occlusionImageIndex = imageIndex;
				materialData.occlusionTexCoord = materialData.metallicRoughnessTexCoord;
				materialData.occlusionTexScale = materialData.metallicRoughnessTexScale;
				materialData.occlusionStrength = imagesBlend.getImageFactor(MAPPING_TYPE_GLTF_OCCLUSION).red;
			}
		}
	}

	//----------------------------------------------------.
	// オクルージョンを格納.
	//----------------------------------------------------.
	if (!imagesBlend.getUseOcclusionInMetallicRoughnessTexture()) {
		const sxsdk::enums::mapping_type iType = MAPPING_TYPE_GLTF_OCCLUSION;
		const sxsdk::rgb_class factor = imagesBlend.getImageFactor(iType);

		sxsdk::image_interface* image = imagesBlend.getImage(iType);
		if (image) {
			materialData.occlusionStrength = factor.red;
			const sx::vec<int,2> repeatV = imagesBlend.getImageRepeat(iType);
			materialData.occlusionTexScale = sxsdk::vec2(repeatV.x, repeatV.y);
			materialData.occlusionTexCoord = imagesBlend.getTexCoord(iType);

			CImageData imageData;
			imageData.setCustomImage(image);
			imageData.name = imagesBlend.getImageName(iType);
			imageData.name = m_sceneData->getUniqueImageName(imageData.name);
			imageData.materialName = materialData.name;
			imageData.imageMask    = CImageData::gltf_image_mask_occlusion;

			int imageIndex = m_sceneData->findSameImage(imageData);
			if (imageIndex < 0) {
				imageIndex = (int)m_sceneData->images.size();
				m_sceneData->images.push_back(imageData);
			}

			if (imageIndex >= 0) {
				materialData.occlusionImageIndex = imageIndex;
			}
		}
	}

	return true;
}

bool CGLTFExporterInterface::m_setMaterialData (sxsdk::master_surface_class* master_surface, CMaterialData& materialData, const std::string& smName)
{
	materialData.clear();
	try {
		if (!master_surface->has_surface()) return false;

		sxsdk::surface_class* surface = master_surface->get_surface();
		bool ret = m_setMaterialData(surface, materialData, smName);
		if (!ret) return false;
		//materialData.name = std::string(master_surface->get_name());

		materialData.shadeMasterSurface = master_surface;

		// マスターサーフェス名に「doubleSided」が含まれる場合は、doubleSidedを有効化.
		std::string materialName = materialData.name;
		std::transform(materialName.begin(), materialName.end(), materialName.begin(), ::tolower);
		if (materialName.find("doublesided") != std::string::npos) {
			materialData.doubleSided = true;
		}

		return true;
	} catch (...) { }

	return false;
}

/**
 * 指定の形状に割り当てられているマテリアル/イメージを格納.
 * @param[in] shape           対象形状.
 * @param[in] faceGroupIndex  フェイスグループ番号.
 * @return マテリアル番号.
 */
int CGLTFExporterInterface::m_setMaterialCurrentShape (sxsdk::shape_class* shape, const int faceGroupIndex)
{
	// サーフェスを持つ親までたどる.
	sxsdk::shape_class* shape2 = m_shapeStack.getHasSurfaceShape();
	if (shape2 == NULL) shape2 = shape;

	sxsdk::master_surface_class* masterSurface = NULL;

	try {
		if (faceGroupIndex < 0) {
			masterSurface = shape2->get_master_surface();

		} else {
			// ポリゴンメッシュの場合、フェイスグループに割り当てられているマスターサーフェスを取得.
			if (shape->get_type() == sxsdk::enums::polygon_mesh) {
				sxsdk::polygon_mesh_class& pMesh = shape->get_polygon_mesh();
				const int faceGroupCou = pMesh.get_number_of_face_groups();
				if (faceGroupIndex < faceGroupCou) {
					masterSurface = pMesh.get_face_group_surface(faceGroupIndex);
				}
			}
		}

		// 同一マスターサーフェスがすでに格納済みかチェック.
		if (masterSurface) {
			int mIndex = m_sceneData->findSameMaterial(masterSurface);
			if (mIndex >= 0) return mIndex;
		}

		CMaterialData materialData;
		bool ret = false;
		if (masterSurface) {
			ret = m_setMaterialData(masterSurface, materialData, masterSurface->get_name());
		} else {
			if (shape2->get_has_surface_attributes()) {
				ret = m_setMaterialData(shape2->get_surface(), materialData, shape->get_name());
			}
		}
		if (!ret) {
			// デフォルトのサーフェス.
			materialData.clear();
		}
		if (materialData.name == "") materialData.name = m_sceneData->getUniqueMaterialName("material");

		int materialIndex = m_sceneData->findSameMaterial(materialData);
		if (materialIndex < 0) {
			materialIndex = (int)m_sceneData->materials.size();
			m_sceneData->materials.push_back(materialData);
		}

		return materialIndex;
	} catch (...) { }

	return -1;
}

/**
 * 形状のハンドルに対応するノード番号を取得.
 */
int CGLTFExporterInterface::m_findNodeIndexFromShapeHandle (void* handle)
{
	const size_t nodesCou = m_sceneData->nodes.size();
	int nodeIndex = -1;

	for (size_t i = 0; i < nodesCou; ++i) {
		CNodeData& nodeD = m_sceneData->nodes[i];
		if (nodeD.pShapeHandle == handle) {
			nodeIndex = i;
			break;
		}
	}
	return nodeIndex;
}

/**
 * 指定の配列内のボーン要素から一番親を探す.
 */
sxsdk::shape_class* CGLTFExporterInterface::m_getBoneRoot (const std::vector<sxsdk::shape_class *>& shapes)
{
	sxsdk::shape_class* pBoneRoot = NULL;
	int minDepth = -1;

	const size_t sCou = shapes.size();
	for (size_t i = 0; i < sCou; ++i) {
		sxsdk::shape_class *pS = shapes[i];
		const int depth = Shade3DUtil::getShapeHierarchyDepth(pS);
		if (pBoneRoot == NULL || depth < minDepth) {
			pBoneRoot = pS;
			minDepth  = depth;
		}
	}
	return pBoneRoot;
}

/**
 * ルートボーンから子をたどり、要素をマップに格納していく(再帰).
 * @param[in]  depth      再帰のdepth.
 * @param[in]  pShape     ボーン形状.
 * @param[out] mapD       first : shape_classのhandle、second : 出現回数.
 */
void CGLTFExporterInterface::m_storeChildBonesLoop (const int depth, sxsdk::shape_class* pShape, std::map<void *, int>& mapD)
{
	const int type = pShape->get_type();
	if (type != sxsdk::enums::part) return;
	const int partType = pShape->get_part().get_part_type();
	if (partType != sxsdk::enums::bone_joint && partType != sxsdk::enums::ball_joint) return;

	mapD[pShape->get_handle()]++;

	if (!pShape->has_son()) return;

	sxsdk::shape_class* pS = pShape->get_son();
	while (pS->has_bro()) {
		pS = pS->get_bro();
		m_storeChildBonesLoop(depth + 1, pS, mapD);
	}
}

/**
 * ポリゴンメッシュに格納したスキン情報より、スキン情報をm_sceneDataに格納する.
 */
void CGLTFExporterInterface::m_setSkinsFromMeshes ()
{
	const size_t meshsCou = m_sceneData->meshes.size();
	if (meshsCou == 0) return;
	m_sceneData->skins.clear();

	for (size_t meshLoop = 0; meshLoop < meshsCou; ++meshLoop) {
		CMeshData& meshD = m_sceneData->meshes[meshLoop];
		const size_t primCou = meshD.primitives.size();
		if (primCou <= 0 || meshD.primitives[0].skinJointsHandle.empty()) continue;

		// メッシュに対応するノード番号を取得.
		const int meshNodeIndex = m_findNodeIndexFromShapeHandle(meshD.pMeshHandle);

		std::map<void *, int> shapeHandleMap;

		bool errF = false;
		for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
			CPrimitiveData& primD = meshD.primitives[primLoop];
			const size_t versCou = primD.vertices.size();
			if (versCou != primD.skinJointsHandle.size() || versCou != primD.skinWeights.size()) {
				errF = true;
				break;
			}

			// 形状のハンドルのmapを作成.
			for (size_t i = 0; i < versCou; ++i) {
				const sx::vec<void *,4>& hD = primD.skinJointsHandle[i];
				for (int j = 0; j < 4; ++j) {
					if (hD[j] != NULL) {
						if (shapeHandleMap.count(hD[j]) == 0) {
							shapeHandleMap[hD[j]] = 0;
						} else {
							shapeHandleMap[hD[j]]++;
						}
					}
				}
			}
		}

		// shapeHandleMap[]に格納されたジョイントハンドルのうち、ボーンのルートを探し.
		// ボーンの階層構造の末端まで漏れがないようにmapに格納.
		sxsdk::shape_class* pBoneRoot = NULL;
		int skeletonID = -1;
		if (!errF && !shapeHandleMap.empty()) {
			// sxsdk::shape_class * に変換.
			std::vector<sxsdk::shape_class *> tmpShapes;
			const size_t jCou = shapeHandleMap.size();
			tmpShapes.resize(jCou, NULL);
			auto iter = shapeHandleMap.begin();
			for (size_t i = 0; i < jCou; ++i) {
				tmpShapes[i] = m_pScene->get_shape_by_handle(iter->first);
				iter++;
			}

			// ボーンルートを取得.
			pBoneRoot = m_getBoneRoot(tmpShapes);

			// 対応する形状のノード番号を取得.
			if (pBoneRoot) {
				skeletonID = m_findNodeIndexFromShapeHandle(pBoneRoot->get_handle());
			}

			// pBoneRootからたどり、先端のボーンまでmapに格納していく.
			m_storeChildBonesLoop(0, pBoneRoot, shapeHandleMap);
		}

		const size_t jointsCou = shapeHandleMap.size();

		// スキン情報を破棄する.
		if (errF || jointsCou == 0) {
			for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
				CPrimitiveData& primD = meshD.primitives[primLoop];
				primD.skinJointsHandle.clear();
				primD.skinWeights.clear();
				primD.skinJoints.clear();
			}
			continue;
		}

		// Skinとして採用するノード番号を取得し、skin情報として格納.
		const size_t skinIndex = m_sceneData->skins.size();
		CSkinData skinData;
		std::vector<void *> jointHandleList;

		skinData.joints.resize(jointsCou);
		skinData.inverseBindMatrices.resize(jointsCou);
		jointHandleList.resize(jointsCou);
		auto iter = shapeHandleMap.begin();
		for (size_t i = 0; i < jointsCou; ++i) {
			const int nodeIndex = m_findNodeIndexFromShapeHandle(iter->first);
			skinData.joints[i] = nodeIndex;
			shapeHandleMap[iter->first] = i;
			jointHandleList[i] = iter->first;

			// ボーンの逆変換行列を指定.
			const CNodeData& nodeD = m_sceneData->nodes[nodeIndex];
			//const sxsdk::mat4 m = (nodeIndex == skeletonID) ? (m_sceneData->getLocalToWorldMatrix(nodeIndex)) : nodeD.getMatrix();
			const sxsdk::mat4 m = m_sceneData->getLocalToWorldMatrix(nodeIndex);

			skinData.inverseBindMatrices[i] = inv(m);

			iter++;
		}

		// node番号の若い順に入れ替え.
		// 一部のGLTFビュワーではnodeの順番が前後しているとうまいことSkeletonが表示されないため.
		for (size_t i = 0; i < jointsCou; ++i) {
			for (size_t j = i + 1; j < jointsCou; ++j) {
				if (skinData.joints[i] > skinData.joints[j]) {
					std::swap(skinData.joints[i], skinData.joints[j]);
					std::swap(shapeHandleMap[jointHandleList[i]], shapeHandleMap[jointHandleList[j]]);
					std::swap(jointHandleList[i], jointHandleList[j]);
					{
						const sxsdk::mat4 m = skinData.inverseBindMatrices[j];
						skinData.inverseBindMatrices[j] = skinData.inverseBindMatrices[i];
						skinData.inverseBindMatrices[i] = m;
					}
				}
			}
		}

		skinData.meshIndex  = meshLoop;
		skinData.skeletonID = skeletonID;
		m_sceneData->skins.push_back(skinData);

		// skinで参照しているインデックスに置きかえる.
		for (size_t primLoop = 0; primLoop < primCou; ++primLoop) {
			CPrimitiveData& primD = meshD.primitives[primLoop];
			const size_t versCou = primD.vertices.size();

			primD.skinJoints.resize(versCou, sx::vec<int,4>(0, 0, 0, 0));
			for (size_t i = 0; i < versCou; ++i) {
				const sx::vec<void *,4>& hD = primD.skinJointsHandle[i];
				const sxsdk::vec4& weightV  = primD.skinWeights[i];
				for (int j = 0; j < 4; ++j) {
					if (hD[j] != NULL && !MathUtil::isZero(weightV[j])) {
						primD.skinJoints[i][j] = shapeHandleMap[ hD[j] ];
					}
				}
			}
		}

		if (meshNodeIndex >= 0) {
			m_sceneData->nodes[meshNodeIndex].skinIndex = skinIndex;
		}
	}
}

/**
 * ボーンよりアニメーション情報を格納.
 */
void CGLTFExporterInterface::m_setAnimations ()
{
	m_sceneData->animations.clear();
	const size_t nodesCou =  m_sceneData->nodes.size();
	if (nodesCou <= 1) return;

	// シーケンス情報を取得.
	// startFrame - endFrame の間のキーフレームを採用する.
	const float frameRate = (float)(m_pScene->get_frame_rate());
	const int totalFrames = m_pScene->get_total_frames();
	int startFrame = m_pScene->get_start_frame();
	if (startFrame < 0) startFrame = 0;
	int endFrame = m_pScene->get_end_frame();
	if (endFrame < 0) endFrame = totalFrames;

	for (size_t nLoop = 0; nLoop < nodesCou; ++nLoop) {
		const CNodeData& nodeD = m_sceneData->nodes[nLoop];
		sxsdk::shape_class* shape = m_pScene->get_shape_by_handle(nodeD.pShapeHandle);
		if (!shape || !Shade3DUtil::isSupportJoint(*shape)) continue;
		if (!shape->has_motion()) continue;

		const std::string name = shape->get_name();
		const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
		sxsdk::vec3 boneLCenter = Shade3DUtil::getBoneBallJointCenterL(*shape);

		// ボーンの場合.
		sxsdk::mat4 boneLMat = sxsdk::mat4::identity;
		bool isBoneJoint = false;
		if (Shade3DUtil::isBone(*shape)) {
			isBoneJoint = true;
			sxsdk::part_class& part = shape->get_part();
			compointer<sxsdk::bone_joint_interface> bone(shape->get_bone_joint_interface());
			boneLMat = bone->get_matrix();
			boneLCenter = sxsdk::vec3(0, 0, 0);
		}

		std::vector<float> tmpKeyframes;
		std::vector<sxsdk::vec3> tmpOffsets;
		std::vector<sxsdk::vec3> tmpRotations;
		std::vector<bool> tmpLinears;
		try {
			compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
			const int pointsCou = motion->get_number_of_motion_points();
			if (pointsCou == 0) continue;

			float prevRotatorVal = 0.0f;
			sxsdk::vec3 eularV;
			float oldSeqPos = -1.0f;
			for (int i = 0; i < pointsCou; ++i) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(i));
				float seqPos = motionPoint->get_sequence();
				if (seqPos < (float)startFrame || seqPos > (float)endFrame) continue;

				// 同一のフレーム位置が格納済みの場合はスキップ.
				// glTFの処理では、同一のフレーム位置である場合はエラーになる.
				if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;

				const bool linearF = motionPoint->get_corner();

				// ボーンまたはボールジョイント時.
				const sxsdk::vec3 offset  = motionPoint->get_offset();
				sxsdk::vec3 offset2       = offset + boneLCenter;
				sxsdk::quaternion_class q = motionPoint->get_rotation();
				q.get_euler(eularV);

				if (isBoneJoint) {
					// ボーンの変換行列は、回転 * ローカル変換行列 * オフセット.
					sxsdk::mat4 m = sxsdk::mat4::rotate(eularV) * boneLMat * sxsdk::mat4::translate(offset2);
					sxsdk::vec3 scale, shear, rotate, trans;
					m.unmatrix(scale, shear, rotate, trans);
					offset2 = trans;
					eularV  = rotate;
				}

				tmpKeyframes.push_back(seqPos);
				tmpOffsets.push_back(offset2);
				tmpRotations.push_back(eularV);
				tmpLinears.push_back(linearF);

				oldSeqPos = seqPos;
			}

			if (tmpKeyframes.empty()) continue;

			// 移動(offset)/回転要素をキーフレームとして格納.
			const int transI = (int)m_sceneData->animations.channelData.size();
			m_sceneData->animations.channelData.push_back(CAnimChannelData());
			m_sceneData->animations.samplerData.push_back(CAnimSamplerData());
			const int rotationI = (int)m_sceneData->animations.channelData.size();
			m_sceneData->animations.channelData.push_back(CAnimChannelData());
			m_sceneData->animations.samplerData.push_back(CAnimSamplerData());

			CAnimChannelData& transChannelD    = m_sceneData->animations.channelData[transI];
			CAnimSamplerData& transSamplerD    = m_sceneData->animations.samplerData[transI];
			CAnimChannelData& rotationChannelD = m_sceneData->animations.channelData[rotationI];
			CAnimSamplerData& rotationSamplerD = m_sceneData->animations.samplerData[rotationI];

			transChannelD.pathType           = CAnimChannelData::path_type_translation;
			transChannelD.targetNodeIndex    = nLoop;
			transChannelD.samplerIndex       = transI;
			rotationChannelD.pathType        = CAnimChannelData::path_type_rotation;
			rotationChannelD.targetNodeIndex = nLoop;
			rotationChannelD.samplerIndex    = rotationI;

			for (size_t i = 0; i < tmpKeyframes.size(); ++i) {
				const float seqPos = tmpKeyframes[i] / frameRate;		// 秒単位に変換.

				transSamplerD.inputData.push_back(seqPos);
				sxsdk::vec3 offset = tmpOffsets[i] * 0.001f;		// メートルに変換.;
				for (int j = 0; j < 3; ++j) transSamplerD.outputData.push_back(offset[j]);
				sxsdk::quaternion_class q(tmpRotations[i]);

				rotationSamplerD.inputData.push_back(seqPos);
				rotationSamplerD.outputData.push_back(q.x);
				rotationSamplerD.outputData.push_back(q.y);
				rotationSamplerD.outputData.push_back(q.z);
				rotationSamplerD.outputData.push_back(-q.w);

				rotationSamplerD.interpolationType = (tmpLinears[i]) ? CAnimSamplerData::interpolation_type_linear : CAnimSamplerData::interpolation_type_smooth;
			}

		} catch (...) { }
	}
}

/**
 * ボーンはユニーク名である必要がある。同一名がある場合はリネーム.
 */
void CGLTFExporterInterface::m_renameUniqueNameInBone ()
{
	const size_t nodesCou =  m_sceneData->nodes.size();
	if (nodesCou <= 1) return;

	std::vector<std::string> namesList;
	for (size_t nLoop = 0; nLoop < nodesCou; ++nLoop) {
		CNodeData& nodeD = m_sceneData->nodes[nLoop];
		sxsdk::shape_class* shape = m_pScene->get_shape_by_handle(nodeD.pShapeHandle);
		if (!shape || !Shade3DUtil::isSupportJoint(*shape)) continue;
		if (!shape->has_motion()) continue;

		// 同一名が存在する場合、連番の名前を付ける.
		std::string name = nodeD.name;
		{
			int cou = 0;
			bool chkSameF = true;
			while(true) {
				chkSameF = false;
				for (size_t i = 0; i < namesList.size(); ++i) {
					if (namesList[i] == name) {
						chkSameF = true;
						break;
					}
				}
				if (!chkSameF) break;
				cou++;
				name = nodeD.name + std::string("_") + std::to_string(cou);
			}
			namesList.push_back(name);
			nodeD.name = name;
		}
	}
}

