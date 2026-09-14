#define _CRT_SECURE_NO_WARNINGS
#include "modelLoader.h"

#include "engine/buffer/gameBuff.h"
#include "textureLoader.h"
#include "engine/fileio/fileio.h"

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <float.h>

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

//######################################################################
// private:
//######################################################################

//今思ったけど型名おかしくね?LoadRequestやろ　何renderって　描くわけでもないのに　頭おかしいやろ/
typedef struct ModelRenderRequest{
	const char* path;//パス/
	Model3D* mdl;//書き込み先/
}ModelRenderRequest;
#define MODEL_LOAD_MAX 32
typedef struct ModelLoadList{
	ModelRenderRequest r[MODEL_LOAD_MAX];//キュー/
	int cnt;//カウント/

	int memoly_backMarker;
}ModelLoadList;

ModelLoadList* list;
static void modelListDestroy(ModelLoadList** l, int needFree){
	if(needFree)gm_free_back_to_marker((*l)->memoly_backMarker);
	//free(*l);
	*l = NULL;
}

static int _loadMtlFile_texture(FILE* mtlFile, const char* mtlName, const char* basePath){
	//戻る/
	rewind(mtlFile);

	//読み取る/
	char line[256];
	while(fgets(line, sizeof(line), mtlFile)){
		if(!memcmp(line, "newmtl", sizeof("newmtl") - 1)){
			char mtlName_l[256] = { 0 };
			sscanf_s(line, "newmtl %s", mtlName_l, (uint32_t)sizeof(mtlName_l));

			int check = 0;
			int len1 = strlen(mtlName);
			int len2 = strlen(mtlName_l);
			if(len1 != len2)continue;
			if(!memcmp(mtlName_l, mtlName, len1))goto findMTL;//一致/
		}
	}
	ASSERT(0, "マテリアルが見つからんかった");
	return -1;
findMTL:
	while(fgets(line, sizeof(line), mtlFile)){
		if(!memcmp(line, "map_Kd", sizeof("map_Kd") - 1)){
			char filePath_rel[256];
			char filePath[256];
			sscanf_s(line, "map_Kd %s", filePath_rel, (uint32_t)sizeof(filePath_rel));
			attachFilePath(basePath, filePath_rel, filePath);
			return loadTexture(filePath);
		}
		if(!memcmp(line, "newmtl", sizeof("newmtl") - 1))break;//次のマテリアルを読んでる/
	}
	ASSERT(0, "テクスチャが見つからんかった");//今んとこはないとおかしい/
	return -1;
}

#define MAX_TEX_SWITCH 32

