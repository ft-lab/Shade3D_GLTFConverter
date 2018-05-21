/**
 * GLTFエクスポータ.
 */
#include "GLTFExporterInterface.h"
#include "GLTFSaver.h"
#include "SceneData.h"
#include "MathUtil.h"
#include "Shade3DUtil.h"

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

	shade.message("----- GLTF Exporter -----");

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

	CGLTFSaver gltfSaver(&shade);
	if (gltfSaver.saveGLTF(m_sceneData->filePath, &(*m_sceneData))) {
		shade.message("export success!");
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
		m_sceneData->beginNode(name, m);
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

	m_meshData.vertices[i] = (v * m_spMat) * 0.001f;		// mm ==> m変換.
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
		m_meshData.faceGroupIndex.push_back(m_currentFaceGroupIndex);
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
			const int meshIndex = (int)m_sceneData->meshes.size();
			m_sceneData->meshes.push_back(CMeshData());
			CMeshData& meshD = m_sceneData->meshes.back();
			meshD.convert(m_meshData);

			// マテリアル情報を格納.
			meshD.materialIndex = m_setMaterialCurrentShape(m_pCurrentShape);

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
	const int curNodeIndex = m_sceneData->getCurrentNodeIndex();
	const int meshIndex = (int)m_sceneData->meshes.size();

	// フェイスグループごとにメッシュを分離.
	std::vector<CMeshData> meshDataList;
	std::vector<int> faceGroupIndexList;
	const int meshesCou = CMeshData::convert(m_meshData, meshDataList, faceGroupIndexList);
	if (meshesCou == 0) return;

	// マテリアル情報を格納.
	for (int i = 0; i < meshesCou; ++i) {
		const int faceGroupIndex = faceGroupIndexList[i];
		// 形状に割り当てられているマテリアル情報を格納.
		meshDataList[i].materialIndex = m_setMaterialCurrentShape(m_pCurrentShape, faceGroupIndex);
	}

	// noesではノードで分け、メッシュを格納.
	for (int i = 0; i < meshesCou; ++i) {
		CMeshData& meshD = meshDataList[i];

		const std::string name = std::string(m_pCurrentShape->get_name()) + std::string("_") + std::to_string(i);
		m_sceneData->beginNode(name);

		{
			const int curNodeIndex = m_sceneData->getCurrentNodeIndex();
			const int meshIndex = (int)m_sceneData->meshes.size();
			meshD.name = name;
			m_sceneData->meshes.push_back(meshD);
			m_sceneData->nodes[curNodeIndex].meshIndex = meshIndex;
		}

		m_sceneData->endNode();
	}
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
				break;

			case sxsdk::enums::glow_mapping:
				emissionMappingIndexList.push_back(sx::vec<int,2>(i, imageIndex));
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

		// test.
		// マスターサーフェス名に「doubleSided」が含まれる場合は、doubleSidedを有効化.
		if (materialData.name.find_first_of("doubleSided") != std::string::npos) {
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

