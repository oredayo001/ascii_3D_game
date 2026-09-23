#include "mapManager.h"
#include "engine/graphics/render3d.h"
#include "engine/buffer/gameBuff.h"
#include "common.h"
#include <float.h>

#define _ENABLE_DEBUG_X 1
#define _ENABLE_DEBUG (_ENABLE_DEBUG_X&&ENABLE_DEBUG)

enum{
	triType_floor,//床/
	triType_wall,//壁/
	triType_ceiling,//天井/
	triType_max,
};

typedef struct{
	int* indices;//三角形のインデックス配列/
	int cnt;
}GridData;

#define GRID_DATA_ARRAY_SIZE (M_GRID_NUM * M_GRID_NUM * triType_max)

typedef struct{
	GridData gridData[GRID_DATA_ARRAY_SIZE];
}mapCollisionData;

static struct{
	StaticRenderStack* worldModel;
	mapCollisionData mapData;
}m;

//小さい方/
#define MIN(a,b) (((a)<(b))?(a):(b))
//でかい方/
#define MAX(a,b) (((a)>(b))?(a):(b))
//クランプ/
#define CLAMP(x,min,max) (MAX(MIN((max),(x)),(min)))

//######################################################################
// private:
//######################################################################

static inline int getTriType(vec3 norm){
	if(.15f < norm.y) return triType_floor;//下向き/
	else if(norm.y < -.15f) return triType_ceiling;//上向き/
	else return triType_wall;//ほぼ垂直/
}

static inline int getGridIndex(int gx, int gz, int triType){
	return ((gx + gz * M_GRID_NUM) * triType_max) + triType;
}

//----------------------------------------------------------------------
// mapの操作
//----------------------------------------------------------------------

static void countGridDataSize(mapCollisionData* map, int gridIndex, int triIndex){
	_CRT_UNUSED(triIndex);
	map->gridData[gridIndex].cnt++;
}
static void pushTriIndex(mapCollisionData* map, int gridIndex, int triIndex){
	int* cnt = &(map->gridData[gridIndex].cnt);
	map->gridData[gridIndex].indices[*cnt] = triIndex;
	(*cnt)++;
}

typedef void (*checkUpToDo)(mapCollisionData*, int gridInd, int triInd);
//全ポリゴンのグリッド位置と傾きからタイプを調べてfuncでセットしていく/
static int checkUpPolygone(mapCollisionData* map, const StaticRenderStack* mdl, checkUpToDo func){
	int totalCount = 0;
	for(int i = 0; i < mdl->triCnt; i++){//i:triIndex
		struct Triangle tri = mdl->tri[i];
		// --- aabb --- /

		//最大最小位置を計算/
		//f to int は floorf してintキャストするやつ/
		int minX = FtoINT(MIN(MIN(tri.v[0].x, tri.v[1].x), tri.v[2].x));
		int maxX = FtoINT(MAX(MAX(tri.v[0].x, tri.v[1].x), tri.v[2].x));
		int minZ = FtoINT(MIN(MIN(tri.v[0].z, tri.v[1].z), tri.v[2].z));
		int maxZ = FtoINT(MAX(MAX(tri.v[0].z, tri.v[1].z), tri.v[2].z));

		//位置からグリッドのマスのどこにあるかに変換/
		int minGX = getGrid(minX);
		int maxGX = getGrid(maxX);
		int minGZ = getGrid(minZ);
		int maxGZ = getGrid(maxZ);

		//type
		int type = getTriType(tri.norm);

		//trueBreakPoint((type == triType_ceiling) && (tri.v[0].y > 0.f) && (fabsf(tri.v[0].x) < 1000.f) && (fabsf(tri.v[0].z) < 1000.f));

		//最小の場所から最大の場所までfuncを実行的な/
		for(int gz = minGZ; gz <= maxGZ; gz++){
			for(int gx = minGX; gx <= maxGX; gx++){
				int gridIndex = getGridIndex(gx, gz, type);
				func(map, gridIndex, i);//まあ最適化で2つ分作ってくれるかな?/
				totalCount++;
			}
		}
	}
	return totalCount;
}

