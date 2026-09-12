#pragma once
#include<stdint.h>
#include "control/control.h"


void setKeyStates(const KeyStates*);
void updateMouse(float spd);

int keyboardCheck(uint32_t);
int keyboardCheckPressed(uint32_t);
int keyboardCheckReleased(uint32_t);

float getMouseDx();
float getMouseDy();
