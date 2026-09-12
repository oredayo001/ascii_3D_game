#include"screen.h"
#include<Windows.h>
#include"../buffer/gameBuff.h"

#define BYTES_PER_PX 1

//---------------------------------------------
// private:
//---------------------------------------------

#define AS_ASCII_TABLE(temp,c) c,
const char asciiTable[] = {
	//shade
	ASCII_SHADE_TABLE_LIST_X(AS_ASCII_TABLE)
	//ex
	ASCII_TABLE_LIST_X(AS_ASCII_TABLE)
};

static HANDLE hStdout;//自分/

static void initScreen(){
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);//私は誰?
}

//8の倍数にする/
#define SC_ALIGN_SIZE 64
#define MEM_ALIGN(size) (((size_t)(size) + (SC_ALIGN_SIZE - 1)) & ~(SC_ALIGN_SIZE - 1))
#define IS_ALIGNED(p) (!(((size_t)p)&(SC_ALIGN_SIZE-1)))

//width,heightから何バイト要るかを求める/
static inline size_t getScreenNeed(size_t width, size_t height){
	return (size_t)(height * (BYTES_PER_PX * width + 1)) + 1;
}

//---------------------------------------------
// public:
//---------------------------------------------

//スクリーンを作る奴/
static Screen* _createScreen_old(){//旧/
	//最適化のために定数にする どうせ画面二つも作らんし/
	const int width = WIDTH;
	const int height = HEIGHT;
	initScreen();

	//それぞれ4の倍数になってる(はず)/
	size_t size = sizeof(Screen);
	size_t screenBuffSize = MEM_ALIGN(getScreenNeed(width, height));
	size_t screenSize = MEM_ALIGN(width * height * sizeof(pixel_t));
	size_t zbuffSize = MEM_ALIGN(width * height * sizeof(float));
	size_t extra = SC_ALIGN_SIZE;

	//合計/
	size_t total = size + screenBuffSize + (screenSize << 1) + zbuffSize + extra;

	//全員同時にメモリ確保/
	Screen* r = (Screen*)gm_allocate(total);//r自体は8byteアライメントくらいはされてるけどまあ64まではされてない/
	memset(r, 0, total);

	//4の倍数+4の倍数は4の倍数/
	//r|zbuff|screen|prev|buff の順で並ぶ 解放はr まあ戻り値をfreeするだけ/
	uint8_t* base = (uint8_t*)(MEM_ALIGN((size_t)(&(r[1]))));//すぐ隣のメモリ位置+(align-1)を63ビット以下切り捨て これで64byteアライメント/

	r->zbuff = (float*)base;
	base += zbuffSize;

	r->screen = (pixel_t*)base;
	base += screenSize;

	r->prevscreen = (pixel_t*)base;
	base += screenSize;

	r->buff = (char*)base;
	r->size = 0;//buffに書き込んだ時に書き換える/

	ASSERT(IS_ALIGNED(r->zbuff), "なんか64byteアライメントで来てない　zbuff");
	ASSERT(IS_ALIGNED(r->screen), "なんか64byteアライメントで来てない　sc");
	ASSERT(IS_ALIGNED(r->prevscreen), "なんか64byteアライメントで来てない　prevsc");
	ASSERT(IS_ALIGNED(r->buff), "なんか64byteアライメントで来てない　buff");

	return r;
}

