#include"render3d.h"
#include<stdint.h>
#include"../screen/screen.h"
#include "../buffer/gameBuff.h"
#include "loader/textureLoader.h"
#include<intrin.h>

//なんとなくでやってみたけど結構早かった　simdってすごいんやな/
#define ENABLE_SIMD 1

//128 or 256 思ったよりそこまで速度は変わらない/
//128:600-800fps 256:600-900fps まあ大体このあたりかな(release 512x256px 800ポリゴン くらいの)/
#define SIMD_MODE 256
#include"render3d_simd.h"
#define ENABLE_ASSERT 0

//思ったより速度が落ちなかった/
//simdのほうは今んとこ対応してない　気が向いたらやるかな?/
#define uvmode_ 0
#define uvmode_fast 1
#define uvmode_exact 2
#define UV_MODE uvmode_exact

//このファイル限定/
#if ENABLE_ASSERT + 0
#define _ASSERT(x,msg) ASSERT(x,msg)
#else
#define _ASSERT(x,msg) do{}while(0)
#endif

#define ACTIVE_WIRE 0

#define ENABLE_UV

//なんかバグの5割はこいつをでかくしたら治る/
#define near 15.f
#define far 5000.f

//ワイヤーの最大描画距離/
#define WIRE_MAX_FAR 200.f
#define WIRE_MAX_INV_FAR (1.f/WIRE_MAX_FAR)
//明るさが半減する距離/
const float shadeLength = ((WIRE_MAX_FAR / 5.f) * 2.f);

//小さい方/
#define MIN(a,b) (((a)<(b))?(a):(b))
//でかい方/
#define MAX(a,b) (((a)>(b))?(a):(b))
//小さい方/
#define MIN3(a,b,c) (MIN(MIN(a,b),c))
//でかい方/
#define MAX3(a,b,c) (MAX(MAX(a,b),c))
//クランプ/
#define CLAMP(x,min,max) (MAX(MIN((max),(x)),(min)))

//---------------------------------------------
// private:
//---------------------------------------------

//test
vec2 testUV[3] = {
	{0.f,0.f},
	{3.f,0.f},
	{0.f,3.f},
};
//testEnd

pixel_t testTex_mario[16 * 16] = {
	0,0,0,0,0,8,8,8,8,8,0,0,0,0,0,0,
	0,0,0,0,8,8,8,8,8,8,8,8,8,8,0,0,
	0,0,0,0,1,1,1,4,4,1,4,0,0,0,0,0,
	0,0,0,1,4,1,4,4,4,1,4,4,4,0,0,0,
	0,0,0,1,4,1,1,4,4,0,1,4,4,4,0,0,
	0,0,0,1,1,4,4,4,4,1,1,1,1,0,0,0,
	0,0,0,0,0,4,4,4,4,4,4,4,0,0,0,0,
	0,0,0,0,1,1,8,1,1,1,0,0,0,0,0,0,
	0,0,0,1,1,1,8,1,1,8,1,1,1,0,0,0,
	0,0,1,1,1,1,8,8,8,8,1,1,1,1,0,0,
	0,0,4,4,1,8,1,8,8,1,8,1,4,4,0,0,
	0,0,4,4,4,8,8,8,8,8,8,4,4,4,0,0,
	0,0,4,4,8,8,8,8,8,8,8,8,4,4,0,0,
	0,0,0,0,8,8,8,0,0,8,8,8,0,0,0,0,
	0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,
	0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
};

#define RENDER_REQUEST_MAX 256

typedef struct FaceContext{
	float light;
	pixel_t* texture;
	int txSize;
	int shiftCount;
	int debug___;
	float shadeLength;
} FaceContext;

typedef struct RenderStack{
	int cnt;
	//pad 4B

	const Model3D* mdl[RENDER_REQUEST_MAX];
	vec3 p[RENDER_REQUEST_MAX];
	vec3 scale[RENDER_REQUEST_MAX];
	Basis angle[RENDER_REQUEST_MAX];
}RenderStack;
RenderStack rStack;

#define SRS_STACK_SIZE 4
typedef struct SRS_stack{//static render stack の stack
	int cnt;
	//pad 4B
	const StaticRenderStack* placedMdls[SRS_STACK_SIZE];
}SRS_stack;
SRS_stack srsStack;

