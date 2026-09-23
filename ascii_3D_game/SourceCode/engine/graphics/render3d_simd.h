#pragma once
#include<immintrin.h>

/*
_mm_load_ps
_mm_store_ps
_mm_storeu_si128
_mm_set_ps
_mm_set1_ps
_mm_set_epi32
_mm_set1_epi32
_mm_setzero_si128
_mm_add_ps
_mm_add_epi32
_mm_sub_epi32
_mm_mul_ps
_mm_mullo_epi32
_mm_rcp_ps
_mm_cmplt_ps
_mm_cmplt_epi32
_mm_and_ps
_mm_and_si128
_mm_castsi128_ps
_mm_castps_si128
_mm_movemask_ps
_mm_blendv_ps
_mm_packus_epi32
_mm_packus_epi16
_mm_cvttps_epi32
_mm_cvtsi128_si32
*/

#ifndef SIMD_MODE
static_assert(0, "マクロ SIMD_MODE が作られてない");
#endif

#if SIMD_MODE == 128
// --- 型 --- /
typedef __m128 v_float;
typedef __m128i v_int;
// --- サイズ系 --- /
#define SIMD_STEP 4
#define ALIGN_MASK (~(SIMD_STEP-1))
#define SIMD_ALIGN 16
// --- 関数系 --- /

//16byteアライメントされてる場所からfloat4つをロード/
#define simd_load_ps _mm_load_ps
//16byteアライメントされてる場所からint4つをロード/
#define simd_load_si128 _mm_load_si128
//16byteアライメントされてる場所にfloat4つを書き込み/
#define simd_store_ps _mm_store_ps
//128byte書き込み/
#define simd_storeu_si128 _mm_storeu_si128
//128byte書き込み アライメント済み/
#define simd_store_si128 _mm_store_si128
//float4つをセットする/
#define simd_set_ps _mm_set_ps
//4つに同じfloatの値をセットする/
#define simd_set1_ps _mm_set1_ps
//int4つをセットする/
#define simd_set_epi32 _mm_set_epi32
//全部に同じintの値をセットする/
#define simd_set1_epi32 _mm_set1_epi32
//全部に0をセットする/
#define simd_setzero_si128 _mm_setzero_si128
//全部に0をセットする/
#define simd_setzero_ps _mm_setzero_ps
//floatの足し算/
#define simd_add_ps _mm_add_ps
//符号付きの足し算/
#define simd_add_epi32 _mm_add_epi32
//符号付きの引き算/
#define simd_sub_epi32 _mm_sub_epi32
//符号付きの引き算/
#define simd_sub_ps _mm_sub_ps
//掛け算/
#define simd_mul_ps _mm_mul_ps
//割り算/
#define simd_div_ps _mm_div_ps
//符号付きの足し算　オーバーフローしたとこは無視/
#define simd_mullo_epi32 _mm_mullo_epi32
//多分シフト演算/
#define simd_slli_epi32 _mm_slli_epi32
//逆数 float/
#define simd_rcp_ps _mm_rcp_ps
//イメージ各要素に ((第一)<(第二))?-1:0 を代入する感じ/
#define simd_cmplt_ps _mm_cmplt_ps
//イメージ各要素に ((第一)<(第二))?-1:0 を代入する感じ/
#define simd_cmplt_epi32 _mm_cmplt_epi32
#define simd_cmple_ps _mm_cmple_ps
//and
#define simd_and_ps _mm_and_ps
//and
#define simd_and_si128 _mm_and_si128
//ps to signed int 128
#define simd_castsi128_ps _mm_castsi128_ps
//signed int 128 to ps
#define simd_castps_si128 _mm_castps_si128
//各要素の最上位ビットで作ったマスク/
#define simd_movemask_ps _mm_movemask_ps
//(b&mask)|(a&~mask)って感じのやつ/
#define simd_blendv_ps _mm_blendv_ps
//(b&mask)|(a&~mask)って感じのやつ/
#define simd_blendv_epi8 _mm_blendv_epi8
//32bit整数でパックされてる2つのを詰め込む　各要素は32から16bitになるけど飽和処理で上位ビットが切り捨てられたら勝手に最大値にしてくれる/
#define simd_packs_epi32 _mm_packs_epi32
//16bit整数でパックされてる2つのを詰め込む　各要素は16から18itになるけど飽和処理で上位ビットが切り捨てられたら勝手に最大値にしてくれる/
#define simd_packs_epi16 _mm_packs_epi16
//ps to epi32
#define simd_cvttps_epi32 _mm_cvttps_epi32
//epi32 to ps
#define simd_cvtepi32_ps _mm_cvtepi32_ps
//si32 to si 128
#define simd_cvtsi128_si32 _mm_cvtsi128_si32
//f(base,v,s) = [array + v1*s,array + v2*s,...,array + v3*s]
#define simd_i32gather_epi32 _mm_i32gather_epi32


