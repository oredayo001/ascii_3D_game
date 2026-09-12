#include "scenetemplate.h"
#include "engine/screen/screen.h"
#include "engine/graphics/render3d.h"

//---------------------------------------------
// private:
//---------------------------------------------

typedef struct scenetemplateMembers{
	// --- ƒRƒs[ --- /
	RenderContext* rCtx;
	const KeyStates* keyStates;
	// --- ì¬ --- /
}scenetemplateMembers;

static scenetemplateMembers m;

static void scenetemplateInit(ISystemContext* context){
	// --- ã‚ÌŠK‘w‚©‚ç‚à‚ç‚¤ --- /
	m.rCtx = context->rCtx;
	m.keyStates = &context->keyStates;
}

static void scenetemplateUpdate(){
}
static void scenetemplateRender(){
	for(int i = 0; i < WIDTH * HEIGHT; i++){
		m.rCtx->sc->screen[i] = rand() % a_max;
	}
}
static void scenetemplateFin(){
}

//---------------------------------------------
// public:
//---------------------------------------------

SceneInitFunc scenetemplateSetFunc(Scene* scene){
	setSceneFuncs(scene, scenetemplateUpdate, scenetemplateRender, scenetemplateFin);
	return scenetemplateInit;
}