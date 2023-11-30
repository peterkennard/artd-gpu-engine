#include "./GpuEngineImpl.h"
#include "artd/DrawableMesh.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "artd/vecmath.h"

#include <glm/glm.hpp> // all types inspired from GLSL
#include <glm/ext.hpp>


ARTD_BEGIN

namespace fs = std::filesystem;

typedef GpuVertexAttributes VertexData;

//// just for now
//struct VertexData {
//    Vec3f pos;
//    Vec3f normal;
//    glm::vec2 uv;
////    float pad_;
////    float pad2_;
//};

#pragma pack(2)
struct TriangleIndices {
	uint16_t indices[3];
};
#pragma pack(0)

#if 0
//

// items from https://danielsieger.com/blog/2021/01/03/generating-platonic-solids.html
//
// https://danielsieger.com/download/MyViewer_platonic_solids.cpp
// Copyright 2011-2021 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.
//
// https://www.pmp-library.org/
//
// repo:
// git clone https://github.com/pmp-library/pmp-library.git

void dual(SurfaceMesh& mesh)
{
  // the new dual mesh
  SurfaceMesh tmp;

  // a property to remember new vertices per face
  auto fvertex = mesh.add_face_property<Vertex>("f:vertex");

  // for each face add the centroid to the dual mesh
  for (auto f : mesh.faces())
    fvertex[f] = tmp.add_vertex(centroid(mesh, f));

  // add new face for each vertex
  for (auto v : mesh.vertices()) {
    std::vector<Vertex> vertices;
    for (auto f : mesh.faces(v))
      vertices.push_back(fvertex[f]);

    tmp.add_face(vertices);
  }

  // swap old and new meshes, don't copy properties
  mesh.assign(tmp);
}

Point centroid(const SurfaceMesh& mesh, Face f)
{
  Point c(0, 0, 0);
  Scalar n(0);
  for (auto v : mesh.vertices(f)) {
    c += mesh.position(v);
    ++n;
  }
  c /= n;
  return c;
}

#include "MyViewer.h"

#include <imgui.h>

using namespace pmp;

SurfaceMesh tetrahedron()
{
    SurfaceMesh mesh;

    // choose coordinates on the unit sphere
    float a = 1.0f / 3.0f;
    float b = sqrt(8.0f / 9.0f);
    float c = sqrt(2.0f / 9.0f);
    float d = sqrt(2.0f / 3.0f);

    // add the 4 vertices
    auto v0 = mesh.add_vertex(Point(0, 0, 1));
    auto v1 = mesh.add_vertex(Point(-c, d, -a));
    auto v2 = mesh.add_vertex(Point(-c, -d, -a));
    auto v3 = mesh.add_vertex(Point(b, 0, -a));

    // add the 4 faces
    mesh.add_triangle(v0, v1, v2);
    mesh.add_triangle(v0, v2, v3);
    mesh.add_triangle(v0, v3, v1);
    mesh.add_triangle(v3, v2, v1);

    return mesh;
}

SurfaceMesh hexahedron()
{
    SurfaceMesh mesh;

    // choose coordinates on the unit sphere
    float a = 1.0f / sqrt(3.0f);

    // add the 8 vertices
    auto v0 = mesh.add_vertex(Point(-a, -a, -a));
    auto v1 = mesh.add_vertex(Point(a, -a, -a));
    auto v2 = mesh.add_vertex(Point(a, a, -a));
    auto v3 = mesh.add_vertex(Point(-a, a, -a));
    auto v4 = mesh.add_vertex(Point(-a, -a, a));
    auto v5 = mesh.add_vertex(Point(a, -a, a));
    auto v6 = mesh.add_vertex(Point(a, a, a));
    auto v7 = mesh.add_vertex(Point(-a, a, a));

    // add the 6 faces
    mesh.add_quad(v3, v2, v1, v0);
    mesh.add_quad(v2, v6, v5, v1);
    mesh.add_quad(v5, v6, v7, v4);
    mesh.add_quad(v0, v4, v7, v3);
    mesh.add_quad(v3, v7, v6, v2);
    mesh.add_quad(v1, v5, v4, v0);

    return mesh;
}

