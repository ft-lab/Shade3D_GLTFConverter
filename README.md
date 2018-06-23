# glTF Converter for Shade3D

Shade3DでglTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
glTFの処理で、「Microsoft.glTF.CPP」( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )を使用しています。

## 機能

以下の機能があります。

* glTF 2.0の拡張子「gltf」「glb」の3D形状ファイルをShade3Dに読み込み (インポート)
* Shade3DのシーンからglTF 2.0の「gltf」「glb」の3D形状ファイルを出力 （エクスポート）

## 動作環境

* Windows 7/8/10以降のOS  
* Shade3D ver.16/17以降で、Standard/Professional版（Basic版では動作しません）  
  Shade3Dの64bit版のみで使用できます。32bit版のShade3Dには対応していません。   

## 使い方

### プラグインダウンロード

以下から最新版をダウンロードしてください。  
https://github.com/ft-lab/Shade3D_GLTFConverter/releases

### プラグインを配置し、Shade3Dを起動

Windowsの場合は、ビルドされた glTFConverter64.dll をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」が表示されるのを確認します。  

### 使い方

Shade3Dでのシーン情報をエクスポートする場合、  
メインメニューの「ファイル」-「エクスポート」-「glTF(glb)」を選択し、指定のファイルにgltfまたはglb形式でファイル出力します。  
ファイルダイアログボックスでのファイル指定では、デフォルトではglb形式のバイナリで出力する指定になっています。   
ここで拡張子を「gltf」にすると、テキスト形式のgltfファイル、バイナリデータのbinファイル、テクスチャイメージの各種ファイルが出力されます。  

gltf/glbの拡張子の3DモデルデータをShade3Dのシーンにインポートする場合、   
メインメニューの「ファイル」-「インポート」-「glTF(gltf/glb)」を選択し、gltfまたはglbの拡張子のファイルを指定します。  

## glTF入出力として対応している機能

* メッシュ情報の入出力
* シーンの階層構造の入出力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Normal(法線)/Emissive(発光)を出力 (※1)
* ボーンやスキン情報の入出力
* ポリゴンメッシュの頂点カラーの入出力 (頂点カラー0のみ) ver.0.1.0.3で追加。    
頂点カラーとしてRGBAの要素を使用しています。
* テクスチャマッピングの「アルファ透明」の入出力。ver.0.1.0.4で追加。  
* ボーンを使用したスキンアニメーション情報の入出力。ver.0.1.0.6で追加。   

PBR表現としては、metallic-roughnessマテリアルモデルとしてデータを格納しています。  
※1 : ver.0.1.0.0では、エクスポート時のMetallic/Roughness/Occlusion出力はまだ未実装です。   

### エクスポート時のdoubleSidedの対応について

glTFフォーマットでは、デフォルトでは表面のみ表示されます。   
裏面は非表示となります。   
裏面を表示させるには、表面材質に割り当てるマスターサーフェス名にて「doublesided」のテキストを含めると、その表面材質はdoubleSided対応として出力されます。

### エクスポート時の「アルファ透明」の対応について (ver.0.1.0.4 - )

表面材質のマッピングレイヤとして、「イメージ/拡散反射」の「アルファ透明」を選択した場合、    
元のイメージがpngのAlpha要素を持つ場合に、透過処理(glTFのalphaMode:MASK)が行われます。    
<img src="https://github.com/ft-lab/Shade3D_GLTFConverter/blob/master/wiki_images/gltfConverter_alpha_blend.png"/>     

この場合、エクスポート時のオプションで「テクスチャ出力」を「jpegに置き換え」としたときも、png形式で出力されます。

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
ただし、法線マップはガンマ値1.0のままになります。   
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
発光色として、glTFの Emissive Factorの値が反映されます。   
反射値として、glTFのMetallic Factorの値が反映されます。   
荒さ値として、glTFのRoughness Factorの値が反映されます。   

マッピングレイヤは以下の情報が格納されます。   
複雑化しているのは、PBR表現をなんとかShade3Dの表面材質でそれらしく似せようとしているためです。    

|glTFでのイメージの種類|マッピングレイヤ|合成|適用率|反転|   
|----|----|----|----|----|   
|BaseColor(RGB) |イメージ/拡散反射|通常|1.0|—|   
|Normal(RGB) |イメージ/法線|通常|1.0|—|   
|Emissive(RGB) |イメージ/発光|通常|1.0|—|   
|Roughness MetallicのMetallic(B)|イメージ/拡散反射|乗算|0.5|o|   
|Roughness MetallicのMetallic(B)|イメージ/反射|通常|1.0|—|   
|Base Color(RGB) |イメージ/反射|乗算|1.0|-|   
|Roughness MetallicのRoughness(G)|イメージ/荒さ|通常|1.0|o|   
|Roughness MetallicのOcclusion(R)|イメージ/拡散反射|乗算|1.0|—|   

