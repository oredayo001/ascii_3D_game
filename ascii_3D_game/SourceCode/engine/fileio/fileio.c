
#include"debug/debug.h"
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

//baseFileはフォルダじゃなくてファイルまで/
const char* attachFilePath(const char* baseFile, const char* relative, char* dist){

	// ファイル名を取り除く/
	char base_dir[256 + 3 + 1];
	strcpy_s(base_dir, sizeof(base_dir), baseFile);
	PathRemoveFileSpecA(base_dir);
	// base_dir が ".\Data\model\name.mtl" から ".\Data\model" とかになる/

	//結合/
	LPSTR hr = PathCombineA(
		dist,//出力/
		base_dir,//基準/
		relative // 結合したい相対パス
	);
	//成功/
	if(hr!=NULL){
		return dist;
	}
	else{
		debugMSG("file", "file結合に失敗");
		return NULL;//失敗/
	}
}
