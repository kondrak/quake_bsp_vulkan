#include "Math.hpp"

namespace Math
{

/*
 * 4x4 Matrix
 */

    void Matrix4f::Identity()
    {
        m_m[0] = 1.f;
        m_m[1] = 0.f;
        m_m[2] = 0.f;
        m_m[3] = 0.f;

        m_m[4] = 0.f;
        m_m[5] = 1.f;
        m_m[6] = 0.f;
        m_m[7] = 0.f;

        m_m[8]  = 0.f;
        m_m[9]  = 0.f;
        m_m[10] = 1.f;
        m_m[11] = 0.f;

        m_m[12] = 0.f;
        m_m[13] = 0.f;
        m_m[14] = 0.f;
        m_m[15] = 1.f;
    }

    void Matrix4f::Zero()
    {
        for (int i = 0; i < 16; i++)
        {
            m_m[i] = 0.0f;
        }
    }

    void Matrix4f::One()
    {
        for (int i = 0; i < 16; i++)
        {
            m_m[i] = 1.0f;
        }
    }

    void Matrix4f::Transpose()
    {
        Matrix4f temp;

        for (int i = 0; i < 16; i++)
            temp[i] = m_m[i];

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                m_m[i * 4 + j] = temp[i + j * 4];
            }
        }
    }

    void Matrix4f::Invert()
    {
        // todo
    }

    Vector3f Matrix4f::operator*(const Vector3f &v) const
    {
        return Vector3f(m_m[0] * v.m_x + m_m[1] * v.m_y + m_m[2]  * v.m_z + m_m[3]  * 1.f,
                        m_m[4] * v.m_x + m_m[5] * v.m_y + m_m[6]  * v.m_z + m_m[7]  * 1.f,
                        m_m[8] * v.m_x + m_m[9] * v.m_y + m_m[10] * v.m_z + m_m[11] * 1.f);
    }

    Vector4f Matrix4f::operator*(const Vector4f &v) const
    {
        return Vector4f(m_m[0]  * v.m_x + m_m[1]  * v.m_y + m_m[2]  * v.m_z + m_m[3]  * 1.f,
                        m_m[4]  * v.m_x + m_m[5]  * v.m_y + m_m[6]  * v.m_z + m_m[7]  * 1.f,
                        m_m[8]  * v.m_x + m_m[9]  * v.m_y + m_m[10] * v.m_z + m_m[11] * 1.f,
                        m_m[12] * v.m_x + m_m[13] * v.m_y + m_m[14] * v.m_z + m_m[15] * 1.f);
    }

    Matrix4f Matrix4f::operator*(const Matrix4f &m2) const
    {
        return Matrix4f(m_m[0] * m2[0] + m_m[1] * m2[4] + m_m[2] * m2[8]  + m_m[3] * m2[12],
                        m_m[0] * m2[1] + m_m[1] * m2[5] + m_m[2] * m2[9]  + m_m[3] * m2[13],
                        m_m[0] * m2[2] + m_m[1] * m2[6] + m_m[2] * m2[10] + m_m[3] * m2[14],
                        m_m[0] * m2[3] + m_m[1] * m2[7] + m_m[2] * m2[11] + m_m[3] * m2[15], 
                        
                        m_m[4] * m2[0] + m_m[5] * m2[4] + m_m[6] * m2[8]  + m_m[7] * m2[12],
                        m_m[4] * m2[1] + m_m[5] * m2[5] + m_m[6] * m2[9]  + m_m[7] * m2[13],
                        m_m[4] * m2[2] + m_m[5] * m2[6] + m_m[6] * m2[10] + m_m[7] * m2[14],
                        m_m[4] * m2[3] + m_m[5] * m2[7] + m_m[6] * m2[11] + m_m[7] * m2[15],

                        m_m[8] * m2[0] + m_m[9] * m2[4] + m_m[10] * m2[8]  + m_m[11] * m2[12],
                        m_m[8] * m2[1] + m_m[9] * m2[5] + m_m[10] * m2[9]  + m_m[11] * m2[13],
                        m_m[8] * m2[2] + m_m[9] * m2[6] + m_m[10] * m2[10] + m_m[11] * m2[14],
                        m_m[8] * m2[3] + m_m[9] * m2[7] + m_m[10] * m2[11] + m_m[11] * m2[15],

                        m_m[12] * m2[0] + m_m[13] * m2[4] + m_m[14] * m2[8]  + m_m[15] * m2[12],
                        m_m[12] * m2[1] + m_m[13] * m2[5] + m_m[14] * m2[9]  + m_m[15] * m2[13],
                        m_m[12] * m2[2] + m_m[13] * m2[6] + m_m[14] * m2[10] + m_m[15] * m2[14],
                        m_m[12] * m2[3] + m_m[13] * m2[7] + m_m[14] * m2[11] + m_m[15] * m2[15]
                        );
    }