Screen* createScreen(){
	const size_t width = WIDTH;
	const size_t height = HEIGHT;
	initScreen();

	uint8_t* head = gm_getCurrent();gm_d_lockMemoly();
	uint8_t* base = head;

	size_t size = sizeof(Screen);
	size_t screenBuffSize = getScreenNeed(width, height);
	size_t screenSize = width * height * sizeof(pixel_t);
	size_t zbuffSize = width * height * sizeof(float);

	Screen* r = (Screen*)base;//8byteアライメント/
	base += size;

	char* buff = (char*)base;//1byteアライメント/
	base += screenBuffSize;
	base = (uint8_t*)MEM_ALIGN(base);//アライメント/

	float* zbuff = (float*)base;//64byteアライメント/
	base += MEM_ALIGN(zbuffSize);
	
	pixel_t* screen = (pixel_t*)base;//64byteアライメント/
	base += MEM_ALIGN(screenSize);
	
	pixel_t* prevscreen = (pixel_t*)base;//64byteアライメント/
	base += MEM_ALIGN(screenSize);

	size_t used = base - head;
	gm_increment(used); gm_d_unlockMemoly();

	memset(head, 0, used);
	*r = (Screen){
		.buff = buff,
		.prevscreen = prevscreen,
		.screen = screen,
		.zbuff = zbuff,
		.size = 0,
	};

	ASSERT(IS_ALIGNED(r->zbuff), "なんか64byteアライメントで来てない　zbuff");
	ASSERT(IS_ALIGNED(r->screen), "なんか64byteアライメントで来てない　sc");
	ASSERT(IS_ALIGNED(r->prevscreen), "なんか64byteアライメントで来てない　prevsc");

	return r;
}
void updateScreen(Screen* sc){
	//先頭/
	char* p = sc->buff;//printfに渡す画面を文字データに変換したデータの先頭アドレスが入ってるやつ/
	pixel_t* px = sc->screen;//ゲームループ内で描画した色データ/
	pixel_t* ppx = sc->prevscreen;//前回のフレームで描画した色データ/
	//size
	const int width = WIDTH;
	const int height = HEIGHT;

	const size_t lineByte = width * sizeof(pixel_t);

	for(int y = 0; y < height; y++){
		//超簡易的差分描画 printfに送る文字数を減らすためのものであって書き込み回数を減らすものではない/
		if(memcmp(px, ppx, lineByte)){//行単位で一致してるか/
			for(int x = 0; x < width; x++){
				int asciiIndex = px[x];
				ASSERT(asciiIndex < a_max, "ascii範囲外");
				p[x] = asciiTable[asciiIndex];//1行分書き込む/
			}
			p += width;//書き込んだ分ずらす/
		}
		//一致してなかった場合はpは何も書き込んでないからずらさない/
		 
		//一致してなかった場合はそのまま書き込んだ後次の行に行く
		//一致してた場合は前回コンソール画面に書いてた残ってるのを使いまわせるから書き込まずに改行だけでスキップ
		*p++ = '\n';
		//1行分ポインタを進める/
		px += width;
		ppx += width;
	}
	*p++ = '\0';//これがないとprintfで最後の文字より後にあるごみデータを0が車で書き続けてしまう/
	size_t size = p - (sc->buff);//多分無くていい/
	sc->size = size;//多分無くていい/

	//交換/
	pixel_t* temp = sc->screen;//一旦よける/
	sc->screen = sc->prevscreen;
	sc->prevscreen = temp;

	//screenに今描画したのがあった
	//それがprevに移動してprevにあったのがscreenに行く
	//screenはゲームループ内でクリアされてそのまま描画される
	//そんで戻った時にprev都の変更点を見ながらbuffに書き込むって仕組み/

}
//テスト用/
void _updateScreen(Screen* sc){
	//先頭/
	char* p = sc->buff;
	pixel_t* px = sc->screen;
	pixel_t* ppx = sc->prevscreen;
	//size
	int width = WIDTH;
	int height = HEIGHT;

	for(int y = 0; y < height; y++){
		for(int x = 0; x < width; x++){
			pixel_t xx = px[x];
			char c = asciiTable[xx];
			ASSERT(c != 0, "aaa");
			snprintf(&p[x], 2, "%c", c);
		}
		p += width;
		*p++ = '\n';
		px += width;
		ppx += width;
	}
	*p++ = '\0';
	size_t size = p - (sc->buff);
	sc->size = size;

	//交換/
	pixel_t* temp = sc->screen;
	sc->screen = sc->prevscreen;
	sc->prevscreen = temp;
}

//実際に描画するやつ/
void flushScreen(Screen* sc){
	//カーソルを左上に戻してコンソール画面に送って改行を同時にやる/
	printf("\033[H%s\n", sc->buff);
}

void destroyScreen(Screen** s){
	//free(*s);
	*s = NULL;
}