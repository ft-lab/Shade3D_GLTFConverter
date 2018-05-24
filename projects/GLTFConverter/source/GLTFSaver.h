/**
 * GLTFをファイルに保存する.
 */

#ifndef _GLTFSAVER_H
#define _GLTFSAVER_H

#include <string>

class CSceneData;

namespace sxsdk
{
	class shade_interface;
}

class CGLTFSaver
{
private:
	sxsdk::shade_interface* shade;

public:
	CGLTFSaver (sxsdk::shade_interface* shade);

	/**
	 * 指定のGLTFファイルを出力.
	 * @param[in]  fileName    出力ファイル名 (gltf/glb).
	 * @param[in]  sceneData   GLTFのシーン情報.
	 */
	bool saveGLTF (const std::string& fileName, const CSceneData* sceneData);

	/**
	 * エラー時の文字列取得.
	 */
	std::string getErrorString () const;
};

#endif