//こいつが変わればsimd番も変わる　変わらないことを祈る...
static inline pixel_t getAsciiShade(vec2 uv, float invz, FaceContext* fCtx){
	//なんでかわからんけど全ピクセル割り算しても全然fps落ちないんやけどなんで? cpuが優秀なん 実は割り算ってそんな遅くない？/
	const int txsize = fCtx->txSize;
	const int txMask = txsize - 1;
	int u = (int)(txsize * (uv.x)) & txMask;
	int v = (int)(txsize * (uv.y)) & txMask;
	float c = (float)(fCtx->texture[u + (v * txsize)]);
	float lightFalloff = invz * shadeLength;
	lightFalloff = MIN(1.f, lightFalloff);//z<1のとき明るくなりすぎてasciiTableを超える/
	c *= fCtx->light;
	c *= lightFalloff;
	_ASSERT(c < 32, "明るすぎる");
	return asciiShade_0 + (pixel_t)c;
	//テスト用のチェックがら/
	//int u = (int)(64 * (uv.x));
	//int v = (int)(64 * (uv.y));
	//return asciiShade_0 + !!((u & 8) ^ (v & 8))*31;
}

static inline v_int getAsciiShade_simd(v_float v_u, v_float v_v, v_float v_invz, FaceContext* fCtx){
	_ASSERT(((fCtx->txSize) & (fCtx->txSize - 1)) == 0, "テクスチャが2の累乗じゃない");
	//光の減衰/
	v_float v_shadeLength = simd_set1_ps(fCtx->shadeLength);
	v_float _v_lightFalloff = simd_mul_ps(v_invz, v_shadeLength);
	v_float v_lightFalloff_max = simd_set1_ps(1.f);
	v_float v_mask = simd_cmplt_ps(_v_lightFalloff, v_lightFalloff_max);//((第一)<(第二))?-1:0
	v_float v_lightFalloff = simd_blendv_ps(v_lightFalloff_max/*false*/, _v_lightFalloff/*true*/, v_mask);
	v_int v_txsize_i = simd_set1_epi32(fCtx->txSize);

	//context/
	v_float v_light = simd_set1_ps(fCtx->light);
	v_float v_txsize = simd_set1_ps((float)fCtx->txSize);
	v_int v_txMask = simd_set1_epi32(fCtx->txSize - 1);//はみ出たときのマスク/

	//0-1から0-sizeに引き伸ばす　まあ比率から座標って感じ/
	v_float v_tex_x_f = simd_mul_ps(v_u, v_txsize);
	v_float v_tex_y_f = simd_mul_ps(v_v, v_txsize);

	//intにキャスト/
	v_int v_tex_x = simd_cvttps_epi32(v_tex_x_f);
	v_int v_tex_y = simd_cvttps_epi32(v_tex_y_f);

	//mask
	v_tex_x = simd_and_si(v_tex_x, v_txMask);
	v_tex_y = simd_and_si(v_tex_y, v_txMask);

	//テクスチャをもらう/
	v_int v_index = simd_add_epi32(v_tex_x, simd_mullo_epi32(v_tex_y, v_txsize_i));
	//v_int v_index = simd_add_epi32(v_tex_x, simd_slli_epi32(v_tex_y, fCtx->shiftCount));
	//なんかわからんけど1000fps出る用になってる　これをシフト演算にしたおかげ?それともvsを再起動したから？速いときは1100fps/

	v_int v_c = simd_i32gather_epi32((const int*)(fCtx->texture), v_index, 4);

	v_float v_c_f = simd_cvtepi32_ps(v_c);

	//↓pixel_tが1byteから4byte型になったからいらなくなった/

//	//1byte型だからとりあえず全要素ばらばらにもらう/
//	int index[SIMD_STEP];
//	simd_storeu_si((v_int*)index, v_index);
//
//	//テクスチャから取り出す　ギャザーってのがあるらしいけど1byte型でやってるから不可能 (らしい)/
//#if SIMD_MODE == 128
//	v_float v_tx_c = simd_set_ps(
//		(float)fCtx->texture[index[3]],
//		(float)fCtx->texture[index[2]],
//		(float)fCtx->texture[index[1]],
//		(float)fCtx->texture[index[0]]
//	);
//#elif SIMD_MODE == 256
//	v_float v_tx_c = simd_set_ps(
//		(float)fCtx->texture[index[7]],
//		(float)fCtx->texture[index[6]],
//		(float)fCtx->texture[index[5]],
//		(float)fCtx->texture[index[4]],
//		(float)fCtx->texture[index[3]],
//		(float)fCtx->texture[index[2]],
//		(float)fCtx->texture[index[1]],
//		(float)fCtx->texture[index[0]]
//	);
//#endif

	//光のやつ/
	v_c_f = simd_mul_ps(v_c_f, v_light);
	v_c_f = simd_mul_ps(v_c_f, v_lightFalloff);

	//int
	v_c = simd_cvttps_epi32(v_c_f);//32bitに戻す/

	//念のため/
#define ASCII_SHADE_0 0
	static_assert(asciiShade_0 == ASCII_SHADE_0, "asciiShade_0 != 0");
#if ASCII_SHADE_0
	v_int v_ascii_base = simd_set1_epi32((int)asciiShade_0);
	v_c_32 = simd_add_epi32(v_ascii_base, v_c);
#endif

	//debug
	//if(fCtx->debug___==0)v_c = simd_set1_epi32(asciiShade_1);
	//if(fCtx->debug___==1)v_c = simd_set1_epi32(asciiShade_16);
	//if(fCtx->debug___==2)v_c = simd_set1_epi32(asciiShade_31);

	return v_c;
}

