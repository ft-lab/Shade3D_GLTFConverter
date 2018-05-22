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
	triangleFaceGroupIndex.clear();

	materialIndex = 0;
	faceGroupMaterialIndex.clear();
}

/**
 * 最適化 (不要頂点の除去など).
 */
void CTempMeshData::optimize ()
{
	const size_t versCou = vertices.size();

	std::vector<int> useVersList;
	useVersList.resize(versCou, -1);

	for (size_t i = 0; i < triangleIndices.size(); ++i) {
		useVersList[ triangleIndices[i] ] = 1;
	}
	{
		int iPos = 0;
		for (size_t i = 0; i < versCou; ++i) {
			if (useVersList[i] > 0) useVersList[i] = iPos++;
		}
	}

	for (size_t i = 0; i < triangleIndices.size(); ++i) {
		triangleIndices[i] = useVersList[ triangleIndices[i] ];
	}

	for (int i = (int)versCou - 1; i >= 0; --i) {
		if (useVersList[i] < 0) {
			useVersList.erase(useVersList.begin() + i);
			vertices.erase(vertices.begin() + i);
		}
	}
}

//---------------------------------------------------------------.
/**
 * 1つのメッシュ情報 (GLTFでの構成).
 */
CPrimitiveData::CPrimitiveData ()
{
	clear();
}

CPrimitiveData::~CPrimitiveData ()
{
}

void CPrimitiveData::clear ()
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
void CPrimitiveData::convert (const CTempMeshData& tempMeshData)
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

				if (MathUtil::isZero(n_1 - n_2) && MathUtil::isZero(uv0_1 - uv0_2) && MathUtil::isZero(uv1_1 - uv1_2)) {
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
void CPrimitiveData::calcBoundingBox (sxsdk::vec3& bbMin, sxsdk::vec3& bbMax) const
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

/**
 * CTempMeshDataからコンバート(フェイスグループを考慮して分離).
 * @param[in]  tempMeshData        作業用のメッシュ情報.
 * @param[out] primitiveData       プリミティブ情報が返る.
 * @param[out] faceGroupIndexList  プリミティブごとのフェイスグループ番号が返る.
 * @return メッシュの数.
 */
int CPrimitiveData::convert (const CTempMeshData& tempMeshData, std::vector<CPrimitiveData>& primitivesData, std::vector<int>& faceGroupIndexList)
{
	// 使用しているフェイスグループ番号を取得.
	faceGroupIndexList.clear();
	const size_t triCou = tempMeshData.triangleFaceGroupIndex.size();
	for (size_t i = 0; i < triCou; ++i) {
		const int fgIndex = tempMeshData.triangleFaceGroupIndex[i];
		if (std::find(faceGroupIndexList.begin(), faceGroupIndexList.end(), fgIndex) == faceGroupIndexList.end()) {
			faceGroupIndexList.push_back(fgIndex);
		}
	}

	primitivesData.clear();
	for (size_t fgLoop = 0; fgLoop < faceGroupIndexList.size(); ++fgLoop) {
		const int faceGroupIndex = faceGroupIndexList[fgLoop];

		CTempMeshData tempMeshD;
		tempMeshD.name     = tempMeshData.name;
		tempMeshD.vertices = tempMeshData.vertices;

		for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			const int fgIndex = tempMeshData.triangleFaceGroupIndex[i];
			if (fgIndex != faceGroupIndex) continue;
			{
				tempMeshD.triangleIndices.push_back(tempMeshData.triangleIndices[iPos + 0]);
				tempMeshD.triangleIndices.push_back(tempMeshData.triangleIndices[iPos + 1]);
				tempMeshD.triangleIndices.push_back(tempMeshData.triangleIndices[iPos + 2]);
			}
			if (!tempMeshData.triangleNormals.empty()) {
				tempMeshD.triangleNormals.push_back(tempMeshData.triangleNormals[iPos + 0]);
				tempMeshD.triangleNormals.push_back(tempMeshData.triangleNormals[iPos + 1]);
				tempMeshD.triangleNormals.push_back(tempMeshData.triangleNormals[iPos + 2]);
			}
			if (!tempMeshData.triangleUV0.empty()) {
				tempMeshD.triangleUV0.push_back(tempMeshData.triangleUV0[iPos + 0]);
				tempMeshD.triangleUV0.push_back(tempMeshData.triangleUV0[iPos + 1]);
				tempMeshD.triangleUV0.push_back(tempMeshData.triangleUV0[iPos + 2]);
			}
			if (!tempMeshData.triangleUV1.empty()) {
				tempMeshD.triangleUV1.push_back(tempMeshData.triangleUV1[iPos + 0]);
				tempMeshD.triangleUV1.push_back(tempMeshData.triangleUV1[iPos + 1]);
				tempMeshD.triangleUV1.push_back(tempMeshData.triangleUV1[iPos + 2]);
			}
		}

		// 不要頂点の除去.
		tempMeshD.optimize();

		primitivesData.push_back(CPrimitiveData());
		CPrimitiveData& primitiveD = primitivesData.back();
		primitiveD.convert(tempMeshD);
	}

	return (int)primitivesData.size();
}

