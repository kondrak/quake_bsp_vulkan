#ifndef Q3BSP_INCLUDED
#define Q3BSP_INCLUDED

/*
 * Data structures for Quake III BSP (according to http://www.mralligator.com/q3/)
 */


// math types
struct vec2i
{
    int x, y;
};

struct vec3i
{
    int x, y, z;
};

struct vec2f
{
    float x, y;
};

struct vec3f
{
    float x, y, z;
};


//data types (lumps)
enum LumpTypes
{
    Entities = 0,
    Textures,
    Planes,
    Nodes,
    Leafs,
    LeafFaces,
    LeafBrushes,
    Models,
    Brushes,
    BrushSides,
    Vertices,
    MeshVerts,
    Effects,
    Faces,
    Lightmaps,
    LightVols,
    VisData
};


enum FaceTypes
{
    FaceTypePolygon = 1,
    FaceTypePatch = 2,
    FaceTypeMesh = 3,
    FaceTypeBillboard = 4
};


struct Q3BspDirEntry
{
    int offset;
    int length;
};


struct Q3BspHeader
{
    char          magic[4];
    int           version;
    Q3BspDirEntry direntries[17];
};


struct Q3BspEntityLump
{
    int  size;
    char *ents;
};


struct Q3BspTextureLump
{
    char name[64];
    int  flags;
    int  contents;
};


struct Q3BspPlaneLump
{
    vec3f normal;
    float dist;
};


struct Q3BspNodeLump
{
    int   plane;
    vec2i children;
    vec3i mins;
    vec3i maxs;
};


struct Q3BspLeafLump
{
    int   cluster;
    int   area;
    vec3i mins;
    vec3i maxs;
    int   leafFace;
    int   n_leafFaces;
    int   leafBrush;
    int   n_leafBrushes;
};


struct Q3BspLeafFaceLump
{
    int face;
};


struct Q3BspLeafBrushLump
{
    int brush;
};


struct Q3BspModelLump
{
    vec3f mins;
    vec3f maxs;
    int   face;
    int   n_faces;
    int   brush;
    int   n_brushes;
};


struct Q3BspBrushLump
{
    int brushSide;
    int n_brushSides;
    int texture;
};


struct Q3BspBrushSideLump
{
    int plane;
    int texture;
};


struct Q3BspVertexLump
{
    vec3f position;
    vec2f texcoord[2];
    vec3f normal;
    unsigned char color[4];

    Q3BspVertexLump operator+(const Q3BspVertexLump & rhs) const
    {
        Q3BspVertexLump result;
        result.position.x = position.x + rhs.position.x;
        result.position.y = position.y + rhs.position.y;
        result.position.z = position.z + rhs.position.z;

        result.texcoord[0].x = texcoord[0].x + rhs.texcoord[0].x;
        result.texcoord[0].y = texcoord[0].y + rhs.texcoord[0].y;
        result.texcoord[1].x = texcoord[1].x + rhs.texcoord[1].x;
        result.texcoord[1].y = texcoord[1].y + rhs.texcoord[1].y;

        return result;
    }

    Q3BspVertexLump operator*(const float rhs) const
    {
        Q3BspVertexLump result;
        result.position.x = position.x * rhs;
        result.position.y = position.y * rhs;
        result.position.z = position.z * rhs;
        result.texcoord[0].x = texcoord[0].x * rhs;
        result.texcoord[0].y = texcoord[0].y * rhs;
        result.texcoord[1].x = texcoord[1].x * rhs;
        result.texcoord[1].y = texcoord[1].y * rhs;

        return result;
    }
};


struct Q3BspMeshVertLump
{
    int offset;
};


struct Q3BspEffectLump
{
    char name[64];
    int  brush;
    int  unknown;
};


struct Q3BspFaceLump
{
    int   texture;
    int   effect;
    int   type;
    int   vertex;
    int   n_vertexes;
    int   meshvert;
    int   n_meshverts;
    int   lm_index;
    vec2i lm_start;
    vec2i lm_size;
    vec3f lm_origin;
    vec3f lm_vecs[2];
    vec3f normal;
    vec2i size;
};


struct Q3BspLightMapLump
{
    unsigned char map[128 * 128 * 3];
};


struct Q3BspLightVolLump
{
    unsigned char ambient[3];
    unsigned char directional[3];
    unsigned char dir[2];
};


struct Q3BspVisDataLump
{
    int n_vecs;
    int sz_vecs;
    unsigned char* vecs;
};

#endif