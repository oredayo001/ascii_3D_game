#include "gameModelLoader.h"
#include "string.h"


#define TO_STRING_X(X) #X
#define TO_STRING(X) TO_STRING_X(X)

#define AS_OBJ_MODEL_PATH(name)  TO_STRING(ATTACH(ATTACH(.\\Data\\model\\obj\\,name),.obj)),
#define AS_STAGE_MODEL_PATH(name)  TO_STRING(ATTACH(ATTACH(.\\Data\\model\\stage\\,name),.obj)),

const char* gameObjMdlPathes[] = {
	GAME_MODEL_LIST_X(AS_OBJ_MODEL_PATH)
};
const char* gameStageMdlPathes[] = {
	GAME_STAGE_MODEL_LIST_X(AS_STAGE_MODEL_PATH)
};

static struct{
	allObjModels objMdls;
	Model3D stageModel;
}m;

// ####################################################################################################
// obj
// ####################################################################################################

// ----------------------------------------------------------------------------------------------------
// private:
// ----------------------------------------------------------------------------------------------------

static int _loadObjModels(allObjModels* mdls, const int* indexes, int num){
	int result = 0;
	for(int i = 0; i < num; i++){
		int index = indexes[i];
		Model3D* mdl = &(mdls->mdls[index]);
		result = pushLoadRequest(gameObjMdlPathes[index], mdl);
		if(isModelLoadError(result)) return modelLoader_error;
	}
	result = loadAllModel();
	if(isModelLoadError(result)) return modelLoader_error;
	return modelLoader_ok;
}

static void _destroyAllObjModels(allObjModels* mdls){
	for(int i = 0; i < objModel_max; i++){
		destroyModel(&(mdls->mdls[i]));
	}
}


// ----------------------------------------------------------------------------------------------------
// public:
// ----------------------------------------------------------------------------------------------------

int loadObjModels(const int* indexes, int num){
	_loadObjModels(&(m.objMdls), indexes, num);
	return 0;
}

void destroyAllObjModels(){
	_destroyAllObjModels(&(m.objMdls));
}

Model3D* getObjMdl(int i){
	return &(m.objMdls.mdls[i]);
}


// ####################################################################################################
// stage
// ####################################################################################################

// ----------------------------------------------------------------------------------------------------
// private:
// ----------------------------------------------------------------------------------------------------
static inline int modelIDtoStageModelIndex(int modelID){
	return modelID - (stageModel_MIN_MINUS1 + 1);
}
static int _loadStageModel(Model3D* dist, int modelId){
	int result = 0;
	//詰め込む/
	int index = modelIDtoStageModelIndex(modelId);
	Model3D* mdl = dist;//書き込み先/
	result = pushLoadRequest(gameStageMdlPathes[index], mdl);//読み込みリクエスト/
	if(isModelLoadError(result)) return modelLoader_error;
	//ロード/
	result = loadAllModel_temp();//読み込む/
	if(isModelLoadError(result)) return modelLoader_error;
	return modelLoader_ok;
}

// ----------------------------------------------------------------------------------------------------
// public:
// ----------------------------------------------------------------------------------------------------
int loadStageModel(int modelId){
	return _loadStageModel(&(m.stageModel), modelId);
}

void destroyStageModel(){
	destroyModel(&(m.stageModel));
}

Model3D* getStageModel(){
	return (&m.stageModel);
}
