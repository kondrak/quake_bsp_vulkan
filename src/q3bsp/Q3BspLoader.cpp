#include "q3bsp/Q3BspLoader.hpp"

#ifdef __ANDROID__
extern AAssetManager *g_androidAssetMgr;
#endif

Q3BspMap *Q3BspLoader::Load(const char *filename)
{
    BSP_INPUT_TYPE bspFile;
    BSP_OPEN(bspFile, filename);

    if (!BSP_IS_OPEN(bspFile))
    {
        return new Q3BspMap(false);
    }

    // bsp header
    Q3BspHeader bspHeader;
    LoadBspHeader(bspHeader, bspFile);

    //validate the header
    bool validQ3Bsp = !strncmp(bspHeader.magic, "IBSP", 4) && (bspHeader.version == 0x2e);

    if (!validQ3Bsp)
    {
        return new Q3BspMap(false);
    }

    // header is valid - load the rest of the map
    Q3BspMap *q3map = new Q3BspMap(true);

    q3map->header = bspHeader;

    // entities lump
    LoadEntitiesLump(q3map, bspFile);
    // generic lumps
    LoadLump(q3map, Textures, q3map->textures, bspFile);
    LoadLump(q3map, Planes, q3map->planes, bspFile);
    LoadLump(q3map, Nodes, q3map->nodes, bspFile);
    LoadLump(q3map, Leafs, q3map->leaves, bspFile);
    LoadLump(q3map, LeafFaces, q3map->leafFaces, bspFile);
    LoadLump(q3map, LeafBrushes, q3map->leafBrushes, bspFile);
    LoadLump(q3map, Models, q3map->models, bspFile);
    LoadLump(q3map, Brushes, q3map->brushes, bspFile);
    LoadLump(q3map, BrushSides, q3map->brushSides, bspFile);
    LoadLump(q3map, Vertices, q3map->vertices, bspFile);
    LoadLump(q3map, MeshVerts, q3map->meshVertices, bspFile);
    LoadLump(q3map, Effects, q3map->effects, bspFile);
    LoadLump(q3map, Faces, q3map->faces, bspFile);
    LoadLump(q3map, Lightmaps, q3map->lightMaps, bspFile);
    LoadLump(q3map, LightVols, q3map->lightVols, bspFile);
    // vis data lump
    LoadVisDataLump(q3map, bspFile);

    BSP_CLOSE(bspFile);

    return q3map;
}

void Q3BspLoader::LoadBspHeader(Q3BspHeader &hdr, BSP_INPUT_FILE bsp)
{
    BSP_READ(bsp, (char*)&(hdr), sizeof(Q3BspHeader));
}

void Q3BspLoader::LoadEntitiesLump(Q3BspMap *map, BSP_INPUT_FILE bsp)
{
    map->entities.size = map->header.direntries[Entities].length;
    map->entities.ents = new char[map->entities.size];

    BSP_SEEK_SET(bsp, map->header.direntries[Entities].offset);
    BSP_READ(bsp, map->entities.ents, sizeof(char) * map->entities.size);
}

void Q3BspLoader::LoadVisDataLump(Q3BspMap *map, BSP_INPUT_FILE bsp)
{
    BSP_SEEK_SET(bsp, map->header.direntries[VisData].offset);

    BSP_READ(bsp, (char *)&(map->visData.n_vecs), sizeof(int));
    BSP_READ(bsp, (char *)&(map->visData.sz_vecs), sizeof(int));

    int vecSize = map->visData.n_vecs * map->visData.sz_vecs;
    map->visData.vecs = new unsigned char[vecSize];

    BSP_READ(bsp, (char *)(map->visData.vecs), vecSize * sizeof(unsigned char));
}