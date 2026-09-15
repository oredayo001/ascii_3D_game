#if RASTERIZE_MODE == RM_NORMAL
#define FUNC_NAME(name) ATTACH(name,_normal)
#elif RASTERIZE_MODE == RM_BLEND//まあまだ作ってない/
#define FUNC_NAME(name) ATTACH(name,_blend)
#elif RASTERIZE_MODE == RM_TEMP
#define FUNC_NAME(name) ATTACH(name,_temp)
#else
#error
#endif

//0:sl 1:hs 2:switch
#define RASTERIZE_TYPE_X 1
#define RASTERIZE_TYPE (RASTERIZE_TYPE_X&&ENABLE_SIMD)

//ハーフスペースとスキャンラインどっち使うかの境目/
#define RASTERIZE_MODE_SWITCH_AREA (512.f)

#if ENABLE_SIMD + 0

#if WIDTH&(SIMD_ALIGN-1)
#error//widthがSIMD_ALIGNの倍数じゃない loadで死ぬ(loaduにしてない)
#endif
//ハーフスペース/
/*
	int xMin = MAX((int)(MIN3(sp[0].x, sp[1].x, sp[2].x)), 0);
	int xMax = MIN((int)(MAX3(sp[0].x, sp[1].x, sp[2].x)), (WIDTH - 1));
	int yMin = MAX((int)(MIN3(sp[0].y, sp[1].y, sp[2].y)), 0);
	int yMax = MIN((int)(MAX3(sp[0].y, sp[1].y, sp[2].y)), (HEIGHT - 1));
	int xMin = 0;
	int xMax = (WIDTH - 1);
	int yMin = 0;
	int yMax = (HEIGHT - 1);
*/
//test
//#include"scene/game/game.h"
static void FUNC_NAME(rasterizeTri_halfSpace)(Screen* __restrict sc, vec3* __restrict sp, vec2* __restrict uv, FaceContext* __restrict fCtx){

	// --- aabb --- /
	int xMin = MAX((int)(MIN3(sp[0].x, sp[1].x, sp[2].x)), 0);
	int xMax = MIN((int)(MAX3(sp[0].x, sp[1].x, sp[2].x)), (WIDTH - 1));
	int yMin = MAX((int)(MIN3(sp[0].y, sp[1].y, sp[2].y)), 0);
	int yMax = MIN((int)(MAX3(sp[0].y, sp[1].y, sp[2].y)), (HEIGHT - 1));
	if((xMax < xMin) || (yMax < yMin)) return;//画面外/
	// --- ループの開始終了場所 --- /
	int s_x = xMin & ALIGN_MASK;
	int e_x = (xMax + (SIMD_STEP - 1)) & ALIGN_MASK;

	//なんか使うやつ/
	v_float v_zero = simd_setzero_ps();
	v_float v_one = simd_set1_ps(1.f);

	// --- クラメルの公式の定数 --- /
	//E1=V1-V0 E2=V2-V0 S=P-V0 S=sE1+tE2 この時0<=s,0<=t,s+t<=1のとき三角形の内部にある
	//そんでP=rV0+sV1+tV2とするとPの場所のzはrz0+sz1+tz2 uvも同じ rは1-s-tで求まる |E1 E2|=fとおくと [E1 E2](s,t)=S から
	//s=|(s,t) (0,1)|=|S E2|/f, t=|(1,0) (s,t)|=|E1 S|/f, r=1-s-t
	//|S E2| = xs*ye-xe*ys = (xp-x0)*(y2-y0) - (yp-y0)*(x2-x0) = xp(y2-y0) + yp(x0-x2) + (x2y0 - x0y2) = xpA1 + ypB1 + C1
	//|E1 S| = xe*ys-xs*ye = (yp-y0)*(x1-x0) - (xp-x0)*(y1-y0) = xp(y0-y1) + yp(x1-x0) + (x0y1 - x1y0) = xpA2 + ypB2 + C2
	//要はfABCを求めたらいい/

	//ABC
	float A1 = (sp[2].y - sp[0].y), B1 = -(sp[2].x - sp[0].x), C1 = sp[2].x * sp[0].y - sp[0].x * sp[2].y;//s
	float A2 = -(sp[1].y - sp[0].y), B2 = (sp[1].x - sp[0].x), C2 = sp[0].x * sp[1].y - sp[1].x * sp[0].y;//t
	//f = |E1 E2| = (x1-x0)(y2-y0) - (x2-x0)(y1-y0)
	float f = B2 * A1 - (-B1) * (-A2);//まあ負の数は最適化で勝手に消えてくれるであろう/

	// ---- debug ---- /

	//float ttt = (float)getGameFrame() * 1.5f * 3.14f;
	//float ttt2 = ttt * 2;

	//float wave = sinf(ttt) * .001f + 1;
	//A1 *= wave;
	//A2 *= wave;
	//B1 *= wave;
	//B2 *= wave;
	//C1 *= wave;
	//C2 *= wave;
	//float d = fabsf(C1);

	//if(fCtx->debug___ < 2){
		//if(d > 100000.f){
		//	fCtx->debug___ = 2 - (d > 1000000.f) - (d > 10000000.f);
		//}
	//}
	//else fCtx->debug___ = 100;
	
	// ---- debug end ---- /

	//fと1/f
	v_float v_f = simd_set1_ps(f);
	v_float v_invf = simd_div_ps(v_one, v_f);

	//stのxyによる変化量/
	v_float v_dx_s = simd_mul_ps(simd_set1_ps(A1 * (float)SIMD_STEP), v_invf);//xがsimd_step進んだ時のtの増加量/
	v_float v_dy_s = simd_mul_ps(simd_set1_ps(B1 * 1.f), v_invf);//yが増えたときのsの増加量/
	v_float v_dx_t = simd_mul_ps(simd_set1_ps(A2 * (float)SIMD_STEP), v_invf);//xが増えたときのsの増加量/
	v_float v_dy_t = simd_mul_ps(simd_set1_ps(B2 * 1.f), v_invf);//yが増えたときのsの増加量/

	//stはsが1でtが2
#define v_s_line_val_4(x,y,st) ((ATTACH(A,st)*(x+3.f)) + (ATTACH(B,st)*(y)) + ATTACH(C,st)),((ATTACH(A,st)*(x+2.f)) + (ATTACH(B,st)*(y)) + ATTACH(C,st)),((ATTACH(A,st)*(x+1.f)) + (ATTACH(B,st)*(y)) + ATTACH(C,st)),((ATTACH(A,st)*(x+0.f)) + (ATTACH(B,st)*(y)) + ATTACH(C,st))
#define v_s_line_val_8(x,y,st) v_s_line_val_4(x+4.f,y,st),v_s_line_val_4(x,y,st)
#define v_s_line_val(x,y,st) ATTACH(v_s_line_val_,SIMD_STEP)(x,y,st)
	//stの左上 .5fはピクセルの真ん中にするための/
	v_float v_s_line = simd_mul_ps(simd_set_ps(v_s_line_val((float)s_x + .5f, (float)yMin + .5f, 1)), v_invf);
	v_float v_t_line = simd_mul_ps(simd_set_ps(v_s_line_val((float)s_x + .5f, (float)yMin + .5f, 2)), v_invf);

	//zの逆数を取り出す/
	v_float v_invz0 = simd_set1_ps(sp[0].z); v_float v_invz1 = simd_set1_ps(sp[1].z); v_float v_invz2 = simd_set1_ps(sp[2].z);
	//uv * zの逆数を取り出す/
	v_float v_u0_invz = simd_set1_ps(uv[0].x); v_float v_u1_invz = simd_set1_ps(uv[1].x); v_float v_u2_invz = simd_set1_ps(uv[2].x);
	v_float v_v0_invz = simd_set1_ps(uv[0].y); v_float v_v1_invz = simd_set1_ps(uv[1].y); v_float v_v2_invz = simd_set1_ps(uv[2].y);

	for(int y = yMin; y <= yMax; y++){//多分含む/
		//行の先頭　バッファは各行アライメント済み　s_xがsimd_stepの倍数だからloaduを使わなくていい/
		float* zb = &(sc->zbuff[s_x + (y * WIDTH)]);
		pixel_t* scp = &(sc->screen[s_x + (y * WIDTH)]);
		//st
		v_float v_s = v_s_line;
		v_float v_t = v_t_line;
		for(int x = s_x; x < e_x; x += SIMD_STEP){//含まない/
			//debug
			//v_s = simd_mul_ps(simd_set_ps(v_s_line_val((float)x + .5f, (float)y + .5f, 1)), v_invf);
			//v_t = simd_mul_ps(simd_set_ps(v_s_line_val((float)x + .5f, (float)y + .5f, 2)), v_invf);
			// --- 合計 --- /
			v_float v_st_sum = simd_add_ps(v_s, v_t);
			v_float v_r = simd_sub_ps(v_one, v_st_sum);//1-(s+t)

			// --- 三角形の内側かのマスク --- /
			//0<=s 0<=t s+t<=1
			v_float v_sMask = simd_cmple_ps(v_zero, v_s);//真が-1
			v_float v_tMask = simd_cmple_ps(v_zero, v_t);
			v_float v_sumMask = simd_cmple_ps(v_st_sum, v_one);

			// --- zbuff --- /
			//アライメント済み/
			v_float v_invz_old = simd_load_ps(zb);
			//rz0+sz1+tz2
			v_float v_invz_new = simd_add_ps(simd_add_ps(simd_mul_ps(v_r, v_invz0), simd_mul_ps(v_s, v_invz1)), simd_mul_ps(v_t, v_invz2));
			//でかい方が手前 old<new
			v_float v_zMask = simd_cmplt_ps(v_invz_old, v_invz_new);

			// --- 合わせる --- /
			v_float v_mask = simd_and_ps(simd_and_ps(simd_and_ps(v_sMask, v_tMask), v_sumMask), v_zMask);//書き込めるピクセルのマスク/

			if(simd_movemask_ps(v_mask) == 0){//一旦/
				goto skipLogic;
			}

			// --- zbuff書き込み --- /
			v_float v_invz_blended = simd_blendv_ps(v_invz_old/*false*/, v_invz_new/*true*/, v_mask);
			simd_store_ps(zb, v_invz_blended);

			// --- テクスチャ --- /
			v_float v_u_invz = simd_add_ps(simd_add_ps(simd_mul_ps(v_r, v_u0_invz), simd_mul_ps(v_s, v_u1_invz)), simd_mul_ps(v_t, v_u2_invz));
			v_float v_v_invz = simd_add_ps(simd_add_ps(simd_mul_ps(v_r, v_v0_invz), simd_mul_ps(v_s, v_v1_invz)), simd_mul_ps(v_t, v_v2_invz));

			v_float v_z = simd_rcp_ps(v_invz_new);
			//v_float v_z = simd_div_ps(v_one,v_invz_new);//あんま変わらんかった/

			v_float v_u = simd_mul_ps(v_u_invz, v_z);
			v_float v_v = simd_mul_ps(v_v_invz, v_z);

			v_int v_ascii_old = simd_load_si((v_int*)scp);//アライメント済み/
			v_int v_ascii_new = getAsciiShade_simd(v_u, v_v, v_invz_new, fCtx);
			v_int v_ascii_blended = simd_castps_si(simd_blendv_ps(simd_castsi_ps(v_ascii_old)/*false*/, simd_castsi_ps(v_ascii_new)/*true*/, v_mask));//混ぜる/

			simd_store_si((v_int*)scp, v_ascii_blended);

		skipLogic:
			// --- 進める --- /
			zb += SIMD_STEP;
			scp += SIMD_STEP;
			v_s = simd_add_ps(v_s, v_dx_s);
			v_t = simd_add_ps(v_t, v_dx_t);
		}
		//yの増加量を足す/
		v_s_line = simd_add_ps(v_s_line, v_dy_s);
		v_t_line = simd_add_ps(v_t_line, v_dy_t);
	}
}

