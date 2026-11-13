/*
 * css.h - MiniBrow のスタイルシート (CSS) の解析と適用
 *
 * 対応範囲:
 *  - @media (画面の幅などで判定) / @supports / @layer の中身、CSS ネストは読み飛ばす
 *  - セレクタ: 要素名 * .class #id [属性] 子孫 子(>) 隣(+) 兄弟(~) :link :visited :root
 *    :not() :is() :where() :has() (中はセレクタの並び。:has() は > + ~ で始まる相対セレクタも)
 *    :first-child :last-child :only-child :nth-child() :nth-last-child()
 *    :first-of-type :last-of-type :only-of-type :nth-of-type() :nth-last-of-type()
 *    :empty :checked :disabled :enabled :focus :focus-within :hover :active :target :lang()
 *    ::first-letter ::first-line ::before ::after。それ以外を含むルールは捨てる
 *  - 詳細度・記述順・!important・style 属性・カスタムプロパティ (:root 等で定義したもの)
 *  - プロパティ: 文字の装飾、display、背景色、枠線、マージン、パディング、幅、float、Flexbox、Grid など
 */
#ifndef CSS_H
#define CSS_H

#include <windows.h>

#define CSS_UNSET       (-32768)        /* 長さの指定なし */
#define CSS_AUTO        (-32767)        /* auto */
#define CSS_NOCOLOR     0xFFFFFFFFUL
#define CSS_TRANSPARENT 0xFFFFFFFEUL    /* 枠線の色: transparent (描かない) */

struct CssVarNode;                      /* 要素のカスタムプロパティの並び (css.c の中) */

enum { CSS_D_INLINE, CSS_D_BLOCK, CSS_D_LISTITEM, CSS_D_NONE, CSS_D_CELL,
       CSS_D_INLINEBLOCK, CSS_D_TABLE, CSS_D_ROW, CSS_D_ROWGROUP };
/* position */
enum { CSS_P_STATIC, CSS_P_RELATIVE, CSS_P_ABSOLUTE, CSS_P_FIXED };
/* overflow */
enum { CSS_O_VISIBLE, CSS_O_HIDDEN, CSS_O_AUTO, CSS_O_SCROLL };
enum { CSS_F_SANS, CSS_F_SERIF, CSS_F_MONO };
enum { CSS_A_LEFT, CSS_A_CENTER, CSS_A_RIGHT, CSS_A_JUSTIFY };
enum { CSS_TOP, CSS_RIGHT, CSS_BOTTOM, CSS_LEFT };
/* vertical-align */
enum { CSS_V_BASE, CSS_V_SUB, CSS_V_SUPER, CSS_V_TOP, CSS_V_TEXTTOP, CSS_V_MIDDLE, CSS_V_BOTTOM,
       CSS_V_TEXTBOTTOM, CSS_V_LEN };
/* list-style-type (0 = 指定なし: ul は入れ子の深さで disc / circle / square、ol は decimal) */
enum { CSS_L_DEFAULT, CSS_L_DISC, CSS_L_CIRCLE, CSS_L_SQUARE, CSS_L_DECIMAL, CSS_L_DECIMAL0,
       CSS_L_LROMAN, CSS_L_UROMAN, CSS_L_LALPHA, CSS_L_UALPHA, CSS_L_LGREEK,
       CSS_L_HIRAGANA, CSS_L_KATAKANA, CSS_L_HIRAIROHA, CSS_L_KATAIROHA, CSS_L_CJK };
/* cursor (0 = auto: リンクなら指の形) */
enum { CSS_C_AUTO, CSS_C_DEFAULT, CSS_C_POINTER, CSS_C_TEXT, CSS_C_WAIT, CSS_C_PROGRESS, CSS_C_MOVE,
       CSS_C_CROSS, CSS_C_HELP, CSS_C_NO, CSS_C_NS, CSS_C_EW, CSS_C_NWSE, CSS_C_NESW, CSS_C_NONE };
