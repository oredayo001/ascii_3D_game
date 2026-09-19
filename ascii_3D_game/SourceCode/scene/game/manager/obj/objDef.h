#pragma once
#include "macro/macro.h"
#include "math/vec3.h"

//######################################################################
// 多分循環インクルードになりそうなやつの対策で分けた　ヘッダーファイルでincludeする
//######################################################################

struct objIInterfaceVTable;
typedef struct objIInterfaceVTable objIInterfaceVTable;

struct Model3D;
typedef struct Model3D Model3D;
//描画に必要なものだけ/
//! objBase経由でのみアクセス可能 順番がたまに代わるからポインタを直で保存しとくと次のフレームでは別のポインタになることがある/
typedef struct objRender{
	Model3D* model;//将来アニメーションする場合はstep関数内でこのモデルの頂点を動かすって方針/
	vec3 p;
	Basis angle;
	vec3 scale;
}objRender;

//それぞれのキャラの基底/
//8byteアライメント(x64)
typedef struct objBase{
	//毎フレーム呼ばれる/
	void (*step)(struct objBase* me);//返却時にリストの次のポインタ用に使われるかわいそうなスペース/
	//インターフェース/
	objIInterfaceVTable* interfaces;//キャラ同士でやり取りできる属性ごとに共通したインターフェース/
	objRender* render;//!アドレスを次のフレームまで保存できるポインタ変数に入れると次のフレームでは別のアドレスに代わることがあるから毎フレームここを経由する必要がある/

	// --- 基本的なやつ ---
	vec3 v;//16 + 4
	// --- flags 2byte ---
	uint16_t isActive : 1;// 1/16
	uint16_t onGround : 1;//   2/16
	uint16_t __pad14 : 14;//   16/16->2byte
	// --- id ---
	uint16_t objID;//        2
	//		 ^-こいつは返却時にサイズを求めるのとまあデバッグにも使えるかもな　destroyでサイズ返すんもまあありやったけど　でも全体のセーブロードには使える 関数ポインタの復元とかはこいつがないと不可能/

	uint32_t timer;
	uint32_t attribute;//インターフェースするときは一応こいつとビットマスクするアサート張るといい/
	uint32_t __pad32;
	uint32_t generation;
}objBase;
#define OBJ_BASE objBase base

// -----------------------------------------------------------
// 属性
// -----------------------------------------------------------

// --- LIST ---

//各objの属性リスト　増えたらここに追加していく/
#define OBJ_ATTRIBUTE_LIST_X(X)\
X(player)\
X(enemy)\
X(playable)/*カメラ操作が可能*/\
X_MACRO_END

// --- attribute mask --- /

//名前だけ/
#define AS_OBJ_ATTRIBUTE_INDEX(name) ATTACH(objAttributeIndex_,name)
//enum生成用　これは1234って感じで増えていく ビットマスクを生成するために使うヘルパーのenumを生成する用/
#define AS_OBJ_ATTRIBUTE_INDEX_X(name) AS_OBJ_ATTRIBUTE_INDEX(name),
enum{
	OBJ_ATTRIBUTE_LIST_X(AS_OBJ_ATTRIBUTE_INDEX_X)
};
//名前だけ/
#define AS_OBJ_ATTRIBUTE_BIT(name) ATTACH(objAtt_,name)
//ビットマスク生成用　上で作った1234ってなっていくやつでビットシフトすることで1248ってなる まあ2進数で 1 10 100 1000 って感じ/
#define AS_OBJ_ATTRIBUTE_BIT_X(name) AS_OBJ_ATTRIBUTE_BIT(name) = (1 << AS_OBJ_ATTRIBUTE_INDEX(name)),
enum{
	OBJ_ATTRIBUTE_LIST_X(AS_OBJ_ATTRIBUTE_BIT_X)
};

// --- IInterface --- /

// -- struct
#define AS_TYPEDEF_IINTERFASES_NAME(name) ATTACH(name,IInterfaceVTable)

// 属性ごとに継承させる　これで当たり判定で属性ビットマスクでやると確定でそのインターフェースに継承できるという　なんか凄そう/
//!typedefはつけない/
//! こいつらはobjIInterfaceVTableの中で使うだけでこいつだけを使うことは基本無い　はず/
struct AS_TYPEDEF_IINTERFASES_NAME(player){//一旦nullは無しがいいかな/
	void* p;//今のとこ何もない/
};
struct AS_TYPEDEF_IINTERFASES_NAME(enemy){
	void* p;//今のとこ何もない/
};
struct AS_TYPEDEF_IINTERFASES_NAME(playable){
	vec2Basis* (*getCameraIn)(objBase* me);//カメラ入力のポインタのゲッター/
	vec3(*getEyePos)(objBase* me);//目線の高さをもらう/
};

// -- base
#define AS_ALL_OBJ_INTERFACE_NAME(name) ATTACH(name,Interface)
#define AS_ALL_OBJ_INTERFACES(name) struct AS_TYPEDEF_IINTERFASES_NAME(name) AS_ALL_OBJ_INTERFACE_NAME(name);

//キャラ同士の会話用/
//キャラ.cファイルにconstで宣言してキャラinit関数で返すようにする/
struct objIInterfaceVTable{
	// --- 共通 --- /
	void (*destroy)(struct objBase* me);//あとで何とかする/
	// --- 各属性の --- /
	OBJ_ATTRIBUTE_LIST_X(AS_ALL_OBJ_INTERFACES)//;はもうついてる/
};

// -----------------------------------------------------------
// 各obj
// -----------------------------------------------------------

//!ここに追加していく/
#define OBJECTS_LIST_X(X)\
/*
X(名前,型名,属性ビットマスク)\
名前は重複不可　型は重複可*/\
X(player, objPlayer, objAtt_player | objAtt_playable)\
X(dummy_debug, objDummy_debug, 0)\
X_MACRO_END

//前方宣言 まあ全オブジェクトを一括で前方宣言する　オブジェクトはstruct obj...って定義するようにするとobjListに追加するの忘れてたらコンパイルエラーになってくれるからミスに気づきやすい/
#define AS_TYPEDEF_OBJECT_LIST(temp,type,...) struct type;typedef struct type type;
OBJECTS_LIST_X(AS_TYPEDEF_OBJECT_LIST);
#undef AS_TYPEDEF_OBJECT_LIST

//objID
#define AS_ENUM_OBJ_LIST(name,...) ATTACH(obj_,name),
enum{
	OBJECTS_LIST_X(AS_ENUM_OBJ_LIST)
	AS_ENUM_OBJ_LIST(objMax)
};
#undef AS_ENUM_OBJ_LIST

/*
//?RULE obj系はtypedef struct を使ってはならない あとOBJECTS_LIST_Xに追加忘れずに
まあ忘れててもstructつけてないとエラーになるはず/
*/

//アライメントサイズ/
//! long long とか double とか使わん限りvoid*のサイズ/
#define OBJ_ALIGNMENT_SIZE sizeof(void*)

//関数の命名/
#define AS_OBJ_INITIALISE_FUNC(name) ATTACH(ATTACH(obj_,name),Init)

typedef struct{
	Model3D* model;
	void (*step)(struct objBase* me);
	objIInterfaceVTable* interfaces;
}objInitOut;//無いと困るやつの初期値/
