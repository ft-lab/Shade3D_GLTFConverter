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

CTempMeshData::CTempMeshData (const CTempMeshData& v)
{
	this->name            = v.name;
	this->vertices        = v.vertices;
	this->skinWeights     = v.skinWeights;
	this->skinJoints      = v.skinJoints;
	this->skinJointsHandle = v.skinJointsHandle;
	this->triangleIndices = v.triangleIndices;
	this->triangleFaceGroupIndex  = v.triangleFaceGroupIndex;
	this->triangleNormals = v.triangleNormals;
	this->triangleUV0     = v.triangleUV0;
	this->triangleUV1     = v.triangleUV1;
	this->triangleColor0  = v.triangleColor0;
	this->triangleColor1  = v.triangleColor1;

	this->materialIndex   = v.materialIndex;
	this->faceGroupMaterialIndex = v.faceGroupMaterialIndex;

	this->morphTargets = v.morphTargets;
}

CTempMeshData::~CTempMeshData ()
{
}

void CTempMeshData::clear ()
{
	name = "";
	vertices.clear();
	skinWeights.clear();
	skinJoints.clear();
	skinJointsHandle.clear();
	triangleIndices.clear();
	triangleNormals.clear();
	triangleUV0.clear();
	triangleUV1.clear();
	triangleColor0.clear();
	triangleColor1.clear();
	triangleFaceGroupIndex.clear();

	materialIndex = 0;
	faceGroupMaterialIndex.clear();

	morphTargets.clear();
}

/**
 * 最適化 (不要頂点の除去など).
 * @param[in]  removeUnusedVertices   未使用頂点を削除する場合はtrue.
 */