(R)(G)(B)は、テクスチャイメージのRGB要素のどれを参照するかを表します。   
glTFでは、Roughness Metallicモデルの場合は「Roughness(G) Metallic(B)」がパックされている、もしくは、   
「Occlusion(R) Roughness(G) Metallic(B)」がパックされて1枚のイメージになっています。   
別途Occlusionイメージがある場合は、そのイメージが拡散反射の乗算としてマッピングレイヤに追加されます。   
Ambient Occlusionの効果はマッピングレイヤの拡散反射の乗算として表現されます。    

### エクスポート時の表面材質のマッピングレイヤについて

「イメージ/拡散反射」のマッピングレイヤで合成が「乗算」の場合は、glTFのOcclusion(RGB)のテクスチャとして反映しています。    
それ以外の「イメージ/拡散反射」のマッピングレイヤはbaseColorのテクスチャとしました。    
「イメージ/法線」のマッピングレイヤはnormalのテクスチャとして反映しています。    
「イメージ/発光」のマッピングレイヤはemissiveのテクスチャとして反映しています。    

これ以外の「荒さ」「反射」は、本来はRoughness/Metallicに反映する必要があるものですが、まだ未調整です。    

## 制限事項

* カメラ情報の入出力には対応していません。  
* Shade3Dでの自由曲面や掃引体/回転体は、エクスポート時にポリゴンメッシュに変換されます。   
* エクスポートされたポリゴンメッシュはすべて三角形分割されます。   
* レンダリングブーリアンには対応していません。  
* 表面材質のマッピングレイヤは、イメージマッピングのみ対応しています。ソリッドテクスチャの出力には対応していません。また、マッピングはUVのみ使用できます。 
* 表面材質の色反転など、細かい指定には対応していません。   
* BaseColorのAlpha要素（Opasity）のインポート/エクスポートは、表面材質のマッピングレイヤの「アルファ透明」を参照します (ver.0.1.0.4 追加)。
* スキンは「頂点ブレンド」の割り当てのみに対応しています。
* ジョイントは、ボールジョイント/ボーンジョイントのみに対応しています。
* 表面材質のマッピングレイヤでの反復指定には暫定対応しています(ver.0.1.0.6 追加)。    
glTFのextensionのKHR_texture_transformを使用。    
* アニメーションは、ボーン+スキンでのモーション指定のみに対応しています(ver.0.1.0.6 追加)。    
* ボーンの変換行列では移動要素のみを指定するようにしてください。回転/せん断/スケールの指定には対応していません。    

## 制限事項 (glTFフォーマット)

* ポリゴンメッシュはすべて三角形に分解されます。   
* ポリゴンメッシュの1頂点にてスキンの影響を受けるバインドされた情報は最大4つまで使用できます。
* 階層構造としては、GLTFのNodeとして出力します。   
この際に、移動/回転/スケール要素で渡すようにしており、Shade3DのGLTF Exporterでは「せん断」情報は渡していません。   
* テクスチャイメージとして使用できるファイルフォーマットは、pngまたはjpegのみです。   
* UV層は0/1番目を使用できます(ver.0.1.0.1 追加)。   
* ポリゴンメッシュの頂点に割り当てる頂点カラーは、頂点カラー番号0のみを乗算合成として使用できます (ver.0.1.0.3 追加)。
* マテリアルに割り当てているテクスチャのタイリング（反復/繰り返し指定）をglTFのextensionのKHR_texture_transformとして出力していますが(ver.0.1.0.6 追加)、    
まだ規格としては固まっていないため今は暫定です。    
後々仕様変更される可能性があります。    

## ビルド方法 (開発者向け)

Shade3DプラグインSDK( https://github.com/shadedev/pluginsdk )をダウンロードします。  
Shade3D_GLTFConverter/projects/GLTFConverterフォルダをShade3DのプラグインSDKのprojectsフォルダに複製します。  
開発環境でnugetを使い「Microsoft.glTF.CPP」をインストールします。  
以下のようなフォルダ構成になります。  

```c
  [GLTFConverter]  
    [source]             プラグインのソースコード  
    [win_vs2017]         Win(vs2017)のプロジェクトファイル  
      packages.config    nugetでインストールされたパッケージ一覧.  
      [packages]         nugetでインストールされたパッケージ本体が入る.  
         [Microsoft.glTF.CPP.1.3.55.0]  
         [rapidjson.temprelease.0.0.2.20]  
```

GLTFConverter/win_vs2017/GLTFConverter.sln をVS2017で開き、ビルドします。  

## 使用しているモジュール (開発者向け)

* Microsoft.glTF.CPP ( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )
* rapidjson ( https://github.com/Tencent/rapidjson/ )

rapidjsonは、Microsoft.glTF.CPP内で使用されています。   

## ライセンス  

This software is released under the MIT License, see [LICENSE](./LICENSE).  

## 更新履歴

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