//TODO gm_allocateじゃなくてgetCurrentのほう使えば同じ計算二回線でいい/
static void createMapCollisionData(mapCollisionData* map, const StaticRenderStack* mdl){
	//各グリッドのサイズを計算/
	int cnt = checkUpPolygone(map, mdl, countGridDataSize);
	//メモリ確保/
	int* p = (int*)gm_allocate(cnt * sizeof(int));
	//各グリッドにメモリを分配/
	for(int i = 0; i < GRID_DATA_ARRAY_SIZE; i++){
		//pを入れる/
		map->gridData[i].indices = p;
		//pを進める/
		size_t size = map->gridData[i].cnt;
		p += size;
		//あとでまたカウントするから初期化しとく/
		map->gridData[i].cnt = 0;
	}
	//ポリゴンのインデックスを詰める/
	(void)checkUpPolygone(map, mdl, pushTriIndex);//全く同じ計算をもう一回する まあ初期化やし良いやろ/
}
//メモリの開放と0埋め/
static void destroyMapCollisionData(mapCollisionData* map){
	//[0]がmallocした先頭アドレス/
	//free(map->gridData[0].indices);
	//cntリセットとnull埋めを兼ねる/
	memset(map, 0, sizeof(mapCollisionData));
}

//----------------------------------------------------------------------
// mapの機能
//----------------------------------------------------------------------

// --- 計算 --- /

//e1:v1-v0 e2:v2-v1
static int isInTri(vec2 p, vec2 v0, vec2 e1, vec2 e2){
	vec2 s = v2sub(p, v0);
	//hitP - v0 = s = t(e1) + u(e2) 0<=t,0<=u,t+u<=1
	//[e1 e2](t,u) = s
	float f = v2det(e1, e2);//det([e1 e2])/
	//ft=det([(t,u) (0,1)])=det([s e2])
	//fu=det([(1,0) (t,u)])=det([e1 s])
	float ft = v2det(s, e2);
	float fu = v2det(e1, s);
	//fで不等号が反転する可能性がある/
	int judge_p = (0.f <= ft) & (0.f <= fu) & (ft + fu <= f);
	int judge_n = (ft <= 0.f) & (fu <= 0.f) & (f <= ft + fu);
	//範囲内か/
	int judge = (0 < f) ? judge_p : judge_n;
	return judge;
}
//棒が辺に当たってるか/
static int isHitTrisEdge(vec2 p, float len, vec2 v0, vec2 v1, vec2 v2){
	vec2 v[3] = { v0,v1,v2 };
	for(int i = 0; i < 3; i++){
		vec2 a = v[i];
		vec2 b = v[(i + 1) % 3];//まあ最適化で展開されるであろう/
		if((a.x < p.x && b.x < p.x) || (a.x > p.x && b.x > p.x))continue;//tが範囲外/
		if(a.x == b.x) continue;//垂直/
		//線形補完/
		vec2 e = v2sub(b, a);

		float t = (p.x - a.x) / e.x;//0-1 上のほうで範囲外ははじかれてる はず/
		float hitY = a.y + e.y * t;//辺上のpから上になんか飛ばしたときに当たる点のy/

		float sub = hitY - p.y;//当たる場所までの距離/

		if(0 <= sub && sub <= len) return 1;//true
	}
	return 0;//false
}

typedef struct{
	float f;
	float fu;
	float fv;
	vec3 sub;
	vec3 edge1;
	vec3 edge2;
} MTResult;