//---------------------------------------------------------------.
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
	primitives.clear();
}

/**
 * 複数のPrimitiveを1つのメッシュにまとめる (Import時に使用).
 * Shade3Dのフェイスグループを使用する1つのポリゴンメッシュにする.
 * @param[out] tempMeshData  まとめたメッシュ情報を格納.
 */
bool CMeshData::mergePrimitives (CTempMeshData& tempMeshData) const
{
	const size_t primitivesCou = primitives.size();
	tempMeshData.clear();
	if (primitivesCou == 0) return false;

	tempMeshData.name = this->name;

	// どの要素を使用するか.
	bool useUV0, useUV1, useNormal;
	useUV0 = useUV1 = useNormal = false;
	for (size_t loop = 0; loop < primitivesCou; ++loop) {
		const CPrimitiveData& primitiveD = primitives[loop];
		if (!primitiveD.normals.empty()) useNormal = true;
		if (!primitiveD.uv0.empty()) useUV0 = true;
		if (!primitiveD.uv1.empty()) useUV1 = true;
	}

	size_t vOffset = 0;
	tempMeshData.faceGroupMaterialIndex.resize(primitivesCou, -1);
	for (size_t loop = 0; loop < primitivesCou; ++loop) {
		const CPrimitiveData& primitiveD = primitives[loop];

		const size_t versCou = primitiveD.vertices.size();
		const size_t triCou  = primitiveD.triangleIndices.size() / 3;
		for (size_t i = 0; i < versCou; ++i) {
			tempMeshData.vertices.push_back(primitiveD.vertices[i]);
		}
		for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			const int i0 = primitiveD.triangleIndices[iPos + 0];
			const int i1 = primitiveD.triangleIndices[iPos + 1];
			const int i2 = primitiveD.triangleIndices[iPos + 2];
			tempMeshData.triangleIndices.push_back(i0 + vOffset);
			tempMeshData.triangleIndices.push_back(i1 + vOffset);
			tempMeshData.triangleIndices.push_back(i2 + vOffset);

			tempMeshData.triangleFaceGroupIndex.push_back(loop);
		}

		if (useNormal) {
			if (!primitiveD.normals.empty()) {
				for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
					const int i0 = primitiveD.triangleIndices[iPos + 0];
					const int i1 = primitiveD.triangleIndices[iPos + 1];
					const int i2 = primitiveD.triangleIndices[iPos + 2];
					tempMeshData.triangleNormals.push_back(primitiveD.normals[i0]);
					tempMeshData.triangleNormals.push_back(primitiveD.normals[i1]);
					tempMeshData.triangleNormals.push_back(primitiveD.normals[i2]);
				}
			} else {
				for (size_t i = 0; i < triCou; ++i) {
					tempMeshData.triangleNormals.push_back(sxsdk::vec3(0, 0, 0));
					tempMeshData.triangleNormals.push_back(sxsdk::vec3(0, 0, 0));
					tempMeshData.triangleNormals.push_back(sxsdk::vec3(0, 0, 0));
				}
			}
		}
		if (useUV0) {
			if (!primitiveD.uv0.empty()) {
				for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
					const int i0 = primitiveD.triangleIndices[iPos + 0];
					const int i1 = primitiveD.triangleIndices[iPos + 1];
					const int i2 = primitiveD.triangleIndices[iPos + 2];
					tempMeshData.triangleUV0.push_back(primitiveD.uv0[i0]);
					tempMeshData.triangleUV0.push_back(primitiveD.uv0[i1]);
					tempMeshData.triangleUV0.push_back(primitiveD.uv0[i2]);
				}
			} else {
				for (size_t i = 0; i < triCou; ++i) {
					tempMeshData.triangleUV0.push_back(sxsdk::vec2(0, 0));
					tempMeshData.triangleUV0.push_back(sxsdk::vec2(0, 0));
					tempMeshData.triangleUV0.push_back(sxsdk::vec2(0, 0));
				}
			}
		}
		if (useUV1) {
			if (!primitiveD.uv1.empty()) {
				for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
					const int i0 = primitiveD.triangleIndices[iPos + 0];
					const int i1 = primitiveD.triangleIndices[iPos + 1];
					const int i2 = primitiveD.triangleIndices[iPos + 2];
					tempMeshData.triangleUV1.push_back(primitiveD.uv1[i0]);
					tempMeshData.triangleUV1.push_back(primitiveD.uv1[i1]);
					tempMeshData.triangleUV1.push_back(primitiveD.uv1[i2]);
				}
			} else {
				for (size_t i = 0; i < triCou; ++i) {
					tempMeshData.triangleUV1.push_back(sxsdk::vec2(0, 0));
					tempMeshData.triangleUV1.push_back(sxsdk::vec2(0, 0));
					tempMeshData.triangleUV1.push_back(sxsdk::vec2(0, 0));
				}
			}
		}
		tempMeshData.faceGroupMaterialIndex[loop] = primitiveD.materialIndex;

		vOffset += versCou;
	}

	return true;
}