Point centroid(const SurfaceMesh& mesh, Face f)
{
    Point c(0, 0, 0);
    Scalar n(0);
    for (auto v : mesh.vertices(f))
    {
        c += mesh.position(v);
        ++n;
    }
    c /= n;
    return c;
}

void dual(SurfaceMesh& mesh)
{
    // the new dual mesh
    SurfaceMesh tmp;

    // a property to remember new vertices per face
    auto fvertex = mesh.add_face_property<Vertex>("f:vertex");

    // for each face add the centroid to the dual mesh
    for (auto f : mesh.faces())
        fvertex[f] = tmp.add_vertex(centroid(mesh, f));

    // add new face for each vertex
    for (auto v : mesh.vertices())
    {
        std::vector<Vertex> vertices;
        for (auto f : mesh.faces(v))
            vertices.push_back(fvertex[f]);

        tmp.add_face(vertices);
    }

    // swap old and new meshes, don't copy properties
    mesh.assign(tmp);
}

SurfaceMesh octahedron()
{
    auto mesh = hexahedron();
    dual(mesh);
    return mesh;
}

void project_to_unit_sphere(SurfaceMesh& mesh)
{
    for (auto v : mesh.vertices())
    {
        auto p = mesh.position(v);
        auto n = norm(p);
        mesh.position(v) = (1.0 / n) * p;
    }
}

SurfaceMesh octahedron2()
{
    auto mesh = hexahedron();
    dual(mesh);
    project_to_unit_sphere(mesh);
    return mesh;
}

SurfaceMesh icosahedron()
{
    SurfaceMesh mesh;

    float phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
    float a = 1.0f;
    float b = 1.0f / phi;

    // add vertices
    auto v1 = mesh.add_vertex(Point(0, b, -a));
    auto v2 = mesh.add_vertex(Point(b, a, 0));
    auto v3 = mesh.add_vertex(Point(-b, a, 0));
    auto v4 = mesh.add_vertex(Point(0, b, a));
    auto v5 = mesh.add_vertex(Point(0, -b, a));
    auto v6 = mesh.add_vertex(Point(-a, 0, b));
    auto v7 = mesh.add_vertex(Point(0, -b, -a));
    auto v8 = mesh.add_vertex(Point(a, 0, -b));
    auto v9 = mesh.add_vertex(Point(a, 0, b));
    auto v10 = mesh.add_vertex(Point(-a, 0, -b));
    auto v11 = mesh.add_vertex(Point(b, -a, 0));
    auto v12 = mesh.add_vertex(Point(-b, -a, 0));

    project_to_unit_sphere(mesh);

    // add triangles
    mesh.add_triangle(v3, v2, v1);
    mesh.add_triangle(v2, v3, v4);
    mesh.add_triangle(v6, v5, v4);
    mesh.add_triangle(v5, v9, v4);
    mesh.add_triangle(v8, v7, v1);
    mesh.add_triangle(v7, v10, v1);
    mesh.add_triangle(v12, v11, v5);
    mesh.add_triangle(v11, v12, v7);
    mesh.add_triangle(v10, v6, v3);
    mesh.add_triangle(v6, v10, v12);
    mesh.add_triangle(v9, v8, v2);
    mesh.add_triangle(v8, v9, v11);
    mesh.add_triangle(v3, v6, v4);
    mesh.add_triangle(v9, v2, v4);
    mesh.add_triangle(v10, v3, v1);
    mesh.add_triangle(v2, v8, v1);
    mesh.add_triangle(v12, v10, v7);
    mesh.add_triangle(v8, v11, v7);
    mesh.add_triangle(v6, v12, v5);
    mesh.add_triangle(v11, v9, v5);

    return mesh;
}

SurfaceMesh dodecahedron()
{
    auto mesh = icosahedron();
    dual(mesh);
    project_to_unit_sphere(mesh);
    return mesh;
}