//なんかこの辺スネークケース多いな/

//多分テクスチャとスクリーンを1byte型じゃなくて4byte型使えばちょっと早くなる可能性　まあめんどいからやらんけど/
//   ↓120x120だと2500fpsくらい/
//  ↓普通にzbuffからのシェーディングが重かったっぽかったからなくしたらまあまあ上がった　というか統合しても結構早い 大体700-800/
// ↓まあなんかバグが治って最終的には600fps まあ二倍かな/
//↓計算後(途中) まあバグってはいるけどまだ今んとこは1500fps位出てる　バグ修正後どうなるのか/
//uv計算する前の情報 なんかsimdここでちょっとやるだけで300->1000fps(512x256)  uvを入れたらどれだけ遅くなるかにかかってる/
static void FUNC_NAME(_fillHLineZ)(Screen* sc, int min, int max, vec2 uv1, vec2 s_uv, int y, float invz1, float sz, FaceContext* fCtx){
	/*関数名は _mmビット数_演算名_データ型　ビット数は128 256 512があって128の時は何も書かんでいい 演算名はaddとかmulとかstoreとか　データ型は
	ps:packed single -> 4byte 浮動小数 float 128ならfloat4つ
	pd:packed double -> 8byte 2倍精度浮動小数 double 128ならdouble2つ
	epi:extended packed integer -> 整数型 32 16 8 がついたらそれぞれそのビット数の型
	epu:extended packed unsigned integer -> epiの符号なし
	si:signed integer -> si128は128bitのでかい数としてなんか扱う 例えば simd_cvtsi128_si32これは signed int 128 を signed int 32 にconvertする って意味

	ちなみにepiとかのextendedは昔は64bitでやるやつやったのが進化して128bitでやるやつになったから識別につけなあかんくなった
	逆にfloatにはついてないのは64bitの時はfloatのがなくて128の時に追加されたからそもそもeを付ける必要がなかった って感じらしい　
	*/
	int x = min & ALIGN_MASK;//アライメント/
	int max_simd = (max + (SIMD_STEP - 1)) & ALIGN_MASK; // はみ出ないとこまで/
	//x=0のとこは4の倍数でx>=0だから

	//スクリーンの開始ポインタ/
	uint32_t scInd = x + y * WIDTH;
	float* zb = &(sc->zbuff[scInd]);//添え字が4の倍数ならfloatが4byteだから4x4でzbuff+16n(byte)になる あとはzbuffが16の倍数であればいい ちなみに64byteの境界にいる/
	pixel_t* scp = &(sc->screen[scInd]);//まあscIndが4の倍数でかつscreen自体は4byte以上のアライメントされてる/

	float x_steped = (float)(min - x);//アライメントによって戻った数/

	//ステップ数で分岐させるやつ その1/

#define STEP_TABLE_4 { (3.f - x_steped), (2.f - x_steped), (1.f - x_steped), (.0f - x_steped) }
#define STEP_TABLE_8 { (7.f - x_steped), (6.f - x_steped), (5.f - x_steped), (4.f - x_steped),(3.f - x_steped), (2.f - x_steped), (1.f - x_steped), (.0f - x_steped), }
#define STEP_TABLE ATTACH(STEP_TABLE_,SIMD_STEP)

	//呼び出し元はminから始める想定で引数を渡してるけど実際始めるのはx体から戻った分ちょっと開始位置をずらす必要がある/
	float stepTable[SIMD_STEP] = STEP_TABLE;
	//ステップ数で分岐させるやつ その2/
#define _set_ps_arg_steped_4(val,step) (val) + (step) * (stepTable[0]), (val) + ((step) * stepTable[1]), (val) + ((step) * stepTable[2]), (val) + (step) * stepTable[3]
#define _set_ps_arg_steped_8(val,step) (val) + (step) * (stepTable[0]), (val) + ((step) * stepTable[1]), (val) + ((step) * stepTable[2]), (val) + (step) * stepTable[3], (val) + (step) * (stepTable[4]), (val) + ((step) * stepTable[5]), (val) + ((step) * stepTable[6]), (val) + (step) * stepTable[7]
#define set_ps_arg_steped ATTACH(_set_ps_arg_steped_,SIMD_STEP)

	// 4px分のinvz なんか右から詰めるっぽい?/
	v_float v_invz = simd_set_ps(set_ps_arg_steped(invz1, sz));//新しい方/
	//なんか多分メモリ上では　0|1|2|3
	//って並んでるけどこれは　3|2|1|0
	//って並んでる 多分/

	// v_invzの各要素の増加量　ってゆうたけど全部同じ 4sz
	v_float v_sz4 = simd_set1_ps(sz * (float)SIMD_STEP);

	// uv なんか変数名が顔文字みたいやな ←変数名変わったせいで顔っぽさなくなった/
	v_float v_u_invz = simd_set_ps(set_ps_arg_steped(uv1.x, s_uv.x));
	v_float v_v_invz = simd_set_ps(set_ps_arg_steped(uv1.y, s_uv.y));
	v_float v_s_u = simd_set1_ps(s_uv.x * (float)SIMD_STEP);
	v_float v_s_v = simd_set1_ps(s_uv.y * (float)SIMD_STEP);
	//float cnt = 0.f;

	//v_float v_one = simd_set1_ps(1.f);
	for(; x < max_simd; x += SIMD_STEP){
		//ステップ数で分岐させるやつ その3/
#define set_x_indices_4 x + 3, x + 2, x + 1, x
#define set_x_indices_8 x + 7, x + 6, x + 5, x + 4,x + 3, x + 2, x + 1, x
#define set_x_indices ATTACH(set_x_indices_,SIMD_STEP)
		//インデックスを超えてるかのマスク min<=x<=max/
		v_int v_x_indices = simd_set_epi32(set_x_indices);
		v_int v_min = simd_set1_epi32(min);
		v_int v_max = simd_set1_epi32(max - 1);//-1すると<=が<になる/

		//min<=x&&x<=max-1
		v_int v_x_in_mask_i = simd_and_si(
			simd_cmplt_epi32(simd_sub_epi32(v_min, simd_set1_epi32(1)), v_x_indices), // v_min <= x
			simd_cmplt_epi32(simd_sub_epi32(v_x_indices, simd_set1_epi32(1)), v_max)  // x <= v_max
		);

		//float用(?)
		v_float v_x_in_mask = simd_castsi_ps(v_x_in_mask_i);

		// 4px分/
		v_float v_zb = simd_load_ps(zb);//古い方 16byteアライメントされてる/

		//((第一)<(第二))?-1:0
		v_float v_invz_mask = simd_cmplt_ps(v_zb, v_invz);

		//zbuffの比較と画面外の比較を合成する/
		v_float v_mask = simd_and_ps(v_invz_mask, v_x_in_mask);

		if(simd_movemask_ps(v_mask) == 0){//これ以降がでかいから分岐ミスっても早いんかな cpuがfalseと仮定してくれると多分速い(憶測)/
			goto skip_pixel_proc;//今んとこあるほうが速い　多分　600~700 -> 700~800
		}

		// maskによってどっちかを選ぶ 0:第一引数 -1:第二引数/
		v_float v_new_zb = simd_blendv_ps(v_zb/*false*/, v_invz/*true*/, v_mask);
		simd_store_ps(zb, v_new_zb); // 4ピクセル分の書き込み アライメントはされてるはず/

		// 逆数/
		v_float v_z = simd_rcp_ps(v_invz);// 1.f/invz
		//v_float v_z = simd_div_ps(v_one,v_invz);// 1.f/invz
		//とりあえず空白/
		//uint32_t v_ascii_new = ((asciiShade_0 & 0xff) << (8 * 3)) | ((asciiShade_0 & 0xff) << (8 * 2)) | ((asciiShade_0 & 0xff) << (8 * 1)) | ((asciiShade_0 & 0xff) << (8 * 0));

		//圧縮/
		//まあこれはf(a,b)ってのがあってそれぞれa=|32|32|32|32|,b=|32|32|32|32|ってなってたとすると
		//f(a,b) = |16|16|16|16|16|16|16|16| って感じにする epi32はaとbを128bitの中に32bitの整数型が詰められたとしてみてそれぞれを|a|b|って詰める
		//epi16も16bitが詰められてるとしてみてる感じ　それが二つ詰められるから縮小されて各要素は8bitまで詰められるって感じ　上にビットが切り捨てられる
		//packsのsは飽和処理って意味　0x1234を上位1byteを消すとき飽和処理がないと0x34になるけどあると0x7fになる/

		//やばい変数名が顔に見えてきた 何これ/
		v_float v_u = simd_mul_ps(v_u_invz, v_z);
		v_float v_v = simd_mul_ps(v_v_invz, v_z);

		_ASSERT(!(((size_t)scp) & (SIMD_ALIGN - 1)), "アライメント");
		//テクスチャ/
		v_int v_ascii_old = simd_load_si((v_int*)scp);//アライメントされてる/
		v_int v_ascii_new = getAsciiShade_simd(v_u, v_v, v_invz, fCtx);//uv座標からテクスチャを読んでついでに影もつける/
		v_int v_ascii_32 = simd_castps_si(simd_blendv_ps(simd_castsi_ps(v_ascii_old)/*false*/, simd_castsi_ps(v_ascii_new)/*true*/, v_mask));//混ぜる/

		simd_store_si((v_int*)scp, v_ascii_32);//書き込み/

		/*
		並びについてで基本的にリトルエンディアンってのになってて　まあこれは演算の時じゃなくてメモリの読み書きの時に関係するやつなんやけど例えば
		0xABCDEFGHってのはメモリ上では
		|AB|CD|EF|GH|　って並ぶのがビッグエンディアン
		|GH|EF|CD|AB|　って並ぶのがリトルエンディアン
		基本的にリトルエンディアンでやられててなんでかというとリトルエンディアンならキャストの時にまあ基本4byte型なら4byteアライメントされてるはずやから
		|GH|EF|CD|AB|これを1byteにキャストするなら|GH|無視|無視|無視| ってできる
		ビッグエンディアンなら
		|AB|CD|EF|GH|これを1byteにキャストするなら|無視|無視|無視|GH| ってなるから必要なデータが気持ち悪い位置にあることになる
		昔のcpuは1byteづつしか読めなかったからその場所を読むにはp+3を計算する必要があるけどリトルエンディアンなら必要がない
		演算の時もリトルエンディアンならp+0,p+1,p+2,p+3の順に読んでそのままその純に計算して繰り上がってって感じになる
		って風にメリットが大きかったけど今のcpuはそこまでメリットはない　でもそれが昔の名残で残って他的な背景があるっぽい
		まあ互換性がどうのこうのってやつかな?
		*/

	skip_pixel_proc:

		// 進める まあxに連続だからポインタはそのまま進める/
		v_invz = simd_add_ps(v_invz, v_sz4);
		v_u_invz = simd_add_ps(v_u_invz, v_s_u);
		v_v_invz = simd_add_ps(v_v_invz, v_s_v);
		zb += SIMD_STEP;
		scp += SIMD_STEP;
	}
}
//simdは複数の数を一気に計算できるだけやと思ってたけど読み込みも一応同じく一気に読めるっぽい?から早くなったと思われ/
#endif

