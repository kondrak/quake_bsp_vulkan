#ifndef STATSUI_INCLUDED
#define STATSUI_INCLUDED

/*
 * Map stats display UI base class. Future reference for other BSP format support.
 */

class BspMap;

class StatsUI
{
public:
    StatsUI(BspMap *map) : m_map(map) {}
    virtual ~StatsUI() {}

    virtual void OnRender()        = 0; // perform rendering
    virtual void OnWindowChanged() = 0; // perform renderer update (rebuild swapchain, pipelines, etc.)
protected:
    BspMap *m_map;
};


#endif