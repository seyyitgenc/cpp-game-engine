// Model class for loading models from file and custom models
// This implementatin is based on the tutorial from learnopengl.com
// https://learnopengl.com/Model-Loading/Model
// custom model loading is WIP 

// todo: write down implementation of this class into .cpp file

#pragma once
#include "mesh.h"
#include "util/filesystem.hpp"
#include "util/log.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <string>

class Model
{
public:
    // model constructor for pre-defined models
    Model(const std::string path){
        loadModel(path);
    };
    // draws the model, and thus all its meshes
    void Draw(const Shader &shader) const
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
private:
    std::vector<Mesh> meshes;

private:
    std::string directory;

    // ----------------------------------------------------------------
    // loads a model with supported ASSIMP extensions from file
    // ----------------------------------------------------------------
    void loadModel(const std::string& path){
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
            Log::write(
                Log::Fatal,
                LIGHT_RED_TEXT("FATAL::MODEL::ASSIMP::LOAD_MODAL "),
                YELLOW_TEXT(import.GetErrorString()), "\n");
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }
    void processNode(aiNode* node, const aiScene* scene){
        // process each mesh located at the current node
        for (unsigned int  i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh,scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }
    Mesh processMesh(aiMesh* mesh, const aiScene* scene){
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<std::string> texture_names;

        bool hasTexCoords = false, hasNormals = false;
        // process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex{};
            glm::vec3 vector;
            if (mesh->HasPositions())
            {
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;
            }
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
                hasNormals = true;
            }
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                hasTexCoords = true;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            vertices.push_back(vertex); 
        }

        // process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
            
        // process material
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            std::vector<std::string> diffuseMaps = loadMaterialTextures(material,aiTextureType_DIFFUSE, DIFFUSE);
            texture_names.insert(texture_names.end(),diffuseMaps.begin(),diffuseMaps.end());
            std::vector<std::string> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, SPECULAR);
            texture_names.insert(texture_names.end(), specularMaps.begin(), specularMaps.end());
            std::vector<std::string> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, NORMAL);
            texture_names.insert(texture_names.end(), normalMaps.begin(), normalMaps.end());
            std::vector<std::string> heightMap = loadMaterialTextures(material, aiTextureType_AMBIENT, HEIGHT);
            texture_names.insert(texture_names.end(), heightMap.begin(), heightMap.end());    
        }
        Mesh lastMesh(vertices, indices, texture_names, true, hasNormals, hasTexCoords);
        return lastMesh;
    }
    std::vector<std::string> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType myType){
        // gTextureManager->addTexture()
        std::vector<std::string> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++){
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            std::string path;
            path = directory + '/' + str.C_Str(); // complete path to image
            gTextureManager->addTexture(str.C_Str(), path, myType);
            textures.push_back(str.C_Str());
        }
        return textures;
    }
};