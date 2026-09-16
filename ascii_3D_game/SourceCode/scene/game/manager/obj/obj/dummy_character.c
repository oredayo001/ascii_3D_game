#include"../objManager.h"
#include"scene/commonManager/gameModelLoader.h"
#include"dummy_character.h"
#include"scene/commonManager/input.h"
#include"scene/game/manager/map/mapManager.h"

#define baseCast(me) ((objBase*)me)
#define dummyCast(me) ((objDummy_debug*)me)

//–½–¼‹K‘¥‚ðŽç‚è‚â‚·‚­‚·‚é“I‚È/
#define dummyInitializer AS_OBJ_INITIALISE_FUNC(dummy_debug)
#define SELF(name) objDummy_debug* name = dummyCast(base)

//ƒ‰ƒ“ƒ_ƒ€‚ÉŽ€–S/
static void randDeath(objBase* base){
	SELF(me);
	//me->pp->p.x = 0.f;//ŒÃ‚¢render‚Ì‘‚«ž‚Ý@‚»‚Ì‚¤‚¿Á‚·/
	if(!(rand() & 0x3ff)){
		instanceDestroy(base);
	}
}

// --- destroy --- /
static void destroy(objBase* base){
	objDummy_debug* me = ((objDummy_debug*)base);
	_CRT_UNUSED(me);
}

objIInterfaceVTable interfaces = {
	.destroy = destroy,
};

objInitOut dummyInitializer(objBase* base){
	SELF(me);
	base->v = (vec3){ 0 };
	base->render->angle = basisZ;
	base->render->scale = v3one;
	me->pp = base->render;
	return (objInitOut){ .step = randDeath, .model = getObjMdl(objModel_player), .interfaces = &interfaces };
}