/**
 * Ambient Occlusionのためのshader(マッピングレイヤでの計算)派生クラス.
 */
#include "OcclusionShaderInterface.h"

COcclusionTextureShaderInterface::COcclusionTextureShaderInterface (sxsdk::shade_interface& _shade) : sxsdk::shader_interface(_shade)
{
}

COcclusionTextureShaderInterface::~COcclusionTextureShaderInterface ()
{
}

/**
 * シェーダが独自に使うデータ領域を生成 (未使用).
 */
sxsdk::shader_info_base_class *COcclusionTextureShaderInterface::new_shader_info (sxsdk::stream_interface *stream, void *)
{
	COcclusionShaderData data;
	return new ShaderInfoC(data);
}

/**
 * Shaderとしての計算を行う.
 */
void COcclusionTextureShaderInterface::shade (void *)
{
	// UVを持たない場合.
	if (!has_uv()) {
		set_Ci(get_mapping_color());
		return;
	}

	// UV値を取得.
	const sxsdk::vec2 uv = sxsdk::vec2(get_s(), get_t());		// UV1(距離UV).

	// テクスチャ上の色.
	const sxsdk::rgba_class col = sample_image(uv.x, uv.y);
	set_Ci(sx::rgb(col));
}

/**
 * 個々のフレームのレンダリング開始前に呼ばれる.
 * @param[in] b 未使用.
 * @param[in] rendering_context レンダリング情報クラスのポインタ.
 */
void COcclusionTextureShaderInterface::pre_rendering (bool &b, sxsdk::rendering_context_interface *rendering_context, void *aux)
{
}

/**
 * レンダリング終了時に呼ばれる.
 */
void COcclusionTextureShaderInterface::post_rendering (bool &b, sxsdk::rendering_context_interface *rendering_context)
{
}


