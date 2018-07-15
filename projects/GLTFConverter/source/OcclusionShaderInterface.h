/**
 * Ambient Occlusionのためのshader(マッピングレイヤでの計算)派生クラス.
 */

#ifndef _OCCLUSIONSHADERINTERFACE_H
#define _OCCLUSIONSHADERINTERFACE_H

#include "GlobalHeader.h"

#define OCCLISION_SHADER_MAGIC_NUMBER 0x67ac7abe

class COcclusionTextureShaderInterface : public sxsdk::shader_interface
{
private:

	/**
	 * Shaderで参照する情報.
	 */
	class ShaderInfoC : public sxsdk::shader_info_base_class {
	public:
		COcclusionShaderData data;

		ShaderInfoC (COcclusionShaderData& data) : data(data) { }
		unsigned magic_number () const { return OCCLISION_SHADER_MAGIC_NUMBER; }		// 適当な数値.
	};

private:
	template<typename T> inline const T lerp (const T &a, const T &b, float t) {
		return (b - a) * t + a;
	}

private:
	virtual sx::uuid_class get_uuid (void *) { return OCCLUSION_SHADER_INTERFACE_ID; }
	virtual int get_shade_version () const { return SHADE_BUILD_NUMBER; }

	virtual bool supports_evaluate (void *) { return false; }
	virtual bool supports_shade (void *) { return true; }
	virtual bool needs_projection (void *aux=0) { return false; }
	virtual bool needs_uv (void*) { return true; }
	virtual bool needs_to_sample_image (void*) { return true; }
	virtual bool supports_bump (void*) { return false; }

	/**
	 * シェーダが独自に使うデータ領域を生成.
	 */
	virtual sxsdk::shader_info_base_class *new_shader_info (sxsdk::stream_interface *stream, void *);

	/**
	 * Shaderとしての計算を行う.
	 */
	virtual void shade (void *);

	/**
	 * 個々のフレームのレンダリング開始前に呼ばれる.
	 * @param[in] b 未使用.
	 * @param[in] rendering_context レンダリング情報クラスのポインタ.
	 */
	virtual void pre_rendering (bool &b, sxsdk::rendering_context_interface *rendering_context, void *aux = 0);

	/**
	 * レンダリング終了時に呼ばれる.
	 */
	virtual void post_rendering (bool &b, sxsdk::rendering_context_interface *rendering_context);

public:
	COcclusionTextureShaderInterface (sxsdk::shade_interface& _shade);
	virtual ~COcclusionTextureShaderInterface ();

	/**
	 * プラグイン名.
	 */
	static const char *name (sxsdk::shade_interface *_shade) {
		return _shade->gettext("occlusion_shader_title");
	}

};

#endif

