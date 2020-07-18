# ビルド方法 (開発者向け)

glTF Converter for Shade3Dのビルド方法を記載します。    

Shade3DプラグインSDKをダウンロードします。  
Shade3D_GLTFConverter/projects/GLTFConverterフォルダをShade3DのプラグインSDKのprojectsフォルダに複製します。  
Microsoft glTF SDKは事前にビルドしておく必要があります。    
Google dracoは事前にビルドしておく必要があります。    

## WindowsでのMicrosoft glTF SDKのビルド

GLTFSDK.slnをVS2017で開き、ビルドします。    
「Built/Out/v141/x64/Release/GLTFSDK」に静的ライブラリ「GLTFSDK.lib」が生成されます。    
この静的ライブラリと、
「GLTFSDK/Inc」の下の「GLTFSDK」ディレクトリとその中のファイル、    
「packages/rapidjson.temprelease.0.0.2.20/build/native/include」の下の「rapidjson」ディレクトリとその中のファイル、    
を使用します。    

## MacでのMicrosoft glTF SDKのビルド

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

## dracoのソース修正

Draco 1.3.4で、    
glTFでのDraco圧縮 + Morph Targets + 1Meshの複数Primitiveの頂点情報を共有した場合、にdecode(展開/インポート)時にエラーが起きるパターンがありました。    
以下の修正を行ってます。    

src/draco/compression/mesh/mesh_sequential_decoder.cc    
MeshSequentialDecoder::DecodeConnectivity関数内。    

    // if (points_64 > faces_64 * 3) return false;    

のようにコメントアウトしました。

## dracoのビルド

Win/Macともに同じで、CMakeを使用してプロジェクトを作成し、ビルドします。    
WindowsはVS 2017/MacはXcode 10 のプロジェクトを作成して確認しています。    

Windowsは「draco.lib」「dracodec.lib」「dracoenc.lib」の静的ライブラリが生成されます。   
Macは「libdraco.a」「libdracodec.a」「libdracoenc.a」の静的ライブラリが生成されます。   

また、「/src」内のファイル、ビルドディレクトリの「draco/draco_features.h」をincludeとして参照します。

## Windows
```c
  [GLTFConverter]
    [draco]              draco関連のファイル
      [lib]
        [debug]
          draco.lib
          draco.pdb
          dracodec.lib
          dracodec.pdb
          dracoenc.lib
          dracoenc.pdb
        [release]
          draco.lib
          dracodec.lib
          dracoenc.lib
      [src]
        [draco]           dracoで使用するヘッダファイル類
           [animation]
           [attributes]
           ...
           draco_features.h

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
[draco]ディレクトリ内が、draco圧縮で必要な関連ファイルです。    
GLTFConverter/win_vs2017/GLTFConverter.sln をVS2017で開き、ビルドします。  

## Mac
```c
  [GLTFConverter]  
    [draco]              draco関連のファイル
      [lib]
        [debug]
          libdraco.a
          libdracodec.a
          libdracoenc.a
        [release]
          libdraco.a
          libdracodec.a
          libdracoenc.a
      [src]
        [draco]           dracoで使用するヘッダファイル類
           [animation]
           [attributes]
           ...
           draco_features.h

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
[draco]ディレクトリ内が、draco圧縮で必要な関連ファイルです。    
GLTFConverter/mac/plugins/Template.xcodeproj をXcodeで開き、ビルドします。  

## 使用しているモジュール (開発者向け)

* Shade3D プラグイン SDK v.15.1.0 ( https://github.com/shadedev/pluginsdk ) 
* Microsoft glTF SDK r1.6.3.1 ( https://github.com/Microsoft/glTF-SDK )
* rapidjson v.0.0.2.20 ( https://github.com/Tencent/rapidjson/ )
* Google draco v.1.3.4 ( https://github.com/google/draco )

rapidjsonは、Microsoft glTF SDK内で使用されています。   

