# Shade3D_GLTFConverter

Shade3DでGLTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
GLTFの処理で、「Microsoft.glTF.CPP」( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )を使用しています。

## 機能

以下の機能があります。

* GLTF 2.0の拡張子「gltf」「glb」の3D形状ファイルをShade3Dに読み込み (インポート)
* Shade3DのシーンからGLTF 2.0の「gltf」「glb」の3D形状ファイルを出力 （エクスポート）

## 動作環境

* Windows 7/8/10以降のOS  
* Shade3D ver.16/17以降で、Standard/Professional版（Basic版では動作しません）  
  Shade3Dの64bit版のみで使用できます。32bit版のShade3Dには対応していません。   

## 使い方

### プラグインダウンロード

以下から最新版をダウンロードしてください。  
https://github.com/ft-lab/Shade3D_GLTFConverter/releases

### プラグインを配置し、Shade3Dを起動

Windowsの場合は、ビルドされた GLTFConverter64.dll をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
メインメニューの「ファイル」-「エクスポート」-「GLTF(glb)」が表示されるのを確認します。  

### 使い方

Shade3Dでのシーン情報をエクスポートする場合、  
メインメニューの「ファイル」-「エクスポート」-「GLTF(glb)」を選択し、指定のファイルにgltfまたはglb形式でファイル出力します。  
ファイルダイアログボックスでのファイル指定では、デフォルトではglb形式のバイナリで出力する指定になっています。   
ここで拡張子を「gltf」にすると、テキスト形式のgltfファイル、バイナリデータのbinファイル、テクスチャイメージの各種ファイルが出力されます。  

gltf/glbの拡張子の3DモデルデータをShade3Dのシーンにインポートする場合、   
メインメニューの「ファイル」-「インポート」-「GLTF(gltf/glb)」を選択し、gltfまたはglbの拡張子のファイルを指定します。  

## GLTF入出力として対応している機能

* メッシュ情報の入出力
* シーンの階層構造の入出力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入力
* マテリアル/テクスチャイメージとして、BaseColor(基本色)/Normal(法線)/Emissive(発光)を出力 (※1)
* ボーンやスキン情報の入出力

PBR表現としては、metallic-roughnessマテリアルモデルとしてデータを格納しています。  
※1 : ver.0.1.0.0では、エクスポート時のMetallic/Roughness/Occlusion出力はまだ未実装です。   

### エクスポート時のdoubleSidedの対応について

GLTFフォーマットでは、デフォルトでは表面のみ表示されます。   
裏面は非表示となります。   
裏面を表示させるには、表面材質に割り当てるマスターサーフェス名にて「doublesided」のテキストを含めると、その表面材質はdoubleSided対応として出力されます。

### エクスポート時のボーンとスキンについて

GLTF/GLBエクスポート時に、ボーンとスキンを持つ形状の場合は、その情報も出力されます。   
このとき、Shade3Dの「シーケンス Off」としての姿勢で出力されます。  
なお、ver.0.1.0.0ではシーケンス On時の姿勢はエクスポートで出力していません。   

スキンのバインド情報は、ポリゴンメッシュの1頂点に対して最大4つまでです。   

### エクスポート時の出力をスキップする形状について

ブラウザでレンダリングをしない指定にしてる場合、形状名の先頭に「#」を付けている場合は、その形状はエクスポートされません。

### エクスポートオプション

エクスポート時にダイアログボックスが表示され、オプション指定できます。   
自由曲面や掃引体/回転体などの分割は「曲面の分割」で精度を指定できます。    
「テクスチャ出力」で「イメージから拡張子を参照」を選択すると、   
マスターイメージ名で「.png」の拡張子が指定されていればpngとして、   
「.jpg」の拡張子が指定されていればjpegとしてテクスチャイメージが出力されます。   
「テクスチャ出力」で「pngに置き換え」を選択すると、すべてのイメージはpngとして出力されます。   
「jpegに置き換え」を選択すると、すべてのイメージはjpgとして出力されます。   
「ボーンとスキンを出力」チェックボックスをオンにすると、GLTFファイルにボーンとスキンの情報を出力します。   
ファイルサイズを小さくしたい場合やポージングしたそのままの姿勢を出力したい場合はオフにします。   

### インポートオプション

インポート時にダイアログボックスが表示され、オプション指定できます。   
「イメージのガンマ補正」で1.0を選択すると、読み込むテクスチャイメージのガンマ補正はそのまま。   
1.0/2.2を選択すると、リニアワークフローを考慮した逆ガンマ値をテクスチャイメージに指定します。   
ただし、法線マップはガンマ値1.0のままになります。   
「法線を読み込み」チェックボックスをオンにすると、「ベイクされる形」でポリゴンメッシュの法線が読み込まれます。   
Shade3Dではこれはオフにしたままのほうが都合がよいです。   
「限界角度」はポリゴンメッシュの限界角度値です。   
※ GLTFには法線のパラメータはありますが、Shade3Dの限界角度に相当するパラメータは存在しません。   

### インポート時の表面材質の構成について

拡散反射色として、GLTFのBaseColor(RGB)の値が反映されます。   
光沢1として、GLTFのMetallic Factorの値が反映されます。このとき、0.3以下の場合は0.3となります。   
光沢1のサイズは0.7固定です。   
発光色として、GLTFの Emissive Factorの値が反映されます。   
反射値として、GLTFのMetallic Factorの値が反映されます。   
荒さ値として、GLTFのRoughness Factorの値が反映されます。   

マッピングレイヤは以下の情報が格納されます。   
複雑化しているのは、PBR表現をなんとかShade3Dの表面材質でそれらしく似せようとしているためです。    

|GLTFでのイメージの種類|マッピングレイヤ|合成|適用率|反転|   
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
GLTFでは、Roughness Metallicモデルの場合は「Roughness(G) Metallic(B)」がパックされている、もしくは、   
「Occlusion(R) Roughness(G) Metallic(B)」がパックされて1枚のイメージになっています。   
別途Occlusionイメージがある場合は、そのイメージが拡散反射の乗算としてマッピングレイヤに追加されます。   
Ambient Occlusionの効果はマッピングレイヤの拡散反射の乗算として表現されます。    

## 制限事項

* アニメーション情報の入出力には対応していません。  
* カメラ情報の入出力には対応していません。  
* Shade3Dでの自由曲面や掃引体/回転体は、エクスポート時にポリゴンメッシュに変換されます。   
* エクスポートされたポリゴンメッシュはすべて三角形分割されます。   
* レンダリングブーリアンには対応していません。  
* 表面材質のマッピングレイヤは、イメージマッピングのみ対応しています。ソリッドテクスチャの出力には対応していません。   
* 表面材質の繰り返し回数指定や色反転など、細かい指定には対応していません。   
* BaseColorのAlpha要素（Opasity）のインポート/エクスポートは対応していません。
* UV層は0番目のみを使用します。   

## 制限事項 (GLTFフォーマット)

* ポリゴンメッシュはすべて三角形に分解されます。   
* ポリゴンメッシュの1頂点にてスキンの影響を受けるバインドされた情報は最大4つまで使用できます。
* 階層構造としては、GLTFのNodeとして出力します。   
この際に、移動/回転/スケール要素で渡すようにしており、Shade3DのGLTF Exporterでは「せん断」情報は渡していません。   
* テクスチャイメージとして使用できるファイルフォーマットは、pngまたはjpegのみです。   

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

[2018/05/27] ver.0.1.0.0   
* 初回版 (Winのみ)