/* flex-direction */
enum { CSS_FD_ROW, CSS_FD_ROWREV, CSS_FD_COL, CSS_FD_COLREV };
/* justify-content / align-items / align-self / align-content (AUTO = 既定: justify は先頭、align は stretch) */
enum { CSS_AL_AUTO, CSS_AL_STRETCH, CSS_AL_START, CSS_AL_END, CSS_AL_CENTER, CSS_AL_BASELINE,
       CSS_AL_BETWEEN, CSS_AL_AROUND, CSS_AL_EVENLY };
enum { CSS_OF_FILL, CSS_OF_CONTAIN, CSS_OF_COVER, CSS_OF_NONE, CSS_OF_SCALEDOWN };     /* object-fit */
enum { CSS_DS_SOLID, CSS_DS_DOUBLE, CSS_DS_DOTTED, CSS_DS_DASHED, CSS_DS_WAVY };       /* text-decoration-style */
/* border-style の線種 (0 = なし) */
enum { CSS_B_NONE, CSS_B_SOLID, CSS_B_DOTTED, CSS_B_DASHED, CSS_B_DOUBLE, CSS_B_GROOVE, CSS_B_RIDGE,
       CSS_B_INSET, CSS_B_OUTSET };

typedef struct {
    /* 継承するもの */
    DWORD color;
    short fontPx, remPx;                /* 文字の大きさ / 根要素の文字の大きさ (rem 用) */
    BYTE  bold, italic, under, strike, family, align, pre, listNone, invisible;
    BYTE  over;                         /* text-decoration: overline */
    BYTE  decoSet;                      /* 線を自分で引く (色・線種・太さ・位置の指定がある。受け継ぐ) */
    BYTE  decoHasCol;                   /* text-decoration-color の指定がある (無ければ文字の色) */
    DWORD decoColor;                    /* その色 */
    BYTE  decoStyle;                    /* text-decoration-style (CSS_DS_*) */
    BYTE  decoThick;                    /* text-decoration-thickness (px。0 = 自動) */
    BYTE  ulOffSet;                     /* text-underline-offset の指定がある */
    short ulOff;                        /* その値 (px。基準線から下へ) */
    BYTE  transform;                    /* text-transform: 0 なし / 1 大文字 / 2 小文字 / 3 単語の頭を大文字 */
    BYTE  nowrap;                       /* white-space: nowrap (折り返さない) */
    BYTE  smallCaps;                    /* font-variant: small-caps */
    BYTE  listType, listInside;         /* list-style-type (CSS_L_*) / list-style-position: inside */
    short listImg;                      /* list-style-image: 画像の番号 + 1 (0 = なし。CssSetUrlHook が返すもの) */
    short indent;                       /* text-indent (px か %) */
    BYTE  indentPct;
    BYTE  lineMul;                      /* 1 なら lineH は倍率 x100 (1.5 → 150)。0 なら px (0 = normal) */
    short lineH;                        /* line-height */
    short letterSp, wordSp;             /* letter-spacing / word-spacing (px) */
    char  face[32];                     /* font-family のうち、使える具体的な書体名 (無ければ "") */
    char  face2[32];                    /* face が英字だけの書体のとき、その後の、表示の文字コードの文字を出せる書体名 */
    short spacing[2];                   /* border-spacing の横・縦 (px。-1 = 指定なし) */
    BYTE  collapse;                     /* border-collapse: collapse */
    const char *quotes;                 /* quotes (CssIntern の文字列。NULL = 既定) */
    BYTE  cursor;                       /* cursor (CSS_C_*) */
    BYTE  noHit;                        /* pointer-events: none (マウスの当たり判定から外す) */
    BYTE  alignBlk;                     /* 中のブロックも揃える (1 中央 / 2 右。center 要素・align 属性・-webkit-center) */
    BYTE  preLine;                      /* white-space: pre-line (改行だけ残し、空白は詰めて折り返す) */
    BYTE  emptyHide;                    /* empty-cells: hide */
    BYTE  captionBottom;                /* caption-side: bottom */
    BYTE  colorClear;                   /* color: transparent (文字を描かない) */
    BYTE  colorT;                       /* 文字の色の透明度 (255 - アルファ。0 = 不透明。rgba() など) */
    BYTE  breakAll;                     /* word-break: break-all (語の途中でも行の残りを埋めて折り返す) */
    const struct CssVarNode *vars;      /* この要素と祖先で定義したカスタムプロパティ (近いものが先。CssPoolReset まで有効) */
    BYTE  tabSize;                      /* tab-size (空白の数。0 = 既定の 8、255 = 0) */
    BYTE  shadowOn;                     /* text-shadow がある */
    short shadowX, shadowY;             /* text-shadow のずらす量 (px) */
    DWORD shadowColor;                  /* text-shadow の色 (CSS_NOCOLOR = 文字の色) */
    /* 継承しないもの */
    BYTE  display, floatSide, clear, borderBox;
    BYTE  posAbs, offscreen, clipped;   /* 画面外に出して隠す手法の判定用 */
    BYTE  widthPct, maxWidthPct;        /* 1 なら width / maxWidth は % */
    BYTE  bstyle[4];                    /* 枠線の線種 (CSS_B_*。0 = なし) */
    BYTE  valign;                       /* vertical-align (CSS_V_*) */
    short vshift;                       /* CSS_V_LEN のときに上げる量 (px) */
    short bgImg;                        /* background-image: 画像の番号 + 1 (0 = なし) */
    BYTE  bgRepeat, bgFixed;            /* 0 repeat / 1 repeat-x / 2 repeat-y / 3 no-repeat、attachment: fixed */
    short bgPos[2];                     /* background-position の横・縦 (px か、% を 100 倍したもの) */
    BYTE  bgPosPct[2];
    DWORD bg;
    short margin[4], padding[4];        /* px。margin は CSS_AUTO もある */
    short bwidth[4];                    /* 枠線の太さ px (CSS_UNSET = medium) */
    DWORD bcolor[4];                    /* 枠線の色 (CSS_NOCOLOR = 文字色) */
    BYTE  wAuto, hAuto;                 /* width / height に auto を明示した (<img> の width / height 属性より優先する) */
    short width, maxWidth, height;      /* px か % (CSS_UNSET = 指定なし。% は 100 倍) */
    BYTE  pos;                          /* position (CSS_P_*) */
    BYTE  overflow;                     /* overflow (CSS_O_*。overflow-x / -y のどちらでも) */
    BYTE  overflowX;                    /* overflow-x (横のスクロールに使う) */
    short off[4];                       /* top / right / bottom / left (px か %。CSS_AUTO = auto) */
    BYTE  offPct[4];
    short zIndex;                       /* z-index (CSS_AUTO = auto) */
    short minWidth, minHeight, maxHeight;   /* px か % (CSS_UNSET = 指定なし。% は 100 倍) */
    BYTE  minWidthPct, minHeightPct, maxHeightPct, heightPct;
    const char *content;                /* ::before / ::after の content (CssIntern の文字列。NULL = なし) */
    const char *counterReset, *counterInc;
    short marginPct[4], padPct[4];      /* margin / padding の % (100 倍。0 = % ではない。配置のときに幅から求める) */
    BYTE  outlineStyle;                 /* outline の線種 (CSS_B_*。0 = なし) */
    short outlineW;                     /* outline の太さ (0 = medium、-1 = 0px) */
    short outlineOff;                   /* outline-offset (px) */
    DWORD outlineColor;                 /* outline の色 (CSS_NOCOLOR = 文字色) */
    BYTE  clipSet;                      /* clip: rect() がある */
    short clipR[4];                     /* その上・右・下・左 (枠の箱の左上から。CSS_AUTO = 端) */
    BYTE  visCollapse;                  /* visibility: collapse (表の行は場所も取らない) */
    BYTE  tableFixed;                   /* table-layout: fixed */
    BYTE  flex;                         /* display: flex / inline-flex (display は block / inline-block のまま) */
    BYTE  flexDir, flexWrap;            /* flex-direction (CSS_FD_*) / flex-wrap (0 nowrap / 1 wrap / 2 wrap-reverse) */
    BYTE  justify;                      /* justify-content (CSS_AL_*) */
    BYTE  alignItems, alignSelf, alignContent;  /* align-items / align-self / align-content (CSS_AL_*) */
    short grow, shrink;                 /* flex-grow / flex-shrink (100 倍) */
    short basis;                        /* flex-basis (px か、% の 100 倍。CSS_AUTO = auto) */
    BYTE  basisPct;
    short order;                        /* order */
    short gapRow, gapCol;               /* row-gap / column-gap (px か、Pct なら %) */
    BYTE  gapRowPct, gapColPct;         /* gap が % (行の間は箱の高さ、列の間は箱の幅に対して) */
    BYTE  bgT, bcT[4];                  /* 背景色・枠線の色の透明度 (255 - アルファ。0 = 不透明) */
    BYTE  opacityT;                     /* opacity の透明度 (255 - 不透明度。0 = 不透明) */
    short addW, addMinW, addMaxW, addH, addMinH, addMaxH;  /* calc(% ± px) の px の部分 (% の指定のときだけ使う) */
    short offAdd[4];                    /* top / right / bottom / left の calc(% ± px) の px の部分 */
    short radius[4];                    /* border-radius の左上・右上・右下・左下 (px か %。0 = 丸めない) */
    BYTE  radiusPct[4];
    BYTE  bgSizeMode;                   /* background-size: 0 auto / 1 cover / 2 contain / 3 bgSize の大きさ */
    short bgSize[2];                    /* その横・縦 (px か、% の 100 倍。CSS_AUTO = auto) */
    BYTE  bgSizePct[2];
    const char *bgGrad;                 /* 背景の linear-gradient() (CssIntern の文字列。NULL = なし) */
    const char *bgList;                 /* 重ねた背景の 2 つ目からの画像の層 (CssBgLayers で読む。NULL = なし) */
    BYTE  bgClip;                       /* background-clip (0 = border-box、1 = padding-box、2 = content-box、3 = text) */
    BYTE  bgOrigin;                     /* background-origin (0 = padding-box、1 = border-box、2 = content-box) */
    BYTE  contents;                     /* display: contents (箱を作らない。display は inline にして箱の書式を外す) */
    short biImg;                        /* border-image の画像 (画像の番号 + 1。0 = なし) */
    short biSl[4];                      /* border-image-slice (上・右・下・左。画像の px か %。すべて 0 なら 100%) */
    BYTE  biSlP;                        /* その % の辺 (ビット 1 << CSS_TOP など) */
    BYTE  biFill;                       /* fill (中央も描く) */
    BYTE  biRep[2];                     /* border-image-repeat の横・縦 (0 = stretch / 1 = repeat / 2 = round / 3 = space) */
    BYTE  bgFromGrad;                   /* bg はグラデーションの最初の色 (グラデーションを描くときは塗らない) */
    BYTE  boxShOn;                      /* box-shadow がある (最初の影だけ。inset は扱わない) */
    short boxShX, boxShY, boxShBlur, boxShSpread;
    DWORD boxShColor;                   /* 影の色 (CSS_NOCOLOR = 文字の色) */
    BYTE  boxShT;                       /* 影の色の透明度 (255 - アルファ) */
    const char *boxShList;              /* 影が 2 つ以上か inset の影があるときの、すべての影 (CssShadowList で読む) */
    BYTE  textEllipsis;                 /* text-overflow: ellipsis */
    BYTE  lineClamp;                    /* -webkit-line-clamp / line-clamp の行数 (0 = なし) */
    BYTE  colCount;                     /* column-count (0 = auto) */
    short colWidth;                     /* column-width (px。0 = auto) */
    BYTE  colRuleStyle, colRuleW;       /* column-rule の線を引くか / その太さ (px) */
    BYTE  colRuleHasCol;                /* column-rule-color の指定がある (無ければ文字の色) */
    DWORD colRuleColor;
    BYTE  hasFilt;                      /* filter がある (画像・アイコンにだけ効く) */
    short fGray, fSepia, fInvert;       /* grayscale / sepia / invert の量 (1000 倍。0 = なし) */
    short fSat, fBright, fContrast, fOpac;  /* saturate / brightness / contrast / opacity (1000 倍。1000 = そのまま) */
    short fHue;                         /* hue-rotate (度) */
    BYTE  fDs, fDsA;                    /* drop-shadow がある / その色の不透明度 */
    short fDsX, fDsY;                   /* drop-shadow のずらす量 */
    DWORD fDsColor;
    BYTE  sticky;                       /* position: sticky (pos は CSS_P_RELATIVE) */
    BYTE  grid;                         /* display: grid / inline-grid (display は block / inline-block のまま) */
    const char *gridCols, *gridRows;    /* grid-template-columns / rows (長さを px にした写し。CssIntern。NULL = なし) */
    const char *gridAreas;              /* grid-template-areas ("a a b/c c d" の形。CssIntern。NULL = なし) */
    const char *gridAutoCols;           /* grid-auto-columns (gridCols と同じ形。NULL = auto) */
    BYTE  gridFlow;                     /* grid-auto-flow (1 = column、2 = dense) */
    short gridAutoRow;                  /* grid-auto-rows (px。0 = auto) */
    short gcStart, gcEnd, gcSpan;       /* grid-column の始まり・終わりの線 (1 から。負は後ろから。0 = auto) と span */
    short grStart, grEnd, grSpan;       /* grid-row の同じもの */
    const char *gcName[2], *grName[2];  /* 名前で書いた始まり・終わりの線 (CssIntern。NULL = 番号か auto) */
    BYTE  justItems, justSelf;          /* justify-items / justify-self (CSS_AL_*) */
    BYTE  hasTr;                        /* transform の translate がある */
    short trX, trY;                     /* その横・縦 (px) */
    short trXP, trYP;                   /* その % の部分 (自分の枠の箱の幅・高さに対して。100 倍) */
    BYTE  hasXf;                        /* transform の回転・拡大がある (画像・アイコンと 90° 単位の文字にだけ効く) */
    short xfRot;                        /* 回転 (度、時計回り、0～359) */
    short xfSX, xfSY;                   /* 拡大 (1000 倍。負は裏返し) */
    BYTE  xfOSet;                       /* transform-origin の指定がある (無ければ中央) */
    short xfOX, xfOY;                   /* transform-origin (px か、% を 100 倍したもの) */
    BYTE  xfOXP, xfOYP;
    const char *animName;               /* animation-name (CssIntern。NULL = なし) */
    BYTE  animFwd, animInf;             /* animation-fill-mode: forwards / both、animation-iteration-count: infinite */
    BYTE  containerType;                /* container-type が normal 以外 (@container の基準になる) */
    const char *containerName;          /* container-name (CssIntern。NULL = なし) */
    short arW, arH;                     /* aspect-ratio の横と縦 (100 倍。0 = 指定なし) */
    short cqW;                          /* 一番近い container の内容の幅 (px。cqw の単位の基準。0 = 分からない。受け継ぐ) */
    BYTE  objFit;                       /* object-fit (CSS_OF_*) */
    BYTE  objPosSet;                    /* object-position の指定がある (無ければ中央) */
    short objPos[2];                    /* object-position の横・縦 (px か、% を 100 倍したもの) */
    BYTE  objPosPct[2];
    BYTE  maskOn;                       /* mask-image に url() がある (背景は画像の形に塗る。読めなければ塗らない) */
    short maskImg;                      /* その画像 (画像の番号 + 1。0 = 読まない・読めない) */
    BYTE  maskSizeMode;                 /* mask-size (bgSizeMode と同じ形) */
    short maskSize[2];
    BYTE  maskSizePct[2];
    short maskPos[2];                   /* mask-position (bgPos と同じ形。枠の箱に対して) */
    BYTE  maskPosPct[2];
} CssStyle;

