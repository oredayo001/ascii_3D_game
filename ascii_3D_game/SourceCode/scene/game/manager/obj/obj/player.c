#include"../objManager.h"
#include"scene/commonManager/gameModelLoader.h"
#include"player.h"
#include"scene/commonManager/input.h"
#include"scene/game/manager/map/mapManager.h"

#define baseCast(me) ((objBase*)me)
#define playerCast(me) ((objPlayer*)me)

//命名規則を守りやすくする的な/
#define playerInitializer AS_OBJ_INITIALISE_FUNC(player)
#define SELF(name) objPlayer* name = playerCast(base)

//プレイヤーの入力/
typedef struct playerIn{
	float xDir;
	float zDir;
	int jumpPressed;
	int jump;
	int sneakPreassed;
}playerIn;

#define GET_SPD(spd,brake) (spd) * (1.f - brake)
#define NORMAL_SPD 10.f
#define SNEAK_SPD 5.f

#define playerHeight 160.f
#define playerHeight_sneak 75.f
#define playerRadius 30.f
#define maxStepHeight 20.f

//walk
#define WALK_XZ_BRAKE .8f
const float walkXZBrake = WALK_XZ_BRAKE;
const float walkXZSpd = GET_SPD(NORMAL_SPD, WALK_XZ_BRAKE);//constは定数式には含めれないらしい <-は？/
const float walkJumpPow = 30.f;
//const float walkInputInfluence = 1.f;
const float walkFriction = .99f;

//walk
#define WALK_XZ_BRAKE .8f
const float sneakXZBrake = WALK_XZ_BRAKE;
const float sneakXZSpd = GET_SPD(SNEAK_SPD, WALK_XZ_BRAKE);//constは定数式には含めれないらしい <-は？/
const float sneakFriction = .99f;

//idle
const float idleJumpPow = 30.f;
const float idleFriction = 1.f;

//air
#define AIR_XZ_BRAKE .95f
const float airXZBrake = AIR_XZ_BRAKE;
const float airXZSpd = GET_SPD(NORMAL_SPD, AIR_XZ_BRAKE);
const float grv = 2.543f;

//jump
const uint16_t jumpBuffTime = 10;

//##################################################################
// private:
//##################################################################

//------------------------------------------------------------------
//各stateの更新関数の初期化関数のプロトタイプ宣言
//------------------------------------------------------------------

//歩き/
static void s_walk(objBase* base);
//歩き/
static void s_sneak(objBase* base);
//ジャンプ/
static void s_jump(objBase* base);
//入力待ち/
static void s_idle(objBase* base);

static void setValCommon(objPlayer* me, float height){
	me->height = height;
}

//------------------------------------------------------------------
//共通
//------------------------------------------------------------------

//入力/
static playerIn playerInput(playerInMemoly* memoly){
	playerIn r;
	r.xDir = (float)(keyboardCheck(vk_right) - keyboardCheck(vk_left));
	r.zDir = (float)(keyboardCheck(vk_up) - keyboardCheck(vk_down));
	r.jumpPressed = 0;
	r.jump = 0;
	r.sneakPreassed = keyboardCheckPressed(vk_shift);
	// --- jumpPressedの制御 --- /
	if((memoly->jumpInFrame < jumpBuffTime) && (memoly->jumpInFrame)){//押された瞬間じゃなくても15fの間は押された瞬間判定にする/
		if(keyboardCheck(vk_jump)){
			//押されてる間加算/
			memoly->jumpInFrame++;
			r.jumpPressed = 1;
		}
		else{
			//押されてなかったら初期化/
			memoly->jumpInFrame = 0;
		}
	}
	else{//押された瞬間を見る/
		//押されてないor押されてから15f以上/
		uint16_t pressed = (uint16_t)keyboardCheckPressed(vk_jump);//0 or 1
		r.jumpPressed = pressed;
		memoly->jumpInFrame = pressed;
	}
	// --- ジャンプしてる間 --- /
	if(memoly->jumped || memoly->jumpFrame){//ゲーム側がジャンプしたことをこの変数に入れる/
		if(memoly->jumped){//初期化/
			memoly->jumpFrame = 0;//LOW 着地がこれを0にしてくれたらこのifなくせる/
			memoly->jumped = 0;
		}
		if(keyboardCheck(vk_jump)){// ジャンプしてる間だけ/
			memoly->jumpFrame++;
			r.jump = 1;
		}
		else{//ジャンプしてない/
			memoly->jumpFrame = 0;
			r.jump = 0;
		}
	}
	return r;
}