//ドット書き込み/
static inline void putDot(Screen* sc, int x, int y, pixel_t l){
	_ASSERT((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT), "範囲外");
	sc->screen[x + (y * (WIDTH))] = l;
}

//画面外チェック + zbuff書き込み/
static inline void putDotZS(Screen* sc, int x, int y, float invz, pixel_t l){
	if((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT)){
		int p = x + (y * (WIDTH));
		if(sc->zbuff[p] < invz){
			sc->screen[p] = l;
			sc->zbuff[p] = invz;
		}
	}
}
static inline void putDotZ(Screen* sc, int x, int y, float invz, pixel_t l){
	_ASSERT((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT), "範囲外");
	int p = x + (y * (WIDTH));
	if(sc->zbuff[p] < invz){
		sc->screen[p] = l;
		sc->zbuff[p] = invz;
	}
}

static pixel_t getCharFromDelta(int dx, int dy){
	if(0 < dx * dy){//符号一緒/
		return a_luline;
	}
	if(dx * dy < 0){//符号違う/
		return a_ruline;
	}
	if(dx){//右/
		return a_hline;
	}
	if(dy){//左/
		return a_vline;
	}
	return a_sp;
}
// 画面の外側の領域を判定するためのビットマスク/
enum{
	bit_inside = 0, // 0000
	bit_left = 1, // 0001
	bit_right = 2, // 0010
	bit_bottom = 4, // 0100
	bit_top = 8, // 1000
	bit_far = 16, // 1000
};

// 座標が画面のどの領域にあるかを判定する関数/
static inline int computeOutCode(vec3 p){
	int code = bit_inside;
	if(p.x < 0.f)           code |= bit_left;
	else if(p.x >= (float)WIDTH)  code |= bit_right;
	if(p.y < 0.f)           code |= bit_top;
	else if(p.y >= (float)HEIGHT) code |= bit_bottom;
	if(p.z < 1.f / WIRE_MAX_FAR) code |= bit_far;
	return code;
}
static void _drawLine(Screen* sc, int x1, int y1, float invz1, int x2, int y2, float invz2){
	//sub
	int dx = x2 - x1;
	int dy = y2 - y1;
	//step
	int sx = (0 <= dx) - (dx < 0);
	int sy = (0 <= dy) - (dy < 0);
	//abs
	dx = abs(dx);
	dy = abs(dy);
	//常に進めないほうを進めるかの判断基準的な/
	int err = dx - dy;
	pixel_t l = a_plus;
	//z
	float dz = invz2 - invz1;

	if(dy <= dx){//緩やか 常にxを進める/
		if(dx == 0) return;
		float sz = dz / dx;
		while(1){
			putDotZS(sc, x1, y1, invz1, l);
			//z
			invz1 += sz;
			//x
			x1 += sx;
			err -= dy;

			//yを進めるか/
			int ystep = -(err < 0);// 0x00000000 or 0xffffffff
			//y
			int stepedY = sy & ystep;// ystep==0xffffffffのときsyになって0の時0になる 以下同様/
			y1 += stepedY;
			err += dx & ystep;
			//ch
			l = getCharFromDelta(sx, stepedY);
			if(x1 == x2)return;
		}
	}
	else{//灸 常にyを進める/
		if(dy == 0) return;
		float sz = dz / dy;
		while(1){
			putDotZS(sc, x1, y1, invz1, l);
			//z
			invz1 += sz;
			//y
			y1 += sy;
			err -= dx;

			//yを進めるか/
			int xstep = -(err < 0);
			//x
			int stepedX = sx & xstep;
			x1 += stepedX;
			err += dy & xstep;
			//ch
			l = getCharFromDelta(stepedX, sy);
			if(y1 == y2)return;
		}
	}
	putDotZS(sc, x1, y1, invz1, a_plus);
}

