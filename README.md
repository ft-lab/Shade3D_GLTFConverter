# Shade3D_GLTFConverter

Shade3DでGLTF(拡張子 gltf/glb)をインポート/エクスポートするプラグインです。  
GLTFの処理で、「Microsoft.glTF.CPP」( https://www.nuget.org/packages/Microsoft.glTF.CPP/ )を使用しています。


## ビルド方法

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