//ステート変更/
static void changeState(objBase* base, void(*step)(objBase*)){
	base->step = step;
}

//------------------------------------------------------------------
//物理
//------------------------------------------------------------------

//方向ベクトルを取得/
static inline Basis getDir(objPlayer* me){
	return vec2BasisToBasis(me->cameraIn);
}

//xz方向の動き/
static void updateVelXZ(objPlayer* me, playerIn in, float spd, float brake){
	Basis dir = getDir(me);
	vec3 inputDir = v3normalize((vec3){ in.xDir, 0.f, in.zDir });
	vec3 moveLocal = v3mul(inputDir, spd);
	vec3 moveWorld = v3add(v3mul(dir.x, moveLocal.x), v3mul(dir.z, moveLocal.z));
	//前回の影響を下げる/
	me->base.v.x *= brake;
	me->base.v.z *= brake;
	//今回のを加算/
	me->base.v = v3add(me->base.v, moveWorld);
}

//死んだ関数　とある数値より下にいるかを返す/
static bool isOnGround(objPlayer* me){
	return 0;// (me->base.render->p.y) <= -50.f;
}

//y方向の動き/
static void updateVelY(objPlayer* me, playerIn in){
	me->base.v.y -= grv;
	_CRT_UNUSED(in);
}

typedef struct{
	uint16_t wallHit;
	uint16_t onGround;
} collisionResult;//当たり判定が返すやつ/
//当たり判定/
static void _mapCollision(objPlayer* me, float frictionInfluence, collisionResult* result){
	vec3Print("player", me->base.render->p);
	// --- 床 --- /
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);

	{
		fcResult r;
		getNearestFloorDist(me->base.render->p, me->base.v, me->height, &r);
		if(r.nextMinDist < 0.f){//速度的に床を追い越すか/
			vec3 lastV = me->base.v;
			// --- 垂直抗力を速度に加算 ---

			//取り除く/
			vec3 normForce = v3mul(r.norm, r.force);
			me->base.v = v3add(me->base.v, normForce);

			// --- 今の速度から面へ向かうベクトルの成分だけ取りのぞく --- /

			//面へ向かう成分を計算/

			//面と平行なベクトル/
			//n*(n・v)/
			vec3 normV = v3mul(r.norm, v3dot(r.norm, me->base.v));//法線ベクトルの成分/
			vec3 parallel = v3sub(me->base.v, normV);//法線ベクトルの成分をなくしたやつ/

			//面へ向かう成分だけpに足してvから取り除く　これで慣性に残さない/

			//慣性に押し返しのベクトルを残さんようにする/
			me->base.v = parallel;//面と平行な成分だけ/
			me->base.render->p = v3add(me->base.render->p, normV);//押し返し成分だけ足しておく/


			//これでnormVの成分は今のフレームでしか足されない成分になる
			//nextP = p+vが今のフレームでは正しいけどnormVの成分はvから取り除きたい
			//でも単純に取り除くとnextP = p+(v-normV)になる これはp+vとは変わってしまう
			//だからpに足すことで(p+normV)+(v-normV)になって結果を変えずに慣性に残さないようになる って感じ/

			// --- 移動後の足場を見る --- /

			fcResult nextR;
			getNearestFloorDist(me->base.render->p, v3mul(me->base.v, 1.f), me->height, &nextR);

			//前回の足場と移動後の足場の法線の違い/
			float normDiff = v3dot(nextR.norm, r.norm);
			//trueBreakPoint(normDiff < .5f);
			//printf("\n\naa[%.2f]aa\n\n", normDiff);

			//差が大きかった場合強制的にyを修正/
			if((normDiff < .8f) && (nextR.nextMinDist < 10.f)){
				vec3 nForce = v3mul(v3y, -nextR.nextMinDist);
				me->base.render->p = v3add(me->base.render->p, nForce);
			}

			// --- 摩擦 --- /

			//重力のうち面と平行な成分だけ取り出す/
			vec3 v_y = v3make(0.f, -grv, 0.f);//y成分だけ取り出す/
			vec3 normV_grv = v3mul(r.norm, v3dot(r.norm, v_y));//重力のうち/
			vec3 parallel_grv = v3sub(v_y, normV_grv);//法線ベクトルの成分をなくしたやつ/

			//ポリゴンをまたぐ場合摩擦を大きくする/
			float friction = frictionInfluence;//!magic
			friction += (1.f - friction) * (1.f - normDiff);
			//水平な成分の逆ベクトルを摩擦係数で弱めたベクトル/
			vec3 frictionV = v3mul(parallel_grv, -friction * frictionInfluence);
			//引く/
			me->base.v = v3add(me->base.v, frictionV);
			trueBreakPoint(fabsf(me->base.v.x) > 10000.f);

			//床についたときの処理/
			if(r.norm.y <= .4f){
				result->onGround = 0;
			}
			else{
				result->onGround = 1;
			}
		}
	}
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);

	// --- 天井 --- /
	{
		vec3 headPos = v3add(me->base.render->p, v3mul(v3y, me->height));
		fcResult r;
		getNearestCeilingDist(headPos, me->base.v, me->height, &r);
		if(r.nextMinDist < 0.f){//速度的に天井を追い越すか/
			vec3 normForce = v3mul(r.norm, r.force);
			me->base.v = v3add(me->base.v, normForce);//垂直抗力で押し返す/
			//慣性に押し返しのベクトルを残さんようにする/
			vec3 normV = v3mul(r.norm, v3dot(r.norm, me->base.v));//法線ベクトルの成分/
			vec3 parallel = v3sub(me->base.v, normV);//法線ベクトルの成分をなくしたやつ/
			//慣性に押し返しのベクトルを残さんようにする/
			me->base.v = parallel;//面と平行な成分だけ/
			me->base.render->p = v3add(me->base.render->p, normV);//押し返し成分だけ足しておく/
		}
	}
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);

	// --- 壁 --- /
	{
		wallResult r;
		const float radius = playerRadius;
		vec3 checkPos = me->base.render->p;
		checkPos.y += maxStepHeight;
		getNearestWall(checkPos, me->base.v, radius, me->height - maxStepHeight, &r);
		if(r.isHit){
			//printf("\n\nwallHit,%.2f,%d,", r.force, me->base.timer * 7 * 7 * 7 * 7 * 7);
			vec3 normForce = v3mul(r.norm, r.force);
			me->base.v = v3add(me->base.v, normForce);//垂直抗力で押し返す/
			//v - n*(n・v)/
			//慣性に押し返しのベクトルを残さんようにする/
			vec3 normV = v3mul(r.norm, v3dot(r.norm, me->base.v));//法線ベクトルの成分/
			vec3 parallel = v3sub(me->base.v, normV);//法線ベクトルの成分をなくしたやつ/
			//慣性に押し返しのベクトルを残さんようにする/
			me->base.v = parallel;//面と平行な成分だけ/
			me->base.render->p = v3add(me->base.render->p, normV);//押し返し成分だけ足しておく/
			result->wallHit = 1;
		}
	}
}

