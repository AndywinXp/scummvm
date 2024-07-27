/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SCUMM_HE_BASKETBALL_GEOTYPES_H
#define SCUMM_HE_BASKETBALL_GEOTYPES_H

#include "scumm/he/intern_he.h"

#ifdef ENABLE_HE

namespace Scumm {

enum EDimension {
	X_INDEX = 0,
	Y_INDEX = 1,
	Z_INDEX = 2
};

enum ERevDirection {
	kClockwise = -1,
	kNone = 0,
	kCounterClockwise = 1
};

enum EAvoidanceType {
	kSingleObject = 0,
	kMultipleObject = 1
};

template<class Type>
class U32Construct2D {

public:
	Type x;
	Type y;

	U32Construct2D() : x(0), y(0){};
	U32Construct2D(Type xIn, Type yIn) : x(xIn), y(yIn){};

	bool operator==(const U32Construct2D<Type> &other) const {
		return ((other.x == x) && (other.y == y));
	}

	U32Construct2D<Type> operator-(const U32Construct2D<Type> &other) const {
		U32Construct2D<Type> newConstruct;

		newConstruct.x = x - other.x;
		newConstruct.y = y - other.y;

		return newConstruct;
	}

	U32Construct2D<Type> operator+(const U32Construct2D<Type> &other) const {
		U32Construct2D<Type> newConstruct;

		newConstruct.x = x + other.x;
		newConstruct.y = y + other.y;

		return newConstruct;
	}

	Type operator*(const U32Construct2D<Type> &multiplier) const {
		return (x * multiplier.x + y * multiplier.y);
	}

	Type &operator[](EDimension dimension) {
		assert(dimension <= Y_INDEX);
		return *(&x + dimension);
	}

	const Type &operator[](EDimension dimension) const {
		assert(dimension <= Y_INDEX);
		return *(&x + dimension);
	}

	Type Magnitude(void) const {
		return (Type)(sqrt(x * x + y * y));
	}
};

template<class Type>
class U32Construct3D {

public:
	Type x;
	Type y;
	Type z;

	U32Construct3D() : x(0), y(0), z(0){};
	U32Construct3D(Type xIn, Type yIn, Type zIn) : x(xIn), y(yIn), z(zIn){};

	Type Magnitude(void) const {
		return (Type)(sqrt(x * x + y * y + z * z));
	}

	Type XYMagnitude(void) const {
		return (Type)(sqrt(x * x + y * y));
	}

	bool operator==(const U32Construct3D<Type> &other) const {
		return ((other.x == x) && (other.y == y) && (other.z == z));
	}

	bool operator!=(const U32Construct2D<Type> &other) const {
		return ((other.x != x) || (other.y != y));
	}

	Type &operator[](EDimension dimension) {
		assert(dimension <= Z_INDEX);
		return *(&x + dimension);
	}

	const Type &operator[](EDimension dimension) const {
		assert(dimension <= Z_INDEX);
		return *(&x + dimension);
	}

	U32Construct3D<Type> operator+(const U32Construct3D<Type> &other) const {
		U32Construct3D<Type> newConstruct;

		newConstruct.x = x + other.x;
		newConstruct.y = y + other.y;
		newConstruct.z = z + other.z;

		return newConstruct;
	}

	Type operator*(const U32Construct3D<Type> &multiplier) const {
		return (x * multiplier.x + y * multiplier.y + z * multiplier.z);
	}

	friend U32Construct3D<Type> operator*(const Type multiplier1[4][4], U32Construct3D<Type> multiplier2) {
		U32Construct3D<Type> newPoint;
		Type h = 0;
		int column, row;

		for (row = X_INDEX; row <= Z_INDEX; ++row) {
			for (column = X_INDEX; column <= Z_INDEX; ++column) {
				newPoint[(EDimension)row] += multiplier1[row][column] * multiplier2[(EDimension)column];
			}

			newPoint[(EDimension)row] += multiplier1[row][column];
		}

		for (column = X_INDEX; column <= Z_INDEX; ++column) {
			h += multiplier1[row][column] * multiplier2[(EDimension)column];
		}

		h += multiplier1[row][column];

		(h == 0) ? 0 : newPoint.x /= h;
		(h == 0) ? 0 : newPoint.y /= h;
		(h == 0) ? 0 : newPoint.z /= h;

		return newPoint;
	}
};

template<class Type>
class U32Point3D;

template<class Type>
class U32Point2D : public U32Construct2D<Type> {
public:
	U32Point2D() : U32Construct2D<Type>() {};
	U32Point2D(Type xx, Type yy) : U32Construct2D<Type>(xx, yy) {};
	U32Point2D(const U32Construct2D<Type> &other) : U32Construct2D<Type>(other.x, other.y) {};
	U32Point2D(const U32Construct3D<Type> &other) : U32Construct2D<Type>(other.x, other.y) {}; // For 3D

