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
}playerIn;

#define GET_SPD(spd,brake) (spd) * (1.f - brake)
#define NORMAL_SPD 50.f

//walk
#define WALK_XZ_BRAKE .8f
const float walkXZBrake = WALK_XZ_BRAKE;
const float walkXZSpd = GET_SPD(NORMAL_SPD, WALK_XZ_BRAKE);//constは定数式には含めれないらしい <-は？/
const float walkJumpPow = 110.f;
const float walkInputInfluence = 1.f;
const float walkFriction = .2f;

//idle
const float idleJumpPow = 100.f;
const float idleFriction = 1.f;

//air
#define AIR_XZ_BRAKE .95f
const float airXZBrake = AIR_XZ_BRAKE;
const float airXZSpd = GET_SPD(NORMAL_SPD, AIR_XZ_BRAKE);
const float grv = 9.765f;

//jump
const uint16_t jumpBuffTime = 10;

//##################################################################
// private:
//##################################################################

//各stateの更新関数の初期化関数のプロトタイプ宣言/

//歩き/
void s_walk(objBase* base);
//ジャンプ/
void s_jump(objBase* base);
//入力待ち/
void s_idle(objBase* base);

//入力関数/
static playerIn playerInput(playerInMemoly* memoly){
	playerIn r;
	r.xDir = (float)(keyboardCheck(vk_right) - keyboardCheck(vk_left));
	r.zDir = (float)(keyboardCheck(vk_up) - keyboardCheck(vk_down));
	r.jumpPressed = 0;
	r.jump = 0;
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

// --- マリオの物理 --- /
static inline Basis getDir(objPlayer* me){
	return vec2BasisToBasis(me->cameraIn);
}
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

static bool isOnGround(objPlayer* me){
	return (me->base.render->p.y) <= -50.f;
}
static void updateVelY(objPlayer* me, playerIn in){
	me->base.v.y -= grv;
	_CRT_UNUSED(in);
}

typedef struct{
	uint16_t wallHit;
	uint16_t onGround;
} collisionResult;
static void _mapCollision(objPlayer* me, float frictionInfluence, collisionResult* result){
	vec3Print("player", me->base.render->p);
	// --- 床 --- /
	trueBreakPoint(fabsf(me->base.v.x) > 10000.f);

	{
		fcResult r;
		getNearestFloorDist(me->base.render->p, me->base.v, &r);
		if(r.nextMinDist < 0.f){//速度的に床を追い越すか/
			vec3 normForce = v3mul(r.norm, r.force);
			me->base.v = v3add(me->base.v, normForce);
			float friction = .2f * frictionInfluence;//!magic
			//面と平行なベクトル/
			//v - n*(n・v)　まあ速度から法線方向のだけ取りのぞく/
			vec3 normV = v3mul(r.norm, v3dot(r.norm, me->base.v));//法線ベクトルの成分/
			vec3 parallel = v3sub(me->base.v, normV);//法線ベクトルの成分をなくしたやつ/
			//慣性に押し返しのベクトルを残さんようにする/
			me->base.v = parallel;//面と平行な成分だけ/
			me->base.render->p = v3add(me->base.render->p, normV);//押し返し成分だけ足しておく/
			//これでnormVの成分は今のフレームでしか足されない成分になる
			//nextP = p+vが今のフレームでは正しいけどnormVの成分はvから取り除きたい
			//でも単純に取り除くとnextP = p+(v-normV)になる これはp+vとは変わってしまう
			//だからpに足すことで(p+normV)+(v-normV)になって結果を変えずに慣性に残さないようになる って感じ/
			fcResult nextR;
			getNearestFloorDist(me->base.render->p, v3mul(me->base.v, 1.f), &nextR);

			float normDiff = v3dot(nextR.norm, r.norm);
			//trueBreakPoint(normDiff < .5f);
			printf("\n\naa[%.2f]aa\n\n", normDiff);
			friction += (1.f - friction) * (1.f - normDiff);
			if((normDiff < .8f) && (nextR.nextMinDist < 10.f)){
				vec3 nForce = v3mul(v3y, -nextR.nextMinDist);
				me->base.render->p = v3add(me->base.render->p, nForce);
			}
			//水平な成分の逆ベクトルを摩擦係数で弱めたベクトル/
			vec3 frictionV = v3mul(parallel, -friction * frictionInfluence);
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
		vec3 headPos = v3add(me->base.render->p, v3mul(v3y, 160.f));//!magic
		fcResult r;
		getNearestCeilingDist(headPos, me->base.v, &r);
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
		const float radius = 50.f;
		getNearestWall(me->base.render->p, me->base.v, radius, 160.f, &r);
		if(r.isHit){
			printf("\n\nwallHit,%.2f,%d,", r.force, me->base.timer * 7 * 7 * 7 * 7 * 7);
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

static void mapCollision(objPlayer* me, float flictionInfluence){
	collisionResult r;
	r = (collisionResult){ 0 };
	for(int i = 0; i < 1; i++){
		_mapCollision(me, flictionInfluence, &r);
	}
	me->base.onGround = r.onGround;
}

static void movement(objPlayer* me){
	if(keyboardCheck(vk_arrow_up))me->base.v.y += 20.f;
	me->base.render->p = v3add(me->base.render->p, me->base.v);

	if(isOnGround(me)){
		me->base.v.y = 0.f;
		me->base.render->p.y = -50.f;
		me->base.onGround = 1;
	}
}
// --- マリオのアニメーション --- /

// --- ジャンプやら着地やらの --- /
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

// --- jump --- /
static void jumpChangeState(objPlayer* me, playerIn in){
	_CRT_UNUSED(in);
	if(me->base.onGround){//着地/
		changeState((objBase*)me, s_walk);
		return;
	}
}
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
// --- walk --- /
static void walkChangeState(objPlayer* me, playerIn in){
	float spdSq = v3lensq(me->base.v);
	const float minSpd = 8.f;
	int noInput = !(in.xDir || in.zDir);
	if(noInput && (spdSq < minSpd * minSpd)){//入力無し&遅い/
		changeState(baseCast(me), s_idle);
		return;
	}
	else if(in.jumpPressed){
		jump(me, walkJumpPow);
		return;
	}
	if(!me->base.onGround){
		changeState(baseCast(me), s_jump);
		return;
	}
}
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
	if(!me->base.onGround){
		changeState(baseCast(me), s_jump);
		return;
	}
}
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
void s_jump(objBase* base){//初期化用/
	changeState(base, s_jumpStep);//!更新用に状態遷移/
	s_jumpStep(base);
}

//歩き/
static void s_walkStep(objBase* base){
	COMMON_ACTION(input, me);
	walkUpdate(me, input);
}
void s_walk(objBase* base){//初期化用/
	changeState(base, s_walkStep);//!更新用に状態遷移/
	s_walkStep(base);
}

//入力待ち/
static void s_idleStep(objBase* base){
	COMMON_ACTION(input, me);
	idleUpdate(me, input);
	//printf("%d", base->timer * 7 * 7 * 7 * 7 * 7 * 7);
}
void s_idle(objBase* base){//初期化用/
	changeState(base, s_idleStep);//!更新用に状態遷移/
	s_idleStep(base);
}

// --- destroy --- /
static void destroy(objBase* base){
	SELF(me);
	_CRT_UNUSED(me);
}

//##################################################################
// interface:
//##################################################################
static objIInterfaceVTable interfaceVTable = {
	.destroy = destroy,//一旦/
	.playerInterface = {.p = NULL },
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
	return (objInitOut){ .step = s_idle, .model = getObjMdl(objModel_player), .interfaces = &interfaceVTable };
}