//ドット書き込み/
static inline void FUNC_NAME(putDot)(Screen* sc, int x, int y, pixel_t l){
	_ASSERT((0 <= x) && (x < WIDTH) && (0 <= y) && (y < HEIGHT), "範囲外");
	sc->screen[x + (y * (WIDTH))] = l;
}

#if !(ENABLE_SIMD + 0)
static void FUNC_NAME(_fillHLineZ)(Screen* sc, int min, int max, vec2 uv1, vec2 s_uv, int y, float invz1, float sz, FaceContext* fCtx){
	uint32_t scInd = min + y * WIDTH;
	float* zb = &(sc->zbuff[scInd]);
	pixel_t* scp = &(sc->screen[scInd]);
#pragma loop(hint_parallel(0))
	for(int x = min; x < max; x++){
		//uv
#if UV_MODE == uvmode_exact
		float z = 1.f / invz1;
		vec2 uv = v2mul(uv1, z);
#else
		vec2 uv = uv1;
#endif
		pixel_t ascii = getAsciiShade(uv, invz1, fCtx);
		if(*zb < invz1){
			*zb = invz1;
			*scp = ascii;
			//FUNC_NAME(putDot)(sc, x, y, ascii);
		}
		invz1 += sz;

		zb++;
		scp++;
		//uv1 = v2add(uv1, s_uv);
		uv1.x += s_uv.x;
		uv1.y += s_uv.y;
	}
}
#endif

