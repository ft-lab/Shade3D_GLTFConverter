/**
 * GLTFの読み込みクラス.
 */

#ifndef _GLTF_LOADER_H
#define _GLTF_LOADER_H


#include <string>

class CSceneData;

class CGLTFLoader
{
private:

public:
	CGLTFLoader ();

	/**
	 * 指定のGLTFファイルを読み込み.
	 * @param[in]  fileName    読み込み形状名 (gltfまたはglb).
	 * @param[out] sceneData   読み込んだGLTFのシーン情報が返る.
	 */
	bool loadGLTF (const std::string& fileName, CSceneData* sceneData);
};

#endif
