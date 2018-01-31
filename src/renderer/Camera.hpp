#ifndef CAMERA_INCLUDED
#define CAMERA_INCLUDED

#include "Math.hpp"

/*
 *  DOF6/FPS camera
 */

class Camera
{
public:
    enum CameraMode
    {
        CAM_DOF6,
        CAM_FPS,
        CAM_ORTHO
    };

    Camera(float x, float y, float z);

    Camera(const Math::Vector3f &position,
           const Math::Vector3f &up,
           const Math::Vector3f &right,
           const Math::Vector3f &view);

    void UpdateView();
    void RotateCamera(float angle, float x, float y, float z);
    void RotateCamera(const Math::Quaternion &q);
    void Move(const Math::Vector3f &Direction);
    void MoveForward(float Distance);
    void MoveUpward(float Distance);
    void Strafe(float Distance);

    void SetMode(CameraMode cm);
    inline CameraMode GetMode() { return m_mode; }
    const Math::Vector3f &Position() const { return m_position; }

    // rotate in Euler-space - used mainly for some debugging
    void rotateX(float angle);
    void rotateY(float angle);
    void rotateZ(float angle);

    void OnMouseMove(int x, int y);

    void UpdateProjection();
    const Math::Matrix4f &ProjectionMatrix() const { return m_projectionMatrix; }
    const Math::Matrix4f &ViewMatrix() const { return m_viewMatrix; }

    // manually specify vector values
    void SetRightVector(float x, float y, float z)
    {
        m_rightVector.m_x = x;
        m_rightVector.m_y = y;
        m_rightVector.m_z = z;
    }

    void SetUpVector(float x, float y, float z)
    {
        m_upVector.m_x = x;
        m_upVector.m_y = y;
        m_upVector.m_z = z;
    }

    void SetViewVector(float x, float y, float z)
    {
        m_viewVector.m_x = x;
        m_viewVector.m_y = y;
        m_viewVector.m_z = z;
    }
private:
    CameraMode     m_mode;
    Math::Vector3f m_position;
    float          m_yLimit;
    Math::Matrix4f m_viewMatrix;
    Math::Matrix4f m_projectionMatrix;

    // camera orientation vectors
    Math::Vector3f m_viewVector;
    Math::Vector3f m_rightVector;
    Math::Vector3f m_upVector;

    // current rotation info (angles in radian)
    Math::Vector3f m_rotation;
};

#endif