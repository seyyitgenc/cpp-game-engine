#include "model.h"

#include "globals.h"

void Model::loadModel(const std::string& path)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Log::write(
            Log::Fatal,
            LIGHT_RED_TEXT("FATAL::MODEL::ASSIMP::LOAD_MODAL "),
            YELLOW_TEXT(import.GetErrorString()), "\n");
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::string> texture_names;

    bool hasTexCoords = false, hasNormals = false;
    // process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex {};
        glm::vec3 vector;
        if (mesh->HasPositions()) {
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
        }
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
            hasNormals = true;
        }
        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            hasTexCoords = true;
        } else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        vertices.push_back(vertex);
    }

    // process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // process material
    if (mesh->mMaterialIndex > 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<std::string> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::DIFFUSE);
        texture_names.insert(texture_names.end(), diffuseMaps.begin(), diffuseMaps.end());
        std::vector<std::string> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::SPECULAR);
        texture_names.insert(texture_names.end(), specularMaps.begin(), specularMaps.end());
        std::vector<std::string> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, TextureType::NORMAL);
        texture_names.insert(texture_names.end(), normalMaps.begin(), normalMaps.end());
        std::vector<std::string> heightMap = loadMaterialTextures(material, aiTextureType_AMBIENT, TextureType::HEIGHT);
        texture_names.insert(texture_names.end(), heightMap.begin(), heightMap.end());
        std::vector<std::string> reflectionMap = loadMaterialTextures(material, aiTextureType_REFLECTION, TextureType::REFLECTION);
        texture_names.insert(texture_names.end(), reflectionMap.begin(), reflectionMap.end());
        std::vector<std::string> displacementMap = loadMaterialTextures(material, aiTextureType_DISPLACEMENT, TextureType::DISPLACEMENT);
        texture_names.insert(texture_names.end(), displacementMap.begin(), displacementMap.end());
        std::vector<std::string> roughnessMap = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGNESS);
        texture_names.insert(texture_names.end(), roughnessMap.begin(), roughnessMap.end());
    }
    Mesh lastMesh(vertices, indices, texture_names, true, hasNormals, hasTexCoords);
    return lastMesh;
}

std::vector<std::string> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType myType)
{
    std::vector<std::string> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string path;
        path = directory + '/' + str.C_Str(); // complete path to image
        TextureManager::instance()->addTexture(str.C_Str(), path, myType);
        textures.push_back(str.C_Str());
    }
    return textures;
}