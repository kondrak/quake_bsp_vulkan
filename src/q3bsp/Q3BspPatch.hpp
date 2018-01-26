#ifndef Q3BSPPATCH_INCLUDED
#define Q3BSPPATCH_INCLUDED

#include "q3bsp/Q3BspMap.hpp"


// Quake III BSP curved surface component ( biquadratic (3x3) patch )
class Q3BspBiquadPatch
{
public:
    ~Q3BspBiquadPatch()
    {
        delete[] m_trianglesPerRow;
        delete[] m_rowIndexPointers;
    }

    void Tesselate(int tessLevel); // perform tesselation 

    Q3BspVertexLump              controlPoints[9];
    std::vector<Q3BspVertexLump> m_vertices;
    int                          m_tesselationLevel = 0;
    std::vector<unsigned int>    m_indices;
private:
    int*            m_trianglesPerRow  = nullptr;
    unsigned int**  m_rowIndexPointers = nullptr;
};


// Quake III BSP curved surface (an array of biquadratic patches)
struct Q3BspPatch
{
    int textureIdx  = 0; // surface texture index
    int lightmapIdx = 0; // surface lightmap index
    int width  = 0;
    int height = 0;

    std::vector<Q3BspBiquadPatch> quadraticPatches;
};

#endif