static inline void calcMTResult(vec3 p, vec3 rayV_m, const struct Triangle* tri, MTResult* r){
	/*
	とある場所Pから真下ベクトルDの方向にレイを飛ばすと頂点V0V1V2を持つ三角形に当たるか
	当たる場所はP+tD　三角形は(V1-V0)=E1,(V2-V0)=E2とするとV0+uE1+vE2と表せる
	ってことは三角形上に当たるとき P+tD=V0+uE1+vE2,0<=u,0<=v,u+v<=1ってなる
	整理すると　P-V0 = t(-D)+uE1+vE2
	P-V0=Sとすると t(-D) + uE1 + vE2 = S
	つまり行列[-D E1 E2]でベクトル(t,u,v)を変換した結果がSといいかえれる(?)
	t=det([(t,u,v) (0,1,0) (0,0,1)])　これが変換されると　det([ S E1 E2])=( S X E1)・E2
	u=det([(1,0,0) (t,u,v) (0,0,1)])　これが変換されると　det([-D  S E2])=(-D X  S)・E2
	v=det([(1,0,0) (0,1,0) (t,u,v)])　これが変換されると　det([-D E1  S])=(-D X E1)・ S
	それぞれ変換率はdet([-D E1 E2])=(-D X E1)・E2だからこれをfと置くと
	t=(1/f)(( S X E1)・E2)
	u=(1/f)((-D X  S)・E2)
	v=(1/f)((-D X E1)・ S)
	これをちょっといじると使いまわせたりする
	A=E2 X -D
	B=-D X S
	とすると
	f = A・E1 ((-D X E1)・E2 = (E2 X -D)・E1 = A・E1)
	t = (1/f)((S X E1))・E2 tは当たってるときだけだからそんな軽くする必要はない/
	u = (1/f)(B・E2)
	v =-(1/f)(B・E1) ( ((-D X E1)・ S) = ((S X -D)・ E1) = (-(-D X S)・ E1) = ((-B)・E1) ) = -(B・E1)
	でも割り算はそもそも遅いから条件式
	0<=u,0<=v,u+v<=1
	これは
	0<=u,0<=v,f*(u+v)<=f
	ってできるからtを求めるまで1/fを求めるのを遅らせれる
	*/
	//出力ベクトル/
	r->sub = v3sub(p, tri->v[0]);
	//変換行列/
	const vec3 ray_m = rayV_m;//レイの逆ベクトル つまり上ベクトル　基底その1/
	//法線ベクトルは頂点10と20の外積　要は10の辺から見た20の辺は度のポリゴンも同じ向き(?)にある/
	//f = det([-D E1 E2])=(-D X E1)・E2=(E1 X E2)・(-D)
	//天井と上ベクトルを見るときE1とE2の外積は下　-Dも下　つまりfは正/
	//床と下ベクトルを見るときはE1とE2の外積は上　-Dも上　つまりfは正/
	r->edge1 = v3sub(tri->v[1], tri->v[0]);//辺1 基底その2/
	r->edge2 = v3sub(tri->v[2], tri->v[0]);//辺2 基底その3/
	//使いまわせるベクトル/
	vec3 A = v3cross(r->edge2, ray_m);
	vec3 B = v3cross(ray_m, r->sub);
	//三角形のどの辺の位置になるか/
	r->f = v3dot(A, r->edge1);
	r->fu = v3dot(B, r->edge2);
	r->fv = -v3dot(B, r->edge1);
}
static inline int isInPolygone_MT(MTResult* mt){
	return (0.f <= mt->fu) && (0.f <= mt->fv) && ((mt->fu + mt->fv) <= mt->f);
}
static void _getNearestSurface(mapCollisionData* __restrict map, StaticRenderStack* __restrict polygones, vec3 p, vec3 v, fcResult* __restrict result, float checkRange, vec3 rayV_m, int triType){
	//LOW gx gz の範囲外チェックやら/
	int gx = getGrid(FtoINT(p.x));
	int gz = getGrid(FtoINT(p.z));
	GridData* grid = &(map->gridData[getGridIndex(gx, gz, triType)]);
	float minDist = FLT_MAX;
	struct Triangle* hitTri = NULL;
	//グリッド内のポリゴンをループ/
	for(int i = 0; i < grid->cnt; i++){
		int triInd = grid->indices[i];
		struct Triangle* tri = &(polygones->tri[triInd]);

		MTResult mt;
		calcMTResult(p, rayV_m, tri, &mt);

		//交点が三角形の間に収まってるか/
		if(isInPolygone_MT(&mt)){//ここは全体にfかけてたら結果が同じ/
			//tは高さと同じ/
			float ft = v3dot(v3cross(mt.sub, mt.edge1), mt.edge2);
			//数字は許容範囲/
			if(ft < -checkRange * mt.f) continue;
			float t = ft / mt.f;//ここで割る/
			//最小を求める/
			if(t < minDist){
				minDist = t;
				hitTri = tri;
			}
		}
	}
	*result = (fcResult){ 0 };
	result->currentMinDist = minDist;
	if(hitTri){
		vec3 moved = v3add(p, v);
		//めり込んでたら正/
		vec3 sub = v3sub(hitTri->v[0], moved);
		//ポリゴンを含む平面までの垂直距離/
		result->force = v3dot(sub, hitTri->norm);
		result->norm = hitTri->norm;
		result->tri = hitTri;
		// --- 次の当たる場所を見る --- /
		vec3 S = v3sub(moved, hitTri->v[0]);

		const vec3 ray_m = rayV_m;
		//f = (-D X E1)・E2 = (E1 X E2)・(-D)
		//t = (1/f)((S X E1))・E2 = (1/f)((E1 X E2)・S
		vec3 edge1 = v3sub(hitTri->v[1], hitTri->v[0]);//辺1 基底その2/
		vec3 edge2 = v3sub(hitTri->v[2], hitTri->v[0]);//辺2 基底その3/
		vec3 c = v3cross(edge1, edge2);//使いまわせる/
		float f = v3dot(c, ray_m);
		float t = v3dot(c, S) / f;
		result->nextMinDist = t;
	}
}
static int _isPolygoneBetween(mapCollisionData* __restrict map, StaticRenderStack* __restrict polygones, vec3 p1, vec3 p2, int triType){
	vec3 rayV = v3sub(p2, p1);
	vec3 rayV_m = v3sub(p1, p2);
	//LOW gx gz の範囲外チェックやら/
	int gx1 = getGrid(FtoINT(p1.x));
	int gz1 = getGrid(FtoINT(p1.z));
	int gx2 = getGrid(FtoINT(p2.x));
	int gz2 = getGrid(FtoINT(p2.z));

	int xMin = MIN(gx1, gx2); int xMax = MAX(gx1, gx2);
	int zMin = MIN(gz1, gz2); int zMax = MAX(gz1, gz2);
	//TODO ddaアルゴリズム使う/
	for(int gx = xMin; gx <= xMax; gx++){
		for(int gz = zMin; gz <= zMax; gz++){
			GridData* grid = &(map->gridData[getGridIndex(gx, gz, triType)]);
			float minDist = FLT_MAX;
			struct Triangle* hitTri = NULL;
			//グリッド内のポリゴンをループ/
			for(int i = 0; i < grid->cnt; i++){
				int triInd = grid->indices[i];
				struct Triangle* tri = &(polygones->tri[triInd]);

				MTResult mt;
				calcMTResult(p1, rayV_m, tri, &mt);
				if(mt.f < 0) continue;

				//交点が三角形の間に収まってるか/
				if(isInPolygone_MT(&mt)){//ここは全体にfかけてたら結果が同じ/
					//tは高さと同じ/
					float ft = v3dot(v3cross(mt.sub, mt.edge1), mt.edge2);
					//後ろか/
					if(0.f <= ft && ft < mt.f) return 1;
				}
			}
		}
	}
	return 0;
}