void CTempMeshData::optimize (const bool removeUnusedVertices)
{
	// 面積が0の面を削除.
	{
		const float fMin = (float)(1e-5);
		const size_t triCou = triangleIndices.size() / 3;
		std::vector<int> removeTriIndexList;
		const float scale = 1000.0f;
		{
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				const int i1 = triangleIndices[iPos + 0];
				const int i2 = triangleIndices[iPos + 1];
				const int i3 = triangleIndices[iPos + 2];
				if (i1 == i2 || i1 == i3 || i2 == i3) {
					removeTriIndexList.push_back(i);
					continue;
				}
				const sxsdk::vec3 v1 = vertices[i1] * scale;
				const sxsdk::vec3 v2 = vertices[i2] * scale;
				const sxsdk::vec3 v3 = vertices[i3] * scale;
				if (MathUtil::isZero(v1 - v2, fMin) || MathUtil::isZero(v2 - v3, fMin) || MathUtil::isZero(v1 - v3, fMin)) {
					removeTriIndexList.push_back(i);
					continue;
				}
				if (MathUtil::calcTriangleArea(v1, v2, v3) < fMin) {
					removeTriIndexList.push_back(i);
					continue;
				}
			}
		}

		if (!removeTriIndexList.empty()) {
			const int rCou = (int)removeTriIndexList.size();
			for (int i = rCou - 1; i >= 0; --i) {
				const int triIndex = removeTriIndexList[i];
				const int iPos = triIndex * 3;
				for (int j = 0; j < 3; ++j) triangleIndices.erase(triangleIndices.begin() + iPos);
				if (!triangleFaceGroupIndex.empty()) {
					triangleFaceGroupIndex.erase(triangleFaceGroupIndex.begin() + triIndex);
				}
				if (!triangleNormals.empty()) {
					for (int j = 0; j < 3; ++j) triangleNormals.erase(triangleNormals.begin() + iPos);
				}
				if (!triangleUV0.empty()) {
					for (int j = 0; j < 3; ++j) triangleUV0.erase(triangleUV0.begin() + iPos);
				}
				if (!triangleUV1.empty()) {
					for (int j = 0; j < 3; ++j) triangleUV1.erase(triangleUV1.begin() + iPos);
				}
				if (!triangleColor0.empty()) {
					for (int j = 0; j < 3; ++j) triangleColor0.erase(triangleColor0.begin() + iPos);
				}
			}
		}
	}

	// 不要頂点を削除.
	if (removeUnusedVertices) {
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

		// glTFの仕様では、meshのprimitiveごとに、同一のMorph Targetsの要素数で同じ順番である必要がある模様.
		// そのため、頂点数が0のものがあってもあえて残す.
		for (size_t i = 0; i < morphTargets.morphTargetsData.size(); ++i) {
			COneMorphTargetData& mTargetD = morphTargets.morphTargetsData[i];
			const size_t vCou = mTargetD.vIndices.size();
			for (size_t j = 0; j < vCou; ++j) {
				mTargetD.vIndices[j] = useVersList[ mTargetD.vIndices[j] ];
			}
			for (int j = (int)vCou - 1; j >= 0; --j) {
				if (mTargetD.vIndices[j] < 0) {
					mTargetD.vIndices.erase(mTargetD.vIndices.begin() + j);
					mTargetD.position.erase(mTargetD.position.begin() + j);
				}
			}
		}

		for (int i = (int)versCou - 1; i >= 0; --i) {
			if (useVersList[i] < 0) {
				useVersList.erase(useVersList.begin() + i);
				vertices.erase(vertices.begin() + i);
				if (!skinWeights.empty()) skinWeights.erase(skinWeights.begin() + i);
				if (!skinJointsHandle.empty()) skinJointsHandle.erase(skinJointsHandle.begin() + i);
			}
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

CPrimitiveData::CPrimitiveData (const CPrimitiveData& v)
{
	this->name            = v.name;
	this->vertices        = v.vertices;
	this->normals         = v.normals;
	this->uv0             = v.uv0;
	this->uv1             = v.uv1;
	this->color0          = v.color0;
	this->triangleIndices = v.triangleIndices;
	this->materialIndex   = v.materialIndex;
	this->skinWeights     = v.skinWeights;
	this->skinJoints      = v.skinJoints;
	this->skinJointsHandle = v.skinJointsHandle;
	this->morphTargets     = v.morphTargets;
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
	color0.clear();
	triangleIndices.clear();
	materialIndex = 0;
	skinWeights.clear();
	skinJoints.clear();
	skinJointsHandle.clear();
	morphTargets.clear();
}

/**
 * CTempMeshDataからコンバート.
 */
void CPrimitiveData::convert (const CTempMeshData& tempMeshData)
{
	clear();

	name             = tempMeshData.name;
	materialIndex    = tempMeshData.materialIndex;
	vertices         = tempMeshData.vertices;
	triangleIndices  = tempMeshData.triangleIndices;
	skinWeights      = tempMeshData.skinWeights;
	skinJointsHandle = tempMeshData.skinJointsHandle;

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

	// 追加した同一頂点番号を格納するバッファ.
	std::vector< std::vector<int> > sameVersList;
	sameVersList.resize(versCou);
	for (size_t i = 0; i < versCou; ++i) sameVersList[i].clear();

	// 共有する頂点のうち、法線/UV0/UV1が異なる場合は頂点として分離.
	for (size_t vLoop = 0; vLoop < versCou; ++vLoop) {
		const std::vector< sx::vec<int,2> >& facesList = faceIndexInVertexList[vLoop];
		const size_t vtCou = facesList.size();
		if (vtCou <= 1) continue;

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
				{
					const sxsdk::vec3 v = vertices[vLoop];
					vertices.push_back(v);
				}
				if (!skinWeights.empty()) {
					const sxsdk::vec4 v = skinWeights[vLoop];
					skinWeights.push_back(v);
				}
				if (!skinJointsHandle.empty()) {
					const sx::vec<void*,4> v = skinJointsHandle[vLoop];
					skinJointsHandle.push_back(v);
				}
				sameVersList[vLoop].push_back(vIndex);

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

	// 頂点カラーを格納.
	if (!tempMeshData.triangleColor0.empty()) {
		color0.resize(newVersCou);
		for (size_t i = 0, iPos = 0; i < trisCou; ++i, iPos += 3) {
			for (int j = 0; j < 3; ++j) {
				const int iV = triangleIndices[iPos + j];
				color0[iV] = tempMeshData.triangleColor0[iPos + j];
			}
		}

	}

	// Morph Targetsを格納.
	if (!tempMeshData.morphTargets.morphTargetsData.empty()) {
		const CMorphTargetsData& srcMorphTargets = tempMeshData.morphTargets;
		const int targetsCou = (int)srcMorphTargets.morphTargetsData.size();
		if (targetsCou > 0) {
			morphTargets.morphTargetsData.resize(targetsCou);
			for (int i = 0; i < targetsCou; ++i) {
				const COneMorphTargetData& srcTargetD = tempMeshData.morphTargets.morphTargetsData[i];
				morphTargets.morphTargetsData[i] = srcTargetD;
				COneMorphTargetData& dstTargetD = morphTargets.morphTargetsData[i];
				const size_t vCou = srcTargetD.vIndices.size();
				for (size_t j = 0; j < vCou; ++j) {
					const int vIndex = srcTargetD.vIndices[j];
					if (!sameVersList[vIndex].empty()) {
						const std::vector<int>& vIList = sameVersList[vIndex];
						for (size_t k = 0; k < vIList.size(); ++k) {
							const int vNewIndex = vIList[k];
							dstTargetD.vIndices.push_back(vNewIndex);
							dstTargetD.position.push_back(srcTargetD.position[j]);
							if (!srcTargetD.normal.empty()) {
								dstTargetD.normal.push_back(srcTargetD.normal[j]);
							}
							if (!srcTargetD.tangent.empty()) {
								dstTargetD.tangent.push_back(srcTargetD.tangent[j]);
							}
						}
					}
				}
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
 * @param[in]  shareVerticesMesh   頂点情報を共有する場合はtrue.
 * @param[out] primitiveData       プリミティブ情報が返る.
 * @param[out] faceGroupIndexList  プリミティブごとのフェイスグループ番号が返る.
 * @return メッシュの数.
 */
int CPrimitiveData::convert (const CTempMeshData& tempMeshData, const bool shareVerticesMesh, std::vector<CPrimitiveData>& primitivesData, std::vector<int>& faceGroupIndexList)
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
	if (shareVerticesMesh) {		// 1Meshで複数Primitiveの頂点を共有する場合.
		CTempMeshData tempMeshD;
		tempMeshD.name             = tempMeshData.name;
		tempMeshD.vertices         = tempMeshData.vertices;
		tempMeshD.skinWeights      = tempMeshData.skinWeights;
		tempMeshD.skinJointsHandle = tempMeshData.skinJointsHandle;
		tempMeshD.morphTargets     = tempMeshData.morphTargets;
		tempMeshD.faceGroupMaterialIndex.resize(triCou, 0);

		for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
			tempMeshD.faceGroupMaterialIndex[i] = tempMeshData.triangleFaceGroupIndex[i];
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
			if (!tempMeshData.triangleColor0.empty()) {
				tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 0]);
				tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 1]);
				tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 2]);
			}
		}

		// 不要頂点の除去.
		tempMeshD.optimize();

		CPrimitiveData primitiveAllD;
		primitiveAllD.convert(tempMeshD);

		// 面のフェイスグループ番号ごとに分離して格納.
		for (size_t fgLoop = 0; fgLoop < faceGroupIndexList.size(); ++fgLoop) {
			const int faceGroupIndex = faceGroupIndexList[fgLoop];

			primitivesData.push_back(CPrimitiveData());
			CPrimitiveData& primitiveD = primitivesData.back();

			if (fgLoop == 0) {
				primitiveD.name             = tempMeshD.name;
				primitiveD.vertices         = primitiveAllD.vertices;
				primitiveD.normals          = primitiveAllD.normals;
				primitiveD.uv0              = primitiveAllD.uv0;
				primitiveD.uv1              = primitiveAllD.uv1;
				primitiveD.color0           = primitiveAllD.color0;
				primitiveD.skinWeights      = primitiveAllD.skinWeights;
				primitiveD.skinJointsHandle = primitiveAllD.skinJointsHandle;
				primitiveD.morphTargets     = primitiveAllD.morphTargets;
			}
			const size_t triCou = primitiveAllD.triangleIndices.size() / 3;
			for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
				const int fgIndex = tempMeshD.faceGroupMaterialIndex[i];
				if (fgIndex != faceGroupIndex) continue;
				primitiveD.triangleIndices.push_back(primitiveAllD.triangleIndices[iPos + 0]);
				primitiveD.triangleIndices.push_back(primitiveAllD.triangleIndices[iPos + 1]);
				primitiveD.triangleIndices.push_back(primitiveAllD.triangleIndices[iPos + 2]);
			}
		}

	} else {						// 複数Primitiveで独自の頂点に分離する場合.
		for (size_t fgLoop = 0; fgLoop < faceGroupIndexList.size(); ++fgLoop) {
			const int faceGroupIndex = faceGroupIndexList[fgLoop];

			CTempMeshData tempMeshD;
			tempMeshD.name             = tempMeshData.name;
			tempMeshD.vertices         = tempMeshData.vertices;
			tempMeshD.skinWeights      = tempMeshData.skinWeights;
			tempMeshD.skinJointsHandle = tempMeshData.skinJointsHandle;
			tempMeshD.morphTargets     = tempMeshData.morphTargets;

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
				if (!tempMeshData.triangleColor0.empty()) {
					tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 0]);
					tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 1]);
					tempMeshD.triangleColor0.push_back(tempMeshData.triangleColor0[iPos + 2]);
				}
			}

			// 不要頂点の除去.
			tempMeshD.optimize();

			primitivesData.push_back(CPrimitiveData());
			CPrimitiveData& primitiveD = primitivesData.back();
			primitiveD.convert(tempMeshD);
		}
	}

	return (int)primitivesData.size();
}

