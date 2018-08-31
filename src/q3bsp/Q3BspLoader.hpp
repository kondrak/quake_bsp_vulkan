#ifndef Q3BSPLOADER_INCLUDED
#define Q3BSPLOADER_INCLUDED

#include "q3bsp/Q3BspMap.hpp"
#ifdef __ANDROID__
#include <android/asset_manager.h>
#define BSP_INPUT_TYPE AAsset*
#define BSP_INPUT_FILE BSP_INPUT_TYPE
#define BSP_READ(bspFile, ...) AAsset_read(bspFile, __VA_ARGS__)
#define BSP_SEEK_SET(bspFile, offset) AAsset_seek(bspFile, offset, SEEK_SET);
#define BSP_OPEN(bspFile, filename) bspFile = AAssetManager_open(g_androidAssetMgr, filename, AASSET_MODE_STREAMING)
#define BSP_IS_OPEN(bspFile) bspFile
#define BSP_CLOSE(bspFile) AAsset_close(bspFile)
#else
#include <fstream>
#define BSP_INPUT_TYPE std::ifstream
#define BSP_INPUT_FILE BSP_INPUT_TYPE&
#define BSP_READ(bspFile, ...) bspFile.read(__VA_ARGS__)
#define BSP_SEEK_SET(bspFile, offset) bspFile.seekg(offset, std::ios_base::beg)
#define BSP_OPEN(bspFile, filename) bspFile.open(filename, std::ios::in | std::ios::binary);
#define BSP_CLOSE(bspFile) bspFile.close()
#define BSP_IS_OPEN(bspFile) bspFile.is_open()
#endif

/*
 *  Loading class for Q3 bsp
 */

class Q3BspLoader
{
public:
    Q3BspMap *Load(const char *filename);

private:
    void LoadBspHeader(Q3BspHeader &hdr, BSP_INPUT_FILE bsp);
    void LoadEntitiesLump(Q3BspMap *map, BSP_INPUT_FILE bsp);
    void LoadVisDataLump(Q3BspMap  *map, BSP_INPUT_FILE bsp);

    template<class T>
    void LoadLump(Q3BspMap *map, LumpTypes lType, std::vector<T> &container, BSP_INPUT_FILE bsp);
};


// common loader for generic bsp lumps
template<class T>
void Q3BspLoader::LoadLump(Q3BspMap *map, LumpTypes lType, std::vector<T> &container, BSP_INPUT_FILE bsp)
{
    int numElements = map->header.direntries[lType].length / sizeof(T);
    BSP_SEEK_SET(bsp, map->header.direntries[lType].offset);

    container.reserve(numElements);

    for (int i = 0; i < numElements; i++)
    {
        T element;
        BSP_READ(bsp, (char*)&element, sizeof(T));

        container.push_back(element);
    }
}

#endif