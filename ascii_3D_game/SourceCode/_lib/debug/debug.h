#pragma once

void showDebugMessage(const char* title, const char* msg);

//•Ï”‰Â/
#define debugMSG(title,msg) showDebugMessage(title,msg)

#define TO_STRING_X(X) #X
#define TO_STRING(X) TO_STRING_X(X)

//""‚ÅˆÍ‚Ü‚ê‚½“z‚Ì‚İ/
#define ASSERT(x,msg) do{if(!(x)) debugMSG("assert",msg "\n\nline:" TO_STRING(__LINE__) "\n\nfile:" __FILE__);}while(0)
//""‚ÅˆÍ‚Ü‚ê‚½“z‚Ì‚İ/
#define NULL_CHECK(x,msg) do{if(!(x)) debugMSG("assert",msg "\n\nline:" TO_STRING(__LINE__) "\n\nfile:" __FILE__);}while(0)

static int breakPoint(){
	return 1;
}

#define trueBreakPoint(x) if(x){\
breakPoint();\
}
