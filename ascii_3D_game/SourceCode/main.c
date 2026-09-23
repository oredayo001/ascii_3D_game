

#include"./engine/engine.h"
#include"./engine/buffer/gameBuff.h"
#include"common.h"

//firstScene
#include "scene/title/title.h"

#define _ENABLE_DEBUG_MAIN (ENABLE_DEBUG||ENABLE_DEBUG_MAIN_STRONG_X)

static struct{
	clock_t lastClock;
	int accumulator;
	int isRunning;
}m;

#if _ENABLE_DEBUG_MAIN + 0
#include<Windows.h>//win
#define getKeyHold(c) (GetAsyncKeyState(c)&0x01)
DebugMembers debugMember;
int fpsCnt = 0;
int fpsaccumulator = -CLOCKS_PER_SEC;
int lastAccumStart = -CLOCKS_PER_SEC;
#endif

//コピペ用/
#if _ENABLE_DEBUG_MAIN + 0 
#endif

static void loop(){
	//初期化/
	m.lastClock = clock();
	m.accumulator = 0;
	m.isRunning = 1;


	//loop
	while(m.isRunning){
		// --- time --- /
		clock_t current = clock();
		clock_t delta = current - m.lastClock;
		m.accumulator += delta;
		m.lastClock = current;

		// --- debug --- /
#pragma region デバッグ
#if _ENABLE_DEBUG_MAIN + 0
		fpsCnt++;
		fpsaccumulator += delta;
		if(fpsaccumulator >= 0){
			int counted = fpsaccumulator - lastAccumStart;
			debugMember.lastFpsCount = counted;
			float raito = (float)CLOCKS_PER_SEC / (float)(counted);
 			int fixedFpsCnt = (float)fpsCnt * raito;

			debugMember.fps_noFix = fpsCnt;
			debugMember.fps = fixedFpsCnt;
			debugMember.fps_sum += fixedFpsCnt;
			debugMember.fps_countedNum++;
			fpsCnt = 0;
			fpsaccumulator -= CLOCKS_PER_SEC;
			lastAccumStart = fpsaccumulator;
		}
		if(getKeyHold('0')){
			//int wachedClock = clock();
			debugMember.memolyUsed = gm_getMarker() * GAME_BUFF_ALIGN_SIZE;
			char txt[256];
#define d(x) debugMember.x
			uint32_t fpsAvarage = d(fps_countedNum)==0?0:d(fps_sum) / d(fps_countedNum);
			snprintf(txt, 256, "count:%d noFixFPS:%d\nfps:%d avarage:%d\npolygone:%d\nstaticPolygone:%d\nfovAngle:%f\nusedMem:%dB,%dKB\nobjNum:%d ", d(lastFpsCount), d(fps_noFix), d(fps), fpsAvarage, d(triCnt), d(triCntStatic), d(fovAngle), d(memolyUsed), d(memolyUsed) / 1024, d(objNum));
#undef d
			debugMSG("debug text", txt);
			//int deltaWatchdTime = clock() - wachedClock;
			//fpsaccumulator -= deltaWatchdTime;
		}

		//fpsを変更するやつ/
		static int intervalTable[2] = {
			INTERVAL,INTERVAL * 4
		};

		static int intervalMode = 0;
		if(getKeyHold('9')){
			intervalMode = !intervalMode;
		}
#endif
#pragma endregion デバッグ終わり/
		// --- ここからがゲームのやつ --- /

		//update
		int loopCnt = MAX_LOOP_CNT;
		while((m.accumulator > 0)){//最初必ず呼ばれる/
			//update/
			m.isRunning = engineUpdate();
			//accumulatorから1f分のクロック数を引く/
#if _ENABLE_DEBUG_MAIN + 0//デバッグ用 9を押すとfpsを変更できるようにしてるやつ/
			m.accumulator -= intervalTable[intervalMode];//hps可変/
#else//通常/
			m.accumulator -= INTERVAL;//60fps
#endif
			//ループ数をカウント/
			loopCnt--;
			//ループ数上限に達するかゲームが終了してるか/
			if(!m.isRunning || !loopCnt){
				m.accumulator = -1;//ループを抜ける/
			}
		}
		//render
		engineRender();
	}
}

int main(){
	engineInit(titleSetFunc);//初期化　最初のシーンをここで渡す/
	loop();//ループ/
	engineFin();//終了/
	return 0;
}

