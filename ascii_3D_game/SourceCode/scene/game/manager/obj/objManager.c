#include "objManager.h"
#include "allocator/allocator.h"
#include "macro/macro.h"
#include "engine/graphics/render3d.h"
#include "engine/buffer/gameBuff.h"

//objcts
#include "obj/player.h"

//######################################################################
// DEBUG
//######################################################################

//もしうまく動かないときのデバッグ用　アロケーターが悪いかを調べるときにしか使わない/
#define UNUSE_MY_ALLOCATOR 0

//######################################################################
// private:
//######################################################################

// ===================================
// アロケーター
// ===================================

#define AS_UNION(name,type,...) type ATTACH(__,name);//nameは重複しない/
typedef union{
	OBJECTS_LIST_X(AS_UNION)
}MaxObjSize;

// --- アロケーター --- /

//適当なサイズ 64B*n/
#define OBJ_POOL_SIZE (64*64)

#define OBJ_MAX_SIZE (sizeof(MaxObjSize))//1つのオブジェクトのサイズの最大値/
#define OBJ_MIN_SIZE (sizeof(objBase))//1つのオブジェクトのサイズの最小値/

//リストの数/
#define ALLOCATOR_LIST_SIZE (( ((OBJ_MAX_SIZE - OBJ_MIN_SIZE) + (OBJ_ALIGNMENT_SIZE - 1))/OBJ_ALIGNMENT_SIZE ) + 1)

//可読性を上げるためだけの型/
typedef struct List{
	struct List* next;
}List;

static struct{
	LiniarAllocator allocator;
	List* list[ALLOCATOR_LIST_SIZE];
}objAllocator;//オブジェクト管理者がオブジェクトに与えるメモリを管理するやつ/

// --- アロケーター自体の管理をする奴ら ---

//作成/
static void initAllocator(){
#if UNUSE_MY_ALLOCATOR
	return;
#endif
	objAllocator.allocator.p = (uint64_t*)gm_allocate(OBJ_POOL_SIZE);
	ASSERT(objAllocator.allocator.p != NULL, "objAllocatorがNULL");
	setLAParam(&objAllocator.allocator, OBJ_POOL_SIZE);
}
//リセット/
//ステージ切り替えとかに使えたらいいね/
static void resetAllocator(){
#if UNUSE_MY_ALLOCATOR
	return;
#endif
	//null埋め/
	for(int i = 0; i < ALLOCATOR_LIST_SIZE; i++){
		objAllocator.list[i] = NULL;
	}
	//cntを0にする/
	clearLA(&(objAllocator.allocator));
}
//終了/
static void destroyAllocator(){
#if UNUSE_MY_ALLOCATOR
	return;
#endif
	resetAllocator();
	//listはallocatorが管理してるポインタを使ってるからこいつをdestroyするだけでいい/
	//free(objAllocator.allocator.p);
	clearLAParam(&objAllocator.allocator);
}

//byteからリストのインデックスに変換/
static int allocator_sizeToIndex(size_t size){
	return (int)((size - OBJ_MIN_SIZE) / OBJ_ALIGNMENT_SIZE);
}

// --- アロケータを使うやつら --- /

//リストからメモリをもらう　なかったらリニアアロケータからもらう/
static objBase* objAllocate(size_t size){
#if UNUSE_MY_ALLOCATOR
	return malloc(size);
#endif
	ASSERT((OBJ_MIN_SIZE <= size) && (size <= OBJ_MAX_SIZE), "allocate(obj) に 変なサイズ が渡された");
	int index = allocator_sizeToIndex(size);
	List* front = (void*)(objAllocator.list[index]);
	objBase* r = NULL;
	if(front == NULL){//無い/
		//アロケーターからもらう/
		r = allocateLA(&(objAllocator.allocator), size);
	}
	else{//ある/
		//次を先頭に持ってくる/
		r = (void*)front;
		List* next = objAllocator.list[index]->next;
		objAllocator.list[index] = next;
	}
	return r;
}

//インスタンスのポインタとサイズを渡す/
//!ポインタのポインタ/
static void objFree(objBase** _mem, size_t size){
#if UNUSE_MY_ALLOCATOR
	free(*_mem);
	*_mem = NULL;
	return;
#endif
	//キャスト/
	List* mem = (List*)*_mem;//返却された奴自身に次のメモリを書き込むようにする/
	*_mem = NULL;

	//インデックス計算/
	ASSERT((OBJ_MIN_SIZE <= size) && (size <= OBJ_MAX_SIZE), "freeObj に 変なサイズ が渡された");
	int index = allocator_sizeToIndex(size);

	//セット/
	mem->next = objAllocator.list[index];//まあどうせ8byte以上やから問題ないはず/
	objAllocator.list[index] = mem;
}