typedef struct {                        /* グリッドの列・行の 1 本 */
    BYTE kind;                          /* 0 = 長さ (px) / 1 = fr / 2 = auto (中身の幅) */
    int  px;                            /* kind 0 の幅 */
    int  fr;                            /* kind 1 の fr (100 倍) */
    int  minPx;                         /* minmax() の下限 (px。0 = なし) */
    BYTE mm;                            /* minmax(下限, 長さ): 幅が足りなければ下限まで縮める */
} CssTrack;

typedef struct {                        /* box-shadow の影 1 つ */
    BYTE  inset;
    short x, y, blur, spread;
    DWORD color;                        /* CSS_NOCOLOR = 文字の色 */
    BYTE  alpha;                        /* 不透明度 (0～255) */
} CssShadow;

typedef struct {                        /* 重ねた背景の画像の層 1 つ */
    int   img;                          /* 画像 (番号 + 1) */
    BYTE  useMain;                      /* 位置・大きさ・繰り返しは主の層のもの (background-image で並べたもの) */
    BYTE  rep;                          /* 繰り返し (CssStyle.bgRepeat と同じ) */
    short pos[2];
    BYTE  posPct[2];
    BYTE  sizeMode;
    short size[2];
    BYTE  sizePct[2];
} CssBgLayer;

