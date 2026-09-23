#pragma once


#include"macro/macro.h"
#include"engine/graphics/loader/modelLoader.h"

#define GAME_MODEL_LIST_X(X)\
X(player)\
X(chaser)\
X_MACRO_END
#define GAME_STAGE_MODEL_LIST_X(X)\
X(stageDemo)\
X(stage01)\
X_MACRO_END

#define AS_GAME_MODEL_LIST_ENUM_X(name) ATTACH(objModel_,name),
#define AS_GAME_STAGE_MODEL_LIST_ENUM_X(name) ATTACH(stageModel_,name),
enum gameModel{
	model_none = -1,
	GAME_MODEL_LIST_X(AS_GAME_MODEL_LIST_ENUM_X)
	//objModelの最大/
	AS_GAME_MODEL_LIST_ENUM_X(max)
	//stageModelの最小-1
	AS_GAME_STAGE_MODEL_LIST_ENUM_X(MIN_MINUS1)
	GAME_STAGE_MODEL_LIST_X(AS_GAME_STAGE_MODEL_LIST_ENUM_X)
	AS_GAME_STAGE_MODEL_LIST_ENUM_X(max)
};







typedef struct allObjModels{
	Model3D mdls[objModel_max];
}allObjModels;

// --- obj --- /

//in,out
//モデルidの配列を入れる/
int loadObjModels(const int* indexes, int num);

//キャラのモデルの開放/
void destroyAllObjModels();

//モデルのアドレスの取得/
Model3D* getObjMdl(int i);

// --- stage --- /

//ステージのモデルを取得/
int loadStageModel(int modelId);

//モデルの開放/
void destroyStageModel();

//読み込んでるステージのモデルを取得/
Model3D* getStageModel();