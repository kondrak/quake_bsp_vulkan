#include "renderer/CameraDirector.hpp"


CameraDirector::~CameraDirector()
{
    for (auto &it : m_cameras)
    {
        delete it;
    }
}

int CameraDirector::AddCamera(float x, float y, float z)
{
    m_cameras.push_back(new Camera(x, y, z));
    m_activeCamera = m_cameras.back();

    return (int)m_cameras.size();
}

int CameraDirector::AddCamera(const Math::Vector3f &position,
                              const Math::Vector3f &up,
                              const Math::Vector3f &right,
                              const Math::Vector3f &view)
{
    m_cameras.push_back(new Camera(position, up, right, view));
    m_activeCamera = m_cameras.back();

    return (int)m_cameras.size();
}

void CameraDirector::OnMouseMove(int x, int y)
{
    m_activeCamera->OnMouseMove(x, y);
}