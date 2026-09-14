#include "textureLoader.h"
#include "engine/buffer/gameBuff.h"
#include <stdio.h>
#include <windows.h>
//#include <shlwapi.h>

//#pragma comment(lib, "shlwapi.lib")

static const pixel_t testTex_img[64 * 64] = {
#ifdef __INTELLISENSE__
	0,
#else
	//神って書かれたテクスチャ/
#include"output.txt"
#endif
};

static Texture testTex = {
	.texture = testTex_img,
	.size = 64,
	.loded = 1
};

//ファイルの先頭についてるやつ/
const char MAGIC_NUMBER[4] = { 'A', 'A', 'T', 'X' };
/*
txt:
magic/id/size/data...
*/

static struct{
	Texture* textures;
	int texCnt;
}m;

int getMaxTextureID(){
	int maxID = 0;
	WIN32_FIND_DATAA findData;
	char sertchPath[MAX_PATH];
	snprintf(sertchPath, sizeof(sertchPath), "Data\\texture\\*.txt");
	HANDLE hTexFind = FindFirstFileA(sertchPath, &findData);
	if(hTexFind != INVALID_HANDLE_VALUE){
		do{
			char txtPath[256];
			snprintf(txtPath, sizeof(txtPath), "Data\\texture\\%s", findData.cFileName);
			FILE* file = fopen(txtPath, "r");
			int id = 0;
			fread(&id, 4, 1, file);//magic
			fread(&id, 4, 1, file);//id
			maxID = max(maxID, id);
			fclose(file);
		} while(FindNextFileA(hTexFind, &findData));
		FindClose(hTexFind);
	}
	return maxID;
}

void initTextureLoader(int size){
	//メモリ確保/
	m.textures = gm_allocate(size * sizeof(Texture));
	//0クリア/
	memset(m.textures, 0, size * sizeof(Texture));
	//テクスチャの数/
	m.texCnt = size;
}

int loadTexture(const char* path){
	FILE* f = fopen(path, "rb");

	//magic
	char num[4] = { 0 };
	fread(&num, sizeof(char), 4, f);
	if(memcmp(&num, &MAGIC_NUMBER, 4)){
		debugMSG("loadTexture", "ファイルの先頭が違う");
		goto err;
	}

	//id
	int id = 0;
	fread(&id, sizeof(int), 1, f);
	ASSERT(id <= m.textures->size, "初期化時のサイズを超えるid");
	Texture* tex = &m.textures[id];
	ASSERT(0 <= id && id < m.texCnt, "範囲外のid");

	if(tex->loded){
		goto success;
	}

	//size
	fread(&(tex->size), sizeof(int), 1, f);
	ASSERT(!((tex->size - 1) & (tex->size)), "textureが2の累乗じゃない");

	//texture
	int px = tex->size * tex->size;
	tex->texture = gm_allocate(px * sizeof(pixel_t));
	int readCnt = fread(tex->texture, sizeof(pixel_t), px, f);
	ASSERT(readCnt == px, "ファイルのデータがおかしい");

	tex->loded = 1;
success:
	fclose(f);
	return id;
err:
	fclose(f);
	return -1;
}

Texture* getTexture(int id){
	if(id < 0){
		return &testTex;
	}
	else return &m.textures[id];
}