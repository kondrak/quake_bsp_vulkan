#ifndef FRUSTUM_INCLUDED
#define FRUSTUM_INCLUDED

#include "Math.hpp"

/*
 * View frustum
 */

struct Plane
{
    float A, B, C, D;
};

class Frustum
{
public:
    void UpdatePlanes();
    bool BoxInFrustum(const Math::Vector3f *vertices);

private:
    void ExtractPlane(Plane &plane, const Math::Matrix4f &mvpMatrix, int row);
    Plane m_planes[6];
};

#endif