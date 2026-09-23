#include "thread.h"

//engine
#include "engine/buffer/gameBuff.h"

//lib
#include <Windows.h>

#if USE_THREAD

typedef struct threadJob{
	void (*job)(void* arg);
	void* arg;
}threadJob;

typedef struct threadWR{//スレッドが読み書きできるスペース/
	int isWorking;//0:実行中　1:休憩中　もしくは休憩してないのに仕事がない状態/
	int isSleeping;//休憩中か/
	uint8_t currentJob;//今実行中の場所/
	//pad3byte
	int finishedJob;//debug
}threadWR;
typedef struct mainWR{//メインスレッドが読み書きできるスペース/
	int id;
	int canSleep;//眠っていいかどうか/
	int isActive;//生きてるか/
	uint8_t currentJob;//次書き込む場所/
	//pad3byte
	HANDLE workEvent;//旗らけシグナル(?)/
	threadJob job[256];//仕事　リングバッファ/
	int gaveJob;//debug
	//pad4
}mainWR;
typedef struct threadInfo{
	threadWR threadMem;//スレッドが読み書き可能 メインスレッドはリードオンリー/
	mainWR mainMem;//メインスレッドが読み書き可能 スレッドはリードオンリー/
	//currentJobがuint8_tなのはオーバーフローしたら勝手に0を指すようになるから/
}threadInfo;
struct thread_g_mem{
	HANDLE* threads;
	threadInfo* info;
	int threadNum;
}m;
//休憩 子スレッドが実行/
static inline void sleepThread(threadInfo* info){
	const mainWR* readonly = &info->mainMem;
	threadWR* writeable = &info->threadMem;//子スレッドはmainMemがリードオンリー/
	writeable->isWorking = 0;
	writeable->isSleeping = 1;
	WaitForSingleObject(readonly->workEvent, INFINITE);
	writeable->isSleeping = 0;
}

//######################################################################
// private:
//######################################################################

//子スレッドが実行/
static DWORD WINAPI threadWork(LPVOID lpPalam){
	threadInfo* myInfo = (threadInfo*)lpPalam;
	const mainWR* readonly = &myInfo->mainMem;//子スレッドはmainMemがリードオンリー/
	threadWR* writeable = &myInfo->threadMem;
	while(readonly->isActive){//死んでたらループを抜けて終了/
		if(readonly->currentJob != writeable->currentJob){//同じ場合は終わってる/
			writeable->isWorking = 1;
			threadJob job = readonly->job[writeable->currentJob++];
			job.job(job.arg);
			writeable->finishedJob++;
		}
		else{
			sleepThread(myInfo);//休憩の信号が出てたら休憩/
			writeable->isWorking = 0;//仕事がないけど休憩してる状態/
			YieldProcessor();
		}
	}
	writeable->isWorking = 0;
}

static void _prepareThreadFromID(int id){
	threadInfo* info = &(m.info[id]);
	mainWR* writeable = &info->mainMem;
	const threadWR* readonly = &info->threadMem;//メインスレッドはthreadMemがreadonly/
	writeable->canSleep = 0;//眠っていいフラグを消す/
	SetEvent(writeable->workEvent);//起こす/
}

static void _sleepThreadFromID(int id){
	threadInfo* info = &(m.info[id]);
	mainWR* writeable = &info->mainMem;
	const threadWR* readonly = &info->threadMem;//メインスレッドはthreadMemがreadonly/
	writeable->canSleep = 1;//眠らせる/
	ResetEvent(writeable->workEvent);
}

static void _waitThreadFromID(int id){
	threadInfo* info = &(m.info[id]);
	mainWR* writeable = &info->mainMem;
	const threadWR* readonly = &info->threadMem;//メインスレッドはthreadMemがreadonly/
	while(readonly->isWorking)YieldProcessor();
}

//######################################################################
// public:
//######################################################################