static void drawLine(Screen* sc, vec3 p1, vec3 p2){
	int code1 = computeOutCode(p1);
	int code2 = computeOutCode(p2);
	//明らかはみ出てる線を除外/
	if(code1 & code2) return;//同じ方向にはみ出てる/

	//画面外クリッピング/
	if(code1 | code2){
		vec3 sub = v3sub(p2, p1);
		if(((code1 | code2) & (bit_left | bit_right)) && sub.x){
			//yの変化の割合/
			float invDX = 1.f / sub.x;
			float slope_f = ((sub.y) * invDX);
			float slopeZ = sub.z * invDX;
			//左側/
			if(code1 & bit_left){//1<2
				float xstep = 0 - p1.x;//進める分/
				float ystep = (slope_f * xstep);//進める分/
				float invzStep = slopeZ * xstep;
				p1.x = 0;
				p1.y += ystep;
				p1.z += invzStep;
				//_ASSERT((0 <= x1) && (0 <= x2), "クリッピングミス　0<=x");
			}
			else if(code2 & bit_left){//2<1
				float xstep = 0 - p2.x;//進める分/
				float ystep = (slope_f * xstep);//進める分
				float invzStep = slopeZ * xstep;
				p2.x = 0;
				p2.y += ystep;
				p2.z += invzStep;
				//_ASSERT((0 <= x1) && (0 <= x2), "クリッピングミス　0<=x");
			}
			//右側/
			if(code2 & bit_right){//1<2
				float xstep = p2.x - (float)(WIDTH - 1);//戻す分/
				float ystep = (slope_f * xstep);//戻す分/
				float invzStep = slopeZ * xstep;
				p2.x = (float)(WIDTH - 1);
				p2.y -= ystep;
				p2.z -= invzStep;
				//_ASSERT((x1 < WIDTH) && (x2 < WIDTH), "クリッピングミス　x<w");
			}
			else if(code1 & bit_right){//2<1
				float xstep = p1.x - (float)(WIDTH - 1);//戻す分/
				float ystep = (slope_f * xstep);//戻す分/
				float invzStep = slopeZ * xstep;
				p1.x = (float)(WIDTH - 1);
				p1.y -= ystep;
				p1.z -= invzStep;
				//_ASSERT((x1 < WIDTH) && (x2 < WIDTH), "クリッピングミス　x<w");
			}
			code1 = computeOutCode(p1);
			code2 = computeOutCode(p2);
			if(code1 & code2) return;//同じ方向にはみ出てる/
		}
		if(((code1 | code2) & (bit_top | bit_bottom)) && sub.y){
			//yの変化の割合/
			float invDY = 1.f / sub.y;
			float slope_f = ((sub.x) * invDY);
			float slopeZ = sub.z * invDY;

			//上側/
			if(code1 & bit_top){//1<2
				float ystep = 0 - p1.y;//はみ出た分/
				float xstep = (slope_f * ystep);//進める分/
				float invzStep = slopeZ * ystep;
				p1.x += xstep;
				p1.y = 0.f;
				p1.z += invzStep;
				//_ASSERT((0 <= y1) && (0 <= y2), "クリッピングミス　0<=y");
			}
			else if(code2 & bit_top){//2<1
				float ystep = 0 - p2.y;//はみ出た分/
				float xstep = (slope_f * ystep);//進める分/
				float invzStep = slopeZ * ystep;
				p2.x += xstep;
				p2.y = 0.f;
				p2.z += invzStep;
				//_ASSERT((0 <= y1) && (0 <= y2), "クリッピングミス　0<=y");
			}
			//下側/
			if(code2 & bit_bottom){//1<2
				float ystep = p2.y - (float)(HEIGHT - 1);//はみ出た分/
				float xstep = (slope_f * ystep);//戻す分/
				float invzStep = slopeZ * ystep;
				p2.x -= xstep;
				p2.y = (float)(HEIGHT - 1);
				p2.z -= invzStep;
				//_ASSERT((y1 < HEIGHT) && (y2 < HEIGHT), "クリッピングミス　y < HEIGHT");
			}
			else if(code1 & bit_bottom){//2<1
				float ystep = p1.y - (float)(HEIGHT - 1);//はみ出た分/
				float xstep = (slope_f * ystep);//戻す分/
				float invzStep = slopeZ * ystep;
				p1.x -= xstep;
				p1.y = (float)(HEIGHT - 1);
				p1.z -= invzStep;
				//_ASSERT((y1 < HEIGHT) && (y2 < HEIGHT), "クリッピングミス　y < HEIGHT");
			}
		}
		if(((code1 | code2) & (bit_far)) && sub.z){
			//zの変化の割合/
			float invDZ = 1.f / sub.z;
			float slope_x = sub.x * invDZ;
			float slope_y = sub.y * invDZ;

			//上側/
			if(code1 & bit_far){//1<2
				float invzStep = WIRE_MAX_INV_FAR - p1.z;//はみ出た分/
				float xstep = slope_x * invzStep;//進める分/
				float ystep = slope_y * invzStep;
				p1.x += xstep;
				p1.y += ystep;
				p1.z += invzStep;
				//_ASSERT((0 <= y1) && (0 <= y2), "クリッピングミス　0<=y");
			}
			else if(code2 & bit_far){//2<1
				float invzStep = WIRE_MAX_INV_FAR - p2.z;//はみ出た分/
				float xstep = slope_x * invzStep;//進める分/
				float ystep = slope_y * invzStep;
				p2.x += xstep;
				p2.y += ystep;
				p2.z += invzStep;
				//_ASSERT((0 <= y1) && (0 <= y2), "クリッピングミス　0<=y");
			}
		}
	}

	_drawLine(sc, (int)p1.x, (int)p1.y, p1.z, (int)p2.x, (int)p2.y, p2.z);
}

