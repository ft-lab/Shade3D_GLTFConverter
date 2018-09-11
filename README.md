# glTF Converter for Shade3D

Shade3DでglTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
glTFの処理で、「Microsoft glTF SDK」( https://github.com/Microsoft/glTF-SDK )を使用しています。

## 機能

以下の機能があります。

* glTF 2.0の拡張子「gltf」「glb」の3D形状ファイルをShade3Dに読み込み (インポート)
* Shade3DのシーンからglTF 2.0の「gltf」「glb」の3D形状ファイルを出力 （エクスポート）

## 動作環境

* Windows 7/8/10以降のOS    
* macOS 10.11以降   
* Shade3D ver.16/17以降で、Standard/Professional版（Basic版では動作しません）  
  Shade3Dの64bit版のみで使用できます。32bit版のShade3Dには対応していません。   

## 使い方

### プラグインダウンロード

以下から最新版をダウンロードしてください。  
https://github.com/ft-lab/Shade3D_GLTFConverter/releases

### プラグインを配置し、Shade3Dを起動

※ glTF Converter ver.0.2.0.0以降で「MotionUtil」プラグインが必要となりました。    
「MotionUtil」( https://github.com/ft-lab/Shade3D_MotionUtil )は、Morph Targetsやモーション処理のユーティリティプラグインです。    

Windowsの場合は、ビルドされた glTFConverter64.dllとMotionUtil64.dll をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
Macの場合は、ビルドされた glTFConverter.shdpluginとMotionUtil.shdplugin をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」が表示されるのを確認します。  

### 使い方

Shade3Dでのシーン情報をエクスポートする場合、  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」を選択し、指定のファイルにgltfまたはglb形式でファイル出力します。  
ファイルダイアログボックスでのファイル指定では、デフォルトではglb形式のバイナリで出力する指定になっています。   
WIn環境の場合はここで拡張子を「gltf」にすると、テキスト形式のgltfファイル、バイナリデータのbinファイル、テクスチャイメージの各種ファイルが出力されます。  

gltf/glbの拡張子の3DモデルデータをShade3Dのシーンにインポートする場合、   
メインメニューの「ファイル」-「インポート」-「glTF(gltf/glb)」を選択し、gltfまたはglbの拡張子のファイルを指定します。  

## サンプルファイル

サンプルのShade3Dのファイル(shd)、エクスポートしたglbファイルを https://ft-lab.github.io/gltf.html に置いています。    

## glTF入出力として対応している機能

* メッシュ情報の入出力
* シーンの階層構造の入出力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を出力
* ボーンやスキン情報の入出力
* ポリゴンメッシュの頂点カラーの入出力 (頂点カラー0のみ) ver.0.1.0.3で追加。    
頂点カラーとしてRGBAの要素を使用しています。
* テクスチャマッピングの「アルファ透明」の入出力。ver.0.1.0.4で追加。  
* 表面材質の「透明」の入出力。ver.0.1.0.13で追加。    
* ボーンを使用したスキンアニメーション情報の入出力。ver.0.1.0.6で追加。   
* Morph Targets情報の入出力。ver.0.2.0.0で追加。ただし、Morph Targetsを使用したキーフレームアニメーションはまだ未対応です。    

PBR表現としては、metallic-roughnessマテリアルモデルとしてデータを格納しています。  

### インポート時のdoubleSidedのマテリアルについて (ver.0.1.0.11 - )

インポート時にdoubleSidedの属性を持つマテリアルは、    
Shade3Dのマスターサーフェス名に「_doubleSided」が付きます。    
なお、Shade3DのレンダリングではdoubleSidedでなくても両面が表示されることになります。    

### エクスポート時のdoubleSidedの対応について

glTFフォーマットでは、デフォルトでは表面のみ表示されます。   
裏面は非表示となります。   
裏面を表示させるには、表面材質に割り当てるマスターサーフェス名にて「doublesided」のテキストを含めると、その表面材質はdoubleSided対応として出力されます。

### エクスポート時の「アルファ透明」の対応について (ver.0.1.0.4 - )

表面材質のマッピングレイヤとして、「イメージ/拡散反射」の「アルファ透明」を選択した場合、    
元のイメージがpngのAlpha要素を持つ場合に、透過処理(glTFのalphaMode:MASK)が行われます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_alpha_blend.png"/>     

この場合、エクスポート時のオプションで「テクスチャ出力」を「jpegに置き換え」としたときも、png形式で出力されます。

### エクスポート時の「透明」の対応について (ver.0.1.0.13 - )

表面材質の「透明」が0.0よりも大きい場合、    
「1.0 - 透明」の値がglTFのpbrMetallicRoughnessのbaseColorFactorのAlphaに格納されます。    
このとき、glTFのalphaMode:BLENDが使用されます。    

### エクスポート時のボーンとスキンについて

GLTF/GLBエクスポート時に、ボーンとスキンを持つ形状の場合は、その情報も出力されます。   
このとき、Shade3Dの「シーケンス Off」としての姿勢で出力されます。  
なお、ver.0.1.0.0ではシーケンス On時の姿勢はエクスポートで出力していません。   

スキンのバインド情報は、ポリゴンメッシュの1頂点に対して最大4つまでです。   

### エクスポート時の出力をスキップする形状について

ブラウザでレンダリングをしない指定にしてる場合、形状名の先頭に「#」を付けている場合は、その形状はエクスポートされません。

### インポート/エクスポート時のボーンについて

ボーンで、変換行列に回転/せん断/スケール要素を与えると正しくインポート/エクスポートされません。    
変換行列では移動要素のみ使用するようにしてください。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_bone_matrix.png"/>     
Shade3Dでボーンを扱う際は、「軸の整列」の「自動で軸合わせ」をオフにして使用するようにしてください。   
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_bone_matrix_2.png"/>     

### エクスポートオプション

エクスポート時にダイアログボックスが表示され、オプション指定できます。   
自由曲面や掃引体/回転体などの分割は「曲面の分割」で精度を指定できます。    
「テクスチャ出力」で「イメージから拡張子を参照」を選択すると、   
マスターイメージ名で「.png」の拡張子が指定されていればpngとして、   
「.jpg」の拡張子が指定されていればjpegとしてテクスチャイメージが出力されます。   
「テクスチャ出力」で「pngに置き換え」を選択すると、すべてのイメージはpngとして出力されます。   
「jpegに置き換え」を選択すると、すべてのイメージはjpgとして出力されます。   
ただし、テクスチャマッピングで「アルファ透明」を選択している場合は強制的にpng形式で出力されます。    
「ボーンとスキンを出力」チェックボックスをオンにすると、glTFファイルにボーンとスキンの情報を出力します。   
ファイルサイズを小さくしたい場合やポージングしたそのままの姿勢を出力したい場合はオフにします。   
「頂点カラーを出力」チェックボックスをオンにすると、ポリゴンメッシュに割り当てられた頂点カラー情報も出力します。    
「アニメーションを出力」チェックボックスをオンにすると、ボーン＋スキンのモーションを割り当てている場合にそのときのキーフレーム情報を出力します。    

「Asset情報」で主にOculus Homeにglbファイルを持っていくときの情報を指定できます(ver.0.1.0.8 追加)。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_dlg_asset.png"/>     
※ Oculus Homeでは全角の指定は文字化けしますので、半角英数字で指定する必要があります。    
「タイトル」で3Dモデルの名前を指定します。    
「作成者」で3Dモデルの作成者を指定します。    
「ライセンス選択」でライセンスの種類を、All rights reserved、Creative Commons、Public domain、その他、から指定します。    
その他の場合は、「ライセンス」に任意のテキストを入力できます。    
「ライセンス選択」で選択されたときの説明文が表示されますので、もっとも適するライセンス形態を選ぶようにします。    
「原型モデルの参照先」で、モデルデータを提供しているURLなどを指定します。    

ライセンスの種類は以下のものがあります。    

|ライセンスの種類|説明|    
|----|----|    
|All rights reserved|著作権を持ちます|   
|CC BY-4.0 ( https://creativecommons.org/licenses/by/4.0/ )|作品のクレジットを表示する必要あり。改変OK/営利OK。|   
|CC BY-SA-4.0 ( https://creativecommons.org/licenses/by-sa/4.0/ )|作品のクレジットを表示する必要あり。改変OK/営利OK/継承。|   
|CC BY-NC-4.0 ( https://creativecommons.org/licenses/by-nc/4.0/ )|作品のクレジットを表示する必要あり。改変OK/非営利。|   
|CC BY-ND-4.0 ( https://creativecommons.org/licenses/by-nd/4.0/ )|作品のクレジットを表示する必要あり。改変禁止/営利OK。|   
|CC BY-NC-SA-4.0 ( https://creativecommons.org/licenses/by-nc-sa/4.0/ )|作品のクレジットを表示する必要あり。改変OK/非営利/継承。|   
|CC BY-NC-ND-4.0 ( https://creativecommons.org/licenses/by-nc-nd/4.0/ )|作品のクレジットを表示する必要あり。改変禁止/非営利。|   
|CC0(Public domain) ( https://creativecommons.org/publicdomain/zero/1.0/ )|著作権を放棄します|   

「Creative Commons」については「 https://creativecommons.jp/licenses/ 」を参照してください。    
「Public domain」については「 https://creativecommons.org/publicdomain/zero/1.0/ 」を参照してください。    

### インポートオプション

インポート時にダイアログボックスが表示され、オプション指定できます。   
「イメージのガンマ補正」で1.0を選択すると、読み込むテクスチャイメージのガンマ補正はそのまま。   
1.0/2.2を選択すると、リニアワークフローを考慮した逆ガンマ値をテクスチャイメージに指定します。   
この逆ガンマを与えるテクスチャイメージは「BaseColor」「Emissive」のみです。    
「Roughness/Metallic」「Occlusion」「Normal」はガンマ補正を行わず、リニアとして読み込まれます。    
「アニメーションを読み込み」チェックボックスをオンにすると、ボーン＋スキンのモーション情報を読み込みます。    
「法線を読み込み」チェックボックスをオンにすると、「ベイクされる形」でポリゴンメッシュの法線が読み込まれます。   
Shade3Dではこれはオフにしたままのほうが都合がよいです。   
「限界角度」はポリゴンメッシュの限界角度値です。   
※ glTFには法線のパラメータはありますが、Shade3Dの限界角度に相当するパラメータは存在しません。   
「頂点カラーを読み込み」チェックボックスをオンにすると、メッシュに頂点カラーが割り当てられている場合はその情報も読み込まれます。    


### インポート時の表面材質の構成について

拡散反射色として、glTFのBaseColor(RGB)の値が反映されます。   
光沢1として、glTFのMetallic Factorの値が反映されます。このとき、0.3以下の場合は0.3となります。   
光沢1のサイズは0.7固定です。   
発光色として、glTFのEmissive Factorの値が反映されます。   
反射値として、glTFのMetallic Factorの値が反映されます。   
荒さ値として、glTFのRoughness Factorの値が反映されます。   
透明値として、glTFのpbrMetallicRoughnessのbaseColorFactorの「1.0 - Alpha」が反映されます(ver.0.1.0.13 -)。     

マッピングレイヤは以下の情報が格納されます。   
複雑化しているのは、PBR表現をなんとかShade3Dの表面材質でそれらしく似せようとしているためです。    

|glTFでのイメージの種類|マッピングレイヤ|合成|適用率|反転|   
|----|----|----|----|----|   
|BaseColor(RGB) |イメージ/拡散反射|乗算 (※ ver.0.1.0.15 仕様変更)|1.0|—|   
|Normal(RGB) |イメージ/法線|通常|法線マップのscale値 (※ ver.0.1.0.14 対応)|—|   
|Emissive(RGB) |イメージ/発光|通常|1.0|—|   
|Roughness MetallicのMetallic(B)|イメージ/拡散反射|乗算|Metallic Factorの値 (※ ver.0.1.0.9 仕様変更)|o|   
|Roughness MetallicのMetallic(B)|イメージ/反射|通常|1.0|—|   
|Base Color(RGB) |イメージ/反射|乗算|1.0|-|   
|Roughness MetallicのRoughness(G)|イメージ/荒さ|通常|1.0|o|   
|Roughness MetallicのOcclusion(R)|イメージ/拡散反射|乗算|1.0|—|   

(R)(G)(B)は、テクスチャイメージのRGB要素のどれを参照するかを表します。   
glTFでは、Roughness Metallicモデルの場合は「Roughness(G) Metallic(B)」がパックされている、もしくは、   
「Occlusion(R) Roughness(G) Metallic(B)」がパックされて1枚のイメージになっています。   
別途Occlusionイメージがある場合は、「Occlusion (glTF)/拡散反射」の乗算としてマッピングレイヤに追加されます(後述)。   
Ambient Occlusionの効果はマッピングレイヤの拡散反射の乗算として表現されます。    

### Occlusionのマッピングレイヤ (ver.0.1.0.12 対応)

glTFでRoughness MetallicのOcclusion(R)要素を使用せず、単体のOcclusionテクスチャイメージを持つ場合、    
インポート時はglTF Converterプラグインで用意している「Occlusion (glTF)」のレイヤが使用されます。    
これは、「Occlusion (glTF)/拡散反射」として「乗算」合成を割り当てて使用します。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_mapping_layer_occlusion_01.png"/>     
レンダリングした場合、「イメージ/拡散反射」のマッピングレイヤと同じ挙動になります。    

Occlusionテクスチャイメージがマッピングレイヤで指定されている場合は、    
エクスポート時にOcclusionテクスチャとして出力されます。     

制限事項として、Occlusionマッピングレイヤは表面材質のマッピングレイヤで1レイヤのみ指定できます。    
UV1/UV2のいずれかを指定できます(ver.0.1.0.13-)。投影は「ラップ（UVマッピング）」になります。    
「適用率」の指定が、glTFのocclusionTexture.strengthになります。    

UV1/UV2の選択は、「Occlusion (glTF)/拡散反射」マッピングレイヤで「その他」ボタンを押し、表示されるダイアログボックスの「UV」で指定します。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_mapping_layer_occlusion_uv.png"/>     

### Occlusionのマッピングレイヤをインポートした場合の注意事項 (ver.0.1.0.13 - )

glTFインポート時、「Occlusion (glTF)/拡散反射」マッピングレイヤでのUV層の選択が、読み込んだ直後はレンダリングに反映されません。    
(Shade3Dのsxsdk::shader_interfaceに外部からアクセスできないため)     
内部パラメータとしては反映されています。    
Shade3DにglTFをインポートした後は、一度表面材質の「Occlusion (glTF)/拡散反射」マッピングレイヤで「その他」ボタンを押し、OKボタンで確定する必要があります。    

### 表面材質（マテリアル）の拡散反射値と反射値の関係 (ver.0.1.0.9 対応)

Shade3Dでは、鏡のような反射を表現する場合は拡散反射値を暗くする必要があります。    
glTFではBaseColorを暗くすると鏡のような反射にはならないため、インポート時(glTFからShade3Dへの変換)は以下のように変換しています。    

    const sxsdk::rgb_class whiteCol(1, 1, 1);
    const sxsdk::rgb_class col = glTFのBaseColor Factor;
    const float metallicV  = glTFのMetallic Factor;
    const float metallicV2 = 1.0f - metallicV;
    const sxsdk::rgb_class reflectionCol = col * metallicV + whiteCol * metallicV2;

「拡散反射色」はglTFのBaseColor Factorを採用。    
Roughness Metallicテクスチャがない場合は「拡散反射値」は「1.0 - glTFのMetallic Factor」を採用し、「反射色」は上記で計算したreflectionColを採用。    
Roughness Metallicテクスチャがある場合は「拡散反射値」は1.0、「反射色」は白を採用。    
「反射値」はglTFのMetallic Factorを採用。     

エクスポート時(Shade3DからglTFへの変換)は以下のように変換しています。    

    const sxsdk::rgb_class col0 = surface->get_diffuse_color();    
    sxsdk::rgb_class col = col0 * (surface->get_diffuse());    
    sxsdk::rgb_class reflectionCol = surface->get_reflection_color();    
    const float reflectionV  = std::max(std::min(1.0f, surface->get_reflection()), 0.0f);    
    const float reflectionV2 = 1.0f - reflectionV;    
    col = col * reflectionV2 + reflectionCol * reflectionV;    
    col.red   = std::min(col0.red, col.red);    
    col.green = std::min(col0.green, col.green);    
    col.blue  = std::min(col0.blue, col.blue);     

glTFのBaseColor Factorとして上記で計算したcolを採用。    
glTFのMetallic FactorとしてShade3Dの反射値を採用。    

鏡のような鏡面反射をする場合は、「拡散反射」の色を白に近づけるようにし拡散反射のスライダを0.0に近づけるようにします。    
この場合「反射」値は1.0に近づく指定をしています。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_mapping_layer_diffuse_reflection_02.png"/>     
こうすることで、glTF出力後もShade3Dと似た表現になります（あくまでも近似です）。    

黒く光るような反射をする場合は、「拡散反射」の色を反射後の色（ここでは黒）に近づけるようにし拡散反射のスライダを0.0に近づけるようにします。    
この場合「反射」値は0.0より大きな小さめの値を指定しています。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_mapping_layer_diffuse_reflection_01.png"/>     

### エクスポート時の表面材質のマッピングレイヤについて (ver.0.1.0.9 対応)

拡散反射の乗算合成としてOcclusionを指定している場合、glTF出力時にはBaseColorにベイクされます。    
別途、glTFのOcclusionテクスチャは出力していません。    
マッピングレイヤの「イメージ/拡散反射」「イメージ/発光」「イメージ/反射」「イメージ/荒さ」を複数指定している場合、glTF出力用に1枚にベイクされます。    
「イメージ/法線」は1枚のみ参照します。    
この場合、以下の指定が使用できます。    

「合成」は、「通常」「加算」「減算」「乗算」「比較（明）」「比較（暗）」を指定できます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_blend_mode.png"/>     
「色反転」、イメージタブの「左右反転」「上下反転」のチェックボックスを指定できます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_export_mapping_layer_01.png"/>     
「チャンネル合成」の指定は、「イメージ/拡散反射」の場合は「アルファ乗算済み」「アルファ透明」を指定できます。    
「グレイスケール（平均）」「グレイスケール(R)」「グレイスケール(G)」「グレイスケール(B)」「グレイスケール(A)」を指定できます。    

なお、「反復」の指定はver.0.1.0.9でいったん仕様外にしました。正しく動作しません。    

「イメージ/拡散反射」「イメージ/反射」のマッピングレイヤを元に、glTFのBaseColor/Metallicを計算しています。    
「イメージ/荒さ」のマッピングレイヤを元に、glTFのRoughnessを計算しています。    
「イメージ/発光」のマッピングレイヤはglTFのEmissiveのテクスチャとして反映しています。    
「イメージ/法線」のマッピングレイヤはglTFのNormalのテクスチャとして反映しています。    

「頂点カラー/拡散反射」を指定できます(ただし、頂点カラー1のみ使用)。    
「イメージ/法線」では、適用率がglTFの法線マップのscale値として格納されます（ver.0.1.0.14 対応）。    

## Morph Targetsの対応 (ver.0.2.0.0 - )

glTF Converter 0.2.0.0以降で、Morph Targetsに対応しています。   Shade3D ver.17/18段階では、本体機能でMorph Targetsがないため、プラグインとして拡張しています。    
  
この機能は「MotionUtil」プラグインのほうで実装されています。    

Morph Targetsとは、ポリゴンメッシュの頂点移動による変形を登録し、ウエイト値により変形を制御する手段です。    
glTF Converter 0.2.0.0では、Morph Targetsの登録とウエイト値の割り当てのみに対応しています。    
アニメーションでのキーフレーム登録はまだ未実装です。    

<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_MorphTargets_01.png"/>   
メインメニューの「表示」-「Morph Targets」でMorph Targetsウィンドウを表示します。    

※ Morph Targets情報はUNDO/REDOに対応していません。    

Morph Targets情報は、個々のポリゴンメッシュ形状ごとに持つことができます。

### Morph Targets情報を割り当て

ブラウザで選択したポリゴンメッシュに対して、Morph Targetsウィンドウの「Morph Targets情報を割り当て」ボタンを押すと、    
そのときの頂点位置がベース情報として保持されます。    
Morph Targetsの動作を行う場合は、はじめにこの処理が必要になります。    

### Morph Targets情報を削除

ブラウザで選択したポリゴンメッシュに対して、Morph Targetsウィンドウの「Morph Targets情報を削除」ボタンを押すと、    
メッシュに割り当てられたすべてのMorph Targets情報が削除されます。    
このとき、「頂点を戻す」チェックボックスをオンにしておくと、はじめに登録したベースの頂点で戻します。    

### Morph Target情報を追加登録

このターゲットの登録操作は、「Morph Targets情報を割り当て」ではじめに登録した頂点位置をベースとした差分で管理されます。    

ブラウザで選択したポリゴンメッシュが対象になります。    
形状編集モード＋頂点選択モードでポリゴンメッシュの頂点を選択して任意の形に移動させます（頂点削除や追加はしないでください！）。    
その後 Morph Targetsウィンドウの「Morph Target情報を追加登録」ボタンを押すと、   
選択された頂点がウエイト値1.0のターゲットとして新規追加されます。    
追加されたターゲットは、Morph Targetsウィンドウのリストボックスに表示されます。    
リストボックス右の「更新」ボタンを押すと、そのターゲット情報を選択された頂点とその位置で更新します。    
リストボックス右の「削除」ボタンを押すと、そのターゲット情報を削除します。    
このときに、ベースの頂点位置に戻した上でターゲットが削除されます。    
リストボックスのスライダを0.0-1.0に変化させることで、ベースとして登録した頂点位置から    
ターゲットとして追加した頂点位置に変形します。    
複数のターゲットがある場合は、変形は加算されます。    

### Morph Target対象の頂点を選択

リストボックス部でターゲットを選択して    
Morph Targetsウィンドウの「Morph Target対象の頂点を選択」ボタンを押すと、   
そのターゲットとして登録したときの変形対象の頂点が選択されます。   

### ターゲット名を変更

リストボックス部で、ターゲット名の箇所をダブルクリックすると名前変更ダイアログボックスが表示されます。    
デフォルトでは「target」となっています。    

### Morph Targetsの制限事項

今後、仕様が変更/追加になる場合があります。    

* ポリゴンメッシュの頂点数が変わった場合、正しく動作しません。    
全てのモデリング工程が完了した段階でMorph Targetsの変形を使用するようにしてください。
* アニメーションのキーフレーム割り当てにはまだ対応していません。    
* VRM形式のファイルをインポートした場合にMorph Targets情報を持つとき、    
MorphTargets名が表示されます。    
このMorphTargets名はglTFフォーマットとしては情報がありません（VRMとしての情報になります）。    
そのため、glTF出力するとMorph Targets名はデフォルトの「target」となります。    
VRM出力に対応したときに、このあたりは対処予定です。
* glTFエクスポート時は、Morph Targetsとして法線やTangentは出力していません。    
この場合、ベースの頂点での法線が採用されます。

## 制限事項

* カメラ情報の入出力には対応していません。  
* Shade3Dでの自由曲面や掃引体/回転体は、エクスポート時にポリゴンメッシュに変換されます。   
* エクスポートされたポリゴンメッシュはすべて三角形分割されます。   
* レンダリングブーリアンには対応していません。  
* 表面材質のマッピングレイヤは、イメージマッピングのみ対応しています。ソリッドテクスチャの出力には対応していません。また、マッピングはUVのみ使用できます。 
* BaseColorのAlpha要素（Opasity）のインポート/エクスポートは、表面材質のマッピングレイヤの「アルファ透明」を参照します (ver.0.1.0.4 追加)。
* スキンは「頂点ブレンド」の割り当てのみに対応しています。
* ジョイントは、ボールジョイント/ボーンジョイントのみに対応しています。
* 表面材質のマッピングレイヤでの反復指定は仕様外にしました(ver.0.1.0.9 仕様変更)。    
* アニメーションは、ボーン+スキンでのモーション指定のみに対応しています(ver.0.1.0.6 追加)。    
* ボーンの変換行列では移動要素のみを指定するようにしてください。回転/せん断/スケールの指定には対応していません。    
* glTFインポート時、「Occlusion (glTF)/拡散反射」マッピングレイヤでのUV層の選択が、読み込んだ直後はレンダリングに反映されません。    

## 制限事項 (glTFフォーマット)

* ポリゴンメッシュはすべて三角形に分解されます。   
* ポリゴンメッシュの1頂点にてスキンの影響を受けるバインドされた情報は最大4つまで使用できます。
* 階層構造としては、GLTFのNodeとして出力します。   
この際に、移動/回転/スケール要素で渡すようにしており、Shade3DのGLTF Exporterでは「せん断」情報は渡していません。   
* テクスチャイメージとして使用できるファイルフォーマットは、pngまたはjpegのみです。   
* UV層は0/1番目を使用できます(ver.0.1.0.1 追加)。   
* ポリゴンメッシュの頂点に割り当てる頂点カラーは、頂点カラー番号0のみを乗算合成として使用できます (ver.0.1.0.3 追加)。
* Morph Targetsの名前はglTFファイルとしての情報がないため、出力されません (ver.0.2.0.0 追加)。  

## VRMフォーマットの暫定対応 (ver.0.2.0.0 - )

インポート時に、VRMファイルを読み込むことができます。    
VRM ( https://dwango.github.io/vrm/ )は、glTFを拡張したキャラクタに特化した形式です。    
glTF Converterではまだまだ対応できていません。  
以下、対応した項目です。    

* Import : ライセンス情報を読み込み、メッセージウィンドウに表示します。  
* Import : Morph Targetsの名前をShade3Dにインポートします。  

## 注意事項    

Shade3Dに3Dモデルをインポートして改変などを行う、またはエクスポートする際は    
オリジナルの著作権/配布条件を注意深く確認するようにしてください。    
個々の3Dモデルの権利については、それぞれの配布元の指示に従うようにしてください。    
glTF Converter for Shade3Dでは個々の3Dモデルの扱いについては責任を負いません。    

## ビルド方法 (開発者向け)

Shade3DプラグインSDK( https://github.com/shadedev/pluginsdk )をダウンロードします。  
Shade3D_GLTFConverter/projects/GLTFConverterフォルダをShade3DのプラグインSDKのprojectsフォルダに複製します。  
Microsoft glTF SDK ( https://github.com/Microsoft/glTF-SDK )は事前にビルドしておく必要があります。    

### WindowsでのMicrosoft glTF SDKのビルド

GLTFSDK.slnをVS2017で開き、ビルドします。    
「Built/Out/v141/x64/Release/GLTFSDK」に静的ライブラリ「GLTFSDK.lib」が生成されます。    
この静的ライブラリと、
「GLTFSDK/Inc」の下の「GLTFSDK」ディレクトリとその中のファイル、    
「packages/rapidjson.temprelease.0.0.2.20/build/native/include」の下の「rapidjson」ディレクトリとその中のファイル、    
を使用します。    

### MacでのMicrosoft glTF SDKのビルド

CMakeでXcode用のプロジェクトを作成し、ビルドします。    
ビルド時にpowershellのインストールが必要です。    
参考 : https://docs.microsoft.com/ja-jp/powershell/scripting/setup/installing-powershell-core-on-macos?view=powershell-6    
「pwsh」が実行ファイルになるため、「powershell」とシンボリックリンクしておきます。    

また、Project Settingsの「C++ Language Version」にて「GNU++14」としておく必要があります（std::make_uniqueを使用しているため）。    

「Buildディレクトリ/GLTFSDK/Release」に静的ライブラリ「libGLTFSDK.a」が生成されます。    
この静的ライブラリと、
「GLTFSDK/Inc」の下の「GLTFSDK」ディレクトリとその中のファイル、    
「Buildディレクトリ/RapidJSON-src/include」の下の「rapidjson」ディレクトリとその中のファイル、    
を使用します。    

### Windows
```c
  [GLTFConverter]  
    [source]             プラグインのソースコード  
    [win_vs2017]         
    　GLTFConverter.sln　　Win(vs2017)のプロジェクトファイル  　　
      [GLTFSDK]
        [include]
          [GLTFSDK]      glTF SDKで使用するヘッダファイル類    
          [rapidjson]    rapidjsonで使用するヘッダファイル類 
        [lib]
          [debug]
            GLTFSDK.lib  Debug用のGLTFSDKの静的ライブラリ
          [release]
            GLTFSDK.lib  Release用のGLTFSDKの静的ライブラリ
```
[GLTFSDK]ディレクトリ内が、Shade3Dのプラグイン以外で必要なMicrosoft glTF SDKの関連ファイルです。    
GLTFConverter/win_vs2017/GLTFConverter.sln をVS2017で開き、ビルドします。  

### Mac
```c
  [GLTFConverter]  
    [source]             プラグインのソースコード  
    [mac]                
      [plugins]
        Template.xcodeproj   Macのプロジェクトファイル 
      [GLTFSDK]
        [include]
          [GLTFSDK]      glTF SDKで使用するヘッダファイル類    
          [rapidjson]    rapidjsonで使用するヘッダファイル類 
        [lib]
          [release]
            libGLTFSDK.a  Release用のGLTFSDKの静的ライブラリ
```
[GLTFSDK]ディレクトリ内が、Shade3Dのプラグイン以外で必要なMicrosoft glTF SDKの関連ファイルです。    
GLTFConverter/mac/plugins/Template.xcodeproj をXcodeで開き、ビルドします。  

## 使用しているモジュール (開発者向け)

* Microsoft glTF SDK ( https://github.com/Microsoft/glTF-SDK )
* rapidjson ( https://github.com/Tencent/rapidjson/ )

rapidjsonは、Microsoft glTF SDK内で使用されています。   

## ライセンス  

This software is released under the MIT License, see [LICENSE](./LICENSE).  

## 更新履歴

[2018/09/08] ver.0.2.0.0   
* [内部処理] glTF処理ライブラリを「Microsoft.glTF.CPP」から「Microsoft glTF SDK」に置き換え  
* [内部処理] モーション処理部を「MotionUtil」という別プラグインに分けて管理  
* Mac対応
* Import/Export : Morph Targets情報の入出力(アニメーションのキーフレームはまだです)  
* Import : Importダイアログボックスでvrm形式を選択できるようにした  
* Import : VRMのライセンス情報とMorph Targets名を読み込み  

[2018/08/18] ver.0.1.0.15   
* Import : 表面材質の拡散反射レイヤを追加する際に、合成は「乗算」とするようにした（glTFでは、BaseColorFactorとBaseColorTextureは乗算する仕様のため）。
* Import : skinのskeleton要素を持つ場合、skeletonはルートボーンとなるためそのノード以降はボーンと判断。    

[2018/08/13] ver.0.1.0.14   
* Import/Export : 表面材質のNormalレイヤで「適用率」を反映するようにした。    

[2018/07/19] ver.0.1.0.13   
* Import/Export : 表面材質のOcclusionレイヤでUV1/UV2を選択できるようにした。    
* Import/Export : 表面材質の「透明」に対応。    
* Export : 表面材質で「反射」値を指定している場合の拡散反射色（BaseColor相当）の計算を微調整。

[2018/07/15] ver.0.1.0.12   
* 表面材質のマッピングレイヤとして、「Occlusion (glTF)」の種類を追加。    
* Import/Export : Occlusionテクスチャイメージの割り当ては、表面材質のマッピングレイヤの「Occlusion (glTF)/拡散反射」を使用するようにした。

[2018/07/13] ver.0.1.0.11   
* Import : マテリアルがdoubleSidedの場合は、マスターサーフェス名に「_doubleSided」を追加
* Export : 微小の面がある場合に、頂点カラーが正しく出力されないことがある問題を修正

[2018/07/05] ver.0.1.0.10   
* Export : UVが存在しない場合は、UVを出力しないように修正
* Import : Node階層で、glTF構造のSkinリストにない終端Nodeはパートのままとした (以前はボーンに変換していた)
* Import : 「イメージのガンマ補正」を有効にした場合に、Emissiveのテクスチャイメージにもガンマ補正をかけるようにした
* Import/Export : イメージ名に全角文字が入っていると不正になる問題を修正

[2018/06/30] ver.0.1.0.9   
* Export : マスターイメージ名で同じものがある場合、イメージが上書きされて意図しないマッピングになる問題を修正
* Export : Shade3DのDiffuse/Reflection/Roughnessテクスチャイメージより、BaseColor/Roughness/Metallicにコンバートする処理を実装
* Export : 複数のテクスチャがある場合に1枚にベイクする処理を実装
* Export : 表面材質のマッピングレイヤの「反復」の対応はいったん仕様外に。
* Export : 面積がゼロの面は出力しないようにした
* Import : マテリアル名で全角の名前を指定していた場合、うまくイメージが読み込めない問題を修正
* Import : テクスチャイメージのガンマ補正をかける場合、BaseColorのみを補正するようにした (それ以外はLinear)
* Import : インポート時のテクスチャイメージのガンマ補正が効かない場合があった問題を修正

[2018/06/23] ver.0.1.0.8   
* Export : エクスポート時のダイアログボックスで、Asset情報として「タイトル」「作成者」「ライセンス」「原型モデルの参照先」を指定できるようにした
* Export/Import : 出力/入力時のglTF/glbのファイルパスで全角のフォルダ名がある場合などに処理に失敗する問題を修正

[2018/06/20] ver.0.1.0.7   
* Export : 表面材質の「イメージ/拡散反射」で合成が「乗算」の場合は、Occlusionとして出力（暫定対応）
* Import : 一部の形状の読み込みで不正になる問題を修正 (glTFのbyteStrideの判別が正しくなかった)

[2018/06/14] ver.0.1.0.6   
* Export/Import : 表面材質のマッピングのテクスチャの反復に暫定対応 (extensionのKHR_texture_transform)。    
* Export/Import : ボーン＋スキンのアニメーション入出力に対応。

[2018/06/07] ver.0.1.0.5   
* Export/Import : 表面材質のマッピングレイヤでの「アルファ透明」は、glTFのalphaModeをBLENDからMASKに変更。    
この場合は、Export時にalphaCutoffを0.9として出力するようにした。
* Export : Export時のマスターサーフェス名の「doubleSided」指定がうまくglTFに反映されていなかった問題を修正。

[2018/06/07] ver.0.1.0.4   
* Export/Import : 表面材質のマッピングレイヤでの「アルファ透明」の対応。
* GLTF → glTF に名称変更。

[2018/06/05] ver.0.1.0.3   
* Export/Import : ポリゴンメッシュの頂点カラーの入出力に対応。頂点カラー0のみを使用します。

[2018/06/01] ver.0.1.0.2   
* Export : スキンを割り当てた場合に、出力結果が不正になる問題を修正。SkinのinverseBindMatricesの出力を行うようにしました。

[2018/05/30] ver.0.1.0.1   
* Export/Import : マテリアルのテクスチャ割り当てにて、UV1も割り当てできるようにした。

[2018/05/27] ver.0.1.0.0   
* 初回版 (Winのみ)
