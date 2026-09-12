

#include"./engine/engine.h"
#include"./engine/buffer/gameBuff.h"
#include"common.h"

//firstScene
#include "scene/title/title.h"


static struct{
	clock_t lastClock;
	int accumlator;
	int isRunning;
}m;

#if ENABLE_DEBUG + 0
#include<Windows.h>//win
#define getKeyHold(c) (GetAsyncKeyState(c)&0x01)
DebugMembers debugMember;
int fpsCnt = 0;
int fpsAccumlator = -CLOCKS_PER_SEC;
#endif

//コピペ用/
#if ENABLE_DEBUG + 0 
#endif

static void loop(){
	//初期化/
	m.lastClock = clock();
	m.accumlator = 0;
	m.isRunning = 1;


	//loop
	while(m.isRunning){
		// --- time --- /
		clock_t current = clock();
		clock_t delta = current - m.lastClock;
		m.accumlator += delta;
		m.lastClock = current;

		// --- debug --- /
#pragma region デバッグ
#if ENABLE_DEBUG + 0
		fpsCnt++;
		fpsAccumlator += delta;
		if(fpsAccumlator >= 0){
			debugMember.fps = fpsCnt;
			fpsCnt = 0;
			fpsAccumlator = -CLOCKS_PER_SEC;
		}
		if(getKeyHold('0')){
			debugMember.memolyUsed = gm_getMarker()*GAME_BUFF_ALIGN_SIZE;
			char txt[256];
#define d(x) debugMember.x
			snprintf(txt, 256, "fps:%d\npolygone:%d\nstaticPolygone:%d\nfovAngle%f\nusedMem%dB,%dKB ", d(fps), d(triCnt), d(triCntStatic),d(fovAngle),d(memolyUsed),d(memolyUsed)/1024);
#undef d
			debugMSG("debug text", txt);
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
		while((m.accumlator > 0)){//最初必ず呼ばれる/
			//update/
			m.isRunning = engineUpdate();
			//accumlatorから1f分のクロック数を引く/
#if ENABLE_DEBUG + 0//デバッグ用 9を押すとfpsを変更できるようにしてるやつ/
			m.accumlator -= intervalTable[intervalMode];//hps可変/
#else//通常/
			m.accumlator -= INTERVAL;//60fps
#endif
			//ループ数をカウント/
			loopCnt--;
			//ループ数上限に達するかゲームが終了してるか/
			if(!m.isRunning || !loopCnt){
				m.accumlator = -1;//ループを抜ける/
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