	U32Point2D<Type> operator=(const U32Construct3D<Type> &other) {
		this.x = other.x;
		this.y = other.y;
		return *this;
	}

	U32Construct2D<Type> operator-(const U32Construct3D<Type> &other) const {
		U32Construct2D<Type> newPoint;

		newPoint.x = this.x - other.x;
		newPoint.y = this.y - other.y;

		return newPoint;
	}

	U32Construct2D<Type> operator-(const U32Construct2D<Type> &other) const {
		U32Construct2D<Type> newPoint;

		newPoint.x = this.x - other.x;
		newPoint.y = this.y - other.y;

		return newPoint;
	}
};

typedef U32Point2D<int> U32IntPoint2D;
typedef U32Point2D<float> U32FltPoint2D;

template<class Type>
class U32Point3D : public U32Construct3D<Type> {
public:
	U32Point3D() : U32Construct3D<Type>() {};
	U32Point3D(Type xIn, Type yIn, Type zIn) : U32Construct3D<Type>(xIn, yIn, zIn) {};
	U32Point3D(const U32Construct3D<Type> &other) : U32Construct3D<Type>(other.x, other.y, other.z) {};

	bool operator==(const U32Construct2D<Type> &other) const {
		return ((other.x == this.x) && (other.y == this.y));
	}

	bool operator==(const U32Construct3D<Type> &other) const {
		return ((other.x == this.x) && (other.y == this.y) && (other.z == this.z));
	}

	U32Construct3D<Type> operator-(const U32Construct2D<Type> &other) const {
		U32Construct3D<Type> newPoint;

		newPoint.x = this.x - other.x;
		newPoint.y = this.y - other.y;
		newPoint.z = this.z;

		return newPoint;
	}

	U32Construct3D<Type> operator-(const U32Construct3D<Type> &other) const {
		U32Construct3D<Type> newPoint;

		newPoint.x = this.x - other.x;
		newPoint.y = this.y - other.y;
		newPoint.z = this.z - other.z;

		return newPoint;
	}
};

typedef U32Point3D<int> U32IntPoint3D;
typedef U32Point3D<float> U32FltPoint3D;

template<class Type>
class U32Vector2D : public U32Construct2D<Type> {
public:
	U32Vector2D() : U32Construct2D<Type>() {};
	U32Vector2D(Type xx, Type yy) : U32Construct2D<Type>(xx, yy) {};
	U32Vector2D(const U32Construct2D<Type> &other) : U32Construct2D<Type>(other.x, other.y) {};
	U32Vector2D(const U32Construct3D<Type> &other) : U32Construct2D<Type>(other.x, other.y) {};

	U32Vector2D<Type> Normalize() const {
		Type magnitude = this.Magnitude();
		assert2(magnitude > 0, "Tried to normalize a zeroed vector.");

		U32Vector2D<Type> newVector;
		newVector.x = this.x / magnitude;
		newVector.y = this.y / magnitude;

		return newVector;
	}

	U32Vector2D<Type> operator*(Type multiplier) const {
		U32Vector2D<Type> newVector;

		newVector.x = multiplier * this.x;
		newVector.y = multiplier * this.y;

		return newVector;
	}

	Type operator*(const U32Vector2D<Type> &multiplier) const {
		return (this.x * multiplier.x + this.y * multiplier.y);
	}

	const U32Vector2D<Type> &operator*=(Type multiplier) {
		*this = *this * multiplier;
		return *this;
	}

	const U32Vector2D<Type> &Rotate(ERevDirection whichDirection, double radians) {
		U32Point2D<Type> newPoint;

		if (whichDirection == kCounterClockwise) {
			newPoint.x = this.x * cos(radians) - this.y * sin(radians);
			newPoint.y = this.x * sin(radians) + this.y * cos(radians);
		} else {
			newPoint.x = this.x * cos(radians) + this.y * sin(radians);
			newPoint.y = this.y * cos(radians) - this.x * sin(radians);
		}

		this.x = newPoint.x;
		this.y = newPoint.y;
		return *this;
	}