int threadInitialize(int threadNum, size_t stackSize){
	ASSERT(m.threads == NULL, "二回目のスレッド初期化");//一旦1回のみ/
	m.threads = gm_allocate(threadNum * sizeof(HANDLE));
	m.info = gm_allocate(threadNum * sizeof(threadInfo));
	m.threadNum = threadNum;
	for(int i = 0; i < threadNum; i++){
		//infoのセット/
		HANDLE hEvent = CreateEvent(
			NULL,//セキュリティ属性/
			TRUE,//falseは起きたら自動で赤信号になる trueは手動/
			FALSE,//初期状態　falseが赤信号/
			NULL //イベントの名前/
		);
		ASSERT(hEvent != NULL, "hEvent作成失敗したらしい");
		m.info[i] = (threadInfo){
			.mainMem = (mainWR){
				.currentJob = 0,
				.id = i,
				.workEvent = hEvent,
				.isActive = 1,
				.canSleep = 1,
			},
			.threadMem = (threadWR){
				.currentJob = 0,
				.isSleeping = 0,
				.isWorking = 0,
			}
		};
		DWORD threadID;
		HANDLE thread = CreateThread(
			NULL,//セキュリティ属性
			stackSize,//スタックサイズ 0の場合はデフォルト/
			threadWork,//実行する関数名/
			&(m.info[i]),//引数/
			0,//起動フラグ/
			&threadID//スレッドIDを受け取る変数/
		);
		if(thread == NULL){
			debugMSG("thread", "スレッドがNULL");
			return 1;
		}
		m.threads[i] = thread;
	}
	return 0;
}

void threadDestroy(){
	//全員終わらせる/
	for(int i = 0; i < m.threadNum; i++){
		threadInfo* info = &(m.info[i]);
		const threadWR* readonly = &info->threadMem;//メインスレッドはthreadMemがreadonly/
		mainWR* writeable = &info->mainMem;
		writeable->isActive = 0;
		writeable->canSleep = 1;
		if(readonly->isSleeping)SetEvent(writeable->workEvent);
	}
	//全員終わるまで待つ/
	for(int i = 0; i < m.threadNum; i++){
		threadInfo* info = &(m.info[i]);
		const threadWR* readonly = &info->threadMem;
		mainWR* writeable = &info->mainMem;
		while(readonly->isWorking)YieldProcessor();
	}
	for(int i = 0; i < m.threadNum; i++){
		HANDLE thread = m.threads[i];
		threadInfo* info = &(m.info[i]);
		mainWR* writeable = &info->mainMem;

		CloseHandle(thread);
		CloseHandle(writeable->workEvent);
	}
}

void threadSetJob(int id, void (*job)(void*), void* arg){
	threadInfo* info = &(m.info[id]);
	mainWR* writeable = &info->mainMem;
	const threadWR* readonly = &info->threadMem;//メインスレッドはthreadMemがreadonly/

	uint8_t* currentWriting = &(writeable->currentJob);
	const uint8_t* currentThreadReading = &(readonly->currentJob);
	while((uint8_t)(*currentWriting + 1) == *currentThreadReading){
		YieldProcessor();//仕事があくまで待つ/
	}
	writeable->job[writeable->currentJob] = (threadJob){
		.job = job,
		.arg = arg,
	};
	//ASSERT(writeable->currentJob+1 != readonly->currentJob, "仕事割り当て上限に達したらしい");
	writeable->currentJob++;
	writeable->gaveJob++;
}

void prepareThreadFromID(int id){
	_prepareThreadFromID(id);
}

void prepareThreads(){
	for(int i = 0; i < m.threadNum; i++) _prepareThreadFromID(i);
}

void sleepThreadFromID(int id){
	_sleepThreadFromID(id);
}

void sleepThreads(){
	for(int i = 0; i < m.threadNum; i++) _sleepThreadFromID(i);
}

void wateForThreads(){
	for(int i = 0; i < m.threadNum; i++)_waitThreadFromID(i);
	static struct thread_g_mem* p_mem = &m;
	int a = p_mem->info->mainMem.canSleep;
}

#endif