static void _getNearestWall(mapCollisionData* map, StaticRenderStack* polygones, vec3 p, vec3 v, float r, float h, wallResult* result){
	vec3 moved = v3add(p, v);//次の位置/

	int mingx = MAX(getGrid(FtoINT(moved.x - r)), 0);
	int mingz = MAX(getGrid(FtoINT(moved.z - r)), 0);
	int maxgx = MIN(getGrid(FtoINT(moved.x + r)), M_GRID_NUM - 1);
	int maxgz = MIN(getGrid(FtoINT(moved.z + r)), M_GRID_NUM - 1);

	float minDistAbs = 10000.f;
	int isHit = 0;
	float finalDist = 10000.f;
	struct Triangle* hitTri = NULL;

	for(int gx = mingx; gx <= maxgx; gx++){
		for(int gz = mingz; gz <= maxgz; gz++){
			GridData* grid = &(map->gridData[getGridIndex(gx, gz, triType_wall)]);

			//一旦速度が速いときのは無視の簡易的な xzは基本高速には動かんと仮定した奴/
			for(int i = 0; i < grid->cnt; i++){
				int triInd = grid->indices[i];
				struct Triangle* tri = &(polygones->tri[triInd]);

				// --- y --- /
				float yMin = MIN(MIN(tri->v[0].y, tri->v[1].y), tri->v[2].y);
				float yMax = MAX(MAX(tri->v[0].y, tri->v[1].y), tri->v[2].y);

				float bottom = moved.y;
				float top = moved.y + h;
				//topが下より上でbottomが上より下/
				if(top < yMin || yMax < bottom) continue;

				// --- r --- /
				vec3 sub = v3sub(tri->v[0], moved);//位置から頂点/
				float nDist = v3dot(tri->norm, sub);//三角形を含む点のめり込み量 めり込んでたらプラス/
				float distAbs = fabsf(nDist);
				if(r < distAbs)continue;//半径より離れてる/
				if(minDistAbs < distAbs) continue;//計算する価値無い/

				// --- ここからはループ内で1回か2回くらいしかやられないはず --- /

				// --- 投影 --- /
				vec3 projected = v3sub(moved, v3mul(tri->norm, nDist));

				vec3 a = tri->v[0];
				vec3 b = tri->v[1];
				vec3 c = tri->v[2];
				vec2 a2, b2, c2, p2;
				//xz上で当たってるか/
				if(fabsf(tri->norm.x) < .5f){//z方向向いてる/
					//つまりx方向に広がってる平面にぶつかってるってこと/

					//xの最小最大/
					float xMin = MIN(MIN(tri->v[0].x, tri->v[1].x), tri->v[2].x);
					float xMax = MAX(MAX(tri->v[0].x, tri->v[1].x), tri->v[2].x);

					if(moved.x < xMin || xMax < moved.x) continue;//ほぼ当たってない 当たってても隣のポリゴンが何とかする　はず/
					//zを押しつぶす/
					a2 = (vec2){ a.x, a.y };
					b2 = (vec2){ b.x, b.y };
					c2 = (vec2){ c.x, c.y };
					p2 = (vec2){ projected.x,projected.y };
				}
				else{//x方向向いてる/
					//面自体はz方向に広がってる/

					//zの最小最大/
					float zMin = MIN(MIN(tri->v[0].z, tri->v[1].z), tri->v[2].z);
					float zMax = MAX(MAX(tri->v[0].z, tri->v[1].z), tri->v[2].z);

					if(moved.z < zMin || zMax < moved.z) continue;//ほぼ当たってない 多分/

					//xを押しつぶす/
					a2 = (vec2){ a.z, a.y };
					b2 = (vec2){ b.z, b.y };
					c2 = (vec2){ c.z, c.y };
					p2 = (vec2){ projected.z,projected.y };
				}
				vec2 e1 = v2sub(b2, a2);
				vec2 e2 = v2sub(c2, a2);
				if(isInTri(p2, a2, e1, e2))goto HIT;
				vec2 head = (vec2){ p2.x, p2.y + h };
				if(isInTri(head, a2, e1, e2))goto HIT;
				if(isHitTrisEdge(p2, h, a2, b2, c2))goto HIT;
				continue;
			HIT:
				{
					hitTri = tri;
					minDistAbs = distAbs;
					finalDist = nDist;
					isHit = 1;
				}
			}
		}
	}
	*result = (wallResult){ 0 };
	if(hitTri != NULL){
		result->isHit = isHit;

		result->force = finalDist + r;//押し返すべき距離/
		result->norm = hitTri->norm;
		result->tri = hitTri;
	}
}