#endif  // code from sample ...

namespace {

static void generateSimpleCubeMesh(std::vector<float>& pointData,
                             std::vector<uint16_t>& indexData, float extent)
{
    indexData.clear();
    pointData.clear();
    
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
    pointData.resize((sizeof(VertexData) / sizeof(float)) * 36);
    VertexData *pVertex =  reinterpret_cast<VertexData *>(pointData.data());

    indexData.resize(indexCount * sizeof(TriangleIndices));
    TriangleIndices *pIndex = reinterpret_cast<TriangleIndices*>(indexData.data());

	extent *= .5f; //1.0 extent == 1 * 1 * 1 cube

    const float *pData = &vertData[0];
  //  const float *pData = &vertData[6*8];

	for (int i = 0; i < vertexCount; i += 8) {

		pVertex->position.x = pData[0] * extent;
		pVertex->position.y = pData[1] * extent;
		pVertex->position.z = pData[2] * extent;

		pVertex->normal.x = pData[3];
		pVertex->normal.y = pData[4];
		pVertex->normal.z = pData[5];

        // tex coords
		pVertex->uv.x = pData[6];
		pVertex->uv.y = pData[7];

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

class TriangleMeshBuilder {

    VertexData *verts;
    VertexData *currVert;
    int vertCount = 0;

    TriangleIndices *indices;
    int faceCount = 0;
public:

    TriangleMeshBuilder(VertexData *vbuf, TriangleIndices *ti)
        : verts(vbuf)
        , indices(ti)
    {}

    int addVertex(const Vec3f &point)  {
        currVert = &verts[vertCount];
        currVert->position = point;
        int ret = vertCount;
        ++vertCount;
        return(ret);
    }
    
    void setNormal(const Vec3f &normal)  {
        currVert->normal = normal;
    }
    
    void setUV(const glm::vec2 &uv) {
        currVert->uv = uv;
    }
    
    void addTriangle(int i0, int i1, int i2) {
        TriangleIndices &ti = indices[faceCount];
        ++faceCount;
        ti.indices[0] = i0;
        ti.indices[1] = i1;
        ti.indices[2] = i2;
    }

    // This projects all points from origin to the unit (radius) sphere.
    // assuming the current state if a regular polyhedron
    // with it's center at 0,0,0
    
    void projectToUnitSphere(bool setNormals) {
        for(int i = 0; i < vertCount; ++i) {
            Vec3f &v = verts[i].position;
            v = glm::normalize(v);
            if(setNormals) {
                verts[i].normal = v;
            }
        }
    }
};

static void buildIcosahedron(TriangleMeshBuilder &mb) {
    
    float phi = (1.0f + sqrt(5.0f)) * 0.5f; // golden ratio
    float a = 1.0f;
    float b = 1.0f / phi;

    // add vertices
    auto v1 = mb.addVertex(Vec3f(0, b, -a));
    auto v2 = mb.addVertex(Vec3f(b, a, 0));
    auto v3 = mb.addVertex(Vec3f(-b, a, 0));
    auto v4 = mb.addVertex(Vec3f(0, b, a));
    auto v5 = mb.addVertex(Vec3f(0, -b, a));
    auto v6 = mb.addVertex(Vec3f(-a, 0, b));
    auto v7 = mb.addVertex(Vec3f(0, -b, -a));
    auto v8 = mb.addVertex(Vec3f(a, 0, -b));
    auto v9 = mb.addVertex(Vec3f(a, 0, b));
    auto v10 = mb.addVertex(Vec3f(-a, 0, -b));
    auto v11 = mb.addVertex(Vec3f(b, -a, 0));
    auto v12 = mb.addVertex(Vec3f(-b, -a, 0));

    // add triangles
    mb.addTriangle(v3, v2, v1);
    mb.addTriangle(v2, v3, v4);
    mb.addTriangle(v6, v5, v4);
    mb.addTriangle(v5, v9, v4);
    mb.addTriangle(v8, v7, v1);
    mb.addTriangle(v7, v10, v1);
    mb.addTriangle(v12, v11, v5);
    mb.addTriangle(v11, v12, v7);
    mb.addTriangle(v10, v6, v3);
    mb.addTriangle(v6, v10, v12);
    mb.addTriangle(v9, v8, v2);
    mb.addTriangle(v8, v9, v11);
    mb.addTriangle(v3, v6, v4);
    mb.addTriangle(v9, v2, v4);
    mb.addTriangle(v10, v3, v1);
    mb.addTriangle(v2, v8, v1);
    mb.addTriangle(v12, v10, v7);
    mb.addTriangle(v8, v11, v7);
    mb.addTriangle(v6, v12, v5);
    mb.addTriangle(v11, v9, v5);
}

static void generateLatLongSphere(std::vector<float>& pointData,
                                std::vector<uint16_t>& indexData,
                                int numRings, int numSegments)
{
    // TODO: mesh generation needs to be done in a separate thread structure.  Doing it here can result in the generation of multiple copies of the same mesh

    int vertexCount = (numRings + 1) * (numSegments + 1);
    int triCount = (2 * numSegments) + ((numRings - 2) * numSegments * 2) + 1;

    pointData.clear();
    indexData.clear();

    const int floatsPerVert = (int)(sizeof(VertexData) / sizeof(float));

    pointData.resize(vertexCount * floatsPerVert);
    indexData.resize(triCount * 3);

    TriangleMeshBuilder mb(
        reinterpret_cast<VertexData*>(pointData.data())
        , reinterpret_cast<TriangleIndices*>(indexData.data())
    );

    float ringDelta = (glm::pi<float>() / numRings);
    float segmentDelta = (2 * glm::pi<float>() / numSegments);

    // Generate sphere rings ..

    for (int ring = 0; ring <= numRings; ++ring) {

        float r0 = std::sin((float)ring * ringDelta); // Y
        float y0 = std::cos((float)ring * ringDelta); // polar from top down

        // segment group for current ring

        for (int seg = 0; seg <= numSegments; ++seg) {

            float x0 = r0 * std::sin((float)seg * segmentDelta);
            float z0 = r0 * std::cos((float)seg * segmentDelta);

            auto vIndex = mb.addVertex(Vec3f(x0,y0,z0));
            mb.setUV(glm::vec2((float) seg / (float) numRings, (float) ring / (float) numRings));

            if(ring > 0) { // top

                mb.addTriangle(vIndex,
                               vIndex - numSegments,
                               vIndex - (numSegments + 1)
                               );

                if(ring < numRings)  { // bottom or second in quad

                    mb.addTriangle(vIndex,
                                   vIndex - (numSegments + 1),
                                   vIndex - 1
                                   );
                }
            }
        }
    }
    mb.projectToUnitSphere(true);
}

static void generateIcosoSphere(std::vector<float>& pointData,
                                std::vector<uint16_t>& indexData,
                                int // subdivisions = 1
                                )
{
    pointData.clear();
    indexData.clear();

    int vCount = 12;   // 12 vertices
    int triCount = 20; // 20 faces

    // for now only one subdivision but should be scalable.

//    vcount += triCount;
//    triCount *= 3;  // each subdivide creates 3 triangles.

    const int floatsPerVert = (int)(sizeof(VertexData) / sizeof(float));

    pointData.resize(vCount * floatsPerVert);
    indexData.resize(triCount * 3);

    TriangleMeshBuilder mb(
        reinterpret_cast<VertexData*>(pointData.data())
        , reinterpret_cast<TriangleIndices*>(indexData.data())
    );

    buildIcosahedron(mb);
    mb.projectToUnitSphere(true);
}

// https://prideout.net/blog/octasphere/

static void generateOctoSphere(std::vector<float>& pointData,
                                std::vector<uint16_t>& indexData,
                                int // subdivisions = 1
                                )
{
    pointData.clear();
    indexData.clear();

    int vCount = 12;   // 12 vertices
    int triCount = 20; // 20 faces

        // for now only one subdivision but should be scalable.

//    vcount += triCount;
//    triCount *= 3;  // each subdivide creates 3 triangles.

    const int floatsPerVert = (int)(sizeof(VertexData) / sizeof(float));

    pointData.resize(vCount * floatsPerVert);
    indexData.resize(triCount * 3);

    TriangleMeshBuilder mb(
        reinterpret_cast<VertexData*>(pointData.data())
        , reinterpret_cast<TriangleIndices*>(indexData.data())
    );

    buildIcosahedron(mb);
    mb.projectToUnitSphere(true);
}

// This is a tube generator - for a cone use atiny radius for the pointy end.
// TODO: enable optionally adding end caps.

static void generateTubeMesh(uint32_t numSegments, std::vector<float>& pointData,
                                                std::vector<uint16_t>& indexData, float height, float radiusA, float radiusB)
{
    pointData.clear();
    indexData.clear();
    
    int vCount = numSegments * 2;  // + 1 if we have a cone tip
    int triCount = numSegments * 2;

    const int floatsPerVert = (int)(sizeof(VertexData) / sizeof(float));

    pointData.resize(vCount * floatsPerVert);
    indexData.resize((triCount) * 3);

    TriangleMeshBuilder mb(
        reinterpret_cast<VertexData*>(pointData.data())
        , reinterpret_cast<TriangleIndices*>(indexData.data())
    );
    
	// https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
    glm::vec3 vToTop = glm::vec3(-radiusA, height, 0);
    glm::vec3 vToNext = glm::vec3(-radiusB,0.f,1.f);
    glm::vec3 edgeNorm = glm::cross(vToTop,vToNext);
    edgeNorm = glm::normalize(edgeNorm);

    // rotation matrix a 3x3 for one segment delta rotation.
    float segAngle = (2.0 * glm::pi<float>()) / numSegments;
    glm::mat3 segRotation(glm::rotate(glm::mat4(1.f), segAngle, glm::vec3(0.f,1.f,0.f)));

    glm::vec3 sideNorm = edgeNorm;  // rotate normal around Y by segAngle
    glm::vec3 endVert = glm::vec3(radiusA,height,0.);

    for (uint32_t seg = 0; seg < numSegments; ++seg) {

        mb.addVertex(endVert);
        mb.setNormal(sideNorm);

        if(seg < (numSegments-1)) {

            sideNorm = segRotation * sideNorm;  // rotate normal around Y by segAngle
            endVert = segRotation * endVert;  // rotate base position


            mb.addTriangle(seg + 1,
                           seg,
                           seg + numSegments
                           );

            mb.addTriangle(seg + 1,
                           seg + numSegments,
                           seg + numSegments + 1
                           );

           
         } else {
            int nsEnd = numSegments+(numSegments-1);
            mb.addTriangle(0,
                         seg,
                         nsEnd
                         );

            mb.addTriangle(0,
                         nsEnd,
                         numSegments
                         );
         }
    }

    sideNorm = edgeNorm;  // rotate normal around Y by segAngle
    endVert = glm::vec3(radiusB,0.,0.);

    for (uint32_t seg = 0; seg < numSegments; ++seg) {
        
        mb.addVertex(endVert);
        mb.setNormal(sideNorm);
        if(seg < (numSegments-1)) {
            endVert = segRotation * endVert;  // rotate base position
            sideNorm = segRotation * sideNorm;  // rotate normal around Y by segAngle
        }
    }
}


#if 1 // use tube as cone

static void generateConeMesh(uint32_t numSegments, std::vector<float>& pointData,
                                                std::vector<uint16_t>& indexData, float height, float radius)
{
    // not a cone - this is really a tube with a tiny hole in one end and a large end
    // without a bottom cap - this is to so we have decent normals interpolation for s smooth cone

    const float radiusA = height * 1e-8;
    generateTubeMesh(numSegments, pointData,
                                 indexData, height, radiusA, radius);

}

#else

static void generateConeMesh(uint32_t numSegments, std::vector<float>& pointData,
                            std::vector<uint16_t>& indexData, float height, float radius)
{
    //CLEVER?


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

	glm::vec3 bottomNormal = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));

	pVertex->normal = bottomNormal;

	++triIndex;
	++pVertex;

	glm::vec3 top;
    // the top is always in the same place but duplicated as every face has a different normal
	top.z = 0.0f;
	top.x = 0.0f;
	top.y = height;

    glm::vec3 bottom = glm::vec3(radius,0.,0.);

    // rotation matrix a 3x3 for one segment delta rotation.
    glm::mat3 segRotation(glm::rotate(glm::mat4(1.f), segAngle, glm::vec3(0.f,-1.f,0.f)));


	// bottom cap
	for (uint32_t face = 0; face < numSegments; ++face) {

		pVertex->pos = bottom;
		pVertex->normal = bottomNormal;
        // advance to next segment
		bottom = segRotation * bottom;
//		pVertex->textCoord[0] = (float)face / (float)numSegments;
//		pVertex->textCoord[1] = 1.0f;

		++pVertex;

		pIndex->indices[0] = triIndex;
		++triIndex;
		pIndex->indices[1] = (face == numSegments - 1) ? 1 : triIndex;
		pIndex->indices[2] = centerIndex;

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

		// Add top vertex (duplicate for normal !!!)
		pVertex->pos = top;
		pVertex->normal = topNorm;
        topNorm = segRotation * topNorm;  // rotate top normal around Y by segAngle

//		pVertex->texCoord[0] = (float)face / (float)numSegments;
//		pVertex->texCoord[1] = 0.0f;

		++triIndex;
		++pVertex;

		// outside
		pIndex->indices[2] = triIndex - 2; // current bottom
		pIndex->indices[1] = (face == numSegments - 1) ? (numSegments + 1) : triIndex; // next bottom
		pIndex->indices[0] = triIndex - 1; // top
		++pIndex;
	}
}

#endif

