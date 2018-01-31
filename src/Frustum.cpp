#include "Frustum.hpp"
#include "renderer/RenderContext.hpp"

extern RenderContext g_renderContext;

void Frustum::UpdatePlanes()
{
    // extract each plane from MVP matrix
    ExtractPlane(m_planes[0], g_renderContext.ModelViewProjectionMatrix,  1);
    ExtractPlane(m_planes[1], g_renderContext.ModelViewProjectionMatrix, -1);
    ExtractPlane(m_planes[2], g_renderContext.ModelViewProjectionMatrix,  2);
    ExtractPlane(m_planes[3], g_renderContext.ModelViewProjectionMatrix, -2);
    ExtractPlane(m_planes[4], g_renderContext.ModelViewProjectionMatrix,  3);
    ExtractPlane(m_planes[5], g_renderContext.ModelViewProjectionMatrix, -3);
}

bool Frustum::BoxInFrustum(const Math::Vector3f *vertices)
{
    for (int i = 0; i < 6; ++i)
    {
        if ((m_planes[i].A * vertices[0].m_x + m_planes[i].B * vertices[0].m_y + m_planes[i].C * vertices[0].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[1].m_x + m_planes[i].B * vertices[1].m_y + m_planes[i].C * vertices[1].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[2].m_x + m_planes[i].B * vertices[2].m_y + m_planes[i].C * vertices[2].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[3].m_x + m_planes[i].B * vertices[3].m_y + m_planes[i].C * vertices[3].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[4].m_x + m_planes[i].B * vertices[4].m_y + m_planes[i].C * vertices[4].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[5].m_x + m_planes[i].B * vertices[5].m_y + m_planes[i].C * vertices[5].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[6].m_x + m_planes[i].B * vertices[6].m_y + m_planes[i].C * vertices[6].m_z + m_planes[i].D) > 0)
            continue;
        if ((m_planes[i].A * vertices[7].m_x + m_planes[i].B * vertices[7].m_y + m_planes[i].C * vertices[7].m_z + m_planes[i].D) > 0)
            continue;

        return false;
    }

    return true;
}

// extract a plane from a given matrix and row id
void Frustum::ExtractPlane(Plane &plane, const Math::Matrix4f &mvpMatrix, int row)
{
    int scale = (row < 0) ? -1 : 1;
    row = abs(row) - 1;

    // calculate plane coefficients from the matrix
    plane.A = mvpMatrix.m_m[3]  + scale * mvpMatrix.m_m[row + 0];
    plane.B = mvpMatrix.m_m[7]  + scale * mvpMatrix.m_m[row + 4];
    plane.C = mvpMatrix.m_m[11] + scale * mvpMatrix.m_m[row + 8];
    plane.D = mvpMatrix.m_m[15] + scale * mvpMatrix.m_m[row + 12];

    // normalize the plane
    float length = Math::QuickInverseSqrt(plane.A * plane.A + plane.B * plane.B + plane.C * plane.C);

    plane.A *= length;
    plane.B *= length;
    plane.C *= length;
    plane.D *= length;
}