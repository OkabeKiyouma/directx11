#ifndef _KKMATH
#define _KKMATH

#include <float.h>
#include <math.h>

constexpr float PI = 3.1415926535f;
constexpr float PI_INVERSE = 1.0f / PI;
constexpr float TAU = PI / 2;
constexpr float REAL_EPSILON = FLT_EPSILON;
constexpr float REAL_MAX = FLT_MAX;
constexpr float REAL_MIN = FLT_MIN;


union Vec1 {
	struct {
		float x;
	};
	float m[1];
	struct {
		float u;
	};
	Vec1() {
	}
	Vec1(float a) : x(a){
	}
};

union Vec2 {
	struct {
		float x, y;
	};
	struct {
		float u, v;
	};
	float m[2];
	Vec2() {
	}
	Vec2(float a) : x(a), y(a) {
	}
	Vec2(float a, float b) : x(a), y(b) {
	}
};

union Vec3 {
	struct {
		float x, y, z;
	};
	struct {
		float u, v, w;
	};
	struct {
		float u, v, q;
	};
	struct {
		float r, g, b;
	};
	struct {
		Vec2  xy;
		float _z;
	};
	struct {
		float _x;
		Vec2  yz;
	};
	float m[3];
	Vec3() {
	}
	Vec3(float a) : x(a), y(a), z(a) {
	}
	Vec3(float a, float b, float c) : x(a), y(b), z(c) {
	}
	Vec3(Vec2 ab, float c) : xy(ab), _z(c) {
	}
	Vec3(float a, Vec2 bc) : _x(a), yz(bc) {
	}
};

union Vec4 {
	struct {
		float x, y, z, w;
	};
	struct {
		float u, v, w, q;
	};
	struct {
		float r, g, b, a;
	};
	struct {
		Vec2 xy, zw;
	};
	struct {
		Vec3  xyz;
		float _w;
	};
	struct {
		float _x;
		Vec3  yzw;
	};
	float m[4];
	Vec4() {
	}
	Vec4(float a) : x(a), y(a), z(a), w(a) {
	}
	Vec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {
	}
	Vec4(Vec2 ab, Vec2 cd) : xy(ab), zw(cd) {
	}
	Vec4(Vec3 abc, float d) : xyz(abc), _w(d) {
	}
	Vec4(float a, Vec3 bcd) : _x(a), yzw(bcd) {
	}
};

union Mat2 {
	Vec2  rows[2];
	float m[4];
	float m2[2][2];
	inline Mat2() {
	}
	inline Mat2(Vec2 a, Vec2 b) : rows{ a, b } {
	}
};

union Mat3 {
	Vec3  rows[3];
	float m[9];
	float m2[3][3];
	inline Mat3() {
	}
	inline Mat3(Vec3 a, Vec3 b, Vec3 c) : rows{ a, b, c } {
	}
};

union Mat4 {
	Vec4  rows[4];
	float m[16];
	float m2[4][4];
	inline Mat4() {
	}
	inline Mat4(Vec4 a, Vec4 b, Vec4 c, Vec4 d) : rows{ a, b, c, d } {
	}
};

union Quat {
	float m[4];
	struct {
		float x, y, z, w;
	};
	struct {
		float i, j, k, real;
	};
	struct {
		Vec4 v4;
	};
	Quat() {
	}
	Quat(Vec4 v) : v4(v) {
	}
	Quat(float b, float c, float d, float a) {
		m[0] = b;
		m[1] = c;
		m[2] = d;
		m[3] = a;
	}
};
#endif