// --- 128と256で名前が変わるやつのラッパー --- /

//16byteアライメントされてる場所からint4つをロード/
#define simd_load_si simd_load_si128
//128byte書き込む/
#define simd_storeu_si simd_storeu_si128
//128byte書き込み アライメント済み/
#define simd_store_si simd_store_si128
//全部に0をセットする/
#define simd_setzero_si simd_setzero_si128
//128bit全部のand
#define simd_and_si simd_and_si128
//ps to si128 (再解釈するだけ)
#define simd_castsi_ps simd_castsi128_ps
//si128 to ps (再解釈するだけ)
#define simd_castps_si simd_castps_si128
//si32 to si 128
#define simd_cvtsi_si32 simd_cvtsi128_si32

// --- 互換性? --- /

//v_pixelに変換/
//static inline  v_pixel simd_pack_to_v_pixel(v_int v_c_32){//結局使わなくなった/
//	v_int v_c_16 = simd_packs_epi32(v_c_32, simd_setzero_si());
//	v_int v_c_8 = simd_packs_epi16(v_c_16, simd_setzero_si());
//	return _mm_cvtsi128_si32(v_c_8);
//}



#elif SIMD_MODE == 256
// --- 型 --- /
typedef __m256 v_float;
typedef __m256i v_int;
// --- サイズ系 --- /
#define SIMD_STEP 8
#define ALIGN_MASK (~(SIMD_STEP-1))
#define SIMD_ALIGN 32
// --- 関数系 --- /