//スクリーン座標/
static void cPosToScPos(vec3 cp, vec3* out, float fov){
	//inv
	out->z = 1.f / cp.z;

	//to screen pos
	out->x = (cp.x * fov * (out->z)) + (float)(WIDTH >> 1);
	out->y = (-cp.y * fov * (out->z)) + (float)(HEIGHT >> 1);
}

typedef struct{
	vec3 v;
	vec2 uv;
}vartex;

//ニアクリップ/
//p1がはみ出てる/
//はみ出てるほうを修正した位置を返す まあ要はp1からp2の辺上のz=nearの位置を返す関数/
static vartex getNearIntersection(vec3 p1, vec2 uv1, vec3 p2, vec2 uv2){
	//辺の長さ/
	vec3 sub = v3sub(p2, p1);
	vec2 sub_uv = v2sub(uv2, uv1);
	//nearからどんだけはみ出てるか/
	float dz = (0 - p1.z) + near;//戻す量/
	//はみ出てる分の長さに対する比率/
	float invDz = 1.f / sub.z;
	float t = dz * invDz;
	//まあ線形補完/
	return (vartex){
		(vec3){
		p1.x + sub.x * t,//p1の位置から辺の長さにはみ出てる比率分戻す/
			p1.y + sub.y * t,//yも同様/
			near//zはnearにしたいからそのままnear/
	},
			v2add(uv1, v2mul(sub_uv, t))
	};
}

//これをマクロで分岐させたりする/
//renderMode

#define RM_NORMAL 0
#define RM_BLEND 1
#define RM_TEMP 2
#define RASTERIZE_MODE RM_NORMAL
#include "rasterize_template.h"
#undef RASTERIZE_MODE

//カメラ座標に変換するだけ/
static inline vec3 toCameraPos(vec3 p, const Camera* const c){
	vec3 r = v3sub(p, c->p);//相対位置/
	return toLocalBasis(c->b, r);//基底変換/
}
//ワールド座標に変換するだけ　basisとか書いてるけど余裕でスケール変更かけられてたりする/
static inline vec3 toWorld(vec3 p, vec3 pos, Basis b){
	vec3 r = toGlobalBasis(b, p);//回転  ついでにスケール変更とかされてたり/
	return v3add(r, pos);//平行移動/
}
//cntを0にするだけ　簡単/
static void clearRenderStack(RenderStack* st){
	st->cnt = 0;
}
//pushModelされた奴をcの視点かscに描画するだけ/
const vec3 lightVec = { 0.f,-1.f,0.f };//適当にしたベクトル/
static void _renderStackAll(RenderStack* __restrict st, Screen* const __restrict sc, const Camera* const __restrict c){
#if ENABLE_DEBUG + 0
	int dNumOfTri = 0;
#endif
	for(int m = 0; m < st->cnt; m++){
		const Model3D* mdl = st->mdl[m];
		int vcnt = mdl->triCnt * 3;
		int triCnt = 0;
		//デバッグ/
#if ENABLE_DEBUG + 0
		dNumOfTri += mdl->triCnt;
#endif

		vec3 pos = st->p[m];

		//スケールと混ぜとく/
		Basis angle = st->angle[m];
		Basis matWorld = angle;
		matWorld.x = v3mul(matWorld.x, st->scale[m].x);
		matWorld.y = v3mul(matWorld.y, st->scale[m].y);
		matWorld.z = v3mul(matWorld.z, st->scale[m].z);

		//uv
		vec2* uv = mdl->uv;

		mdlTextureRLE* currentTexInfo = (mdl->txInfo);
		mdlTextureRLE currentTex = *currentTexInfo;

		Texture* texture = getTexture(currentTex.index);

		//描画/
		for(int i = 0; i < vcnt; i += 3){
			//テクスチャの切り替え/
			if((currentTex.cnt) <= 0){
				currentTexInfo++;//進める/
				currentTex = *currentTexInfo;
				texture = getTexture(currentTex.index);
			}
			vec3 cp[3];//頂点/

			//頂点3つをもらう/
			for(int j = 0; j < 3; j++){//最適化で展開されるはず/
				//回転/
				cp[j] = toWorld(mdl->vertices[i + j], pos, matWorld);
				//カメラ/
				cp[j] = toCameraPos(cp[j], c);
			}

			//裏面を表示しないやつ/
			//法線/
			vec3 wn = mdl->norms[triCnt];
			wn = toGlobalBasis(angle, wn);
			vec3 n = toLocalBasis(c->b, wn);
			//法線ととある編は垂直だからn・AB = 0
			//つまりn・(B-A) = 0 (ABベクトルは　BベクトルとAベクトルの差ベクトル)
			//よって(n・B)-(N・A) = 0 まあ要はBとの内積もAとの内積も等しい
			//Cベクトルも同様で等しいからcp[0]を見るだけでほかのも等しいからそれぞれを見る必要はない
			//でそもそもの祖の内積は三角形を含む無限に広がる平面への垂直距離を表してる(絶対値が)
			//だから0になった時はカメラがその平面に含まれてることになる　そんで負になった瞬間が裏面を向いてることになる
			//感覚的にはわかるけど厳密に数学的に証明しろって?だまれ/
			if(.0f < v3dot(cp[0], n))goto skipDraw;
			//描画/
			float light = (1.f - v3dot(wn, lightVec)) * .7f;
			light = CLAMP(light, .2f, 1.f);
			const int txSize = 64;
			unsigned long traitingZero = 0;
			_BitScanForward(&traitingZero, txSize);
			FaceContext fCtx = (FaceContext){
				.light = light,
				.texture = texture->texture,
				.txSize = texture->size,
				.shiftCount = traitingZero,
				.debug___ = 100,
				.shadeLength = c->shadeLength,
			};
			vec2* triUV = &uv[i];
			drawTri3d_normal(sc, cp, triUV, c->fov, &fCtx);
		skipDraw:
			triCnt++;
			currentTex.cnt--;
		}
	}
	//clear/
	clearRenderStack(st);
#if ENABLE_DEBUG + 0
	debugMember.triCnt += dNumOfTri;
#endif
}

