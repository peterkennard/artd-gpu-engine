#include "artd/CachedMeshLoader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "artd/vecmath.h"

#include <glm/glm.hpp> // all types inspired from GLSL
#include <glm/ext.hpp>


ARTD_BEGIN

namespace fs = std::filesystem;

// just for now
struct VertexData {
    Vec3f pos;
    Vec3f normal;
    Vec3f color;
};

#pragma pack(2)
struct TriangleIndices {
	uint16_t indices[3];
};
#pragma pack(0)


static void generateSimpleCubeMesh(std::vector<float>& pointData,
                             std::vector<uint16_t>& indexData, float extent)
{
    static const int vDataFloats = 8;

	static const float vertData[] = {

	// position				normal					u,v

	// bottom
	-1.0f,-1.0f,-1.0f,		0.0, -1.0f, 0.0f,		0.0f, 0.0f,
     1.0f,-1.0f,-1.0f,		0.0, -1.0f, 0.0f,		1.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,		0.0, -1.0f, 0.0f,		0.0f, 1.0f,
     1.0f,-1.0f,-1.0f,		0.0, -1.0f, 0.0f,		1.0f, 0.0f,
     1.0f,-1.0f, 1.0f,		0.0, -1.0f, 0.0f,		1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,		0.0, -1.0f, 0.0f,		0.0f, 1.0f,

    // top
    -1.0f, 1.0f,-1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 1.0f,
     1.0f, 1.0f,-1.0f,		0.0f, 1.0f, 0.0f,		1.0f, 0.0f,
     1.0f, 1.0f,-1.0f,		0.0f, 1.0f, 0.0f,		1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		0.0f, 1.0f,
     1.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,

    // front
    -1.0f,-1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		1.0f, 0.0f,
     1.0f,-1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 0.0f,
    -1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 0.0f,
     1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		0.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,		0.0f, 0.0f, 1.0f,		1.0f, 1.0f,

    // back
    -1.0f,-1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 1.0f,
     1.0f,-1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		1.0f, 0.0f,
     1.0f,-1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		1.0f, 0.0f,
    -1.0f, 1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		0.0f, 1.0f,
     1.0f, 1.0f,-1.0f,		0.0f, 0.0f, -1.0f,		1.0f, 1.0f,

    // left
    -1.0f,-1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,		-1.0f, 0.0f, 0.0f,		1.0f, 0.0f,
    -1.0f,-1.0f,-1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 0.0f,
    -1.0f,-1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		0.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,		1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,		-1.0f, 0.0f, 0.0f,		1.0f, 0.0f,

    // right
     1.0f,-1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,		1.0f, 0.0f, 0.0f,		1.0f, 0.0f,
     1.0f, 1.0f,-1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,
     1.0f,-1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 0.0f,
     1.0f, 1.0f, 1.0f,		1.0f, 0.0f, 0.0f,		0.0f, 1.0f

	};

	int vertexCount = 36 * 8;
	int indexCount = 36 * 3;

    // TODO: load directly into a gpu buffers ??
    pointData.resize(sizeof(VertexData) * vertexCount);
    VertexData *pVertex =  reinterpret_cast<VertexData *>(pointData.data());

    indexData.resize(indexCount * sizeof(TriangleIndices));
    TriangleIndices *pIndex = reinterpret_cast<TriangleIndices*>(indexData.data());

	extent *= .5f; //1.0 extent == 1 * 1 * 1 cube

    const float *pData = &vertData[0];

	for (int i = 0; i < vertexCount; i += 8) {

		pVertex->pos[0] = pData[0] * extent;
		pVertex->pos[1] = pData[1] * extent;
		pVertex->pos[2] = pData[2] * extent;

		pVertex->normal[0] = vertData[3];
		pVertex->normal[1] = vertData[4];
		pVertex->normal[2] = vertData[5];

        // tex coords
		// pVertex->normal[] = vertData[6];
		// pVertex->normal[2] = vertData[7];

        pData += vDataFloats;
		++pVertex;
	}

	for (int index = 0; index < indexCount; index += 3) {

		pIndex->indices[0] = index;
		pIndex->indices[1] = index + 1;
		pIndex->indices[2] = index + 2;
		++pIndex;
	}
    return;
}