/**
 * 頂点カラー情報で、Alphaを出力する必要があるか.
 */
bool CPrimitiveData::hasNeedVertexColorAlpha () const
{
	if (color0.empty()) return false;

	bool hasAlpha = false;

	const size_t vCou = color0.size();
	for (size_t i = 0; i < vCou; ++i) {
		const sxsdk::vec4& col = color0[i];
		if (!MathUtil::isZero(col.w - 1.0)) {
			hasAlpha = true;
			break;
		}
	}
	return hasAlpha;
}

//---------------------------------------------------------------.
CMeshData::CMeshData ()
{
	clear();
}

CMeshData::CMeshData (const CMeshData& v)
{
	this->name        = v.name;
	this->primitives  = v.primitives;
	this->pMeshHandle = v.pMeshHandle;
}

CMeshData::~CMeshData ()
{
}

void CMeshData::clear ()
{
	name = "";
	primitives.clear();
	pMeshHandle = NULL;
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
	bool useUV0, useUV1, useNormal, useColor0, useSkinWeights, useSkinJoints, useMorphTargets;
	useUV0 = useUV1 = useNormal = useColor0 = useSkinWeights = useSkinJoints = useMorphTargets = false;
	for (size_t loop = 0; loop < primitivesCou; ++loop) {
		const CPrimitiveData& primitiveD = primitives[loop];
		if (!primitiveD.normals.empty()) useNormal = true;
		if (!primitiveD.uv0.empty()) useUV0 = true;
		if (!primitiveD.uv1.empty()) useUV1 = true;
		if (!primitiveD.skinJoints.empty()) useSkinJoints = true;
		if (!primitiveD.skinWeights.empty()) useSkinWeights = true;
		if (!primitiveD.color0.empty()) useColor0 = true;
		if (!primitiveD.morphTargets.morphTargetsData.empty()) useMorphTargets = true;
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
		if (useColor0) {
			if (!primitiveD.color0.empty()) {
				for (size_t i = 0, iPos = 0; i < triCou; ++i, iPos += 3) {
					const int i0 = primitiveD.triangleIndices[iPos + 0];
					const int i1 = primitiveD.triangleIndices[iPos + 1];
					const int i2 = primitiveD.triangleIndices[iPos + 2];
					tempMeshData.triangleColor0.push_back(primitiveD.color0[i0]);
					tempMeshData.triangleColor0.push_back(primitiveD.color0[i1]);
					tempMeshData.triangleColor0.push_back(primitiveD.color0[i2]);
				}
			} else {
				for (size_t i = 0; i < triCou; ++i) {
					tempMeshData.triangleColor0.push_back(sxsdk::vec4(1, 1, 1, 1));
					tempMeshData.triangleColor0.push_back(sxsdk::vec4(1, 1, 1, 1));
					tempMeshData.triangleColor0.push_back(sxsdk::vec4(1, 1, 1, 1));
				}
			}
		}

		// スキンの情報は頂点ごとに持つ.
		if (useSkinWeights) {
			if (!primitiveD.skinWeights.empty()) {
				for (size_t i = 0; i < versCou; ++i) {
					tempMeshData.skinWeights.push_back(primitiveD.skinWeights[i]);
				}
			} else {
				for (size_t i = 0; i < versCou; ++i) {
					tempMeshData.skinWeights.push_back(sxsdk::vec4(0, 0, 0, 0));
				}
			}
		}
		if (useSkinJoints) {
			if (!primitiveD.skinJoints.empty()) {
				for (size_t i = 0; i < versCou; ++i) {
					tempMeshData.skinJoints.push_back(primitiveD.skinJoints[i]);
				}
			} else {
				for (size_t i = 0; i < versCou; ++i) {
					tempMeshData.skinJoints.push_back(sx::vec<int,4>(0, 0, 0, 0));
				}
			}
		}

		// Morph Targets情報.
		// glTFではPrimitiveの頂点数分の情報を持っているが、すべてを持つのはリソースを消費するため、.
		// 変化のないものは省いて最適化してtempMeshDataへ渡す。この際に、使用している頂点のインデックスも格納.
		if (useMorphTargets) {
			const size_t targetsCou = primitiveD.morphTargets.morphTargetsData.size();
			if (targetsCou > 0) {
				// glTFからの読み込みでは、mesh内のprimitiveはすべて同じ数のMorph Targetsを持っているのが仕様.
				if (loop == 0) {
					for (size_t i = 0; i < targetsCou; ++i) {
						const COneMorphTargetData& mTargetD = primitiveD.morphTargets.morphTargetsData[i];
						COneMorphTargetData tData;
						tData.weight = mTargetD.weight;
						tData.name   = mTargetD.name;
						tempMeshData.morphTargets.morphTargetsData.push_back(tData);
					}
				}

				std::vector<int> useVerList;
				for (size_t i = 0; i < targetsCou; ++i) {
					const COneMorphTargetData& mTargetD = primitiveD.morphTargets.morphTargetsData[i];
					const size_t vCou = mTargetD.position.size();
					if (vCou == 0) continue;

					COneMorphTargetData& tData = tempMeshData.morphTargets.morphTargetsData[i];

					// 使用している頂点に対するインデックスを保持.
					for (size_t j = 0; j < vCou; ++j) {
						if (!sx::zero(mTargetD.position[j])) {
							tData.vIndices.push_back(j + vOffset);
							tData.position.push_back(mTargetD.position[j]);
						}
					}
				}
			}
		}

		tempMeshData.faceGroupMaterialIndex[loop] = primitiveD.materialIndex;

		vOffset += versCou;
	}

	return true;
}