#define CSS_GRAD_STOPS  8
typedef struct {                        /* linear-gradient() を読んだもの */
    int   angle;                        /* 向き (度。0 = 上へ、90 = 右へ、180 = 下へ) */
    int   n;                            /* 色の数 */
    DWORD color[CSS_GRAD_STOPS];        /* 色 (CSS_NOCOLOR = 文字の色) */
    BYTE  alpha[CSS_GRAD_STOPS];        /* 不透明度 (0～255) */
    short pos[CSS_GRAD_STOPS];          /* 位置 (0～1000) */
    BYTE  radial, circle;               /* radial-gradient / その形が円 */
    BYTE  rep;                          /* repeating-linear-gradient / repeating-radial-gradient */
    BYTE  unitPx;                       /* pos は px (描くときに長さで 1000 分率にする) */
    BYTE  sizeKw;                       /* 大きさ: 0 farthest-corner / 1 closest-side / 2 closest-corner / 3 farthest-side / 4 長さ */
    short cx, cy;                       /* 中心 (箱の幅・高さの 1000 分率) */
    short rx, ry;                       /* 長さの半径 (px か、Pct なら 1000 分率) */
    BYTE  rxPct, ryPct;
} CssGrad;

typedef struct {                        /* セレクタ照合に使う要素の情報 */
    const char *tag;                    /* 小文字 */
    const char *id;
    const char *cls;                    /* 空白区切りのクラス名 */
    const char *(*attr)(const void *ctx, const char *name);
    const void *ctx;
    const void *node;                   /* 木の要素 (CssDom に渡す。NULL = 兄弟・状態が分からない) */
    BYTE        isLink, isRoot;                 /* isLink: 1 = リンク、2 = 訪れたリンク */
    const CssStyle *st;                 /* 計算済みの書式 (祖先のとき。@container の判定に使う。NULL = 分からない) */
} CssElem;