static void generateConeMesh(uint32_t numSegments, std::vector<float>& pointData,
                             std::vector<uint16_t>& indexData, float height, float radius)
{
    //CLEVER?

    pointData.clear();
    indexData.clear();

	uint32_t vertexCount = numSegments * 3 + 1; // top and bottom vertices for sides, bottom and center vertices for bottom cap
	uint32_t indexCount = 2 * numSegments; // triangles for sides and triangles for bottom cap

    // TODO: load directly into a gpu buffers ??
    pointData.resize(sizeof(VertexData) * vertexCount);
    VertexData *pVertex =  reinterpret_cast<VertexData *>(pointData.data());

    indexData.resize(indexCount * sizeof(TriangleIndices));
    TriangleIndices *pIndex = reinterpret_cast<TriangleIndices*>(indexData.data());

	float segAngle = (2.0 * glm::pi<float>()) / numSegments;

	uint32_t triIndex = 0;
	const uint32_t centerIndex = 0;

	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
	pVertex->pos = center;

	glm::vec3 vNormalCap = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

	pVertex->normal = vNormalCap;

	++triIndex;
	++pVertex;

	glm::vec3 top;
    // the top is always in the same place but duplicated as every face has a different normal
	top.z = 0.0f;
	top.x = 0.0f;
	top.y = height;

    glm::vec3 bottom = glm::vec3(radius,0,0);

    // rotation matrix a 3x3 for one segment delta rotation.
    glm::mat3 segRotation(glm::rotate(glm::mat4(1.f), segAngle, glm::vec3(0.f,-1.f,0.f)));

	// bottom cap
	for (uint32_t face = 0; face < numSegments; ++face) {

		pVertex->pos = bottom;
		pVertex->normal = -vNormalCap;  // seems wrong - should it not be down ?? y = -1
        // advance to next segment
		bottom = segRotation * bottom;

//		pVertex->textCoord[0] = (float)face / (float)numSegments;
//		pVertex->textCoord[1] = 1.0f;

		++triIndex;
		++pVertex;

		pIndex->indices[0] = centerIndex;
		pIndex->indices[1] = (face == numSegments - 1) ? 1 : triIndex;
		pIndex->indices[2] = triIndex - 1;
		++pIndex;
	}

	// https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
    glm::vec3 vToTop = glm::vec3(-radius, height, 0);
    glm::vec3 vToNext = glm::vec3(0.f,0.f,1.f);
    glm::vec3 edgeNorm = glm::cross(vToTop,vToNext);
    edgeNorm = glm::normalize(edgeNorm);
    bottom = glm::vec3(radius,0.f,0.f);
    // rotate about Y by segAngle/2 as a 3x3 matrix
    // that is normal at top is half way rotated between normals at bottom.
    glm::vec3 topNorm = glm::mat3(glm::rotate(glm::mat4(1.f), segAngle/2.0f, glm::vec3(0.f,-1.f,0.f))) * edgeNorm;

    for (uint32_t face = 0; face < numSegments; ++face) {

		pVertex->pos = bottom;
        pVertex->normal = edgeNorm;

        edgeNorm = segRotation * edgeNorm;  // rotate normal around Y by segAngle
        bottom = segRotation * bottom;  // rotate base position

//		pVertex->texCoord[0] = (float)face / (float)numSegments;
//		pVertex->texCoord[1] = 1.0f;

		++triIndex;
		++pVertex;

		// Add top vertex
		pVertex->pos = top;
		pVertex->normal = topNorm;
        topNorm = segRotation * topNorm;  // rotate top normal around Y by segAngle

//		pVertex->texCoord[0] = (float)face / (float)numSegments;
//		pVertex->texCoord[1] = 0.0f;

		++triIndex;
		++pVertex;

		// outside
		pIndex->indices[0] = triIndex - 2; // current bottom
		pIndex->indices[1] = (face == numSegments - 1) ? (numSegments + 1) : triIndex; // next bottom
		pIndex->indices[2] = triIndex - 1; // top
		++pIndex;
	}
}




bool
CachedMeshLoader::loadGeometry(const fs::path& path, std::vector<float>& pointData,
                               std::vector<uint16_t>& indexData, int /* dimensions */)
{

    if(path == "cone") {
        generateConeMesh( 12, pointData,
                          indexData, 1.0f, .5f);
        return(true);
    }
    if(path == "cube") {
        generateSimpleCubeMesh( pointData,
                                indexData, 1.0f);
        return(true);
    }

//    std::istringstream isstr;
//    std::istream *istr;
    

//    } else {
        return(false);
//        if (!file.is_open()) {
//
//
//            return(false);
//        }
//    }
//
//    std::istream &file = *istr;
//
//	pointData.clear();
//	indexData.clear();
//
//	enum class Section {
//		None,
//		Points,
//		Indices,
//	};
//	Section currentSection = Section::None;
//
//	float value;
//	uint16_t index;
//	std::string line;
//	while (!file.eof()) {
//		getline(file, line);
//		if (line == "[points]") {
//			currentSection = Section::Points;
//		}
//		else if (line == "[indices]") {
//			currentSection = Section::Indices;
//		}
//		else if (line[0] == '#' || line.empty()) {
//			// Do nothing, this is a comment
//		}
//		else if (currentSection == Section::Points) {
//			std::istringstream iss(line);
//			// Get x, y, r, g, b
//			for (int i = 0; i < dimensions + 3; ++i) {
//				iss >> value;
//				pointData.push_back(value);
//			}
//		}
//		else if (currentSection == Section::Indices) {
//			std::istringstream iss(line);
//			// Get corners #0 #1 and #2
//			for (int i = 0; i < 3; ++i) {
//				iss >> index;
//				indexData.push_back(index);
//			}
//		}
//	}
//	return true;
}




ARTD_END