//地形の当たり判定
static void mapCollision(objPlayer* me, float flictionInfluence){
	collisionResult r;
	r = (collisionResult){ 0 };
	for(int i = 0; i < 1; i++){
		_mapCollision(me, flictionInfluence, &r);
	}
	me->base.onGround = r.onGround;
	printf("\n%d\n", me->base.onGround);
}

//速度から位置を更新/
static void movement(objPlayer* me){
	if(keyboardCheck(vk_arrow_up))me->base.v.y += 5.f;
	me->base.render->p = v3add(me->base.render->p, me->base.v);
	//test
	if(isOnGround(me)){
		me->base.v.y = 0.f;
		me->base.render->p.y = -50.f;//magic
		me->base.onGround = 1;
	}
}

static int canStandUp(objPlayer* me){
	fcResult r;
	vec3 headPos = v3add(me->base.render->p, v3mul(v3y, playerHeight));//立ち上がった時の頭の高さ/
	getNearestCeilingDist(headPos, me->base.v, playerHeight, &r);
	return 0.f < r.nextMinDist;
}

//------------------------------------------------------------------
//ジャンプ着地やら
//------------------------------------------------------------------
static void jump(objPlayer* me, float pow){
	me->base.v.y += pow;
	me->base.onGround = 0;
	me->inputMemoly.jumped = 1;
	changeState(baseCast(me), s_jump);
}
static void land(objPlayer* me){
	me->base.onGround = 1;
	me->inputMemoly.jumpFrame = 0;
	changeState(baseCast(me), s_idle);
}