	U32Construct3D<Type> Cross(const U32Vector2D<Type> &otherVector) const {
		U32Construct3D<Type> newVector;

		newVector.x = 0;
		newVector.y = 0;
		newVector.z = this.x * otherVector.y - this.y * otherVector.x;

		return newVector;
	}

	U32Vector2D<Type> Transpose(void) const {
		U32Vector2D<Type> newVector;
		newVector.x = -this.y;
		newVector.y = this.x;

		return newVector;
	}

	Type Distance2(const U32Vector2D<Type> &otherVector) const {
		return ((this.x - otherVector.x) * (this.x - otherVector.x) +
				(this.y - otherVector.y) * (this.y - otherVector.y));
	}

	Type Distance(const U32Vector2D<Type> &otherVector) const {
		return (Type)(sqrt((double)(Distance2(otherVector))));
	}

	ERevDirection GetRevDirection(const U32Vector2D<Type> &otherVector) const {
		U32Construct3D<Type> vector3 = this->Cross(otherVector);

		if (vector3.z > 0) {
			return kCounterClockwise;
		} else if (vector3.z < 0) {
			return kClockwise;
		} else {
			return kNone;
		}
	}

	// Project this vector onto otherVector, and return the resulting vector's magnitude.
	Type ProjectScalar(const U32Vector2D<Type> &otherVector) const {
		Type projectionScalar;

		if (otherVector.Magnitude() == 0) {
			projectionScalar = 0;
		} else {
			float otherMagnitude = otherVector.Magnitude();
			projectionScalar = (*this * otherVector) / otherMagnitude;
		}

		return projectionScalar;
	}
};

typedef U32Vector2D<int> U32IntVector2D;
typedef U32Vector2D<float> U32FltVector2D;

template<class Type>
class U32Vector3D : public U32Construct3D<Type> {
public:
	U32Vector3D() : U32Construct3D<Type>() {};
	U32Vector3D(Type xx, Type yy, Type zz) : U32Construct3D<Type>(xx, yy, zz) {};
	U32Vector3D<Type>(const U32Construct2D<Type> &other) : U32Construct3D<Type>(other.x, other.y, 0) {};
	U32Vector3D<Type>(const U32Construct3D<Type> &other) : U32Construct3D<Type>(other.x, other.y, other.z) {};

	U32Vector3D<Type> operator-(const U32Construct2D<Type> &other) const {
		U32Vector3D<Type> newVector;

		newVector.x = this.x - other.x;
		newVector.y = this.y - other.y;
		newVector.z = this.z;

		return newVector;
	}

	U32Vector3D<Type> operator-(const U32Construct3D<Type> &other) const {
		U32Vector3D<Type> newVector;

		newVector.x = this.x - other.x;
		newVector.y = this.y - other.y;
		newVector.z = this.z - other.z;

		return newVector;
	}

	Type operator*(const U32Construct3D<Type> &multiplier) const {
		return (this.x * multiplier.x + this.y * multiplier.y + this.z * multiplier.z);
	}

	U32Vector3D<Type> &operator+=(const U32Construct3D<Type> &adder) const {
		*this = *this + adder;
		return *this;
	}

	U32Vector3D<Type> &operator-=(const U32Construct3D<Type> &subtractor) {
		*this = *this - subtractor;
		return *this;
	}

	U32Vector3D<Type> operator*(Type multiplier) const {
		U32Vector3D<Type> newVector;

		newVector.x = multiplier * this.x;
		newVector.y = multiplier * this.y;
		newVector.z = multiplier * this.z;

		return newVector;
	}

	const U32Vector3D<Type> &operator*=(Type multiplier) {
		*this = *this * multiplier;
		return *this;
	}

	U32Vector3D<Type> operator/(Type divider) const {
		U32Vector3D<Type> newVector;

		newVector.x = (divider == 0) ? 0 : this.x / divider;
		newVector.y = (divider == 0) ? 0 : this.y / divider;
		newVector.z = (divider == 0) ? 0 : this.z / divider;

		return newVector;
	}

	const U32Vector3D<Type> &operator/=(Type multiplier) {
		*this = *this / multiplier;
		return *this;
	}

	U32Vector3D<Type> Cross(const U32Vector3D<Type> &otherVector) const {
		U32Vector3D<Type> newVector;

		newVector.x = this.y * otherVector.z - this.z * otherVector.y;
		newVector.y = this.z * otherVector.x - this.x * otherVector.z;
		newVector.z = this.x * otherVector.y - this.y * otherVector.x;

		return newVector;
	}