//yは画面内前提　xは画面外ならクリッピングする/
//yの位置にminからmaxまで水平に書き込む/
//min<max前提/
static void FUNC_NAME(fillHLineZ)(Screen* sc, int min, int max, vec2 uv1, vec2 uv2, int y, float invz1, float invz2, FaceContext* fCtx){
	//xの差/
	int dx = max - min;
	if(dx <= 0) return;
	//uvの差/
#if (UV_MODE == uvmode_fast)
	float z1 = 1.f / invz1;
	float z2 = 1.f / invz2;
	uv1 = v2mul(uv1, z1);
	uv2 = v2mul(uv2, z2);
#endif
	vec2 sub_uv = v2sub(uv2, uv1);
	//呼び出し元が範囲外クリッピングするようになったから要らんくなった/
	//ASSERT(0 <= y && y < HEIGHT, "クリッピングできてない");
	//if((dx <= 0) /* || (y < 0 || y >= HEIGHT)*/) return;
	//xの始点から終点までのマス目分でのzの逆数の差/
	float dz = invz2 - invz1;
	//
	float inv_dx = 1.f / (float)dx;
	//1マス当たりのzの逆数の差/
	float sz = dz * inv_dx;
	//1マス当たりのuvの増加量/
	vec2 s_uv = v2mul(sub_uv, inv_dx);
	if(min < 0){
		//開始地点が変わる -minは超過分の升目の数 負の数だから-を付けたら+になる/
		invz1 += sz * (-min);//z修正/
		uv1 = v2add(uv1, v2mul(s_uv, (float)(-min)));
		min = 0;
	}
	if((WIDTH) < max){
		max = WIDTH;
	}
	FUNC_NAME(_fillHLineZ)(sc, min, max, uv1, s_uv, y, invz1, sz, fCtx);
}

