/**
 * gltf/glbのDraco圧縮された構造を展開.
 */
#ifndef _GLTFMESHDECOMPRESSION_H
#define _GLTFMESHDECOMPRESSION_H

#include "GLTFSDK.h"
#include "GLTFSDK/GLBResourceReader.h"

#include <vector>

namespace glTFToolKit {
	//----------------------------------------------------------.
	/**
	 * 解凍したメッシュ情報を格納するクラス.
	 */
	class DecompressMeshData {
	public:
		std::vector<int> indices;			// 三角形の頂点インデックス.
		std::vector<float> vertices;		// 頂点座標 (xyz).
		std::vector<float> normals;			// 法線 (xyz).
		std::vector<float> uvs0;			// UV0 (xy).
		std::vector<float> uvs1;			// UV1 (xy).
		std::vector<float> colors0;			// Color  (rgba).
		std::vector<int> joints;			// joints 4つでスキンに対応するノード指定.
		std::vector<float> weights;			// Skin Weights. 4つでスキンに対応するweight値.
		std::vector<float> tangents;		// Tangents (xyz).

		int pointsCou;						// 頂点数.

	public:
		DecompressMeshData ();
		DecompressMeshData (const DecompressMeshData& v);
		~DecompressMeshData ();

		DecompressMeshData& operator = (const DecompressMeshData &v) {
			this->indices  = v.indices;
			this->vertices = v.vertices;
			this->normals  = v.normals;
			this->uvs0     = v.uvs0;
			this->uvs1     = v.uvs1;
			this->colors0  = v.colors0;
			this->joints   = v.joints;
			this->weights  = v.weights;
			this->tangents = v.tangents;
			this->pointsCou = v.pointsCou;
			return (*this);
		}

		void clear();
	};

	//----------------------------------------------------------.
	/**
	 * Mesh情報を解凍するクラス.
	 */
    class GLTFMeshDecompressionUtils
    {
    public:
		/**
		 * 指定のgltf/glbファイルからDraco情報を展開.
		 * @param[in]  gltfFileName  gltf/glbのファイル名.
		 * @param[out] meshDataList  メッシュ情報が展開されて入る.
		 */
		static bool doDracoDecompress (const std::string gltfFileName, std::vector<DecompressMeshData>& meshDataList);
	};
}

#endif

