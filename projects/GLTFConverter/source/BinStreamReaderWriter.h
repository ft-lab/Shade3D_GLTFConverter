/**
 * IStreamReader/IStreamWriterクラス.
 */

#ifndef _BINSTREAMREADERWRITER_H
#define _BINSTREAMREADERWRITER_H

#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>

#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/IStreamReader.h>
#include <GLTFSDK/IStreamWriter.h>

/**
 * バイナリ読み込み用.
 */
class BinStreamReader : public Microsoft::glTF::IStreamReader
{
private:
	std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.

public:
	BinStreamReader (std::string basePath) : m_basePath(basePath)
	{
	}

	virtual std::shared_ptr<std::istream> GetInputStream (const std::string& filename) const override
	{
		const std::string path = m_basePath + std::string("/") + filename;
		return std::make_shared<std::ifstream>(path, std::ios::binary);
	}
};

/**
 * GLTFのバイナリ出力用.
 */
class BinStreamWriter : public Microsoft::glTF::IStreamWriter
{
private:
	std::string m_basePath;		// gltfファイルの絶対パスのディレクトリ.
	std::string m_fileName;		// 出力ファイル名.
	bool m_appendMode;			// 既存ファイルの末尾に追記.

public:
	BinStreamWriter (std::string basePath, std::string fileName, const bool appendMode = false) : m_basePath(basePath), m_fileName(fileName), m_appendMode(appendMode)
	{
	}
	virtual ~BinStreamWriter () {
	}

	virtual std::shared_ptr<std::ostream> GetOutputStream (const std::string& filename) const override
	{
		const std::string path = m_basePath + std::string("/") + m_fileName;
		int flags = std::ios::binary | std::ios::out;
		if (m_appendMode) flags |= std::ios::app;
		return std::make_shared<std::ofstream>(path, flags);
	}
};

#endif