 static void generateQuadVertices(std::vector<float>& pointData, std::vector<uint16_t>& indexData) {

    // for now one sided with front face pointing out in Z - Y up X to it's left ( stage right )
    indexData.clear();
    pointData.clear();

	float vertData[] = {

	//       position,             uv
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,    0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,     1.0f, 1.0f,
	};

    const int vDataFloats = 5;
	const int vertexCount = 4;
	const int triangleCount = 2;   // two triangles

	const float *pData = &vertData[0];

    pointData.resize((sizeof(VertexData) / sizeof(float)) * 4);
    VertexData *pVertex =  reinterpret_cast<VertexData *>(pointData.data());

    indexData.resize(triangleCount * sizeof(TriangleIndices)/sizeof(indexData[0]));
    TriangleIndices *pIndex = reinterpret_cast<TriangleIndices*>(indexData.data());

    // todo a generator for rectangles.
    const float xExtent = .5;
    const float yExtent = .5;

    for (int i = 0; i < vertexCount; ++i) {

        pVertex->position.x = pData[0] * xExtent;
        pVertex->position.y = pData[1] * yExtent;
        pVertex->position.z = pData[2];

        pVertex->normal.x = 0.0;
        pVertex->normal.y = 0.0;
        pVertex->normal.z = 1.0;

        // tex coords
        pVertex->uv.x = pData[3];
        pVertex->uv.y = pData[4];

        pData += vDataFloats;
        ++pVertex;
    }

    pIndex->indices[0] = 0;
    pIndex->indices[1] = 1;
    pIndex->indices[2] = 2;
     ++pIndex;
    pIndex->indices[0] = 1;
    pIndex->indices[1] = 3;
    pIndex->indices[2] = 2;
}

} // end anonymous namespace

