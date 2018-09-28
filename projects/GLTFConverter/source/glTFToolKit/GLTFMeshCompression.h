/**
 * MeshのDraco圧縮.
 * https://github.com/Microsoft/glTF-Toolkit の GLTFMeshCompressionUtils より.
 * gltf/glbのどちらでもDraco圧縮できるように機能追加.
 */
#ifndef _GLTFMESHCOMPRESSION_H
#define _GLTFMESHCOMPRESSION_H

#include "GLTFSDK.h"
#include "GLTFSDK/BufferBuilder.h"

namespace glTFToolKit {
    /// <summary>Draco compression options.</summary>
    struct CompressionOptions
    {
        int PositionQuantizationBits = 14;
        int TexCoordQuantizationBits = 12;
        int NormalQuantizationBits = 10;
        int ColorQuantizationBits = 8;
        int GenericQuantizationBits = 12;
        int Speed = 3;
    };

    /// <summary>
    /// Utilities to compress textures in a glTF asset.
    /// </summary>
    class GLTFMeshCompressionUtils
    {
    public:
        /// <summary>
        /// Applies <see cref="CompressMesh" /> to every mesh in the document, following the same parameter structure as that function.
        /// </summary>
        /// <param name="streamReader">A stream reader that is capable of accessing the resources used in the glTF asset by URI.</param>
        /// <param name="doc">The document from which the mesh will be loaded.</param>
        /// <param name="options">The compression options that will be used.</param>
        /// <param name="outputDirectory">The output directory to which compressed data should be saved.</param>
        /// <returns>
        /// A new glTF manifest that uses the KHR_draco_mesh_compression extension to point to the compressed meshes.
        /// </returns>
        static Microsoft::glTF::Document CompressMeshes (
            std::shared_ptr<Microsoft::glTF::IStreamReader> streamReader,
            const Microsoft::glTF::Document & doc,
            CompressionOptions options,
            const std::string& outputDirectory,
			const bool isGLB,
			std::shared_ptr<Microsoft::glTF::BufferBuilder>& bufferBuilder);

        /// <summary>
        /// Applies Draco mesh compression to the supplied mesh and creates a new set of vertex buffers for all the primitive attributes.
        /// </summary>
        /// <param name="streamReader">A stream reader that is capable of accessing the resources used in the glTF asset by URI.</param>
        /// <param name="doc">The document from which the mesh will be loaded.</param>
        /// <param name="mesh">The mesh which the mesh will be compressed.</param>
        /// <param name="options">The compression options that will be used.</param>
        /// <param name="builder">The output buffer builder that handles bufferId generation for the return document.</param>
        /// <param name="bufferViewsToRemove">Out parameter of BufferView Ids that are no longer in use and should be removed.</param>
        /// <returns>
        /// A new glTF manifest that uses the KHR_draco_mesh_compression extension to point to the compressed meshes.
        /// </returns>
        static Microsoft::glTF::Document CompressMesh (
            std::shared_ptr<Microsoft::glTF::IStreamReader> streamReader,
            const Microsoft::glTF::Document & doc,
            CompressionOptions options,
            const Microsoft::glTF::Mesh & mesh,
			Microsoft::glTF::BufferBuilder* builder,
            std::unordered_set<std::string>& bufferViewsToRemove);
    };
}

#endif