//1が短いほう/
//zはinv
//yでソート済み/
static void FUNC_NAME(fillSection)(Screen* __restrict sc, vec3 s_sp1, vec2 s_uv1, vec3 s_sp2, vec2 s_uv2, vec3 e_sp1, vec2 e_uv1, vec3 e_sp2, vec2 e_uv2, FaceContext* __restrict fCtx){
	//sy1からey1までループ/
	//2のyは傾きの計算にしか使わない/
	//LOW 呼び出し元が元から1/dyを求めると速そう/

	//sub
	vec3 sub1 = v3sub(e_sp1, s_sp1);
	vec3 sub2 = v3sub(e_sp2, s_sp2);
	vec2 d_uv1 = v2sub(e_uv1, s_uv1);
	vec2 d_uv2 = v2sub(e_uv2, s_uv2);

	//ゴミ/
	if(sub1.y == 0 || sub2.y == 0) return;//0除算防止/

	float invDY1 = 1.f / sub1.y;
	float invDY2 = 1.f / sub2.y;

	//yが1進んだ時のzの増加量/
	float z1_s = sub1.z * invDY1;
	float z2_s = sub2.z * invDY2;

	//yが1進んだ時のxの増加量/
	float dx1 = sub1.x * invDY1;
	float dx2 = sub2.x * invDY2;

	//yが1進んだ時のuvの増加量/
	vec2 delta_uv1 = v2mul(d_uv1, invDY1);
	vec2 delta_uv2 = v2mul(d_uv2, invDY2);

	int sy1 = (int)s_sp1.y;
	int ey1 = (int)e_sp1.y;

	//クリッピング/
	if(sy1 < 0){
		//超過分/
		float sub = -s_sp1.y;

		//画面外をスキップ/
		s_sp1.x += dx1 * sub;
		s_sp2.x += dx2 * sub;
		s_sp1.z += z1_s * sub;
		s_sp2.z += z2_s * sub;
		s_uv1 = v2add(s_uv1, v2mul(delta_uv1, sub));
		s_uv2 = v2add(s_uv2, v2mul(delta_uv2, sub));

		sy1 = 0;
	}
	if(HEIGHT <= ey1){
		ey1 = HEIGHT - 1;
	}
	// --- ここからは s/e_sp1.y は使わない　使うとs/ey1とずれる --- /
	for(int y = sy1; y < ey1; y++){//y更新/
		// --- fill ---
		if(s_sp1.x < s_sp2.x)
			FUNC_NAME(fillHLineZ)(sc, (int)s_sp1.x, (int)s_sp2.x, s_uv1, s_uv2, y, s_sp1.z, s_sp2.z, fCtx);
		else
			FUNC_NAME(fillHLineZ)(sc, (int)s_sp2.x, (int)s_sp1.x, s_uv2, s_uv1, y, s_sp2.z, s_sp1.z, fCtx);

		// --- update x --- /
		//deltaを足す/
		s_sp1.x += dx1;
		s_sp2.x += dx2;

		// --- update z --- /
		s_sp1.z += z1_s;
		s_sp2.z += z2_s;

		// --- update uv ---  /
		s_uv1 = v2add(s_uv1, delta_uv1);
		s_uv2 = v2add(s_uv2, delta_uv2);
	}
}

