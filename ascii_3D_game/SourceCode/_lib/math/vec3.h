#pragma once

#pragma once

#include <math.h>
#include <stdbool.h>
#include <float.h>

//!なんか移植性が悪いかなんかで警告出てるから一旦消したい/
//!まあ気が向いたらc11に変更するかしたほうがいいんかな　知らんけど　とりあえず古いコンパイラでは使えないmsvc特有の機能なんかな/
//?msvc自体cの古い規格を使ってるけどそれとmsvc独自の+αがあってたぶんその+αの部分がこれやったから普通の古いcでコンパイルするとコンパイル通らないよってエラーかな/
//(エラー内容:C4021 非標準の拡張機能が使用されています: 無名の構造体または共用体です。)/

//まず今の状態を保存/
#pragma warning(push)
//警告を消す/
#pragma warning(disable : 4201)

//angleはyがz てかzにしとけばよかったかな/
typedef union vec2{
	struct{
		float x, y;
	};
	float a[2];
} vec2;
typedef union vec3{
	struct{
		float x, y, z;
	};
	float a[3];
} vec3;

typedef union ivec3{
	struct{
		int x, y, z;
	};
	int a[3];
}ivec3;

//保存した警告を戻す/
#pragma warning(pop)

typedef struct Basis{
	vec3 x, y, z;
}Basis;

typedef struct vec2Basis{
	vec2 x, y;
}vec2Basis;

//キャストするときに使う/
#define FtoINT(x) ((int)floorf(x))

#define PI 3.14159265f

#define v3zero (vec3){ 0.0f, 0.0f, 0.0f }
#define v3one  (vec3){ 1.0f, 1.0f, 1.0f }
#define v3max  (vec3){ FLT_MAX,FLT_MAX,FLT_MAX }
#define v3min  (vec3){ FLT_MIN,FLT_MIN,FLT_MIN }
#define v3x    (vec3){ 1.0f, 0.0f, 0.0f }
#define v3y    (vec3){ 0.0f, 1.0f, 0.0f }
#define v3z    (vec3){ 0.0f, 0.0f, 1.0f }
#define v3xm    (vec3){ -1.0f, 0.0f, 0.0f }
#define v3ym    (vec3){ 0.0f, -1.0f, 0.0f }
#define v3zm    (vec3){ 0.0f, 0.0f, -1.0f }
#define iv3x    (ivec3){ 1, 0, 0 }
#define iv3y    (ivec3){ 0, 1, 0 }
#define iv3z    (ivec3){ 0, 0, 1 }
#define iv3xm    (ivec3){ -1, 0, 0 }
#define iv3ym    (ivec3){ 0, -1, 0 }
#define iv3zm    (ivec3){ 0, 0, -1 }
#define v2x (vec2){1.f,0.f}
#define v2y (vec2){0.f,1.f}
#define iv3zero    (ivec3){ 0, 0, 0 }
#define basisZ (Basis){v3x,	v3y, v3z}
#define vec2basisY (vec2Basis){v2x,v2y}

static __forceinline vec3 v3make(float x, float y, float z){ vec3 v = { x, y, z }; return v; }
static __forceinline vec3 fltToV3(float x){ return v3make(x, x, x); }
static __forceinline ivec3 iv3make(int x, int y, int z){ ivec3 v = { x, y, z }; return v; }

static __forceinline vec3 v3make1(float a){ vec3 v = { a,a,a }; return v; }
static __forceinline ivec3 vec3toIvec3(vec3 a){ return iv3make(FtoINT(a.x), FtoINT(a.y), FtoINT(a.z)); }
static __forceinline vec3 v3floor(vec3 a){ return v3make(floorf(a.x), floorf(a.y), floorf(a.z)); }
static __forceinline vec3 v3abs(vec3 a){ return v3make(fabsf(a.x), fabsf(a.y), fabsf(a.z)); }

