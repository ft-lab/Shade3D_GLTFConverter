/**
 * GLTF用のメッシュデータ.
 */
#include "MeshData.h"

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
