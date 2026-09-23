#include"../objManager.h"
#include"scene/commonManager/gameModelLoader.h"
#include"player.h"
#include"scene/commonManager/input.h"
#include"scene/game/manager/map/mapManager.h"

#define baseCast(me) ((objBase*)(me))
#define chaserCast(me) ((objChaser*)(me))

//命名規則を守りやすくする的な/
#define chaserInitializer AS_OBJ_INITIALISE_FUNC(chaser)
#define SELF(name) objChaser* name = chaserCast(base)

//初期位置は大体この辺がよさそうかな/
//(312.089,-500.000,-324.060)


static void doNothing(objBase* base){ _CRT_UNUSED(base); }


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

//objInitOut playerInitializer(objBase* me);//提案がうっとおしかったからここでプロトタイプ宣言/
objInitOut chaserInitializer(objBase* base){
	SELF(me);
	_CRT_UNUSED(me);
	base->v = (vec3){ 0 };
	base->render->angle = basisZ;
	base->render->scale = v3one;
	return (objInitOut){ .step = doNothing, .model = getObjMdl(objModel_chaser), .interfaces = &interfaceVTable };
}