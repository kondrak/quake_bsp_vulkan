#ifndef Q3BSPLOADER_INCLUDED
#define Q3BSPLOADER_INCLUDED

#include "q3bsp/Q3BspMap.hpp"
#include <fstream>

/*
 *  Loading class for Q3 bsp
 */

class Q3BspLoader
{
public:
    Q3BspMap *Load(const std::string &filename);

private:
    void LoadBspHeader(Q3BspHeader &hdr, std::ifstream &fstream);
    void LoadEntitiesLump(Q3BspMap *map, std::ifstream &fstream);
    void LoadVisDataLump(Q3BspMap  *map, std::ifstream &fstream);

    template<class T>
    void LoadLump(Q3BspMap *map, LumpTypes lType, std::vector<T> &container, std::ifstream &fstream);
};


// common loader for generic bsp lumps
template<class T>
void Q3BspLoader::LoadLump(Q3BspMap *map, LumpTypes lType, std::vector<T> &container, std::ifstream &fstream)
{
    int numElements = map->header.direntries[lType].length / sizeof(T);
    fstream.seekg(map->header.direntries[lType].offset, std::ios_base::beg);

    container.reserve(numElements);

    for (int i = 0; i < numElements; i++)
    {
        T element;
        fstream.read((char*)&element, sizeof(T));

        container.push_back(element);
    }
}

#endif