// ===================================
// obj
// ===================================

// ---------------------------------------------------------
// オブジェクト生成に必要なテーブルやらプロトタイプ宣言やらの集まり
// ---------------------------------------------------------

#pragma region 関数ポインタテーブルとサイズテーブルを作るためのマクロやら

//プロトタイプ宣言自動生成マクロ/
#define AS_OBJ_INITIALISE_FUNC_PROTOTYPE(name,...) objInitOut AS_OBJ_INITIALISE_FUNC(name)(objBase*);

//ここで全オブジェクトの初期化関数をプロトタイプ宣言/
OBJECTS_LIST_X(AS_OBJ_INITIALISE_FUNC_PROTOTYPE)

//baseが最初になかったらはじくアサート/
#define AS_OBJ_STATIC_ASSERT_BASE(name,type,...) static_assert(offsetof(type,base)==0,"type\"" #type "\"of offset is not 0");
//アサートを一気にセットするやつ/
OBJECTS_LIST_X(AS_OBJ_STATIC_ASSERT_BASE);

typedef objInitOut(*objInitializeFunc)(objBase*);

//テーブル自動生成マクロ/
#define AS_OBJ_INITIALIZE_TABLE(name,type,...) AS_OBJ_INITIALISE_FUNC(name),
#define AS_OBJ_SIZE_TABLE(temp,type,...) sizeof(type),
#define AS_OBJ_ATTRIBUTE_BIT_MASKLIST(temp,temp2,mask,...) mask,

#pragma endregion ここまでがテンプレ


//こいつが全オブジェクトの初期化関数のテーブル/
static objInitializeFunc const objInitializeFuncTable[] = {
	OBJECTS_LIST_X(AS_OBJ_INITIALIZE_TABLE)
};

//こいつが全オブジェクトのsizeのテーブル/
static const size_t objSizeTable[] = {
	OBJECTS_LIST_X(AS_OBJ_SIZE_TABLE)
};//今思ったけどこいつ全キャラの構造体の定義インクルードせんとエラー吐くやん/


//こいつが全オブジェクトの属性のテーブル/
static const uint32_t objAttributeTable[] = {
	OBJECTS_LIST_X(AS_OBJ_ATTRIBUTE_BIT_MASKLIST)
};

// ---------------------------------------------------------
// ここからが天ぷら
// ---------------------------------------------------------

//死んでる世代/
#define GENERATION_DEAD (0)
//最大/
#define MAX_OBJECTS 64
static struct{
	//添え字同じ奴が同じやつに対応してる(?)/
	objRender instancesRender[MAX_OBJECTS];
	objBase* instances[MAX_OBJECTS];//描画には使われない部分/
	int cnt;
	int currentGeneration;
}m;

// --- 個々の操作 --- /
static void eraseInstance(int instanceId){
	//カウントを戻す/
	m.cnt--;//戻す前のcntの場所は一番最後の一つ次の場所/
	ASSERT((0 <= m.cnt), "インスタンスがいないはずなのにdestroyが呼ばれた");
	ASSERT((0 <= instanceId && instanceId <= m.cnt), "無効なinstanceID");
	//まず破壊関数を呼び出す/
	m.instances[instanceId]->interfaces->destroy(m.instances[instanceId]);

	//インスタンス取得/
	objBase** p_inst = &(m.instances[instanceId]);//今から消すやつのポインタ/
	objBase* inst = *p_inst;//今から消すやつのポインタ/

	//消す前に世代変数を死んでる値に変更/
	(*p_inst)->generation = GENERATION_DEAD;

	//メモリ返却/
	int objId = inst->objID;
	size_t size = objSizeTable[objId];//idからサイズを取得/
	objFree(p_inst, size);//返却/

	if(m.cnt != instanceId){//後ろと違うときだけ交換/
		//後ろを前に持ってくる/
		//cnt:移動元　instanceID:消去対象 兼 移動先/
		
		//render
		m.instancesRender[instanceId] = m.instancesRender[m.cnt];//後ろを消すキャラの床に上書き/
		//inst
		m.instances[m.cnt]->render = &m.instancesRender[instanceId];//後ろにいたキャラのrenderの順番が変わったからアドレスを更新/
		m.instances[instanceId] = m.instances[m.cnt];//後ろを消すキャラに上書き/
	}
	//一応/
	m.instances[m.cnt] = NULL;
}

