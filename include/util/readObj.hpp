#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

struct ObjModel
{
    // Raw per-vertex data from the file
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    // Indices into those arrays
    std::vector<uint32_t> positionIndices;
    std::vector<uint32_t> texCoordIndices;
    std::vector<uint32_t> normalIndices;

    void merge(const ObjModel &other)
    {
        // Append positions, texCoords, normals
        positions.insert(positions.end(), other.positions.begin(), other.positions.end());
        texCoords.insert(texCoords.end(), other.texCoords.begin(), other.texCoords.end());
        normals.insert(normals.end(), other.normals.begin(), other.normals.end());

        // Adjust indices to point to the new positions
        uint32_t offset = static_cast<uint32_t>(positions.size() - other.positions.size());
        for (auto &index : other.positionIndices) {
            positionIndices.push_back(index + offset);
        }
        for (auto &index : other.texCoordIndices) {
            texCoordIndices.push_back(index + offset);
        }
        for (auto &index : other.normalIndices) {
            normalIndices.push_back(index + offset);
        }
    }
};

static void splitFaceToken(const std::string &token,
                           int &outV, int &outVT, int &outVN)
{
    // Initialize to “not present”
    outV = outVT = outVN = -1;

    // find first slash
    size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos) {
        // just "v"
        outV = std::stoi(token) - 1;
        return;
    }

    // parse v
    outV = std::stoi(token.substr(0, firstSlash)) - 1;

    // find second slash
    size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos) {
        // only "v/vt"
        outVT = std::stoi(token.substr(firstSlash + 1)) - 1;
        return;
    }

    // we have "v/vt/vn" or "v//vn"
    if (secondSlash == firstSlash + 1) {
        // "v//vn" => no vt
        outVT = -1;
    } else {
        outVT = std::stoi(token.substr(firstSlash + 1,
                                      secondSlash - firstSlash - 1)) - 1;
    }

    // vn is after second slash
    outVN = std::stoi(token.substr(secondSlash + 1)) - 1;
}

ObjModel readObj(const std::string& filename)
{
    ObjModel model;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file: " + filename);
    }
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        if (prefix == "v") {
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            model.positions.push_back(position);
        } else if (prefix == "f") {
            uint32_t v1, v2, v3;
            iss >> v1 >> v2 >> v3;
            model.positionIndices.push_back(v1 - 1); // OBJ indices are 1-based
            model.positionIndices.push_back(v2 - 1);
            model.positionIndices.push_back(v3 - 1);
        }
    }
    file.close();
    if (model.positions.empty() || model.positionIndices.empty()) {
        throw std::runtime_error("No valid vertices or indices found in OBJ file: " + filename);
    }
    std::cout << "Read " << model.positions.size() << " vertices and " << model.positionIndices.size() / 3 << " faces from OBJ file: " << filename << std::endl;
    return model;
}

ObjModel readObj2(const std::string& filename)
{
    ObjModel model;
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Failed to open OBJ file: " + filename);

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            model.positions.push_back(pos);

        } else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            model.texCoords.push_back(uv);

        } else if (prefix == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            model.normals.push_back(n);

        } else if (prefix == "f") {
            // Read three tokens (triangles only)
            std::string tok[3];
            iss >> tok[0] >> tok[1] >> tok[2];

            for (int i = 0; i < 3; ++i) {
                int v, vt, vn;
                splitFaceToken(tok[i], v, vt, vn);

                if (v  < 0 || v  >= (int)model.positions.size())  throw std::runtime_error("Invalid vertex index");
                model.positionIndices.push_back((uint32_t)v);

                if (vt >= 0) {
                    if (vt >= (int)model.texCoords.size()) throw std::runtime_error("Invalid texcoord index");
                    model.texCoordIndices.push_back((uint32_t)vt);
                }

                if (vn >= 0) {
                    if (vn >= (int)model.normals.size())   throw std::runtime_error("Invalid normal index");
                    model.normalIndices.push_back((uint32_t)vn);
                }
            }
        }
        // else: ignore other prefixes
    }

    file.close();

    if (model.positions.empty() || model.positionIndices.empty())
        throw std::runtime_error("No valid geometry in OBJ file: " + filename);

    std::cout 
      << "Loaded OBJ: " 
      << model.positions.size() << " positions, " 
      << model.texCoords.size()  << " uvs, " 
      << model.normals.size()    << " normals\n"
      << "  Triangles: " 
      << (model.positionIndices.size()/3) << "\n";

    return model;
}