//------------------------------------------------------------------
//各stateの状態遷移
//------------------------------------------------------------------

//jump
static void jumpChangeState(objPlayer* me, playerIn in){
	_CRT_UNUSED(in);
	if(me->base.onGround){//着地/
		changeState((objBase*)me, s_walk);
		return;
	}
}

// --- walk --- /
static void walkChangeState(objPlayer* me, playerIn in){
	float spdSq = v3lensq(me->base.v);
	const float minSpd = 4.f;
	int noInput = !(in.xDir || in.zDir);
	if(noInput && (spdSq < minSpd * minSpd)){//入力無し&遅い/
		changeState(baseCast(me), s_idle);
		return;
	}
	else if(in.jumpPressed){
		jump(me, walkJumpPow);
		return;
	}
	else if(!me->base.onGround){
		changeState(baseCast(me), s_jump);
		return;
	}
	else if(in.sneakPreassed){
		changeState(baseCast(me), s_sneak);
		return;
	}
}

// --- sneak --- /
static void sneakChangeState(objPlayer* me, playerIn in){
	float spdSq = v3lensq(me->base.v);
	const float minSpd = 4.f;
	int noInput = !(in.xDir || in.zDir);
	if(in.jumpPressed || in.sneakPreassed){
		//立ち上がれるか調べる/
		if(canStandUp(me)){
			changeState(baseCast(me), s_walk);
			return;
		}
	}
	else if(!(me->base.onGround)){
		if(canStandUp(me)){
			changeState(baseCast(me), s_jump);
			return;
		}
	}
}

// --- idle --- /
static void idleChangeState(objPlayer* me, playerIn in){
	if(in.xDir || in.zDir){
		changeState(baseCast(me), s_walk);//!状態遷移/
		return;
	}
	else if(in.jumpPressed){
		jump(me, idleJumpPow);
		return;
	}
	else if(!me->base.onGround){
		changeState(baseCast(me), s_jump);
		return;
	}
	else if(in.sneakPreassed){
		changeState(baseCast(me), s_sneak);
	}
}

//------------------------------------------------------------------
//各stateの更新
//------------------------------------------------------------------

//jump
static void jumpUpdate(objPlayer* me, playerIn in){
	// --- update vel --- /
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
	updateVelXZ(me, in, airXZSpd, airXZBrake);
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
	updateVelY(me, in);
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
	// --- update pos --- /
	mapCollision(me, .6f);
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
	movement(me);
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
	// --- change state --- /
	jumpChangeState(me, in);
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);
}

//walk
static void walkUpdate(objPlayer* me, playerIn in){
	// --- update vel --- /
	updateVelXZ(me, in, walkXZSpd, walkXZBrake);
	updateVelY(me, in);
	// --- update pos --- /
	mapCollision(me, walkFriction);
	movement(me);
	// --- change state --- /
	walkChangeState(me, in);
}

