/**
 * GLTFエクスポータ.
 */
#include "GLTFExporterInterface.h"
#include "GLTFSaver.h"
#include "SceneData.h"

CGLTFExporterInterface::CGLTFExporterInterface (sxsdk::shade_interface& shade) : shade(shade)
{
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
	return "GLTF";
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
		m_sceneData->assetGenerator = "GLTF Converter for Shade3D";
		m_sceneData->filePath       = std::string(m_pluginExporter->get_file_path());
	}

	m_pScene = scene;

	// エクスポートを開始.
	plugin_exporter->do_export();
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

	CGLTFSaver gltfSaver;
	if (gltfSaver.saveGLTF(m_sceneData->filePath, &(*m_sceneData))) {
		shade.message("export success!");
	}
}

/**
 * カレント形状の処理の開始.
 */
void CGLTFExporterInterface::begin (void *)
{
	m_pCurrentShape = NULL;
	m_skip = false;

	// カレントの形状管理クラスのポインタを取得.
	m_pCurrentShape        = m_pluginExporter->get_current_shape();
	const sxsdk::mat4 gMat = m_pluginExporter->get_transformation();

	if (m_pCurrentShape->get_render_flag() == 0) m_skip = true;
	const int type = m_pCurrentShape->get_type();
	if (type == sxsdk::enums::part) {
		const int partType = m_pCurrentShape->get_part().get_part_type();
		if (partType == sxsdk::enums::master_surface_part || partType == sxsdk::enums::master_image_part) m_skip = true;
	} else {
		if (type == sxsdk::enums::master_surface || type == sxsdk::enums::master_image) m_skip = true;
	}

	m_shapeStack.push(m_currentDepth, m_pCurrentShape, gMat);

	m_spMat = sxsdk::mat4::identity;
	m_currentLWMatrix = gMat;
	m_currentDepth++;

	if (!m_skip) {
		sxsdk::mat4 m = m_pCurrentShape->get_transformation();
		if (type == sxsdk::enums::polygon_mesh) m = sxsdk::mat4::identity;

		const std::string name = std::string(m_pCurrentShape->get_name());
		m_sceneData->beginNode(name, m);
	}
}

/**
 * カレント形状の処理の終了.
 */
void CGLTFExporterInterface::end (void *)
{
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
	}

	if (!m_skip) {
		m_sceneData->endNode();
	}

	m_skip = false;
}

/**
 * カレント形状が掃引体の上面部分の場合、掃引に相当する変換マトリクスが渡される.
 */
void CGLTFExporterInterface::set_transformation (const sxsdk::mat4 &t, void *)
{
	m_spMat = t;
}

/**
 * カレント形状が掃引体の上面部分の場合の行列クリア.
 */
void CGLTFExporterInterface::clear_transformation (void *)
{
	m_spMat = sxsdk::mat4::identity;
}

/**
 * ポリゴンメッシュの開始時に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh (void *)
{
	if (m_skip) return;
	m_currentFaceGroupIndex = -1;

	m_LWMat = m_spMat * m_currentLWMatrix;

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
}

/**
 * 頂点が格納されるときに呼ばれる.
 */
void CGLTFExporterInterface::polymesh_vertex (int i, const sxsdk::vec3 &v, const sxsdk::skin_class *skin)
{
	if (m_skip) return;

	// 座標値をワールド座標変換する.
	sxsdk::vec3 pos = v;
	//if (skin) pos = pos * (skin->get_skin_world_matrix());
	pos = pos * m_LWMat;

	m_meshData.vertices[i] = (v * 0.001f) * m_spMat;		// mm ==> m変換.
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
		const int curNodeIndex = m_sceneData->getCurrentNodeIndex();
		const int meshIndex = (int)m_sceneData->meshes.size();
		m_sceneData->meshes.push_back(CMeshData());
		CMeshData& meshD = m_sceneData->meshes.back();
		meshD.convert(m_meshData);

		// マテリアル情報を格納.
		meshD.materialIndex = m_setMaterialCurrentShape(m_pCurrentShape);

		m_sceneData->nodes[curNodeIndex].meshIndex = meshIndex;
	}
}

/**
 * 面情報格納前に呼ばれる.
 */
void CGLTFExporterInterface::begin_polymesh_face2 (int n, int number_of_face_groups, void *)
{
	if (m_skip) return;
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
		return true;
	} catch (...) { }

	return false;
}

/**
 * 指定の形状に割り当てられているマテリアルを格納.
 * @param[in] shape  対象形状.
 * @return マテリアル番号.
 */
int CGLTFExporterInterface::m_setMaterialCurrentShape (sxsdk::shape_class* shape)
{
	try {
		sxsdk::master_surface_class* masterSurface = shape->get_master_surface();
		CMaterialData materialData;
		bool ret = false;
		if (masterSurface) {
			ret = m_setMaterialData(masterSurface, materialData);
		} else {
			if (shape->get_has_surface_attributes()) {
				ret = m_setMaterialData(shape->get_surface(), materialData);
			}
		}
		if (!ret) {
			// デフォルトのサーフェス.
			materialData.clear();
		}
		if (materialData.name == "") materialData.name = "material";

		int materialIndex = m_sceneData->findSameMaterial(materialData);
		if (materialIndex < 0) {
			materialIndex = (int)m_sceneData->materials.size();
			m_sceneData->materials.push_back(materialData);
		}

		return materialIndex;
	} catch (...) { }

	return -1;
}

