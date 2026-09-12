#include "debug.h"
#include<Windows.h>
void showDebugMessage(const char* title, const char* msg){
	MessageBoxA(GetConsoleWindow(), msg, title, MB_OK);
}