bool
CachedMeshLoader::loadGeometry(const fs::path& path, std::vector<float>& pointData,
                               std::vector<uint16_t>& indexData, int)
{

    if(path == "cone") {
        generateConeMesh( 12, pointData,
                          indexData, 1.0f, .4f);
        return(true);
    }
    if(path == "cube") {
        generateSimpleCubeMesh( pointData,
                                indexData, 1.0f);
        return(true);
    }
    if(path == "tube") {
        generateTubeMesh( 12, pointData,
                          indexData, 1.0f, .5, .5);
        return(true);
    }
    if(path == "sphere") {
        generateLatLongSphere( pointData,
                             indexData, 12, 24);
        return(true);

    }
    if(path == "quad") {
        generateQuadVertices( pointData,
                              indexData );
        return(true);
    }
    if(path == "octoSphere") {
        generateOctoSphere( pointData,
                            indexData, 1);
        return(true);
    }
    if(path == "icosoSphere") {
        generateIcosoSphere( pointData,
                             indexData, 1);
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


class CachedMeshLoader::CachedMesh
    : public DrawableMesh
{
    CachedMeshLoader *owner_;
    const char *name_;
public:
    CachedMesh(CachedMeshLoader *owner, const char *name)
        : owner_(owner)
        , name_(name)
    {
    }
    ~CachedMesh() {
        if(owner_) {
            owner_->onMeshDestroy(this);
            owner_ = nullptr;
        }
    }
    const char *getName() const {
        return(name_);
    }
};

void
CachedMeshLoader::onMeshDestroy(CachedMesh *mesh) {
    auto found = cache_.find(RcString(mesh->getName()));
    if(found != cache_.end()) {
        cache_.erase(found);
    }
}

ObjectPtr<DrawableMesh>
CachedMeshLoader::loadMesh( StringArg pathName) {

    RcString key(pathName);

    MMapT::iterator found = cache_.find(key);
    if(found != cache_.end()) {
        WeakPtr<CachedMesh> &wp = found->second;
        ObjectPtr<CachedMesh> op = wp.lock();
        if(op) {
            return(op);
        }
    }

    std::vector<float> pointData;
    std::vector<uint16_t> indexData;

    int ret = loadGeometry(key.c_str(), pointData, indexData);

    if (!ret) {
        AD_LOG(info) << "Could not load geometry for " << pathName;
        return(nullptr);
    }

    ObjectPtr<CachedMesh> loaded = ObjectPtr<CachedMesh>::make(this,key.c_str());
    cache_.emplace(key,WeakPtr<CachedMesh>(loaded));

    loaded->indexCount_ = (int)indexData.size();

    loaded->iChunk_ = owner().bufferManager_->allocIndexChunk((int)indexData.size(), (const uint16_t *)(indexData.data()));
    loaded->vChunk_ = owner().bufferManager_->allocVertexChunk((int)pointData.size(), pointData.data());
    return(loaded);
}

ObjectPtr<DrawableMesh>
CachedMeshLoader::createMesh(const DrawableMeshDescriptor &desc) {

    // cludge for generating names for un-named meshes.
    static int id = 0;

    RcString key;
    if(desc.cacheName) {
        key = desc.cacheName;
    } else {
        key = RcString::format("_mesh-%d_", id);
        ++id;
    }
    
    if(desc.vertexCount == 0 || desc.indexCount == 0 || desc.vertices == nullptr || desc.indices == nullptr) {
        AD_LOG(error) << "invalid values in MeshDescriptor!";
        return(nullptr);
    }

    MMapT::iterator found = cache_.find(key);
    if(found != cache_.end()) {
        AD_LOG(error) << "mesh \"" << key << "\" already in use!";
        return(nullptr);
    }

    ObjectPtr<CachedMesh> loaded = ObjectPtr<CachedMesh>::make(this,key.c_str());
    cache_.emplace(key,WeakPtr<CachedMesh>(loaded));

    const float *vertices = (float*)(desc.vertices);
    uint32_t vertexCount = desc.vertexCount * GpuVertexAttributes::floatsPerVertex();
    
    loaded->indexCount_ = (int)(desc.indexCount);
    loaded->iChunk_ = owner().bufferManager_->allocIndexChunk(desc.indexCount, desc.indices);
    loaded->vChunk_ = owner().bufferManager_->allocVertexChunk(vertexCount, vertices );

    return(loaded);
}


ARTD_END