enum { CSS_Q_EMPTY, CSS_Q_CHECKED, CSS_Q_DISABLED, CSS_Q_FOCUS, CSS_Q_FOCUS_WITHIN, CSS_Q_ACTIVE,
       CSS_Q_HOVER, CSS_Q_TARGET, CSS_Q_ORPHAN,     /* ORPHAN: 親の無い要素 */
       CSS_Q_REQUIRED, CSS_Q_READWRITE, CSS_Q_PHSHOWN,      /* REQUIRED: 1 必須 / 0 任意 / -1 部品でない */
       CSS_Q_VALID };                                       /* 1 問題なし / 0 問題あり / -1 検査の対象でない */
/* CssUsesState のビット: 使っている状態の疑似クラス (状態が変わったら表示し直す) */
#define CSS_U_ACTIVE    1
#define CSS_U_HOVER     2
#define CSS_U_FOCUS     4               /* :focus / :focus-within */
#define CSS_U_TARGET    8
#define CSS_U_VALID     16              /* :valid / :invalid / :placeholder-shown (欄を離れたら作り直す) */

typedef struct {                        /* 兄弟要素をたどる・状態を調べる (+ ~ と構造・状態の疑似クラス用) */
    const void *(*prev)(const void *node);         /* 前の要素 (無ければ NULL) */
    const void *(*next)(const void *node);         /* 後の要素 (無ければ NULL) */
    void        (*fill)(const void *node, CssElem *e);  /* 照合に使う情報 */
    int         (*state)(const void *node, int q);      /* CSS_Q_* が当てはまれば 1 (DISABLED は部品でなければ -1) */
    const char *(*lang)(const void *node);              /* 要素か祖先の lang 属性 (無ければ NULL。:lang() 用) */
    const void *(*first)(const void *node);             /* 最初の子の要素 (無ければ NULL。:has() 用) */
} CssDom;

