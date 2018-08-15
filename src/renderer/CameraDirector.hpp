#ifndef CAMERADIRECTOR_INCLUDED
#define CAMERADIRECTOR_INCLUDED

#include "renderer/Camera.hpp"
#include <vector>

/*
 * global manager for all game cameras
 */

class CameraDirector
{
public:
    ~CameraDirector();

    int AddCamera(float x, float y, float z);
    int AddCamera(const Math::Vector3f &position,
                  const Math::Vector3f &up,
                  const Math::Vector3f &right,
                  const Math::Vector3f &view);
    Camera *GetActiveCamera()        { return m_activeCamera; }
    Camera *GetCamera(int camIdx)    { return m_cameras[camIdx]; }
    void SetActiveCamera(int camIdx) { m_activeCamera = m_cameras[camIdx]; }
private:
    Camera *m_activeCamera = nullptr;
    std::vector<Camera *> m_cameras;
};

#endif