//16byteアライメントされてる場所からfloat8つをロード/
#define simd_load_ps _mm256_load_ps
//16byteアライメントされてる場所からint8つをロード/
#define simd_load_si256 _mm256_load_si256
//16byteアライメントされてる場所にfloat8つを書き込み/
#define simd_store_ps _mm256_store_ps
//256byte書き込み/
#define simd_store_si256 _mm256_store_si256/*!変更*/
//float8つをセットする/
#define simd_set_ps _mm256_set_ps
//8つに同じfloatの値をセットする/
#define simd_set1_ps _mm256_set1_ps
//int8つをセットする/
#define simd_set_epi32 _mm256_set_epi32
//全部に同じintの値をセットする/
#define simd_set1_epi32 _mm256_set1_epi32
//全部に0をセットする/
#define simd_setzero_si256 _mm256_setzero_si256/*!変更*/
//全部に0をセットする/
#define simd_setzero_ps _mm256_setzero_ps/*!変更*/
//floatの足し算/
#define simd_add_ps _mm256_add_ps
//符号付きの足し算/
#define simd_add_epi32 _mm256_add_epi32
//符号付きの引き算/
#define simd_sub_epi32 _mm256_sub_epi32
//符号付きの引き算/
#define simd_sub_ps _mm256_sub_ps
//掛け算/
#define simd_mul_ps _mm256_mul_ps
//割り算/
#define simd_div_ps _mm256_div_ps
//符号付きの足し算　オーバーフローしたとこは無視/
#define simd_mullo_epi32 _mm256_mullo_epi32
//多分シフト演算/
#define simd_slli_epi32 _mm256_slli_epi32
//逆数 float/
#define simd_rcp_ps _mm256_rcp_ps
//イメージ各要素に ((第一)<(第二))?-1:0 を代入する感じ compare less than/
#define simd_cmplt_ps(a, b) _mm256_cmp_ps((a), (b), _CMP_LT_OQ)
//イメージ各要素に ((第一)<=(第二))?-1:0 を代入する感じ compare less or equal/
#define simd_cmple_ps(a, b) _mm256_cmp_ps((a), (b), _CMP_LE_OQ)
//イメージ各要素に ((第一)<(第二))?-1:0 を代入する感じ/
#define simd_cmplt_epi32(a, b) _mm256_cmpgt_epi32((b), (a))
//and
#define simd_and_ps _mm256_and_ps
//or
#define simd_or_ps _mm256_or_ps
//xor
#define simd_xor_ps _mm256_xor_ps
//and
#define simd_and_si256 _mm256_and_si256/*!変更*/
//or
#define simd_or_si256 _mm256_or_si256/*!変更*/
//xor
#define simd_xor_si256 _mm256_xor_si256/*!変更*/
//ps to signed int 128
#define simd_castsi256_ps _mm256_castsi256_ps/*!変更*/
//signed int 128 to ps 
#define simd_castps_si256 _mm256_castps_si256/*!変更*/
//各要素の最上位ビットで作ったマスク/
#define simd_movemask_ps _mm256_movemask_ps
//(b&mask)|(a&~mask)って感じのやつ/
#define simd_blendv_ps _mm256_blendv_ps
//(b&mask)|(a&~mask)って感じのやつ/
#define simd_blendv_epi8 _mm256_blendv_epi8
//32bit整数でパックされてる2つのを詰め込む　各要素は32から16bitになるけど飽和処理で上位ビットが切り捨てられたら勝手に最大値にしてくれる/
#define simd_packs_epi32 _mm256_packs_epi32
//16bit整数でパックされてる2つのを詰め込む　各要素は16から18itになるけど飽和処理で上位ビットが切り捨てられたら勝手に最大値にしてくれる/
#define simd_packs_epi16 _mm256_packs_epi16
//ps to epi32
#define simd_cvttps_epi32 _mm256_cvttps_epi32
//epi32 to ps
#define simd_cvtepi32_ps _mm256_cvtepi32_ps
//si64 to si 256
#define simd_cvtsi256_si32 _mm256_cvtsi256_si32 /*!変更*/
//f(base,v,s) = [array + v1*s,array + v2*s,...,array + v7*s]
#define simd_i32gather_epi32 _mm256_i32gather_epi32
//f(base,v,mask,s) = [array + v1*s,array + v2*s,...,array + v7*s] maskで0になってるとこは読まない/
#define simd_mask_i32gather_epi32(base, index, mask, scale) _mm256_mask_i32gather_epi32(_mm256_setzero_si256(), (const int*)(base), (index), _mm256_castps_si256(mask), (scale))

// --- 128と256で名前が変わるやつのラッパー --- /

//32byteアライメントされてる場所からint8つをロード/
#define simd_load_si simd_load_si256
//256byte書き込む/
#define simd_store_si simd_store_si256
//全部に0をセットする/
#define simd_setzero_si simd_setzero_si256
//256bit全部のand
#define simd_and_si simd_and_si256
//or
#define simd_or_si simd_or_si256
//xor
#define simd_xor_si simd_xor_si256
//ps to si256 (再解釈するだけ)
#define simd_castsi_ps simd_castsi256_ps
//si256 to ps (再解釈するだけ)
#define simd_castps_si simd_castps_si256

#define simd_cvtsi_si32 simd_cvtsi256_si32 


// --- 互換性? --- /

//↓使わなくなった pixelが1byteやった時に使ってたやつ/
//v_pixelに変換/
//static inline v_pixel simd_pack_to_v_pixel(v_int v_c_32){//[7][6][5][4][3][2][1][0]
//	//128で分割/
//	__m128i lo_32 = _mm256_castsi256_si128(v_c_32);//[3][2][1][0] _mm256_castsi256_si128(v_c_32, 0)と同じだけどこれはコンパイラが型変換するのに等しいらしいから0サイクルになるらしい/
//	__m128i hi_32 = _mm256_extracti128_si256(v_c_32, 1);//[7][6][5][4]
//
//	__m128i v_c_16 = _mm_packs_epi32(lo_32, hi_32);//[7][6][5][4][3][2][1][0]
//	__m128i v_c_8 = _mm_packs_epi16(v_c_16, _mm_setzero_si128());//0000[7][6][5][4][3][2][1][0]
//
//	return _mm_cvtsi128_si64(v_c_8);//256bitに戻す/
//}

#else
static_assert(0, "マクロ SIMD_MODE が 128 or 256 になってない");
#endif