typedef struct {                        /* @media と長さの計算に使う表示環境 */
    int viewW, viewH, basePx;
    int strict;                         /* 標準モード (単位の無い長さなどを無効にする) */
    int keepAll;                        /* 表示に使わない性質も残す (CssCascaded で読む) */
} CssMedia;

typedef struct CssSheet CssSheet;

/* メモリ確保 (呼び出し側が用意する。失敗時は戻らないこと) */
void *CssXmalloc(size_t n);
void *CssXrealloc(void *p, size_t n);

/* 木をたどる関数を登録する (登録するまで + ~ と構造・状態の疑似クラスは合わない) */
void      CssSetDom(const CssDom *d);
/* url(...) の画像を登録する関数 (画像の番号 + 1 を返す。0 = 使えない) と、
   書体名がこの PC にあって使えるかを答える関数を登録する (無ければ画像・書体名は使わない)。
   書体の関数は 1 = 使える / 2 = あるが表示の文字コードの文字が無い (英字だけに使う) / 0 = 無い を返す */
void      CssSetHooks(int (*url)(const char *url), int (*font)(const char *face));
/* @font-face (書体名と src の url()) を知らせる関数を登録する */
void      CssSetFontFaceHook(void (*face)(const char *family, const char *url));
/* content などの文字列を保持する (同じものは共有)。CssPoolReset まで有効 */
const char *CssIntern(const char *s);
void      CssPoolReset(void);
/* mask-image の画像を読む (1) / 読まない (0。マスクのある背景は塗らない) */
void      CssSetMasks(int on);
/* 要素ごとのカスタムプロパティの項目の、1 回の表示での上限を決める (0 なら :root などの値だけ) */
void      CssSetVarCap(int n);
/* 文字の拡大・縮小の倍率 (%)。pxToo が 0 なら px などの絶対の単位の文字には掛けない */
void      CssSetZoom(int pct, int pxToo);

