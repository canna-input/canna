/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 *
 *  Author : Osamu Hata, NEC Corporation  (hata@d1.bs2.mt.nec.co.jp)
 *
 */

/*
*/

#include "symbolname.h"

#define NHENKAN_MAX 9999

/* variables declarations */
/* ccustom.c */
extern char err_mess[];
extern char *initFileSpecified;
extern int is_icustom;
extern char *old_mode_ichiran[];
extern char *old_mode_ichiran2[], *old_mode_ichiran3[];
extern char *mode_ichiran2[], *mode_ichiran3[];

/* parse.c */
extern int InhibitHankakuKana;
extern char *Dictionary;
extern int IROHA_ParseError;
extern char IROHA_rcfilename[];
extern char CANNA_rcfilename[];
extern char readCannaFile[];

/* set.h */
extern char *kanjidicname[], *userdicname[],  *bushudicname[], *localdicname[]; 
extern int  nkanjidics, nuserdics, nbushudics, nlocaldics;
extern char  *RomkanaTable, *RengoGakushu, *KatakanaGakushu;
extern int InitialMode, CursorWrap, SelectDirect, HexkeySelect, BunsetsuKugiri;
extern int ChBasedMove, ReverseWidely, Gakushu, QuitIchiranIfEnd;
extern int kakuteiIfEndOfBunsetsu, stayAfterValidate, BreakIntoRoman;
extern int kouho_threshold, gramaticalQuestion;
extern char *mode_mei[], null_mode[];
extern int forceKana, kCount, chikuji, iListCB ,nKouhoBunsetsu;
extern int keepCursorPosition, CannaVersion, abandonIllegalPhono;
extern int hexCharacterDefiningStyle, kojin, ReverseWord , allowNextInput;
extern int indexhankaku,ignorecase,romajiyuusen,autosync,nkeysuu,quicklyescape;
extern int ckverbose, nothermodes;
extern int protocol_version;
extern int server_version;
#ifdef DEBUG
extern int iroha_debug;
#endif /* DEBUG */

extern char *allKey[], *alphaKey[], *yomiganaiKey[];
extern char *yomiKey[], *jishuKey[], *tankouhoKey[];
extern char *ichiranKey[], *zenHiraKey[], *zenKataKey[];
extern char *zenAlphaKey[], *hanKataKey[], *hanAlphaKey[];
extern char *allFunc[], *alphaFunc[], *yomiganaiFunc[];
extern char *yomiFunc[], *jishuFunc[], *tankouhoFunc[];
extern char *ichiranFunc[], *zenHiraFunc[], *zenKataFunc[];
extern char *zenAlphaFunc[], *hanKataFunc[], *hanAlphaFunc[];
extern int NallKeyFunc, NalphaKeyFunc, NyomiganaiKeyFunc, NyomiKeyFunc;
extern int NjishuKeyFunc, NtankouhoKeyFunc,  NichiranKeyFunc;
extern int NzenHiraKeyFunc, NzenKataKeyFunc, NzenAlphaKeyFunc;
extern int NhanKataKeyFunc, NhanAlphaKeyFunc;
extern char returnKey[];
extern char *funcList[];
extern char *cfuncList[];

/* functin prototypes */
char *showChar(int);
int scc(char *);
void tilda(char *);
void changeModeName(int, char *);
void initKeyFunc(void);
int specialen(unsigned char *);
void specpy(unsigned char *, unsigned char *);
char *copy_acts(unsigned char *);
char *copy_keys(unsigned char *);
void changeKeyfunc(int, int, int, unsigned char *, unsigned char *);
void cchangeKeyfunc(int, int, int, unsigned char *, unsigned char *);
void changeKeyfuncOfAll(int, int, unsigned char *, unsigned char *);
void append_dic(int, char *);
void delete_dic(int, int);
void etc_action(int, int);
void init_mode_mei(void);
void free_mode_mei(void);
char *toChar(int);
char *print_buff(unsigned char *, unsigned char *);
void write_iroha(FILE *);
char *toTnil(int);
void write_canna(FILE *);
void before_parse(void);
void parse_string(char *);
void parse(void);
void cparse(void);
int IROHA_input(void);
void IROHA_unput(int);
void IROHA_output(int);
int clisp_init(void);
void clisp_fin(void);
int LLparse_by_rcfilename(char *);
void exitccustom(void);