static void sneakUpdate(objPlayer* me, playerIn in){
	// --- update vel --- /
	updateVelXZ(me, in, sneakXZSpd, sneakXZBrake);
	updateVelY(me, in);
	// --- update pos --- /
	mapCollision(me, sneakFriction);
	movement(me);
	// --- change state --- /
	sneakChangeState(me, in);
}

//idle
static void idleUpdate(objPlayer* me, playerIn in){
	me->base.v = (vec3){ 0 };
	updateVelXZ(me, in, 0.f, 1.f);
	updateVelY(me, in);
	// --- update pos --- /
	mapCollision(me, idleFriction);
	movement(me);
	// --- change state --- /
	idleChangeState(me, in);
}

// --------------------------------------------------------------
// managerから呼ばれる奴ら
// --------------------------------------------------------------

//初期化以外の共通の/
#define COMMON_ACTION(in,playerPtrName) SELF(playerPtrName);playerIn in = playerInput(&(playerPtrName->inputMemoly))
// --- state毎の更新関数 --- /

//歩き/
static void s_jumpStep(objBase* base){
	COMMON_ACTION(input, me);
	jumpUpdate(me, input);
}
static void s_jump(objBase* base){//初期化用/
	changeState(base, s_jumpStep);//!更新用に状態遷移/
	setValCommon(playerCast(base), playerHeight);
	s_jumpStep(base);
}

//歩き/
static void s_walkStep(objBase* base){
	COMMON_ACTION(input, me);
	walkUpdate(me, input);
}
static void s_walk(objBase* base){//初期化用/
	changeState(base, s_walkStep);//!更新用に状態遷移/
	setValCommon(playerCast(base), playerHeight);
	s_walkStep(base);
}

//しゃがみ/
static void s_sneakStep(objBase* base){
	COMMON_ACTION(input, me);
	sneakUpdate(me, input);
}
static void s_sneak(objBase* base){//初期化用/
	changeState(base, s_sneakStep);//!更新用に状態遷移/
	setValCommon(playerCast(base), playerHeight_sneak);
	s_sneakStep(base);
}

//入力待ち/
static void s_idleStep(objBase* base){
	COMMON_ACTION(input, me);
	idleUpdate(me, input);
	//printf("%d", base->timer * 7 * 7 * 7 * 7 * 7 * 7);
}
static void s_idle(objBase* base){//初期化用/
	changeState(base, s_idleStep);//!更新用に状態遷移/
	setValCommon(playerCast(base), playerHeight);
	s_idleStep(base);
}


//##################################################################
// interface:
//##################################################################

// --- destroy --- /
static void destroy(objBase* base){
	SELF(me);
	_CRT_UNUSED(me);
}
// --- playable --- /
static vec2Basis* getCameraIn(objBase* base){
	SELF(me);
	return &(me->cameraIn);
}
static vec3 getEyePos(objBase* base){
	SELF(me);
	return (vec3){ 0.f, me->height - 20.f, 0.f };
}
// --- set --- /
static objIInterfaceVTable interfaceVTable = {
	.destroy = destroy,//一旦/
	.playerInterface = {
		.p = NULL
	},
	.playableInterface = {
		.getCameraIn = getCameraIn,
		.getEyePos = getEyePos,
	},
};

//##################################################################
// public:
//##################################################################

//objInitOut playerInitializer(objBase* me);//提案がうっとおしかったからここでプロトタイプ宣言/
objInitOut playerInitializer(objBase* base){
	SELF(me);
	base->v = (vec3){ 0 };
	base->render->angle = basisZ;
	base->render->scale = v3one;
	me->cameraIn = vec2basisY;
	me->inputMemoly = (playerInMemoly){ 0 };
	me->height = playerHeight;
	return (objInitOut){ .step = s_idle, .model = getObjMdl(objModel_player), .interfaces = &interfaceVTable };
}