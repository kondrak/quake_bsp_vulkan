#ifndef BSPMAP_INCLUDED
#define BSPMAP_INCLUDED

#include "q3bsp/Q3BspRenderHelpers.hpp"

/*
 *  Base class for renderable bsp map. Future reference for other BSP format support.
 */

class BspMap
{
public:
    BspMap(bool bspValid) : m_bspValid(bspValid) {}
    virtual ~BspMap() {}

    virtual void Init() = 0;
    virtual void OnRender()        = 0;  // perform rendering
    virtual void OnWindowChanged() = 0;  // perform renderer update (rebuild swapchain, pipelines, etc.)

    virtual bool ClusterVisible(int cameraCluster, int testCluster) const    = 0;  // determine bsp cluster visibility
    virtual int  FindCameraLeaf(const Math::Vector3f &cameraPosition) const  = 0;  // return bsp leaf index containing the camera
    virtual void CalculateVisibleFaces(const Math::Vector3f &cameraPosition) = 0;  // determine which bsp faces are visible

    // render helpers - extra flags + map statistics
    virtual void ToggleRenderFlag(int flag) = 0;
    inline bool  HasRenderFlag(int flag) const { return (m_renderFlags & flag) == flag; }
    inline bool  Valid() const { return m_bspValid; }
    inline const BspStats &GetMapStats() const { return m_mapStats; }
protected:
    int      m_renderFlags = 0;
    bool     m_bspValid;
    BspStats m_mapStats;
};

#endif