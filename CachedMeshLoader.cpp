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

static const char *pyramidTxt = R"(

[points]
# We add normal vector (nx, ny, nz)
# x    y    z       nx   ny  nz     r   g   b

# The base
-0.5 -0.5 0.3     0.0 -1.0 0.0    1.0 1.0 1.0
+0.5 -0.5 0.3     0.0 -1.0 0.0    1.0 1.0 1.0
+0.5 +0.5 0.3     0.0 -1.0 0.0    1.0 1.0 1.0
-0.5 +0.5 0.3     0.0 -1.0 0.0    1.0 1.0 1.0

# Face sides have their own copy of the vertices
# because they have a different normal vector.
-0.5 -0.5 0.3  0.0 -0.848 0.53    1.0 1.0 1.0
+0.5 -0.5 0.3  0.0 -0.848 0.53    1.0 1.0 1.0
+0.0 +0.0 -0.5  0.0 -0.848 0.53    1.0 1.0 1.0

+0.5 -0.5 0.3   0.848 0.0 0.53    1.0 1.0 1.0
+0.5 +0.5 0.3   0.848 0.0 0.53    1.0 1.0 1.0
+0.0 +0.0 -0.5   0.848 0.0 0.53    1.0 1.0 1.0

+0.5 +0.5 0.3   0.0 0.848 0.53    1.0 1.0 1.0
-0.5 +0.5 0.3   0.0 0.848 0.53    1.0 1.0 1.0
+0.0 +0.0 -0.5   0.0 0.848 0.53    1.0 1.0 1.0

-0.5 +0.5 0.3  -0.848 0.0 0.53    1.0 1.0 1.0
-0.5 -0.5 0.3  -0.848 0.0 0.53    1.0 1.0 1.0
+0.0 +0.0 -0.5  -0.848 0.0 0.53    1.0 1.0 1.0


[indices]
# Base
 0  1  2
 0  2  3
# Sides
 4  5  6
 7  8  9
10 11 12
13 14 15

)";

//[indices]
//# Base
// 2  1  0
// 3  2  0
//# Sides
// 6  5  4
// 9  8  7
//12 11 10
//15 14 13





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
                               std::vector<uint16_t>& indexData, int dimensions)
{
    
    
    if(path == "cone") {
        generateConeMesh(4, pointData,
                         indexData, 1.0f, .5f);
        return(true);
    }

    std::istringstream isstr;
    std::istream *istr;
    

    if(path == "pyramid.txt") {
        isstr = std::istringstream(pyramidTxt);
        istr = &isstr;
    } else {
        return(false);
//        if (!file.is_open()) {
//
//
//            return(false);
//        }
    }

    std::istream &file = *istr;
    
	pointData.clear();
	indexData.clear();

	enum class Section {
		None,
		Points,
		Indices,
	};
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (!file.eof()) {
		getline(file, line);
		if (line == "[points]") {
			currentSection = Section::Points;
		}
		else if (line == "[indices]") {
			currentSection = Section::Indices;
		}
		else if (line[0] == '#' || line.empty()) {
			// Do nothing, this is a comment
		}
		else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			// Get x, y, r, g, b
			for (int i = 0; i < dimensions + 3; ++i) {
				iss >> value;
				pointData.push_back(value);
			}
		}
		else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			// Get corners #0 #1 and #2
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indexData.push_back(index);
			}
		}
	}
	return true;
}




ARTD_END
