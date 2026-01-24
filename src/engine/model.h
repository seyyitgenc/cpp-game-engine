// Model class for loading models from file and custom models
// This implementatin is based on the tutorial from learnopengl.com
// https://learnopengl.com/Model-Loading/Model
// custom model loading is WIP

#pragma once
#include "external/stb_image.h"
#include "mesh.h"

#include "util/filesystem.hpp"
#include "util/log.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>

class Model {
public:
    // model constructor for pre-defined models
    Model(const std::string& path)
    {
        loadModel(path);
    };

    // draws the model, and thus all its meshes
    void Draw(const Shader& shader) const
    {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader);
        }
    }

private:
    std::vector<Mesh> meshes;

private:
    std::string directory;

    // ----------------------------------------------------------------
    // loads a model with supported ASSIMP extensions from file
    // ----------------------------------------------------------------
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::string> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType myType);
};