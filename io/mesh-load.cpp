#include "mesh-load.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <sstream>
#include <filesystem>
#include "asset-paths.h"
const aiScene* LoadScene(Assimp::Importer& importer, const std::string& path) {
    const aiScene* scene = importer.ReadFile(path.c_str(),
        aiProcess_Triangulate |
        aiProcess_FlipWindingOrder |
        aiProcess_JoinIdenticalVertices);
    const char* err = importer.GetErrorString();
    if (!scene) {
        throw std::runtime_error(importer.GetErrorString());
    }
    else {
        return scene;
    }
}
glm::vec3 aiVecToGlmVec(aiVector3D v) {
    return glm::vec3(v.x, v.y, v.z);
}

glm::vec4 aiVecToGlmVec(aiColor4D v) {
    return glm::vec4(v.r, v.g, v.b, v.a);
}

namespace io {
    std::vector<std::shared_ptr<MeshData>> LoadMeshes(const std::string& file)
    {
        Assimp::Importer importer;
        const std::string path = CalculatePathForAsset(file);
        const aiScene* scene = LoadScene(importer, path);
        std::vector<std::shared_ptr<MeshData>> result(scene->mNumMeshes);
        for (uint32_t m = 0; m < scene->mNumMeshes; m++) 
        {
            auto currMesh = scene->mMeshes[m];
            std::shared_ptr<MeshData> md = std::make_shared<MeshData>();
            md->name = std::string(currMesh->mName.C_Str());
            md->normals.resize(currMesh->mNumVertices);
            md->vertices.resize(currMesh->mNumVertices);
            md->uv0s.resize(currMesh->mNumVertices);
            for (uint32_t i = 0; i < currMesh->mNumVertices; i++) {
                md->vertices[i] = aiVecToGlmVec(currMesh->mVertices[i]);
                md->normals[i] = aiVecToGlmVec(currMesh->mNormals[i]);
                md->uv0s[i] = aiVecToGlmVec(currMesh->mTextureCoords[0][i]);
            }
            std::vector<uint16_t> indexData;
            for (unsigned int j = 0; j < currMesh->mNumFaces; j++) {
                aiFace face = currMesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indexData.push_back(face.mIndices[k]);
                }
            }
            md->indices = indexData;
            assert(md->indices.size() > 0);
            assert(md->vertices.size() > 0);
            result[m] = md;
        }
        return result;
    }
}
