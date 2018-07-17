/**
 * Ambient Occlusionのためのshader(マッピングレイヤでの計算)派生クラス.
 */
#include "OcclusionShaderInterface.h"
#include "StreamCtrl.h"
#include "Shade3DUtil.h"

// SXULダイアログボックスでのイベントID.
enum
{
	dlg_uvIndex_id = 101,			// UV.
};

COcclusionTextureShaderInterface::COcclusionTextureShaderInterface (sxsdk::shade_interface& _shade) : sxsdk::shader_interface(_shade), m_shade(&_shade)
{
}

COcclusionTextureShaderInterface::~COcclusionTextureShaderInterface ()
{
}

/**
 * シェーダが独自に使うデータ領域を生成.
 */
sxsdk::shader_info_base_class *COcclusionTextureShaderInterface::new_shader_info (sxsdk::stream_interface *stream, void *)
{
	COcclusionShaderData data;

	// streamから情報読み込み.
	// ここのstreamは、mapping_layerから別途取得する手段はない模様.
	// そのため、このstream情報をExport/Importのために取り出すことはできない.
	if (stream) {
		StreamCtrl::loadOcclusionParam(stream, data);
	}

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

	// Shaderで参照する情報.
	COcclusionShaderData data;
	try {
		ShaderInfoC* shaderInfo = (ShaderInfoC *)(get_shader_info());
		if (shaderInfo) {
			if ((shaderInfo->magic_number()) == OCCLISION_SHADER_MAGIC_NUMBER) data = shaderInfo->data;
		}
	} catch (...) { }

	// UV値を取得.
	sxsdk::vec2 uv(0, 0);
	if (data.uvIndex == 0) {
		uv = sxsdk::vec2(get_s(), get_t());		// UV1(距離UV).
	} else {
		uv = sxsdk::vec2(get_u(), get_v());		// UV2(パラメータUV).
	}

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

/**
 * パラメータの指定.
 */
bool COcclusionTextureShaderInterface::ask (sxsdk::stream_interface *stream, void *)
{
	compointer<sxsdk::dialog_interface> dlg(m_shade->create_dialog_interface_with_uuid(OCCLUSION_SHADER_INTERFACE_ID));
	dlg->set_resource_name("occlusion_mapping_dlg");

	dlg->set_responder(this);
	this->AddRef();			// set_responder()に合わせて、参照カウンタを増やす。 .

	// shader_interfaceのstreamとmapping_layer_classのstreamは別もので、相互に取得する関数はない模様。.
	// sxsdk::scene_interfaceでのsceneから指定のマスターサーフェスのshader_interfaceを手繰り寄せる手段がない.
	// stream情報がゲットできない.
	// であるので、shader_interfaceのstreamを保存時にmapping_layer_classのstreamにも同じ情報を保存する.

	try {
		compointer<sxsdk::scene_interface> scene(m_shade->get_scene_interface());
		sxsdk::mapping_layer_class* pMLayer = Shade3DUtil::getActiveShapeOcclusionMappingLayer(scene);

		// mapping_layerのstreamから情報を読み込み.
		if (pMLayer) {
			StreamCtrl::loadOcclusionParam(*pMLayer, m_data);
		}

		// ダイアログの表示.
		if (dlg->ask()) {
			// shader_interfaceのstreamに情報を保持.
			StreamCtrl::saveOcclusionParam(stream, m_data);

			// mappingLayerに情報を保持.
			if (pMLayer) {
				StreamCtrl::saveOcclusionParam(*pMLayer, m_data);
			}
			return true;
		}

	} catch (...) { }

	return false;
}

//--------------------------------------------------.
//  ダイアログのイベント処理用.
//--------------------------------------------------.
/**
 * ダイアログの初期化.
 */
void COcclusionTextureShaderInterface::initialize_dialog (sxsdk::dialog_interface &d, void *)
{
}

/** 
 * ダイアログのイベントを受け取る.
 */
bool COcclusionTextureShaderInterface::respond (sxsdk::dialog_interface &d, sxsdk::dialog_item_class &item, int action, void *)
{
	const int id = item.get_id();		// アクションがあったダイアログアイテムのID.

	if (id == sx::iddefault) {
		m_data.clear();				// パラメータを初期化.
		load_dialog_data(d);		// ダイアログのパラメータを更新.
		return true;
	}

	if (id == dlg_uvIndex_id) {
		m_data.uvIndex = item.get_selection();
		return true;
	}

	return false;
}

/**
 * ダイアログのデータを設定する.
 */
void COcclusionTextureShaderInterface::load_dialog_data (sxsdk::dialog_interface &d, void *)
{
	{
		sxsdk::dialog_item_class* item;
		item = &(d.get_dialog_item(dlg_uvIndex_id));
		item->set_selection(m_data.uvIndex);
	}
}

/**
 * 値の変更を保存するときに呼ばれる.
 */
void COcclusionTextureShaderInterface::save_dialog_data (sxsdk::dialog_interface &d, void *)
{
}

