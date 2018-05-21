﻿/**
 * GLTFファイルを読み込むインポータクラス.
 */
#include "GLTFImporterInterface.h"
#include "GLTFLoader.h"
#include "SceneData.h"

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
	if (index == 0) return "GLTF";
	if (index == 1) return "GLTF";

	return 0;
}

/**
 * @brief  前処理(ダイアログを出す場合など).
 */
void CGLTFImporterInterface::do_pre_import (const sxsdk::mat4 &t, void *path)
{
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
	if (!gltfLoader.loadGLTF(fileName, &sceneData)) return;

	shade.message("----- GLTF Importer -----");
	shade.message(std::string("File : ") + fileName);
	shade.message(std::string("Asset generator : ") + sceneData.assetGenerator);
	shade.message(std::string("Asset version : ") + sceneData.assetVersion);
	shade.message(std::string("Asset copyright : ") + sceneData.assetCopyRight);

	shade.message(std::string("Meshes : ") + std::to_string(sceneData.meshes.size()));
	shade.message(std::string("Materials : ") + std::to_string(sceneData.materials.size()));
	shade.message(std::string("Images : ") + std::to_string(sceneData.images.size()));

	// シーン情報を構築.
	m_createGLTFScene(scene, &sceneData);
}

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

	scene->begin_part(sceneData->getFileName().c_str());

	// シーン階層をたどってノードとメッシュ作成.
	m_createGLTFNodeHierarchy(scene, sceneData, 0);

	scene->end_part();
	scene->end_creating();
}

/**
 * GLTFを読み込んだシーン情報より、ノードを再帰的にたどって階層構造を構築.
 */
void CGLTFImporterInterface::m_createGLTFNodeHierarchy (sxsdk::scene_interface *scene, CSceneData* sceneData, const int nodeIndex)
{
	const CNodeData& nodeD = sceneData->nodes[nodeIndex];
	sxsdk::part_class* part = NULL;
	if (nodeIndex > 0 && (nodeD.meshIndex < 0 || nodeD.childNodeIndex >= 0)) {
		part = &(scene->begin_part(nodeD.name.c_str()));
	}

	if (nodeIndex > 0 && nodeD.meshIndex >= 0) {
		// メッシュを生成.
		const sxsdk::mat4 m = (nodeD.childNodeIndex >= 0) ? sxsdk::mat4::identity : (sceneData->getNodeMatrix(nodeIndex));
		m_createGLTFMesh(scene, sceneData, nodeD.meshIndex, m);
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

	if (nodeIndex > 0 && (nodeD.meshIndex < 0 || nodeD.childNodeIndex >= 0)) {
		// nodeIndexでの変換行列を取得.
		const sxsdk::mat4 m = sceneData->getNodeMatrix(nodeIndex, true);

		// パートの行列を指定.
		part->reset_transformation();
		part->set_transformation_matrix(m);

		scene->end_part();
	}
}

/**
 * 指定のメッシュを生成.
 */
void CGLTFImporterInterface::m_createGLTFMesh (sxsdk::scene_interface *scene, CSceneData* sceneData, const int meshIndex, const sxsdk::mat4& matrix)
{
	const CMeshData& meshD = sceneData->meshes[meshIndex];

	// ポリゴンメッシュ形状を作成.
	sxsdk::polygon_mesh_class& pMesh = scene->begin_polygon_mesh(meshD.name.c_str());

	// 頂点座標を格納。.
	// GLTFではメートル単位であるので、Shade3Dのミリメートルに変換して渡している.
	const size_t versCou = meshD.vertices.size();
	{
		const float scale = 1000.0f;
		for (size_t i = 0; i < versCou; ++i) {
			const sxsdk::vec3 v =  (meshD.vertices[i] * scale) * matrix;
			scene->append_polygon_mesh_point(v);
		}
	}

	// 三角形の頂点インデックスを格納.
	{
		const size_t triCou = meshD.triangleIndices.size() / 3;
		for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			const int i0 = meshD.triangleIndices[iPos + 0];
			const int i1 = meshD.triangleIndices[iPos + 1];
			const int i2 = meshD.triangleIndices[iPos + 2];
			if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= versCou || i1 >= versCou || i2 >= versCou) {
				continue;
			}

			scene->append_polygon_mesh_face(i0, i1, i2);
		}
	}

	scene->end_polygon_mesh();

	// UVを格納.
	// UVは面の頂点ごとに格納することになる.
	if (!meshD.uv0.empty()) {
		const int uvIndex = pMesh.append_uv_layer();

		const int triCou = pMesh.get_number_of_faces();
		int triIndices[3];
		for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			sxsdk::face_class& f = pMesh.face(i);
			f.get_vertex_indices(triIndices);
			for (int j = 0; j < 3; ++j) {
				f.set_face_uv(uvIndex, j, meshD.uv0[ triIndices[j] ]);
			}
		}
	}
	if (!meshD.uv1.empty()) {
		const int uvIndex = pMesh.append_uv_layer();

		const int triCou = pMesh.get_number_of_faces();
		int triIndices[3];
		for (int i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			sxsdk::face_class& f = pMesh.face(i);
			f.get_vertex_indices(triIndices);
			for (int j = 0; j < 3; ++j) {
				f.set_face_uv(uvIndex, j, meshD.uv1[ triIndices[j] ]);
			}
		}
	}

	// マテリアルを割り当て.
	if (meshD.materialIndex >= 0) {
		const CMaterialData& materialD = sceneData->materials[meshD.materialIndex];
		if (materialD.shadeMasterSurface) {
			pMesh.set_master_surface(materialD.shadeMasterSurface);
		}
	}

	// 重複頂点のマージ.
	pMesh.cleanup_redundant_vertices();

	// 稜線を生成.
	pMesh.make_edges();
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
				}
			}

			masterSurface.update();
		}

	} catch (...) { }
}