/*
 * Vector
 */
    void Vector3f::Normalize()
    {
        float l = Length(); 
        if (l == 0.0f) return;
        m_x /= l;
        m_y /= l;
        m_z /= l;
    }

    void Vector3f::QuickNormalize()
    {
        float l = QuickInverseSqrt( m_x*m_x + m_y*m_y + m_z*m_z );
        m_x *= l;
        m_y *= l;
        m_z *= l;
    }

    Vector3f Vector3f::CrossProduct(const Vector3f &v2) const
    {
        return Vector3f( m_y*v2.m_z - m_z*v2.m_y,
                         m_z*v2.m_x - m_x*v2.m_z,
                         m_x*v2.m_y - m_y*v2.m_x );
    }

    float Vector3f::DotProduct(const Vector3f &v2) const
    {
        return m_x*v2.m_x + m_y*v2.m_y + m_z*v2.m_z;
    }

    Vector3f Vector3f::operator+(const Vector3f &v2) const 
    {
        return Vector3f( m_x + v2.m_x,
                         m_y + v2.m_y,
                         m_z + v2.m_z );;
    }

    Vector3f Vector3f::operator-(const Vector3f &v2) const
    {
        return Vector3f( m_x-v2.m_x,
                         m_y-v2.m_y,
                         m_z-v2.m_z );
    }

    Vector3f Vector3f::operator*( float r ) const
    {
        return Vector3f( m_x*r,
                         m_y*r,
                         m_z*r );
    }

    Vector3f Vector3f::operator/(float r) const
    {
        return Vector3f(m_x/r,
                        m_y/r,
                        m_z/r);
    }

/*
 *  Quaternion
 */
    Quaternion Quaternion::GetConjugate() const
    {
        return Quaternion( -m_x, -m_y, -m_z, m_w);
    }

    void Quaternion::Normalize()
    {
        float l = sqrtf( m_x*m_x + m_y*m_y + m_z*m_z + m_w*m_w );
        m_x /= l;
        m_y /= l;
        m_z /= l;
        m_w /= l;
    }

    void Quaternion::QuickNormalize()
    {
        float l = QuickInverseSqrt( m_x*m_x + m_y*m_y + m_z*m_z + m_w*m_w );
        m_x *= l;
        m_y *= l;
        m_z *= l;
        m_w *= l;
    }

    Quaternion Quaternion::operator*(const Quaternion &q2) const
    {
        return Quaternion( m_w*q2.m_x + m_x*q2.m_w + m_y*q2.m_z - m_z*q2.m_y,
                           m_w*q2.m_y - m_x*q2.m_z + m_y*q2.m_w + m_z*q2.m_x,
                           m_w*q2.m_z + m_x*q2.m_y - m_y*q2.m_x + m_z*q2.m_w,
                           m_w*q2.m_w - m_x*q2.m_x - m_y*q2.m_y - m_z*q2.m_z );
    }


    Vector3f Quaternion::operator*(const Vector3f &vec) const
    {
        Vector3f vn(vec);
        vn.QuickNormalize();

        Quaternion vecQuat( vn.m_x,
                            vn.m_y,
                            vn.m_z,
                            0.0f );

        Quaternion resQuat( vecQuat * GetConjugate() );
        resQuat = *this * resQuat;

        return Vector3f(resQuat.m_x, resQuat.m_y, resQuat.m_z);
    }


