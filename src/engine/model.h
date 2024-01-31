// Model class for loading models from file and custom models
// This implementatin is based on the tutorial from learnopengl.com
// https://learnopengl.com/Model-Loading/Model
// custom model loading is WIP 

// todo: write down implementation of this class into .cpp file

#pragma once
#include "mesh.h"
#include "util/filesystem.h"
#include "util/log.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <string>

unsigned int TextureFromFile(const char* path, const std::string& directory);

class Model
{

public:
    std::vector<Mesh> meshes;
    std::vector<Texture> textures_loaded;

public:
    Model() = default;
    // model constructor for pre-defined models
    Model(const std::string path){
        loadModel(path);
    };
    // model constructor for custom model if normal mapping/indices and textures are not required
    Model(std::vector<float> &v){
        loadModel(v);
    };
    // model constructor for custom models if normal mapping is not required
    Model(std::vector<float> verticies, std::vector<unsigned int> 
    indices, std::vector<std::pair<std::string,std::string>> textures = {}){
        
        loadModel(verticies, indices, textures);
    };
    // model constructor for custom model if normal mapping is required
    Model(std::vector<float> verticies, std::vector<unsigned int> 
    indices, std::vector<std::pair<std::string,std::string>> textures,  std::vector<float> normals){
        // loadModel(vertices, indices, textures, normals);
    };
    ~Model() = default;
    
    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
private:
    std::string directory;
    // ----------------------------------------------------------------
    // this will load custom models without normal mapping and indices
    // ----------------------------------------------------------------
    void loadModel(std::vector<float> &v){
        std::vector<Vertex> verticies;
        
        for (int i = 0; i < v.size(); i += 3)
        {
            Vertex vertex;
            vertex.Position = glm::vec3(v[i], v[i + 1], v[i + 2]);
            verticies.push_back(vertex);
        }
        // FIXME : this is a temporary solution  
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        Mesh myMesh(verticies, indices, textures);
        meshes.push_back(myMesh);
    }

    // ----------------------------------------------------------------------
    // this will load custom models with indices/texture but no normal mapping
    // note : incoming texture files is structured like this :  {{"texture_diffuse.jpg","diffuse_texture"},{"texture_normal.jpg","normal_texture"}}
    // ----------------------------------------------------------------------
    void loadModel(std::vector<float>& vert, std::vector<unsigned int>& ind, std::vector<std::pair<std::string,std::string>>& tex){
        std::vector<Vertex> vertices;
        std::vector<Texture> textures;

        // FIXME:  this is temporary solution
        // note: this will not handle theh different path textures
        directory = FileSystem::getPath("resources/textures");

        // we assume that incoming vertices array contains texcoords
        for (int i = 0; i < vert.size(); i+=5)
        {
            Vertex vertex;
            vertex.Position = glm::vec3(vert[i], vert[i + 1], vert[i + 2]);
            vertex.TexCoords = glm::vec2(vert[i + 3], vert[i + 4]);
            vertices.push_back(vertex);
        }

        for (int i = 0; i < tex.size(); i++)
        {
            Texture texture;
            texture.id = TextureFromFile("bricks2.jpg", directory);
            texture.path = tex[i].first;
            texture.type = tex[i].second;
            textures.push_back(texture);
        }
        
        for (unsigned int i = 0; i < textures.size(); i++)
            CLog::write(CLog::Debug,"Texture loaded -> %s\n", textures[i].path.c_str());

        Mesh myMesh(vertices, ind, textures, true,false, true);
        
        meshes.push_back(myMesh);
    }
    // ----------------------------------------------------------------------
    // this will load custom models with indices/textures/normal mapping
    // ----------------------------------------------------------------------
    // void loadModel(std::vector<float>&v, std::vector<float>&i, std::vector<float>&t, const std::vector<float>&normals){
    // }

    // ----------------------------------------------------------------
    // loads a model with supported ASSIMP extensions from file
    // ----------------------------------------------------------------
    void loadModel(const std::string& path){
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
            CLog::write(CLog::Fatal,"ERROR::ASSIMP::%s\n", import.GetErrorString());
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
        std::vector<Texture> textures;

        bool hasIndices = false, hasTexCoords = false, hasNormals = false;
        // process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
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
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material,aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(),diffuseMaps.begin(),diffuseMaps.end());
            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            std::vector<Texture> heightMap = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
            textures.insert(textures.end(), heightMap.begin(), heightMap.end());    
        }
        Mesh lastMesh(vertices, indices, textures, true, hasNormals, hasTexCoords);
        return lastMesh;
    }
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName){
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
                CLog::write(CLog::Debug,"Texture loaded -> %s\n", textures[i].path.c_str());
            }
        }
        return textures;
    }

};

inline unsigned int TextureFromFile(const char* path, const std::string& directory){
    std::string filename = std::string(path);
    filename = directory + '/' + filename; // complete path to image
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if(nrComponents == 3)
            format = GL_RGB;
        else if(nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        CLog::write(CLog::Fatal, "Texture failed to load at path : %s\n",path);
        stbi_image_free(data);
    }

    return textureID;
}