	bool operator>(const U32Vector3D<Type> &otherVector) const {
		return (this.Magnitude() > otherVector.Magnitude());
	}

	bool operator<(const U32Vector3D<Type> &otherVector) const {
		return (this.Magnitude() < otherVector.Magnitude());
	}

	// Project this vector onto otherVector, and return the resulting vector.
	U32Vector3D<Type> ProjectVector(const U32Vector3D<Type> &otherVector) const {
		Type projectionScalar;

		if (otherVector.Magnitude() == 0)
			projectionScalar = 0;
		else
			projectionScalar = (*this * otherVector) / (otherVector.Magnitude() * otherVector.Magnitude());

		U32Vector3D<Type> newVector = otherVector * projectionScalar;
		return newVector;
	}

	// Project this vector onto otherVector, and return the resulting vector's magnitude.
	Type ProjectScalar(const U32Vector3D<Type> &otherVector) const {
		Type projectionScalar;

		if (otherVector.Magnitude() == 0) {
			projectionScalar = 0;
		} else {
			float otherMagnitude = otherVector.Magnitude();
			projectionScalar = (*this * otherVector) / otherMagnitude;
		}

		return projectionScalar;
	}

	U32Vector3D<Type> Normalize() const {
		Type magnitude = this.Magnitude();
		assert2(magnitude > 0, "Tried to normalize a zeroed vector.");

		U32Vector3D<Type> newVector;

		if (magnitude != 0) {
			newVector.x = this.x / magnitude;
			newVector.y = this.y / magnitude;
			newVector.z = this.z / magnitude;
		}

		return newVector;
	}
};

typedef U32Vector3D<int> U32IntVector3D;
typedef U32Vector3D<float> U32FltVector3D;

class U32Distance3D : public U32Construct3D<float> {
public:
	U32Distance3D() : U32Construct3D<float>() {};
	U32Distance3D(const U32Construct2D<float> &other) : U32Construct3D<float>(other.x, other.y, 0) {};
	U32Distance3D(const U32Construct3D<float> &other) : U32Construct3D<float>(other.x, other.y, other.z) {};

	U32Distance3D operator-(int subtractor) const {
		U32Distance3D newDistance;

		if (Magnitude() != 0) {
			newDistance.x = x - ((x * subtractor) / Magnitude());
			newDistance.y = y - ((y * subtractor) / Magnitude());
			newDistance.z = z - ((z * subtractor) / Magnitude());
		}

		return newDistance;
	}

	U32Distance3D operator-=(int subtractor) {
		*this = *this - subtractor;
		return *this;
	}
};

struct U32Circle {

	U32FltPoint2D m_center;
	float m_radius;
};

struct U32Sphere {

	U32FltPoint3D m_center;
	float m_radius;
};

struct U32Cylinder : public U32Sphere {
	float m_height;
};

struct U32BoundingBox {

	U32IntPoint3D m_minPoint;
	U32IntPoint3D m_maxPoint;

	void SetMinPoint(int x, int y) {
		m_minPoint.x = x;
		m_minPoint.y = y;
	}

	void SetMaxPoint(int x, int y) {
		m_maxPoint.x = x;
		m_maxPoint.y = y;
	}

	const U32IntPoint3D &GetMinPoint(void) const {
		return m_minPoint;
	}

	const U32IntPoint3D &GetMaxPoint(void) const {
		return m_maxPoint;
	}

	bool Intersect(const U32BoundingBox &targetBox) const {
		return ((((m_minPoint.x <= targetBox.m_minPoint.x) &&
				  (m_maxPoint.x >= targetBox.m_minPoint.x)) ||

				 ((m_minPoint.x <= targetBox.m_maxPoint.x) &&
				  (m_maxPoint.x >= targetBox.m_maxPoint.x)) ||

				 ((targetBox.m_minPoint.x <= m_minPoint.x) &&
				  (targetBox.m_maxPoint.x >= m_minPoint.x)) ||

				 ((targetBox.m_minPoint.x <= m_maxPoint.x) &&
				  (targetBox.m_maxPoint.x >= m_maxPoint.x)))

				&&

				(((m_minPoint.y <= targetBox.m_minPoint.y) &&
				  (m_maxPoint.y >= targetBox.m_minPoint.y)) ||

				 ((m_minPoint.y <= targetBox.m_maxPoint.y) &&
				  (m_maxPoint.y >= targetBox.m_maxPoint.y)) ||

				 ((targetBox.m_minPoint.y <= m_minPoint.y) &&
				  (targetBox.m_maxPoint.y >= m_minPoint.y)) ||

				 ((targetBox.m_minPoint.y <= m_maxPoint.y) &&
				  (targetBox.m_maxPoint.y >= m_maxPoint.y))));
	}

