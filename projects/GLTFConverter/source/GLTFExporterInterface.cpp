﻿/**
 * GLTFエクスポータ.
 */
#include "GLTFExporterInterface.h"
#include "GLTFSaver.h"
#include "SceneData.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"

#include <iostream>
#include <map>

enum
{
	dlg_output_texture_id = 101,			// テクスチャ出力:|マスターサーフェス名の拡張子指定を参照|pngに置き換え|jpegに置き換え.
	dlg_output_bones_and_skins_id = 102,	// ボーンとスキンを出力.
	dlg_output_vertex_color_id = 103,		// 頂点カラーを出力.
	dlg_output_animation_id = 104,			// アニメーションを出力.
};

CGLTFExporterInterface::CGLTFExporterInterface (sxsdk::shade_interface& shade) : shade(shade)
{
	m_dlgOK = false;
}

CGLTFExporterInterface::~CGLTFExporterInterface ()
{
}

/**
 * ファイル拡張子.
 */
const char *CGLTFExporterInterface::get_file_extension (void *aux)
{
	return "glb";
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
	m_shapeStack.clear();
	m_currentDepth = 0;
	m_sceneData.reset(new CSceneData());

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

	// エクスポートを開始.
	plugin_exporter->do_export();

	// 元のシーケンスモードに戻す.
	if (m_exportParam.outputBonesAndSkins) {
		if (oldSequenceMode) {
			m_pScene->set_sequence_mode(oldSequenceMode);
			m_pScene->set_dirty(oldDirty);
		}
	}
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

	// アニメーション情報を格納.
	if (m_exportParam.outputAnimation) {
		m_setAnimations();
	}

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

	// カレントの形状管理クラスのポインタを取得.
	m_pCurrentShape        = m_pluginExporter->get_current_shape();
	const sxsdk::mat4 gMat = m_pluginExporter->get_transformation();
	const std::string name(m_pCurrentShape->get_name());

	// 形状により、スキップする形状を判断.
	const int type = m_pCurrentShape->get_type();
	m_skip = m_checkSkipShape(m_pCurrentShape);

	m_shapeStack.push(m_currentDepth, m_pCurrentShape, gMat);

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
		sxsdk::mat4 m = m_pCurrentShape->get_transformation();
		if (type == sxsdk::enums::polygon_mesh) m = sxsdk::mat4::identity;

		const std::string name = std::string(m_pCurrentShape->get_name());
		const int nodeIndex = m_sceneData->beginNode(name, m);

		// 形状に対応するハンドルを保持 (出力前に、スキンでのノード番号取得で使用).
		m_sceneData->nodes[nodeIndex].pShapeHandle = m_pCurrentShape->get_handle();
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
	if (skin) {
		// スキン変換前の座標値を計算.
		if (m_exportParam.outputBonesAndSkins) {
			const sxsdk::mat4 skin_m = skin->get_skin_world_matrix();
			const sxsdk::mat4 skin_m_inv = inv(skin_m);
			sxsdk::vec4 v4 = sxsdk::vec4(pos, 1) * m_LWMat * skin_m_inv;
			pos = sxsdk::vec3(v4.x, v4.y, v4.z);
		}
	}

	m_meshData.vertices[i] = (pos * m_spMat) * 0.001f;		// mm ==> m変換.

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
				if (!MathUtil::isZero(sumWeight) && !MathUtil::isZero(1.0f - sumWeight)) {
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
	}

	if (n_list == 3) {
		for (int i = 0; i < 3; ++i) {
			m_meshData.triangleIndices.push_back(indicesList[i]);
		}
		for (int i = 0; i < 3; ++i) {
			m_meshData.triangleNormals.push_back(normalsList[i]);
		}
		for (int i = 0; i < 3; ++i) {
			m_meshData.triangleUV0.push_back(uvs0List[i]);
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

	if (n_list == 3 && layer_index == 0) {
		for (int i = 0; i < n_list; ++i) {
			const sxsdk::rgba_class col = vertex_colors[i];
			m_meshData.triangleColor0.push_back(sxsdk::vec4(col.red, col.green, col.blue, col.alpha));
		}
	}
}

/**
 * ポリゴンメッシュの終了時に呼ばれる.
 */
void CGLTFExporterInterface::end_polymesh (void *)
{
	if (m_skip) return;

	// m_meshDataからGLTFの形式にコンバートして格納.
	if (!m_meshData.triangleIndices.empty() && !m_meshData.vertices.empty()) {
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
		} else {
			// フェイスグループを死闘しているポリゴンメッシュの場合、ノードを作成してそれに分けて格納.
			m_storeMeshesWithFaceGroup();
		}
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
	const int primitivesCou = CPrimitiveData::convert(m_meshData, primitivesDataList, faceGroupIndexList);
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
}

/**
 * フェイスグループごとの面列挙前に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh_face_group (int face_group_index, void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = face_group_index;
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
void CGLTFExporterInterface::initialize_dialog (sxsdk::dialog_interface& dialog, void*)
{
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

	return false;
}


/***********************************************************************/

/**
 * Shade3Dのsurface情報をマテリアルとして格納.
 */
bool CGLTFExporterInterface::m_setMaterialData (sxsdk::surface_class* surface, CMaterialData& materialData)
{
	materialData.clear();

	try {
		materialData.baseColorFactor = sxsdk::rgb_class(0.8f, 0.8f, 0.8f);
		materialData.emissionFactor  = sxsdk::rgb_class(0, 0, 0);
		materialData.metallicFactor  = 0.0f;
		materialData.roughnessFactor = 1.0f;

		if (surface->get_has_diffuse()) {
			materialData.baseColorFactor = (surface->get_diffuse_color()) * (surface->get_diffuse());
		}

		if (surface->get_has_glow()) {
			materialData.emissionFactor = (surface->get_glow_color()) * (surface->get_glow());
		}

		if (surface->get_has_reflection()) {
			materialData.metallicFactor = std::max(0.0f, std::min(surface->get_reflection(), 1.0f));
		}

		if (surface->get_has_roughness()) {
			if (materialData.metallicFactor > 0.0f) {
				materialData.roughnessFactor = std::max(0.0f, std::min(surface->get_roughness(), 1.0f));
			}
		}

		// マッピングレイヤでイメージのテクスチャの指定がある場合.
		std::vector< sx::vec<int,2> > diffuseMappingIndexList, reflectionMappingIndexList, roughnessMappingIndexList, normalMappingIndexList, emissionMappingIndexList;
		const int mappingLayerCou = surface->get_number_of_mapping_layers();
		for (int i = 0; i < mappingLayerCou; ++i) {
			sxsdk::mapping_layer_class& mappingLayer = surface->mapping_layer(i);
			if (mappingLayer.get_pattern() != sxsdk::enums::image_pattern) continue;
			const float weight = mappingLayer.get_weight();
			if (MathUtil::isZero(weight)) continue;

			const int type = mappingLayer.get_type();
			if (type != sxsdk::enums::diffuse_mapping && type != sxsdk::enums::reflection_mapping &&
				type != sxsdk::enums::roughness_mapping && type != sxsdk::enums::normal_mapping &&
				type != sxsdk::enums::glow_mapping) continue;

			// master_imageクラスを取得.
			compointer<sxsdk::image_interface> image(mappingLayer.get_image_interface());
			sxsdk::master_image_class* masterImage = Shade3DUtil::getMasterImageFromImage(m_pScene, image);
			if (!masterImage) continue;

			int imageIndex = -1;
			{
				CImageData imageData;
				imageData.m_shadeMasterImage = masterImage;
				imageData.width  = image->get_size().x;
				imageData.height = image->get_size().y;
				imageData.name   = std::string(masterImage->get_name());
				imageIndex = m_sceneData->findSameImage(imageData);
				if (imageIndex < 0) {
					imageIndex = (int)m_sceneData->images.size();
					m_sceneData->images.push_back(imageData);
				}
			}

			switch (type) {
			case sxsdk::enums::diffuse_mapping:
				diffuseMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
				materialData.baseColorImageIndex = imageIndex;
				materialData.baseColorTexCoord   = mappingLayer.get_uv_mapping();
				materialData.baseColorTexScale   = sxsdk::vec2(mappingLayer.get_repetition_x(), mappingLayer.get_repetition_y());

				// アルファ透過する場合.
				if (mappingLayer.get_channel_mix() == sxsdk::enums::mapping_transparent_alpha_mode) {
					materialData.alphaMode = 3;			// ALPHA_MASK : アルファを考慮.
					materialData.alphaCutOff = 0.9f;

					CImageData& imageData = m_sceneData->images[imageIndex];
					imageData.useBaseColorAlpha = true;
				}

				break;

			case sxsdk::enums::reflection_mapping:
				reflectionMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
				break;

			case sxsdk::enums::roughness_mapping:
				roughnessMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
				break;

			case sxsdk::enums::normal_mapping:
				normalMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
				materialData.normalImageIndex = imageIndex;
				materialData.normalTexCoord   = mappingLayer.get_uv_mapping();
				materialData.normalTexScale   = sxsdk::vec2(mappingLayer.get_repetition_x(), mappingLayer.get_repetition_y());
				break;

			case sxsdk::enums::glow_mapping:
				emissionMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
				materialData.emissionImageIndex = imageIndex;
				materialData.emissionTexCoord   = mappingLayer.get_uv_mapping();
				materialData.emissionTexScale   = sxsdk::vec2(mappingLayer.get_repetition_x(), mappingLayer.get_repetition_y());
				break;
			}
		}

		return true;

	} catch (...) { }

	return false;
}

bool CGLTFExporterInterface::m_setMaterialData (sxsdk::master_surface_class* master_surface, CMaterialData& materialData)
{
	materialData.clear();
	try {
		if (!master_surface->has_surface()) return false;

		sxsdk::surface_class* surface = master_surface->get_surface();
		bool ret = m_setMaterialData(surface, materialData);
		if (!ret) return false;
		materialData.name = std::string(master_surface->get_name());

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

		CMaterialData materialData;
		bool ret = false;
		if (masterSurface) {
			ret = m_setMaterialData(masterSurface, materialData);
		} else {
			if (shape2->get_has_surface_attributes()) {
				ret = m_setMaterialData(shape2->get_surface(), materialData);
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
				for (int j = 0; j < 4; ++j) {
					if (hD[j] != NULL) {
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
		if (!shape || !Shade3DUtil::isBone(*shape)) continue;
		if (!shape->has_motion()) continue;

		const std::string name = shape->get_name();
		const sxsdk::vec3 boneWCenter = Shade3DUtil::getBoneCenter(*shape, NULL);
		const sxsdk::mat4 lwMat = shape->get_local_to_world_matrix();
		const sxsdk::vec3 boneLCenter = (boneWCenter * inv(lwMat));

		try {
			compointer<sxsdk::motion_interface> motion(shape->get_motion_interface());
			const int pointsCou = motion->get_number_of_motion_points();
			if (pointsCou == 0) continue;

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

			float oldSeqPos = -1.0f;
			for (int i = 0; i < pointsCou; ++i) {
				compointer<sxsdk::motion_point_interface> motionPoint(motion->get_motion_point_interface(i));
				float seqPos = motionPoint->get_sequence();
				if (seqPos < (float)startFrame || seqPos > (float)endFrame) continue;
				seqPos /= frameRate;		// 秒単位に変換.

				// 同一のフレーム位置が格納済みの場合はスキップ.
				// glTFの処理では、同一のフレーム位置である場合はエラーになる.
				if (oldSeqPos < 0.0f) oldSeqPos = seqPos;
				if (MathUtil::isZero(seqPos - oldSeqPos)) continue;
				oldSeqPos = seqPos;

				const sxsdk::vec3 offset        = motionPoint->get_offset();
				sxsdk::vec3 offset2             = (offset + boneLCenter) * 0.001f;		// メートルに変換.
				const sxsdk::quaternion_class q = motionPoint->get_rotation();

				transSamplerD.inputData.push_back(seqPos);
				for (int j = 0; j < 3; ++j) transSamplerD.outputData.push_back(offset2[j]);

				rotationSamplerD.inputData.push_back(seqPos);
				rotationSamplerD.outputData.push_back(q.x);
				rotationSamplerD.outputData.push_back(q.y);
				rotationSamplerD.outputData.push_back(q.z);
				rotationSamplerD.outputData.push_back(-q.w);
			}
		} catch (...) { }
	}
}