static __forceinline vec3 v3add(vec3 a, vec3 b){ return v3make(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vec3 v3add1(vec3 a, float b){ return v3make(a.x + b, a.y + b, a.z + b); }
static __forceinline vec3 v3sub(vec3 a, vec3 b){ return v3make(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vec3 v3mul(vec3 v, float s){ return v3make(v.x * s, v.y * s, v.z * s); }
static __forceinline vec3 v3mulv(vec3 a, vec3 b){ return v3make(a.x * b.x, a.y * b.y, a.z * b.z); }
static __forceinline vec3 v3div(vec3 v, float s){ return v3make(v.x / s, v.y / s, v.z / s); }
static __forceinline vec3 v3divv(vec3 a, vec3 b){ return v3make(a.x / b.x, a.y / b.y, a.z / b.z); }

static __forceinline vec3 v3getMin(vec3 a, vec3 b){ return v3make(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
static __forceinline float v3min1(vec3 a){ return a.x < a.y && a.x < a.z ? (a.x) : (a.y < a.z ? a.y : a.z); }
static __forceinline int v3min1id(vec3 a){ return a.x < a.y && a.x < a.z ? (0) : (a.y < a.z ? 1 : 2); }//x0 y1 z2 vec3
static __forceinline vec3 v3getMax(vec3 a, vec3 b){ return v3make(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }
static __forceinline float v3max1(vec3 a){
	return a.x > a.y && a.x > a.z ? (a.x) : (a.y > a.z ? a.y : a.z);
}
static __forceinline vec3 v3clamp(vec3 v, vec3 lo, vec3 hi){
	return v3make(
		v.x < lo.x ? lo.x : (v.x > hi.x ? hi.x : v.x),
		v.y < lo.y ? lo.y : (v.y > hi.y ? hi.y : v.y),
		v.z < lo.z ? lo.z : (v.z > hi.z ? hi.z : v.z)
	);
}

static __forceinline float v3dot(vec3 a, vec3 b){ return a.x * b.x + a.y * b.y + a.z * b.z; }
static __forceinline vec3 v3cross(vec3 a, vec3 b){
	return v3make(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}
static __forceinline float v3lensq(vec3 v){ return v3dot(v, v); }
static __forceinline float v3len(vec3 v){ return sqrtf(v3lensq(v)); }

static __forceinline float v3crossLen(vec3 a, vec3 b){
	return v3len(v3cross(a, b));
}

static __forceinline vec3 v3normalize(vec3 v){
	float l = v3len(v);
	if(l > 1e-8f) return v3div(v, l);
	return v3zero;
}

static __forceinline vec3 v3lerp(vec3 a, vec3 b, float t){
	return v3make(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

static __forceinline float v3distance(vec3 a, vec3 b){ return v3len(v3sub(a, b)); }

static __forceinline bool v3equals_eps(vec3 a, vec3 b, float eps){
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return (dx * dx + dy * dy + dz * dz) <= (eps * eps);
}
static __forceinline bool v3equals(vec3 a, vec3 b){ return v3equals_eps(a, b, 1e-6f); }

static __forceinline bool v3iszero(vec3 v){ return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f; }

static __forceinline vec3 ivec3toVec3(ivec3 a){ return v3make((float)a.x, (float)a.y, (float)a.z); }

static __forceinline ivec3 iv3add(ivec3 a, ivec3 b){ return iv3make(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline int iv3sum(ivec3 a){ return (a.x + a.y + a.z); }
static __forceinline ivec3 iv3sub(ivec3 a, ivec3 b){ return iv3make(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline ivec3 iv3mul(ivec3 v, int s){ return iv3make(v.x * s, v.y * s, v.z * s); }
static __forceinline ivec3 iv3div(ivec3 v, int s){ return iv3make(v.x / s, v.y / s, v.z / s); }//s>0
static __forceinline ivec3 iv3mulv(ivec3 a, ivec3 b){ return iv3make(a.x * b.x, a.y * b.y, a.z * b.z); }

static __forceinline int iv3min1(ivec3 a){ return a.x < a.y && a.x < a.z ? (a.x) : (a.y < a.z ? a.y : a.z); }
static __forceinline int iv3min1id(ivec3 a){ return a.x < a.y && a.x < a.z ? (0) : (a.y < a.z ? 1 : 2); }//x0 y1 z2 ivec3

//ベクトルの法線のうち上と右のベクトルをもらうやつ/
static __forceinline void v3basis(vec3 angle, vec3* right, vec3* up){
	//向いてる方向の真横(右)のvec
	*right = v3normalize(v3cross(v3y, angle));//angleと真上の外積/
	//向いてる方向の法線(上)vec
	*up = v3normalize(v3cross(angle, *right));
}
static __forceinline void v3basisAll(vec3 angle, vec3* right, vec3* up){
	// 分岐を排除するためのマジックナンバー（-1.0fに近い値との判定を滑らかにするため）/
	float a = 1.0f / (1.0f + angle.z);
	float b = -angle.x * angle.y * a;

	// 右方向ベクトルを数式のみで計算/
	right->x = 1.0f - angle.x * angle.x * a;
	right->y = b;
	right->z = -angle.x;

	// 上方向ベクトルを数式のみで計算/
	up->x = b;
	up->y = 1.0f - angle.y * angle.y * a;
	up->z = -angle.y;
}

static __forceinline vec2 v2add(vec2 a, vec2 b){ return (vec2){ a.x + b.x, a.y + b.y }; }
static __forceinline vec2 v2sub(vec2 a, vec2 b){ return (vec2){ a.x - b.x, a.y - b.y }; }
static __forceinline vec2 v2mul(vec2 v, float t){ return (vec2){ v.x* t, v.y* t }; }

static __forceinline vec2 v2right(vec2 v){ return (vec2){ v.y, -v.x }; }

static __forceinline float v2det(vec2 a, vec2 b){ return a.x * b.y - b.x * a.y; }

static __forceinline float v2dot(vec2 a, vec2 b){
	return a.x * b.x + a.y * b.y;
}
static __forceinline float v2lenSq(vec2 v){
	return v2dot(v, v);
}
static __forceinline float v2len(vec2 v){
	return sqrtf(v2lenSq(v));
}
static __forceinline vec2 v2normalize(vec2 v){
	float inv = 1.f / v2len(v);
	return v2mul(v, inv);
}

//正規化あり/
//yだけ消す/
static __forceinline vec2 angleToVec2(vec3 angle){
	vec3 noY = v3normalize((vec3){ angle.x, 0.f, angle.z });
	return (vec2){ noY.x, noY.z };
}
static __forceinline vec2Basis createVec2Basis(vec2 angle){
	return (vec2Basis){ v2right(angle), angle };
}
static __forceinline Basis vec2BasisToBasis(vec2Basis b){
	return (Basis){
		(vec3){
		b.x.x, 0.f, b.x.y
	}, v3y, (vec3){ b.y.x, 0.f, b.y.y }
	};
}

//angleは正規化済み前提 呼び出し元がやれ　y方向に向いてたらバグるからy方向に向かんようにしろ/
static __forceinline Basis createBasis(vec3 angle){
	Basis r;
	r.x = v3normalize(v3cross(v3y, angle));
	r.y = v3cross(angle, r.x);
	r.z = angle;
	return r;
}
static __forceinline Basis createBasisV2(vec2 angle){
	Basis r;
	r.z = (vec3){ angle.x,0.f,angle.y };
	r.x = v3cross(v3y, r.z);
	r.y = v3y;
	return r;
}

//とあるベクトルをbasisを基底ベクトルとしたベクトルにする
static __forceinline vec3 toLocalBasis(Basis b, vec3 v){
	return (vec3){
		v3dot(b.x, v),
			v3dot(b.y, v),
			v3dot(b.z, v)
	};
}
//とあるベクトルをbasisを基底ベクトルとしたベクトルにする
static __forceinline vec3 toGlobalBasis(Basis b, vec3 v){
	return v3add(v3add(
		v3mul(b.x, v.x), v3mul(b.y, v.y)), v3mul(b.z, v.z)
	);
}
