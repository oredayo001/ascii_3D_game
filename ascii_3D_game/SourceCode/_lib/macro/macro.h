#pragma once

// --- Xマクロによく使うやつ --- /

//まあ可読性?/

//!こいつの後ろには何も書くな 書いたら殴る/
#define X_MACRO_END

//くっつけるやつ/

//普通こいつ直では使わないほうがいい　ATTACH()を使うべき 特殊なケースを除いて 手か特殊なケースとかあるか?/
#define ATTACH_X(a,b) a##b
//くっつけるやつ/
#define ATTACH(a,b) ATTACH_X(a,b)

//__VA_ARGS__

//__VA__ARGS__を使うやつで使うやつ/
#define HELPER(X) X

#define DO_NOTHING do{}while(0)