//まあなんかバイナリファイルとかに変更したらここを差し替えればいいはず　でまあobjからバイナリコードに変換するやつがいるかもな/
static int _loadAllModel(int isTemp){
	void* (*allocator)() = isTemp ? gm_allocate_back : gm_allocate;
	//何もない場合何もないのにlistの開放をしてしまう よろしくない　listの開放でnullチェックするべきでもあるﾝかもしれんけどどうなんやろ/
	if(list == NULL) return 0;//何もない/
	//サイズは読んでみるまで分からんから一旦一時ででかいメモリとっとく　そんで後で解放する　まあ別にわからんこともないけどめんどいしな/
	//サイズ/
	size_t vSize = sizeof(vec3) * MAX_V_CNT;
	size_t iSize = sizeof(int) * MAX_V_CNT;
	size_t uv_iSize = sizeof(int) * MAX_V_CNT;
	size_t uvSize = sizeof(vec2) * MAX_V_CNT;

	size_t total = vSize + iSize + uv_iSize + uvSize;
	//一時メモリ確保/
	int gmMarker = gm_getMarker_back();
	uint8_t* tempMemory = (uint8_t*)gm_allocate_back(total);//1byte型/

	//全員4の倍数だからアライメントは気にしなくていい/
	vec3* vertices = (vec3*)(tempMemory);
	tempMemory += vSize;
	int* indices = (int*)(tempMemory);
	tempMemory += iSize;
	int* uv_indices = (int*)(tempMemory);
	tempMemory += uv_iSize;
	vec2* uvs = (vec2*)(tempMemory);

	//デバッグ用/
#if ENABLE_DEBUG
	for(int i = 0; i < list->cnt; i++){
		Model3D* mdl = list[i].r->mdl;
		ASSERT(mdl->norms == NULL, "法線初期化不足");
		ASSERT(mdl->vertices == NULL, "頂点初期化不足");
		ASSERT(mdl->uv == NULL, "uv初期化不足");
	}
#endif

	//読み込み開始/
	for(int i = 0; i < list->cnt; i++){
		ModelRenderRequest* request = &list->r[i];
		FILE* fp = NULL;
		errno_t err = fopen_s(&fp, request->path, "r");
		//errno_t err = fopen_s(&fp, "./Data/model/obj/player.obj", "r");
		ASSERT(!err, "ファイルが開けなかった");
		int icnt = 0;
		int vcnt = 0;
		int uvcnt = 0;
		char line[256];
		bbox_t bbox = {
		.min = { FLT_MAX,  FLT_MAX,  FLT_MAX },
		.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX }
		};
		FILE* mtlFile = NULL;
		mdlTextureRLE* texHead = gm_allocate_back(MAX_TEX_SWITCH * sizeof(mdlTextureRLE));
		mdlTextureRLE* currentTexture = texHead;
		int switched = 0;
		*currentTexture = (mdlTextureRLE){ 0 };
		currentTexture->index = -1;
		// --- ファイル読み込み --- /
		while(fgets(line, sizeof(line), fp)){
			// マテリアルファイル/
			if(!memcmp(line, "mtllib", sizeof("mtllib") - 1)){
				if(mtlFile != NULL) fclose(mtlFile);
				char txt_fileName[256 + 3 + 1];
				char txt_path[256 + 3 + 1];
				if(sscanf_s(line, "mtllib %s", txt_fileName, (uint32_t)sizeof(txt_fileName))){
					attachFilePath(request->path, txt_fileName, txt_path);
					err = fopen_s(&mtlFile, txt_path, "r");
					ASSERT(!err, "ファイルが開けなかった");
				}
				else{
					ASSERT(0, "マテリアル読み込み失敗");
				}
			}
			//マテリアル変更/
			else if(!memcmp(line, "usemtl", sizeof("usemtl") - 1)){
				ASSERT(mtlFile != NULL, "マテリアルファイルを読み込めてないのに待てリファイルの変更が呼ばれた?");
				int mtlIndex = 0;
				//0個の場合はずらさない/
				if(currentTexture->cnt != 0){
					switched++;
					ASSERT(switched < MAX_TEX_SWITCH, "テクスチャの切り替え数が多い");
					currentTexture++;
					*currentTexture = (mdlTextureRLE){ 0 };
				}
				char mtlName[256];
				sscanf_s(line, "usemtl %s", mtlName, (uint32_t)sizeof(mtlName));
				mtlIndex = _loadMtlFile_texture(mtlFile, mtlName, request->path);
				currentTexture->index = mtlIndex;
			}
			// uv/
			else if(line[0] == 'v' && line[1] == 't'){
				float x, y;
				// 文字列から3つの浮動小数点数を抽出/
				if(sscanf_s(line + 2, "%f %f", &x, &y) == 2){
					uvs[uvcnt++] = (vec2){ x, 1.f - y };//blenderでは左下が原点になってることが多いらしい/
				}
			}
			else if(line[0] == 'v' && line[1] == ' '){
				float x, y, z;
				// 文字列から3つの浮動小数点数を抽出/
				if(sscanf_s(line + 2, "%f %f %f", &x, &y, &z) == 3){
					vertices[vcnt++] = (vec3){ x,y,z };
					bbox.min = (vec3){ MIN(x,bbox.min.x),MIN(y,bbox.min.y),MIN(z,bbox.min.z) };
					bbox.max = (vec3){ MAX(x,bbox.max.x),MAX(y,bbox.max.y),MAX(z,bbox.max.z) };
				}
			}
			// 先頭がf/
			else if(line[0] == 'f' && line[1] == ' '){
				currentTexture->cnt++;
				int index[3] = { 0 };
				int uvIndex[3] = { 0 };
				// f 1/1/1 2/2/2 3/3/3
				// f 頂点/uv/法線 頂点/uv/法線 頂点/uv/法線 って感じで並んでる/
				// /%*[^ ] は%[^文字]ってのがあって文字まで変数に突っ込むって感じで*がついてたら突っ込まずに文字まで虫って感じにしてくれる/
				// つまり今回の場合空白まで無視ってこと/
				int parsed = sscanf_s(line, "f %d/%d%*[^ ] %d/%d%*[^ ] %d/%d", &index[0], &uvIndex[0], &index[1], &uvIndex[1], &index[2], &uvIndex[2]);

				//f 1 2 3
				//だったときのためのやつ/
				if(parsed != 6){
					parsed = sscanf_s(line, "f %d %d %d", &index[0], &index[1], &index[2]);//ブレークポイントでここは通らなかった/
					ASSERT(0, "謎 parsed != 6");
				}

				if(parsed == 6){
					//詰める/
					for(int j = 0; j < 3; j++){
						indices[icnt] = index[j] - 1;// objの1始まりを0始まりにする/
						uv_indices[icnt] = uvIndex[j] - 1;// objの1始まりを0始まりにする/
						icnt++;
					}
				}
				if(parsed == 3){
					//詰める/
					for(int j = 0; j < 3; j++){
						indices[icnt++] = index[j] - 1;// objの1始まりを0始まりにする/
					}
				}
			}
		}
		if(mtlFile != NULL) fclose(mtlFile);
		fclose(fp);
		// --- モデル作成 --- /
		Model3D* model = request->mdl;
		//メモリ確保/
		size_t texRLESize = currentTexture - texHead;
		//if(texRLESize || currentTexture->cnt){
		size_t texCpySize = (texRLESize + 1) * sizeof(mdlTextureRLE);
		model->txInfo = (mdlTextureRLE*)allocator(texCpySize);
		memcpy(model->txInfo, texHead, texCpySize);
		//}
		int triNum = icnt / 3;
		size_t varticleSize = sizeof(vec3) * icnt;
		size_t nSize = sizeof(vec3) * triNum;
		size_t uvSize = sizeof(vec2) * icnt;
		model->vertices = (vec3*)allocator(varticleSize);
		model->norms = (vec3*)allocator(nSize);
		model->uv = (vec2*)allocator(uvSize);

		//三角形の数/
		model->triCnt = triNum;
		//頂点を展開/
		for(int j = 0; j < icnt; j++){
			int index = indices[j];
			model->vertices[j] = vertices[index];
			int uv_index = uv_indices[j];
			model->uv[j] = uvs[uv_index];
		}
		//法線を計算/
		//objの法線は頂点毎らしいからこっちで面ごとのを計算する/
		for(int j = 0; j < triNum; j++){
			vec3 v0 = model->vertices[(j * 3) + 0];
			vec3 v1 = model->vertices[(j * 3) + 1];
			vec3 v2 = model->vertices[(j * 3) + 2];
			vec3 edge1 = v3sub(v1, v0);
			vec3 edge2 = v3sub(v2, v0);
			model->norms[j] = v3normalize(v3cross(edge1, edge2));
		}
	}
	//一時メモリの消去/
	//free(tempMemory);
	//listの解放 まあ使いまわすことはないであろう というか使いまわすなら同じモデルが2度生成されることになるんか?/
	if(!isTemp)	gm_free_back_to_marker(gmMarker);
	modelListDestroy(&list, !isTemp);
	//成功/
	return modelLoader_ok;
}