	const U32IntPoint3D &operator[](int point) const {
		assert(point <= 1);
		return ((point == 0) ? m_minPoint : m_maxPoint);
	}

	U32IntPoint3D &operator[](int point) {
		assert(point <= 1);
		return ((point == 0) ? m_minPoint : m_maxPoint);
	}

	bool IsPointWithin(U32FltPoint2D point) const {
		return ((m_minPoint.x <= point.x) && (point.x <= m_maxPoint.x) &&
				(m_minPoint.y <= point.y) && (point.y <= m_maxPoint.y));
	}
};

struct U32Plane {
	U32FltPoint3D m_point;
	U32FltVector3D m_normal;
	float m_collisionEfficiency;
	float m_friction;

	// Average this plane with another plane.
	U32Plane &Average(const U32Plane &otherPlane) {
		m_point.x = (m_point.x + otherPlane.m_point.x) / 2;
		m_point.y = (m_point.y + otherPlane.m_point.y) / 2;
		m_point.z = (m_point.z + otherPlane.m_point.z) / 2;

		m_normal.x = (m_normal.x + otherPlane.m_normal.x) / 2;
		m_normal.y = (m_normal.y + otherPlane.m_normal.y) / 2;
		m_normal.z = (m_normal.z + otherPlane.m_normal.z) / 2;

		float normalMag = m_normal.Magnitude();
		m_normal.x = (normalMag == 0) ? 0 : m_normal.x / normalMag;
		m_normal.y = (normalMag == 0) ? 0 : m_normal.y / normalMag;
		m_normal.z = (normalMag == 0) ? 0 : m_normal.z / normalMag;

		m_collisionEfficiency = (m_collisionEfficiency + otherPlane.m_collisionEfficiency) / 2;
		m_friction = (m_friction + otherPlane.m_friction) / 2;

		return *this;
	}
};

class U32Ray2D {
public:
	U32FltPoint2D m_origin;
	U32FltVector2D m_direction;

	bool Intersection(const U32Ray2D &otherRay, U32FltPoint2D *intersection) const {
		float numerator = ((otherRay.m_origin - m_origin) * otherRay.m_direction.Transpose());
		float denominator = m_direction * otherRay.m_direction.Transpose();

		if ((denominator == 0) ||
			(denominator < 0) != (numerator < 0)) {
			return false;
		} else {
			float s = numerator / denominator;
			assert(s >= 0);

			intersection->x = m_origin.x + (m_direction.x * s);
			intersection->y = m_origin.y + (m_direction.y * s);
			return true;
		}
	}

	bool NearIntersection(const U32Circle &circle, U32FltPoint2D *pIntersection) const {
		U32FltVector2D coVector = m_origin - circle.m_center;
		double b = m_direction.Normalize() * coVector;
		double c = (coVector * coVector) - (circle.m_radius * circle.m_radius);
		double d = (b * b) - c;

		if (d < 0) {
			return false;
		} else {
			double t = -b - sqrt(d);

			if (t < 0) {
				return false;
			} else {
				*pIntersection = m_origin + (m_direction.Normalize() * t);
				return true;
			}
		}
	}

	bool FarIntersection(const U32Circle &circle, U32FltPoint2D *pIntersection) const {
		U32FltVector2D coVector = m_origin - circle.m_center;
		double b = m_direction.Normalize() * coVector;
		double c = (coVector * coVector) - (circle.m_radius * circle.m_radius);
		double d = (b * b) - c;

		if (d < 0) {
			return false;
		} else {
			double t = -b + sqrt(d);

			if (t < 0) {
				return false;
			} else {
				*pIntersection = m_origin + (m_direction.Normalize() * t);
				return true;
			}
		}
	}
};

class U32Ray3D {
public:
	U32FltPoint3D m_origin;
	U32FltVector3D m_direction;