/*
 * General purpose functions
 */
    float QuickInverseSqrt( float number )
    {
        long i;
        float x2, y;

        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;
        i  = 0x5f3759df - ( i >> 1 );
        y  = * ( float * ) &i;
        y  = y * ( 1.5f - ( x2 * y * y ) );   // 1st iteration
        y  = y * ( 1.5f - ( x2 * y * y ) );   // 2nd iteration

        return y;
    }

    int PointPlanePos(float normalX, float normalY, float normalZ, float intercept, const Math::Vector3f &point)
    {
        float distance = point.m_x * normalX + point.m_y * normalY + point.m_z * normalZ - intercept;

        if(distance >= 0.0f)
            return PointInFrontOfPlane;

        return PointBehindPlane; 
    }

    void Translate(Matrix4f &matrix, float x, float y, float z)
    {
        float tx = x;
        float ty = y;
        float tz = z;
        float tw = 1.f;

        float t1 = matrix[0] * tx + matrix[4] * ty + matrix[8]  * tz + matrix[12] * tw;
        float t2 = matrix[1] * tx + matrix[5] * ty + matrix[9]  * tz + matrix[13] * tw;
        float t3 = matrix[2] * tx + matrix[6] * ty + matrix[10] * tz + matrix[14] * tw;
        float t4 = matrix[3] * tx + matrix[7] * ty + matrix[11] * tz + matrix[15] * tw;

        matrix[12] = t1;
        matrix[13] = t2;
        matrix[14] = t3;
        matrix[15] = t4;
    }

    void Scale(Matrix4f &matrix, float x, float y, float z)
    {
        float sx = x;
        float sy = y;
        float sz = z;
        float sw = 1.f;

        matrix[0]  *= sx;
        matrix[1]  *= sx;
        matrix[2]  *= sx;
        matrix[3]  *= sx;
        matrix[4]  *= sy;
        matrix[5]  *= sy;
        matrix[6]  *= sy;
        matrix[7]  *= sy;
        matrix[8]  *= sz;
        matrix[9]  *= sz;
        matrix[10] *= sz;
        matrix[11] *= sz;
        matrix[12] *= sw;
        matrix[13] *= sw;
        matrix[14] *= sw;
        matrix[15] *= sw;
    }

    // create perspective projection matrix
    void MakePerspective(Math::Matrix4f &matrix, float fov, float scrRatio, float nearPlane, float farPlane)
    {
        matrix.Zero();

        float tanFov = tanf(0.5f * fov); // fov is in radians!

        matrix[0] = 1.f / (scrRatio * tanFov);
        matrix[5] = 1.f / tanFov;
        matrix[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        matrix[11] = -1.f;
        matrix[14] = -2.f * farPlane * nearPlane / (farPlane - nearPlane);
    }


    // create orthographic projection matrix
    void MakeOrthogonal(Math::Matrix4f &matrix, float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        matrix.Identity();

        matrix[0] = 2.f / (right - left);
        matrix[5] = 2.f / (top - bottom);
        matrix[10] = -2.f / (farPlane - nearPlane);
        matrix[12] = -(right + left) / (right - left);
        matrix[13] = -(top + bottom) / (top - bottom);
        matrix[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    }

    // create view matrix
    void MakeView(Math::Matrix4f &matrix, const Math::Vector3f &eye, const Math::Vector3f &target, const Math::Vector3f &up)
    {
        Math::Vector3f z = target;
        z.Normalize();
        Math::Vector3f x = z.CrossProduct(up);
        x.Normalize();
        Math::Vector3f y = x.CrossProduct(z);

        matrix.Identity();

        matrix[0] = x.m_x;
        matrix[4] = x.m_y;
        matrix[8] = x.m_z;

        matrix[1] = y.m_x;
        matrix[5] = y.m_y;
        matrix[9] = y.m_z;

        matrix[2] = -z.m_x;
        matrix[6] = -z.m_y;
        matrix[10] = -z.m_z;

        matrix[12] = -x.DotProduct(eye);
        matrix[13] = -y.DotProduct(eye);
        matrix[14] = z.DotProduct(eye);
    }
}