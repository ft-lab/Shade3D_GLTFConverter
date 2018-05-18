/**
 * GLTFをファイルに保存する.
 */

#ifndef _GLTFSAVER_H
#define _GLTFSAVER_H

#include <string>

class CSceneData;

class CGLTFSaver
{
private:

public:
	CGLTFSaver ();

	/**
	 * 指定のGLTFファイルを出力.
	 * @param[in]  fileName    出力ファイル名 (glb).
	 * @param[in]  sceneData   GLTFのシーン情報.
	 */
	bool saveGLTF (const std::string& fileName, const CSceneData* sceneData);
};

#endif