//######################################################################
// public:
//######################################################################

// --- 操作 --- /

//呼び出し元はpushModelをした後にこれを呼ぶ　(でいいんかな)/
void createMap(){
	ASSERT(m.worldModel == NULL, "マップの二回目の読み込み");
	m.worldModel = createStaticRenderStack();
	createMapCollisionData(&(m.mapData), m.worldModel);
}

void renderMap(){
	NULL_CHECK(m.worldModel, "worldModelがぬるぽ");
	pushStaticRenderStack(m.worldModel);
	//renderStaticRenderStack(m.worldModel, rCtx);
}

void destroyMap(){
	destroyMapCollisionData(&(m.mapData));
	destroyStaticRenderStack(&(m.worldModel));
}

// --- 機能 --- /
void getNearestFloorDist(vec3 p, vec3 v, float checkRange, fcResult* result){
	_getNearestSurface(&(m.mapData), m.worldModel, p, v, result, checkRange, v3y, triType_floor);//下ベクトルだからマイナスは上ベクトル/
}

void getNearestCeilingDist(vec3 p, vec3 v, float checkRange, fcResult* result){
	_getNearestSurface(&(m.mapData), m.worldModel, p, v, result, checkRange, v3ym, triType_ceiling);//上ベクトルだからマイナスは下ベクトル/
}