//pushModelされたモデルたちを一つのでかいすでにワールド上に配置されたでかいモデル?として出力/
static StaticRenderStack* _createStaticRenderStack(RenderStack* st){
	// --- サイズ計算 --- /

	//三角形の数/
	int totalTri = 0;
	for(int m = 0; m < st->cnt; m++){
		const Model3D* mdl = st->mdl[m];
		totalTri += mdl->triCnt;
	}
	//サイズ/
	size_t rSize = sizeof(StaticRenderStack);
	size_t buffSize = sizeof(struct Triangle) * totalTri;
	size_t uvSize = sizeof(vec2) * totalTri * 3;
	size_t totalSize = rSize + buffSize + uvSize;//8byteアライメント/

	//メモリ確保/
	StaticRenderStack* r = (StaticRenderStack*)gm_allocate(totalSize);

	r->texInfo = (mdlTextureRLE*)gm_getCurrent();//サイズが決まってない/
	gm_d_lockMemoly();

	struct Triangle* tris = (struct Triangle*)(&r[1]);
	vec2* uv = (vec2*)(&tris[totalTri]);
	r->tri = tris;
	r->uv = uv;
	r->triCnt = totalTri;
	mdlTextureRLE* texInfoDist = r->texInfo;

	totalTri = 0;
	for(int m = 0; m < st->cnt; m++){
		const Model3D* mdl = st->mdl[m];

		//スケールと混ぜとく/
		Basis angle = (st->angle[m]);
		Basis matWorld = angle;
		matWorld.x = v3mul(matWorld.x, st->scale[m].x);
		matWorld.y = v3mul(matWorld.y, st->scale[m].y);
		matWorld.z = v3mul(matWorld.z, st->scale[m].z);

		vec3 pos = st->p[m];

		//uv
		int triCnt = mdl->triCnt;
		memcpy(&uv[totalTri * 3], mdl->uv, sizeof(vec2) * triCnt * 3);


		mdlTextureRLE* currentTexInfo = (mdl->txInfo);
		mdlTextureRLE currentTex = *currentTexInfo;
		*(texInfoDist++) = currentTex;//書き込み/

		//描画/
		for(int t = 0; t < triCnt; t++){//三角形/
			// --- テクスチャ --- /
			if((currentTex.cnt) <= 0){
				currentTexInfo++;//進める/
				currentTex = *currentTexInfo;
				*(texInfoDist++) = currentTex;//書き込み/
			}

			// --- 法線 --- /

			//回転/
			vec3 n = toGlobalBasis(angle, mdl->norms[t]);
			tris[totalTri].norm = n;

			// --- 頂点 --- /
			int vInd = t * 3;
			const vec3* triVs = &(mdl->vertices[vInd]);
			//頂点3つをもらう/
			for(int i = 0; i < 3; i++){//頂点/
				//回転/
				tris[totalTri].v[i] = toWorld(triVs[i], pos, matWorld);
			}

			totalTri++;
			currentTex.cnt--;
		}
	}
	//テクスチャのサイズの確定/
	size_t texMemSize = sizeof(mdlTextureRLE) * ((size_t)(texInfoDist - r->texInfo));
	gm_increment(texMemSize);
	gm_d_unlockMemoly();
	return r;
}