//↓uv座標とかマップとかやる前のやつ　今は25万ポリゴンで40fpsくらい　多分今後も下がっていくはず 2026/8/20 /
//この辺にrestrictつけたら50fps->57fps(50万ポリゴン)
//逆にconstを変につけまくったら47fpsに下がった　なんでや(一応呼び出し元にもつけたけど)/
static void FUNC_NAME(rasterizeTri)(Screen* __restrict sc, vec3* __restrict sp, vec2* __restrict uv, FaceContext* __restrict fCtx){
	// --- yで小さい順にソート --- /
	for(int i = 0; i < 2; i++){
		for(int j = i + 1; j < 3; j++){
			if(sp[j].y < sp[i].y){
				//交換/
				//sp
				vec3 tempv3 = sp[i]; sp[i] = sp[j]; sp[j] = tempv3;
				//uv
				vec2 tempv2 = uv[i]; uv[i] = uv[j]; uv[j] = tempv2;
			}
		}
	}
	//0からスタートして1の高さで終わって次1の高さからスタートして2の点で終わる/
	//1個目は辺01と02と点1を通る水平な線で囲まれた三角形/
	//2個目は辺12と02と点1を通る水平な線で囲まれた三角形/

	// --- 描画 --- /
	//上半分/
	FUNC_NAME(fillSection)(sc, sp[0], uv[0], sp[0], uv[0], sp[1], uv[1], sp[2], uv[2], fCtx);

	//02の辺上でyが点1の場所のxzを求める/
	vec3 sub = v3sub(sp[2], sp[0]);
	float t = (sp[1].y - sp[0].y) / (sub.y);//02の辺の0から1までの長さの比/
	sp[0].x += ((sub.x) * t);//初期位置に全体に01までの長さの比をかけたものを足す/
	sp[0].z += (sp[2].z - sp[0].z) * t;//同様/
	sp[0].y = sp[1].y;
	uv[0] = v2add(uv[0], v2mul(v2sub(uv[2], uv[0]), t));//uvの/
	//下半分/
	FUNC_NAME(fillSection)(sc, sp[1], uv[1], sp[0], uv[0], sp[2], uv[2], sp[2], uv[2], fCtx);
}

