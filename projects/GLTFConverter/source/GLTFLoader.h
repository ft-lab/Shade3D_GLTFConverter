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

private:
	/**
	 * DeserializeJson()を呼ぶ前に、Deserializeに失敗する要素を削除しておく.
	 * @param[in]  jsonStr           gltfのjsonテキスト.
	 * @param[out] outputJsonStr     修正後のjsonテキストが返る.
	 */
	bool m_checkPreDeserializeJson (const std::string jsonStr, std::string& outputJsonStr);

public:
	CGLTFLoader ();

	/**
	 * 指定のGLTFファイルを読み込み.
	 * @param[in]  fileName    読み込むファイル名 (gltfまたはglb).
	 * @param[out] sceneData   読み込んだGLTFのシーン情報が返る.
	 */
	bool loadGLTF (const std::string& fileName, CSceneData* sceneData);

	/**
	 * エラー時の文字列取得.
	 */
	std::string getErrorString () const;
};

#endif
