#include"Control.h"
#include<Windows.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdint.h>


#define getKey(key) GetAsyncKeyState(key)
#define HOLD 0x8000
#define PRESSED 0x0001




#define AS_KEY_CONFIG(temp,key) key,
static char keyConfig[] = {
	KEY_LIST_X(AS_KEY_CONFIG)
};

//ƒ}ƒEƒX‚Íˆê‚Â‚µ‚©‚È‚¢‚©‚ç‚±‚±‚ÅŠÇ—‚·‚é‚ñ‚ª‚¢‚¢/
static const int xm = 1200;
static const int ym = 600;

static int mouse_dx;
static int mouse_dy;

//ugokanai
static bool _isWindowActive(){
	HWND fHwnd = GetForegroundWindow();
	HWND hwnd = GetConsoleWindow();
	return (fHwnd == hwnd);
}

void lib_updateMouse(){
	POINT pt;
	GetCursorPos(&pt);

	mouse_dx = pt.x - xm;
	mouse_dy = pt.y - ym;

	SetCursorPos(xm, ym);
}



void lib_updateControl(KeyStates* s){
	//‘O‰ñ‚Ì‚ð‚à‚ç‚¤/
	uint32_t lastKeyState = s->h;
	//‰Šú‰»/
	s->h = 0;
	s->p = 0;
	s->r = 0;
	//“ü—Í‚ð‹l‚ß‚Ä‚¢‚­/
	for(int i = 0; i < vk_max; i++){
		//‘O‰ñ‰Ÿ‚³‚ê‚Ä‚½‚©/
		bool last = (lastKeyState >> (i)) & 1;
		//¡‰Ÿ‚³‚ê‚Ä‚é‚©/
		bool hold = (getKey(keyConfig[i]) & HOLD);
		s->h |= hold << (i);
		//‘O‰ñ‚ª‰Ÿ‚³‚ê‚Ä‚È‚¢‚©‚Â¡‰Ÿ‚³‚ê‚Ä‚é = ‰Ÿ‚³‚ê‚½uŠÔ/
		bool pressed = (!last) && (hold);
		s->p |= pressed << (i);
		//‘S‰ñ‰Ÿ‚³‚ê‚Ä‚½‚©‚Â¡‰Ÿ‚³‚ê‚Ä‚È‚¢ = —£‚³‚ê‚½uŠÔ/
		bool released = (last) && (!hold);
		s->r |= released << (i);
	}
}

void lib_updateKeyStateUsingInput(KeyStates* s, uint32_t input){
	//‘O‰ñ‚Ì‚ðˆê’U•ÛŽ/
	uint32_t lastKeyState = s->h;
	//‰Šú‰»/
	s->h = input;
	s->p = 0;
	s->r = 0;
	//“ü—Í‚ð‹l‚ß‚é/
	for(int i = 0; i < vk_max; i++){
		//‘O‰ñ‚Ì‚ª‰Ÿ‚³‚ê‚Ä‚½‚©/
		bool last = (lastKeyState >> (i)) & 1;
		//¡‰Ÿ‚³‚ê‚Ä‚é‚©/
		bool hold = (input >> i) & 1;
		//s->h |= hold << (i);
		bool pressed = (!last) && (hold);
		s->p |= pressed << (i);
		bool released = (last) && (!hold);
		s->r |= released << (i);

	}
}





int lib_getMouseDx(){
	return mouse_dx;
}
int lib_getMouseDy(){
	return mouse_dy;
}