CssSheet *CssNew(const CssMedia *m);
/* @media の条件 (picture の source の media など) に合うか */
int       CssMediaMatch(const CssSheet *s, const char *q);
void      CssFree(CssSheet *s);
void      CssParse(CssSheet *s, const char *text, long len);
int       CssRuleCount(const CssSheet *s);
/* :checked を使うルールがある (チェックを変えたら表示し直す) */
int       CssUsesChecked(const CssSheet *s);
/* 使っている状態の疑似クラス (CSS_U_* の組み合わせ。状態が変わったときだけ表示を作り直すため) */
int       CssUsesState(const CssSheet *s);
/* @media の条件か vw / vh の単位を使ったか (画面の大きさが変わったら当て直すため)。reset なら覚えたことを消す */
int       CssViewUsed(int reset);
/* 状態 u (CSS_U_*) の要素が変わったときに e の表示が変わりうるか
   (その疑似クラスを含む複合セレクタが、その疑似クラスを合うものとして e に合う) */
int       CssStateCould(const CssSheet *s, const CssElem *e, int u);
/* list-style-type の名前 (counter() の書き方) を CSS_L_* にする (分からなければ -1) */
int       CssListType(const char *name);

/* 親の値を継承し、継承しないものを初期値にする */
void      CssInherit(const CssStyle *parent, CssStyle *st);
/* シートに疑似要素 pe (Compound.pe の番号) のルールがあるか (無ければ要素ごとの照合を飛ばせる) */
int       CssUsesPe(const CssSheet *s, int pe);
/* chain[0] = 対象の要素, chain[1] = 親, ... に合うルールと style 属性 (inl) を st に適用する */
void      CssApply(const CssSheet *s, const CssElem *chain, int n, const char *inl,
                   const CssStyle *parent, CssStyle *st);
