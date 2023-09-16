#include "artd/CachedMeshLoader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>


ARTD_BEGIN

namespace fs = std::filesystem;

static const char *pyramidTxt = R"(

[points]
# We add normal vector (nx, ny, nz)
# x    y    z       nx   ny  nz     r   g   b

# The base
-0.5 -0.5 -0.3     0.0 -1.0 0.0    1.0 1.0 1.0
+0.5 -0.5 -0.3     0.0 -1.0 0.0    1.0 1.0 1.0
+0.5 +0.5 -0.3     0.0 -1.0 0.0    1.0 1.0 1.0
-0.5 +0.5 -0.3     0.0 -1.0 0.0    1.0 1.0 1.0

# Face sides have their own copy of the vertices
# because they have a different normal vector.
-0.5 -0.5 -0.3  0.0 -0.848 0.53    1.0 1.0 1.0
+0.5 -0.5 -0.3  0.0 -0.848 0.53    1.0 1.0 1.0
+0.0 +0.0 +0.5  0.0 -0.848 0.53    1.0 1.0 1.0

+0.5 -0.5 -0.3   0.848 0.0 0.53    1.0 1.0 1.0
+0.5 +0.5 -0.3   0.848 0.0 0.53    1.0 1.0 1.0
+0.0 +0.0 +0.5   0.848 0.0 0.53    1.0 1.0 1.0

+0.5 +0.5 -0.3   0.0 0.848 0.53    1.0 1.0 1.0
-0.5 +0.5 -0.3   0.0 0.848 0.53    1.0 1.0 1.0
+0.0 +0.0 +0.5   0.0 0.848 0.53    1.0 1.0 1.0

-0.5 +0.5 -0.3  -0.848 0.0 0.53    1.0 1.0 1.0
-0.5 -0.5 -0.3  -0.848 0.0 0.53    1.0 1.0 1.0
+0.0 +0.0 +0.5  -0.848 0.0 0.53    1.0 1.0 1.0

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


bool
CachedMeshLoader::loadGeometry(const fs::path& path, std::vector<float>& pointData,
                               std::vector<uint16_t>& indexData, int dimensions)
{
    
    
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
