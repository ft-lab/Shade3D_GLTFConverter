# Shade3D_GLTFConverter

Shade3DでGLTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
GLTFの処理で、「Microsoft.glTF.CPP」( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )を使用しています。

## 機能

以下の機能があります。

* GLTF 2.0の拡張子「gltf」「glb」の3D形状ファイルをShade3Dに読み込み (インポート)
* Shade3DのシーンからGLTF 2.0の「gltf」「glb」の3D形状ファイルを出力 （エクスポート）

## 動作環境

* Windows 7/8/10以降のOS  
* Mac 10.9以降のOS  
* Shade3D ver.16/17以降で、Standard/Professional版（Basic版では動作しません）  

## 使い方

### プラグインダウンロード

以下から最新版をダウンロードしてください。  
https://github.com/ft-lab/Shade3D_GLTFConverter/releases

### プラグインを配置し、Shade3Dを起動

Windowsの場合は、ビルドされた GLTFConverter64.dll をShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
Macの場合は、ビルドされた GLTFConverter.shdpluginをShade3Dのpluginsディレクトリに格納してShade3Dを起動。  
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
* マテリアルとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入出力
* テクスチャイメージとして、BaseColor(基本色)/Metallic(メタリック)/Roughness(荒さ)/Normal(法線)/Emissive(発光)/Occlusion(遮蔽)を入出力

PBR表現としては、metallic-roughnessマテリアルモデルとしてデータを格納しています。  

## 制限事項

* ボーンやスキンの入出力には対応していません (ver.1.0では対応予定)。  
* アニメーション情報の入出力には対応していません。  
* カメラ情報の入出力には対応していません。  

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

This software is released under the MIT License, see [LICENSE.txt](./LICENSE).  

## 更新履歴