// --- 全インスタンスに対する操作 --- /

static void updateAllInstances(){
	// --- 更新 --- /
	for(int i = 0; i < m.cnt; i++){
		//アドレスをもらう/
		objBase* inst = m.instances[i];
		++inst->timer;
		inst->step(inst);
	}
	// --- 当たり判定 --- /

	//LOW 当たり判定 プレイヤー出してカメラ出来て地形との当たり判定できたそのあと/

	// --- activeチェック --- /
	//逆順/
	for(int i = m.cnt - 1; 0 <= i; i--){
		//アドレスをもらう/
		objBase* inst = m.instances[i];
		if(!inst->isActive){//死んでる/
			eraseInstance(i);//すでにチェック済みの後ろを今の場所に持ってくる/
			//すでにチェック済みの後ろをiの移動させるからiはもう一度確認する必要がない/
		}
	}
}

//全員を描画(まあキューに入れるだけ)
static void renderAllInstances(){
	for(int i = 0; i < m.cnt; i++){
		//アドレス/
		objRender* inst = &m.instancesRender[i];
		pushModelBS3(inst->model, inst->p, inst->angle, inst->scale);
	}
}

//全部返却するだけ/
static void _destroyInstances(){
#if UNUSE_MY_ALLOCATOR
	//一応個別で解放/
	for(int i = 0; i < m.cnt; i++){
		objFree(&m.instances[i], 0);
	}
	return;
#endif
	resetAllocator();//全部返却/
	m.cnt = 0;
	//まあ初期化は要らんやろ/
}

//初期化関数ポインタの型/
typedef objInitOut(*initFunc)(objBase*);
static objBase* spawnObj(initFunc initializer, size_t size,uint32_t attribute, vec3 p){
	ASSERT(m.cnt < MAX_OBJECTS, "オブジェクトあふれ");
	//メモリをもらう/
	objBase* r = objAllocate(size);

	//セット/
	m.instances[m.cnt] = r;
	r->render = &m.instancesRender[m.cnt];
	//初期化/
	r->render->p = p;//
	r->isActive = 1;
	objInitOut out = initializer(r);
	r->render->model = out.model;
	r->step = out.step;
	r->interfaces = out.interfaces;
	r->timer = 0;
	r->attribute = attribute;

	if(m.currentGeneration == GENERATION_DEAD)m.currentGeneration = GENERATION_DEAD+1;//念のため まあ万が一 一周したときのためのやつ/
	r->generation = m.currentGeneration++;
	//カウント/
	m.cnt++;

	return r;
}

//######################################################################
// public:
//######################################################################

// --- 管理者向け　game側から呼び出される奴 --- /

//インスタンスの初期化/
void initInstances(){
	initAllocator();
	ASSERT(m.cnt == 0, "なぜかinstancesのcntが0じゃない");
}

//全更新/
void updateInstances(){
	updateAllInstances();
}

//全描画/
void renderInstances(){
	renderAllInstances();
}

//メモリを返却するだけ/
void destroyInstances(){
	_destroyInstances();
}

//メモリの開放までする/
void finInstances(){
	m.cnt = 0;
	destroyAllocator();
}

// --- どこからでも呼び出されるやつ --- /

objBase* instanceCreate(uint16_t objID, vec3 p){
	objBase* r = spawnObj(objInitializeFuncTable[objID], objSizeTable[objID], objAttributeTable[objID], p);
	r->objID = objID;
	return r;
}

/*
必要	な奴ら
=====================
--ここだけ--
	 オブジェクトの破壊 activeチェックの時に非アクティブな時に後ろを持ってくるやつ/
=====================
--gameから--
	--基本--
	 初期化		メモリの確保
	 更新		全インスタンスの更新->当たり判定->activeチェック
	 描画		全インスタンスを描画(スタックに入れる)
	 終了		メモリの開放
	--たまに--
	 リセット		アロケータのメモリを全部線形に返却してリストをリセット
=====================
--いろんなとこから--
	 生成		メモリの貸し出しとそのオブジェの初期化関数を呼ぶ
=====================
あといろいろ
*/