//######################################################################
// public:
//######################################################################

//パスと読み込み先をキューに詰める/
int pushLoadRequest(const char* path, Model3D* target){
	//nullなら新しく作成/
	if(list == NULL){
		int marker = gm_getMarker_back();
		//一時的に使うけどスタックには影響するとよくないからこっち/
		list = (ModelLoadList*)gm_allocate_back(sizeof(ModelLoadList));
		if(list == NULL) return modelLoader_error;
		list->cnt = 0;
		list->memoly_backMarker = marker;
	}
	ASSERT(list->cnt < MODEL_LOAD_MAX, "loadRequestあふれ");
	//パスと書き込み先を入れる/
	list->r[list->cnt].path = path;
	list->r[list->cnt].mdl = target;
	list->cnt++;
	return modelLoader_ok;
}

int loadAllModel(){
	return _loadAllModel(0);
}
//呼び出し元が後ろの開放をする/
int loadAllModel_temp(){
	return _loadAllModel(1);
}

//モデルの開放/
void destroyModel(Model3D* model){
	//解放/
	//free(model->norms);
	//free(model->vertices);
	//free(model->uv);
	//塗るぽ埋め/
	model->norms = NULL;
	model->vertices = NULL;
	model->uv = NULL;
	model->triCnt = 0;
}