void getNearestWall(vec3 p, vec3 v, float r, float h, wallResult* result){
	_getNearestWall(&(m.mapData), m.worldModel, p, v, r, h, result);
}

int isNoWall(vec3 p1, vec3 p2){
	vec3 sub = v3sub(p2, p1);//reverse
	int floorOrCeiling = sub.y < 0 ? triType_floor : triType_ceiling;//下やったら床/
	if(100.f < fabsf(sub.y)){
		//縦長なら床天井から見る/
		if(_isPolygoneBetween(&(m.mapData), m.worldModel, p1, p2, floorOrCeiling))return 0;
		if(_isPolygoneBetween(&(m.mapData), m.worldModel, p1, p2, triType_wall))return 0;
	}
	else{
		//壁から見る/
		if(_isPolygoneBetween(&(m.mapData), m.worldModel, p1, p2, triType_wall))return 0;
		if(_isPolygoneBetween(&(m.mapData), m.worldModel, p1, p2, floorOrCeiling))return 0;
	}
	return 1;
}



// ---- DEBUG ---- /
void _mapDebug_drawGrid(void* sc){
	//float yMin = m.worldModel->bbox.min.y;
	//float yMax = m.worldModel->bbox.max.y;
	float yMin = -10000.f;
	float yMax = 10000.f;
	for(int gx = 0; gx <= M_GRID_NUM; gx++){
		for(int gz = 0; gz <= M_GRID_NUM; gz++){
			float x = (gx * M_GRID_SIZE) - POS_MAX;
			float z = (gz * M_GRID_SIZE) - POS_MAX;
			vec3 p1 = (vec3){ x, yMin, z };
			vec3 p2 = (vec3){ x, yMax, z };
			drawLine3D(sc, p1, p2);
		}
	}
	for(int i = 0; i <= M_GRID_NUM; i++){
		float coord = POS_MIN + (i * M_GRID_SIZE);
		// x方向の格子線/
		drawLine3D(sc, v3make(coord, 0.f, POS_MIN), v3make(coord, 0.f, POS_MAX));
		// z方向の格子線/
		drawLine3D(sc, v3make(POS_MIN, 0.f, coord), v3make(POS_MAX, 0.f, coord));
	}
}