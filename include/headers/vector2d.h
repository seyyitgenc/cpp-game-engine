#ifndef VECTOR_H
#define VECTOR_H
#include <math.h>
#endif

// this struct can be generated as any type
template <class T>
struct v2d_generic
{
    T x = 0.0f;
    T y = 0.0f;
    v2d_generic() : x(0), y(0) {}                         // default constructor
    v2d_generic(T _x, T _y) : x(_x), y(_y) {}             // x and y contructor
    v2d_generic(const v2d_generic &v) : x(v.x), y(v.y) {} // vector constructor
    T mag() { return sqrt(x * x + y * y); }               // magnitude of vector
    v2d_generic norm()                                    // normalization of a vector
    {
        T r = 1 / mag();
        return v2d_generic(x * r, y * r);
    }
    v2d_generic prep() { return v2d_generic(-y, x); }                               // perpendicular
    T dot(const v2d_generic &rhs) { return (this->x * rhs.x + this->y * rhs.y); }   // dot product
    T cross(const v2d_generic &rhs) { return (this->x * rhs.y - this->y * rhs.y); } // cross product
    // operator overloading
    v2d_generic operator+(const v2d_generic &rhs) { return v2d_generic(this->x + rhs.x, this->y + rhs.y); }
    v2d_generic operator-(const v2d_generic &rhs) { return v2d_generic(this->x - rhs.x, this->y - rhs.y); }
    v2d_generic operator*(const T &rhs) { return v2d_generic(this->x * rhs, this->y * rhs); }
    v2d_generic operator/(const T &rhs) { return v2d_generic(this->x / rhs, this->y / rhs); }
    v2d_generic operator+=(const v2d_generic &rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }
    v2d_generic operator-=(const v2d_generic &rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }
    v2d_generic operator*=(const T &rhs)
    {
        this->x *= rhs;
        this->y *= rhs;
        return *this;
    }
    v2d_generic operator/=(const T &rhs)
    {
        this->x /= rhs;
        this->y /= rhs;
        return *this;
    }
};
// for float vectors
typedef v2d_generic<float> vf2d;
// for double vectors
typedef v2d_generic<double> vd2d;
// for int vectors
typedef v2d_generic<int> vi2d;