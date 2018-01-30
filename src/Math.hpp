#ifndef MATH_INCLUDED
#define MATH_INCLUDED

#include <math.h>

/*
 * Basic math structures (vectors, quaternions, matrices)
 */

#define PI 3.1415926535897932384626433832795
#define PIdiv180inv 57.29577951308f
#define PIdiv180    0.01745329251f
#define PIdiv2      1.57079632679f

namespace Math
{
    // 2D vector
    struct Vector2f
    {
        Vector2f() {}
        Vector2f(float x, float y) : m_x(x), m_y(y) {}

        float m_x = 0.f;
        float m_y = 0.f;
    };

    // 3D vector
    class Vector3f
    {
    public:
        Vector3f() {}
        Vector3f(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}
        Vector3f(const Vector3f &v2) : m_x(v2.m_x), m_y(v2.m_y), m_z(v2.m_z) {}

        float Length()
        {
            return sqrtf(m_x*m_x + m_y*m_y + m_z*m_z);
        }

        void Normalize();
        void QuickNormalize(); // normalize with Q_rsqrt
        Vector3f CrossProduct(const Vector3f &v2) const;
        float DotProduct(const Vector3f &v2) const;

        float m_x = 0.f;
        float m_y = 0.f;
        float m_z = 0.f;

        Vector3f operator+ (const Vector3f &v2) const;
        Vector3f operator- (const Vector3f &v2) const;
        Vector3f operator* (float r) const;           // scale the vector by r
        Vector3f operator/ (float r) const;           // added for completeness with *
    };

    // 4D vector
    struct Vector4f
    {
        Vector4f() {}
        Vector4f(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}

        float m_x = 0.f;
        float m_y = 0.f;
        float m_z = 0.f;
        float m_w = 0.f;
    };

    struct Matrix4f
    {
        Matrix4f()
        {
            Identity();
        }

        Matrix4f(float m0,  float m1,  float m2,  float m3,
                 float m4,  float m5,  float m6,  float m7,
                 float m8,  float m9,  float m10, float m11,
                 float m12, float m13, float m14, float m15)
        {
            m_m[0]  = m0;
            m_m[1]  = m1;
            m_m[2]  = m2;
            m_m[3]  = m3;
            m_m[4]  = m4;
            m_m[5]  = m5;
            m_m[6]  = m6;
            m_m[7]  = m7;
            m_m[8]  = m8;
            m_m[9]  = m9;
            m_m[10] = m10;
            m_m[11] = m11;
            m_m[12] = m12;
            m_m[13] = m13;
            m_m[14] = m14;
            m_m[15] = m15;
        }

        // construct matrix from raw array of floats
        Matrix4f(float *mData)
        {
            m_m[0]  = mData[0];
            m_m[1]  = mData[1];
            m_m[2]  = mData[2];
            m_m[3]  = mData[3];
            m_m[4]  = mData[4];
            m_m[5]  = mData[5];
            m_m[6]  = mData[6];
            m_m[7]  = mData[7];
            m_m[8]  = mData[8];
            m_m[9]  = mData[9];
            m_m[10] = mData[10];
            m_m[11] = mData[11];
            m_m[12] = mData[12];
            m_m[13] = mData[13];
            m_m[14] = mData[14];
            m_m[15] = mData[15];
        }

        void Identity();
        void Zero();
        void One();
        void Transpose();
        void Invert();

        float m_m[16];

        Vector3f operator* (const Vector3f &v) const;
        Vector4f operator* (const Vector4f &v) const;
        Matrix4f operator* (const Matrix4f &m) const;
        float& operator[](unsigned int i){ return m_m[i]; }
        const float& operator[](unsigned int i)const{ return m_m[i]; }
    };


    // quaternion
    class Quaternion
    {
    public:
        Quaternion() {}
        // create based on standard parameters
        Quaternion(float x, float y, float z, float w) : m_x(x), m_y(y), m_z(z), m_w(w) {}
        // create from axis/angle representaiton
        Quaternion(const Vector3f &axis, float angle) : m_x( axis.m_x * sinf(angle/2) ),
                                                        m_y( axis.m_y * sinf(angle/2) ),
                                                        m_z( axis.m_z * sinf(angle/2) ),
                                                        m_w( cosf(angle/2) ) {}
        Quaternion(const Quaternion &q2) : m_x(q2.m_x), m_y(q2.m_y), m_z(q2.m_z), m_w(q2.m_w) {}

        Quaternion GetConjugate() const;
        void Normalize();
        void QuickNormalize(); // normalize with Q_rsqrt

        float m_x = 0.f;
        float m_y = 0.f;
        float m_z = 0.f;
        float m_w = 0.f;

        Vector3f   operator*(const Vector3f &vec)  const;   // apply quat-rotation to vec
        Quaternion operator*(const Quaternion &q2) const;
    };


/* ************** */

    // general purpose functions
    enum PointPlanePosition
    {
        PointBehindPlane,
        PointInFrontOfPlane
    };

    // quick inverse square root
    float QuickInverseSqrt( float number );

    // determine whether a point is in front of or behind a plane (based on its normal vector)
    int PointPlanePos(float normalX, float normalY, float normalZ, float intercept, const Math::Vector3f &point);

    // translate matrix by (x,y,z)
    void Translate(Matrix4f &matrix, float x, float y=0.0f, float z=0.0f);
    // scale matrix by (x,y,z)
    void Scale(Matrix4f &matrix, float x, float y=1.0f, float z=1.0f);

    // rendering matrices
    void MakePerspective(Math::Matrix4f &matrix, float fov, float scrRatio, float nearPlane, float farPlane);
    void MakeOrthogonal(Math::Matrix4f &matrix, float left, float right, float bottom, float top, float nearPlane, float farPlane);
    void MakeView(Math::Matrix4f &matrix, const Math::Vector3f &eye, const Math::Vector3f &target, const Math::Vector3f &up);
}
#endif