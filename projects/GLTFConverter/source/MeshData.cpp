/**
 * GLTF用のメッシュデータ.
 */
#include "MeshData.h"
#include "MathUtil.h"

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (Shade3Dでの構成).
 */
CTempMeshData::CTempMeshData ()
{
	clear();
}

CTempMeshData::~CTempMeshData ()
{
}

void CTempMeshData::clear ()
{
	name = "";
	vertices.clear();
	triangleIndices.clear();
	triangleNormals.clear();
	triangleUV0.clear();
	triangleUV1.clear();

	materialIndex = 0;
}

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (GLTFでの構成).
 */
CMeshData::CMeshData ()
{
	clear();
}

CMeshData::~CMeshData ()
{
}

void CMeshData::clear ()
{
	name = "";
	vertices.clear();
	normals.clear();
	uv0.clear();
	uv1.clear();
	triangleIndices.clear();
	materialIndex = 0;
}

/**
 * CTempMeshDataからコンバート.
 */
void CMeshData::convert (const CTempMeshData& tempMeshData)
{
	clear();

	name            = tempMeshData.name;
	materialIndex   = tempMeshData.materialIndex;
	vertices        = tempMeshData.vertices;
	triangleIndices = tempMeshData.triangleIndices;

	//---------------------------------------------------------.
	// 法線やUVは、三角形ごとの頂点から頂点数分の配列に格納するようにコンバート.
	//---------------------------------------------------------.
	const size_t versCou = vertices.size();
	const size_t trisCou = triangleIndices.size() / 3;
	if (versCou == 0 || trisCou <= 1) return;

	// 各頂点で共有する面をリスト.
	std::vector< std::vector< sx::vec<int,2> > > faceIndexInVertexList;
	faceIndexInVertexList.resize(versCou);
	for (size_t i = 0; i < versCou; ++i) faceIndexInVertexList[i].clear();

	for (size_t i = 0, iPos = 0; i < trisCou; ++i, iPos += 3) {
		const int i0 = triangleIndices[iPos + 0];
		const int i1 = triangleIndices[iPos + 1];
		const int i2 = triangleIndices[iPos + 2];
		faceIndexInVertexList[i0].push_back(sx::vec<int,2>(i, 0));
		faceIndexInVertexList[i1].push_back(sx::vec<int,2>(i, 1));
		faceIndexInVertexList[i2].push_back(sx::vec<int,2>(i, 2));
	}

	// 共有する頂点のうち、法線/UV0/UV1が異なる場合は頂点として分離.
	for (size_t vLoop = 0; vLoop < versCou; ++vLoop) {
		const std::vector< sx::vec<int,2> >& facesList = faceIndexInVertexList[vLoop];
		const size_t vtCou = facesList.size();
		if (vtCou <= 1) continue;
		const sxsdk::vec3 v = vertices[vLoop];

		// 法線/UVをチェック.
		for (size_t i = 1; i < vtCou; ++i) {
			const int iP1 = facesList[i][0] * 3 + facesList[i][1];
			const sxsdk::vec3& n_1   = tempMeshData.triangleNormals[iP1];
			const sxsdk::vec2& uv0_1 = (tempMeshData.triangleUV0.empty()) ? sxsdk::vec2(0, 0) : tempMeshData.triangleUV0[iP1];
			const sxsdk::vec2& uv1_1 = (tempMeshData.triangleUV1.empty()) ? sxsdk::vec2(0, 0) : tempMeshData.triangleUV1[iP1];

			int sIndex = -1;
			for (size_t j = 0; j < i; ++j) {
				const int iP2 = facesList[j][0] * 3 + facesList[j][1];
				const sxsdk::vec3& n_2   = tempMeshData.triangleNormals[iP2];
				const sxsdk::vec2& uv0_2 = (tempMeshData.triangleUV0.empty()) ? sxsdk::vec2(0, 0) : tempMeshData.triangleUV0[iP2];
				const sxsdk::vec2& uv1_2 = (tempMeshData.triangleUV1.empty()) ? sxsdk::vec2(0, 0) : tempMeshData.triangleUV1[iP2];

				if (MathUtil::IsZero(n_1 - n_2) && MathUtil::IsZero(uv0_1 - uv0_2) && MathUtil::IsZero(uv1_1 - uv1_2)) {
					sIndex = (int)j;
					break;
				}
			}

			// 新しく頂点を追加して対応.
			if (sIndex < 0) {
				const size_t vIndex = vertices.size();
				vertices.push_back(v);

				triangleIndices[iP1] = vIndex;
			} else {
				const int iP2 = facesList[sIndex][0] * 3 + facesList[sIndex][1];
				triangleIndices[iP1] = triangleIndices[iP2];
			}
		}
	}

	// 法線とUVを格納.
	const size_t newVersCou = vertices.size();
	normals.resize(newVersCou);
	if (!tempMeshData.triangleUV0.empty()) {
		uv0.resize(newVersCou);
	}
	if (!tempMeshData.triangleUV1.empty()) {
		uv1.resize(newVersCou);
	}

	for (size_t i = 0, iPos = 0; i < trisCou; ++i, iPos += 3) {
		for (int j = 0; j < 3; ++j) {
			const int iV = triangleIndices[iPos + j];
			normals[iV] = tempMeshData.triangleNormals[iPos + j];
		}
		if (!tempMeshData.triangleUV0.empty()) {
			for (int j = 0; j < 3; ++j) {
				const int iV = triangleIndices[iPos + j];
				uv0[iV] = tempMeshData.triangleUV0[iPos + j];
			}
		}
		if (!tempMeshData.triangleUV1.empty()) {
			for (int j = 0; j < 3; ++j) {
				const int iV = triangleIndices[iPos + j];
				uv1[iV] = tempMeshData.triangleUV1[iPos + j];
			}
		}
	}
}

/**
 * 頂点座標のバウンディングボックスを計算.
 */
void CMeshData::calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const
{
	const size_t versCou = vertices.size();
	bbMin = bbMax = sxsdk::vec3(0, 0, 0);
	if (versCou == 0) return;

	bbMin = bbMax = vertices[0];
	for (size_t i = 1; i < versCou; ++i) {
		const sxsdk::vec3& v = vertices[i];
		bbMin.x = std::min(bbMin.x, v.x);
		bbMin.y = std::min(bbMin.y, v.y);
		bbMin.z = std::min(bbMin.z, v.z);
		bbMax.x = std::max(bbMax.x, v.x);
		bbMax.y = std::max(bbMax.y, v.y);
		bbMax.z = std::max(bbMax.z, v.z);
	}
}