/* getComputedStyle 用: chain[0] の要素の性質 name の指定値 (ルールと style 属性 inl の中で勝つもの。
   var() は展開する。"--x" ならカスタムプロパティ) を out に入れる。指定が無ければ 0 */
int       CssCascaded(const CssSheet *s, const CssElem *chain, int n, const char *inl, const char *name,
                      char *out, int size);
/* 色の解析。1 = 色, 2 = 透明, 3 = currentColor, 0 = 不明 */
int       CssParseColor(const char *v, DWORD *out);
/* 同じく、アルファ (0～255) も返す */
int       CssParseColorA(const char *v, DWORD *out, int *alpha);
/* linear-gradient() の文字列 (CssStyle.bgGrad) を読む。読めなければ 0 */
int       CssGradient(const char *text, CssGrad *g);
/* CssStyle.boxShList を影の並びにする (書いた順。戻り値は数) */
int       CssShadowList(const char *list, CssShadow *out, int max);
/* CssStyle.bgList を画像の層の並びにする (上の層から。戻り値は数) */
int       CssBgLayers(const char *list, CssBgLayer *out, int max);
/* grid-template-columns などの写し (CssStyle.gridCols) を、幅 W (不明なら -1) と隙間 gap で列に展開する。
   repeat(auto-fill / auto-fit, ...) の数もここで決める。列の数を返す */
int       CssGridTracks(const char *spec, int W, int gap, CssTrack *t, int max);
/* その写しで name という名前の最初の線の番号 (1 から。無ければ 0) */
int       CssGridLine(const char *spec, int W, int gap, const char *name);
/* grid-template-areas の写し (CssStyle.gridAreas) で name の領域の行・列 (0 から。終わりは含まない) を返す。
   無ければ 0。name が NULL なら全体の行と列の数を *r1 / *c1 に入れる */
int       CssGridArea(const char *areas, const char *name, int *r0, int *c0, int *r1, int *c1);
/* 枠線の実際の太さ (線種 none なら 0) */
int       CssBorder(const CssStyle *st, int side);
/* ::first-letter / ::first-line / ::before / ::after のルール (pe = 1 / 2 / 3 / 4) を st に重ねる。当てたら 1 */
int       CssApplyPseudo(const CssSheet *s, const CssElem *chain, int n, int pe,
                         const CssStyle *parent, CssStyle *st);

/* querySelector 用: セレクタの一覧を読む (対応していないものがあれば NULL。CssFree で解放) */
CssSheet *CssSelCompile(const char *text);
/* chain[0] = 要素, chain[1] = 親, ... がセレクタの一覧のどれかに合うか */
int       CssSelMatch(const CssSheet *s, const CssElem *chain, int n);

#endif