static void _pushModel(RenderStack* s, const Model3D* mdl, vec3 p, Basis angle, vec3 scale){
	if(mdl == NULL) return;
	ASSERT(s->cnt < RENDER_REQUEST_MAX, "RenderStack 入れすぎ");
	//詰める/
	s->mdl[s->cnt] = mdl;
	s->p[s->cnt] = p;
	s->angle[s->cnt] = angle;
	s->scale[s->cnt] = scale;

	s->cnt++;
}

static void _renderStaticRenderStack(const StaticRenderStack* __restrict st, Screen* const __restrict sc, const Camera* const __restrict c){
	//描画/
	mdlTextureRLE* currentTexInfo = (st->texInfo);
	mdlTextureRLE currentTex = *currentTexInfo;
	Texture* texture = getTexture(currentTex.index);
	for(int i = 0; i < st->triCnt; i++){
		//テクスチャの切り替え/
		if((currentTex.cnt) <= 0){
			currentTexInfo++;//進める/
			currentTex = *currentTexInfo;
			texture = getTexture(currentTex.index);
		}
		//三角形の取り出し/
		struct Triangle* triangle = &(st->tri[i]);

		vec3 cp[3];//頂点/

		//頂点3つをもらう/
		for(int j = 0; j < 3; j++){//最適化で展開されるはず/
			//カメラ/
			cp[j] = toCameraPos(triangle->v[j], c);
		}

		//裏面を表示しないやつ/
		vec3 n = toLocalBasis(c->b, triangle->norm);
		if(0.f < v3dot(cp[0], n)){
			goto skipDraw;
		}

		//描画/
		vec2* triUV = &(st->uv[i*3]);
		_CRT_UNUSED(triUV);//test

		const float lightMin = .7f;
		float light = -v3dot(triangle->norm, c->lightVec);
		light = lightMin + ((1.f + light) * .5f) * (1.f - lightMin);
		int txSize = texture->size;
		unsigned long traitingZero = 0;
		_BitScanForward(&traitingZero, txSize);
		FaceContext fCtx = (FaceContext){
				.light = light,
				//まだ定数/
				.texture = texture->texture,
				.txSize = txSize,
				.shiftCount = traitingZero,
				.debug___ = 100,
				.shadeLength = c->shadeLength,
		};
		drawTri3d_normal(sc, cp, triUV, c->fov, &fCtx);
	skipDraw:
		currentTex.cnt--;
	}
#if ENABLE_DEBUG + 0
	debugMember.triCntStatic += st->triCnt;
#endif
}

#define samplePoints 8
#define maxSampleOffset 2
static const int shadingDXTable[samplePoints] = {
	-2,  0,  2,  0, -1,  1,  1, -1
};
static const int shadingDYTable[samplePoints] = {
	0, -2,  0,  2, -1, -1,  1,  1
};
#define getScPos(x,y) ((x) + ((y) * WIDTH))
static void _shadingScreen(Screen* sc){//気が向いたら二つ統合する　<-結局ラスタライズに統合して使わんくなった/
	_CRT_UNUSED(sc);//エラー消し/
	return;
	//for(int i = 0; i < WIDTH * HEIGHT; i++){
	//	pixel_t c = sc->screen[i];
	//	pixel_t shade = c - asciiShade_0;
	//	if(a_sp <= c || c == 0) continue;//特殊文字/
	//	float invz = sc->zbuff[i];
	//	float z_raito = (shadeLength * invz);
	//	z_raito = CLAMP(z_raito, 0.f, 1.f);
	//	shade = (pixel_t)(((float)shade) * z_raito);
	//	sc->screen[i] = shade + asciiShade_0;
	//}
#define shadeInfluence (1.f)
	//for(int y = maxSampleOffset; y < HEIGHT - maxSampleOffset; y++){
	//	for(int x = maxSampleOffset; x < WIDTH - maxSampleOffset; x++){
	//		int index = getScPos(x, y);
	//		float cz = sc->zbuff[index];//current
	//		if(cz <= .0f) continue;//無限遠/
	//		pixel_t c = sc->screen[index];
	//		if(a_sp <= c || c == 0) continue;//特殊文字/
	//		pixel_t nextC = c - asciiShade_0;

	//		float raw_z = 1.f / cz;//とりあえず/
	//		int cnt = 0;//currentより手前のカウント/
	//		for(int i = 0; i < samplePoints; i++){
	//			int sampleP = getScPos(x + shadingDXTable[i], y + shadingDYTable[i]);
	//			float sampleZ = 1.f/sc->zbuff[sampleP];
	//			float sub = raw_z - sampleZ;//czが奥なら正/
	//			//奥でかつ近く/
	//			if(1.f < sub && sub < 10.f)cnt++;
	//		}
	//		float shade = 1.f - ((cnt / (float)samplePoints) * shadeInfluence);
	//		//確定で0-1のはず/
	//		nextC = (pixel_t)(((float)nextC) * shade);
	//		sc->screen[index] = nextC + asciiShade_0;
	//	}

	//}
#undef shadeInfluence
}
#undef maxSampleOffset
#undef samplePoints