	/*
	// Find the point where a ray intersects a sphere closest to the ray's origin.
	float FindSphereIntersection (const U32Sphere& sphere) const
	{
		float b,c;
		float result1, result2;

		b = m_direction * (m_origin - sphere.m_center);
		c = (m_origin - sphere.m_center) * (m_origin - sphere.m_center) - (sphere.m_radius * sphere.m_radius);

		assert2( (b * b) >= c, "Tried tp use sqrt on a negative number");

		result1 = -b - sqrt(b * b - c);
		result2 = -b + sqrt(b * b - c);

		if ((0 < result1) && (result1 <= result2))
		{
			return result1;
		}
		else if ((0 < result2) && (result2 <= result1))
		{
			return result2;
		}
		else
		{
			return 0;
		}
	}
	*/
};

const float FLOAT_EPSILON = 0.001f;

inline int Sqr(int x) { return x * x; }

class Line2D {
public:
	Line2D(float x_coef, float y_coef, float constant) {
		A = x_coef;
		B = y_coef;
		C = constant;
	}

	Line2D(const U32FltVector2D &point1, const U32FltVector2D &point2) {
		LineFromTwoPoints(point1, point2);
	}

	void LineFromTwoPoints(U32FltVector2D pt1, U32FltVector2D pt2) {
		float temp = (pt2.x - pt1.x);

		if (fabs(temp) < FLOAT_EPSILON) {
			assert(((fabs(pt2.y - pt1.y) >= FLOAT_EPSILON)));
			A = 1;
			B = 0;
		} else {
			float m = (pt2.y - pt1.y) / temp;
			A = -m;
			B = 1;
		}

		C = -(A * pt2.x + B * pt2.y);
	}

	inline float Distance(U32FltVector2D point) {
		return fabs((A * point.x + B * point.y + C) / sqrt(Sqr(A) + Sqr(B)));
	}

	inline float Distance2(U32FltVector2D point) {
		return fabs(Sqr(A * point.x + B * point.y + C) / (Sqr(A) + Sqr(B)));
	}

	inline float Angle() {
		return atan2(-A, B);
	}

	bool InBetween(U32FltVector2D point, U32FltVector2D end1, U32FltVector2D end2) {
		assert((!OnLine(end1) || !OnLine(end2)));

		point = ProjectPoint(point);
		float Distance2 = end1.Distance2(end2);

		return (point.Distance2(end1) <= Distance2 && point.Distance2(end2) <= Distance2) ? true : false;
	}

	bool OnLine(U32FltVector2D point) {
		return (Distance2(point) < 1.0f) ? true : false;
	}

	U32FltVector2D ProjectPoint(U32FltVector2D point) {
		return intersection(perpendicular(point));
	}

	float get_y(float x) {
		if (B != 0)
			return (-A * x - C) / B;

		return 0;
	}

	float get_x(float y) {
		if (A != 0)
			return (-B * y - C) / A;

		return 0;
	}

	U32FltVector2D intersection(Line2D line) {
		U32FltVector2D result(0, 0);

		assert(!SameSlope(line));

		if (B == 0) {
			result.x = -C / A;
			result.y = line.get_y(result.x);
			return result;
		}

		if (line.B == 0) {
			result.x = -line.C / line.A;
			result.y = get_y(result.y);
			return result;
		}

		result.x = (C * line.B - B * line.C) / (line.A * B - A * line.B);
		result.y = get_y(result.x);
		return result;
	}

	Line2D perpendicular(U32FltVector2D point) {
		return Line2D(B, -A, A * point.y - B * point.x);
	}

	Line2D shift_y(float val) {
		return Line2D(A, B, C - val * B);
	}

	Line2D shift_x(float val) {
		return Line2D(A, B, C - val * A);
	}

	/* returns whether the projection of point1 is closer to targ_point than the
	 projection of point2 */
	bool IsPointCloserToPointOnLine(U32FltVector2D point1, U32FltVector2D point2, U32FltVector2D targ_point) {

		assert(!OnLine(targ_point));

		point1 = ProjectPoint(point1);
		point2 = ProjectPoint(point2);

		return (point1.Distance(targ_point) < point2.Distance(targ_point)) ? true : false;
	}

	bool HalfPlaneTest(U32FltVector2D point) {
		if (B == 0)
			return (point.x < -C / A) ? true : false;

		return (point.y > get_y(point.x)) ? true : false;
	}

	bool SameSlope(Line2D line) {
		return (B == 0 && line.B == 0 || A / B == line.A / line.B) ? true : false;
	}

	// private:
	float A, B, C; /* the three coeffs in the line equation */
				   /* Ax + By + C = 0 */
};

} // End of namespace Scumm

#endif // ENABLE_HE

#endif // SCUMM_HE_BASKETBALL_GEOTYPES_H
