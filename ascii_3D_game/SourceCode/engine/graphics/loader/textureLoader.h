#pragma once

#include "engine/engineTypes.h"

typedef struct Texture{
	int loded;
	int size;
	pixel_t* texture;
}Texture;

int getMaxTextureID();

void initTextureLoader(int size);

int loadTexture(const char* path);

Texture* getTexture(int id);