static void _renderAllStaticRenderStack(SRS_stack* __restrict stack, const RenderContext* __restrict const ctx){
	for(int i = 0; i < stack->cnt; i++)_renderStaticRenderStack(stack->placedMdls[i], ctx->sc, ctx->c);
	stack->cnt = 0;
}

//---------------------------------------------
// public:
//---------------------------------------------

void renderStaticRenderStack(StaticRenderStack* __restrict st, const RenderContext* const __restrict ctx){
	_renderStaticRenderStack(st, ctx->sc, ctx->c);
}

Camera* createCamera(){
	//メモリ確保/
	Camera* r = (Camera*)gm_allocate(sizeof(Camera));
	//初期化/
	*r = (Camera){ 0 };
	r->fov = angleToFov(.25f);//45度/
	r->lightVec = v3normalize((vec3){ 200.f, 50.f, -1000.f });
	r->shadeLength = 80.f;
	return r;
}

void cameraDestroy(Camera** c){
	//解放/
	//free(*c);
	//0/
	*c = NULL;
}

RenderContext* createRenderContext(Screen* __restrict sc, Camera* __restrict c){
	RenderContext* r = (RenderContext*)gm_allocate(sizeof(RenderContext));
	r->c = c;
	r->sc = sc;
	return r;
}
void renderContextDestroy(RenderContext** ctx){
	//free(*ctx);
	*ctx = NULL;
}

void renderStackAll(RenderContext* ctx){
	_renderStackAll(&rStack, ctx->sc, ctx->c);
	_renderAllStaticRenderStack(&srsStack, ctx);
}

StaticRenderStack* createStaticRenderStack(){
	return _createStaticRenderStack(&rStack);
}
void destroyStaticRenderStack(StaticRenderStack** s){
	//free(*s);
	*s = NULL;
}
// --- 呼び出し元がbasisを設定するやつ --- /

void pushModelBS3(const Model3D* mdl, vec3 p, Basis angle, vec3 scale){
	_pushModel(&rStack, mdl, p, angle, scale);
}

void pushModelBS(const Model3D* mdl, vec3 p, Basis angle, float scale){
	pushModelBS3(mdl, p, angle, fltToV3(scale));
}

void pushModelB(const Model3D* mdl, vec3 p, Basis angle){
	pushModelBS3(mdl, p, angle, v3one);
}

// --- ほぼデフォルト --- /
void pushModel(const Model3D* mdl, vec3 p){
	pushModelBS3(mdl, p, basisZ, v3one);
}

// --- angleからbasisを作る系 --- /

void pushModelAS3(const Model3D* mdl, vec3 p, vec3 angle, vec3 scale){
	pushModelBS3(mdl, p, createBasis(angle), scale);
}

void pushModelAS(const Model3D* mdl, vec3 p, vec3 angle, float scale){
	pushModelAS3(mdl, p, angle, fltToV3(scale));
}

void pushModelA(const Model3D* mdl, vec3 p, vec3 angle){
	pushModelAS3(mdl, p, angle, v3one);
}

// --- vec2からbasisを生成 normalizeがないから速い --- /

void pushModelA2S3(const Model3D* mdl, vec3 p, vec2 angle, vec3 scale){
	pushModelBS3(mdl, p, createBasisV2(angle), scale);
}
void pushModelA2S(const Model3D* mdl, vec3 p, vec2 angle, float scale){
	pushModelA2S3(mdl, p, angle, fltToV3(scale));
}
void pushModelA2(const Model3D* mdl, vec3 p, vec2 angle){
	pushModelA2S3(mdl, p, angle, v3one);
}

// --- staticRenderStack --- /

void pushStaticRenderStack(const StaticRenderStack* srs){
	srsStack.placedMdls[srsStack.cnt++] = srs;
}

// --- shading --- /

void shadingScreen(Screen* sc){
	_shadingScreen(sc);
}