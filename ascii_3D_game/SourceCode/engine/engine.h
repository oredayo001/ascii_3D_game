#pragma once

#include "common.h"

struct RenderContext;
typedef struct RenderContext RenderContext;

//寿命がプログラム終了まで/
typedef struct ISystemContext{
	RenderContext* rCtx;
	KeyStates keyStates;
	void* gameData;//シーン同士をつなぐデータ　caming soon/
}ISystemContext;
typedef void (*SceneFunc)();
typedef struct Scene{
	SceneFunc update;
	SceneFunc render;
	SceneFunc fin;
}Scene;

typedef void (*SceneInitFunc)(ISystemContext*);

/*ex:
SceneInitFunc titleSet(Scene*scene){
	scene->update = titleUpdate;
	scene->render = titleRender;
	scene->fin = titlerFin;
	return titleInit;
}
*/
typedef SceneInitFunc(*SceneSetFunc)(Scene*);


static inline void setSceneFuncs(Scene* v, SceneFunc update, SceneFunc render, SceneFunc fin){
	v->update = update;
	v->render = render;
	v->fin = fin;
}

//シーン変更/
void requestChangeScene(SceneSetFunc);
//ゲーム終了/
void requestQuitMsg();

//最初のシーンを入れる/
void engineInit(SceneSetFunc firstSceneFunc);

int engineUpdate();

void engineRender();

void engineFin();
