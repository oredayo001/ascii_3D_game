// --- obj --- /
#include"../objManager.h"
#include"chaser.h"

// --- manager --- /
//model
#include"scene/commonManager/gameModelLoader.h"
//map
#include"scene/game/manager/map/mapManager.h"

// --- obj --- /
#include"player.h"

#define POWDER_NUM __CHASER_POWDER_NUM__
#define POWDER_STEP (256/__CHASER_POWDER_NUM__)

#define baseCast(me) ((objBase*)(me))
#define chaserCast(me) ((objChaser*)(me))

//命名規則を守りやすくする的な/
#define chaserInitializer AS_OBJ_INITIALISE_FUNC(chaser)
#define SELF(name) objChaser* name = chaserCast(base)

//初期位置は大体この辺がよさそうかな/
//(312.089,-500.000,-324.060)

#define spd 7.f

static void chaserStep(objBase* base){ 
	SELF(me);
	//プレイヤーを取得/
	objBase* player = getInstPtr(&me->target);
	if(player == NULL){
		//無かったら探していったん終わる/
		me->target = getNearestInst_id(base->render->p, obj_player);
		return;
	}
	vec3 dir = v3normalize(v3sub(player->render->p, base->render->p));
	base->v = v3mul(dir, spd);
	base->render->p = v3add(base->render->p, base->v);
	base->render->angle = createBasis(dir);
}


//##################################################################
// interface:
//##################################################################

// --- destroy --- /
static void destroy(objBase* base){
	SELF(me);
	_CRT_UNUSED(me);
}
// --- set --- /
static objIInterfaceVTable interfaceVTable = {
	.destroy = destroy,//一旦/
	.playerInterface = {
		.p = NULL
	},
	.enemyInterface = {
		.p = NULL,
	}
};

//##################################################################
// public:
//##################################################################

objInitOut chaserInitializer(objBase* base){
	SELF(me);
	base->v = (vec3){ 0 };
	base->render->angle = basisZ;
	base->render->scale = v3one;
	me->target = makeInstPtr(NULL);
	getInstFromID(obj_player, &me->target, 1);//プレイヤーを取得/
	return (objInitOut){ .step = chaserStep, .model = getObjMdl(objModel_chaser), .interfaces = &interfaceVTable };
}