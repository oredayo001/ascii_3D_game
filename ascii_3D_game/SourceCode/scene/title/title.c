#include "title.h"
#include "engine/screen/screen.h"
#include "engine/graphics/render3d.h"
#include "engine/buffer/gameBuff.h"

//ƒV[ƒ“‘JˆÚ/
#include "scene/game/game.h"
//---------------------------------------------
// private:
//---------------------------------------------

typedef struct titleMembers{
	// --- ƒRƒs[ --- /
	RenderContext* rCtx;
	const KeyStates* keyStates;
	// --- ì¬ --- /

	// --- memoly --- /
	int memolyMarkar;
}titleMembers;

static titleMembers m;

static void titleInit(ISystemContext* context){
	// --- ƒƒ‚ƒŠ‚Ì€”õ --- /
	gm_d_pushStack();
	m.memolyMarkar = gm_getMarker();

	// --- ã‚ÌŠK‘w‚©‚ç‚à‚ç‚¤ --- /
	m.rCtx = context->rCtx;
	m.keyStates = &context->keyStates;
}

static void titleUpdate(){
	if(lib_keyboardCheck(vk_jump, m.keyStates->h)) requestChangeScene(gameSetFunc);
	if(lib_keyboardCheck(vk_esc, m.keyStates->p)) requestQuitMsg();
}

static void titleRender(){
	//ƒeƒXƒg/
	for(int i = 0; i < WIDTH * HEIGHT; i++){
		uint8_t c = i % a_max;
		if(i % 257 == 0) c = (rand() >> 3) % a_max;//·•ª•`‰æ‘Îô/
		m.rCtx->sc->screen[i] = c;
	}
}
static void titleFin(){
	gm_d_popStack();
	gm_freeToMarker(m.memolyMarkar);
}

//---------------------------------------------
// public:
//---------------------------------------------

SceneInitFunc titleSetFunc(Scene* scene){
	setSceneFuncs(scene, titleUpdate, titleRender, titleFin);
	return titleInit;
}

