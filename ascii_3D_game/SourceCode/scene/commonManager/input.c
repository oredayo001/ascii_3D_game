#include "input.h"

static struct{
	KeyStates key;
	float mDx;
	float mDy;
}m;

void setKeyStates(const KeyStates* state){
	m.key = *state;
}
void updateMouse(float spd){
	m.mDy = spd * (float)lib_getMouseDy();
	m.mDx = spd * (float)lib_getMouseDx();
}


int keyboardCheck(uint32_t key){
	return lib_keyboardCheck(key, m.key.h);
}

int keyboardCheckPressed(uint32_t key){
	return lib_keyboardCheck(key, m.key.p);
}

int keyboardCheckReleased(uint32_t key){
	return lib_keyboardCheck(key, m.key.r);
}

float getMouseDx(){
	return m.mDx;
}

float getMouseDy(){
	return m.mDy;
}