static void FUNC_NAME(rasterizeTriCopyArray)(Screen* sc, const vec3* __restrict _sp, const vec2* __restrict _uv, FaceContext* __restrict fCtx){
	vec3 sp[3];
	vec2 uv[3];
	memcpy(sp, _sp, sizeof(vec3) * 3);
	memcpy(uv, _uv, sizeof(vec2) * 3);
	FUNC_NAME(rasterizeTri)(sc, sp, uv, fCtx);
}

#ifndef __RASTERIZETEMPLATE__
//スクリーン座標に変換したポリゴンの二辺の外積を返す/
static inline float getTriArea(vec3* sp){
	vec2 edge1 = (vec2){ sp[1].x - sp[0].x, sp[1].y - sp[0].y };
	vec2 edge2 = (vec2){ sp[2].x - sp[0].x, sp[2].y - sp[0].y };
	float r = v2det(edge1, edge2);
	_ASSERT(0 <= r, "面積が負(外積の書ける順番が逆)");
	return r;
}
#endif
#define __RASTERIZETEMPLATE__

//カメラ座標が欲しい/
static void FUNC_NAME(drawTri3d)(Screen* sc, vec3 cp[3], vec2 uv[3], float fov, FaceContext* fCtx){
	// --- ニアクリップ ---/
	vartex out[4] = { 0 };
	int cnt = 0;
	for(unsigned int i = 0; i < 3; i++){
		//今と次の点/
		vec3 c = cp[i];
		vec3 n = cp[(i + 1) % 3];
		vec2 c_uv = uv[i];
		vec2 n_uv = uv[(i + 1) % 3];
		//カメラより前か//
		int cIn = near < c.z;
		int nIn = near < n.z;
		//入ってたらまずそこを入れる/
		if(cIn){
			out[cnt].v = c;
			out[cnt].uv = c_uv;
			cnt++;
		}
		//次がはみ出てた場合は今入れたやつともう一個次見るやつとの間のz=nearの場所を探してそこを入れる/
		//逆に今はみ出てて次が出てない場合は今の位置を入れずに(今の位置ははみ出てるから入れる必要がない)次との間のz=nearの場所を入れる/
		if(cIn != nIn){
			if(cIn)fCtx->debug___ = i;
			//引数1ははみ出てるほう/
			//渡した二つの点を結ぶ辺上でz=nearとなる位置を線形補完で計算する/
			out[cnt++] = cIn ?//今見てるやつが入ってるかをチェック/
				getNearIntersection(n, n_uv, c, c_uv) ://nがはみ出てるからnを引数1に入れる/
				getNearIntersection(c, c_uv, n, n_uv);//cがはみ出てるからcを引数1に入れる/
		}
		//今見てる頂点がはみ出てないかつ次がはみ出てたら2つoutに入れる・/
		//2つともがはみ出てたら1つも入れない/
	}

	//スクリーン座標/
	vec3 sp[4];
	vec2 tri_uv[4];

	for(int i = 0; i < 4; i++){
		tri_uv[i] = out[i].uv;
	}

	// --- 三角形描画 --- /
	if(cnt == 3){//1個もはみ出てない　もしくは2つだけはみ出てた時　まあ要は三角形の時の描画/
		//変換 カメラからスクリーン まあzで割ってfovかけるだけ ここで一緒に1/zも求めとく/
		for(int i = 0; i < 3; i++){
			cPosToScPos(out[i].v, &sp[i], fov);

#if (UV_MODE == uvmode_exact)||(UV_MODE == uvmode_fast)
			tri_uv[i] = v2mul(tri_uv[i], sp[i].z);
#endif
		}
		//線 まあワイヤーフレームの描画/
#if ACTIVE_WIRE
		drawLine(sc, sp[0], sp[1]);// 0 1
		drawLine(sc, sp[1], sp[2]);// 1 2
		drawLine(sc, sp[2], sp[0]);// 2 0
#endif
		//埋める/
#if RASTERIZE_TYPE == 0
		FUNC_NAME(rasterizeTri)(sc, sp, tri_uv, fCtx);//032
#elif RASTERIZE_TYPE == 1
		FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
#elif RASTERIZE_TYPE == 2
		float area = getTriArea(sp);
		if(area < RASTERIZE_MODE_SWITCH_AREA){
			FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
		}
		else{
			FUNC_NAME(rasterizeTri)(sc, sp, tri_uv, fCtx);//012
		}
#endif
	}
	else if(cnt == 4){//1展だけはみ出てて四角形になった時 なんで1てんで展に変換されるん/
		// - 01 12　の線 -/
		//変換 カメラからスクリーン/
		for(int i = 0; i < 3; i++){
			cPosToScPos(out[i].v, &sp[i], fov);
#if (UV_MODE == uvmode_exact)||(UV_MODE == uvmode_fast)
			tri_uv[i] = v2mul(tri_uv[i], sp[i].z);
#endif
		}
		//線/
#if ACTIVE_WIRE
		drawLine(sc, sp[0], sp[1]);// 0 1
		drawLine(sc, sp[1], sp[2]);// 1 2
#endif
		//012の三角形を埋める 3つの配列は後で使うから配列コピーするほうを使う/

#if RASTERIZE_TYPE == 0
		FUNC_NAME(rasterizeTriCopyArray)(sc, sp, tri_uv, fCtx);//012
#elif RASTERIZE_TYPE == 1
		FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
#elif RASTERIZE_TYPE == 2
		float area = getTriArea(sp);
		if(area < RASTERIZE_MODE_SWITCH_AREA){
			FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
		}
		else{
			FUNC_NAME(rasterizeTriCopyArray)(sc, sp, tri_uv, fCtx);//012
		}
#endif
		// - 23 30 の線 -/
		//変換/
		cPosToScPos(out[3].v, &sp[1], fov);//1からのは引き切って用済みやから3に変更する/
#if (UV_MODE == uvmode_exact)||(UV_MODE == uvmode_fast)
		tri_uv[1] = v2mul(tri_uv[3], sp[1].z);
#else
		tri_uv[1] = tri_uv[3];
#endif

		//線/
#if ACTIVE_WIRE
		drawLine(sc, sp[2], sp[1]);// 2 3  3は1に入れられてる/
		drawLine(sc, sp[1], sp[0]);// 3 0  3は1に入れられてる/
#endif

		// 032の三角形を埋める/

#if RASTERIZE_TYPE == 0
		FUNC_NAME(rasterizeTri)(sc, sp, tri_uv, fCtx);//032
#elif RASTERIZE_TYPE == 1
		FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
#elif RASTERIZE_TYPE == 2
		area = getTriArea(sp);
		if(area < RASTERIZE_MODE_SWITCH_AREA){

			FUNC_NAME(rasterizeTri_halfSpace)(sc, sp, tri_uv, fCtx);//012
		}
		else{
			FUNC_NAME(rasterizeTri)(sc, sp, tri_uv, fCtx);//032
		}
#endif
	}
}

//2十で定義されないようにするやつ/
#undef FUNC_NAME

/*
128x128
SL:1000-2000fps
HS:2000-4000fps
切り替え:1000-1800
512x256
SL:450-800fps
HS:450-800
切り替え:800-1100

*/
