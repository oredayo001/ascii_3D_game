#pragma once

#include"common.h"
#include"objDef.h"

// --- 管理者が呼ぶ --- /

//初期化/
void initInstances();
//更新/
void updateInstances();
//描画/
void renderInstances();
//終わり ゲーム終了時とかに/
void finInstances();

// --- ゲームロジック側が呼ぶ --- /

//インスタンスを全消去するだけ/
//一区切りついたときに呼ぶ/
void destroyInstances();

// --- 誰でも呼べる --- /

//作成 objID:id_obj.../
objBase* instanceCreate(uint16_t objID, vec3 p);

static inline int hasAttribute_any(objBase* inst, uint64_t att){ return !!(inst->attribute & att); }
static inline int hasAttribute_all(objBase* inst, uint64_t att){ return (inst->attribute & att) == att; }

static inline void instanceDestroy(objBase* inst){
	inst->isActive = 0;
}

// --- objのポインタを安全に持つ仕組み --- /

//キャラのポインタを入れるためのもの/
typedef union{
	void* _____align_ptr___DONT_USE_____[2];
} InstPtr;//偽物の型/

//本物の型/
#define REAL_INST_PTR_STRUCT \
union { \
struct{\
    objBase* ptr; \
    uint32_t generation; \
}p;\
	InstPtr __false;/*厳密ななんやかんや対策　になってるかは知らんけど*/\
}

#define DEF_REAL_INST_PTR(T) typedef REAL_INST_PTR_STRUCT T

//キャラのポインタを2f以上保持しておきたいときに使うやつ/
//NULL可能/
static inline InstPtr makeInstPtr(objBase* target){
	DEF_REAL_INST_PTR(real_instPtr);

	//サイズが違うかったらはじく 関数の中でやってるのはローカルの型だから/
	static_assert(sizeof(InstPtr) == sizeof(real_instPtr), "objRefとrealObjRefとのサイズが合わない");

	InstPtr r = { 0 };
	real_instPtr* real = (real_instPtr*)&r;
	if(target != NULL){
		//何かしらはさしてる/
		real->p.ptr = target;
		real->p.generation = target->generation;
	}
	return r;
}

//生きてるか死んでるか/
static inline int isAliveInstPtr(const InstPtr* _ref){
	DEF_REAL_INST_PTR(real_instPtr);

	const real_instPtr* ref = (const real_instPtr*)_ref;
	if(ref->p.ptr == NULL) return 0;
	return ref->p.ptr->generation == ref->p.generation;
}

//死んでたらNULLが返る/
static inline objBase* getInstPtr(const InstPtr* _ref){
	DEF_REAL_INST_PTR(real_instPtr);

	if(isAliveInstPtr(_ref)){
		const real_instPtr* ref = (const real_instPtr*)_ref;
		return ref->p.ptr;
	}
	//死んでたらnull
	return NULL;
}
//ほかで使われたら困るから消す/
#undef DEF_REAL_INST_PTR
#undef REAL_INST_PTR_STRUCT

#define SEARCH_ALL_INST -1
//インスタンスを探すidから探す 全インスタンスを探す場合SEARCH_ALL_INST/
int getInstFromID(int id, InstPtr* dist, int num);
//インスタンスを探す属性から探す 全インスタンスを探す場合SEARCH_ALL_INST/
int getInstHasAttribute_all(uint64_t targetAttribute, InstPtr* dist, int num);
//インスタンスを探す属性から探す 全インスタンスを探す場合SEARCH_ALL_INST/
int getInstHasAttribute_any(uint64_t targetAttribute, InstPtr* dist, int num);
//一番近いインスタンスをidから探す/
InstPtr getNearestInst_id(vec3 p, int id);
//一番近いインスタンスを属性から探す/
InstPtr getNearestInst_allAtt(vec3 p, uint64_t targetAttribute);
//一番近いインスタンスを属性から探す/
InstPtr getNearestInst_anyAtt(vec3 p, uint64_t targetAttribute);