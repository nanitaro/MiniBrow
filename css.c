/*
 * css.c - MiniBrow のスタイルシート (CSS) の解析と適用
 *
 * ルールは右端の複合セレクタの id / 先頭のクラス / 要素名で索引を作り、
 * 要素ごとに候補だけを照合する (古い CPU で数千ルールの CSS を扱うため)。
 * 対応できないセレクタを含むルールは捨てる (誤って要素を隠すより安全なため)。
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "css.h"

#define HASH_N          2048
#define MAX_PARTS       12
#define MAX_CLS         16
#define MAX_MATCH       512
#define MAX_INLINE      64
#define VAR_DEPTH       4
#define NEST_DEPTH_MAX  16                      /* CSS の入れ子・@media などの深さの上限 (再帰でスタックがあふれないように) */
#define NEST_ONE_MAX    (1024UL * 1024UL)       /* 入れ子の規則 1 つを展開した大きさの上限 */
#define NEST_TOTAL_MAX  (4UL * 1024UL * 1024UL) /* 1 つのシートで入れ子を展開する合計の上限 (指数的に増えないように) */
#define VALBUF          1024

enum {
    P_NONE, P_COLOR, P_BACKGROUND, P_BGCOLOR, P_FONT, P_FONTSIZE, P_FONTWEIGHT, P_FONTSTYLE,
    P_FONTFAMILY, P_TEXTDECO, P_TEXTALIGN, P_WHITESPACE, P_DISPLAY, P_VISIBILITY, P_LISTSTYLE,
    P_MARGIN, P_MARGIN_T, P_MARGIN_R, P_MARGIN_B, P_MARGIN_L,
    P_PADDING, P_PADDING_T, P_PADDING_R, P_PADDING_B, P_PADDING_L,
    P_BORDER, P_BORDER_T, P_BORDER_R, P_BORDER_B, P_BORDER_L,
    P_BORDER_WIDTH, P_BORDER_STYLE, P_BORDER_COLOR,
    P_BW_T, P_BW_R, P_BW_B, P_BW_L, P_BS_T, P_BS_R, P_BS_B, P_BS_L, P_BC_T, P_BC_R, P_BC_B, P_BC_L,
    P_WIDTH, P_MAXWIDTH, P_HEIGHT, P_FLOAT, P_CLEAR, P_BOXSIZING, P_POSITION, P_LEFT, P_TOP,
    P_CLIP, P_CLIPPATH, P_OPACITY,
    P_TEXTTRANSFORM, P_TEXTINDENT, P_FONTVARIANT, P_LISTTYPE, P_LISTPOS, P_LISTIMG, P_LINEHEIGHT,
    P_VALIGN, P_LETTERSP, P_WORDSP, P_BGIMAGE, P_BGREPEAT, P_BGATTACH, P_BGPOS,
    P_RIGHT, P_BOTTOM, P_ZINDEX, P_OVERFLOW, P_MINWIDTH, P_MINHEIGHT, P_MAXHEIGHT, P_CONTENT,
    P_COUNTERRESET, P_COUNTERINC, P_QUOTES, P_BORDERSPACING, P_BORDERCOLLAPSE,
    P_CAPTIONSIDE, P_CURSOR, P_EMPTYCELLS, P_OUTLINE, P_OUTLINE_C, P_OUTLINE_O, P_OUTLINE_S, P_OUTLINE_W,
    P_TABLELAYOUT, P_OVERFLOW_X, P_OVERFLOW_Y, P_TEXTSHADOW,
    P_FLEX, P_FLEXBASIS, P_FLEXDIR, P_FLEXFLOW, P_FLEXGROW, P_FLEXSHRINK, P_FLEXWRAP,
    P_JUSTIFY, P_ALIGNITEMS, P_ALIGNSELF, P_ALIGNCONTENT, P_ORDER, P_GAP, P_ROWGAP, P_COLGAP,
    P_RADIUS, P_RADIUS_TL, P_RADIUS_TR, P_RADIUS_BR, P_RADIUS_BL, P_BGSIZE, P_BOXSHADOW,
    P_WORDBREAK, P_LINEBREAK, P_TABSIZE, P_TEXTOVERFLOW,
    P_GRIDCOLS, P_GRIDROWS, P_GRIDAUTOROWS, P_GRIDCOL, P_GRIDROW, P_GRIDCOLSTART, P_GRIDCOLEND,
    P_GRIDROWSTART, P_GRIDROWEND, P_GRIDAREA, P_JUSTITEMS, P_JUSTSELF, P_PLACEITEMS, P_PLACESELF,
    P_GRID, P_GRIDTEMPLATE, P_GRIDAREAS, P_GRIDFLOW, P_GRIDAUTOCOLS,
    P_INSET, P_INSET_BLOCK, P_INSET_INLINE, P_MARGIN_BLOCK, P_MARGIN_INLINE, P_PAD_BLOCK, P_PAD_INLINE,
    P_TRANSFORM, P_TRANSLATE, P_ANIMATION, P_ANIMNAME, P_ANIMFILL, P_ANIMITER,
    P_CONTAINER, P_CONTNAME, P_CONTTYPE, P_ASPECT, P_OBJFIT, P_OBJPOS,
    P_MASK, P_MASKIMAGE, P_MASKPOS, P_MASKSIZE, P_ROTATE, P_SCALE, P_TRORIGIN,
    P_LINECLAMP, P_DECOCOLOR, P_DECOSTYLE, P_DECOTHICK, P_ULOFFSET,
    P_COLCOUNT, P_COLWIDTH, P_COLUMNS, P_COLRULE, P_COLRULECOLOR, P_COLRULESTYLE, P_COLRULEWIDTH, P_FILTER,
    P_BGCLIP, P_BGORIGIN, P_TEXTFILL, P_BORDERIMAGE, P_BISOURCE, P_BISLICE, P_BIREPEAT,
    P_POINTEREVENTS, P_ALL, P_PLACECONTENT,
    P_VAR,                      /* 普通のルールのカスタムプロパティ (要素ごとに並びへ足す。name に名前) */
    P_OTHER                     /* 表示には使わない性質 (CssMedia.keepAll のときだけ、getComputedStyle 用に残す) */
};

typedef struct {
    const char *name;
    short       id;
} PropName;

/* 名前順 (二分探索するため) */
static const PropName g_props[] = {
    { "-moz-border-radius", P_RADIUS }, { "-moz-tab-size", P_TABSIZE }, { "-ms-transform", P_TRANSFORM },
    { "-webkit-animation", P_ANIMATION }, { "-webkit-background-clip", P_BGCLIP }, { "-webkit-background-size", P_BGSIZE },
    { "-webkit-border-image", P_BORDERIMAGE },
    { "-webkit-border-radius", P_RADIUS }, { "-webkit-box-shadow", P_BOXSHADOW }, { "-webkit-filter", P_FILTER },
    { "-webkit-line-clamp", P_LINECLAMP },
    { "-webkit-mask", P_MASK }, { "-webkit-mask-image", P_MASKIMAGE }, { "-webkit-mask-position", P_MASKPOS },
    { "-webkit-mask-size", P_MASKSIZE }, { "-webkit-text-fill-color", P_TEXTFILL }, { "-webkit-transform", P_TRANSFORM },
    { "-webkit-transform-origin", P_TRORIGIN },
    { "align-content", P_ALIGNCONTENT }, { "align-items", P_ALIGNITEMS }, { "align-self", P_ALIGNSELF },
    { "all", P_ALL }, { "animation", P_ANIMATION }, { "animation-fill-mode", P_ANIMFILL }, { "animation-iteration-count", P_ANIMITER },
    { "animation-name", P_ANIMNAME }, { "aspect-ratio", P_ASPECT },
    { "background", P_BACKGROUND }, { "background-attachment", P_BGATTACH }, { "background-clip", P_BGCLIP },
    { "background-color", P_BGCOLOR },
    { "background-image", P_BGIMAGE }, { "background-origin", P_BGORIGIN }, { "background-position", P_BGPOS }, { "background-repeat", P_BGREPEAT },
    { "background-size", P_BGSIZE }, { "block-size", P_HEIGHT },
    { "border", P_BORDER }, { "border-block-end", P_BORDER_B }, { "border-block-start", P_BORDER_T },
    { "border-bottom", P_BORDER_B }, { "border-bottom-color", P_BC_B },
    { "border-bottom-left-radius", P_RADIUS_BL }, { "border-bottom-right-radius", P_RADIUS_BR },
    { "border-bottom-style", P_BS_B }, { "border-bottom-width", P_BW_B },
    { "border-collapse", P_BORDERCOLLAPSE },
    { "border-color", P_BORDER_COLOR }, { "border-image", P_BORDERIMAGE }, { "border-image-repeat", P_BIREPEAT },
    { "border-image-slice", P_BISLICE }, { "border-image-source", P_BISOURCE },
    { "border-inline-end", P_BORDER_R }, { "border-inline-start", P_BORDER_L },
    { "border-left", P_BORDER_L }, { "border-left-color", P_BC_L },
    { "border-left-style", P_BS_L }, { "border-left-width", P_BW_L }, { "border-radius", P_RADIUS },
    { "border-right", P_BORDER_R }, { "border-right-color", P_BC_R }, { "border-right-style", P_BS_R },
    { "border-right-width", P_BW_R }, { "border-spacing", P_BORDERSPACING },
    { "border-style", P_BORDER_STYLE }, { "border-top", P_BORDER_T },
    { "border-top-color", P_BC_T }, { "border-top-left-radius", P_RADIUS_TL },
    { "border-top-right-radius", P_RADIUS_TR }, { "border-top-style", P_BS_T }, { "border-top-width", P_BW_T },
    { "border-width", P_BORDER_WIDTH }, { "bottom", P_BOTTOM }, { "box-shadow", P_BOXSHADOW },
    { "box-sizing", P_BOXSIZING },
    { "caption-side", P_CAPTIONSIDE }, { "clear", P_CLEAR },
    { "clip", P_CLIP }, { "clip-path", P_CLIPPATH }, { "color", P_COLOR }, { "column-count", P_COLCOUNT },
    { "column-gap", P_COLGAP }, { "column-rule", P_COLRULE }, { "column-rule-color", P_COLRULECOLOR },
    { "column-rule-style", P_COLRULESTYLE }, { "column-rule-width", P_COLRULEWIDTH }, { "column-width", P_COLWIDTH },
    { "columns", P_COLUMNS },
    { "container", P_CONTAINER }, { "container-name", P_CONTNAME }, { "container-type", P_CONTTYPE },
    { "content", P_CONTENT },
    { "counter-increment", P_COUNTERINC }, { "counter-reset", P_COUNTERRESET }, { "cursor", P_CURSOR },
    { "display", P_DISPLAY }, { "empty-cells", P_EMPTYCELLS }, { "filter", P_FILTER },
    { "flex", P_FLEX }, { "flex-basis", P_FLEXBASIS }, { "flex-direction", P_FLEXDIR }, { "flex-flow", P_FLEXFLOW },
    { "flex-grow", P_FLEXGROW }, { "flex-shrink", P_FLEXSHRINK }, { "flex-wrap", P_FLEXWRAP },
    { "float", P_FLOAT }, { "font", P_FONT }, { "font-family", P_FONTFAMILY },
    { "font-size", P_FONTSIZE }, { "font-style", P_FONTSTYLE }, { "font-variant", P_FONTVARIANT },
    { "font-weight", P_FONTWEIGHT }, { "gap", P_GAP }, { "grid", P_GRID }, { "grid-area", P_GRIDAREA },
    { "grid-auto-columns", P_GRIDAUTOCOLS }, { "grid-auto-flow", P_GRIDFLOW }, { "grid-auto-rows", P_GRIDAUTOROWS },
    { "grid-column", P_GRIDCOL }, { "grid-column-end", P_GRIDCOLEND }, { "grid-column-gap", P_COLGAP },
    { "grid-column-start", P_GRIDCOLSTART }, { "grid-gap", P_GAP }, { "grid-row", P_GRIDROW },
    { "grid-row-end", P_GRIDROWEND }, { "grid-row-gap", P_ROWGAP }, { "grid-row-start", P_GRIDROWSTART },
    { "grid-template", P_GRIDTEMPLATE }, { "grid-template-areas", P_GRIDAREAS },
    { "grid-template-columns", P_GRIDCOLS }, { "grid-template-rows", P_GRIDROWS },
    { "height", P_HEIGHT }, { "inline-size", P_WIDTH }, { "inset", P_INSET }, { "inset-block", P_INSET_BLOCK },
    { "inset-block-end", P_BOTTOM }, { "inset-block-start", P_TOP }, { "inset-inline", P_INSET_INLINE },
    { "inset-inline-end", P_RIGHT }, { "inset-inline-start", P_LEFT },
    { "justify-content", P_JUSTIFY }, { "justify-items", P_JUSTITEMS },
    { "justify-self", P_JUSTSELF }, { "left", P_LEFT }, { "letter-spacing", P_LETTERSP },
    { "line-break", P_LINEBREAK }, { "line-clamp", P_LINECLAMP }, { "line-height", P_LINEHEIGHT },
    { "list-style", P_LISTSTYLE }, { "list-style-image", P_LISTIMG }, { "list-style-position", P_LISTPOS },
    { "list-style-type", P_LISTTYPE }, { "margin", P_MARGIN }, { "margin-block", P_MARGIN_BLOCK },
    { "margin-block-end", P_MARGIN_B }, { "margin-block-start", P_MARGIN_T }, { "margin-bottom", P_MARGIN_B },
    { "margin-inline", P_MARGIN_INLINE }, { "margin-inline-end", P_MARGIN_R }, { "margin-inline-start", P_MARGIN_L },
    { "margin-left", P_MARGIN_L }, { "margin-right", P_MARGIN_R }, { "margin-top", P_MARGIN_T },
    { "mask", P_MASK }, { "mask-image", P_MASKIMAGE }, { "mask-position", P_MASKPOS }, { "mask-size", P_MASKSIZE },
    { "max-block-size", P_MAXHEIGHT }, { "max-height", P_MAXHEIGHT }, { "max-inline-size", P_MAXWIDTH },
    { "max-width", P_MAXWIDTH }, { "min-block-size", P_MINHEIGHT }, { "min-height", P_MINHEIGHT },
    { "min-inline-size", P_MINWIDTH }, { "min-width", P_MINWIDTH }, { "object-fit", P_OBJFIT },
    { "object-position", P_OBJPOS }, { "opacity", P_OPACITY }, { "order", P_ORDER },
    { "outline", P_OUTLINE }, { "outline-color", P_OUTLINE_C }, { "outline-offset", P_OUTLINE_O },
    { "outline-style", P_OUTLINE_S }, { "outline-width", P_OUTLINE_W },
    { "overflow", P_OVERFLOW }, { "overflow-x", P_OVERFLOW_X }, { "overflow-y", P_OVERFLOW_Y },
    { "padding", P_PADDING }, { "padding-block", P_PAD_BLOCK }, { "padding-block-end", P_PADDING_B },
    { "padding-block-start", P_PADDING_T }, { "padding-bottom", P_PADDING_B },
    { "padding-inline", P_PAD_INLINE }, { "padding-inline-end", P_PADDING_R }, { "padding-inline-start", P_PADDING_L },
    { "padding-left", P_PADDING_L }, { "padding-right", P_PADDING_R }, { "padding-top", P_PADDING_T },
    { "place-content", P_PLACECONTENT }, { "place-items", P_PLACEITEMS }, { "place-self", P_PLACESELF },
    { "pointer-events", P_POINTEREVENTS },
    { "position", P_POSITION }, { "quotes", P_QUOTES }, { "right", P_RIGHT }, { "rotate", P_ROTATE },
    { "row-gap", P_ROWGAP }, { "scale", P_SCALE },
    { "tab-size", P_TABSIZE },
    { "table-layout", P_TABLELAYOUT }, { "text-align", P_TEXTALIGN }, { "text-decoration", P_TEXTDECO },
    { "text-decoration-color", P_DECOCOLOR }, { "text-decoration-line", P_TEXTDECO },
    { "text-decoration-style", P_DECOSTYLE }, { "text-decoration-thickness", P_DECOTHICK },
    { "text-indent", P_TEXTINDENT }, { "text-overflow", P_TEXTOVERFLOW },
    { "text-shadow", P_TEXTSHADOW },
    { "text-transform", P_TEXTTRANSFORM }, { "text-underline-offset", P_ULOFFSET },
    { "top", P_TOP }, { "transform", P_TRANSFORM }, { "transform-origin", P_TRORIGIN }, { "translate", P_TRANSLATE }, { "vertical-align", P_VALIGN }, { "visibility", P_VISIBILITY },
    { "white-space", P_WHITESPACE }, { "width", P_WIDTH }, { "word-break", P_WORDBREAK }, { "word-spacing", P_WORDSP },
    { "z-index", P_ZINDEX }
};

static const struct { const char *name; DWORD rgb; } g_colors[] = {
    { "aliceblue", 0xF0F8FF }, { "aqua", 0x00FFFF }, { "azure", 0xF0FFFF }, { "beige", 0xF5F5DC },
    { "black", 0x000000 }, { "blue", 0x0000FF }, { "brown", 0xA52A2A }, { "chocolate", 0xD2691E },
    { "coral", 0xFF7F50 }, { "cornflowerblue", 0x6495ED }, { "crimson", 0xDC143C }, { "cyan", 0x00FFFF },
    { "darkblue", 0x00008B }, { "darkgray", 0xA9A9A9 }, { "darkgreen", 0x006400 },
    { "darkgrey", 0xA9A9A9 }, { "darkorange", 0xFF8C00 }, { "darkred", 0x8B0000 },
    { "darkslategray", 0x2F4F4F }, { "darkviolet", 0x9400D3 }, { "deeppink", 0xFF1493 },
    { "deepskyblue", 0x00BFFF }, { "dimgray", 0x696969 }, { "dimgrey", 0x696969 },
    { "dodgerblue", 0x1E90FF }, { "firebrick", 0xB22222 }, { "floralwhite", 0xFFFAF0 },
    { "forestgreen", 0x228B22 }, { "fuchsia", 0xFF00FF }, { "gainsboro", 0xDCDCDC },
    { "ghostwhite", 0xF8F8FF }, { "gold", 0xFFD700 }, { "goldenrod", 0xDAA520 }, { "gray", 0x808080 },
    { "green", 0x008000 }, { "grey", 0x808080 }, { "honeydew", 0xF0FFF0 }, { "hotpink", 0xFF69B4 },
    { "indigo", 0x4B0082 }, { "ivory", 0xFFFFF0 }, { "khaki", 0xF0E68C }, { "lavender", 0xE6E6FA },
    { "lightblue", 0xADD8E6 }, { "lightgray", 0xD3D3D3 }, { "lightgreen", 0x90EE90 },
    { "lightgrey", 0xD3D3D3 }, { "lightslategray", 0x778899 }, { "lightyellow", 0xFFFFE0 },
    { "lime", 0x00FF00 }, { "linen", 0xFAF0E6 }, { "magenta", 0xFF00FF }, { "maroon", 0x800000 },
    { "midnightblue", 0x191970 }, { "mintcream", 0xF5FFFA }, { "navy", 0x000080 },
    { "oldlace", 0xFDF5E6 }, { "olive", 0x808000 }, { "orange", 0xFFA500 }, { "orchid", 0xDA70D6 },
    { "pink", 0xFFC0CB }, { "plum", 0xDDA0DD }, { "purple", 0x800080 }, { "rebeccapurple", 0x663399 },
    { "red", 0xFF0000 }, { "royalblue", 0x4169E1 }, { "salmon", 0xFA8072 }, { "seagreen", 0x2E8B57 },
    { "seashell", 0xFFF5EE }, { "sienna", 0xA0522D }, { "silver", 0xC0C0C0 }, { "skyblue", 0x87CEEB },
    { "slategray", 0x708090 }, { "snow", 0xFFFAFA }, { "steelblue", 0x4682B4 }, { "tan", 0xD2B48C },
    { "teal", 0x008080 }, { "thistle", 0xD8BFD8 }, { "tomato", 0xFF6347 }, { "violet", 0xEE82EE },
    { "wheat", 0xF5DEB3 }, { "white", 0xFFFFFF }, { "whitesmoke", 0xF5F5F5 }, { "yellow", 0xFFFF00 }
};

typedef struct Blk {
    struct Blk *next;
    size_t      used, cap;
} Blk;

typedef struct {
    char *name, *val;
    BYTE  op, icase;            /* op: 0 = 属性があるだけ, '=' '~' '^' '$' '*' '|' */
} AttrCond;

enum {                          /* 構造・状態の疑似クラス */
    PS_NTH, PS_NTH_LAST, PS_NTH_TYPE, PS_NTH_LAST_TYPE, PS_ONLY, PS_ONLY_TYPE,
    PS_EMPTY, PS_CHECKED, PS_DISABLED, PS_ENABLED, PS_FOCUS, PS_FOCUS_WITHIN, PS_ACTIVE, PS_HOVER,
    PS_TARGET, PS_LANG, PS_REQUIRED, PS_OPTIONAL, PS_READONLY, PS_READWRITE, PS_PHSHOWN, PS_VALID, PS_INVALID,
    PS_IS, PS_NOT, PS_HAS       /* セレクタの並びを持つもの (:is() :where() :not() :has()) */
};

struct Rule;

typedef struct {
    BYTE  kind;
    short a, b;                 /* :nth-child(an+b) など */
    const char *str;            /* :lang() の言語 */
    const struct Rule *sel;     /* :is() などの中のセレクタ (:has() は先頭が基準の要素) */
    int   nSel;
} PseudoCond;

#define MAX_PSEUDO      8

typedef struct Compound {
    char            *tag;       /* NULL なら任意 */
    char            *id;
    char           **cls;
    int              nCls;
    AttrCond        *attrs;
    int              nAttrs;
    PseudoCond      *ps;
    int              nPs;
    BYTE             link, root;    /* link: 1 = :link、2 = :visited、3 = :any-link */
    BYTE             nested;    /* :is() などのセレクタの並びを含む (祖先の情報を渡して照合する) */
    BYTE             anchor;    /* :has() の基準の要素 (その要素だけに合う) */
    BYTE             comb;      /* 左の複合セレクタとの結合子: ' ' (子孫) / '>' (子) / '+' / '~'。先頭は 0 */
    BYTE             pe;        /* 疑似要素: 1 = ::first-letter / 2 = ::first-line / 3 = ::before / 4 = ::after / 5 = ::marker / 6 = ::placeholder (セレクタの最後だけ) */
} Compound;

typedef struct Rule {
    Compound *parts;            /* parts[n - 1] が対象の要素 */
    int       n, spec, d0, nd;
    int       cq;               /* @container の中のルール: CssSheet.cqs の番号 + 1 (0 = 条件なし) */
    int       layer;            /* @layer の中のルール: 層の順位 (CssSheet.layers の番号 + 1。0 = 層の外) */
} Rule;

typedef struct {
    short prop;
    BYTE  imp;
    char *val;
    char *name;                 /* P_OTHER の性質の名前 */
} Decl;

typedef struct Bucket {
    struct Bucket *next;
    char           kind;        /* '#' / '.' / 't' */
    char          *key;
    int           *ids;
    int            n, cap;
} Bucket;

typedef struct {
    char *name, *val;
} Var;

typedef struct {                /* @container の条件 */
    char *name;                 /* 基準の名前 (NULL = 一番近い基準) */
    char *cond;                 /* 条件 ("(min-width: 400px)" など) */
} CQuery;

typedef struct {                /* @keyframes の最後のコマ (to / 100%) の宣言 */
    char *name;
    int   d0, nd;               /* decls の範囲 */
} Keyframes;

struct CssSheet {
    CssMedia m;
    Blk     *blk;
    Rule    *rules;  int nRules, capRules;
    Decl    *decls;  int nDecls, capDecls;
    Bucket  *hash[HASH_N];
    int     *univ;   int nUniv, capUniv;
    Var     *vars;   int nVars, capVars;
    Keyframes *kfs;  int nKfs, capKfs;
    CQuery  *cqs;    int nCqs, capCqs;
    int      curCq;                     /* 読んでいる @container の番号 + 1 (その中のルールに付ける) */
    int      curLayer;                  /* 読んでいる @layer の順位 (その中のルールに付ける。0 = 層の外) */
    char   **layers; int nLayers, capLayers;    /* @layer の名前 (現れた順。名前の無い層は NULL) */
    BYTE     usesState;                 /* 使っている状態の疑似クラス (CSS_U_*) */
    BYTE     usesPe;                    /* 使っている疑似要素 (1 << Compound.pe) */
    BYTE     usesChecked;               /* :checked を使うルールがある */
    int      ruleDepth;                 /* 読んでいる規則の入れ子の深さ */
    size_t   nestUsed;                  /* 入れ子の展開に使った合計の大きさ */
};

static CssDom g_cssDom;                 /* 兄弟・状態を調べる関数 (CssSetDom) */
static int  (*g_urlHook)(const char *url);      /* url() の画像を登録する (CssSetHooks) */
static int    g_masks = 1;                      /* mask-image の画像を読む (CssSetMasks) */
static int  (*g_fontHook)(const char *face);    /* 書体名が使えるか */
static void (*g_faceHook)(const char *family, const char *url);    /* @font-face を知らせる */
/* 要素のカスタムプロパティの並び (CssStyle.vars)。項目は CssPoolReset まで持つ領域に置き、数に上限を設ける */
struct CssVarNode {
    const char              *name, *val;
    const struct CssVarNode *next;
};
static int    g_varCap = 20000;         /* 1 回の表示で作る項目の上限 (超えたら :root などの値だけで扱う。CssSetVarCap) */
#define VAR_BLK     16384
static char **g_varBlk;                 /* 確保した領域の並び */
static int    g_nVarBlk, g_capVarBlk, g_varUsed = VAR_BLK, g_varCount;
static const struct CssVarNode *g_curVars;  /* 適用している要素の並び (CssApply の間だけ) */
static int    g_viewUse;                /* @media の条件か vw / vh を使った (画面の大きさが変わったら当て直す) */
/* 適用している要素の祖先の並び (style 属性のカスタムプロパティを探す。CssApply などの間だけ) */
static const CssElem *g_varChain;
static int            g_varN;
static const char    *g_varInl;         /* その要素の style 属性 */
static int    g_wantPe;                 /* 照合する疑似要素 (0 = ふつうの要素) */
static int    g_forceState;             /* CssStateCould の照合中: この状態の疑似クラス (CSS_U_*) は合うものとする */

void CssSetMasks(int on)
{
    g_masks = on;
}

void CssSetHooks(int (*url)(const char *url), int (*font)(const char *face))
{
    g_urlHook  = url;
    g_fontHook = font;
}

void CssSetFontFaceHook(void (*face)(const char *family, const char *url))
{
    g_faceHook = face;
}

void CssSetDom(const CssDom *d)
{
    g_cssDom = *d;
}

/* ---------------------------------------------------------------- メモリ */

static void *Alloc(CssSheet *s, size_t n)
{
    Blk  *b = s->blk;
    char *p;

    n = (n + 7) & ~(size_t)7;
    if (b == NULL || b->used + n > b->cap) {
        size_t cap = n > 32768 ? n : 32768;
        b = CssXmalloc(sizeof(Blk) + 8 + cap);
        b->next = s->blk;
        b->used = 0;
        b->cap  = cap;
        s->blk  = b;
    }
    p = (char *)(b + 1) + 8 + b->used;
    b->used += n;
    return p;
}

static char *Dup(CssSheet *s, const char *a, const char *b)
{
    size_t n = (size_t)(b - a);
    char  *d = Alloc(s, n + 1);
    memcpy(d, a, n);
    d[n] = '\0';
    return d;
}

static void *Grow(void *p, int *cap, int need, int elem)
{
    if (need > *cap) {
        int nc = *cap ? *cap * 2 : 64;
        while (nc < need) {
            nc *= 2;
        }
        p = CssXrealloc(p, (size_t)nc * elem);
        *cap = nc;
    }
    return p;
}

/* ---------------------------------------------------------------- 字句 */

static const char *SkipWs(const char *p)
{
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

/* p は開き括弧を指す。対応する閉じ括弧の次を返す (文字列の中は無視) */
static const char *SkipBlock(const char *p)
{
    int  depth = 0;
    char q = 0;

    for (; *p; p++) {
        if (q) {
            if (*p == '\\' && p[1]) {
                p++;
            } else if (*p == q) {
                q = 0;
            }
            continue;
        }
        if (*p == '\\' && p[1]) {
            p++;                        /* \} などのエスケープした文字 */
        } else if (*p == '"' || *p == '\'') {
            q = *p;
        } else if (*p == '{' || *p == '(' || *p == '[') {
            depth++;
        } else if (*p == '}' || *p == ')' || *p == ']') {
            if (--depth <= 0) {
                return p + 1;
            }
        }
    }
    return p;
}

/* 括弧と文字列の外にある stops のどれかの文字まで進む (無ければ文字列の終わり) */
static const char *ScanTo(const char *p, const char *stops)
{
    int  depth = 0;
    char q = 0;

    for (; *p; p++) {
        if (q) {
            if (*p == '\\' && p[1]) {
                p++;
            } else if (*p == q) {
                q = 0;
            }
            continue;
        }
        if (*p == '\\' && p[1]) {
            p++;                        /* \} などのエスケープした文字 */
        } else if (*p == '"' || *p == '\'') {
            q = *p;
        } else if (*p == '(' || *p == '[') {
            depth++;
        } else if ((*p == ')' || *p == ']') && depth > 0) {
            depth--;
        } else if (depth == 0 && strchr(stops, *p) != NULL) {
            return p;
        }
    }
    return p;
}

static void Trim(const char **a, const char **b)
{
    while (*a < *b && isspace((unsigned char)**a)) (*a)++;
    while (*b > *a && isspace((unsigned char)(*b)[-1])) (*b)--;
}

static int HexVal(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* CSS の識別子 (エスケープ対応) を読む。長さを返す */
static int ParseIdent(const char **pp, const char *end, char *out, int max)
{
    const char *p = *pp;
    int         n = 0;

    while (p < end && *p) {
        unsigned char c = (unsigned char)*p;
        if (c == '\\' && p + 1 < end) {
            p++;
            if (HexVal((unsigned char)*p) >= 0) {
                unsigned long cp = 0;
                int           k  = 0;
                while (k < 6 && p < end && HexVal((unsigned char)*p) >= 0) {
                    cp = cp * 16 + HexVal((unsigned char)*p);
                    p++;
                    k++;
                }
                if (p < end && *p == ' ') {
                    p++;
                }
                if (cp < 0x80) {
                    c = (unsigned char)cp;
                } else if (cp == 0xA0 || cp == 0x2002 || cp == 0x2003 || cp == 0x2009) {
                    c = 0x1F;           /* ホストと同じく、詰めない空白の印 */
                } else {
                    /* 表示の文字コードにする (無い文字は ?) */
                    WCHAR w = (WCHAR)(cp > 0xFFFF ? 0xFFFD : cp);
                    char  mb[4];
                    int   k2 = WideCharToMultiByte(CP_ACP, 0, &w, 1, mb, sizeof(mb), NULL, NULL), j;
                    for (j = 0; j < k2 - 1 && n < max - 1; j++) out[n++] = mb[j];
                    c = (unsigned char)(k2 > 0 ? mb[k2 - 1] : '?');
                }
            } else {
                c = (unsigned char)*p++;
            }
            if (n < max - 1) out[n++] = (char)c;
            continue;
        }
        if (isalnum(c) || c == '-' || c == '_' || c >= 0x80) {
            if (n < max - 1) out[n++] = (char)c;
            p++;
            continue;
        }
        break;
    }
    out[n] = '\0';
    *pp = p;
    return n;
}

static void Lower(char *s)
{
    for (; *s; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

static int StrIStr(const char *s, const char *pat)
{
    size_t n = strlen(pat);
    for (; *s; s++) {
        if (strnicmp(s, pat, n) == 0) {
            return 1;
        }
    }
    return 0;
}

static int AllDigits(const char *s)
{
    if (*s == '\0') return 0;
    for (; *s; s++) {
        if (!isdigit((unsigned char)*s)) return 0;
    }
    return 1;
}

/* 値を空白区切りの語に分ける (括弧の中は分けない)。語の数を返す */
static int Tokens(const char *v, char tok[][128], int max)
{
    int n = 0;

    for (;;) {
        const char *e;
        int         len;
        v = SkipWs(v);
        if (*v == '\0' || n >= max) {
            break;
        }
        e = ScanTo(v, " \t\r\n");
        len = (int)(e - v);
        if (len > 127) len = 127;
        memcpy(tok[n], v, len);
        tok[n][len] = '\0';
        n++;
        v = e;
    }
    return n;
}

/* ---------------------------------------------------------------- 値の解析 */

typedef struct {
    int em, rem, vw, vh;
    int cq;                             /* 一番近い container の幅 (cqw の基準。0 = 分からない: 画面の幅) */
    int strict;                         /* 標準モード: 0 以外の単位の無い長さは無効 */
} LenCtx;

static double Num(const char **pp, int *ok)
{
    char  *e;
    double d = strtod(*pp, &e);
    *ok = e != *pp;
    *pp = e;
    return d;
}

/* calc() / min() / max() / clamp() を大まかに計算する (% を含むものは不可) */
static int ParseLen(const char *v, const LenCtx *c, int *px);

static double g_calcPct;                /* ParseLen が 4 を返したときの % の部分 */

typedef struct {
    double px, pct;                     /* 長さの px の部分と % の部分 (num なら px に数) */
    int    num;                         /* 単位の無い数 */
} CalcVal;

static int CalcExpr(const char **pp, const char *e, const LenCtx *c, CalcVal *out);

/* calc() の中の 1 つの値: 数・長さ・%・( 式 )・入れ子の calc() など */
static int CalcFactor(const char **pp, const char *e, const LenCtx *c, CalcVal *out)
{
    const char *p = SkipWs(*pp), *q;
    char        buf[128];
    int         n, px, k;

    memset(out, 0, sizeof(*out));
    if (p >= e) return 0;
    if (*p == '(') {
        p++;
        if (!CalcExpr(&p, e, c, out)) return 0;
        p = SkipWs(p);
        if (*p != ')') return 0;
        *pp = p + 1;
        return 1;
    }
    if (isalpha((unsigned char)*p)) {
        q = p;
        while (isalpha((unsigned char)*q) || *q == '-') q++;
        if (*q != '(') return 0;
        q = SkipBlock(q);
    } else {
        q = p;
        if (*q == '+' || *q == '-') q++;
        while (q < e && (isalnum((unsigned char)*q) || *q == '.' || *q == '%')) q++;
    }
    n = (int)(q - p);
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    memcpy(buf, p, n);
    buf[n] = '\0';
    *pp = q;
    {
        char  *end;
        double d = strtod(buf, &end);
        if (end != buf && *end == '\0') {
            out->px  = d;
            out->num = 1;
            return 1;
        }
    }
    k = ParseLen(buf, c, &px);
    if (k == 1) {
        out->px = px;
    } else if (k == 2) {
        out->pct = strtod(buf, NULL);
    } else if (k == 4) {
        out->px  = px;
        out->pct = g_calcPct;
    } else {
        return 0;
    }
    return 1;
}

/* 掛け算・割り算 (片方は単位の無い数) */
static int CalcTerm(const char **pp, const char *e, const LenCtx *c, CalcVal *out)
{
    CalcVal b;
    if (!CalcFactor(pp, e, c, out)) return 0;
    for (;;) {
        const char *p = SkipWs(*pp);
        char        op = *p;
        if (p >= e || (op != '*' && op != '/')) break;
        *pp = p + 1;
        if (!CalcFactor(pp, e, c, &b)) return 0;
        if (op == '/') {
            if (!b.num || b.px == 0.0) return 0;
            out->px  /= b.px;
            out->pct /= b.px;
        } else if (b.num) {
            out->px  *= b.px;
            out->pct *= b.px;
        } else if (out->num) {
            double f = out->px;
            out->px  = b.px * f;
            out->pct = b.pct * f;
            out->num = 0;
        } else {
            return 0;
        }
    }
    return 1;
}

/* 足し算・引き算 (+ と - の前後には空白が要る) */
static int CalcExpr(const char **pp, const char *e, const LenCtx *c, CalcVal *out)
{
    CalcVal b;
    if (!CalcTerm(pp, e, c, out)) return 0;
    for (;;) {
        const char *p = SkipWs(*pp);
        char        op = *p;
        if (p >= e || (op != '+' && op != '-') || !isspace((unsigned char)p[1])) break;
        *pp = p + 1;
        if (!CalcTerm(pp, e, c, &b)) return 0;
        if (op == '+') {
            out->px  += b.px;
            out->pct += b.pct;
        } else {
            out->px  -= b.px;
            out->pct -= b.pct;
        }
        out->num = out->num && b.num;
    }
    return 1;
}

static int ParseFunc(const char *v, const LenCtx *c, int *px)
{
    char        name[16], arg[128];
    const char *p = v, *e, *a;
    int         n = 0, vals[8], nv = 0, i, r;

    while (isalpha((unsigned char)*p) && n < 15) name[n++] = (char)tolower((unsigned char)*p++);
    name[n] = '\0';
    if (*p != '(') return 0;
    e = SkipBlock(p) - 1;               /* ')' */
    if (strcmp(name, "calc") == 0) {
        /* + - * / と括弧の式。% を含むものは 4 を返し、% の部分を g_calcPct に入れる */
        CalcVal r0;
        double  d;
        a = p + 1;
        if (!CalcExpr(&a, e, c, &r0) || SkipWs(a) != e) return 0;
        if (r0.num && r0.px != 0.0) return 0;
        d = r0.px > 30000.0 ? 30000.0 : r0.px < -30000.0 ? -30000.0 : r0.px;
        *px = (int)(d + (d < 0 ? -0.5 : 0.5));
        if (r0.pct != 0.0) {
            g_calcPct = r0.pct;
            return 4;
        }
        return 1;
    }
    if (strcmp(name, "min") && strcmp(name, "max") && strcmp(name, "clamp")) return 0;
    a = p + 1;
    while (a < e && nv < 8) {
        const char *t = ScanTo(a, ",)");
        const char *s1 = a, *s2 = t > e ? e : t;
        Trim(&s1, &s2);
        if (s2 - s1 <= 0 || s2 - s1 >= (int)sizeof(arg)) return 0;
        memcpy(arg, s1, s2 - s1);
        arg[s2 - s1] = '\0';
        if (ParseLen(arg, c, &vals[nv]) != 1) return 0;
        nv++;
        a = t + 1;
    }
    if (nv == 0) return 0;
    r = vals[0];
    if (name[0] == 'c') {
        if (nv != 3) return 0;
        r = vals[1] < vals[0] ? vals[0] : vals[1] > vals[2] ? vals[2] : vals[1];
    } else {
        for (i = 1; i < nv; i++) {
            if ((name[1] == 'i') ? vals[i] < r : vals[i] > r) r = vals[i];
        }
    }
    *px = r;
    return 1;
}

/* 長さ。戻り値 1 = px, 2 = % (px に百分率), 3 = auto, 4 = calc() の % と px (px に px の部分、g_calcPct に %), 0 = 不正 */
static int ParseLen(const char *v, const LenCtx *c, int *px)
{
    double d;
    int    ok;
    char   unit[8];
    int    n = 0;

    v = SkipWs(v);
    if (strnicmp(v, "auto", 4) == 0) return 3;
    if (isalpha((unsigned char)*v)) return ParseFunc(v, c, px);
    d = Num(&v, &ok);
    if (!ok) return 0;
    while (isalpha((unsigned char)*v) && n < 7) unit[n++] = (char)tolower((unsigned char)*v++);
    unit[n] = '\0';
    if (*v == '%') {
        *px = (int)(d + (d < 0 ? -0.5 : 0.5));
        return 2;
    }
    v = SkipWs(v);
    if (*v != '\0') return 0;
    if (n == 0 && d != 0.0 && c->strict) return 0;     /* "width: 200" は標準モードでは無効 */
    if (n == 0 || strcmp(unit, "px") == 0) ;
    else if (strcmp(unit, "pt") == 0) d = d * 96.0 / 72.0;
    else if (strcmp(unit, "pc") == 0) d = d * 16.0;
    else if (strcmp(unit, "em") == 0) d = d * c->em;
    else if (strcmp(unit, "rem") == 0) d = d * c->rem;
    else if (strcmp(unit, "ex") == 0 || strcmp(unit, "ch") == 0) d = d * c->em / 2.0;
    else if (strcmp(unit, "vw") == 0 || strcmp(unit, "dvw") == 0 || strcmp(unit, "svw") == 0 || strcmp(unit, "lvw") == 0
             || strcmp(unit, "vi") == 0) {
        d = d * c->vw / 100.0, g_viewUse = 1;
    } else if (strcmp(unit, "vh") == 0 || strcmp(unit, "dvh") == 0 || strcmp(unit, "svh") == 0 || strcmp(unit, "lvh") == 0
               || strcmp(unit, "vb") == 0) {
        d = d * c->vh / 100.0, g_viewUse = 1;
    } else if (strcmp(unit + (unit[0] == 'v' ? 0 : 1), "vmin") == 0) {
        d = d * (c->vw < c->vh ? c->vw : c->vh) / 100.0, g_viewUse = 1;      /* vmin・dvmin・svmin・lvmin */
    } else if (strcmp(unit + (unit[0] == 'v' ? 0 : 1), "vmax") == 0) {
        d = d * (c->vw > c->vh ? c->vw : c->vh) / 100.0, g_viewUse = 1;
    } else if (strcmp(unit, "cqw") == 0 || strcmp(unit, "cqi") == 0) {
        d = d * (c->cq > 0 ? c->cq : c->vw) / 100.0, g_viewUse = 1;         /* container の幅 (分からなければ画面) */
    } else if (strcmp(unit, "cqh") == 0 || strcmp(unit, "cqb") == 0) {
        d = d * c->vh / 100.0, g_viewUse = 1;
    } else if (strcmp(unit, "cqmin") == 0 || strcmp(unit, "cqmax") == 0) {
        int w = c->cq > 0 ? c->cq : c->vw;
        d = d * (unit[3] == 'i' ? (w < c->vh ? w : c->vh) : (w > c->vh ? w : c->vh)) / 100.0, g_viewUse = 1;
    }
    else if (strcmp(unit, "in") == 0) d = d * 96.0;
    else if (strcmp(unit, "cm") == 0) d = d * 96.0 / 2.54;
    else if (strcmp(unit, "mm") == 0) d = d * 96.0 / 25.4;
    else return 0;
    if (d > 30000.0) d = 30000.0;
    if (d < -30000.0) d = -30000.0;
    *px = (int)(d + (d < 0 ? -0.5 : 0.5));
    return 1;
}

static DWORD Rgb(int r, int g, int b)
{
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    return RGB(r, g, b);
}

static double HueToRgb(double p, double q, double t)
{
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0 / 6) return p + (q - p) * 6 * t;
    if (t < 0.5) return q;
    if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6;
    return p;
}

static int g_colorA = 255;              /* 最後に読んだ色のアルファ (0～255) */

/* 色の関数の引数 (空白・カンマ・/ で区切った数。none は 0)。数を a、% かどうかを pct に入れ、数の個数を返す
   (読めなければ -1)。角度の turn / rad / grad は度にする */
static int ColorArgs(const char *p, double *a, int *pct)
{
    int na = 0, ok;
    while (na < 4) {
        p = SkipWs(p);
        if (*p == ',' || *p == '/') {
            p++;
            continue;
        }
        if (*p == ')' || *p == '\0') break;
        if (strnicmp(p, "none", 4) == 0) {
            a[na] = 0.0;
            pct[na++] = 0;
            p += 4;
            continue;
        }
        a[na] = Num(&p, &ok);
        if (!ok) return -1;
        pct[na] = *p == '%';
        if (*p == '%') p++;
        else if (strnicmp(p, "turn", 4) == 0) a[na] *= 360.0;
        else if (strnicmp(p, "grad", 4) == 0) a[na] *= 0.9;
        else if (strnicmp(p, "rad", 3) == 0) a[na] *= 57.29577951;
        while (isalpha((unsigned char)*p)) p++;
        na++;
    }
    return na;
}

/* 4 つ目の引数のアルファを g_colorA に入れる。透明なら 1 */
static int ColorAlpha(const double *a, const int *pct, int na)
{
    double al;
    if (na < 4) return 0;
    al = pct[3] ? a[3] / 100.0 : a[3];
    if (al <= 0.0) return 1;
    g_colorA = al >= 1.0 ? 255 : (int)(al * 255.0 + 0.5);
    return 0;
}

/* 線形の sRGB (0～1) を色にする */
static DWORD LinRgb(double r, double g, double b)
{
    double v[3];
    int    o[3], i;
    v[0] = r;
    v[1] = g;
    v[2] = b;
    for (i = 0; i < 3; i++) {
        double x = v[i];
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        x = x <= 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
        o[i] = (int)(x * 255.0 + 0.5);
    }
    return Rgb(o[0], o[1], o[2]);
}

static double SrgbLin(int c)
{
    double x = c / 255.0;
    return x <= 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

static double Cbrt(double x)
{
    return x < 0.0 ? -pow(-x, 1.0 / 3.0) : x > 0.0 ? pow(x, 1.0 / 3.0) : 0.0;
}

/* OKLab を色にする */
static DWORD OklabRgb(double L, double a, double b)
{
    double l = L + 0.3963377774 * a + 0.2158037573 * b;
    double m = L - 0.1055613458 * a - 0.0638541728 * b;
    double s = L - 0.0894841775 * a - 1.2914855480 * b;
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;
    return LinRgb(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
                  -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
                  -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
}

/* 色を OKLab にする */
static void RgbOklab(DWORD c, double *L, double *a, double *b)
{
    double r = SrgbLin(GetRValue(c)), g = SrgbLin(GetGValue(c)), bl = SrgbLin(GetBValue(c));
    double l = Cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * bl);
    double m = Cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * bl);
    double s = Cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * bl);
    *L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
    *a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s;
    *b = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s;
}

/* CIE Lab (白色点 D50) を色にする */
static DWORD LabRgb(double L, double a, double b)
{
    double e = 216.0 / 24389.0, k = 24389.0 / 27.0;
    double fy = (L + 16.0) / 116.0, fx = fy + a / 500.0, fz = fy - b / 200.0;
    double x = fx * fx * fx > e ? fx * fx * fx : (116.0 * fx - 16.0) / k;
    double y = L > k * e ? fy * fy * fy : L / k;
    double z = fz * fz * fz > e ? fz * fz * fz : (116.0 * fz - 16.0) / k;
    x *= 0.96422;
    z *= 0.82521;
    return LinRgb(3.1338561 * x - 1.6168667 * y - 0.4906146 * z,
                  -0.9787684 * x + 1.9161415 * y + 0.0334540 * z,
                  0.0719453 * x - 0.2289914 * y + 1.4052427 * z);
}

int CssParseColor(const char *v, DWORD *out);
static const char *FindI(const char *s, const char *pat);

/* color-mix(in 色空間, 色 [%], 色 [%]) の 1 つの色と割合 (無ければ *pc = -1) */
static int MixArg(const char *a, const char *b, DWORD *col, double *alpha, double *pc)
{
    char        buf[256];
    const char *s1 = a, *s2 = b, *sp;
    int         n, k;
    Trim(&s1, &s2);
    *pc = -1.0;
    if (s2 > s1 && s2[-1] == '%') {
        /* 後ろの割合 */
        sp = s2 - 1;
        while (sp > s1 && !isspace((unsigned char)sp[-1])) sp--;
        if (sp > s1) {
            *pc = strtod(sp, NULL) / 100.0;
            s2 = sp;
            Trim(&s1, &s2);
        }
    } else if (s1 < s2 && (isdigit((unsigned char)*s1) || *s1 == '.')) {
        /* 前の割合 */
        char *e;
        *pc = strtod(s1, &e) / 100.0;
        if (*e == '%') e++;
        s1 = e;
        Trim(&s1, &s2);
    }
    n = (int)(s2 - s1);
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    memcpy(buf, s1, n);
    buf[n] = '\0';
    k = CssParseColor(buf, col);
    *alpha = k == 2 ? 0.0 : g_colorA / 255.0;
    return k;
}

/* color-mix() (p は "(" の次)。oklab / oklch / lab / lch は OKLab で、ほかは sRGB で混ぜる */
static int ColorMix(const char *p, DWORD *out)
{
    const char *e = SkipBlock(p - 1) - 1, *c1 = ScanTo(p, ","), *c2;
    DWORD       x, y;
    double      ax, ay, px, py, sum, mult = 1.0, al;
    int         ok, kx, ky;

    if (*c1 != ',' || c1 > e) return 0;
    c2 = ScanTo(c1 + 1, ",");
    if (*c2 != ',' || c2 > e) return 0;
    ok = FindI(p, "lab") != NULL && FindI(p, "lab") < c1;      /* oklab・lab */
    ok = ok || (FindI(p, "lch") != NULL && FindI(p, "lch") < c1);
    kx = MixArg(c1 + 1, c2, &x, &ax, &px);
    ky = MixArg(c2 + 1, e, &y, &ay, &py);
    if (kx == 0 || ky == 0) return 0;
    if (kx == 3 || ky == 3) return 3;   /* currentColor は混ぜられない: 文字の色で代える */
    if (kx == 2) x = y;                 /* transparent は相手の色の透明なもの */
    if (ky == 2) y = x;
    if (px < 0 && py < 0) px = py = 0.5;
    else if (px < 0) px = 1.0 - py;
    else if (py < 0) py = 1.0 - px;
    sum = px + py;
    if (sum <= 0.0) return 0;
    if (sum < 1.0) mult = sum;          /* 合計が 100% 未満なら、その分だけ透明にする */
    px /= sum;
    py /= sum;
    al = ax * px + ay * py;
    if (al * mult <= 0.0) return 2;
    if (ok) {
        double L1, A1, B1, L2, A2, B2;
        RgbOklab(x, &L1, &A1, &B1);
        RgbOklab(y, &L2, &A2, &B2);
        *out = OklabRgb((L1 * ax * px + L2 * ay * py) / al, (A1 * ax * px + A2 * ay * py) / al,
                        (B1 * ax * px + B2 * ay * py) / al);
    } else {
        *out = Rgb((int)((GetRValue(x) * ax * px + GetRValue(y) * ay * py) / al + 0.5),
                   (int)((GetGValue(x) * ax * px + GetGValue(y) * ay * py) / al + 0.5),
                   (int)((GetBValue(x) * ax * px + GetBValue(y) * ay * py) / al + 0.5));
    }
    al *= mult;
    g_colorA = al >= 1.0 ? 255 : (int)(al * 255.0 + 0.5);
    return 1;
}

/* oklab() / oklch() / lab() / lch() / hwb() / color() / color-mix() */
static int ColorFunc(const char *v, DWORD *out)
{
    const char *p = strchr(v, '(');
    double      a[4];
    int         pct[4], na, i;

    if (p == NULL) return 0;
    if (strnicmp(v, "color-mix(", 10) == 0) {
        return ColorMix(p + 1, out);
    }
    if (strnicmp(v, "color(", 6) == 0) {
        /* color(色空間 r g b [/ a])。srgb-linear 以外 (display-p3 など) は sRGB とみなす */
        const char *q = SkipWs(p + 1);
        int         lin = strnicmp(q, "srgb-linear", 11) == 0;
        while (isalnum((unsigned char)*q) || *q == '-') q++;
        na = ColorArgs(q, a, pct);
        if (na < 3) return 0;
        if (ColorAlpha(a, pct, na)) return 2;
        for (i = 0; i < 3; i++) {
            if (pct[i]) a[i] /= 100.0;
        }
        *out = lin ? LinRgb(a[0], a[1], a[2])
                   : Rgb((int)(a[0] * 255.0 + 0.5), (int)(a[1] * 255.0 + 0.5), (int)(a[2] * 255.0 + 0.5));
        return 1;
    }
    na = ColorArgs(p + 1, a, pct);
    if (na < 3) return 0;
    if (ColorAlpha(a, pct, na)) return 2;
    if (strnicmp(v, "oklab(", 6) == 0) {
        *out = OklabRgb(pct[0] ? a[0] / 100.0 : a[0], pct[1] ? a[1] * 0.004 : a[1], pct[2] ? a[2] * 0.004 : a[2]);
    } else if (strnicmp(v, "oklch(", 6) == 0) {
        double c = pct[1] ? a[1] * 0.004 : a[1], h = a[2] * 3.14159265358979 / 180.0;
        *out = OklabRgb(pct[0] ? a[0] / 100.0 : a[0], c * cos(h), c * sin(h));
    } else if (strnicmp(v, "lab(", 4) == 0) {
        *out = LabRgb(a[0], pct[1] ? a[1] * 1.25 : a[1], pct[2] ? a[2] * 1.25 : a[2]);
    } else if (strnicmp(v, "lch(", 4) == 0) {
        double c = pct[1] ? a[1] * 1.5 : a[1], h = a[2] * 3.14159265358979 / 180.0;
        *out = LabRgb(a[0], c * cos(h), c * sin(h));
    } else {
        /* hwb(色相 白 黒) */
        double h = a[0] / 360.0, w = a[1] / 100.0, bk = a[2] / 100.0, f;
        h -= (int)h;
        if (h < 0) h += 1;
        if (w + bk >= 1.0) {
            int gray = (int)(w / (w + bk) * 255.0 + 0.5);
            *out = Rgb(gray, gray, gray);
            return 1;
        }
        f = 1.0 - w - bk;
        *out = Rgb((int)((HueToRgb(0, 1, h + 1.0 / 3) * f + w) * 255 + 0.5),
                   (int)((HueToRgb(0, 1, h) * f + w) * 255 + 0.5),
                   (int)((HueToRgb(0, 1, h - 1.0 / 3) * f + w) * 255 + 0.5));
    }
    return 1;
}

int CssParseColor(const char *v, DWORD *out)
{
    char name[32];
    int  n = 0, lo, hi, mid;

    g_colorA = 255;
    v = SkipWs(v);
    if (strnicmp(v, "oklab(", 6) == 0 || strnicmp(v, "oklch(", 6) == 0 || strnicmp(v, "lab(", 4) == 0
        || strnicmp(v, "lch(", 4) == 0 || strnicmp(v, "hwb(", 4) == 0 || strnicmp(v, "color(", 6) == 0
        || strnicmp(v, "color-mix(", 10) == 0) {
        return ColorFunc(v, out);
    }
    if (strnicmp(v, "light-dark(", 11) == 0) {
        /* 明るい配色の方 (最初の引数) を使う */
        char        first[64];
        const char *e = ScanTo(v + 11, ",)");
        int         k = (int)(e - (v + 11)) < 63 ? (int)(e - (v + 11)) : 63;
        memcpy(first, v + 11, k);
        first[k] = '\0';
        return CssParseColor(first, out);
    }
    if (*v == '#') {
        int  d[8], k = 0;
        v++;
        while (k < 8 && HexVal((unsigned char)v[k]) >= 0) {
            d[k] = HexVal((unsigned char)v[k]);
            k++;
        }
        if (k == 3 || k == 4) {
            if (k == 4 && d[3] == 0) return 2;
            if (k == 4) g_colorA = d[3] * 17;
            *out = Rgb(d[0] * 17, d[1] * 17, d[2] * 17);
            return 1;
        }
        if (k == 6 || k == 8) {
            if (k == 8 && d[6] == 0 && d[7] == 0) return 2;
            if (k == 8) g_colorA = d[6] * 16 + d[7];
            *out = Rgb(d[0] * 16 + d[1], d[2] * 16 + d[3], d[4] * 16 + d[5]);
            return 1;
        }
        return 0;
    }
    if (strnicmp(v, "rgb", 3) == 0 || strnicmp(v, "hsl", 3) == 0) {
        int         hsl = tolower((unsigned char)*v) == 'h';
        const char *p   = strchr(v, '(');
        double      a[4];
        int         na = 0, ok, pct[4];
        if (p == NULL) return 0;
        p++;
        while (na < 4) {
            p = SkipWs(p);
            if (*p == ',' || *p == '/') {
                p++;
                continue;
            }
            a[na] = Num(&p, &ok);
            if (!ok) break;
            pct[na] = (*p == '%');
            if (*p == '%') p++;
            while (isalpha((unsigned char)*p)) p++;        /* deg など */
            na++;
        }
        if (na < 3) return 0;
        if (hsl && (!pct[1] || !pct[2])) return 0;      /* hsl の彩度・明度は % だけ */
        if (na == 4 && a[3] <= 0.0) return 2;
        if (na == 4) {
            double al = pct[3] ? a[3] / 100.0 : a[3];
            g_colorA = al >= 1.0 ? 255 : (int)(al * 255.0 + 0.5);
        }
        if (hsl) {
            double h = a[0] / 360.0, s = a[1] / 100.0, l = a[2] / 100.0, q, pp;
            h -= (int)h;
            if (h < 0) h += 1;
            q  = l < 0.5 ? l * (1 + s) : l + s - l * s;
            pp = 2 * l - q;
            *out = Rgb((int)(HueToRgb(pp, q, h + 1.0 / 3) * 255 + 0.5),
                       (int)(HueToRgb(pp, q, h) * 255 + 0.5),
                       (int)(HueToRgb(pp, q, h - 1.0 / 3) * 255 + 0.5));
        } else {
            int k;
            for (k = 0; k < 3; k++) {
                if (pct[k]) a[k] = a[k] * 255.0 / 100.0;
            }
            *out = Rgb((int)(a[0] + 0.5), (int)(a[1] + 0.5), (int)(a[2] + 0.5));
        }
        return 1;
    }
    while (isalpha((unsigned char)*v) && n < 31) name[n++] = (char)tolower((unsigned char)*v++);
    name[n] = '\0';
    if (strcmp(name, "transparent") == 0) return 2;
    if (strcmp(name, "currentcolor") == 0) return 3;
    lo = 0;
    hi = (int)(sizeof(g_colors) / sizeof(g_colors[0])) - 1;
    while (lo <= hi) {
        int c;
        mid = (lo + hi) / 2;
        c = strcmp(name, g_colors[mid].name);
        if (c == 0) {
            DWORD x = g_colors[mid].rgb;
            *out = RGB((x >> 16) & 255, (x >> 8) & 255, x & 255);
            return 1;
        }
        if (c < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return 0;
}

int CssParseColorA(const char *v, DWORD *out, int *alpha)
{
    int k = CssParseColor(v, out);
    *alpha = k == 2 ? 0 : g_colorA;
    return k;
}

/* s の中の pat (英字の大小を区別しない) の位置。無ければ NULL */
static const char *FindI(const char *s, const char *pat)
{
    size_t n = strlen(pat);
    for (; *s; s++) {
        if (strnicmp(s, pat, n) == 0) return s;
    }
    return NULL;
}

/* グラデーションの向きの語 (to の後、古い書き方なら from) を角度にする。読めなければ -1 */
static int GradSide(const char *s, int from)
{
    int x = 0, y = 0, a;
    while (*s) {
        s = SkipWs(s);
        if (strnicmp(s, "top", 3) == 0) { y = -1; s += 3; }
        else if (strnicmp(s, "bottom", 6) == 0) { y = 1; s += 6; }
        else if (strnicmp(s, "left", 4) == 0) { x = -1; s += 4; }
        else if (strnicmp(s, "right", 5) == 0) { x = 1; s += 5; }
        else if (*s) return -1;
    }
    if (from) {
        x = -x;
        y = -y;
    }
    if (x == 0 && y == 0) return -1;
    a = x == 0 ? (y < 0 ? 0 : 180) : y == 0 ? (x > 0 ? 90 : 270)
      : x > 0 ? (y < 0 ? 45 : 135) : (y < 0 ? 315 : 225);
    return a;
}

/* 列の 1 つ ("100px"・"25%"・"1fr"・"auto"・"minmax(a, b)") */
static void GridTrackOne(const char *s, int W, CssTrack *t)
{
    char *e;
    double d;

    memset(t, 0, sizeof(*t));
    t->kind = 2;
    s = SkipWs(s);
    if (strnicmp(s, "minmax(", 7) == 0) {
        const char *cm = ScanTo(s + 7, ","), *f = SkipBlock(s + 6) - 1;
        char        a[64], b[64];
        CssTrack    ta, tb;
        int         n = (int)(cm - (s + 7)) < 63 ? (int)(cm - (s + 7)) : 63;
        memcpy(a, s + 7, n);
        a[n] = '\0';
        n = *cm == ',' && f > cm ? ((int)(f - cm - 1) < 63 ? (int)(f - cm - 1) : 63) : 0;
        memcpy(b, cm + 1, n);
        b[n] = '\0';
        GridTrackOne(a, W, &ta);
        GridTrackOne(b, W, &tb);
        *t = tb;
        t->minPx = ta.kind == 0 ? ta.px : 0;
        t->mm    = (BYTE)(tb.kind == 0);
        if (tb.kind == 0 && ta.kind == 0 && ta.px > tb.px) t->px = ta.px;
        return;
    }
    d = strtod(s, &e);
    if (e == s) {
        return;                         /* auto */
    }
    if (strnicmp(e, "fr", 2) == 0) {
        t->kind = 1;
        t->fr   = (int)(d * 100.0 + 0.5);
    } else if (*e == '%') {
        if (W >= 0) {
            t->kind = 0;
            t->px   = (int)(W * d / 100.0 + 0.5);
        }
    } else if (strnicmp(e, "px", 2) == 0) {
        t->kind = 0;
        t->px   = (int)(d + 0.5);
    }
}

/* 線の名前の並び "[a b]" (p は '[') に name があるか */
static int GridHasName(const char *p, const char *name)
{
    const char *e = SkipBlock(p) - 1, *q;
    size_t      n = strlen(name);
    for (p++; p < e; p = q) {
        while (p < e && isspace((unsigned char)*p)) p++;
        for (q = p; q < e && !isspace((unsigned char)*q); q++) ;
        if (n > 0 && (size_t)(q - p) == n && strncmp(p, name, n) == 0) {
            return 1;
        }
    }
    return 0;
}

/* CssGridTracks の本体。name があれば列は作らず、その名前の最初の線の番号 (1 から。無ければ 0) を返す */
static int GridWalk(const char *spec, int W, int gap, CssTrack *t, int max, const char *name)
{
    const char *p = spec;
    int         n = 0;
    CssTrack    dummy;

    if (spec == NULL) {
        return 0;
    }
    while (*p && n < max) {
        const char *q;
        p = SkipWs(p);
        if (*p == '\0') break;
        q = ScanTo(p, " ");
        if (*p == '[') {
            if (name != NULL && GridHasName(p, name)) return n + 1;
            p = q;
            continue;
        }
        if (strnicmp(p, "repeat(", 7) == 0) {
            /* repeat(数か auto-fill / auto-fit, 列の並び) */
            const char *f = SkipBlock(p + 6) - 1, *cm = ScanTo(p + 7, ","), *r;
            CssTrack    one[16];
            int         m = 0, cnt, k, unit = 0, at = -1;
            for (r = cm + 1; r < f && m < 16; ) {
                const char *rq;
                char        tok[128];
                int         len;
                r = SkipWs(r);
                if (r >= f) break;
                rq = ScanTo(r, " ");
                if (rq > f) rq = f;
                if (*r == '[') {
                    if (name != NULL && at < 0 && GridHasName(r, name)) at = m;
                    r = rq;
                    continue;
                }
                len = (int)(rq - r) < 127 ? (int)(rq - r) : 127;
                memcpy(tok, r, len);
                tok[len] = '\0';
                GridTrackOne(tok, W, &one[m]);
                unit += (one[m].kind == 0 ? one[m].px : one[m].minPx) + gap;
                m++;
                r = rq;
            }
            if (m == 0) return n;
            if (strnicmp(SkipWs(p + 7), "auto", 4) == 0) {
                cnt = W > 0 && unit > 0 ? (W + gap) / unit : 1;     /* 幅に入るだけ */
            } else {
                cnt = atoi(SkipWs(p + 7));
            }
            if (cnt < 1) cnt = 1;
            if (at >= 0) return n + at + 1;
            for (; cnt > 0 && n < max; cnt--) {
                for (k = 0; k < m && n < max; k++) {
                    if (t != NULL) t[n] = one[k];
                    n++;
                }
            }
            p = f + 1;
            continue;
        }
        {
            char tok[128];
            int  len = (int)(q - p) < 127 ? (int)(q - p) : 127;
            memcpy(tok, p, len);
            tok[len] = '\0';
            GridTrackOne(tok, W, t != NULL ? &t[n] : &dummy);
            n++;
        }
        p = q;
    }
    return name != NULL ? 0 : n;
}

int CssGridTracks(const char *spec, int W, int gap, CssTrack *t, int max)
{
    return GridWalk(spec, W, gap, t, max, NULL);
}

int CssGridLine(const char *spec, int W, int gap, const char *name)
{
    return GridWalk(spec, W, gap, NULL, 4096, name);
}

int CssGridArea(const char *areas, const char *name, int *r0, int *c0, int *r1, int *c1)
{
    const char *p = areas;
    int         r = 0, c = 0, found = 0, maxC = 0;
    size_t      n = name != NULL ? strlen(name) : 0;

    if (areas == NULL) {
        return 0;
    }
    *r0 = *c0 = 0x7FFF;
    *r1 = *c1 = 0;
    while (*p) {
        const char *q = p;
        while (*q && *q != ' ' && *q != '/') q++;
        if (n > 0 && (size_t)(q - p) == n && strncmp(p, name, n) == 0) {
            found = 1;
            if (r < *r0) *r0 = r;
            if (c < *c0) *c0 = c;
            if (r + 1 > *r1) *r1 = r + 1;
            if (c + 1 > *c1) *c1 = c + 1;
        }
        if (++c > maxC) maxC = c;
        if (*q == '/') {
            r++;
            c = 0;
        }
        p = *q ? q + 1 : q;
    }
    if (name == NULL) {
        *r0 = *c0 = 0;
        *r1 = r + 1;
        *c1 = maxC;
        return 1;
    }
    return found;
}

static int IsWord(const char *v, const char *w);

/* radial-gradient の最初の引数 (形・大きさ・at 位置)。位置は % とキーワードだけ */
static void RadialShape(const char *arg, CssGrad *g)
{
    char tok[8][128];
    int  n = Tokens(arg, tok, 8), i, at = 0, np = 0, nl = 0;

    for (i = 0; i < n; i++) {
        const char *t = tok[i];
        if (IsWord(t, "at")) {
            at = 1;
        } else if (at) {
            int axis = -1, v = -1;
            if (IsWord(t, "left")) { axis = 0; v = 0; }
            else if (IsWord(t, "right")) { axis = 0; v = 1000; }
            else if (IsWord(t, "top")) { axis = 1; v = 0; }
            else if (IsWord(t, "bottom")) { axis = 1; v = 1000; }
            else if (IsWord(t, "center")) { v = 500; }
            else if (strchr(t, '%') != NULL) { double d = strtod(t, NULL) * 10.0; v = d < -5000 ? -5000 : d > 5000 ? 5000 : (int)d; }
            if (v >= 0 || strchr(t, '%') != NULL) {
                if (axis < 0) axis = np == 0 ? 0 : 1;
                if (axis == 0) g->cx = (short)v; else g->cy = (short)v;
            }
            np++;
        } else if (IsWord(t, "circle")) {
            g->circle = 1;
        } else if (IsWord(t, "closest-side")) {
            g->sizeKw = 1;
        } else if (IsWord(t, "closest-corner")) {
            g->sizeKw = 2;
        } else if (IsWord(t, "farthest-side")) {
            g->sizeKw = 3;
        } else if (nl < 2 && (isdigit((unsigned char)*t) || *t == '.')) {
            double d   = strtod(t, NULL);
            int    pct = strchr(t, '%') != NULL;
            short  r   = (short)(pct ? (d * 10.0 > 5000 ? 5000 : d * 10.0) : (d > 8000 ? 8000 : d));
            if (nl == 0) { g->rx = r; g->rxPct = (BYTE)pct; } else { g->ry = r; g->ryPct = (BYTE)pct; }
            g->sizeKw = 4;
            nl++;
        }
    }
    if (nl == 1) {
        g->ry     = g->rx;              /* 長さが 1 つなら円 */
        g->ryPct  = g->rxPct;
        g->circle = 1;
    }
}

int CssGradient(const char *text, CssGrad *g)
{
    const char *p = FindI(text, "linear-gradient("), *r = FindI(text, "radial-gradient("), *e;
    int         legacy, first = 1, i, j, known[CSS_GRAD_STOPS];

    memset(g, 0, sizeof(*g));
    g->angle = 180;
    if (r != NULL && (p == NULL || r < p)) {
        p = r;
        g->radial = 1;
        g->cx = g->cy = 500;
    }
    if (p == NULL) return 0;
    legacy = !g->radial && p > text && p[-1] == '-' && strnicmp(text, "repeating", 9) != 0;   /* -webkit-linear-gradient など */
    g->rep = (BYTE)(p - text >= 10 && strnicmp(p - 10, "repeating-", 10) == 0);
    if (g->rep) legacy = 0;
    p += 16;
    e = SkipBlock(p - 1) - 1;
    while (p < e) {
        const char *q = ScanTo(p, ","), *s1 = p, *s2;
        char        arg[256], *sp;
        int         len;
        if (q > e) q = e;
        s2 = q;
        Trim(&s1, &s2);
        len = (int)(s2 - s1) < 255 ? (int)(s2 - s1) : 255;
        memcpy(arg, s1, len);
        arg[len] = '\0';
        p = q + 1;
        if (first) {
            char  *u;
            double d;
            first = 0;
            if (g->radial) {
                /* 最初の字句が色でなければ、形・大きさ・位置の指定 */
                char  t0[128];
                DWORD tc;
                int   ta, m = (int)strcspn(arg, " \t");
                if (m > 127) m = 127;
                memcpy(t0, arg, m);
                t0[m] = '\0';
                if (CssParseColorA(t0, &tc, &ta) == 0) {
                    RadialShape(arg, g);
                    continue;
                }
            }
            if (strnicmp(arg, "to ", 3) == 0) {
                if ((g->angle = GradSide(arg + 3, 0)) < 0) return 0;
                continue;
            }
            d = strtod(arg, &u);
            if (u != arg && (stricmp(u, "deg") == 0 || stricmp(u, "turn") == 0 || stricmp(u, "rad") == 0
                             || stricmp(u, "grad") == 0)) {
                d = tolower((unsigned char)*u) == 't' ? d * 360.0 : tolower((unsigned char)*u) == 'r' ? d * 57.29578
                  : tolower((unsigned char)*u) == 'g' ? d * 0.9 : d;
                if (legacy) d = 90.0 - d;   /* 古い書き方の角度は右向きが 0 で反時計回り */
                g->angle = ((int)(d + (d < 0 ? -0.5 : 0.5)) % 360 + 360) % 360;
                continue;
            }
            if (legacy && GradSide(arg, 1) >= 0) {
                g->angle = GradSide(arg, 1);
                continue;
            }
        }
        if (g->n >= CSS_GRAD_STOPS) break;
        /* 色 [位置] */
        sp = (char *)ScanTo(arg, " \t");
        if (*sp) {
            *sp++ = '\0';
        }
        {
            DWORD c;
            int   a, k = CssParseColorA(arg, &c, &a);
            if (k == 0) return 0;
            g->color[g->n] = k == 1 ? c : CSS_NOCOLOR;
            g->alpha[g->n] = (BYTE)(k == 2 ? 0 : a);
        }
        sp = (char *)SkipWs(sp);
        known[g->n] = 0;
        if (*sp && strchr(sp, '%') != NULL) {
            double d = strtod(sp, NULL) * 10.0;
            g->pos[g->n] = (short)(d < 0 ? 0 : d > 1000 ? 1000 : (int)(d + 0.5));
            known[g->n] = 1;
        } else if (*sp && FindI(sp, "px") != NULL) {
            /* px の位置 (描くときに長さで割る) */
            double d = strtod(sp, NULL);
            g->pos[g->n] = (short)(d < 0 ? 0 : d > 30000 ? 30000 : (int)(d + 0.5));
            g->unitPx = 1;
            known[g->n] = 1;
        }
        g->n++;
    }
    if (g->n < 2) return 0;
    /* 位置の無い色: 両端は 0 と 100%、間は前後の位置から等分。前より手前には戻さない */
    if (!known[0]) g->pos[0] = 0, known[0] = 1;
    if (!known[g->n - 1]) g->pos[g->n - 1] = g->unitPx ? 0 : 1000, known[g->n - 1] = 1;
    for (i = 1; i < g->n; i++) {
        if (!known[i]) {
            for (j = i; !known[j]; j++) ;
            g->pos[i] = (short)(g->pos[i - 1] + (g->pos[j] - g->pos[i - 1]) / (j - i + 1));
        }
        if (g->pos[i] < g->pos[i - 1]) g->pos[i] = g->pos[i - 1];
        known[i] = 1;
    }
    /* transparent は隣の色の透明なものとして扱う (灰色に混ざらないように) */
    for (i = 0; i < g->n; i++) {
        if (g->alpha[i] == 0 && g->color[i] == CSS_NOCOLOR) {
            g->color[i] = i + 1 < g->n && g->alpha[i + 1] ? g->color[i + 1] : i > 0 ? g->color[i - 1] : 0;
        }
    }
    return 1;
}

int CssBorder(const CssStyle *st, int side)
{
    if (!st->bstyle[side]) return 0;
    return st->bwidth[side] == CSS_UNSET ? 3 : st->bwidth[side];
}

/* ---------------------------------------------------------------- @media */

static int MediaLen(const char *v, const CssMedia *m)
{
    LenCtx c;
    int    px = 0;
    c.em = 16;                          /* メディアクエリの em は常に 16px */
    c.rem = 16;
    c.strict = 0;
    c.vw = m->viewW;
    c.vh = m->viewH;
    c.cq = 0;
    if (ParseLen(v, &c, &px) != 1) {
        double d = atof(v);             /* resolution の dppx / dpi など */
        px = (int)d;
    }
    return px;
}

static int Cmp(int a, const char *op, int b)
{
    if (strcmp(op, "<") == 0) return a < b;
    if (strcmp(op, "<=") == 0) return a <= b;
    if (strcmp(op, ">") == 0) return a > b;
    if (strcmp(op, ">=") == 0) return a >= b;
    return a == b;
}

static int EvalFeature(const CssMedia *m, const char *f)
{
    char        name[48], val[64], op[3];
    const char *p = f, *c;
    int         n = 0;

    g_viewUse = 1;
    p = SkipWs(p);
    /* 範囲の書き方: (width >= 600px) / (400px < width < 800px) */
    if (strpbrk(p, "<>=") != NULL && strchr(p, ':') == NULL) {
        const char *q = p;
        int         ok = 1, w;
        char        a[64], b[64], c2[64], o1[3], o2[3];
        int         na = 0, nb = 0, nc = 0, k;
        while (*q && !strchr("<>=", *q) && na < 63) a[na++] = *q++;
        a[na] = '\0';
        k = 0;
        while (*q && strchr("<>=", *q) && k < 2) o1[k++] = *q++;
        o1[k] = '\0';
        while (*q && !strchr("<>=", *q) && nb < 63) b[nb++] = *q++;
        b[nb] = '\0';
        k = 0;
        while (*q && strchr("<>=", *q) && k < 2) o2[k++] = *q++;
        o2[k] = '\0';
        while (*q && nc < 63) c2[nc++] = *q++;
        c2[nc] = '\0';
        {
            const char *s1 = a, *e1 = a + na, *s2 = b, *e2 = b + nb;
            Trim(&s1, &e1);
            Trim(&s2, &e2);
            a[e1 - a] = '\0';
            b[e2 - b] = '\0';
            if (strstr(s1, "width") == s1 || strstr(s1, "height") == s1) {
                w = s1[0] == 'w' ? m->viewW : m->viewH;
                ok = Cmp(w, o1, MediaLen(s2, m));
            } else {
                w = strstr(s2, "width") ? m->viewW : m->viewH;
                ok = Cmp(MediaLen(s1, m), o1, w);
                if (o2[0]) ok = ok && Cmp(w, o2, MediaLen(c2, m));
            }
        }
        return ok;
    }
    while (*p && *p != ':' && n < 47) name[n++] = *p++;
    name[n] = '\0';
    {
        const char *s1 = name, *e1 = name + n;
        Trim(&s1, &e1);
        memmove(name, s1, e1 - s1);
        name[e1 - s1] = '\0';
    }
    val[0] = '\0';
    if (*p == ':') {
        const char *s1 = p + 1, *e1 = p + strlen(p);
        Trim(&s1, &e1);
        n = (int)(e1 - s1) < 63 ? (int)(e1 - s1) : 63;
        memcpy(val, s1, n);
        val[n] = '\0';
    }
    op[0] = '\0';
    c = name;
    if (strncmp(c, "min-", 4) == 0) { strcpy(op, ">="); c += 4; }
    else if (strncmp(c, "max-", 4) == 0) { strcpy(op, "<="); c += 4; }
    if (strcmp(c, "width") == 0) return val[0] ? Cmp(m->viewW, op[0] ? op : "=", MediaLen(val, m)) : 1;
    if (strcmp(c, "height") == 0) return val[0] ? Cmp(m->viewH, op[0] ? op : "=", MediaLen(val, m)) : 1;
    if (strcmp(c, "device-width") == 0) return val[0] ? Cmp(m->viewW, op[0] ? op : "=", MediaLen(val, m)) : 1;
    if (strcmp(name, "prefers-color-scheme") == 0) return strcmp(val, "light") == 0;
    if (strcmp(name, "prefers-reduced-motion") == 0) return strcmp(val, "reduce") == 0;
    if (strcmp(name, "orientation") == 0) return (strcmp(val, "landscape") == 0) == (m->viewW >= m->viewH);
    if (strcmp(name, "hover") == 0 || strcmp(name, "any-hover") == 0) return strcmp(val, "none") != 0;
    if (strcmp(name, "pointer") == 0 || strcmp(name, "any-pointer") == 0) return strcmp(val, "fine") == 0 || !val[0];
    if (strstr(c, "resolution") || strstr(c, "pixel-ratio")) {
        double d = atof(val);
        if (strstr(val, "dpi")) d /= 96.0;
        return op[0] == '>' ? d <= 1.0 : 1;
    }
    /* 色は成分ごとに 8 ビット、白黒の画面ではない */
    if (strcmp(c, "color") == 0) return val[0] ? Cmp(8, op[0] ? op : "=", atoi(val)) : 1;
    if (strcmp(c, "monochrome") == 0) return val[0] ? Cmp(0, op[0] ? op : "=", atoi(val)) : 0;
    if (strcmp(name, "scripting") == 0) return strcmp(val, "none") == 0;
    if (strcmp(name, "display-mode") == 0) return strcmp(val, "browser") == 0;
    return 0;
}

static int PropId(const char *name);
static int IsWord(const char *v, const char *w);
static int CompileSelector(CssSheet *s, const char *a, const char *b, Rule *r, int rel);

/* @container の条件 ("(min-width: 400px) and (width < 800px)"・"not (...)" など) を、m の幅で評価する。
   inline-size / block-size は width / height として読む。style() などの分からないものは偽 */
static int CqEval(const CssMedia *m, const char *cond)
{
    char  buf[512], *p;
    int   o = 0, v = -1, orOp = 0;
    const char *s = cond;

    while (*s && o < 500) {
        if (strnicmp(s, "inline-size", 11) == 0) { memcpy(buf + o, "width", 5); o += 5; s += 11; }
        else if (strnicmp(s, "block-size", 10) == 0) { memcpy(buf + o, "height", 6); o += 6; s += 10; }
        else buf[o++] = (char)tolower((unsigned char)*s++);
    }
    buf[o] = '\0';
    for (p = buf; *p; ) {
        int r = 0, neg = 0;
        p = (char *)SkipWs(p);
        if (*p == '\0') break;
        if (strncmp(p, "and", 3) == 0 && !isalnum((unsigned char)p[3])) { orOp = 0; p += 3; continue; }
        if (strncmp(p, "or", 2) == 0 && !isalnum((unsigned char)p[2])) { orOp = 1; p += 2; continue; }
        if (strncmp(p, "not", 3) == 0 && !isalnum((unsigned char)p[3])) {
            neg = 1;
            p = (char *)SkipWs(p + 3);
        }
        if (*p == '(') {
            char *e = (char *)SkipBlock(p), save = e[-1];
            const char *in = SkipWs(p + 1);
            e[-1] = '\0';
            r = *in == '(' || strncmp(in, "not", 3) == 0 ? CqEval(m, in) : EvalFeature(m, p + 1);
            e[-1] = save;
            p = e;
        } else {
            while (isalnum((unsigned char)*p) || *p == '-') p++;
            if (*p == '(') p = (char *)SkipBlock(p);    /* style() など */
            else if (*p) p++;
        }
        if (neg) r = !r;
        v = v < 0 ? r : orOp ? (v || r) : (v && r);
    }
    return v > 0;
}

/* @supports の宣言 1 つ (name: value) を扱えるか */
static int SupDecl(const char *name, const char *value)
{
    int id;
    if (name[0] == '-' && name[1] == '-') {
        return 1;                       /* カスタムプロパティ */
    }
    id = PropId(name);
    if (id == P_NONE || *value == '\0') {
        return 0;                       /* 知らない性質 (backdrop-filter など) */
    }
    if (id == P_DISPLAY) {
        static const char *const ok[] = {
            "none", "block", "inline", "inline-block", "list-item", "flex", "inline-flex", "grid", "inline-grid",
            "table", "inline-table", "table-row", "table-cell", "table-row-group", "table-header-group",
            "table-footer-group", "table-caption", "table-column", "table-column-group", "flow-root", "contents", NULL
        };
        int k;
        for (k = 0; ok[k] != NULL && stricmp(value, ok[k]) != 0; k++) ;
        return ok[k] != NULL;
    }
    if (id == P_POSITION) {
        return IsWord(value, "static") || IsWord(value, "relative") || IsWord(value, "absolute")
            || IsWord(value, "fixed") || IsWord(value, "sticky") || IsWord(value, "-webkit-sticky");
    }
    return 1;
}

static int SupCond(CssSheet *s, const char **pp, const char *e);

/* @supports の項 1 つ: not 項 / ( 条件 ) / ( 宣言 ) / selector(...) */
static int SupTerm(CssSheet *s, const char **pp, const char *e)
{
    const char *p = SkipWs(*pp);

    if (p >= e) {
        *pp = e;
        return 0;
    }
    if (strnicmp(p, "not", 3) == 0 && (isspace((unsigned char)p[3]) || p[3] == '(')) {
        *pp = p + 3;
        return !SupTerm(s, pp, e);
    }
    if (strnicmp(p, "selector(", 9) == 0) {
        const char *f = SkipBlock(p + 8);
        Rule        r;
        int         ok;
        memset(&r, 0, sizeof(r));
        ok = CompileSelector(s, p + 9, f - 1, &r, 0) && !r.parts[r.n - 1].pe;
        *pp = f;
        return ok;
    }
    if (*p == '(') {
        const char *f = SkipBlock(p), *in = SkipWs(p + 1), *colon = ScanTo(in, ":");
        *pp = f;
        if (*in == '(' || (strnicmp(in, "not", 3) == 0 && (isspace((unsigned char)in[3]) || in[3] == '('))
            || colon >= f - 1) {
            return SupCond(s, &in, f - 1);
        }
        {
            char        name[64], value[256];
            const char *a = in, *b = colon, *c = colon + 1, *d = f - 1;
            int         n;
            Trim(&a, &b);
            Trim(&c, &d);
            n = (int)(b - a) < 63 ? (int)(b - a) : 63;
            memcpy(name, a, n);
            name[n] = '\0';
            Lower(name);
            n = (int)(d - c) < 255 ? (int)(d - c) : 255;
            memcpy(value, c, n);
            value[n] = '\0';
            return SupDecl(name, value);
        }
    }
    /* 分からない書き方 (font-tech() など) は偽 */
    while (p < e && !isspace((unsigned char)*p)) {
        if (*p == '(') p = SkipBlock(p); else p++;
    }
    *pp = p;
    return 0;
}

/* @supports の条件: 項を and / or でつなげたもの */
static int SupCond(CssSheet *s, const char **pp, const char *e)
{
    int v = SupTerm(s, pp, e);
    for (;;) {
        const char *p = SkipWs(*pp);
        int         t;
        if (p < e && strnicmp(p, "and", 3) == 0 && !isalnum((unsigned char)p[3])) {
            *pp = p + 3;
            t = SupTerm(s, pp, e);
            v = v && t;
        } else if (p < e && strnicmp(p, "or", 2) == 0 && !isalnum((unsigned char)p[2])) {
            *pp = p + 2;
            t = SupTerm(s, pp, e);
            v = v || t;
        } else {
            break;
        }
    }
    return v;
}

/* @media の条件 (a..b) を評価する */
static int EvalMedia(const CssSheet *s, const char *a, const char *b)
{
    char buf[512];
    int  n = (int)(b - a) < 511 ? (int)(b - a) : 511;
    char *p, *q;

    memcpy(buf, a, n);
    buf[n] = '\0';
    Lower(buf);
    for (p = buf; p != NULL; p = q) {
        int  neg = 0, ok = 1;
        char *t;
        q = (char *)ScanTo(p, ",");
        if (*q == ',') {
            *q++ = '\0';
        } else {
            q = NULL;
        }
        t = (char *)SkipWs(p);
        if (strncmp(t, "not ", 4) == 0) { neg = 1; t += 4; }
        if (strncmp(t, "only ", 5) == 0) t += 5;
        t = (char *)SkipWs(t);
        while (*t && ok) {
            if (*t == '(') {
                char *e = (char *)SkipBlock(t);
                char  save = e[-1];
                e[-1] = '\0';
                ok = EvalFeature(&s->m, t + 1);
                e[-1] = save;
                t = e;
            } else if (isalpha((unsigned char)*t)) {
                char word[32];
                int  k = 0;
                while (isalpha((unsigned char)*t) || *t == '-') {
                    if (k < 31) word[k++] = *t;
                    t++;
                }
                word[k] = '\0';
                if (strcmp(word, "and") && strcmp(word, "all") && strcmp(word, "screen")) {
                    ok = 0;             /* print などのメディア種別 */
                }
            } else {
                t++;
            }
        }
        if (ok != neg) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------- セレクタ */

/* :nth-child() の引数 (a..b) "odd" "even" "3" "2n+1" "-n+3" を読む。読めなければ 0 */
static int ParseNth(const char *a, const char *b, short *pa, short *pb)
{
    char buf[32], *p;
    int  n = 0, sign = 1, num = 0, digits = 0;
    long va, vb = 0;

    for (; a < b && n < 31; a++) {
        if (!isspace((unsigned char)*a)) {
            buf[n++] = (char)tolower((unsigned char)*a);
        }
    }
    buf[n] = '\0';
    if (strcmp(buf, "odd") == 0)  { *pa = 2; *pb = 1; return 1; }
    if (strcmp(buf, "even") == 0) { *pa = 2; *pb = 0; return 1; }
    p = buf;
    if (*p == '+' || *p == '-') sign = *p++ == '-' ? -1 : 1;
    while (isdigit((unsigned char)*p)) {
        num = num * 10 + (*p++ - '0');
        digits = 1;
        if (num > 9999) return 0;
    }
    if (*p == 'n') {
        va = sign * (digits ? num : 1);
        p++;
        if (*p == '+' || *p == '-') {
            sign = *p++ == '-' ? -1 : 1;
            num = 0;
            if (!isdigit((unsigned char)*p)) return 0;
            while (isdigit((unsigned char)*p)) {
                num = num * 10 + (*p++ - '0');
                if (num > 9999) return 0;
            }
            vb = sign * num;
        }
    } else {
        if (!digits) return 0;
        va = 0;
        vb = sign * num;
    }
    if (*p != '\0') return 0;           /* "2n+1 of .x" などは扱わない */
    *pa = (short)va;
    *pb = (short)vb;
    return 1;
}

static int CompileSelector(CssSheet *s, const char *a, const char *b, Rule *r, int rel);

static int g_selDepth, g_inHas;         /* 読んでいる :is() などの入れ子の深さ / :has() の中 */

/* セレクタの並びを引数に取る疑似クラスの名前か */
static int SelListName(const char *name)
{
    static const char *const names[] = { "not", "is", "where", "matches", "-webkit-any", "-moz-any", "has" };
    int k;
    for (k = 0; k < (int)(sizeof(names) / sizeof(names[0])); k++) {
        if (strcmp(name, names[k]) == 0) return 1;
    }
    return 0;
}

/* :is() :where() :not() :has() の中のセレクタの並び (a..b) を pc に読む。
   :is() と :where() は読めないセレクタを飛ばす。spec に一番大きい詳細度を入れる */
static int ParseSelList(CssSheet *s, const char *a, const char *b, const char *name, PseudoCond *pc, int *spec)
{
    Rule rs[16];
    int  n = 0, has = strcmp(name, "has") == 0, forgive = !has && strcmp(name, "not") != 0;

    if (g_selDepth >= 4 || (has && g_inHas)) {
        return 0;                       /* 深すぎる入れ子と :has() の中の :has() */
    }
    g_selDepth++;
    g_inHas += has;
    while (a < b) {
        const char *c = ScanTo(a, ","), *s1 = a, *s2;
        if (c > b) c = b;
        s2 = c;
        Trim(&s1, &s2);
        if (n < 16 && s1 < s2 && CompileSelector(s, s1, s2, &rs[n], has) && !rs[n].parts[rs[n].n - 1].pe) {
            if (rs[n].spec > *spec) *spec = rs[n].spec;
            n++;
        } else if (!forgive) {
            n = -1;
            break;
        }
        a = c + 1;
    }
    g_selDepth--;
    g_inHas -= has;
    if (n < 0 || (n == 0 && !forgive)) {
        return 0;
    }
    pc->kind = (BYTE)(has ? PS_HAS : strcmp(name, "not") == 0 ? PS_NOT : PS_IS);
    pc->nSel = n;
    if (n) {
        Rule *d = Alloc(s, n * sizeof(Rule));
        memcpy(d, rs, n * sizeof(Rule));
        pc->sel = d;
    }
    return 1;
}

static int ParseCompound(CssSheet *s, const char **pp, const char *b, Compound *c, int *spec)
{
    const char *p = *pp;
    char        buf[256];
    char       *cls[MAX_CLS];
    AttrCond    attrs[8];
    PseudoCond  ps[MAX_PSEUDO];
    int         nCls = 0, nAttrs = 0, nPs = 0, got = 0;

    memset(c, 0, sizeof(*c));
    memset(ps, 0, sizeof(ps));          /* 種類ごとに使わない項目 (sel・nSel・str など) を 0 にしておく */
    if (p < b && *p == '*') {
        p++;
        got = 1;
    } else if (p < b && (isalpha((unsigned char)*p) || *p == '_' || *p == '\\' || (unsigned char)*p >= 0x80)) {
        ParseIdent(&p, b, buf, sizeof(buf));
        Lower(buf);
        c->tag = Dup(s, buf, buf + strlen(buf));
        *spec += 1;
        got = 1;
    }
    while (p < b) {
        if (*p == '#') {
            p++;
            if (ParseIdent(&p, b, buf, sizeof(buf)) == 0) return 0;
            c->id = Dup(s, buf, buf + strlen(buf));
            *spec += 10000;
        } else if (*p == '.') {
            p++;
            if (ParseIdent(&p, b, buf, sizeof(buf)) == 0 || nCls >= MAX_CLS) return 0;
            cls[nCls++] = Dup(s, buf, buf + strlen(buf));
            *spec += 100;
        } else if (*p == '[') {
            AttrCond *a;
            const char *e = SkipBlock(p);
            if (nAttrs >= 8) return 0;
            a = &attrs[nAttrs];
            memset(a, 0, sizeof(*a));
            p = SkipWs(p + 1);
            if (ParseIdent(&p, b, buf, sizeof(buf)) == 0) return 0;
            Lower(buf);
            a->name = Dup(s, buf, buf + strlen(buf));
            p = SkipWs(p);
            if (*p == '=') {
                a->op = '=';
                p++;
            } else if (*p && strchr("~^$*|", *p) && p[1] == '=') {
                a->op = (BYTE)*p;
                p += 2;
            }
            if (a->op) {
                p = SkipWs(p);
                if (*p == '"' || *p == '\'') {
                    char        q = *p++;
                    const char *v = p;
                    while (p < b && *p && *p != q) p++;
                    a->val = Dup(s, v, p);
                    if (*p == q) p++;
                } else {
                    ParseIdent(&p, b, buf, sizeof(buf));
                    a->val = Dup(s, buf, buf + strlen(buf));
                }
                p = SkipWs(p);
                if (*p == 'i' || *p == 'I') {
                    a->icase = 1;
                    p++;
                } else if (*p == 's' || *p == 'S') {
                    p++;
                }
            }
            p = SkipWs(p);
            if (*p != ']' || e > b) return 0;
            p++;
            nAttrs++;
            *spec += 100;
        } else if (*p == ':' && (strnicmp(p + (p[1] == ':' ? 2 : 1), "before", 6) == 0
                                 || strnicmp(p + (p[1] == ':' ? 2 : 1), "after", 5) == 0)
                   && !isalnum((unsigned char)p[(p[1] == ':' ? 2 : 1) + (tolower((unsigned char)p[(p[1] == ':' ? 2 : 1)]) == 'b' ? 6 : 5)])
                   && p[(p[1] == ':' ? 2 : 1) + (tolower((unsigned char)p[(p[1] == ':' ? 2 : 1)]) == 'b' ? 6 : 5)] != '-') {
            /* ::before / ::after (CSS2 の :before も) */
            const char *q = p + (p[1] == ':' ? 2 : 1);
            c->pe = (BYTE)(tolower((unsigned char)*q) == 'b' ? 3 : 4);
            *spec += 1;
            p = q + (c->pe == 3 ? 6 : 5);
        } else if (*p == ':' && p[1] == ':' && strnicmp(p + 2, "placeholder", 11) == 0 && !isalnum((unsigned char)p[13])
                   && p[13] != '-' && p[13] != '(') {
            /* ::placeholder (文字の色だけ使う) */
            c->pe = 6;
            *spec += 1;
            p += 13;
        } else if (*p == ':' && p[1] == ':' && strnicmp(p + 2, "-webkit-input-placeholder", 25) == 0) {
            c->pe = 6;
            *spec += 1;
            p += 27;
        } else if (*p == ':' && p[1] == ':' && strnicmp(p + 2, "marker", 6) == 0 && !isalnum((unsigned char)p[8])
                   && p[8] != '-' && p[8] != '(') {
            /* ::marker (リストの記号) */
            c->pe = 5;
            *spec += 1;
            p += 8;
        } else if (*p == ':' && (strnicmp(p + (p[1] == ':' ? 2 : 1), "first-letter", 12) == 0
                                 || strnicmp(p + (p[1] == ':' ? 2 : 1), "first-line", 10) == 0)) {
            /* ::first-letter / ::first-line (CSS1 の :first-letter の書き方も) */
            const char *q = p + (p[1] == ':' ? 2 : 1);
            int         n = tolower((unsigned char)q[6]) == 'l' && tolower((unsigned char)q[7]) == 'e' ? 12 : 10;
            if (q + n < b && (isalnum((unsigned char)q[n]) || q[n] == '-' || q[n] == '(')) return 0;
            c->pe = (BYTE)(n == 12 ? 1 : 2);
            *spec += 1;
            p = q + n;
        } else if (*p == ':') {
            if (p[1] == ':') return 0;  /* ほかの疑似要素 (::before など) は描けない */
            p++;
            ParseIdent(&p, b, buf, sizeof(buf));
            Lower(buf);
            if (*p == '(') {
                const char *e  = SkipBlock(p);
                const char *ap = p + 1;
                int         add = 100;
                if (e > b || e[-1] != ')') return 0;
                if (SelListName(buf)) {
                    /* :not(a, b) = a でも b でもない。:is() は合うものがある。:where() は詳細度 0。
                       :has(> a) は a の子 (子孫・後の兄弟) がある */
                    int sp = 0;
                    if (nPs >= MAX_PSEUDO || !ParseSelList(s, ap, e - 1, buf, &ps[nPs], &sp)) return 0;
                    nPs++;
                    c->nested = 1;
                    add = strcmp(buf, "where") == 0 ? 0 : sp;
                } else if (strcmp(buf, "lang") == 0) {
                    /* :lang(ja): 要素か祖先の lang 属性が ja か ja- で始まる */
                    const char *la = SkipWs(ap), *lb = e - 1;
                    while (lb > la && isspace((unsigned char)lb[-1])) lb--;
                    if (la < lb && (*la == '"' || *la == '\'')) {
                        la++;
                        if (lb > la && (lb[-1] == '"' || lb[-1] == '\'')) lb--;
                    }
                    if (la >= lb || nPs >= MAX_PSEUDO) return 0;
                    ps[nPs].kind = PS_LANG;
                    ps[nPs].str  = Dup(s, la, lb);
                    nPs++;
                } else {
                    static const char *const nth[4] = { "nth-child", "nth-last-child", "nth-of-type", "nth-last-of-type" };
                    int k;
                    const char *of = NULL, *x;
                    for (k = 0; k < 4 && strcmp(buf, nth[k]) != 0; k++) ;
                    for (x = ap; k < 2 && x + 4 < e - 1; x++) {
                        /* :nth-child(an+b of セレクタ) */
                        if (isspace((unsigned char)x[0]) && strnicmp(x + 1, "of", 2) == 0 && isspace((unsigned char)x[3])) {
                            of = x;
                            break;
                        }
                    }
                    if (k == 4 || nPs >= MAX_PSEUDO || !ParseNth(ap, of != NULL ? of : e - 1, &ps[nPs].a, &ps[nPs].b)) return 0;
                    if (of != NULL) {
                        PseudoCond tmp;
                        int        sp2 = 0;
                        memset(&tmp, 0, sizeof(tmp));
                        if (!ParseSelList(s, of + 4, e - 1, "is", &tmp, &sp2) || tmp.nSel == 0) return 0;
                        ps[nPs].sel  = tmp.sel;
                        ps[nPs].nSel = tmp.nSel;
                        *spec += sp2;
                    }
                    ps[nPs++].kind = (BYTE)(PS_NTH + k);
                }
                *spec += add;
                p = e;
            } else if (strcmp(buf, "link") == 0 || strcmp(buf, "visited") == 0 || strcmp(buf, "any-link") == 0) {
                c->link = (BYTE)(buf[0] == 'l' ? 1 : buf[0] == 'v' ? 2 : 3);
                *spec += 100;
            } else if (strcmp(buf, "root") == 0) {
                c->root = 1;
                *spec += 100;
            } else if (strcmp(buf, "defined") == 0) {
                *spec += 100;                   /* カスタム要素は無いので、どの要素も定義済み */
            } else {
                static const struct { const char *name; BYTE kind; short a, b; } simple[] = {
                    { "first-child", PS_NTH, 0, 1 }, { "last-child", PS_NTH_LAST, 0, 1 },
                    { "only-child", PS_ONLY, 0, 0 }, { "first-of-type", PS_NTH_TYPE, 0, 1 },
                    { "last-of-type", PS_NTH_LAST_TYPE, 0, 1 }, { "only-of-type", PS_ONLY_TYPE, 0, 0 },
                    { "empty", PS_EMPTY, 0, 0 }, { "checked", PS_CHECKED, 0, 0 },
        { "required", PS_REQUIRED, 0, 0 }, { "optional", PS_OPTIONAL, 0, 0 },
        { "read-only", PS_READONLY, 0, 0 }, { "read-write", PS_READWRITE, 0, 0 },
        { "placeholder-shown", PS_PHSHOWN, 0, 0 }, { "valid", PS_VALID, 0, 0 }, { "invalid", PS_INVALID, 0, 0 },
        { "user-invalid", PS_INVALID, 0, 0 }, { "user-valid", PS_VALID, 0, 0 },
                    { "disabled", PS_DISABLED, 0, 0 }, { "enabled", PS_ENABLED, 0, 0 },
                    { "focus", PS_FOCUS, 0, 0 }, { "focus-visible", PS_FOCUS, 0, 0 },
                    { "focus-within", PS_FOCUS_WITHIN, 0, 0 },
                    /* マウスの位置などで変わるもの (変わったら表示し直す) */
                    { "hover", PS_HOVER, 0, 0 }, { "active", PS_ACTIVE, 0, 0 }, { "target", PS_TARGET, 0, 0 }
                };
                int k;
                for (k = 0; k < (int)(sizeof(simple) / sizeof(simple[0])) && strcmp(buf, simple[k].name) != 0; k++) ;
                if (k == (int)(sizeof(simple) / sizeof(simple[0])) || nPs >= MAX_PSEUDO) {
                    return 0;           /* :lang() / :placeholder-shown など */
                }
                ps[nPs].kind = simple[k].kind;
                ps[nPs].a    = simple[k].a;
                ps[nPs].b    = simple[k].b;
                nPs++;
                switch (simple[k].kind) {
                case PS_ACTIVE: s->usesState |= CSS_U_ACTIVE; break;
                case PS_HOVER:  s->usesState |= CSS_U_HOVER; break;
                case PS_TARGET: s->usesState |= CSS_U_TARGET; break;
                case PS_FOCUS: case PS_FOCUS_WITHIN: s->usesState |= CSS_U_FOCUS; break;
                case PS_VALID: case PS_INVALID: case PS_PHSHOWN: s->usesState |= CSS_U_VALID; break;
                }
                if (simple[k].kind == PS_CHECKED) {
                    s->usesChecked = 1;
                }
                *spec += 100;
            }
        } else {
            break;
        }
        got = 1;
    }
    if (nCls) {
        c->cls = Alloc(s, nCls * sizeof(char *));
        memcpy(c->cls, cls, nCls * sizeof(char *));
        c->nCls = nCls;
    }
    if (nAttrs) {
        c->attrs = Alloc(s, nAttrs * sizeof(AttrCond));
        memcpy(c->attrs, attrs, nAttrs * sizeof(AttrCond));
        c->nAttrs = nAttrs;
    }
    if (nPs) {
        c->ps = Alloc(s, nPs * sizeof(PseudoCond));
        memcpy(c->ps, ps, nPs * sizeof(PseudoCond));
        c->nPs = nPs;
    }
    *pp = p;
    return got;
}

/* セレクタ 1 つ (a..b) を r に組み立てる。扱えなければ 0。
   rel なら :has() の中の "> a" のような相対セレクタで、先頭に基準の要素を置く */
static int CompileSelector(CssSheet *s, const char *a, const char *b, Rule *r, int rel)
{
    Compound parts[MAX_PARTS];
    int      n = 0, spec = 0;
    BYTE     comb = 0;
    const char *p = a;

    Trim(&p, &b);
    if (b > p && b[-1] == '\\' && (*b == ' ' || *b == '\t')) {
        const char *q = b - 1;
        while (q > p && q[-1] == '\\') q--;
        if ((b - q) % 2) b++;           /* "#\ " のエスケープした空白は削らない */
    }
    if (rel) {
        memset(&parts[0], 0, sizeof(parts[0]));
        parts[0].anchor = 1;
        n    = 1;
        comb = ' ';
        if (p < b && (*p == '>' || *p == '+' || *p == '~')) {
            comb = (BYTE)*p;
            p = SkipWs(p + 1);
        }
        if (p >= b) return 0;
    }
    while (p < b) {
        int ws = 0;
        if (n >= MAX_PARTS) return 0;
        if (!ParseCompound(s, &p, b, &parts[n], &spec)) return 0;
        parts[n].comb = (BYTE)(n == 0 ? 0 : comb);
        n++;
        while (p < b && isspace((unsigned char)*p)) {
            p++;
            ws = 1;
        }
        if (p >= b) break;
        if (*p == '>' || *p == '+' || *p == '~') {
            comb = (BYTE)*p;
            p = SkipWs(p + 1);
        } else if (ws) {
            comb = ' ';
        } else {
            return 0;
        }
    }
    if (n == 0) return 0;
    {
        int k;
        for (k = 0; k < n - 1; k++) {
            if (parts[k].pe) return 0;  /* 疑似要素はセレクタの最後だけ */
        }
    }
    r->cq    = s->curCq;
    r->layer = s->curLayer;
    r->parts = Alloc(s, n * sizeof(Compound));
    memcpy(r->parts, parts, n * sizeof(Compound));
    r->n    = n;
    r->spec = spec;
    return 1;
}

static unsigned Hash(char kind, const char *k)
{
    unsigned h = (unsigned char)kind * 31u;
    for (; *k; k++) {
        h = h * 33u + (unsigned char)*k;
    }
    return h & (HASH_N - 1);
}

static Bucket *FindBucket(const CssSheet *s, char kind, const char *key)
{
    Bucket *b;
    for (b = s->hash[Hash(kind, key)]; b != NULL; b = b->next) {
        if (b->kind == kind && strcmp(b->key, key) == 0) {
            return b;
        }
    }
    return NULL;
}

static void AddRule(CssSheet *s, const Rule *r)
{
    const Compound *c = &r->parts[r->n - 1];
    int             idx = s->nRules;
    char            kind = 0;
    const char     *key = NULL;

    s->rules = Grow(s->rules, &s->capRules, s->nRules + 1, sizeof(Rule));
    s->rules[s->nRules++] = *r;
    s->usesPe |= (BYTE)(1 << c->pe);
    if (c->id) { kind = '#'; key = c->id; }
    else if (c->nCls) { kind = '.'; key = c->cls[0]; }
    else if (c->tag) { kind = 't'; key = c->tag; }
    if (key == NULL) {
        s->univ = Grow(s->univ, &s->capUniv, s->nUniv + 1, sizeof(int));
        s->univ[s->nUniv++] = idx;
    } else {
        Bucket *b = FindBucket(s, kind, key);
        if (b == NULL) {
            unsigned h = Hash(kind, key);
            b = Alloc(s, sizeof(Bucket));
            memset(b, 0, sizeof(*b));
            b->kind = kind;
            b->key  = (char *)key;
            b->next = s->hash[h];
            s->hash[h] = b;
        }
        b->ids = Grow(b->ids, &b->cap, b->n + 1, sizeof(int));
        b->ids[b->n++] = idx;
    }
}

/* ---------------------------------------------------------------- 宣言 */

typedef void (*DeclCb)(void *ctx, const char *name, const char *vs, const char *ve, int imp);

/* 宣言の並び "a: b; c: d !important" を読む */
static void ParseDeclText(const char *p, const char *end, DeclCb cb, void *ctx)
{
    while (p < end && *p) {
        const char *q, *ns, *ne, *vs, *ve;
        char        name[64];
        int         imp = 0, n;

        p = SkipWs(p);
        if (p >= end || *p == '\0') break;
        if (*p == ';') {
            p++;
            continue;
        }
        q = ScanTo(p, ":;{}");
        if (q >= end || *q == '\0' || *q == '}') break;
        if (*q == '{') {                /* CSS ネストの入れ子ルールは読み飛ばす */
            p = SkipBlock(q);
            continue;
        }
        if (*q == ';') {
            p = q + 1;
            continue;
        }
        ns = p;
        ne = q;
        Trim(&ns, &ne);
        vs = q + 1;
        q = ScanTo(vs, ";{}");
        if (*q == '{') {                /* a:hover { ... } のような入れ子ルール */
            p = SkipBlock(q);
            continue;
        }
        ve = q > end ? end : q;
        Trim(&vs, &ve);
        /* !important */
        {
            const char *x = ve;
            while (x > vs && isalpha((unsigned char)x[-1])) x--;
            if (ve - x == 9 && strnicmp(x, "important", 9) == 0) {
                const char *y = x;
                while (y > vs && isspace((unsigned char)y[-1])) y--;
                if (y > vs && y[-1] == '!') {
                    imp = 1;
                    ve = y - 1;
                    Trim(&vs, &ve);
                }
            }
        }
        {
            /* "! error" のように important 以外の ! がある宣言は無効 */
            const char *x;
            char        qc = 0;
            for (x = vs; x < ve; x++) {
                if (qc) {
                    if (*x == qc) qc = 0;
                } else if (*x == '"' || *x == '\'') {
                    qc = *x;
                } else if (*x == '!') {
                    break;
                }
            }
            if (x < ve) {
                ve = vs;
            }
        }
        n = (int)(ne - ns);
        if (n > 0 && n < (int)sizeof(name) && ve > vs) {
            memcpy(name, ns, n);
            name[n] = '\0';
            if (name[0] != '-' || name[1] != '-') {
                Lower(name);
            }
            cb(ctx, name, vs, ve, imp);
        }
        if (*q == '\0' || q >= end || *q == '}') break;
        p = q + 1;
    }
}

static int CmpProp(const void *a, const void *b)
{
    return strcmp((const char *)a, ((const PropName *)b)->name);
}

static int PropId(const char *name)
{
    const PropName *p = bsearch(name, g_props, sizeof(g_props) / sizeof(g_props[0]),
                                sizeof(PropName), CmpProp);
    if (p == NULL && name[0] == '-') {
        /* -webkit- などの接頭辞付きは無視 */
        return P_NONE;
    }
    return p ? p->id : P_NONE;
}

typedef struct {
    CssSheet *s;
    int       global;
} SheetCtx;

static void AddVar(CssSheet *s, const char *name, const char *vs, const char *ve)
{
    int i;
    for (i = 0; i < s->nVars; i++) {
        if (strcmp(s->vars[i].name, name) == 0) {
            s->vars[i].val = Dup(s, vs, ve);
            return;
        }
    }
    s->vars = Grow(s->vars, &s->capVars, s->nVars + 1, sizeof(Var));
    s->vars[s->nVars].name = Dup(s, name, name + strlen(name));
    s->vars[s->nVars].val  = Dup(s, vs, ve);
    s->nVars++;
}

static int CursorOf(const char *v);

static void SheetDecl(void *ctx, const char *name, const char *vs, const char *ve, int imp)
{
    SheetCtx *sc = ctx;
    CssSheet *s  = sc->s;
    int       id;
    Decl     *d;

    if (name[0] == '-' && name[1] == '-') {
        if (sc->global) {
            AddVar(s, name, vs, ve);    /* :root などで定義したもの */
        } else {
            /* 普通のルールのもの: 合った要素の並びに足す (CssApply) */
            s->decls = Grow(s->decls, &s->capDecls, s->nDecls + 1, sizeof(Decl));
            d = &s->decls[s->nDecls++];
            d->prop = P_VAR;
            d->imp  = (BYTE)imp;
            d->val  = Dup(s, vs, ve);
            d->name = Dup(s, name, name + strlen(name));
        }
        return;
    }
    id = PropId(name);
    if (id == P_NONE) {
        if (!s->m.keepAll || name[0] == '-') {
            return;
        }
        id = P_OTHER;
    }
    if (id == P_CURSOR && ve - vs < 200) {
        char cv[200];
        memcpy(cv, vs, ve - vs);
        cv[ve - vs] = '\0';
        if (strstr(cv, "var(") == NULL && CursorOf(SkipWs(cv)) < 0) {
            return;                     /* 分からない cursor は宣言ごと無視する */
        }
    }
    s->decls = Grow(s->decls, &s->capDecls, s->nDecls + 1, sizeof(Decl));
    d = &s->decls[s->nDecls++];
    d->prop = (short)id;
    d->imp  = (BYTE)imp;
    d->val  = Dup(s, vs, ve);
    d->name = id == P_OTHER ? Dup(s, name, name + strlen(name)) : NULL;
}

/* ---------------------------------------------------------------- ルールの解析 */

static const char *ParseRules(CssSheet *s, const char *p, int nested);
static void ParseNested(CssSheet *s, const char *p, const char *q, const char *end);

/* @layer の名前 a..b の順位 (1 から。初めての名前は後ろに足す。a が NULL なら名前の無い層を新しく作る) */
static int LayerRank(CssSheet *s, const char *a, const char *b)
{
    int i;
    if (a != NULL) {
        for (i = 0; i < s->nLayers; i++) {
            if (s->layers[i] != NULL && (int)strlen(s->layers[i]) == (int)(b - a) && strncmp(s->layers[i], a, b - a) == 0) {
                return i + 1;
            }
        }
    }
    s->layers = Grow(s->layers, &s->capLayers, s->nLayers + 1, sizeof(char *));
    s->layers[s->nLayers++] = a != NULL ? Dup(s, a, b) : NULL;
    return s->nLayers;
}

/* p はセレクタの先頭、q は '{' を指す。ブロックの次を返す */
static const char *ParseStyleRule(CssSheet *s, const char *p, const char *q)
{
    const char *end = SkipBlock(q);
    const char *a;
    Rule        sels[64];
    int         nSel = 0, global = 0, i, d0;
    SheetCtx    sc;

    for (a = p; a < q; ) {
        const char *c = ScanTo(a, ",{");
        const char *s1 = a, *s2;
        if (c > q) c = q;
        s2 = c;
        Trim(&s1, &s2);
        if (s2 - s1 == 5 && strncmp(s1, ":root", 5) == 0) global = 1;
        if (s2 - s1 == 4 && strncmp(s1, "html", 4) == 0) global = 1;
        if (s2 - s1 == 4 && strncmp(s1, "body", 4) == 0) global = 1;
        if (s2 - s1 == 1 && *s1 == '*') global = 1;
        if (nSel < 64 && CompileSelector(s, s1, s2, &sels[nSel], 0)) {
            nSel++;
        }
        a = c + 1;
    }
    d0 = s->nDecls;
    sc.s = s;
    sc.global = global;
    ParseDeclText(q + 1, end > q + 1 ? end - 1 : end, SheetDecl, &sc);
    if (s->nDecls > d0) {
        for (i = 0; i < nSel; i++) {
            sels[i].d0 = d0;
            sels[i].nd = s->nDecls - d0;
            AddRule(s, &sels[i]);
        }
    }
    ParseNested(s, p, q, end);
    return end;
}

/* 文字列 a..b を out に足す (out は十分な大きさ) */
static char *Put(char *o, const char *a, const char *b)
{
    memcpy(o, a, b - a);
    return o + (b - a);
}

/* 規則 (セレクタ p..q、ブロック q..end) の中の入れ子の規則を読む (CSS の入れ子)。
   セレクタの & は親のセレクタ (並びなら :is(親)) にし、& が無ければ親の子孫とする。
   中の @media などは "@media … { 親 { … } }" の形にして読む */
static void ParseNested(CssSheet *s, const char *p, const char *q, const char *end)
{
    const char *sp = p, *se = q, *a = q + 1, *bend = end > q + 1 ? end - 1 : end;
    int         list;

    Trim(&sp, &se);
    if (se <= sp || bend <= a) {
        return;
    }
    if (s->ruleDepth >= NEST_DEPTH_MAX) {
        return;
    }
    s->ruleDepth++;
    list = ScanTo(sp, ",") < se;
    while (a < bend) {
        const char *t, *ns, *ne, *be;
        a = SkipWs(a);
        if (a >= bend) break;
        t = ScanTo(a, ";{}");
        if (t >= bend || *t != '{') {
            if (t >= bend || *t != ';') break;
            a = t + 1;                  /* 宣言 */
            continue;
        }
        be = SkipBlock(t);              /* 入れ子の規則 a..t { … } */
        ns = a;
        ne = t;
        Trim(&ns, &ne);
        if (ne > ns) {
            size_t      plen = (size_t)(se - sp), k = 1, n;
            const char *z;
            char       *buf, *o;
            /* 親のセレクタを写す回数は & とカンマの数で決まる。掛ける前に上限を確かめる (桁あふれ) */
            for (z = ns; z < ne; z++) {
                if (*z == '&' || *z == ',') k++;
            }
            if (k > NEST_ONE_MAX / (plen + 8)) {
                a = be;
                continue;
            }
            n = k * (plen + 8) + (size_t)(ne - ns) + (size_t)(be - t) + plen * 2 + 32;
            if (n > NEST_ONE_MAX || n > NEST_TOTAL_MAX - s->nestUsed) {
                a = be;
                continue;
            }
            s->nestUsed += n;
            buf = CssXmalloc(n);
            o   = buf;
            if (*ns == '@') {
                o = Put(o, ns, ne);
                *o++ = '{';
                o = Put(o, sp, se);
                o = Put(o, t, be);      /* { … } */
                *o++ = '}';
                *o = '\0';
                ParseRules(s, buf, 0);
            } else {
                const char *x = ns;
                while (x < ne) {
                    const char *c = ScanTo(x, ","), *s1 = x, *s2, *y;
                    if (c > ne) c = ne;
                    s2 = c;
                    Trim(&s1, &s2);
                    if (s2 > s1) {
                        int amp = memchr(s1, '&', s2 - s1) != NULL;
                        if (o > buf) *o++ = ',';
                        if (!amp) {
                            if (list) o = Put(o, ":is(", ":is(" + 4);
                            o = Put(o, sp, se);
                            if (list) *o++ = ')';
                            *o++ = ' ';
                            o = Put(o, s1, s2);
                        } else {
                            for (y = s1; y < s2; y++) {
                                if (*y != '&') {
                                    *o++ = *y;
                                    continue;
                                }
                                if (list) o = Put(o, ":is(", ":is(" + 4);
                                o = Put(o, sp, se);
                                if (list) *o++ = ')';
                            }
                        }
                    }
                    x = c + 1;
                }
                if (o > buf) {
                    const char *brace = o;
                    o = Put(o, t, be);
                    *o = '\0';
                    ParseStyleRule(s, buf, brace);
                }
            }
            free(buf);
        }
        a = be;
    }
    s->ruleDepth--;
}

static int UrlArg(const char *v, char *out, int size);

/* @font-face の宣言 (p..end) から書体名と、使える形式の src の url() を取り出してホストに知らせる */
static void FontFace(const char *p, const char *end)
{
    char fam[64], url[1024];

    fam[0] = url[0] = '\0';
    while (p < end) {
        const char *e = ScanTo(p, ";}"), *n1 = p, *n2, *v, *ve;
        if (e > end) e = end;
        n2 = ScanTo(p, ":");
        if (n2 < e) {
            v  = n2 + 1;
            ve = e;
            Trim(&n1, &n2);
            Trim(&v, &ve);
            if (n2 - n1 == 11 && strnicmp(n1, "font-family", 11) == 0) {
                int k = 0;
                if (v < ve && (*v == '"' || *v == '\'')) v++;
                if (ve > v && (ve[-1] == '"' || ve[-1] == '\'')) ve--;
                while (v < ve && k < (int)sizeof(fam) - 1) fam[k++] = *v++;
                fam[k] = '\0';
            } else if (n2 - n1 == 3 && strnicmp(n1, "src", 3) == 0 && url[0] == '\0') {
                /* 並びのうち、format が無いか truetype / opentype のもの (woff などは読めない) */
                while (v < ve && url[0] == '\0') {
                    const char *ie = ScanTo(v, ","), *f;
                    char        item[1024];
                    int         k = (int)((ie < ve ? ie : ve) - v);
                    if (k > (int)sizeof(item) - 1) k = (int)sizeof(item) - 1;
                    memcpy(item, v, k);
                    item[k] = '\0';
                    f = strstr(item, "format(");
                    if ((f == NULL || StrIStr(f, "truetype") || StrIStr(f, "opentype"))
                        && UrlArg(item, url, sizeof(url))) {
                        if (f == NULL && (StrIStr(url, ".woff") || StrIStr(url, ".eot") || StrIStr(url, ".svg"))) {
                            url[0] = '\0';
                        }
                    }
                    v = ie < ve ? ie + 1 : ve;
                }
            }
        }
        p = e + 1;
    }
    if (fam[0] && url[0] && g_faceHook != NULL) {
        g_faceHook(fam, url);
    }
}

static const char *ParseAtRule(CssSheet *s, const char *p)
{
    char        name[32];
    int         n = 0;
    const char *pre, *q;

    p++;
    while ((isalnum((unsigned char)*p) || *p == '-') && n < 31) {
        name[n++] = (char)tolower((unsigned char)*p++);
    }
    name[n] = '\0';
    pre = p;
    q = ScanTo(p, "{;}");
    if (*q != '{') {
        if (strcmp(name, "layer") == 0 && *q == ';') {
            /* @layer a, b; : 層の順番を決める */
            const char *a = pre;
            while (a < q) {
                const char *c = ScanTo(a, ",;"), *s1 = a, *s2;
                if (c > q) c = q;
                s2 = c;
                Trim(&s1, &s2);
                if (s2 > s1) LayerRank(s, s1, s2);
                a = c < q ? c + 1 : q;
            }
        }
        return *q == ';' ? q + 1 : q;   /* @import / @charset など */
    }
    if (strcmp(name, "media") == 0) {
        return EvalMedia(s, pre, q) ? ParseRules(s, q + 1, 1) : SkipBlock(q);
    }
    if (strcmp(name, "supports") == 0) {
        /* 条件が成り立つときだけ中のルールを読む (扱えない性質は偽: 古いブラウザ向けの書式を使う) */
        const char *a = pre;
        return SupCond(s, &a, q) ? ParseRules(s, q + 1, 1) : SkipBlock(q);
    }
    if (strcmp(name, "layer") == 0) {
        /* @layer [名前] { … }: 中のルールに層の順位を付ける (層の外のルールが一番強い) */
        const char *s1 = pre, *s2 = q, *r;
        int         save = s->curLayer;
        Trim(&s1, &s2);
        s->curLayer = LayerRank(s, s2 > s1 ? s1 : NULL, s2);
        r = ParseRules(s, q + 1, 1);
        s->curLayer = save;
        return r;
    }
    if (strcmp(name, "container") == 0) {
        /* @container [名前] 条件: 中のルールに条件を付け、照合するときに基準の要素の幅で判定する */
        const char *a = SkipWs(pre), *b = a, *e;
        int         save = s->curCq;
        CQuery     *cq;
        while (b < q && (isalnum((unsigned char)*b) || *b == '-' || *b == '_')) b++;
        s->cqs = Grow(s->cqs, &s->capCqs, s->nCqs + 1, sizeof(CQuery));
        cq = &s->cqs[s->nCqs];
        cq->name = NULL;
        if (b > a && !(b - a == 3 && strnicmp(a, "not", 3) == 0)) {
            cq->name = Dup(s, a, b);    /* 名前 */
            a = b;
        }
        cq->cond = Dup(s, a, q);
        s->curCq = ++s->nCqs;
        e = ParseRules(s, q + 1, 1);
        s->curCq = save;
        return e;
    }
    if (strcmp(name, "font-face") == 0) {
        const char *e = SkipBlock(q);
        FontFace(q + 1, e - 1);
        return e;
    }
    if (strcmp(name, "keyframes") == 0 || strcmp(name, "-webkit-keyframes") == 0 || strcmp(name, "-moz-keyframes") == 0) {
        /* 最後のコマ (to か 100%) の宣言だけを覚える (animation の終わった状態に使う) */
        const char *e = SkipBlock(q), *f = q + 1, *a = pre, *b = q;
        int         d0 = -1, nd = 0, i;
        Trim(&a, &b);
        if (a < b && (*a == '"' || *a == '\'')) a++;
        if (b > a && (b[-1] == '"' || b[-1] == '\'')) b--;
        for (;;) {
            const char *fs, *fb, *s1, *s2;
            int         last = 0;
            f = SkipWs(f);
            if (f >= e - 1 || *f == '}') break;
            fs = f;
            fb = ScanTo(f, "{}");
            if (*fb != '{' || fb >= e) break;
            for (s1 = fs; s1 < fb; s1 = s2 + (s2 < fb)) {
                s2 = ScanTo(s1, ",{");
                if (s2 > fb) s2 = fb;
                {
                    const char *t1 = s1, *t2 = s2;
                    Trim(&t1, &t2);
                    if ((t2 - t1 == 2 && strnicmp(t1, "to", 2) == 0) || (t2 - t1 == 4 && strncmp(t1, "100%", 4) == 0)) {
                        last = 1;
                    }
                }
            }
            if (last) {
                SheetCtx sc;
                sc.s      = s;
                sc.global = 0;
                d0 = s->nDecls;
                ParseDeclText(fb + 1, SkipBlock(fb) - 1, SheetDecl, &sc);
                nd = s->nDecls - d0;
            }
            f = SkipBlock(fb);
        }
        if (d0 >= 0 && b > a) {
            for (i = 0; i < s->nKfs && !(strlen(s->kfs[i].name) == (size_t)(b - a)
                                         && strncmp(s->kfs[i].name, a, b - a) == 0); i++) ;
            if (i == s->nKfs) {
                s->kfs = Grow(s->kfs, &s->capKfs, s->nKfs + 1, sizeof(Keyframes));
                s->kfs[i].name = Dup(s, a, b);
                s->nKfs++;
            }
            s->kfs[i].d0 = d0;
            s->kfs[i].nd = nd;
        }
        return e;
    }
    return SkipBlock(q);                /* @page など */
}

static const char *ParseRulesBody(CssSheet *s, const char *p, int nested)
{
    for (;;) {
        const char *q;
        p = SkipWs(p);
        if (*p == '\0') {
            return p;
        }
        if (*p == '}') {
            if (nested) {
                return p + 1;
            }
            p++;
            continue;
        }
        if (*p == '@') {
            p = ParseAtRule(s, p);
            continue;
        }
        if (*p == ';') {
            /* ルールの外の ";" は次のルールのセレクタの一部になり、そのルールは無効になる */
            q = ScanTo(p + 1, "{}");
            p = *q == '{' ? SkipBlock(q) : q;
            continue;
        }
        if ((p[0] == '<' && p[1] == '!') || (p[0] == '-' && p[1] == '-' && p[2] == '>')) {
            p++;                        /* <!-- --> で囲んだ古い書き方 */
            continue;
        }
        q = ScanTo(p, "{};");
        if (*q != '{') {
            p = *q ? q + 1 : q;
            continue;
        }
        p = ParseStyleRule(s, p, q);
    }
}

/* 規則の並びを読む。nested なら p は '{' の次。深すぎる入れ子は読まずに飛ばす */
static const char *ParseRules(CssSheet *s, const char *p, int nested)
{
    const char *e;

    if (nested && s->ruleDepth >= NEST_DEPTH_MAX) {
        return SkipBlock(p - 1);
    }
    s->ruleDepth++;
    e = ParseRulesBody(s, p, nested);
    s->ruleDepth--;
    return e;
}

/* ---------------------------------------------------------------- 公開関数 */

CssSheet *CssNew(const CssMedia *m)
{
    CssSheet *s = CssXmalloc(sizeof(CssSheet));
    memset(s, 0, sizeof(*s));
    s->m = *m;
    return s;
}

void CssFree(CssSheet *s)
{
    int  i;
    Blk *b, *nb;

    if (s == NULL) {
        return;
    }
    for (i = 0; i < HASH_N; i++) {
        Bucket *k;
        for (k = s->hash[i]; k != NULL; k = k->next) {
            free(k->ids);
        }
    }
    for (b = s->blk; b != NULL; b = nb) {
        nb = b->next;
        free(b);
    }
    free(s->rules);
    free(s->decls);
    free(s->univ);
    free(s->vars);
    free(s->layers);
    free(s);
}

int CssRuleCount(const CssSheet *s)
{
    return s ? s->nRules : 0;
}

int CssUsesChecked(const CssSheet *s)
{
    return s ? s->usesChecked : 0;
}

int CssViewUsed(int reset)
{
    int u = g_viewUse;
    if (reset) {
        g_viewUse = 0;
    }
    return u;
}

int CssUsesState(const CssSheet *s)
{
    return s ? s->usesState : 0;
}

static const char *LookupVar(const CssSheet *s, const char *name);
static int MatchedRules(const CssSheet *s, const CssElem *chain, int n, int *matched);

int CssUsesPe(const CssSheet *s, int pe)
{
    return s != NULL && (s->usesPe & (1 << pe)) != 0;
}

static int ExpandVars(const CssSheet *s, const char *v, char *out, int size, int depth);

static void StrCopy0(char *d, const char *s, int size)
{
    int n = (int)strlen(s);
    if (n >= size) {
        n = size - 1;
    }
    memcpy(d, s, n);
    d[n] = '\0';
}

typedef struct {
    const char *name;
    int         id;
    char        val[2][VALBUF];         /* style 属性の値 ([imp]) */
    BYTE        has[2];
} CascCtx;

static void CascInline(void *ctx, const char *name, const char *vs, const char *ve, int imp)
{
    CascCtx *cc = ctx;
    int      n  = (int)(ve - vs);
    if (cc->id != P_NONE ? PropId(name) != cc->id : strcmp(name, cc->name) != 0) {
        return;
    }
    if (n >= VALBUF) {
        n = VALBUF - 1;
    }
    memcpy(cc->val[imp], vs, n);
    cc->val[imp][n] = '\0';
    cc->has[imp] = 1;
}

int CssCascaded(const CssSheet *s, const CssElem *chain, int n, const char *inl, const char *name,
                char *out, int size)
{
    int         matched[MAX_MATCH], nm = 0, i, j, imp;
    const char *best = NULL;
    CascCtx    *cc;

    g_varChain = chain;
    g_varN     = n;
    g_varInl   = inl;
    if (name[0] == '-' && name[1] == '-') {
        /* カスタムプロパティ (style 属性と、:root などで定義したもの) */
        best = s != NULL ? LookupVar(s, name) : NULL;
        g_varChain = NULL;
        g_varInl   = NULL;
        if (best == NULL) {
            return 0;
        }
        StrCopy0(out, best, size);
        return 1;
    }
    cc = CssXmalloc(sizeof(CascCtx));
    memset(cc, 0, sizeof(*cc));
    cc->name = name;
    cc->id   = PropId(name);
    if (s != NULL && n > 0) {
        nm = MatchedRules(s, chain, n, matched);
    }
    if (inl != NULL && *inl) {
        ParseDeclText(inl, inl + strlen(inl), CascInline, cc);
    }
    for (imp = 0; imp < 2; imp++) {
        for (i = 0; i < nm; i++) {
            const Rule *r = &s->rules[matched[i]];
            for (j = 0; j < r->nd; j++) {
                const Decl *d = &s->decls[r->d0 + j];
                if (d->imp == imp && (cc->id != P_NONE ? d->prop == cc->id
                                      : d->prop == P_OTHER && strcmp(d->name, name) == 0)) {
                    best = d->val;
                }
            }
        }
        if (cc->has[imp]) {
            best = cc->val[imp];
        }
    }
    if (best != NULL && strstr(best, "var(") != NULL && s != NULL) {
        if (!ExpandVars(s, best, out, size, 0)) {
            best = NULL;
        } else {
            best = out;
        }
    }
    if (best != NULL && best != out) {
        StrCopy0(out, best, size);
    }
    free(cc);
    g_varChain = NULL;
    g_varInl   = NULL;
    return best != NULL;
}

/* セレクタの一覧 ("a, div > p.x") を読む。対応していないセレクタがあれば NULL */
CssSheet *CssSelCompile(const char *text)
{
    CssMedia    m;
    CssSheet   *s;
    const char *a = text, *end = text + strlen(text);

    memset(&m, 0, sizeof(m));
    s = CssNew(&m);
    while (a < end) {
        const char *c = ScanTo(a, ","), *s1 = a, *s2;
        Rule        r;
        if (c > end) c = end;
        s2 = c;
        Trim(&s1, &s2);
        memset(&r, 0, sizeof(r));
        if (s1 >= s2 || !CompileSelector(s, s1, s2, &r, 0) || r.parts[r.n - 1].pe) {
            CssFree(s);                 /* スクリプトの照合では疑似要素は扱わない */
            return NULL;
        }
        s->rules = Grow(s->rules, &s->capRules, s->nRules + 1, sizeof(Rule));
        s->rules[s->nRules++] = r;
        a = c + 1;
    }
    if (s->nRules == 0) {
        CssFree(s);
        return NULL;
    }
    return s;
}

static int MatchRule(const Rule *r, const CssElem *chain, int n);

/* chain[0] = 要素, chain[1] = 親, ... がセレクタの一覧のどれかに合うか */
int CssSelMatch(const CssSheet *s, const CssElem *chain, int n)
{
    int i;
    for (i = 0; i < s->nRules; i++) {
        if (MatchRule(&s->rules[i], chain, n)) {
            return 1;
        }
    }
    return 0;
}

void CssParse(CssSheet *s, const char *text, long len)
{
    char *buf = CssXmalloc(len + 1);
    long  i, o = 0;
    char  q = 0;

    /* コメントを取り除く (文字列の中は残す) */
    for (i = 0; i < len; i++) {
        char c = text[i];
        if (c == '\0') {
            c = ' ';
        }
        if (q) {
            if (c == '\\' && i + 1 < len) {
                buf[o++] = c;
                c = text[++i];
            } else if (c == q || c == '\n') {
                q = 0;
            }
            buf[o++] = c;
            continue;
        }
        if (c == '"' || c == '\'') {
            q = c;
        } else if (c == '/' && i + 1 < len && text[i + 1] == '*') {
            i += 2;
            while (i + 1 < len && !(text[i] == '*' && text[i + 1] == '/')) i++;
            i++;
            buf[o++] = ' ';
            continue;
        }
        buf[o++] = c;
    }
    buf[o] = '\0';
    ParseRules(s, buf, 0);
    free(buf);
}

/* ---------------------------------------------------------------- 照合 */

static int HasWord(const char *list, const char *w)
{
    size_t      n = strlen(w);
    const char *p = list, *q;

    if (list == NULL) {
        return 0;
    }
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;
        q = p;
        while (*q && !isspace((unsigned char)*q)) q++;
        if ((size_t)(q - p) == n && memcmp(p, w, n) == 0) {
            return 1;
        }
        p = q;
    }
    return 0;
}

static int MatchAttr(const AttrCond *a, const CssElem *e)
{
    const char *v;
    size_t      n, m;

    if (strcmp(a->name, "id") == 0) v = e->id;
    else if (strcmp(a->name, "class") == 0) v = e->cls;
    else v = e->attr ? e->attr(e->ctx, a->name) : NULL;
    if (v == NULL) return 0;
    if (a->op == 0) return 1;
    n = strlen(v);
    m = strlen(a->val);
    switch (a->op) {
    case '=': return (a->icase ? stricmp(v, a->val) : strcmp(v, a->val)) == 0;
    case '~': return HasWord(v, a->val);
    case '^': return m > 0 && strncmp(v, a->val, m) == 0;
    case '$': return m > 0 && n >= m && strcmp(v + n - m, a->val) == 0;
    case '*': return m > 0 && strstr(v, a->val) != NULL;
    case '|': return strncmp(v, a->val, m) == 0 && (v[m] == '\0' || v[m] == '-');
    }
    return 0;
}

/* 兄弟の中での位置 (1 から)。last なら後ろから数える。type なら同じ要素名だけ数える */
static int SibIndex(const CssElem *e, int last, int type)
{
    const void *p = e->node;
    CssElem     o;
    int         k = 1;

    for (;;) {
        p = last ? g_cssDom.next(p) : g_cssDom.prev(p);
        if (p == NULL) {
            return k;
        }
        if (type) {
            g_cssDom.fill(p, &o);
            if (strcmp(o.tag, e->tag) != 0) {
                continue;
            }
        }
        k++;
    }
}

static int MatchFrom(const Rule *r, int k, const CssElem *e, const CssElem *chain, int ei, int n);

/* e が :nth-child(… of S) の S のどれかに合うか (祖先は分からないものとして照合する) */
static int SelListMatch(const PseudoCond *c, const CssElem *e)
{
    int i;
    for (i = 0; i < c->nSel; i++) {
        if (MatchFrom(&c->sel[i], c->sel[i].n - 1, e, e, 0, 1) > 0) {
            return 1;
        }
    }
    return 0;
}

/* :nth-child(an+b of S): e が S に合えば、S に合う兄弟の中での位置 (1 から)。合わなければ 0 */
static int SibIndexOf(const CssElem *e, const PseudoCond *c, int last)
{
    const void *p = e->node;
    CssElem     o;
    int         k = 1;

    if (!SelListMatch(c, e)) {
        return 0;
    }
    for (;;) {
        p = last ? g_cssDom.next(p) : g_cssDom.prev(p);
        if (p == NULL) {
            return k;
        }
        g_cssDom.fill(p, &o);
        if (SelListMatch(c, &o)) {
            k++;
        }
    }
}

/* 照合している複合セレクタの祖先の情報 (:is() などの中の子孫・子の結合子に使う。NULL = 分からない) */
static const CssElem *g_mChain;
static int            g_mEi, g_mN;
static const void    *g_hasAnchor;      /* :has() の基準の要素 */

#define HAS_DEPTH 64

/* :has() の中のセレクタが node (基準からの深さ d) かその子孫に合うか。st は祖先の並び (後ろから使う) */
static int HasVisit(const PseudoCond *c, const void *node, CssElem *st, int d, int deep)
{
    CssElem    *x = &st[HAS_DEPTH - 1 - d];
    const void *k;
    int         i;

    g_cssDom.fill(node, x);
    for (i = 0; i < c->nSel; i++) {
        if (MatchFrom(&c->sel[i], c->sel[i].n - 1, x, x, 0, d + 1) > 0) {
            return 1;
        }
    }
    if (!deep || d + 1 >= HAS_DEPTH) {
        return 0;
    }
    for (k = g_cssDom.first(node); k != NULL; k = g_cssDom.next(k)) {
        if (HasVisit(c, k, st, d + 1, deep)) {
            return 1;
        }
    }
    return 0;
}

/* :has(): e の子孫 (> と空白) か、後の兄弟とその子孫 (+ と ~) に合うものがあるか */
static int MatchHas(const PseudoCond *c, const CssElem *e)
{
    CssElem     st[HAS_DEPTH];
    const void *save = g_hasAnchor, *k;
    int         i, down = 0, side = 0, deep = 0, hit = 0;

    if (e->node == NULL || g_cssDom.first == NULL) {
        return 0;
    }
    for (i = 0; i < c->nSel; i++) {
        BYTE comb = c->sel[i].parts[1].comb;
        if (comb == '+' || comb == '~') side = 1; else down = 1;
        if (comb == ' ' || c->sel[i].n > 2) deep = 1;   /* 孫より下も調べる */
    }
    g_hasAnchor = e->node;
    if (down) {
        st[HAS_DEPTH - 1] = *e;
        for (k = g_cssDom.first(e->node); k != NULL && !hit; k = g_cssDom.next(k)) {
            hit = HasVisit(c, k, st, 1, deep);
        }
    }
    if (side) {
        for (k = g_cssDom.next(e->node); k != NULL && !hit; k = g_cssDom.next(k)) {
            hit = HasVisit(c, k, st, 0, deep);
        }
    }
    g_hasAnchor = save;
    return hit;
}

static int MatchPseudo(const PseudoCond *c, const CssElem *e)
{
    int i;

    if (c->kind == PS_IS || c->kind == PS_NOT) {
        /* 祖先が分からなければ e だけで照合する */
        for (i = 0; i < c->nSel; i++) {
            const Rule *r = &c->sel[i];
            if ((g_mChain != NULL ? MatchFrom(r, r->n - 1, e, g_mChain, g_mEi, g_mN)
                                  : MatchFrom(r, r->n - 1, e, e, 0, 1)) > 0) {
                return c->kind == PS_IS;
            }
        }
        return c->kind == PS_NOT;
    }
    if (c->kind == PS_HAS) {
        return MatchHas(c, e);
    }
    if (e->node == NULL || g_cssDom.prev == NULL) {
        return 0;                       /* 木の要素が分からない */
    }
    if (c->kind <= PS_ONLY_TYPE && g_cssDom.state(e->node, CSS_Q_ORPHAN)) {
        return 0;                       /* 親の無い要素は兄弟の位置の疑似クラスに合わない */
    }
    switch (c->kind) {
    case PS_NTH: case PS_NTH_LAST: case PS_NTH_TYPE: case PS_NTH_LAST_TYPE:
        if (c->nSel > 0) {
            i = SibIndexOf(e, c, c->kind == PS_NTH_LAST);
            if (i == 0) return 0;
        } else {
            i = SibIndex(e, c->kind == PS_NTH_LAST || c->kind == PS_NTH_LAST_TYPE,
                         c->kind == PS_NTH_TYPE || c->kind == PS_NTH_LAST_TYPE);
        }
        if (c->a == 0) {
            return i == c->b;
        }
        return (i - c->b) / c->a >= 0 && (i - c->b) % c->a == 0;
    case PS_ONLY:
        return g_cssDom.prev(e->node) == NULL && g_cssDom.next(e->node) == NULL;
    case PS_ONLY_TYPE:
        return SibIndex(e, 0, 1) == 1 && SibIndex(e, 1, 1) == 1;
    case PS_EMPTY:
        return g_cssDom.state(e->node, CSS_Q_EMPTY);
    case PS_CHECKED:
        return g_cssDom.state(e->node, CSS_Q_CHECKED);
    case PS_DISABLED:
        return g_cssDom.state(e->node, CSS_Q_DISABLED) == 1;
    case PS_ENABLED:
        return g_cssDom.state(e->node, CSS_Q_DISABLED) == 0;
    case PS_FOCUS:
        return (g_forceState & CSS_U_FOCUS) || g_cssDom.state(e->node, CSS_Q_FOCUS);
    case PS_FOCUS_WITHIN:
        return (g_forceState & CSS_U_FOCUS) || g_cssDom.state(e->node, CSS_Q_FOCUS_WITHIN);
    case PS_ACTIVE:
        return g_cssDom.state(e->node, CSS_Q_ACTIVE);
    case PS_HOVER:
        return (g_forceState & CSS_U_HOVER) || g_cssDom.state(e->node, CSS_Q_HOVER);
    case PS_TARGET:
        return (g_forceState & CSS_U_TARGET) || g_cssDom.state(e->node, CSS_Q_TARGET);
    case PS_REQUIRED:
        return g_cssDom.state(e->node, CSS_Q_REQUIRED) == 1;
    case PS_OPTIONAL:
        return g_cssDom.state(e->node, CSS_Q_REQUIRED) == 0;
    case PS_READONLY:
        return g_cssDom.state(e->node, CSS_Q_READWRITE) == 0;
    case PS_READWRITE:
        return g_cssDom.state(e->node, CSS_Q_READWRITE) == 1;
    case PS_PHSHOWN:
        return g_cssDom.state(e->node, CSS_Q_PHSHOWN);
    case PS_VALID:
        return g_cssDom.state(e->node, CSS_Q_VALID) == 1;
    case PS_INVALID:
        return g_cssDom.state(e->node, CSS_Q_VALID) == 0;
    case PS_LANG: {
        const char *l = g_cssDom.lang != NULL ? g_cssDom.lang(e->node) : NULL;
        size_t      n = strlen(c->str);
        return l != NULL && strnicmp(l, c->str, n) == 0 && (l[n] == '\0' || l[n] == '-');
    }
    }
    return 0;
}

static int MatchCompound(const Compound *c, const CssElem *e)
{
    int i;
    if (c->anchor) return e->node != NULL && e->node == g_hasAnchor;
    if (c->tag && strcmp(c->tag, e->tag) != 0) return 0;
    if (c->id && (e->id == NULL || strcmp(c->id, e->id) != 0)) return 0;
    for (i = 0; i < c->nCls; i++) {
        if (!HasWord(e->cls, c->cls[i])) return 0;
    }
    for (i = 0; i < c->nAttrs; i++) {
        if (!MatchAttr(&c->attrs[i], e)) return 0;
    }
    if (c->link == 3 ? !e->isLink : c->link && e->isLink != c->link) return 0;
    if (c->root && !e->isRoot) return 0;
    for (i = 0; i < c->nPs; i++) {
        if (!MatchPseudo(&c->ps[i], e)) return 0;
    }
    return 1;
}

/* parts[k] が e に合い、その左も合うか。e の祖先は chain[ei + 1] から (e は chain[ei] かその兄弟)。
   1 = 合う、0 = 合わない、-1 = もっと上の祖先から始めても合わない (子孫の探し直しを打ち切る) */
static int MatchFrom(const Rule *r, int k, const CssElem *e, const CssElem *chain, int ei, int n)
{
    BYTE comb;
    int  i, m;

    if (r->parts[k].nested) {
        /* :is() などの中で e の祖先を使えるようにする */
        const CssElem *sc = g_mChain;
        int            se = g_mEi, sn = g_mN;
        g_mChain = chain;
        g_mEi    = ei;
        g_mN     = n;
        m = MatchCompound(&r->parts[k], e);
        g_mChain = sc;
        g_mEi    = se;
        g_mN     = sn;
        if (!m) {
            return 0;
        }
    } else if (!MatchCompound(&r->parts[k], e)) {
        return 0;
    }
    if (k == 0) {
        return 1;
    }
    comb = r->parts[k].comb;
    if (comb == '>') {
        return ei + 1 < n ? MatchFrom(r, k - 1, &chain[ei + 1], chain, ei + 1, n) : -1;
    }
    if (comb == '+' || comb == '~') {
        const void *p;
        CssElem     sib;
        if (e->node == NULL || g_cssDom.prev == NULL) {
            return 0;
        }
        for (p = g_cssDom.prev(e->node); p != NULL; p = g_cssDom.prev(p)) {
            g_cssDom.fill(p, &sib);
            if ((m = MatchFrom(r, k - 1, &sib, chain, ei, n)) != 0) {
                return m;
            }
            if (comb == '+') {
                break;
            }
        }
        return 0;
    }
    for (i = ei + 1; i < n; i++) {
        if ((m = MatchFrom(r, k - 1, &chain[i], chain, i, n)) != 0) {
            return m;
        }
    }
    return -1;
}

static int MatchRule(const Rule *r, const CssElem *chain, int n)
{
    return MatchFrom(r, r->n - 1, &chain[0], chain, 0, n) > 0;
}

/* 疑似クラスの種類 → CSS_U_* (状態で変わらないものは 0) */
static int StateBit(int kind)
{
    switch (kind) {
    case PS_HOVER: return CSS_U_HOVER;
    case PS_FOCUS: case PS_FOCUS_WITHIN: return CSS_U_FOCUS;
    case PS_TARGET: return CSS_U_TARGET;
    case PS_ACTIVE: return CSS_U_ACTIVE;
    }
    return 0;
}

/* 複合セレクタ c が使う状態の疑似クラス (CSS_U_*。:is() などの中も)。:not() の中にあれば *neg を 1 にする */
static int CompoundState(const Compound *c, int *neg)
{
    int i, j, k, u = 0;
    for (i = 0; i < c->nPs; i++) {
        const PseudoCond *p = &c->ps[i];
        u |= StateBit(p->kind);
        for (j = 0; p->kind >= PS_IS && j < p->nSel; j++) {     /* セレクタの並びを持つものだけ */
            for (k = 0; k < p->sel[j].n; k++) {
                int v = CompoundState(&p->sel[j].parts[k], neg);
                if (v && p->kind == PS_NOT) *neg = 1;
                u |= v;
            }
        }
    }
    return u;
}

int CssStateCould(const CssSheet *s, const CssElem *e, int u)
{
    int i, j, hit = 0;

    if (s == NULL || !(s->usesState & u)) {
        return 0;
    }
    g_forceState = u;
    for (i = 0; i < s->nRules && !hit; i++) {
        const Rule *r = &s->rules[i];
        for (j = 0; j < r->n && !hit; j++) {
            const Compound *c = &r->parts[j];
            int             neg = 0;
            if (CompoundState(c, &neg) & u) {
                /* :not(:hover) などは状態を合うものとして調べられないので、変わりうるとする */
                hit = neg || MatchCompound(c, e);
            }
        }
    }
    g_forceState = 0;
    return hit;
}

/* @container の条件 (cq は CssSheet.cqs の番号 + 1) が要素 chain[0] で成り立つか。基準は一番近い container-type の
   祖先で、幅はその width の指定 (分からなければ画面の幅)。祖先の書式が分からなければ画面の幅で判定する */
static int CqMatch(const CssSheet *s, int cq, const CssElem *chain, int n)
{
    const CQuery *q = &s->cqs[cq - 1];
    CssMedia      m = s->m;
    int           i;

    for (i = 1; i < n; i++) {
        const CssStyle *st = chain[i].st;
        if (st == NULL) {
            break;
        }
        if (st->containerType && (q->name == NULL || (st->containerName != NULL && strcmp(st->containerName, q->name) == 0))) {
            if (st->width != CSS_UNSET && !st->widthPct) {
                m.viewW = st->width - (st->borderBox ? st->padding[CSS_LEFT] + st->padding[CSS_RIGHT]
                                                       + CssBorder(st, CSS_LEFT) + CssBorder(st, CSS_RIGHT) : 0);
            }
            break;
        }
    }
    if (i == n && n > 0) {
        return 0;                       /* 基準になる祖先が無い */
    }
    return CqEval(&m, q->cond);
}

static void AddCandidates(const CssSheet *s, const int *ids, int n, const CssElem *chain, int depth,
                          int *out, int *nOut)
{
    int i;
    for (i = 0; i < n && *nOut < MAX_MATCH; i++) {
        const Rule *r = &s->rules[ids[i]];
        if (r->parts[r->n - 1].pe == g_wantPe && MatchRule(r, chain, depth) && (r->cq == 0 || CqMatch(s, r->cq, chain, depth))) {
            out[(*nOut)++] = ids[i];
        }
    }
}

/* ---------------------------------------------------------------- 適用 */

/* style 属性の文字列 st から、カスタムプロパティ name の値を out に取り出す。無ければ 0 */
static int InlineVar(const char *st, const char *name, char *out, int size)
{
    size_t      len = strlen(name);
    const char *p = st;

    while ((p = strstr(p, name)) != NULL) {
        const char *q = SkipWs(p + len), *e;
        int         head = p == st || p[-1] == ';' || isspace((unsigned char)p[-1]);
        if (head && *q == ':') {
            int n;
            q = SkipWs(q + 1);
            e = ScanTo(q, ";");
            while (e > q && isspace((unsigned char)e[-1])) e--;
            n = (int)(e - q) < size - 1 ? (int)(e - q) : size - 1;
            memcpy(out, q, n);
            out[n] = '\0';
            return 1;
        }
        p += len;
    }
    return 0;
}

static const char *LookupVar(const CssSheet *s, const char *name)
{
    static char ring[8][VALBUF];
    static int  next;
    int         i;
    const struct CssVarNode *v;
    for (v = g_curVars; v != NULL; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            return v->val;              /* 要素と祖先のルール・style 属性で定義したもの */
        }
    }
    if (g_varChain != NULL || g_varInl != NULL) {
        /* 要素とその祖先の style 属性に書いたもの (近いものが勝つ) */
        char *buf = ring[next];
        next = (next + 1) % 8;
        for (i = 0; i < (g_varChain != NULL ? g_varN : 1); i++) {
            const char *st = i == 0 && g_varInl != NULL ? g_varInl
                           : g_varChain != NULL && g_varChain[i].attr != NULL ? g_varChain[i].attr(g_varChain[i].ctx, "style")
                           : NULL;
            if (st != NULL && strstr(st, "--") != NULL && InlineVar(st, name, buf, VALBUF)) {
                return buf;
            }
        }
    }
    for (i = s->nVars - 1; i >= 0; i--) {
        if (strcmp(s->vars[i].name, name) == 0) {
            return s->vars[i].val;
        }
    }
    return NULL;
}

/* var(--x, 既定値) を展開する。展開できなければ 0 */
static int ExpandVars(const CssSheet *s, const char *v, char *out, int size, int depth)
{
    int o = 0;

    while (*v) {
        if (strnicmp(v, "var(", 4) == 0) {
            const char *e  = SkipBlock(v + 3);
            const char *a  = SkipWs(v + 4), *c, *val;
            char        name[64], fb[VALBUF], tmp[VALBUF];
            int         n = 0;
            c = ScanTo(a, ",)");
            while (a < c && !isspace((unsigned char)*a) && n < 63) name[n++] = *a++;
            name[n] = '\0';
            val = LookupVar(s, name);
            if (val != NULL && strstr(val, "var(") != NULL) {
                /* 値の中の var() を展開する。循環 (--x: var(--x, 1rem) など) で展開できなければ無効として既定値を使う */
                if (depth >= VAR_DEPTH || !ExpandVars(s, val, tmp, sizeof(tmp), depth + 1)) {
                    val = NULL;
                } else {
                    val = tmp;
                }
            }
            if (val == NULL && *c == ',') {
                const char *f1 = c + 1, *f2 = e - 1;
                int         k;
                Trim(&f1, &f2);
                k = (int)(f2 - f1) < VALBUF - 1 ? (int)(f2 - f1) : VALBUF - 1;
                memcpy(fb, f1, k);
                fb[k] = '\0';
                val = fb;
                if (strstr(val, "var(") != NULL) {
                    if (depth >= VAR_DEPTH || !ExpandVars(s, fb, tmp, sizeof(tmp), depth + 1)) {
                        return 0;
                    }
                    val = tmp;
                }
            }
            if (val == NULL) {
                return 0;
            }
            while (*val && o < size - 1) out[o++] = *val++;
            v = e;
            continue;
        }
        if (o < size - 1) out[o++] = *v;
        v++;
    }
    out[o] = '\0';
    return 1;
}

static int IsWord(const char *v, const char *w)
{
    return stricmp(v, w) == 0;
}

static int Family(const char *v)
{
    char        buf[256];
    const char *p;
    int         n = 0;

    while (*v && n < 255) buf[n++] = (char)tolower((unsigned char)*v++);
    buf[n] = '\0';
    for (p = buf; *p; ) {
        char        f[64];
        const char *e = strchr(p, ',');
        int         k = 0;
        if (e == NULL) e = p + strlen(p);
        while (p < e) {
            if (*p != '"' && *p != '\'' && (k > 0 || !isspace((unsigned char)*p)) && k < 63) f[k++] = *p;
            p++;
        }
        while (k > 0 && isspace((unsigned char)f[k - 1])) k--;
        f[k] = '\0';
        if (strstr(f, "mono") || strstr(f, "courier") || strstr(f, "consolas") || strstr(f, "menlo")
            || strstr(f, "monaco") || strstr(f, "code")) {
            return CSS_F_MONO;
        }
        if (strstr(f, "sans") || strstr(f, "arial") || strstr(f, "helvetica") || strstr(f, "gothic")
            || strstr(f, "meiryo") || strstr(f, "verdana") || strstr(f, "segoe") || strstr(f, "system")
            || strstr(f, "roboto") || strstr(f, "noto") || strstr(f, "hiragino")) {
            return CSS_F_SANS;
        }
        if (strcmp(f, "serif") == 0 || strstr(f, "times") || strstr(f, "georgia") || strstr(f, "mincho")
            || strstr(f, "garamond") || strstr(f, "palatino") || strstr(f, "serif")) {
            return CSS_F_SERIF;
        }
        if (*e == ',') e++;
        p = e;
    }
    return CSS_F_SANS;
}

static int FontSizeKeyword(const char *v, int base, int parent, int *px)
{
    static const struct { const char *k; int pct; } kw[] = {
        { "xx-small", 60 }, { "x-small", 75 }, { "small", 89 }, { "medium", 100 },
        { "large", 120 }, { "x-large", 150 }, { "xx-large", 200 }, { "xxx-large", 300 }
    };
    int i;
    for (i = 0; i < (int)(sizeof(kw) / sizeof(kw[0])); i++) {
        if (IsWord(v, kw[i].k)) {
            *px = base * kw[i].pct / 100;
            return 1;
        }
    }
    if (IsWord(v, "smaller")) { *px = parent * 5 / 6; return 1; }
    if (IsWord(v, "larger")) { *px = parent * 6 / 5; return 1; }
    return 0;
}

/* 長さの値が絶対の単位 (px・pt など、単位の無いものも) か */
static int AbsLen(const char *v)
{
    char u[8];
    int  n = 0, ok;

    v = SkipWs(v);
    if (isalpha((unsigned char)*v)) {
        return 0;                       /* calc() など */
    }
    (void)Num(&v, &ok);
    if (!ok) {
        return 0;
    }
    while (isalpha((unsigned char)*v) && n < 7) {
        u[n++] = (char)tolower((unsigned char)*v++);
    }
    u[n] = '\0';
    return n == 0 || strcmp(u, "px") == 0 || strcmp(u, "pt") == 0 || strcmp(u, "pc") == 0
           || strcmp(u, "in") == 0 || strcmp(u, "cm") == 0 || strcmp(u, "mm") == 0;
}

/* 文字の拡大・縮小: 絶対の単位で指定した文字の大きさに掛ける倍率 (%) */
static int g_zoomAbs = 100;

void CssSetZoom(int pct, int pxToo)
{
    g_zoomAbs = pxToo ? pct : 100;
}

static int ZoomAbs(int px)
{
    return (int)(((long)px * g_zoomAbs + 50) / 100);
}

static int SetFontSize(const CssSheet *s, const char *v, const CssStyle *parent, CssStyle *st)
{
    LenCtx c;
    int    px, k;

    if (!FontSizeKeyword(v, s->m.basePx, parent->fontPx, &px)) {
        c.em  = parent->fontPx;
        c.rem = parent->remPx;
        c.vw  = s->m.viewW;
        c.vh  = s->m.viewH;
        c.cq  = 0;
        c.strict = s->m.strict;
        k = ParseLen(v, &c, &px);
        if (k == 2) {
            px = parent->fontPx * px / 100;
        } else if (k != 1) {
            return 0;
        } else if (AbsLen(v)) {
            px = ZoomAbs(px);
        }
    }
    if (px <= 0) px = 0;                /* 0 は文字の無い行の高さを 0 にする手法で使うので残す */
    else if (px < 6) px = 6;
    if (px > 400) px = 400;
    st->fontPx = (short)px;
    return 1;
}

static void SetWeight(const char *v, CssStyle *st)
{
    if (IsWord(v, "bold") || IsWord(v, "bolder") || atoi(v) >= 600) {
        st->bold = 1;
    } else if (IsWord(v, "normal") || IsWord(v, "lighter") || (atoi(v) > 0 && atoi(v) < 600)) {
        st->bold = 0;
    }
}

/* ---- CSS1 の残り (背景画像・リスト・行の高さ・縦の位置・書体名) の値の解析 */

/* "url(...)" の中身 (無ければ 0) */
static int UrlArg(const char *v, char *out, int size)
{
    const char *p = v;
    char        q = 0;
    int         n = 0;

    while (*p && strnicmp(p, "url(", 4) != 0) {
        p++;
    }
    if (*p == '\0') {
        return 0;
    }
    p = SkipWs(p + 4);
    if (*p == '"' || *p == '\'') {
        q = *p++;
    }
    while (*p && (q ? *p != q : (*p != ')' && !isspace((unsigned char)*p))) && n < size - 1) {
        out[n++] = *p++;
    }
    out[n] = '\0';
    return n > 0;
}

/* url() の画像を登録して、番号 + 1 を返す (none などは 0) */
static int UrlImage(const char *v)
{
    char   url[1024];
    size_t n = strlen(v);
    if (g_urlHook == NULL) {
        return 0;
    }
    if (n >= sizeof(url)) {
        /* 長い値 (CSS に埋め込んだ data: の画像など): 値の長さだけ確保する */
        char *buf = CssXmalloc(n + 1);
        int   r   = UrlArg(v, buf, (int)n + 1) ? g_urlHook(buf) : 0;
        free(buf);
        return r;
    }
    if (!UrlArg(v, url, sizeof(url))) {
        return 0;
    }
    return g_urlHook(url);
}

/* font-family の一覧から、この PC で使える最初の書体名 (総称の書体名が先に来たらそれまで)。
   それが英字だけの書体なら、その後の表示の文字コードの文字を出せる書体名を out2 に入れる */
static void FaceOf(const char *v, char *out, char *out2, int size)
{
    static const char *const generic[] = {
        "serif", "sans-serif", "monospace", "cursive", "fantasy", "system-ui", "ui-sans-serif", "ui-serif",
        "ui-monospace", "ui-rounded", "-apple-system", "blinkmacsystemfont", "emoji", "math", "inherit",
        "initial", "unset", NULL
    };
    const char *p = v;

    out[0]  = '\0';
    out2[0] = '\0';
    if (g_fontHook == NULL) {
        return;
    }
    while (*p) {
        const char *e = ScanTo(p, ","), *s1 = p, *s2;
        char        f[64];
        int         k = 0, i;
        s2 = e;
        Trim(&s1, &s2);
        if (s1 < s2 && (*s1 == '"' || *s1 == '\'')) s1++;
        if (s2 > s1 && (s2[-1] == '"' || s2[-1] == '\'')) s2--;
        while (s1 < s2 && k < (int)sizeof(f) - 1) f[k++] = *s1++;
        f[k] = '\0';
        for (i = 0; generic[i] != NULL && stricmp(f, generic[i]) != 0; i++) ;
        if (generic[i] != NULL) {
            return;                     /* 総称の書体名: それまでに使えるものが無ければ総称を使う */
        }
        if (f[0]) {
            int r = g_fontHook(f);
            if (r == 1) {
                StrCopy0(out[0] ? out2 : out, f, size);
                return;
            }
            if (r == 2 && out[0] == '\0') {
                StrCopy0(out, f, size);     /* 英字だけの書体: 続けて日本語などを出せる書体を探す */
            }
        }
        if (*e != ',') {
            break;
        }
        p = e + 1;
    }
}

static int ListTypeOf(const char *w)
{
    static const struct { const char *name; BYTE type; } t[] = {
        { "disc", CSS_L_DISC }, { "circle", CSS_L_CIRCLE }, { "square", CSS_L_SQUARE },
        { "decimal", CSS_L_DECIMAL }, { "decimal-leading-zero", CSS_L_DECIMAL0 },
        { "lower-roman", CSS_L_LROMAN }, { "upper-roman", CSS_L_UROMAN },
        { "lower-alpha", CSS_L_LALPHA }, { "lower-latin", CSS_L_LALPHA },
        { "upper-alpha", CSS_L_UALPHA }, { "upper-latin", CSS_L_UALPHA }, { "lower-greek", CSS_L_LGREEK },
        { "hiragana", CSS_L_HIRAGANA }, { "katakana", CSS_L_KATAKANA },
        { "hiragana-iroha", CSS_L_HIRAIROHA }, { "katakana-iroha", CSS_L_KATAIROHA },
        { "cjk-ideographic", CSS_L_CJK }, { "cjk-decimal", CSS_L_CJK }, { "japanese-informal", CSS_L_CJK },
        { "armenian", CSS_L_DECIMAL }, { "georgian", CSS_L_DECIMAL }     /* その文字が出せないので数字 */
    };
    int i;
    for (i = 0; i < (int)(sizeof(t) / sizeof(t[0])); i++) {
        if (IsWord(w, t[i].name)) {
            return t[i].type;
        }
    }
    return -1;
}

static void ListType(const char *v, CssStyle *st)
{
    int k = ListTypeOf(v);
    if (IsWord(v, "none")) {
        st->listNone = 1;
        return;
    }
    st->listNone = 0;
    st->listType = (BYTE)(k >= 0 ? k : CSS_L_DECIMAL);   /* 分からないものは数字で代用 */
}

int CssListType(const char *name)
{
    return ListTypeOf(name);
}

/* list-style の省略記法 */
static void ListStyle(const char *v, CssStyle *st)
{
    char tok[6][128];
    int  n = Tokens(v, tok, 6), i, none = 0;

    st->listNone   = 0;
    st->listType   = 0;
    st->listInside = 0;
    st->listImg    = 0;
    for (i = 0; i < n; i++) {
        int k;
        if (IsWord(tok[i], "none")) none = 1;
        else if (IsWord(tok[i], "inside")) st->listInside = 1;
        else if (IsWord(tok[i], "outside")) st->listInside = 0;
        else if (StrIStr(tok[i], "url(")) st->listImg = (short)UrlImage(v);   /* 長い URL は字句が切れるので元の値から */
        else if ((k = ListTypeOf(tok[i])) >= 0) st->listType = (BYTE)k;
    }
    if (none && st->listImg == 0 && st->listType == 0) {
        st->listNone = 1;
    }
}

/* line-height: normal / 数 (倍率。子の文字の大きさに合わせる) / 長さ / % */
static void LineHeight(const char *v, const LenCtx *c, CssStyle *st)
{
    const char *p = SkipWs(v);
    double      d;
    int         ok, px, k;

    if (IsWord(v, "normal")) {
        st->lineH   = 0;
        st->lineMul = 0;
        return;
    }
    d = Num(&p, &ok);
    if (ok && *SkipWs(p) == '\0') {
        if (d >= 0 && d < 20) {
            st->lineH   = (short)(d * 100 + 0.5);
            st->lineMul = 1;
        }
        return;
    }
    k = ParseLen(v, c, &px);
    if (k == 2) {
        px = st->fontPx * px / 100;
    } else if (k == 1 && AbsLen(v)) {
        px = ZoomAbs(px);               /* 文字だけ大きくなって行が重ならないように */
    }
    if (k == 1 || k == 2) {
        st->lineH   = (short)(px < 0 ? 0 : px > 2000 ? 2000 : px);
        st->lineMul = 0;
    }
}

/* text-shadow (最初の影だけ。ぼかしは無視する) */
static void TextShadow(const char *v, const LenCtx *c, CssStyle *st)
{
    char  one[256], tok[6][128];
    int   n, i, k = 0, len[3], px, depth = 0;
    DWORD col = CSS_NOCOLOR, cc;

    if (IsWord(v, "none")) {
        st->shadowOn = 0;
        return;
    }
    for (i = 0; v[i] && i < (int)sizeof(one) - 1; i++) {
        if (v[i] == '(') depth++;
        else if (v[i] == ')') depth--;
        else if (v[i] == ',' && depth == 0) break;
        one[i] = v[i];
    }
    one[i] = '\0';
    n = Tokens(one, tok, 6);
    for (i = 0; i < n; i++) {
        int r = CssParseColor(tok[i], &cc);
        if (r == 1) {
            col = cc;
        } else if (r == 2) {
            col = CSS_TRANSPARENT;
        } else if (r == 3) {
            col = CSS_NOCOLOR;
        } else if (k < 3 && ParseLen(tok[i], c, &px) == 1) {
            len[k++] = px;
        } else {
            return;                     /* 分からない値は宣言ごと無視する */
        }
    }
    if (k < 2) return;
    st->shadowOn    = (BYTE)(col != CSS_TRANSPARENT);
    st->shadowX     = (short)(len[0] < -200 ? -200 : len[0] > 200 ? 200 : len[0]);
    st->shadowY     = (short)(len[1] < -200 ? -200 : len[1] > 200 ? 200 : len[1]);
    st->shadowColor = col;
}

static void VAlign(const char *v, const LenCtx *c, CssStyle *st)
{
    static const struct { const char *name; BYTE k; } kw[] = {
        { "baseline", CSS_V_BASE }, { "sub", CSS_V_SUB }, { "super", CSS_V_SUPER }, { "top", CSS_V_TOP },
        { "text-top", CSS_V_TEXTTOP }, { "middle", CSS_V_MIDDLE }, { "bottom", CSS_V_BOTTOM },
        { "text-bottom", CSS_V_TEXTBOTTOM }
    };
    int i, px, k;

    for (i = 0; i < (int)(sizeof(kw) / sizeof(kw[0])); i++) {
        if (IsWord(v, kw[i].name)) {
            st->valign = kw[i].k;
            st->vshift = 0;
            return;
        }
    }
    k = ParseLen(v, c, &px);
    if (k == 2) {
        int lh = st->lineMul ? st->fontPx * st->lineH / 100 : st->lineH ? st->lineH : st->fontPx * 6 / 5;
        px = lh * px / 100;
    }
    if (k == 1 || k == 2) {
        st->valign = CSS_V_LEN;
        st->vshift = (short)(px < -2000 ? -2000 : px > 2000 ? 2000 : px);
    }
}

static BYTE RepeatOf(const char *v)
{
    char tok[2][128];
    int  n = Tokens(v, tok, 2);
    if (n == 2) {
        int x = !IsWord(tok[0], "no-repeat"), y = !IsWord(tok[1], "no-repeat");
        return (BYTE)(x && y ? 0 : x ? 1 : y ? 2 : 3);
    }
    if (IsWord(v, "repeat-x")) return 1;
    if (IsWord(v, "repeat-y")) return 2;
    if (IsWord(v, "no-repeat")) return 3;
    return 0;
}

/* background-position: キーワード (left / center / right / top / bottom) と長さ・% (1 - 2 個)。% は 100 倍して持つ */
static void BgPosTokens(char tok[][128], int n, const LenCtx *c, CssStyle *st)
{
    int i, set[2] = { 0, 0 }, idx = 0;

    st->bgPos[0] = st->bgPos[1] = 0;
    st->bgPosPct[0] = st->bgPosPct[1] = 1;
    for (i = 0; i < n && i < 4; i++) {
        int axis = -1, val = 0, pct = 1, px, k;
        if (IsWord(tok[i], "left")) { axis = 0; val = 0; }
        else if (IsWord(tok[i], "right")) { axis = 0; val = 10000; }
        else if (IsWord(tok[i], "top")) { axis = 1; val = 0; }
        else if (IsWord(tok[i], "bottom")) { axis = 1; val = 10000; }
        else if (IsWord(tok[i], "center")) { val = 5000; }
        else if ((k = ParseLen(tok[i], c, &px)) == 1 || k == 2) {
            pct = k == 2;
            if (pct) {
                double d = strtod(SkipWs(tok[i]), NULL) * 100.0;      /* 99.8392283% のような端数を残す */
                val = d > 32000.0 ? 32000 : d < -32000.0 ? -32000 : (int)(d < 0 ? d - 0.5 : d + 0.5);
            } else {
                val = px < -32000 ? -32000 : px > 32000 ? 32000 : px;
            }
        }
        else continue;
        if (axis < 0) {
            axis = idx == 0 ? (set[0] ? 1 : 0) : (set[1] ? 0 : 1);
        }
        st->bgPos[axis]    = (short)val;
        st->bgPosPct[axis] = (BYTE)pct;
        set[axis] = 1;
        idx++;
    }
    if (idx == 1) {
        /* 1 つだけなら、もう一方は中央 */
        int other = set[0] ? 1 : 0;
        st->bgPos[other]    = 5000;
        st->bgPosPct[other] = 1;
    }
}

static void BgPos(const char *v, const LenCtx *c, CssStyle *st)
{
    char tok[4][128];
    BgPosTokens(tok, Tokens(v, tok, 4), c, st);
}

/* font: caption / menu などのシステムフォントを、Windows の画面の設定から取る */
static void SystemFont(const char *v, CssStyle *st)
{
    NONCLIENTMETRICS ncm;
    LOGFONT          lf, *f = NULL;

    memset(&ncm, 0, sizeof(ncm));
    ncm.cbSize = offsetof(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);     /* Win95 の大きさ */
    if (IsWord(v, "icon")) {
        if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0)) f = &lf;
    } else if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
        if (IsWord(v, "caption")) f = &ncm.lfCaptionFont;
        else if (IsWord(v, "small-caption")) f = &ncm.lfSmCaptionFont;
        else if (IsWord(v, "menu")) f = &ncm.lfMenuFont;
        else if (IsWord(v, "status-bar")) f = &ncm.lfStatusFont;
        else if (IsWord(v, "message-box")) f = &ncm.lfMessageFont;
    }
    if (f == NULL) {
        return;
    }
    if (f->lfHeight != 0) {
        st->fontPx = (short)(f->lfHeight < 0 ? -f->lfHeight : f->lfHeight);
    }
    st->bold      = (BYTE)(f->lfWeight >= 600);
    st->italic    = (BYTE)(f->lfItalic != 0);
    st->smallCaps = 0;
    st->lineH     = 0;
    st->lineMul   = 0;
    st->family    = CSS_F_SANS;
    st->face[0]   = '\0';
    st->face2[0]  = '\0';
    if (g_fontHook != NULL && strlen(f->lfFaceName) < sizeof(st->face) && g_fontHook(f->lfFaceName)) {
        strcpy(st->face, f->lfFaceName);
    }
}

static void FontShorthand(const CssSheet *s, const char *v, const CssStyle *parent, CssStyle *st)
{
    char tok[16][128];
    int  n = Tokens(v, tok, 16), i, sizeAt = -1;
    char fam[512], sz[128], *slash;

    /* 大きさの語を探す (数字だけの語は太さ 700 などなので除く) */
    for (i = 0; i < n && sizeAt < 0; i++) {
        CssStyle tmp = *st;
        strcpy(sz, tok[i]);
        if ((slash = strchr(sz, '/')) != NULL) *slash = '\0';
        if ((!AllDigits(sz) || atoi(sz) == 0) && SetFontSize(s, sz, parent, &tmp)) {
            sizeAt = i;
        }
    }
    if (sizeAt < 0) {
        if (n == 1) {
            SystemFont(tok[0], st);     /* caption などのシステムフォント */
        }
        return;
    }
    st->italic = 0;
    st->bold = 0;
    st->smallCaps = 0;
    for (i = 0; i < sizeAt; i++) {
        if (IsWord(tok[i], "italic") || IsWord(tok[i], "oblique")) st->italic = 1;
        else if (IsWord(tok[i], "small-caps")) st->smallCaps = 1;
        else SetWeight(tok[i], st);
    }
    strcpy(sz, tok[sizeAt]);
    if ((slash = strchr(sz, '/')) != NULL) *slash = '\0';
    SetFontSize(s, sz, parent, st);
    {
        /* 行の高さ ("12px/1.5" や "/ 1.5")。書いていなければ normal に戻す */
        const char *lh = slash != NULL && slash[1] ? slash + 1 : NULL;
        char        lhb[128];
        i = sizeAt + 1;
        if (i < n && strcmp(tok[i], "/") == 0) {
            if (i + 1 < n) lh = tok[i + 1];
            i += 2;
        } else if (i < n && tok[i][0] == '/') {
            lh = tok[i] + 1;
            i++;
        }
        st->lineH   = 0;
        st->lineMul = 0;
        if (lh != NULL && *lh) {
            LenCtx c;
            StrCopy0(lhb, lh, sizeof(lhb));
            c.em     = st->fontPx;
            c.rem    = st->remPx;
            c.vw     = s->m.viewW;
            c.vh     = s->m.viewH;
            c.cq     = 0;
            c.strict = s->m.strict;
            LineHeight(lhb, &c, st);
        }
    }
    fam[0] = '\0';
    for (; i < n; i++) {
        if (strlen(fam) + strlen(tok[i]) + 2 < sizeof(fam)) {
            strcat(fam, tok[i]);
            strcat(fam, " ");
        }
    }
    if (fam[0]) {
        st->family = (BYTE)Family(fam);
        FaceOf(fam, st->face, st->face2, sizeof(st->face));
    }
}

/* margin / padding の % を 100 倍した値 (±160% まで) */
static int PctX100(const char *v, int allowNeg)
{
    double d = strtod(SkipWs(v), NULL) * 100.0;
    int    p = (int)(d < 0 ? d - 0.5 : d + 0.5);
    if (p > 16000) p = 16000;
    if (p < (allowNeg ? -16000 : 0)) p = allowNeg ? -16000 : 0;
    return p;
}

/* margin / padding の 1 - 4 値 (% は pct に入れ、out は 0 にする) */
static void Box4(const char *v, const LenCtx *c, short *out, short *pct, int allowAuto, int allowNeg)
{
    char tok[4][128];
    int  n = Tokens(v, tok, 4), i, val[4], pv[4];
    for (i = 0; i < n; i++) {
        int px, k = ParseLen(tok[i], c, &px);
        pv[i] = 0;
        if (k == 3 && allowAuto) px = CSS_AUTO;
        else if (k == 2) { pv[i] = PctX100(tok[i], allowNeg); px = 0; }
        else if (k != 1) px = 0;
        else if (!allowNeg && px < 0) px = 0;
        if (px != CSS_AUTO && px > 16000) px = 16000;
        if (px != CSS_AUTO && px < -16000) px = -16000;
        val[i] = px;
    }
    if (n == 0) return;
    if (n == 1) { val[1] = val[2] = val[3] = val[0]; pv[1] = pv[2] = pv[3] = pv[0]; }
    else if (n == 2) { val[2] = val[0]; val[3] = val[1]; pv[2] = pv[0]; pv[3] = pv[1]; }
    else if (n == 3) { val[3] = val[1]; pv[3] = pv[1]; }
    for (i = 0; i < 4; i++) {
        out[i] = (short)val[i];
        pct[i] = (short)pv[i];
    }
}

static int BorderWidth(const char *v, const LenCtx *c, int *px)
{
    if (IsWord(v, "thin")) { *px = 1; return 1; }
    if (IsWord(v, "medium")) { *px = 3; return 1; }
    if (IsWord(v, "thick")) { *px = 5; return 1; }
    if (ParseLen(v, c, px) == 1) {
        if (*px < 0) *px = 0;
        if (*px > 40) *px = 40;
        return 1;
    }
    return 0;
}

/* 線種 (CSS_B_*)。読めなければ 0 を返す */
static int BorderStyle(const char *v, int *on)
{
    static const struct { const char *name; int k; } t[] = {
        { "none", CSS_B_NONE }, { "hidden", CSS_B_NONE }, { "solid", CSS_B_SOLID }, { "dotted", CSS_B_DOTTED },
        { "dashed", CSS_B_DASHED }, { "double", CSS_B_DOUBLE }, { "groove", CSS_B_GROOVE },
        { "ridge", CSS_B_RIDGE }, { "inset", CSS_B_INSET }, { "outset", CSS_B_OUTSET }
    };
    int i;
    for (i = 0; i < (int)(sizeof(t) / sizeof(t[0])); i++) {
        if (IsWord(v, t[i].name)) {
            *on = t[i].k;
            return 1;
        }
    }
    return 0;
}

/* outline の省略記法 (太さ・線種・色) */
static void Outline(const char *v, const LenCtx *c, CssStyle *st)
{
    char  tok[8][128];
    int   n = Tokens(v, tok, 8), i, px, k;
    DWORD cc;

    st->outlineStyle = 0;
    st->outlineW     = 0;
    st->outlineColor = CSS_NOCOLOR;
    for (i = 0; i < n; i++) {
        if (BorderWidth(tok[i], c, &px)) st->outlineW = (short)(px > 0 ? px : -1);
        else if (IsWord(tok[i], "auto")) st->outlineStyle = CSS_B_SOLID;
        else if (BorderStyle(tok[i], &k)) st->outlineStyle = (BYTE)k;
        else if (CssParseColor(tok[i], &cc) == 1) st->outlineColor = cc;
    }
}

/* cursor の値 (url() の後の代わりの名前を使う。分からなければ -1) */
static int CursorOf(const char *v)
{
    static const struct { const char *name; BYTE c; } t[] = {
        { "auto", CSS_C_AUTO }, { "default", CSS_C_DEFAULT }, { "pointer", CSS_C_POINTER }, { "hand", CSS_C_POINTER },
        { "text", CSS_C_TEXT }, { "vertical-text", CSS_C_TEXT }, { "wait", CSS_C_WAIT },
        { "progress", CSS_C_PROGRESS }, { "move", CSS_C_MOVE }, { "all-scroll", CSS_C_MOVE },
        { "grab", CSS_C_MOVE }, { "grabbing", CSS_C_MOVE }, { "crosshair", CSS_C_CROSS }, { "cell", CSS_C_CROSS },
        { "help", CSS_C_HELP }, { "not-allowed", CSS_C_NO }, { "no-drop", CSS_C_NO },
        { "n-resize", CSS_C_NS }, { "s-resize", CSS_C_NS }, { "ns-resize", CSS_C_NS }, { "row-resize", CSS_C_NS },
        { "e-resize", CSS_C_EW }, { "w-resize", CSS_C_EW }, { "ew-resize", CSS_C_EW }, { "col-resize", CSS_C_EW },
        { "nw-resize", CSS_C_NWSE }, { "se-resize", CSS_C_NWSE }, { "nwse-resize", CSS_C_NWSE },
        { "ne-resize", CSS_C_NESW }, { "sw-resize", CSS_C_NESW }, { "nesw-resize", CSS_C_NESW },
        { "none", CSS_C_NONE }, { "context-menu", CSS_C_DEFAULT }, { "alias", CSS_C_DEFAULT },
        { "copy", CSS_C_DEFAULT }, { "zoom-in", CSS_C_DEFAULT }, { "zoom-out", CSS_C_DEFAULT }
    };
    const char *p = strrchr(v, ',');
    int         i;
    if (p != NULL) {
        v = SkipWs(p + 1);
    }
    for (i = 0; i < (int)(sizeof(t) / sizeof(t[0])); i++) {
        if (IsWord(v, t[i].name)) {
            return t[i].c;
        }
    }
    return -1;
}

/* clip: rect(上, 右, 下, 左) (カンマは省いてもよい。auto は端) */
static void ClipRect(const char *v, const LenCtx *c, CssStyle *st)
{
    char        buf[128];
    const char *p;
    int         i = 0, n = 0, px;

    st->clipSet = 0;
    if (strnicmp(v, "rect(", 5) != 0) {
        return;
    }
    p = v + 5;
    while (*p && *p != ')' && i < 4) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        n = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != ',' && *p != ')' && n < (int)sizeof(buf) - 1) buf[n++] = *p++;
        buf[n] = '\0';
        if (n == 0) break;
        if (IsWord(buf, "auto")) px = CSS_AUTO;
        else if (ParseLen(buf, c, &px) != 1) return;
        else if (px > 30000) px = 30000;
        else if (px < -30000) px = -30000;
        st->clipR[i++] = (short)px;
    }
    if (i == 4) {
        st->clipSet = 1;
    }
}

/* border / border-top などの省略記法。mask は対象の辺 (bit0 = 上 ...) */
static void BorderShorthand(const char *v, const LenCtx *c, int mask, CssStyle *st)
{
    char  tok[8][128];
    int   n = Tokens(v, tok, 8), i, w = CSS_UNSET, on = 0, px, side;
    DWORD col = CSS_NOCOLOR, cc;

    int   ca = 255;
    for (i = 0; i < n; i++) {
        int k;
        if (BorderWidth(tok[i], c, &px)) w = px;
        else if (BorderStyle(tok[i], &k)) on = k;
        else if ((k = CssParseColor(tok[i], &cc)) == 1) col = cc, ca = g_colorA;
        else if (k == 2) col = CSS_TRANSPARENT;
    }
    for (side = 0; side < 4; side++) {
        if (mask & (1 << side)) {
            st->bwidth[side] = (short)w;
            st->bstyle[side] = (BYTE)on;
            st->bcolor[side] = col;
            st->bcT[side]    = (BYTE)(255 - ca);
        }
    }
}

/* border-width / -style / -color の 1 - 4 値 */
static void Border4(const char *v, const LenCtx *c, int what, CssStyle *st)
{
    char tok[4][128];
    int  n = Tokens(v, tok, 4), i, idx[4];
    if (n == 0) return;
    for (i = 0; i < 4; i++) {
        idx[i] = n == 1 ? 0 : n == 2 ? i % 2 : n == 3 ? (i == 3 ? 1 : i) : i;
    }
    for (i = 0; i < 4; i++) {
        const char *t = tok[idx[i]];
        int         px, on;
        DWORD       col;
        if (what == 0 && BorderWidth(t, c, &px)) st->bwidth[i] = (short)px;
        else if (what == 1 && BorderStyle(t, &on)) st->bstyle[i] = (BYTE)on;
        else if (what == 2 && (on = CssParseColor(t, &col)) != 0 && on != 3) {
            st->bcolor[i] = on == 2 ? CSS_TRANSPARENT : col;
            st->bcT[i]    = (BYTE)(255 - g_colorA);
        }
    }
}

/* 背景の省略記法: 色・画像・繰り返し・位置・固定。グラデーションは最初の色で塗る */
/* background の値の字句がすべて分かるものか (分からない字句・色が 2 つある値は宣言ごと無効)。
   重ねた背景 (カンマ区切り) は調べない */
/* 括弧の外の "/" の前後に空白を入れた写し ("center/cover" → "center / cover")。CssXmalloc で確保する */
static char *SpaceSlash(const char *v)
{
    size_t      n = strlen(v);
    char       *out = CssXmalloc(n * 3 + 1), *o = out;
    int         depth = 0;
    const char *p;

    for (p = v; *p; p++) {
        if (*p == '(') depth++;
        else if (*p == ')' && depth > 0) depth--;
        if (*p == '/' && depth == 0) {
            *o++ = ' ';
            *o++ = '/';
            *o++ = ' ';
        } else {
            *o++ = *p;
        }
    }
    *o = '\0';
    return out;
}

/* background-size の字句 (1 つか 2 つ) */
static void BgSizeTokens(char tok[][128], int n, const LenCtx *lc, CssStyle *st)
{
    int i, k, px;

    if (n >= 1 && (IsWord(tok[0], "cover") || IsWord(tok[0], "contain"))) {
        st->bgSizeMode = (BYTE)(IsWord(tok[0], "cover") ? 1 : 2);
        return;
    }
    st->bgSize[0] = st->bgSize[1] = CSS_AUTO;
    st->bgSizePct[0] = st->bgSizePct[1] = 0;
    for (i = 0; i < n && i < 2; i++) {
        if (IsWord(tok[i], "auto")) continue;
        k = ParseLen(tok[i], lc, &px);
        if (k == 1) {
            st->bgSize[i] = (short)(px < 0 ? 0 : px > 16000 ? 16000 : px);
        } else if (k == 2) {
            double d = strtod(tok[i], NULL) * 100.0;
            st->bgSize[i]    = (short)(d < 0 ? 0 : d > 30000 ? 30000 : (int)(d + 0.5));
            st->bgSizePct[i] = 1;
        }
    }
    st->bgSizeMode = (BYTE)(st->bgSize[0] == CSS_AUTO && st->bgSize[1] == CSS_AUTO ? 0 : 3);
}

/* 値の中の linear-gradient() / radial-gradient() (と -webkit- などの古い書き方) を、書いた順 (上の層から) に
   改行でつないで CssIntern する。無ければ NULL */
static const char *GradOf(const char *v)
{
    char       *buf = CssXmalloc(strlen(v) + 2);
    const char *p = v, *r;
    int         o = 0;

    for (;;) {
        const char *a = FindI(p, "gradient("), *s, *e;
        if (a == NULL) break;
        e = SkipBlock(a + 8);
        if ((a - v >= 7 && (strnicmp(a - 7, "linear-", 7) == 0 || strnicmp(a - 7, "radial-", 7) == 0))) {
            for (s = a; s > v && (isalpha((unsigned char)s[-1]) || s[-1] == '-'); s--) ;
            if (o > 0) buf[o++] = '\n';
            memcpy(buf + o, s, e - s);
            o += (int)(e - s);
        }
        p = e;
    }
    buf[o] = '\0';
    r = o > 0 ? CssIntern(buf) : NULL;
    free(buf);
    return r;
}

static int BackgroundValid(const char *v, const LenCtx *lc)
{
    char  tok[12][128];
    int   n = Tokens(v, tok, 12), i, colors = 0, depth = 0, k, px;
    DWORD c;
    const char *p;

    for (p = v; *p; p++) {
        if (*p == '(') depth++;
        else if (*p == ')') depth--;
        else if (*p == ',' && depth == 0) return 1;
    }
    for (i = 0; i < n; i++) {
        const char *t = tok[i];
        if (strchr(t, '(') != NULL || t[0] == '/' || IsWord(t, "none")
            || IsWord(t, "repeat") || IsWord(t, "repeat-x") || IsWord(t, "repeat-y") || IsWord(t, "no-repeat")
            || IsWord(t, "space") || IsWord(t, "round") || IsWord(t, "fixed") || IsWord(t, "scroll") || IsWord(t, "local")
            || IsWord(t, "left") || IsWord(t, "right") || IsWord(t, "top") || IsWord(t, "bottom") || IsWord(t, "center")
            || IsWord(t, "cover") || IsWord(t, "contain") || IsWord(t, "auto")
            || IsWord(t, "border-box") || IsWord(t, "padding-box") || IsWord(t, "content-box") || IsWord(t, "text")) {
            continue;
        }
        if ((k = ParseLen(t, lc, &px)) == 1 || k == 2) {
            continue;
        }
        if (CssParseColor(t, &c) != 0) {
            if (++colors > 1) return 0;
            continue;
        }
        return 0;
    }
    return 1;
}

/* 背景の箱のキーワード (0 = border-box、1 = padding-box、2 = content-box、3 = text。違えば -1) */
static int BoxKw(const char *t)
{
    return IsWord(t, "border-box") ? 0 : IsWord(t, "padding-box") ? 1 : IsWord(t, "content-box") ? 2
         : IsWord(t, "text") ? 3 : -1;
}

/* グラデーション (g は "…-gradient(" を含む文字列) の最初の色。あれば 1 (アルファは g_colorA) */
static int GradFirstColor(const char *g, DWORD *c)
{
    const char *p = strchr(g, '(');
    if (p == NULL) {
        return 0;
    }
    p++;
    while (*p) {
        char        one[128];
        const char *e = ScanTo(p, ",)");
        const char *s1 = p, *s2 = e;
        int         m;
        Trim(&s1, &s2);
        m = (int)(s2 - s1) < 127 ? (int)(s2 - s1) : 127;
        memcpy(one, s1, m);
        one[m] = '\0';
        if (CssParseColor(one, c) == 1) {
            return 1;
        }
        {
            /* "red 10%" のような色の後ろの位置指定 */
            char *sp = strchr(one, ' ');
            if (sp) {
                *sp = '\0';
                if (CssParseColor(one, c) == 1) {
                    return 1;
                }
            }
        }
        if (*e != ',') break;
        p = e + 1;
    }
    return 0;
}

static void Background(const char *v0, const LenCtx *lc, CssStyle *st)
{
    char  tok[12][128], pos[4][128];
    char *v = SpaceSlash(v0);           /* "center/cover" の / を字句に分ける */
    int   n = Tokens(v, tok, 12), i, np = 0, nbox = 0;
    DWORD c;

    if (!BackgroundValid(v, lc)) {
        free(v);
        return;                         /* "red pink" など: 宣言ごと無視する */
    }

    st->bg       = CSS_NOCOLOR;
    st->bgImg    = 0;
    st->bgRepeat = 0;
    st->bgFixed  = 0;
    st->bgPos[0] = st->bgPos[1] = 0;
    st->bgPosPct[0] = st->bgPosPct[1] = 1;
    st->bgSizeMode = 0;
    st->bgFromGrad = 0;
    st->bgGrad     = GradOf(v);
    st->bgClip     = 0;
    st->bgOrigin   = 0;
    for (i = 0; i < n; i++) {
        int k, px;
        if ((k = BoxKw(tok[i])) >= 0) {
            /* 1 つ目の箱のキーワードは origin と clip、2 つ目は clip */
            if (nbox++ == 0 && k < 3) st->bgOrigin = (BYTE)(k == 0 ? 1 : k == 1 ? 0 : 2);
            st->bgClip = (BYTE)k;
            continue;
        }
        if (strcmp(tok[i], "/") == 0) {
            /* 位置 / 大きさ */
            int m = 0;
            while (m < 2 && i + 1 + m < n
                   && (IsWord(tok[i + 1 + m], "auto") || IsWord(tok[i + 1 + m], "cover") || IsWord(tok[i + 1 + m], "contain")
                       || (k = ParseLen(tok[i + 1 + m], lc, &px)) == 1 || k == 2)) {
                m++;
            }
            BgSizeTokens(&tok[i + 1], m, lc, st);
            i += m;
            continue;
        }
        if (strnicmp(tok[i], "url(", 4) == 0) {
            st->bgImg = (short)UrlImage(v);     /* 長い URL は字句が切れるので元の値から */
            continue;
        }
        if (IsWord(tok[i], "repeat") || IsWord(tok[i], "repeat-x") || IsWord(tok[i], "repeat-y")
            || IsWord(tok[i], "no-repeat") || IsWord(tok[i], "space") || IsWord(tok[i], "round")) {
            st->bgRepeat = RepeatOf(tok[i]);
            continue;
        }
        if (IsWord(tok[i], "fixed") || IsWord(tok[i], "scroll") || IsWord(tok[i], "local")) {
            st->bgFixed = (BYTE)IsWord(tok[i], "fixed");
            continue;
        }
        if ((IsWord(tok[i], "left") || IsWord(tok[i], "right") || IsWord(tok[i], "top") || IsWord(tok[i], "bottom")
             || IsWord(tok[i], "center") || (k = ParseLen(tok[i], lc, &px)) == 1 || k == 2) && np < 4) {
            strcpy(pos[np++], tok[i]);
            continue;
        }
        k = CssParseColor(tok[i], &c);
        if (k == 1) {
            st->bg  = c;
            st->bgT = (BYTE)(255 - g_colorA);
        } else if (k == 3) {
            st->bg  = st->color;
            st->bgT = st->colorT;
        }
        if (strstr(tok[i], "gradient(") != NULL && st->bg == CSS_NOCOLOR) {
            st->bgFromGrad = 1;         /* グラデーションを描かないときに使う色 */
            if (GradFirstColor(tok[i], &c)) {
                st->bg  = c;
                st->bgT = (BYTE)(255 - g_colorA);
            }
        }
    }
    if (np > 0) {
        BgPosTokens(pos, np, lc, st);
    }
    if (st->bg == CSS_NOCOLOR) {
        st->bgFromGrad = 0;
    }
    free(v);
}

/* 重ねた背景 (カンマ区切り): url() の画像のある最初の層を主の層 (bgImg など) にし、残りの画像の層を
   "画像 繰り返し 横 横% 縦 縦% 大きさ 幅 幅% 高さ 高さ%;" の一覧 (bgList) にする。
   imgOnly (background-image) なら画像だけで、位置などは主の層のもの ("画像 m;") */
static void PutLong(char *out, int *o, int size, long v);
static void PutStr(char *out, int *o, int size, const char *s);

static void BgLayers(const char *v, const LenCtx *lc, CssStyle *st, int imgOnly)
{
    char        list[1024];
    const char *p = v;
    int         o = 0, first = 1;

    st->bgList = NULL;
    if (*ScanTo(v, ",") != ',') {
        return;
    }
    list[0] = '\0';
    while (*p) {
        const char *e = ScanTo(p, ",");
        size_t      n = (size_t)(e - p);
        char       *seg = CssXmalloc(n + 1);
        memcpy(seg, p, n);
        seg[n] = '\0';
        if (FindI(seg, "url(") != NULL) {
            CssStyle t;
            if (imgOnly) {
                t.bgImg = (short)UrlImage(seg);
            } else {
                memset(&t, 0, sizeof(t));
                Background(seg, lc, &t);
            }
            if (t.bgImg > 0 && first) {
                /* 一番上の画像の層 */
                st->bgImg = t.bgImg;
                if (!imgOnly) {
                    st->bgRepeat   = t.bgRepeat;
                    st->bgFixed    = t.bgFixed;
                    memcpy(st->bgPos, t.bgPos, sizeof(st->bgPos));
                    memcpy(st->bgPosPct, t.bgPosPct, sizeof(st->bgPosPct));
                    st->bgSizeMode = t.bgSizeMode;
                    memcpy(st->bgSize, t.bgSize, sizeof(st->bgSize));
                    memcpy(st->bgSizePct, t.bgSizePct, sizeof(st->bgSizePct));
                }
                first = 0;
            } else if (t.bgImg > 0 && o < (int)sizeof(list) - 80) {
                PutLong(list, &o, sizeof(list), t.bgImg);
                if (imgOnly) {
                    PutStr(list, &o, sizeof(list), " m;");
                } else {
                    long f[10];
                    int  k;
                    f[0] = t.bgRepeat; f[1] = t.bgPos[0]; f[2] = t.bgPosPct[0]; f[3] = t.bgPos[1]; f[4] = t.bgPosPct[1];
                    f[5] = t.bgSizeMode; f[6] = t.bgSize[0]; f[7] = t.bgSizePct[0]; f[8] = t.bgSize[1]; f[9] = t.bgSizePct[1];
                    for (k = 0; k < 10; k++) {
                        PutStr(list, &o, sizeof(list), " ");
                        PutLong(list, &o, sizeof(list), f[k]);
                    }
                    PutStr(list, &o, sizeof(list), ";");
                }
            }
        }
        free(seg);
        p = *e ? e + 1 : e;
    }
    st->bgList = o > 0 ? CssIntern(list) : NULL;
}

/* border-image-repeat のキーワード (0 = stretch / 1 = repeat / 2 = round / 3 = space。違えば -1) */
static int BiRepOf(const char *t)
{
    return IsWord(t, "stretch") ? 0 : IsWord(t, "repeat") ? 1 : IsWord(t, "round") ? 2 : IsWord(t, "space") ? 3 : -1;
}

/* border-image (part: 0 = 一括指定 / 1 = -source / 2 = -slice / 3 = -repeat)。画像は url() だけ読む
   (Decorations=0 なら読まない)。グラデーションは描かず、枠線の色をその最初の色にする。
   一括指定の / の後ろ (border-image-width / -outset) は扱わない */
static void BorderImage(const char *v0, int part, CssStyle *st)
{
    char  tok[12][128];
    char *v = SpaceSlash(v0);
    int   n = Tokens(v, tok, 12), i, ns = 0, nr = 0, slash = 0, fill = 0;
    int   sl[4], slp[4], r[2];

    r[0] = r[1] = 0;
    if (part <= 1) {
        const char *g = strstr(v, "gradient(");
        DWORD       gc;
        st->biImg = (short)(FindI(v, "url(") != NULL && g_masks ? UrlImage(v) : 0);
        if (g != NULL && GradFirstColor(g, &gc)) {
            for (i = 0; i < 4; i++) {
                st->bcolor[i] = gc;
                st->bcT[i]    = (BYTE)(255 - g_colorA);
            }
        }
        if (part == 0) {
            /* 一括指定は書いていないものを既定に戻す (slice 100%、stretch) */
            for (i = 0; i < 4; i++) st->biSl[i] = 100;
            st->biSlP   = 15;
            st->biFill  = 0;
            st->biRep[0] = st->biRep[1] = 0;
        }
    }
    for (i = 0; i < n && part != 1; i++) {
        const char *t = tok[i];
        char       *e;
        double      d;
        int         k;
        if (strchr(t, '(') != NULL) continue;
        if (strcmp(t, "/") == 0) {
            slash = 1;
            continue;
        }
        if ((k = BiRepOf(t)) >= 0) {
            if (nr < 2) r[nr++] = k;
            continue;
        }
        if (slash) continue;
        if (IsWord(t, "fill")) {
            fill = 1;
            continue;
        }
        d = strtod(t, &e);
        if (e != t && ns < 4) {
            sl[ns]  = (int)(d + 0.5);
            slp[ns] = *e == '%';
            ns++;
        }
    }
    if (ns > 0 && part != 3) {
        /* 1～4 個を上・右・下・左に (margin と同じ) */
        if (ns == 1) { sl[1] = sl[2] = sl[3] = sl[0]; slp[1] = slp[2] = slp[3] = slp[0]; }
        else if (ns == 2) { sl[2] = sl[0]; slp[2] = slp[0]; sl[3] = sl[1]; slp[3] = slp[1]; }
        else if (ns == 3) { sl[3] = sl[1]; slp[3] = slp[1]; }
        st->biSlP = 0;
        for (i = 0; i < 4; i++) {
            st->biSl[i] = (short)(sl[i] < 0 ? 0 : sl[i] > 30000 ? 30000 : sl[i]);
            if (slp[i]) st->biSlP |= (BYTE)(1 << i);
        }
        st->biFill = (BYTE)fill;
    }
    if (nr > 0 && part != 2) {
        st->biRep[0] = (BYTE)r[0];
        st->biRep[1] = (BYTE)(nr > 1 ? r[1] : r[0]);
    }
    free(v);
}

int CssBgLayers(const char *p, CssBgLayer *out, int max)
{
    int n = 0;
    if (p == NULL) {
        return 0;
    }
    while (*p && n < max) {
        char *e;
        long  v[11];
        int   k;
        memset(&out[n], 0, sizeof(out[n]));
        v[0] = strtol(p, &e, 10);
        if (e == p) break;
        p = SkipWs(e);
        if (*p == 'm') {
            out[n].img     = (int)v[0];
            out[n].useMain = 1;
        } else {
            for (k = 1; k < 11; k++) {
                v[k] = strtol(p, &e, 10);
                if (e == p) return n;
                p = e;
            }
            out[n].img        = (int)v[0];
            out[n].rep        = (BYTE)v[1];
            out[n].pos[0]     = (short)v[2];
            out[n].posPct[0]  = (BYTE)v[3];
            out[n].pos[1]     = (short)v[4];
            out[n].posPct[1]  = (BYTE)v[5];
            out[n].sizeMode   = (BYTE)v[6];
            out[n].size[0]    = (short)v[7];
            out[n].sizePct[0] = (BYTE)v[8];
            out[n].size[1]    = (short)v[9];
            out[n].sizePct[1] = (BYTE)v[10];
        }
        n++;
        while (*p && *p != ';') p++;
        if (*p == ';') p++;
    }
    return n;
}

/* border-radius の値 1 つ (px か %)。読めなければ 0 */
static int RadiusOne(const char *t, const LenCtx *lc, short *v, BYTE *pct)
{
    int px, k = ParseLen(t, lc, &px);
    if (k == 1) {
        *v   = (short)(px < 0 ? 0 : px > 16000 ? 16000 : px);
        *pct = 0;
        return 1;
    }
    if (k == 2) {
        *v   = (short)(px < 0 ? 0 : px > 100 ? 100 : px);
        *pct = 1;
        return 1;
    }
    return 0;
}

/* mask-image (url() のものだけ。グラデーションのマスクは無視する) */
static void MaskImage(const char *v, CssStyle *st)
{
    st->maskOn  = (BYTE)(FindI(v, "url(") != NULL);
    st->maskImg = (short)(st->maskOn && g_masks ? UrlImage(v) : 0);
}

static void MaskPosTokens(char tok[][128], int n, const LenCtx *c, CssStyle *st)
{
    CssStyle tmp;
    BgPosTokens(tok, n, c, &tmp);
    memcpy(st->maskPos, tmp.bgPos, sizeof(st->maskPos));
    memcpy(st->maskPosPct, tmp.bgPosPct, sizeof(st->maskPosPct));
}

static void MaskSizeTokens(char tok[][128], int n, const LenCtx *c, CssStyle *st)
{
    CssStyle tmp;
    tmp.bgSize[0] = tmp.bgSize[1] = CSS_AUTO;
    tmp.bgSizePct[0] = tmp.bgSizePct[1] = 0;
    BgSizeTokens(tok, n, c, &tmp);
    st->maskSizeMode = tmp.bgSizeMode;
    memcpy(st->maskSize, tmp.bgSize, sizeof(st->maskSize));
    memcpy(st->maskSizePct, tmp.bgSizePct, sizeof(st->maskSizePct));
}

/* mask の一括指定: 画像・位置 [/ 大きさ] と、使わない繰り返しなど */
static void MaskShorthand(const char *v0, const LenCtx *c, CssStyle *st)
{
    char *v = SpaceSlash(v0);
    char  tok[12][128], pos[4][128], size[2][128];
    int   n = Tokens(v, tok, 12), i, np = 0, ns = 0, slash = 0, k, px;

    free(v);
    MaskImage(v0, st);
    st->maskSizeMode = 0;
    st->maskPos[0] = st->maskPos[1] = 0;
    st->maskPosPct[0] = st->maskPosPct[1] = 0;
    for (i = 0; i < n; i++) {
        int len = strcmp(tok[i], "/") != 0 && strnicmp(tok[i], "url(", 4) != 0
                  && ((k = ParseLen(tok[i], c, &px)) == 1 || k == 2);
        if (strcmp(tok[i], "/") == 0) {
            slash = 1;
            continue;
        }
        if (slash && ns < 2 && (len || IsWord(tok[i], "cover") || IsWord(tok[i], "contain") || IsWord(tok[i], "auto"))) {
            strcpy(size[ns++], tok[i]);
            continue;
        }
        slash = 0;
        if (np < 4 && (len || IsWord(tok[i], "left") || IsWord(tok[i], "right") || IsWord(tok[i], "top")
                       || IsWord(tok[i], "bottom") || IsWord(tok[i], "center"))) {
            strcpy(pos[np++], tok[i]);
        }
    }
    if (np > 0) MaskPosTokens(pos, np, c, st);
    if (ns > 0) MaskSizeTokens(size, ns, c, st);
}

/* border-radius: 左上 右上 右下 左下 (省略は margin と同じ。"/" の後の縦の半径は使わない) */
static void BorderRadius(const char *v0, const LenCtx *lc, CssStyle *st)
{
    char  tok[8][128];
    char *v = SpaceSlash(v0);
    int   n = Tokens(v, tok, 8), i, m;
    short r[4];
    BYTE  p[4];

    free(v);
    for (m = 0; m < n && m < 4 && strcmp(tok[m], "/") != 0; m++) {
        if (!RadiusOne(tok[m], lc, &r[m], &p[m])) return;
    }
    if (m == 0) return;
    for (i = 0; i < 4; i++) {
        int k = m == 1 ? 0 : m == 2 ? i % 2 : m == 3 ? (i == 3 ? 1 : i) : i;
        st->radius[i]    = r[k];
        st->radiusPct[i] = p[k];
    }
}

/* box-shadow: 最初の inset でない影 (横 縦 [ぼかし [広げる]] 色) */
static void PutStr(char *out, int *o, int size, const char *s);

/* 整数を 10 進で out に足す */
static void PutLong(char *out, int *o, int size, long v)
{
    char d[16];
    int  m = 0, neg = v < 0;
    unsigned long u = neg ? (unsigned long)(-v) : (unsigned long)v;
    do {
        d[m++] = (char)('0' + u % 10);
        u /= 10;
    } while (u > 0 && m < 15);
    if (neg && *o < size - 1) out[(*o)++] = '-';
    while (m > 0 && *o < size - 1) out[(*o)++] = d[--m];
    out[*o] = '\0';
}

/* box-shadow: 最初の inset でない影は boxShX などに、2 つ以上か inset の影があればすべてを
   "inset x y ぼかし 広げる 色 不透明度;" の一覧 (boxShList。色 -1 = 文字の色) にする */
static void BoxShadow(const char *v, const LenCtx *lc, CssStyle *st)
{
    char  list[512];
    int   o = 0, count = 0, anyInset = 0, haveFirst = 0, fx = 0, fy = 0, fb = 0, fs = 0, fa = 255;
    DWORD fc = CSS_NOCOLOR;

    if (IsWord(v, "none")) {
        st->boxShOn   = 0;
        st->boxShList = NULL;
        return;
    }
    list[0] = '\0';
    while (*v) {
        const char *e = ScanTo(v, ",");
        char        one[512], tok[8][128];
        int         len = (int)(e - v) < 511 ? (int)(e - v) : 511, n, i, nl = 0, lens[4], inset = 0, k, px;
        int         blur, spread;
        DWORD       col = CSS_NOCOLOR, cc;
        int         alpha = 255, hasCol = 0;
        memcpy(one, v, len);
        one[len] = '\0';
        n = Tokens(one, tok, 8);
        for (i = 0; i < n; i++) {
            if (IsWord(tok[i], "inset")) {
                inset = 1;
            } else if ((k = ParseLen(tok[i], lc, &px)) == 1 && nl < 4) {
                lens[nl++] = px;
            } else if ((k = CssParseColor(tok[i], &cc)) != 0) {
                hasCol = 1;
                col    = k == 1 ? cc : CSS_NOCOLOR;
                alpha  = k == 2 ? 0 : g_colorA;
            } else {
                return;                 /* 読めない影: 宣言ごと無視する */
            }
        }
        if (nl < 2) {
            return;
        }
        blur   = nl > 2 && lens[2] > 0 ? (lens[2] > 200 ? 200 : lens[2]) : 0;
        spread = nl > 3 ? (lens[3] > 200 ? 200 : lens[3] < -200 ? -200 : lens[3]) : 0;
        if (!inset && !haveFirst) {
            haveFirst = 1;
            fx = lens[0];
            fy = lens[1];
            fb = blur;
            fs = spread;
            fc = hasCol ? col : CSS_NOCOLOR;
            fa = alpha;
        }
        anyInset |= inset;
        count++;
        PutLong(list, &o, sizeof(list), inset);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), lens[0] < -2000 ? -2000 : lens[0] > 2000 ? 2000 : lens[0]);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), lens[1] < -2000 ? -2000 : lens[1] > 2000 ? 2000 : lens[1]);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), blur);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), spread);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), hasCol && col != CSS_NOCOLOR ? (long)col : -1L);
        PutStr(list, &o, sizeof(list), " ");
        PutLong(list, &o, sizeof(list), alpha);
        PutStr(list, &o, sizeof(list), ";");
        if (*e != ',') break;
        v = e + 1;
    }
    st->boxShOn = (BYTE)(haveFirst && fa > 0);
    if (haveFirst) {
        st->boxShX      = (short)fx;
        st->boxShY      = (short)fy;
        st->boxShBlur   = (short)fb;
        st->boxShSpread = (short)fs;
        st->boxShColor  = fc;
        st->boxShT      = (BYTE)(255 - fa);
    }
    st->boxShList = count > 1 || anyInset ? CssIntern(list) : NULL;
}

int CssShadowList(const char *p, CssShadow *out, int max)
{
    int n = 0;

    if (p == NULL) {
        return 0;
    }
    while (*p && n < max) {
        long  v[7];
        char *e;
        int   k;
        for (k = 0; k < 7; k++) {
            v[k] = strtol(p, &e, 10);
            if (e == p) return n;
            p = e;
        }
        out[n].inset  = (BYTE)(v[0] != 0);
        out[n].x      = (short)v[1];
        out[n].y      = (short)v[2];
        out[n].blur   = (short)v[3];
        out[n].spread = (short)v[4];
        out[n].color  = v[5] < 0 ? CSS_NOCOLOR : (DWORD)v[5];
        out[n].alpha  = (BYTE)(v[6] < 0 ? 0 : v[6] > 255 ? 255 : v[6]);
        n++;
        while (*p && *p != ';') p++;
        if (*p == ';') p++;
    }
    return n;
}

/* 幅・高さ。add があれば calc(% ± px) も読み、px の部分を *add に入れる */
static void WidthCalc(const char *v, const LenCtx *c, short *w, BYTE *pct, short *add)
{
    int px, k;
    if (IsWord(v, "none") || IsWord(v, "auto")) {
        *w = CSS_UNSET;
        *pct = 0;
        return;
    }
    k = ParseLen(v, c, &px);
    if (k == 4 && add != NULL) {
        double d = g_calcPct * 100.0;
        *w   = (short)(d < 0 ? 0 : d > 30000 ? 30000 : (int)(d + 0.5));
        *pct = 1;
        *add = (short)px;
        return;
    }
    if (add != NULL && (k == 1 || k == 2)) {
        *add = 0;
    }
    if (k == 2) {
        /* % は 100 倍して小数第 2 位まで持つ (10.638% のような指定で 1px ずれないように) */
        double d = strtod(SkipWs(v), NULL) * 100.0 + 0.5;
        px = d < 0 ? 0 : d > 30000 ? 30000 : (int)d;
    }
    if (k == 1 || k == 2) {
        if (px < 0) px = 0;
        if (px > 30000) px = 30000;
        *w = (short)px;
        *pct = (BYTE)(k == 2);
    }
}

static void Width(const char *v, const LenCtx *c, short *w, BYTE *pct)
{
    WidthCalc(v, c, w, pct, NULL);
}

/* justify-content / align-items などの値 (CSS_AL_*。分からなければ -1) */
static int AlignWord(const char *v)
{
    if (strnicmp(v, "safe ", 5) == 0) v = SkipWs(v + 5);
    else if (strnicmp(v, "unsafe ", 7) == 0) v = SkipWs(v + 7);
    if (IsWord(v, "normal") || IsWord(v, "auto")) return CSS_AL_AUTO;
    if (IsWord(v, "stretch")) return CSS_AL_STRETCH;
    if (IsWord(v, "flex-start") || IsWord(v, "start") || IsWord(v, "self-start") || IsWord(v, "left")) return CSS_AL_START;
    if (IsWord(v, "flex-end") || IsWord(v, "end") || IsWord(v, "self-end") || IsWord(v, "right")) return CSS_AL_END;
    if (IsWord(v, "center")) return CSS_AL_CENTER;
    if (IsWord(v, "baseline") || IsWord(v, "first baseline") || IsWord(v, "last baseline")) return CSS_AL_BASELINE;
    if (IsWord(v, "space-between")) return CSS_AL_BETWEEN;
    if (IsWord(v, "space-around")) return CSS_AL_AROUND;
    if (IsWord(v, "space-evenly")) return CSS_AL_EVENLY;
    return -1;
}

/* 空白で区切った次の語を buf に取り出す。無ければ 0 */
static int NextToken(const char **pv, char *buf, int size)
{
    const char *v = SkipWs(*pv);
    int         n = 0;
    while (*v && !isspace((unsigned char)*v)) {
        if (n < size - 1) buf[n++] = *v;
        v++;
    }
    buf[n] = '\0';
    *pv = v;
    return n > 0;
}

/* flex-grow などの数 (100 倍)。数でなければ -1 */
static int FlexNumber(const char *v)
{
    char  *e;
    double d = strtod(v, &e);
    if (e == v || *SkipWs(e) != '\0' || d < 0) return -1;
    return d > 300.0 ? 30000 : (int)(d * 100.0 + 0.5);
}

/* flex-direction / flex-wrap の語を当てる。分からなければ 0 */
static int FlexFlowWord(const char *v, CssStyle *st)
{
    if (IsWord(v, "row")) st->flexDir = CSS_FD_ROW;
    else if (IsWord(v, "row-reverse")) st->flexDir = CSS_FD_ROWREV;
    else if (IsWord(v, "column")) st->flexDir = CSS_FD_COL;
    else if (IsWord(v, "column-reverse")) st->flexDir = CSS_FD_COLREV;
    else if (IsWord(v, "nowrap")) st->flexWrap = 0;
    else if (IsWord(v, "wrap")) st->flexWrap = 1;
    else if (IsWord(v, "wrap-reverse")) st->flexWrap = 2;
    else return 0;
    return 1;
}

/* flex: none / auto / 伸びる率 [縮む率] [基準の大きさ] */
static void FlexShorthand(const char *v, const LenCtx *c, CssStyle *st)
{
    char  tok[64];
    int   nNum = 0, gotBasis = 0, g = 100, s = 100, b = 0, bp = 0, x;

    if (IsWord(v, "none")) {
        st->grow = st->shrink = 0;
        st->basis = CSS_AUTO;
        return;
    }
    if (IsWord(v, "auto") || IsWord(v, "initial")) {
        st->grow     = (short)(IsWord(v, "auto") ? 100 : 0);
        st->shrink   = 100;
        st->basis    = CSS_AUTO;
        st->basisPct = 0;
        return;
    }
    while (NextToken(&v, tok, sizeof(tok))) {
        if (nNum < 2 && !gotBasis && (x = FlexNumber(tok)) >= 0) {
            if (nNum++ == 0) g = x; else s = x;
        } else if (!gotBasis && (IsWord(tok, "auto") || IsWord(tok, "content"))) {
            b = CSS_AUTO;
            gotBasis = 1;
        } else if (!gotBasis) {
            short w = CSS_UNSET;
            BYTE  pct = 0;
            Width(tok, c, &w, &pct);
            if (w == CSS_UNSET) return;
            b  = w;
            bp = pct;
            gotBasis = 1;
        } else {
            return;
        }
    }
    if (nNum == 0 && !gotBasis) return;
    st->grow     = (short)g;            /* "flex: 200px" は 1 1 200px、"flex: 1" は 1 1 0 */
    st->shrink   = (short)s;
    st->basis    = (short)b;
    st->basisPct = (BYTE)bp;
}

/* 文字を out (大きさ size、今 *o 文字) に足す */
static void PutStr(char *out, int *o, int size, const char *s)
{
    while (*s && *o < size - 1) out[(*o)++] = *s++;
    out[*o] = '\0';
}

/* grid-template-columns などの p..e を、長さを px にして out に写す。repeat() / minmax() の形と
   線の名前 [..] は残し、fit-content() などは auto にする */
static void GridNormList(const char *p, const char *e, const LenCtx *c, char *out, int *o, int size)
{
    while (p < e) {
        const char *q;
        char        tok[64];
        int         n, px;
        p = SkipWs(p);
        if (p >= e) break;
        if (*p == '[') {
            q = SkipBlock(p);
            if (q > e) q = e;
            while (p < q && *o < size - 2) {
                out[(*o)++] = isspace((unsigned char)*p) ? ' ' : *p;
                p++;
            }
            out[*o] = '\0';
            PutStr(out, o, size, " ");
            p = q;
            continue;
        }
        if (*p == ',') {
            PutStr(out, o, size, ",");
            p++;
            continue;
        }
        if (isalpha((unsigned char)*p)) {
            for (q = p; isalpha((unsigned char)*q) || *q == '-'; q++) ;
            if (*q == '(') {
                const char *f = SkipBlock(q);
                if ((q - p == 6 && strnicmp(p, "repeat", 6) == 0) || (q - p == 6 && strnicmp(p, "minmax", 6) == 0)) {
                    PutStr(out, o, size, tolower((unsigned char)*p) == 'r' ? "repeat(" : "minmax(");
                    GridNormList(q + 1, f - 1, c, out, o, size);
                    PutStr(out, o, size, ") ");
                } else {
                    PutStr(out, o, size, "auto ");
                }
                p = f;
                continue;
            }
        }
        q = p;
        while (q < e && !isspace((unsigned char)*q) && *q != ',') q++;
        n = (int)(q - p) < 63 ? (int)(q - p) : 63;
        memcpy(tok, p, n);
        tok[n] = '\0';
        p = q;
        if ((n > 2 && stricmp(tok + n - 2, "fr") == 0) || strchr(tok, '%') != NULL || IsWord(tok, "auto-fill")
            || IsWord(tok, "auto-fit") || strspn(tok, "0123456789") == (size_t)n) {
            PutStr(out, o, size, tok);  /* fr・%・repeat の数はそのまま */
        } else if (ParseLen(tok, c, &px) == 1) {
            int  v2 = px < 0 ? 0 : px, m = 0;
            char d[12];
            do {
                d[m++] = (char)('0' + v2 % 10);
                v2 /= 10;
            } while (v2 > 0 && m < 11);
            n = 0;
            while (m > 0) tok[n++] = d[--m];
            strcpy(tok + n, "px");
            PutStr(out, o, size, tok);
        } else {
            PutStr(out, o, size, "auto");   /* auto / min-content / max-content など */
        }
        PutStr(out, o, size, " ");
    }
}

/* grid-template-columns などの値 v..e を px にした写し (CssIntern。none なら NULL) */
static const char *GridNormN(const char *v, const char *e, const LenCtx *c)
{
    char out[1024];
    int  o = 0;
    const char *s = v;
    Trim(&s, &e);
    if ((e - s == 4 && strnicmp(s, "none", 4) == 0) || memchr(s, '"', e - s) != NULL) {
        return NULL;
    }
    out[0] = '\0';
    GridNormList(s, e, c, out, &o, sizeof(out));
    return o > 0 ? CssIntern(out) : NULL;
}

static const char *GridNorm(const char *v, const LenCtx *c)
{
    return GridNormN(v, v + strlen(v), c);
}

/* grid-template-areas の文字列 (v..e) を "a a b/c c d" の形にする (. の並びは "."。列の数がそろわなければ NULL) */
static const char *GridAreasNorm(const char *v, const char *e)
{
    char out[512];
    int  o = 0, cols = -1;

    out[0] = '\0';
    while (v < e) {
        char qc;
        int  k = 0;
        while (v < e && *v != '"' && *v != '\'') v++;
        if (v >= e) break;
        qc = *v++;
        if (cols >= 0) PutStr(out, &o, sizeof(out), "/");
        for (;;) {
            while (v < e && *v != qc && isspace((unsigned char)*v)) v++;
            if (v >= e || *v == qc) break;
            if (k > 0) PutStr(out, &o, sizeof(out), " ");
            if (*v == '.') {
                while (v < e && *v == '.') v++;
                PutStr(out, &o, sizeof(out), ".");
            } else {
                for (; v < e && *v != qc && *v != '.' && !isspace((unsigned char)*v); v++) {
                    if (o < (int)sizeof(out) - 1) out[o++] = *v;
                }
                out[o] = '\0';
            }
            k++;
        }
        if (v < e) v++;
        if (k == 0 || (cols >= 0 && k != cols)) {
            return NULL;
        }
        cols = k;
    }
    return o > 0 ? CssIntern(out) : NULL;
}

/* grid-template の値 (行 / 列、または文字列と行の大きさを並べた書き方 [/ 列]) を性質に分ける */
static void GridTemplate(const char *v, const LenCtx *c, CssStyle *st)
{
    const char *sl = ScanTo(v, "/");
    st->gridRows  = NULL;
    st->gridCols  = NULL;
    st->gridAreas = NULL;
    if (IsWord(v, "none")) {
        return;
    }
    if (*sl == '/') {
        st->gridCols = GridNorm(sl + 1, c);
    }
    if (memchr(v, '"', sl - v) != NULL || memchr(v, '\'', sl - v) != NULL) {
        /* 文字列は領域に、その後の大きさ (無ければ auto) と線の名前は行にする */
        char        rows[512];
        int         o = 0, pend = 0;
        const char *p = v;
        rows[0] = '\0';
        st->gridAreas = GridAreasNorm(v, sl);
        while (p < sl) {
            const char *q;
            p = SkipWs(p);
            if (p >= sl) break;
            q = ScanTo(p, " \t\r\n");
            if (q > sl) q = sl;
            if (*p == '"' || *p == '\'' || *p == '[') {
                if (pend) PutStr(rows, &o, sizeof(rows), "auto ");
                pend = *p != '[';
            } else {
                pend = 0;
            }
            if (*p != '"' && *p != '\'') {
                while (p < q && o < (int)sizeof(rows) - 2) rows[o++] = *p++;
                rows[o] = '\0';
                PutStr(rows, &o, sizeof(rows), " ");
            }
            p = q;
        }
        if (pend) PutStr(rows, &o, sizeof(rows), "auto");
        st->gridRows = GridNorm(rows, c);
    } else if (*sl == '/') {
        st->gridRows = GridNormN(v, sl, c);
    }
}

/* grid の一括指定の auto-flow の側 (v..e)。流し方と、残りの大きさ (grid-auto-rows / columns) を入れる */
static void GridAutoFlowSide(const char *v, const char *e, int column, const LenCtx *c, CssStyle *st)
{
    char        rest[256];
    int         o = 0, px;
    const char *p = v;
    rest[0] = '\0';
    st->gridFlow = (BYTE)(column ? 1 : 0);
    while (p < e) {
        const char *q;
        p = SkipWs(p);
        if (p >= e) break;
        q = ScanTo(p, " \t\r\n");
        if (q > e) q = e;
        if (q - p == 5 && strnicmp(p, "dense", 5) == 0) {
            st->gridFlow |= 2;
        } else if (!(q - p == 9 && strnicmp(p, "auto-flow", 9) == 0)) {
            while (p < q && o < (int)sizeof(rest) - 2) rest[o++] = *p++;
            rest[o++] = ' ';
            rest[o] = '\0';
        }
        p = q;
    }
    if (o == 0) return;
    rest[o - 1] = '\0';
    if (column) {
        st->gridAutoCols = GridNorm(rest, c);
    } else if (ParseLen(rest, c, &px) == 1 && px > 0) {
        st->gridAutoRow = (short)(px > 16000 ? 16000 : px);
    }
}

/* grid-column / grid-row の片側 ("3"・"-1"・"span 2"・"auto"・名前)。*line か *span か *name に入れる */
static void GridLineOne(const char *a, const char *b, short *line, short *span, const char **name)
{
    char  t[64], *s = t;
    int   n = (int)(b - a) < 63 ? (int)(b - a) : 63;
    memcpy(t, a, n);
    t[n] = '\0';
    while (*s && isspace((unsigned char)*s)) s++;
    while (n > 0 && isspace((unsigned char)t[n - 1])) t[--n] = '\0';
    if (strnicmp(s, "span", 4) == 0 && (s[4] == '\0' || isspace((unsigned char)s[4]))) {
        int k = atoi(SkipWs(s + 4));
        *span = (short)(k < 1 ? 1 : k > 64 ? 64 : k);
    } else if (isdigit((unsigned char)*s) || (*s == '-' && isdigit((unsigned char)s[1]))) {
        int k = atoi(s);
        if (k != 0) *line = (short)(k > 512 ? 512 : k < -512 ? -512 : k);
    } else if (*s && !IsWord(s, "auto")) {
        char *sp = s;
        while (*sp && !isspace((unsigned char)*sp)) sp++;
        *sp = '\0';                     /* 名前 (後ろの数は扱わない) */
        *name = CssIntern(s);
    }
}

/* grid-column / grid-row: 始まり [/ 終わり]。終わりが無く始まりが名前なら、終わりも同じ名前 */
static void GridLine(const char *v, short *start, short *end, short *span, const char **names)
{
    const char *sl = ScanTo(v, "/");
    *start = *end = *span = 0;
    names[0] = names[1] = NULL;
    GridLineOne(v, sl, start, span, &names[0]);
    if (*sl == '/') {
        short sp2 = 0;
        GridLineOne(sl + 1, sl + strlen(sl), end, &sp2, &names[1]);
        if (sp2) *span = sp2;
    } else {
        names[1] = names[0];
    }
}

/* translate の値 1 つ (長さか %) を *px と *pct (100 倍) に足す */
static void TrAdd(const char *a, const char *b, const LenCtx *c, int *px, int *pct)
{
    char        t[64];
    const char *s1 = a, *s2 = b;
    int         n, k, v;
    Trim(&s1, &s2);
    n = (int)(s2 - s1) < 63 ? (int)(s2 - s1) : 63;
    memcpy(t, s1, n);
    t[n] = '\0';
    k = ParseLen(t, c, &v);         /* 単位の無い 0 も長さとして読める */
    if (k == 1) {
        *px += v;
    } else if (k == 2) {
        *pct += (int)(strtod(t, NULL) * 100.0);
    }
}

/* transform の translate*() / matrix() のずらす量だけを読む (回転・拡大などは扱わない)。
   individual なら translate 性質 ("10px 20px") */
/* 角度 a..b (deg・turn・rad・grad。単位が無ければ度) を度にする */
static double AngleOf(const char *a, const char *b)
{
    char   *u;
    double  d = strtod(SkipWs(a), &u);
    if ((const char *)u >= b) return d;
    if (strnicmp(u, "turn", 4) == 0) return d * 360.0;
    if (strnicmp(u, "grad", 4) == 0) return d * 0.9;
    if (strnicmp(u, "rad", 3) == 0) return d * 57.29577951;
    return d;
}

/* 拡大の率 a..b (数か %) */
static double ScaleOf(const char *a, const char *b)
{
    char   *u;
    double  d = strtod(SkipWs(a), &u);
    if ((const char *)u < b && *u == '%') d /= 100.0;
    return d;
}

/* 回転 (度、時計回り) と拡大を CssStyle に入れる */
static void SetXf(CssStyle *st, double rot, double sx, double sy)
{
    long r = (long)(rot < 0 ? rot - 0.5 : rot + 0.5) % 360;
    if (r < 0) r += 360;
    if (sx > 16.0) sx = 16.0;
    if (sx < -16.0) sx = -16.0;
    if (sy > 16.0) sy = 16.0;
    if (sy < -16.0) sy = -16.0;
    st->xfRot = (short)r;
    st->xfSX  = (short)(sx * 1000.0 + (sx < 0 ? -0.5 : 0.5));
    st->xfSY  = (short)(sy * 1000.0 + (sy < 0 ? -0.5 : 0.5));
    st->hasXf = (BYTE)(st->xfRot != 0 || st->xfSX != 1000 || st->xfSY != 1000);
}

static void Transform(const char *v, const LenCtx *c, CssStyle *st, int individual)
{
    int    x = 0, y = 0, xp = 0, yp = 0;
    double rot = 0.0, sx = 1.0, sy = 1.0;

    st->hasTr = 0;
    st->trX = st->trY = st->trXP = st->trYP = 0;
    if (!individual) {
        SetXf(st, 0.0, 1.0, 1.0);
    }
    if (IsWord(v, "none")) {
        return;
    }
    if (individual) {
        const char *sp = ScanTo(v, " ");
        TrAdd(v, sp, c, &x, &xp);
        if (*sp) TrAdd(sp, sp + strlen(sp), c, &y, &yp);
    } else {
        const char *p = v;
        while (*p) {
            const char *q, *e, *cm;
            char        name[16];
            int         n = 0;
            p = SkipWs(p);
            if (!isalpha((unsigned char)*p)) {
                if (*p == '\0') break;
                p++;
                continue;
            }
            for (q = p; isalnum((unsigned char)*q) && n < 15; q++) name[n++] = (char)tolower((unsigned char)*q);
            name[n] = '\0';
            while (isalnum((unsigned char)*q)) q++;
            if (*q != '(') {
                p = q;
                continue;
            }
            e  = SkipBlock(q) - 1;
            cm = ScanTo(q + 1, ",");
            if (cm > e) cm = e;
            if (strcmp(name, "translate") == 0 || strcmp(name, "translate3d") == 0) {
                TrAdd(q + 1, cm, c, &x, &xp);
                if (cm < e) {
                    const char *cm2 = ScanTo(cm + 1, ",");
                    TrAdd(cm + 1, cm2 > e ? e : cm2, c, &y, &yp);
                }
            } else if (strcmp(name, "translatex") == 0) {
                TrAdd(q + 1, e, c, &x, &xp);
            } else if (strcmp(name, "translatey") == 0) {
                TrAdd(q + 1, e, c, &y, &yp);
            } else if (strcmp(name, "rotate") == 0 || strcmp(name, "rotatez") == 0) {
                rot += AngleOf(q + 1, e);
            } else if (strcmp(name, "scale") == 0 || strcmp(name, "scale3d") == 0) {
                double a = ScaleOf(q + 1, cm);
                sx *= a;
                sy *= cm < e ? ScaleOf(cm + 1, e) : a;
            } else if (strcmp(name, "scalex") == 0) {
                sx *= ScaleOf(q + 1, e);
            } else if (strcmp(name, "scaley") == 0) {
                sy *= ScaleOf(q + 1, e);
            } else if (strcmp(name, "matrix") == 0) {
                /* matrix(a, b, c, d, e, f): a～d から回転と拡大、e と f はずらす量 */
                const char *r = q + 1;
                double      m[4];
                int         i;
                for (i = 0; i < 4 && r < e; i++) {
                    m[i] = strtod(SkipWs(r), NULL);
                    r = ScanTo(r, ",");
                    if (r < e) r++;
                }
                if (i == 4) {
                    double s0 = sqrt(m[0] * m[0] + m[1] * m[1]);
                    if (s0 > 0.0001) {
                        rot += atan2(m[1], m[0]) * 57.29577951;
                        sx  *= s0;
                        sy  *= (m[0] * m[3] - m[1] * m[2]) / s0;
                    }
                }
                if (r < e) {
                    double dx = strtod(r, NULL), dy;
                    cm = ScanTo(r, ",");
                    dy = cm < e ? strtod(cm + 1, NULL) : 0;
                    x += (int)dx;
                    y += (int)dy;
                }
            }
            p = e + 1;
        }
    }
    st->trX  = (short)(x < -16000 ? -16000 : x > 16000 ? 16000 : x);
    st->trY  = (short)(y < -16000 ? -16000 : y > 16000 ? 16000 : y);
    st->trXP = (short)(xp < -30000 ? -30000 : xp > 30000 ? 30000 : xp);
    st->trYP = (short)(yp < -30000 ? -30000 : yp > 30000 ? 30000 : yp);
    st->hasTr = (BYTE)(x || y || xp || yp);
    if (!individual) {
        SetXf(st, rot, sx, sy);
    }
}

/* filter の関数の量 (数か %。書いていなければ def) */
static double FiltAmt(const char *a, const char *b, double def)
{
    const char *p = SkipWs(a);
    char       *u;
    double      d;
    if (p >= b) return def;
    d = strtod(p, &u);
    if ((const char *)u == p) return def;
    if ((const char *)u < b && *u == '%') d /= 100.0;
    return d;
}

static short FiltK(double d, double lo, double hi)
{
    if (d < lo) d = lo;
    if (d > hi) d = hi;
    return (short)(d * 1000.0 + 0.5);
}

/* filter: grayscale・sepia・saturate・hue-rotate・invert・brightness・contrast・opacity・drop-shadow (blur は扱わない) */
static void Filter(const char *v, const LenCtx *c, CssStyle *st)
{
    const char *p = v;

    st->hasFilt = 0;
    st->fGray = st->fSepia = st->fInvert = st->fHue = 0;
    st->fSat = st->fBright = st->fContrast = st->fOpac = 1000;
    st->fDs = 0;
    if (IsWord(v, "none")) {
        return;
    }
    while (*p) {
        const char *q, *e;
        char        name[16];
        int         n = 0;
        p = SkipWs(p);
        if (!isalpha((unsigned char)*p)) {
            if (*p == '\0') break;
            p++;
            continue;
        }
        for (q = p; (isalnum((unsigned char)*q) || *q == '-') && n < 15; q++) name[n++] = (char)tolower((unsigned char)*q);
        name[n] = '\0';
        while (isalnum((unsigned char)*q) || *q == '-') q++;
        if (*q != '(') {
            p = q;
            continue;
        }
        e = SkipBlock(q) - 1;
        if (strcmp(name, "grayscale") == 0) st->fGray = FiltK(FiltAmt(q + 1, e, 1.0), 0, 1);
        else if (strcmp(name, "sepia") == 0) st->fSepia = FiltK(FiltAmt(q + 1, e, 1.0), 0, 1);
        else if (strcmp(name, "invert") == 0) st->fInvert = FiltK(FiltAmt(q + 1, e, 1.0), 0, 1);
        else if (strcmp(name, "saturate") == 0) st->fSat = FiltK(FiltAmt(q + 1, e, 1.0), 0, 10);
        else if (strcmp(name, "brightness") == 0) st->fBright = FiltK(FiltAmt(q + 1, e, 1.0), 0, 10);
        else if (strcmp(name, "contrast") == 0) st->fContrast = FiltK(FiltAmt(q + 1, e, 1.0), 0, 10);
        else if (strcmp(name, "opacity") == 0) st->fOpac = FiltK(FiltAmt(q + 1, e, 1.0), 0, 1);
        else if (strcmp(name, "hue-rotate") == 0) st->fHue = (short)((long)AngleOf(q + 1, e) % 360);
        else if (strcmp(name, "drop-shadow") == 0) {
            /* drop-shadow(横 縦 [ぼかし] 色) (色は前でもよい。ぼかしは使わない) */
            char  one[256], tok[5][128];
            int   len = (int)(e - q - 1) < 255 ? (int)(e - q - 1) : 255, nt, i, nl = 0, px, k;
            DWORD col = RGB(0, 0, 0);
            int   al = 255;
            memcpy(one, q + 1, len);
            one[len] = '\0';
            nt = Tokens(one, tok, 5);
            for (i = 0; i < nt; i++) {
                DWORD cc;
                if (ParseLen(tok[i], c, &px) == 1) {
                    if (nl == 0) st->fDsX = (short)(px < -500 ? -500 : px > 500 ? 500 : px);
                    else if (nl == 1) st->fDsY = (short)(px < -500 ? -500 : px > 500 ? 500 : px);
                    nl++;
                } else if ((k = CssParseColor(tok[i], &cc)) != 0) {
                    col = k == 1 ? cc : RGB(0, 0, 0);
                    al  = k == 2 ? 0 : g_colorA;
                }
            }
            if (nl >= 2 && al > 0) {
                st->fDs      = 1;
                st->fDsColor = col;
                st->fDsA     = (BYTE)al;
            }
        }
        p = e + 1;
    }
    st->hasFilt = (BYTE)(st->fGray || st->fSepia || st->fInvert || st->fHue || st->fSat != 1000 || st->fBright != 1000
                         || st->fContrast != 1000 || st->fOpac != 1000 || st->fDs);
}

/* text-decoration-style の語 (CSS_DS_*。線種でなければ -1) */
static int DecoStyleOf(const char *v)
{
    return IsWord(v, "solid") ? CSS_DS_SOLID : IsWord(v, "double") ? CSS_DS_DOUBLE : IsWord(v, "dotted") ? CSS_DS_DOTTED
         : IsWord(v, "dashed") ? CSS_DS_DASHED : IsWord(v, "wavy") ? CSS_DS_WAVY : -1;
}

/* 線を自分で引くか (色・線種・太さ・位置のどれかの指定がある) */
static void DecoSetFlag(CssStyle *st)
{
    st->decoSet = (BYTE)(st->decoHasCol || st->decoStyle != CSS_DS_SOLID || st->decoThick || st->ulOffSet);
}

/* animation の一括指定 (最初の 1 つ): 名前・fill-mode・繰り返し */
static void Animation(const char *v, CssStyle *st)
{
    static const char *const skip[] = {
        "ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end", "normal", "reverse",
        "alternate", "alternate-reverse", "running", "paused", "none", "backwards", "initial", "inherit", NULL
    };
    char tok[8][128], one[512];
    const char *cm = ScanTo(v, ",");
    int  n, i, k, len = (int)(cm - v) < 511 ? (int)(cm - v) : 511;

    memcpy(one, v, len);
    one[len] = '\0';
    n = Tokens(one, tok, 8);
    st->animName = NULL;
    st->animFwd  = st->animInf = 0;
    for (i = 0; i < n; i++) {
        const char *t = tok[i];
        if (IsWord(t, "forwards") || IsWord(t, "both")) { st->animFwd = 1; continue; }
        if (IsWord(t, "infinite")) { st->animInf = 1; continue; }
        if (!isalpha((unsigned char)*t) && *t != '_' && *t != '-' && *t != '"' && *t != '\'') continue;   /* 時間・数 */
        if (strchr(t, '(') != NULL) continue;                                                             /* cubic-bezier() など */
        for (k = 0; skip[k] != NULL && !IsWord(t, skip[k]); k++) ;
        if (skip[k] == NULL && st->animName == NULL) {
            char nm[128];
            int  m = 0;
            if (*t == '"' || *t == '\'') t++;
            while (*t && *t != '"' && *t != '\'' && m < 127) nm[m++] = *t++;
            nm[m] = '\0';
            st->animName = CssIntern(nm);
        }
    }
}

/* gap / row-gap / column-gap (px か %。% なら *pct を 1 にする) */
static void GapValue(const char *v, const LenCtx *c, short *out, BYTE *pct)
{
    int px, k;
    if (IsWord(v, "normal")) {
        *out = 0;
        *pct = 0;
    } else if ((k = ParseLen(v, c, &px)) == 1 || k == 2) {
        *out = (short)(px < 0 ? 0 : px > 16000 ? 16000 : px);
        *pct = (BYTE)(k == 2);
    }
}

static int g_uaDisplay;                 /* CSS を当てる前の display (display: revert 用) */

static void ApplyDecl(const CssSheet *s, int prop, const char *v0, const CssStyle *parent, CssStyle *st)
{
    char        buf[VALBUF];
    const char *v = v0;
    LenCtx      c;
    DWORD       col;
    int         k, px;

    if (prop == P_VAR) {
        return;                         /* カスタムプロパティは CssApply で並びに足してある */
    }
    if (strstr(v, "var(") != NULL) {
        if (!ExpandVars(s, v, buf, sizeof(buf), 0)) {
            return;
        }
        v = buf;
    }
    v = SkipWs(v);
    c.em  = st->fontPx;
    c.rem = st->remPx;
    c.vw  = s->m.viewW;
    c.vh  = s->m.viewH;
    c.cq  = parent->cqW;
    c.strict = s->m.strict;
    if (IsWord(v, "inherit")) {
        /* 継承しない性質の inherit: 親の値を写す */
        int k;
        switch (prop) {
        case P_FLOAT: st->floatSide = parent->floatSide; return;
        case P_CLEAR: st->clear = parent->clear; return;
        case P_DISPLAY:
            st->display  = parent->display;
            st->flex     = parent->flex;
            st->grid     = parent->grid;
            st->contents = parent->contents;
            return;
        case P_POSITION: st->pos = parent->pos; st->posAbs = parent->posAbs; st->sticky = parent->sticky; return;
        case P_TOP: case P_RIGHT: case P_BOTTOM: case P_LEFT:
            k = prop == P_TOP ? CSS_TOP : prop == P_RIGHT ? CSS_RIGHT : prop == P_BOTTOM ? CSS_BOTTOM : CSS_LEFT;
            st->off[k] = parent->off[k];
            st->offPct[k] = parent->offPct[k];
            st->offAdd[k] = parent->offAdd[k];
            return;
        case P_ZINDEX: st->zIndex = parent->zIndex; return;
        case P_OVERFLOW: case P_OVERFLOW_X: case P_OVERFLOW_Y:
            st->overflow = parent->overflow;
            if (prop != P_OVERFLOW_Y) st->overflowX = parent->overflowX;
            return;
        case P_WIDTH:
            st->width = parent->width; st->widthPct = parent->widthPct; st->addW = parent->addW; st->wAuto = parent->wAuto;
            return;
        case P_HEIGHT:
            st->height = parent->height; st->heightPct = parent->heightPct; st->addH = parent->addH; st->hAuto = parent->hAuto;
            return;
        case P_MAXWIDTH:
            st->maxWidth = parent->maxWidth; st->maxWidthPct = parent->maxWidthPct; st->addMaxW = parent->addMaxW;
            return;
        case P_MINWIDTH:
            st->minWidth = parent->minWidth; st->minWidthPct = parent->minWidthPct; st->addMinW = parent->addMinW;
            return;
        case P_MINHEIGHT:
            st->minHeight = parent->minHeight; st->minHeightPct = parent->minHeightPct; st->addMinH = parent->addMinH;
            return;
        case P_MAXHEIGHT:
            st->maxHeight = parent->maxHeight; st->maxHeightPct = parent->maxHeightPct; st->addMaxH = parent->addMaxH;
            return;
        case P_BOXSIZING: st->borderBox = parent->borderBox; return;
        case P_MARGIN:
            memcpy(st->margin, parent->margin, sizeof(st->margin));
            memcpy(st->marginPct, parent->marginPct, sizeof(st->marginPct));
            return;
        case P_MARGIN_T: case P_MARGIN_R: case P_MARGIN_B: case P_MARGIN_L:
            st->margin[prop - P_MARGIN_T] = parent->margin[prop - P_MARGIN_T];
            st->marginPct[prop - P_MARGIN_T] = parent->marginPct[prop - P_MARGIN_T];
            return;
        case P_PADDING:
            memcpy(st->padding, parent->padding, sizeof(st->padding));
            memcpy(st->padPct, parent->padPct, sizeof(st->padPct));
            return;
        case P_PADDING_T: case P_PADDING_R: case P_PADDING_B: case P_PADDING_L:
            st->padding[prop - P_PADDING_T] = parent->padding[prop - P_PADDING_T];
            st->padPct[prop - P_PADDING_T] = parent->padPct[prop - P_PADDING_T];
            return;
        case P_OUTLINE: case P_OUTLINE_C: case P_OUTLINE_O: case P_OUTLINE_S: case P_OUTLINE_W:
            if (prop == P_OUTLINE || prop == P_OUTLINE_S) st->outlineStyle = parent->outlineStyle;
            if (prop == P_OUTLINE || prop == P_OUTLINE_W) st->outlineW = parent->outlineW;
            if (prop == P_OUTLINE || prop == P_OUTLINE_C) st->outlineColor = parent->outlineColor;
            if (prop == P_OUTLINE_O) st->outlineOff = parent->outlineOff;
            return;
        case P_CLIP:
            st->clipSet = parent->clipSet;
            memcpy(st->clipR, parent->clipR, sizeof(st->clipR));
            return;
        case P_CONTENT: st->content = parent->content; return;
        case P_COUNTERRESET: st->counterReset = parent->counterReset; return;
        case P_COUNTERINC: st->counterInc = parent->counterInc; return;
        case P_TABLELAYOUT: st->tableFixed = parent->tableFixed; return;
        case P_BORDER: case P_BORDER_WIDTH: case P_BORDER_STYLE: case P_BORDER_COLOR:
            for (k = 0; k < 4; k++) {
                if (prop == P_BORDER || prop == P_BORDER_WIDTH) st->bwidth[k] = parent->bwidth[k];
                if (prop == P_BORDER || prop == P_BORDER_STYLE) st->bstyle[k] = parent->bstyle[k];
                if (prop == P_BORDER || prop == P_BORDER_COLOR) st->bcolor[k] = parent->bcolor[k];
            }
            return;
        case P_BORDER_T: case P_BORDER_R: case P_BORDER_B: case P_BORDER_L:
            k = prop - P_BORDER_T;
            st->bwidth[k] = parent->bwidth[k];
            st->bstyle[k] = parent->bstyle[k];
            st->bcolor[k] = parent->bcolor[k];
            return;
        case P_BW_T: case P_BW_R: case P_BW_B: case P_BW_L: st->bwidth[prop - P_BW_T] = parent->bwidth[prop - P_BW_T]; return;
        case P_BS_T: case P_BS_R: case P_BS_B: case P_BS_L: st->bstyle[prop - P_BS_T] = parent->bstyle[prop - P_BS_T]; return;
        case P_BC_T: case P_BC_R: case P_BC_B: case P_BC_L: st->bcolor[prop - P_BC_T] = parent->bcolor[prop - P_BC_T]; return;
        case P_VALIGN: st->valign = parent->valign; st->vshift = parent->vshift; return;
        case P_TEXTDECO: st->under = parent->under; st->strike = parent->strike; st->over = parent->over; return;
        case P_TEXTTRANSFORM: st->transform = parent->transform; return;
        case P_TEXTINDENT: st->indent = parent->indent; st->indentPct = parent->indentPct; return;
        case P_LINEHEIGHT: st->lineH = parent->lineH; st->lineMul = parent->lineMul; return;
        case P_LETTERSP: st->letterSp = parent->letterSp; return;
        case P_WORDSP: st->wordSp = parent->wordSp; return;
        case P_FONTVARIANT: st->smallCaps = parent->smallCaps; return;
        case P_VISIBILITY: st->invisible = parent->invisible; st->visCollapse = parent->visCollapse; return;
        case P_BGIMAGE: st->bgImg = parent->bgImg; return;
        case P_BGREPEAT: st->bgRepeat = parent->bgRepeat; return;
        case P_BGATTACH: st->bgFixed = parent->bgFixed; return;
        case P_BGPOS:
            memcpy(st->bgPos, parent->bgPos, sizeof(st->bgPos));
            memcpy(st->bgPosPct, parent->bgPosPct, sizeof(st->bgPosPct));
            return;
        case P_BACKGROUND:
            st->bg = parent->bg;
            st->bgImg = parent->bgImg;
            st->bgRepeat = parent->bgRepeat;
            st->bgFixed = parent->bgFixed;
            memcpy(st->bgPos, parent->bgPos, sizeof(st->bgPos));
            memcpy(st->bgPosPct, parent->bgPosPct, sizeof(st->bgPosPct));
            return;
        }
    }
    if (prop == P_ALL) {
        /* all: すべての性質を親から受け継いだ初期の状態に戻す (カスタムプロパティは残す。revert は何もしない) */
        if (IsWord(v, "unset") || IsWord(v, "initial")) {
            const struct CssVarNode *vars = st->vars;
            CssInherit(parent, st);
            st->vars = vars;
        }
        return;
    }
    if (prop == P_DISPLAY && IsWord(v, "revert")) {
        st->display = (BYTE)g_uaDisplay;        /* all: unset の後に display: revert と書くリセットのため */
        st->flex = st->grid = st->contents = 0;
        return;
    }
    if (IsWord(v, "inherit") || IsWord(v, "unset") || IsWord(v, "revert") || IsWord(v, "initial")) {
        /* 継承する性質は親の値、継承しないものは何もしない (初期値のまま) */
        switch (prop) {
        case P_COLOR: st->color = parent->color; st->colorClear = parent->colorClear; st->colorT = parent->colorT; break;
        case P_TEXTSHADOW:
            st->shadowOn = parent->shadowOn; st->shadowX = parent->shadowX; st->shadowY = parent->shadowY;
            st->shadowColor = parent->shadowColor;
            break;
        case P_FONTSIZE: st->fontPx = parent->fontPx; break;
        case P_FONTWEIGHT: st->bold = parent->bold; break;
        case P_FONTSTYLE: st->italic = parent->italic; break;
        case P_TEXTALIGN: st->align = parent->align; st->alignBlk = parent->alignBlk; break;
        case P_WHITESPACE: st->pre = parent->pre; st->nowrap = parent->nowrap; st->preLine = parent->preLine; break;
        case P_CURSOR: st->cursor = parent->cursor; break;
        case P_EMPTYCELLS: st->emptyHide = parent->emptyHide; break;
        case P_CAPTIONSIDE: st->captionBottom = parent->captionBottom; break;
        case P_QUOTES: st->quotes = parent->quotes; break;
        case P_LISTTYPE: st->listType = parent->listType; st->listNone = parent->listNone; break;
        case P_LISTPOS: st->listInside = parent->listInside; break;
        case P_LISTIMG: st->listImg = parent->listImg; break;
        case P_LISTSTYLE:
            st->listType   = parent->listType;
            st->listNone   = parent->listNone;
            st->listInside = parent->listInside;
            st->listImg    = parent->listImg;
            break;
        case P_BORDERSPACING: st->spacing[0] = parent->spacing[0]; st->spacing[1] = parent->spacing[1]; break;
        case P_BORDERCOLLAPSE: st->collapse = parent->collapse; break;
        case P_FONTFAMILY:
            st->family = parent->family;
            memcpy(st->face, parent->face, sizeof(st->face));
            memcpy(st->face2, parent->face2, sizeof(st->face2));
            break;
        case P_BGCOLOR: case P_BACKGROUND: st->bg = IsWord(v, "inherit") ? parent->bg : CSS_NOCOLOR; break;
        }
        return;
    }
    switch (prop) {
    case P_COLOR:
        k = CssParseColor(v, &col);
        if (k == 1) {
            st->color      = col;
            st->colorClear = 0;
            st->colorT     = (BYTE)(255 - g_colorA);
        } else if (k == 2) {
            st->colorClear = 1;         /* 文字を描かない (色は親のまま持つ) */
        }
        break;
    case P_TEXTSHADOW:
        TextShadow(v, &c, st);
        break;
    case P_BGCOLOR:
        k = CssParseColor(v, &col);
        if (k == 1) st->bg = col, st->bgT = (BYTE)(255 - g_colorA);
        else if (k == 2) st->bg = CSS_NOCOLOR;
        else if (k == 3) st->bg = st->color, st->bgT = st->colorT;
        break;
    case P_BACKGROUND:
        Background(v, &c, st);
        BgLayers(v, &c, st, 0);
        break;
    case P_FONTSIZE:
        SetFontSize(s, v, parent, st);
        break;
    case P_FONTWEIGHT:
        SetWeight(v, st);
        break;
    case P_FONTSTYLE:
        st->italic = (BYTE)(IsWord(v, "italic") || strnicmp(v, "oblique", 7) == 0);
        break;
    case P_FONTFAMILY:
        st->family = (BYTE)Family(v);
        FaceOf(v, st->face, st->face2, sizeof(st->face));
        break;
    case P_FONT:
        FontShorthand(s, v, parent, st);
        break;
    case P_TEXTDECO: {
        /* text-decoration (と -line): 線の種類。一括指定の色・線種・太さも読む */
        char tok[6][128];
        int  n = Tokens(v, tok, 6), i, k;
        if (StrIStr(v, "none")) {
            st->under = 0;
            st->strike = 0;
            st->over = 0;
        }
        if (StrIStr(v, "underline")) st->under = 1;
        if (StrIStr(v, "line-through")) st->strike = 1;
        if (StrIStr(v, "overline")) st->over = 1;
        for (i = 0; i < n; i++) {
            DWORD col;
            if ((k = DecoStyleOf(tok[i])) >= 0) {
                st->decoStyle = (BYTE)k;
            } else if (CssParseColor(tok[i], &col) == 1) {
                st->decoColor  = col;
                st->decoHasCol = 1;
            } else if (ParseLen(tok[i], &c, &px) == 1 && px > 0) {
                st->decoThick = (BYTE)(px > 20 ? 20 : px);
            }
        }
        DecoSetFlag(st);
        break;
    }
    case P_DECOCOLOR: {
        DWORD col;
        k = CssParseColor(v, &col);
        st->decoHasCol = (BYTE)(k == 1);
        st->decoColor  = col;
        DecoSetFlag(st);
        break;
    }
    case P_DECOSTYLE:
        st->decoStyle = (BYTE)(DecoStyleOf(v) > 0 ? DecoStyleOf(v) : 0);
        DecoSetFlag(st);
        break;
    case P_DECOTHICK:
        st->decoThick = (BYTE)(ParseLen(v, &c, &px) == 1 && px > 0 ? (px > 20 ? 20 : px) : 0);
        DecoSetFlag(st);
        break;
    case P_ULOFFSET:
        st->ulOffSet = (BYTE)(ParseLen(v, &c, &px) == 1);
        st->ulOff    = (short)(st->ulOffSet ? (px > 100 ? 100 : px < -100 ? -100 : px) : 0);
        DecoSetFlag(st);
        break;
    case P_FILTER:
        Filter(v, &c, st);
        break;
    case P_COLCOUNT:
        k = atoi(v);
        st->colCount = (BYTE)(IsWord(v, "auto") || k < 1 ? 0 : k > 32 ? 32 : k);
        break;
    case P_COLWIDTH:
        st->colWidth = (short)(ParseLen(v, &c, &px) == 1 && px > 0 ? (px > 16000 ? 16000 : px) : 0);
        break;
    case P_COLUMNS: {
        /* columns: 段の数と段の幅 (どちらか・両方。auto は指定なし) */
        char tok[2][128];
        int  n = Tokens(v, tok, 2), i;
        st->colCount = 0;
        st->colWidth = 0;
        for (i = 0; i < n; i++) {
            if (strspn(tok[i], "0123456789") == strlen(tok[i])) {
                k = atoi(tok[i]);
                st->colCount = (BYTE)(k < 1 ? 0 : k > 32 ? 32 : k);
            } else if (ParseLen(tok[i], &c, &px) == 1 && px > 0) {
                st->colWidth = (short)(px > 16000 ? 16000 : px);
            }
        }
        break;
    }
    case P_COLRULE: case P_COLRULECOLOR: case P_COLRULESTYLE: case P_COLRULEWIDTH: {
        /* column-rule: 太さ・線種・色 (個別の性質も同じ読み方) */
        char tok[3][128];
        int  n = Tokens(v, tok, 3), i;
        if (prop == P_COLRULE) {
            st->colRuleStyle = 0;
            st->colRuleW     = 3;
            st->colRuleHasCol = 0;
        }
        for (i = 0; i < n; i++) {
            DWORD col;
            if (IsWord(tok[i], "none") || IsWord(tok[i], "hidden")) {
                st->colRuleStyle = 0;
            } else if (IsWord(tok[i], "solid") || IsWord(tok[i], "dotted") || IsWord(tok[i], "dashed")
                       || IsWord(tok[i], "double") || IsWord(tok[i], "groove") || IsWord(tok[i], "ridge")
                       || IsWord(tok[i], "inset") || IsWord(tok[i], "outset")) {
                st->colRuleStyle = 1;
            } else if (IsWord(tok[i], "thin") || IsWord(tok[i], "medium") || IsWord(tok[i], "thick")) {
                st->colRuleW = (BYTE)(IsWord(tok[i], "thin") ? 1 : IsWord(tok[i], "medium") ? 3 : 5);
            } else if (ParseLen(tok[i], &c, &px) == 1) {
                st->colRuleW = (BYTE)(px < 0 ? 0 : px > 50 ? 50 : px);
            } else if (CssParseColor(tok[i], &col) == 1) {
                st->colRuleColor  = col;
                st->colRuleHasCol = 1;
            }
        }
        break;
    }
    case P_LINECLAMP:
        k = atoi(v);
        st->lineClamp = (BYTE)(IsWord(v, "none") || k < 0 ? 0 : k > 255 ? 255 : k);
        break;
    case P_TEXTALIGN:
        st->alignBlk = (BYTE)(IsWord(v, "-webkit-center") || IsWord(v, "-moz-center") ? 1 : 0);
        if (IsWord(v, "center") || IsWord(v, "-webkit-center") || IsWord(v, "-moz-center")) st->align = CSS_A_CENTER;
        else if (IsWord(v, "right") || IsWord(v, "end")) st->align = CSS_A_RIGHT;
        else if (IsWord(v, "justify")) st->align = CSS_A_JUSTIFY;
        else if (IsWord(v, "left") || IsWord(v, "start")) st->align = CSS_A_LEFT;
        break;
    case P_WHITESPACE:
        /* pre-wrap は空白を残して折り返す (nowrap にしない)。pre-line は改行だけ残す。分からない値は宣言ごと無視する */
        if (!IsWord(v, "normal") && !IsWord(v, "pre") && !IsWord(v, "nowrap") && !IsWord(v, "pre-wrap")
            && !IsWord(v, "pre-line") && !IsWord(v, "break-spaces")) {
            break;
        }
        st->preLine = (BYTE)IsWord(v, "pre-line");
        st->pre     = (BYTE)(!st->preLine && (strnicmp(v, "pre", 3) == 0 || IsWord(v, "break-spaces")));
        st->nowrap  = (BYTE)(IsWord(v, "nowrap") || IsWord(v, "pre"));
        break;
    case P_DISPLAY:
        k = 1;
        if (IsWord(v, "none")) st->display = CSS_D_NONE;
        else if (IsWord(v, "list-item")) st->display = CSS_D_LISTITEM;
        else if (IsWord(v, "table-cell")) st->display = CSS_D_CELL;
        else if (IsWord(v, "table-row")) st->display = CSS_D_ROW;
        else if (IsWord(v, "table-row-group") || IsWord(v, "table-header-group") || IsWord(v, "table-footer-group")) {
            st->display = CSS_D_ROWGROUP;
        } else if (IsWord(v, "table") || IsWord(v, "inline-table")) st->display = CSS_D_TABLE;
        else if (IsWord(v, "table-column") || IsWord(v, "table-column-group")) st->display = CSS_D_NONE;
        else if (IsWord(v, "table-caption")) st->display = CSS_D_BLOCK;
        else if (IsWord(v, "inline-block") || IsWord(v, "inline-flex") || IsWord(v, "inline-grid")
                 || IsWord(v, "-webkit-inline-box")) {
            st->display = CSS_D_INLINEBLOCK;
        } else if (strnicmp(v, "inline", 6) == 0 || IsWord(v, "contents")) st->display = CSS_D_INLINE;
        else if (IsWord(v, "block") || IsWord(v, "flex") || IsWord(v, "grid") || IsWord(v, "flow-root")
                 || IsWord(v, "-webkit-box") || IsWord(v, "-ms-flexbox")) {
            st->display = CSS_D_BLOCK;
        } else {
            k = 0;
        }
        if (k) {
            st->contents = (BYTE)IsWord(v, "contents");
            st->flex = (BYTE)(IsWord(v, "flex") || IsWord(v, "inline-flex"));   /* 箱は block / inline-block のまま */
            st->grid = (BYTE)(IsWord(v, "grid") || IsWord(v, "inline-grid"));
        }
        break;
    case P_VISIBILITY:
        if (IsWord(v, "hidden") || IsWord(v, "collapse")) st->invisible = 1;
        else if (IsWord(v, "visible")) st->invisible = 0;
        st->visCollapse = (BYTE)IsWord(v, "collapse");
        break;
    case P_LISTSTYLE:
        ListStyle(v, st);
        break;
    case P_LISTTYPE:
        ListType(v, st);
        break;
    case P_LISTPOS:
        st->listInside = (BYTE)IsWord(v, "inside");
        break;
    case P_LISTIMG:
        st->listImg = (short)UrlImage(v);
        break;
    case P_TEXTTRANSFORM:
        st->transform = (BYTE)(IsWord(v, "uppercase") ? 1 : IsWord(v, "lowercase") ? 2 : IsWord(v, "capitalize") ? 3 : 0);
        break;
    case P_TEXTINDENT:
        k = ParseLen(v, &c, &px);
        if (k == 1 || k == 2) {
            if (px > 2000) px = 2000;
            if (px < -20000) px = -20000;
            st->indent    = (short)px;
            st->indentPct = (BYTE)(k == 2);
        }
        break;
    case P_FONTVARIANT:
        st->smallCaps = (BYTE)(StrIStr(v, "small-caps") != NULL);
        break;
    case P_LINEHEIGHT:
        LineHeight(v, &c, st);
        break;
    case P_VALIGN:
        VAlign(v, &c, st);
        break;
    case P_LETTERSP: case P_WORDSP:
        if (IsWord(v, "normal")) {
            px = 0;
        } else if (ParseLen(v, &c, &px) != 1) {
            break;
        }
        if (px < -20) px = -20;
        if (px > 200) px = 200;
        if (prop == P_LETTERSP) st->letterSp = (short)px;
        else st->wordSp = (short)px;
        break;
    case P_BGIMAGE:
        st->bgImg  = (short)UrlImage(v);
        st->bgGrad = GradOf(v);
        BgLayers(v, &c, st, 1);
        break;
    case P_BGCLIP: case P_BGORIGIN: {
        int b = BoxKw(v);
        if (b >= 0 && prop == P_BGCLIP) st->bgClip = (BYTE)b;
        if (b >= 0 && b < 3 && prop == P_BGORIGIN) st->bgOrigin = (BYTE)(b == 0 ? 1 : b == 1 ? 0 : 2);
        break;
    }
    case P_TEXTFILL: {
        /* -webkit-text-fill-color: 透明なら文字を描かない (background-clip: text と組み合わせる)。色なら文字の色 */
        DWORD col;
        k = CssParseColor(v, &col);
        if (k == 2) {
            st->colorClear = 1;
        } else if (k == 1) {
            st->color      = col;
            st->colorT     = (BYTE)(255 - g_colorA);
            st->colorClear = 0;
        }
        break;
    }
    case P_BGSIZE: {
        char tok[4][128];
        int  n = Tokens(v, tok, 4);
        if (n > 0 && strchr(v, ',') == NULL) {
            BgSizeTokens(tok, n, &c, st);
        }
        break;
    }
    case P_RADIUS:
        BorderRadius(v, &c, st);
        break;
    case P_RADIUS_TL: case P_RADIUS_TR: case P_RADIUS_BR: case P_RADIUS_BL: {
        char tok[2][128];
        if (Tokens(v, tok, 2) > 0) {
            RadiusOne(tok[0], &c, &st->radius[prop - P_RADIUS_TL], &st->radiusPct[prop - P_RADIUS_TL]);
        }
        break;
    }
    case P_BOXSHADOW:
        BoxShadow(v, &c, st);
        break;
    case P_WORDBREAK:
        if (IsWord(v, "break-all")) st->breakAll = 1;
        else if (IsWord(v, "normal") || IsWord(v, "keep-all") || IsWord(v, "break-word")) st->breakAll = 0;
        break;
    case P_LINEBREAK:
        if (IsWord(v, "anywhere")) st->breakAll = 1;
        break;
    case P_TABSIZE: {
        /* 空白の数 (長さなら文字の大きさの半分を空白 1 つとみなす) */
        char  *e;
        double d = strtod(v, &e);
        int    n;
        if (e == v || d < 0) break;
        if (*SkipWs(e) != '\0') {
            int px;
            if (ParseLen(v, &c, &px) != 1) break;
            d = st->fontPx > 1 ? px * 2.0 / st->fontPx : px;
        }
        n = (int)(d + 0.5);
        st->tabSize = (BYTE)(n <= 0 ? 255 : n > 64 ? 64 : n);
        break;
    }
    case P_TEXTOVERFLOW:
        st->textEllipsis = (BYTE)(StrIStr(v, "ellipsis") != 0);
        break;
    case P_GRIDCOLS:
        st->gridCols = GridNorm(v, &c);
        break;
    case P_GRIDROWS:
        st->gridRows = GridNorm(v, &c);
        break;
    case P_GRIDAREAS:
        st->gridAreas = IsWord(v, "none") ? NULL : GridAreasNorm(v, v + strlen(v));
        break;
    case P_GRIDTEMPLATE:
        GridTemplate(v, &c, st);
        break;
    case P_GRID: {
        /* grid-template の書き方か、片側に auto-flow を書いたもの */
        const char *sl = ScanTo(v, "/"), *af = FindI(v, "auto-flow");
        st->gridAutoRow  = 0;
        st->gridAutoCols = NULL;
        st->gridFlow     = 0;
        if (af == NULL || *sl != '/') {
            GridTemplate(v, &c, st);
        } else {
            st->gridAreas = NULL;
            if (af < sl) {
                GridAutoFlowSide(v, sl, 0, &c, st);
                st->gridRows = NULL;
                st->gridCols = GridNorm(sl + 1, &c);
            } else {
                GridAutoFlowSide(sl + 1, sl + strlen(sl), 1, &c, st);
                st->gridRows = GridNormN(v, sl, &c);
                st->gridCols = NULL;
            }
        }
        break;
    }
    case P_MASKIMAGE:
        MaskImage(v, st);
        break;
    case P_MASK:
        MaskShorthand(v, &c, st);
        break;
    case P_MASKPOS: case P_MASKSIZE: {
        char tok[4][128];
        int  n = Tokens(v, tok, 4);
        if (n > 0 && strchr(v, ',') == NULL) {
            if (prop == P_MASKPOS) MaskPosTokens(tok, n, &c, st); else MaskSizeTokens(tok, n, &c, st);
        }
        break;
    }
    case P_ROTATE: {
        /* rotate: 角度 (軸を書いたもの "z 45deg" は最後の角度) */
        const char *sp = strrchr(v, ' '), *a = sp != NULL ? sp + 1 : v;
        double      sx = st->xfSX ? st->xfSX / 1000.0 : 1.0, sy = st->xfSY ? st->xfSY / 1000.0 : 1.0;
        SetXf(st, IsWord(v, "none") ? 0.0 : AngleOf(a, a + strlen(a)), sx, sy);
        break;
    }
    case P_SCALE: {
        /* scale: x [y] */
        const char *sp = ScanTo(v, " ");
        double      a;
        if (IsWord(v, "none")) {
            SetXf(st, st->xfRot, 1.0, 1.0);
            break;
        }
        a = ScaleOf(v, sp);
        SetXf(st, st->xfRot, a, *sp ? ScaleOf(sp + 1, sp + strlen(sp)) : a);
        break;
    }
    case P_TRORIGIN: {
        /* transform-origin: background-position と同じ書き方 (3 つ目の z は使わない) */
        CssStyle tmp;
        char     tok[4][128];
        int      n = Tokens(v, tok, 4);
        BgPosTokens(tok, n > 2 ? 2 : n, &c, &tmp);
        st->xfOX    = tmp.bgPos[0];
        st->xfOY    = tmp.bgPos[1];
        st->xfOXP   = tmp.bgPosPct[0];
        st->xfOYP   = tmp.bgPosPct[1];
        st->xfOSet  = 1;
        break;
    }
    case P_OBJFIT:
        st->objFit = (BYTE)(IsWord(v, "contain") ? CSS_OF_CONTAIN : IsWord(v, "cover") ? CSS_OF_COVER
                          : IsWord(v, "none") ? CSS_OF_NONE : IsWord(v, "scale-down") ? CSS_OF_SCALEDOWN : CSS_OF_FILL);
        break;
    case P_OBJPOS: {
        /* background-position と同じ書き方 */
        CssStyle tmp;
        char     tok[4][128];
        BgPosTokens(tok, Tokens(v, tok, 4), &c, &tmp);
        memcpy(st->objPos, tmp.bgPos, sizeof(st->objPos));
        memcpy(st->objPosPct, tmp.bgPosPct, sizeof(st->objPosPct));
        st->objPosSet = 1;
        break;
    }
    case P_GRIDFLOW:
        st->gridFlow = (BYTE)((StrIStr(v, "column") ? 1 : 0) | (StrIStr(v, "dense") ? 2 : 0));
        break;
    case P_GRIDAUTOCOLS:
        st->gridAutoCols = GridNorm(v, &c);
        break;
    case P_GRIDAUTOROWS:
        st->gridAutoRow = (short)(ParseLen(v, &c, &px) == 1 && px > 0 ? (px > 16000 ? 16000 : px) : 0);
        break;
    case P_GRIDCOL:
        GridLine(v, &st->gcStart, &st->gcEnd, &st->gcSpan, st->gcName);
        break;
    case P_GRIDROW:
        GridLine(v, &st->grStart, &st->grEnd, &st->grSpan, st->grName);
        break;
    case P_GRIDCOLSTART: case P_GRIDCOLEND: case P_GRIDROWSTART: case P_GRIDROWEND: {
        short       *line = prop == P_GRIDCOLSTART ? &st->gcStart : prop == P_GRIDCOLEND ? &st->gcEnd
                          : prop == P_GRIDROWSTART ? &st->grStart : &st->grEnd;
        short       *span = prop == P_GRIDCOLSTART || prop == P_GRIDCOLEND ? &st->gcSpan : &st->grSpan;
        const char **name = prop == P_GRIDCOLSTART ? &st->gcName[0] : prop == P_GRIDCOLEND ? &st->gcName[1]
                          : prop == P_GRIDROWSTART ? &st->grName[0] : &st->grName[1];
        *line = 0;
        *name = NULL;
        GridLineOne(v, v + strlen(v), line, span, name);
        break;
    }
    case P_GRIDAREA: {
        /* 行の始まり / 列の始まり / 行の終わり / 列の終わり。書いていない側は、対の側が名前なら同じ名前 */
        const char  *p = v, *sl;
        short       *lines[4], dummy = 0;
        const char **names[4];
        int          i, np = 0;
        lines[0] = &st->grStart; lines[1] = &st->gcStart; lines[2] = &st->grEnd; lines[3] = &st->gcEnd;
        names[0] = &st->grName[0]; names[1] = &st->gcName[0]; names[2] = &st->grName[1]; names[3] = &st->gcName[1];
        st->grStart = st->gcStart = st->grEnd = st->gcEnd = st->grSpan = st->gcSpan = 0;
        st->grName[0] = st->grName[1] = st->gcName[0] = st->gcName[1] = NULL;
        for (i = 0; i < 4 && *p; i++) {
            sl = ScanTo(p, "/");
            GridLineOne(p, sl, lines[i], i == 0 || i == 2 ? &st->grSpan : i == 1 || i == 3 ? &st->gcSpan : &dummy,
                        names[i]);
            np = i + 1;
            if (*sl != '/') break;
            p = sl + 1;
        }
        if (np < 2) st->gcName[0] = st->grName[0];
        if (np < 3) st->grName[1] = st->grName[0];
        if (np < 4) st->gcName[1] = st->gcName[0];
        break;
    }
    case P_INSET: case P_INSET_BLOCK: case P_INSET_INLINE: case P_MARGIN_BLOCK: case P_MARGIN_INLINE:
    case P_PAD_BLOCK: case P_PAD_INLINE: {
        /* 論理的な一括指定 (横書きとして各辺の性質に分ける)。inset は margin と同じく 1～4 個、ほかは 1～2 個 */
        char tok[4][128];
        int  n = Tokens(v, tok, 4), i;
        if (n == 0) break;
        if (prop == P_INSET) {
            static const short sides[4] = { P_TOP, P_RIGHT, P_BOTTOM, P_LEFT };
            for (i = 0; i < 4; i++) {
                int t = n == 1 ? 0 : n == 2 ? i % 2 : n == 3 ? (i == 3 ? 1 : i) : i;
                ApplyDecl(s, sides[i], tok[t], parent, st);
            }
        } else {
            int a = prop == P_INSET_BLOCK ? P_TOP : prop == P_INSET_INLINE ? P_LEFT
                  : prop == P_MARGIN_BLOCK ? P_MARGIN_T : prop == P_MARGIN_INLINE ? P_MARGIN_L
                  : prop == P_PAD_BLOCK ? P_PADDING_T : P_PADDING_L;
            int b = prop == P_INSET_BLOCK ? P_BOTTOM : prop == P_INSET_INLINE ? P_RIGHT
                  : prop == P_MARGIN_BLOCK ? P_MARGIN_B : prop == P_MARGIN_INLINE ? P_MARGIN_R
                  : prop == P_PAD_BLOCK ? P_PADDING_B : P_PADDING_R;
            ApplyDecl(s, a, tok[0], parent, st);
            ApplyDecl(s, b, tok[n > 1 ? 1 : 0], parent, st);
        }
        break;
    }
    case P_CONTTYPE:
        st->containerType = (BYTE)(IsWord(v, "inline-size") || IsWord(v, "size"));
        break;
    case P_CONTNAME: {
        char tok[1][128];
        st->containerName = Tokens(v, tok, 1) > 0 && !IsWord(tok[0], "none") ? CssIntern(tok[0]) : NULL;
        break;
    }
    case P_CONTAINER: {
        /* container: 名前 [/ 種類] */
        const char *sl = ScanTo(v, "/");
        char        nm[128];
        const char *a = v, *b = sl;
        int         k2;
        Trim(&a, &b);
        k2 = (int)(b - a) < 127 ? (int)(b - a) : 127;
        memcpy(nm, a, k2);
        nm[k2] = '\0';
        st->containerName = k2 > 0 && !IsWord(nm, "none") ? CssIntern(nm) : NULL;
        st->containerType = (BYTE)(*sl == '/' && (StrIStr(sl, "inline-size") || StrIStr(sl, "size")));
        break;
    }
    case P_ASPECT: {
        /* aspect-ratio: [auto] 横 [/ 縦] */
        char  tok[4][128];
        char *sv = SpaceSlash(v);
        int   n = Tokens(sv, tok, 4), i, a = 0, b = 100;
        free(sv);
        for (i = 0; i < n; i++) {
            if (IsWord(tok[i], "auto")) continue;
            if (strcmp(tok[i], "/") == 0 && i + 1 < n) {
                b = (int)(strtod(tok[i + 1], NULL) * 100.0 + 0.5);
                break;
            }
            if (a == 0) a = (int)(strtod(tok[i], NULL) * 100.0 + 0.5);
        }
        if (a > 0 && b > 0 && a <= 30000 && b <= 30000) {
            st->arW = (short)a;
            st->arH = (short)b;
        } else if (n == 1 && IsWord(tok[0], "auto")) {
            st->arW = st->arH = 0;
        }
        break;
    }
    case P_TRANSFORM:
        Transform(v, &c, st, 0);
        break;
    case P_TRANSLATE:
        Transform(v, &c, st, 1);
        break;
    case P_ANIMATION:
        Animation(v, st);
        break;
    case P_ANIMNAME: {
        char tok[1][128];
        st->animName = NULL;
        if (Tokens(v, tok, 1) > 0 && !IsWord(tok[0], "none")) {
            char *e = (char *)ScanTo(tok[0], ",");
            *e = '\0';
            st->animName = CssIntern(tok[0]);
        }
        break;
    }
    case P_ANIMFILL:
        st->animFwd = (BYTE)(strnicmp(SkipWs(v), "forwards", 8) == 0 || strnicmp(SkipWs(v), "both", 4) == 0);
        break;
    case P_ANIMITER:
        st->animInf = (BYTE)(strnicmp(SkipWs(v), "infinite", 8) == 0);
        break;
    case P_JUSTITEMS: case P_JUSTSELF:
        if ((k = AlignWord(v)) >= 0) {
            if (prop == P_JUSTITEMS) st->justItems = (BYTE)k; else st->justSelf = (BYTE)k;
        }
        break;
    case P_POINTEREVENTS:
        st->noHit = (BYTE)IsWord(v, "none");
        break;
    case P_PLACECONTENT: {
        /* place-content: 縦 [横] */
        char tok[2][128];
        int  n = Tokens(v, tok, 2), a = n > 0 ? AlignWord(tok[0]) : -1, b = n > 1 ? AlignWord(tok[1]) : a;
        if (a >= 0 && b >= 0) {
            st->alignContent = (BYTE)a;
            st->justify      = (BYTE)b;
        }
        break;
    }
    case P_PLACEITEMS: case P_PLACESELF: {
        /* place-items: 縦 [横] */
        char tok[2][128];
        int  n = Tokens(v, tok, 2), a = n > 0 ? AlignWord(tok[0]) : -1, b = n > 1 ? AlignWord(tok[1]) : a;
        if (a >= 0 && b >= 0) {
            if (prop == P_PLACEITEMS) { st->alignItems = (BYTE)a; st->justItems = (BYTE)b; }
            else { st->alignSelf = (BYTE)a; st->justSelf = (BYTE)b; }
        }
        break;
    }
    case P_BGREPEAT:
        st->bgRepeat = RepeatOf(v);
        break;
    case P_BGATTACH:
        st->bgFixed = (BYTE)IsWord(v, "fixed");
        break;
    case P_BGPOS:
        BgPos(v, &c, st);
        break;
    case P_MARGIN:   Box4(v, &c, st->margin, st->marginPct, 1, 1); break;
    case P_PADDING:  Box4(v, &c, st->padding, st->padPct, 0, 0); break;
    case P_MARGIN_T: case P_MARGIN_R: case P_MARGIN_B: case P_MARGIN_L:
        k = ParseLen(v, &c, &px);
        st->marginPct[prop - P_MARGIN_T] = (short)(k == 2 ? PctX100(v, 1) : 0);
        if (k == 3) px = CSS_AUTO;
        else if (k != 1) px = 0;
        else if (px > 16000) px = 16000;
        else if (px < -16000) px = -16000;
        st->margin[prop - P_MARGIN_T] = (short)px;
        break;
    case P_PADDING_T: case P_PADDING_R: case P_PADDING_B: case P_PADDING_L:
        k = ParseLen(v, &c, &px);
        if (k == 1) {
            st->padding[prop - P_PADDING_T] = (short)(px < 0 ? 0 : px > 16000 ? 16000 : px);
            st->padPct[prop - P_PADDING_T]  = 0;
        } else if (k == 2) {
            st->padding[prop - P_PADDING_T] = 0;
            st->padPct[prop - P_PADDING_T]  = (short)PctX100(v, 0);
        }
        break;
    case P_OUTLINE:
        Outline(v, &c, st);
        break;
    case P_OUTLINE_W:
        if (BorderWidth(v, &c, &px)) st->outlineW = (short)(px > 0 ? px : -1);
        break;
    case P_OUTLINE_S:
        if (IsWord(v, "auto")) st->outlineStyle = CSS_B_SOLID;
        else if (BorderStyle(v, &k)) st->outlineStyle = (BYTE)k;
        break;
    case P_OUTLINE_C:
        st->outlineColor = CssParseColor(v, &col) == 1 ? col : CSS_NOCOLOR;     /* invert は文字色 */
        break;
    case P_OUTLINE_O:
        if (ParseLen(v, &c, &px) == 1) st->outlineOff = (short)(px < -100 ? -100 : px > 100 ? 100 : px);
        break;
    case P_CURSOR:
        if ((k = CursorOf(v)) >= 0) st->cursor = (BYTE)k;
        break;
    case P_EMPTYCELLS:
        st->emptyHide = (BYTE)IsWord(v, "hide");
        break;
    case P_CAPTIONSIDE:
        st->captionBottom = (BYTE)IsWord(v, "bottom");
        break;
    case P_TABLELAYOUT:
        st->tableFixed = (BYTE)IsWord(v, "fixed");
        break;
    case P_FLEX:
        FlexShorthand(v, &c, st);
        break;
    case P_FLEXBASIS:
        if (IsWord(v, "auto") || IsWord(v, "content")) {
            st->basis    = CSS_AUTO;
            st->basisPct = 0;
        } else {
            short w = CSS_UNSET;
            BYTE  pct = 0;
            Width(v, &c, &w, &pct);
            if (w != CSS_UNSET) {
                st->basis    = w;
                st->basisPct = pct;
            }
        }
        break;
    case P_FLEXGROW: case P_FLEXSHRINK:
        if ((k = FlexNumber(v)) >= 0) {
            if (prop == P_FLEXGROW) st->grow = (short)k; else st->shrink = (short)k;
        }
        break;
    case P_FLEXDIR: case P_FLEXWRAP:
        FlexFlowWord(v, st);
        break;
    case P_FLEXFLOW: {
        char tok[32];
        while (NextToken(&v, tok, sizeof(tok))) {
            FlexFlowWord(tok, st);
        }
        break;
    }
    case P_JUSTIFY: case P_ALIGNITEMS: case P_ALIGNSELF: case P_ALIGNCONTENT:
        if ((k = AlignWord(v)) >= 0) {
            if (prop == P_JUSTIFY) st->justify = (BYTE)k;
            else if (prop == P_ALIGNITEMS) st->alignItems = (BYTE)k;
            else if (prop == P_ALIGNSELF) st->alignSelf = (BYTE)k;
            else st->alignContent = (BYTE)k;
        }
        break;
    case P_ORDER: {
        char *e;
        long  n = strtol(v, &e, 10);
        if (e != v && *SkipWs(e) == '\0') {
            st->order = (short)(n > 30000 ? 30000 : n < -30000 ? -30000 : n);
        }
        break;
    }
    case P_GAP: {
        /* gap: 行の間 [列の間] */
        char tok[64];
        const char *p = v;
        if (NextToken(&p, tok, sizeof(tok))) {
            GapValue(tok, &c, &st->gapRow, &st->gapRowPct);
            st->gapCol    = st->gapRow;
            st->gapColPct = st->gapRowPct;
            if (NextToken(&p, tok, sizeof(tok))) {
                GapValue(tok, &c, &st->gapCol, &st->gapColPct);
            }
        }
        break;
    }
    case P_ROWGAP:
        GapValue(v, &c, &st->gapRow, &st->gapRowPct);
        break;
    case P_COLGAP:
        GapValue(v, &c, &st->gapCol, &st->gapColPct);
        break;
    case P_BORDERIMAGE: BorderImage(v, 0, st); break;
    case P_BISOURCE:    BorderImage(v, 1, st); break;
    case P_BISLICE:     BorderImage(v, 2, st); break;
    case P_BIREPEAT:    BorderImage(v, 3, st); break;
    case P_BORDER:   BorderShorthand(v, &c, 15, st); break;
    case P_BORDER_T: case P_BORDER_R: case P_BORDER_B: case P_BORDER_L:
        BorderShorthand(v, &c, 1 << (prop - P_BORDER_T), st);
        break;
    case P_BORDER_WIDTH: Border4(v, &c, 0, st); break;
    case P_BORDER_STYLE: Border4(v, &c, 1, st); break;
    case P_BORDER_COLOR: Border4(v, &c, 2, st); break;
    case P_BW_T: case P_BW_R: case P_BW_B: case P_BW_L:
        if (BorderWidth(v, &c, &px)) st->bwidth[prop - P_BW_T] = (short)px;
        break;
    case P_BS_T: case P_BS_R: case P_BS_B: case P_BS_L:
        if (BorderStyle(v, &k)) st->bstyle[prop - P_BS_T] = (BYTE)k;
        break;
    case P_BC_T: case P_BC_R: case P_BC_B: case P_BC_L:
        if ((k = CssParseColor(v, &col)) == 1) {
            st->bcolor[prop - P_BC_T] = col;
            st->bcT[prop - P_BC_T]    = (BYTE)(255 - g_colorA);
        } else if (k == 2) {
            st->bcolor[prop - P_BC_T] = CSS_TRANSPARENT;
        }
        break;
    case P_WIDTH:
        WidthCalc(v, &c, &st->width, &st->widthPct, &st->addW);
        st->wAuto = (BYTE)IsWord(v, "auto");
        break;
    case P_MAXWIDTH:
        WidthCalc(v, &c, &st->maxWidth, &st->maxWidthPct, &st->addMaxW);
        break;
    case P_HEIGHT:
        st->height = CSS_UNSET;
        st->heightPct = 0;
        WidthCalc(v, &c, &st->height, &st->heightPct, &st->addH);
        st->hAuto = (BYTE)IsWord(v, "auto");
        break;
    case P_MINWIDTH:
        st->minWidth = CSS_UNSET;
        WidthCalc(v, &c, &st->minWidth, &st->minWidthPct, &st->addMinW);
        break;
    case P_MINHEIGHT:
        st->minHeight = CSS_UNSET;
        WidthCalc(v, &c, &st->minHeight, &st->minHeightPct, &st->addMinH);
        break;
    case P_MAXHEIGHT:
        st->maxHeight = CSS_UNSET;
        WidthCalc(v, &c, &st->maxHeight, &st->maxHeightPct, &st->addMaxH);
        break;
    case P_OVERFLOW: case P_OVERFLOW_X: case P_OVERFLOW_Y:
        /* overflow は切り抜き・縦のスクロールに使う (x・y のどちらを書いても)。overflowX は横のスクロール用 */
        st->overflow = (BYTE)(IsWord(v, "hidden") || IsWord(v, "clip") ? CSS_O_HIDDEN
                              : IsWord(v, "scroll") ? CSS_O_SCROLL : IsWord(v, "auto") || IsWord(v, "overlay") ? CSS_O_AUTO
                              : CSS_O_VISIBLE);
        if (prop != P_OVERFLOW_Y) {
            st->overflowX = st->overflow;
        }
        break;
    case P_ZINDEX:
        st->zIndex = (short)(IsWord(v, "auto") ? CSS_AUTO : atoi(v));
        if (st->zIndex < -30000) st->zIndex = -30000;
        break;
    case P_CONTENT:
        st->content = IsWord(v, "none") || IsWord(v, "normal") ? NULL : CssIntern(v);
        break;
    case P_COUNTERRESET:
        st->counterReset = IsWord(v, "none") ? NULL : CssIntern(v);
        break;
    case P_COUNTERINC:
        st->counterInc = IsWord(v, "none") ? NULL : CssIntern(v);
        break;
    case P_QUOTES:
        st->quotes = IsWord(v, "auto") ? NULL : CssIntern(v);
        break;
    case P_BORDERSPACING: {
        char tok[2][128];
        int  n = Tokens(v, tok, 2), a = 0, b;
        if (n >= 1 && ParseLen(tok[0], &c, &a) == 1) {
            b = a;
            if (n == 2 && ParseLen(tok[1], &c, &b) != 1) b = a;
            st->spacing[0] = (short)(a < 0 ? 0 : a > 200 ? 200 : a);
            st->spacing[1] = (short)(b < 0 ? 0 : b > 200 ? 200 : b);
        }
        break;
    }
    case P_BORDERCOLLAPSE:
        st->collapse = (BYTE)IsWord(v, "collapse");
        break;
    case P_FLOAT:
        st->floatSide = (BYTE)(IsWord(v, "left") || IsWord(v, "inline-start") ? 1 :
                               IsWord(v, "right") || IsWord(v, "inline-end") ? 2 : 0);
        break;
    case P_CLEAR:
        st->clear = (BYTE)(IsWord(v, "left") ? 1 : IsWord(v, "right") ? 2 : IsWord(v, "both") ? 3 : 0);
        break;
    case P_BOXSIZING:
        st->borderBox = (BYTE)IsWord(v, "border-box");
        break;
    case P_POSITION:
        st->posAbs = (BYTE)(IsWord(v, "absolute") || IsWord(v, "fixed"));
        st->pos = (BYTE)(IsWord(v, "relative") ? CSS_P_RELATIVE : IsWord(v, "absolute") ? CSS_P_ABSOLUTE
                         : IsWord(v, "fixed") ? CSS_P_FIXED : IsWord(v, "sticky") || IsWord(v, "-webkit-sticky")
                         ? CSS_P_RELATIVE : CSS_P_STATIC);
        st->sticky = (BYTE)(IsWord(v, "sticky") || IsWord(v, "-webkit-sticky"));
        break;
    case P_LEFT: case P_TOP: case P_RIGHT: case P_BOTTOM: {
        int side = prop == P_TOP ? CSS_TOP : prop == P_RIGHT ? CSS_RIGHT : prop == P_BOTTOM ? CSS_BOTTOM : CSS_LEFT;
        k = ParseLen(v, &c, &px);
        if (k == 1 && px <= -500 && (prop == P_LEFT || prop == P_TOP)) st->offscreen = 1;
        if (k == 1 || k == 2) {
            st->off[side]    = (short)(px < -30000 ? -30000 : px > 30000 ? 30000 : px);
            st->offPct[side] = (BYTE)(k == 2);
            st->offAdd[side] = 0;
        } else if (k == 4) {
            /* calc(% ± px): % は整数で持つ */
            st->off[side]    = (short)(g_calcPct < -30000 ? -30000 : g_calcPct > 30000 ? 30000
                                       : (int)(g_calcPct + (g_calcPct < 0 ? -0.5 : 0.5)));
            st->offPct[side] = 1;
            st->offAdd[side] = (short)px;
        } else {
            st->off[side] = CSS_AUTO;
        }
        break;
    }
    case P_CLIP:
        ClipRect(v, &c, st);
        /* 1px に切り抜いて隠す手法 (画面の読み上げ用の文字) */
        if (StrIStr(v, "rect(0") || StrIStr(v, "rect(1px")) st->clipped = 1;
        break;
    case P_CLIPPATH:
        if (StrIStr(v, "inset(50%") || StrIStr(v, "inset(100%") || StrIStr(v, "circle(0")) st->clipped = 1;
        break;
    case P_OPACITY: {
        /* 0 に近いもの: 位置を固定したもの (重ねて置く部品) だけ隠す。普通の流れの中のものは
           JavaScript やアニメーションで後から表示する前提のことが多いので、不透明のまま残す。
           それ以外は、色を下の色と混ぜる (配置のときに) */
        double d = atof(v);
        if (strchr(v, '%') != NULL) d /= 100.0;
        st->clipped  = (BYTE)(st->clipped || d <= 0.01);
        st->opacityT = (BYTE)(d > 0.01 && d < 1.0 ? 255 - (int)(d * 255.0 + 0.5) : 0);
        break;
    }
    }
}

/* 先に当てる性質: 文字の大きさ (em の基準) と色 (同じ要素の currentColor の基準) */
static int IsFontProp(int prop)
{
    return prop == P_FONTSIZE || prop == P_FONT || prop == P_COLOR || prop == P_ALL;
}

typedef struct {
    short prop;
    BYTE  imp;
    char *val;
} InlDecl;

typedef struct {
    InlDecl *d;
    int      n;
    char    *buf;
    int      used;
} InlCtx;

static void InlineDecl(void *ctx, const char *name, const char *vs, const char *ve, int imp)
{
    InlCtx *ic = ctx;
    int     id = PropId(name), len = (int)(ve - vs);
    if (id == P_NONE || ic->n >= MAX_INLINE || ic->used + len + 1 > 4096) {
        return;
    }
    ic->d[ic->n].prop = (short)id;
    ic->d[ic->n].imp  = (BYTE)imp;
    ic->d[ic->n].val  = ic->buf + ic->used;
    memcpy(ic->buf + ic->used, vs, len);
    ic->buf[ic->used + len] = '\0';
    ic->used += len + 1;
    ic->n++;
}

void CssInherit(const CssStyle *parent, CssStyle *st)
{
    int i;
    memset(st, 0, sizeof(*st));
    st->color     = parent->color;
    st->fontPx    = parent->fontPx;
    st->remPx     = parent->remPx;
    st->bold      = parent->bold;
    st->italic    = parent->italic;
    st->under     = parent->under;
    st->strike    = parent->strike;
    st->decoSet   = parent->decoSet;    /* 線の色・線種・太さ・位置 (線と一緒に子へ) */
    st->decoColor = parent->decoColor;
    st->decoHasCol = parent->decoHasCol;
    st->decoStyle = parent->decoStyle;
    st->decoThick = parent->decoThick;
    st->ulOffSet  = parent->ulOffSet;
    st->ulOff     = parent->ulOff;
    st->family    = parent->family;
    st->align     = parent->align;
    st->alignBlk  = parent->alignBlk;
    st->pre       = parent->pre;
    st->listNone  = parent->listNone;
    st->invisible = parent->invisible;
    st->over       = parent->over;
    st->transform  = parent->transform;
    st->nowrap     = parent->nowrap;
    st->smallCaps  = parent->smallCaps;
    st->listType   = parent->listType;
    st->listInside = parent->listInside;
    st->listImg    = parent->listImg;
    st->indent     = parent->indent;
    st->indentPct  = parent->indentPct;
    st->lineMul    = parent->lineMul;
    st->lineH      = parent->lineH;
    st->letterSp   = parent->letterSp;
    st->wordSp     = parent->wordSp;
    memcpy(st->face, parent->face, sizeof(st->face));
    memcpy(st->face2, parent->face2, sizeof(st->face2));
    st->cursor        = parent->cursor;
    st->noHit         = parent->noHit;
    st->preLine       = parent->preLine;
    st->emptyHide     = parent->emptyHide;
    st->captionBottom = parent->captionBottom;
    st->colorClear    = parent->colorClear;
    st->colorT        = parent->colorT;
    st->vars          = parent->vars;   /* カスタムプロパティは受け継ぐ */
    st->cqW           = parent->cqW;    /* cqw の基準の幅 */
    st->breakAll      = parent->breakAll;
    st->tabSize       = parent->tabSize;
    st->shadowOn      = parent->shadowOn;
    st->shadowX       = parent->shadowX;
    st->shadowY       = parent->shadowY;
    st->shadowColor   = parent->shadowColor;
    st->bgPosPct[0] = st->bgPosPct[1] = 1;  /* background-position の初期値は 0% 0% */
    st->display   = CSS_D_INLINE;
    st->bg        = CSS_NOCOLOR;
    for (i = 0; i < 4; i++) {
        st->bwidth[i] = CSS_UNSET;
        st->bcolor[i] = CSS_NOCOLOR;
    }
    st->width    = CSS_UNSET;
    st->maxWidth = CSS_UNSET;
    st->height   = CSS_UNSET;
    st->spacing[0] = parent->spacing[0];
    st->spacing[1] = parent->spacing[1];
    st->collapse   = parent->collapse;
    st->quotes     = parent->quotes;
    for (i = 0; i < 4; i++) {
        st->off[i] = CSS_AUTO;
    }
    st->zIndex    = CSS_AUTO;
    st->minWidth  = CSS_UNSET;
    st->minHeight = CSS_UNSET;
    st->maxHeight = CSS_UNSET;
    st->shrink    = 100;
    st->basis     = CSS_AUTO;
}

/* ---- content などの文字列 (ページを替えるまで持つ) */

static char **g_pool;
static int    g_nPool, g_capPool;

int CssMediaMatch(const CssSheet *s, const char *q)
{
    return EvalMedia(s, q, q + strlen(q));
}

const char *CssIntern(const char *s)
{
    int i;
    for (i = g_nPool - 1; i >= 0 && i >= g_nPool - 256; i--) {
        if (strcmp(g_pool[i], s) == 0) {
            return g_pool[i];
        }
    }
    if (g_nPool >= g_capPool) {
        g_capPool = g_capPool ? g_capPool * 2 : 64;
        g_pool = CssXrealloc(g_pool, g_capPool * sizeof(char *));
    }
    g_pool[g_nPool] = CssXmalloc(strlen(s) + 1);
    strcpy(g_pool[g_nPool], s);
    return g_pool[g_nPool++];
}

void CssSetVarCap(int n)
{
    g_varCap = n < 0 ? 0 : n;
}

void CssPoolReset(void)
{
    int i;
    for (i = 0; i < g_nPool; i++) {
        free(g_pool[i]);
    }
    g_nPool = 0;
    for (i = 0; i < g_nVarBlk; i++) {
        free(g_varBlk[i]);
    }
    g_nVarBlk  = 0;
    g_varUsed  = VAR_BLK;
    g_varCount = 0;
}

/* カスタムプロパティの並びの領域から n バイトを確保する */
static void *VarAlloc(size_t n)
{
    n = (n + 7) & ~(size_t)7;
    if (n > VAR_BLK) {
        return NULL;
    }
    if (g_varUsed + (int)n > VAR_BLK) {
        if (g_nVarBlk >= g_capVarBlk) {
            g_capVarBlk = g_capVarBlk ? g_capVarBlk * 2 : 16;
            g_varBlk = CssXrealloc(g_varBlk, g_capVarBlk * sizeof(char *));
        }
        g_varBlk[g_nVarBlk++] = CssXmalloc(VAR_BLK);
        g_varUsed = 0;
    }
    g_varUsed += (int)n;
    return g_varBlk[g_nVarBlk - 1] + g_varUsed - n;
}

/* 文字列 a..b の写しを並びの領域に作る */
static const char *VarDup(const char *a, const char *b)
{
    size_t n = (size_t)(b - a);
    char  *p = VarAlloc(n + 1);
    if (p == NULL) {
        return NULL;
    }
    memcpy(p, a, n);
    p[n] = '\0';
    return p;
}

/* 要素の並びの前にカスタムプロパティを 1 つ足す (上限を超えたら足さない) */
static void VarPush(CssStyle *st, const char *name, const char *val)
{
    struct CssVarNode *v;
    if (name == NULL || val == NULL || g_varCount >= g_varCap || (v = VarAlloc(sizeof(*v))) == NULL) {
        return;
    }
    v->name = name;
    v->val  = val;
    v->next = st->vars;
    st->vars = v;
    g_varCount++;
}

/* style 属性の --x: 値 を要素の並びに足す (属性の文字列はすぐ無くなるので写す) */
static void InlineVarDecl(void *ctx, const char *name, const char *vs, const char *ve, int imp)
{
    (void)imp;
    if (name[0] == '-' && name[1] == '-' && g_varCount < g_varCap) {
        VarPush((CssStyle *)ctx, VarDup(name, name + strlen(name)), VarDup(vs, ve));
    }
}

/* chain[0] の要素に合うルールの番号を、詳細度・記述順に並べて matched に入れる。数を返す */
static int MatchedRules(const CssSheet *s, const CssElem *chain, int n, int *matched)
{
    int nm = 0, i, j;
    {
        /* 疑似要素のルールは g_wantPe を合わせたときだけ (ふつうは 0 のまま) */
        const CssElem *e = &chain[0];
        Bucket        *b;
        if (e->id && (b = FindBucket(s, '#', e->id)) != NULL) {
            AddCandidates(s, b->ids, b->n, chain, n, matched, &nm);
        }
        if (e->cls) {
            const char *p = e->cls, *q;
            char        w[128];
            while (*p) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == '\0') break;
                q = p;
                while (*q && !isspace((unsigned char)*q)) q++;
                if (q - p < (int)sizeof(w)) {
                    memcpy(w, p, q - p);
                    w[q - p] = '\0';
                    if ((b = FindBucket(s, '.', w)) != NULL) {
                        /* 同じルールが他のクラスの索引にも入ることはない (先頭のクラスだけで索引) */
                        AddCandidates(s, b->ids, b->n, chain, n, matched, &nm);
                    }
                }
                p = q;
            }
        }
        if ((b = FindBucket(s, 't', e->tag)) != NULL) {
            AddCandidates(s, b->ids, b->n, chain, n, matched, &nm);
        }
        AddCandidates(s, s->univ, s->nUniv, chain, n, matched, &nm);
    }
    /* @layer の順位 (層の外が一番強い)、詳細度、同じなら記述順 (= ルール番号) に並べる */
    for (i = 1; i < nm; i++) {
        int x = matched[i], sx = s->rules[x].spec, lx = s->rules[x].layer ? s->rules[x].layer : 0x7FFF;
        for (j = i - 1; j >= 0; j--) {
            int y = matched[j], sy = s->rules[y].spec, ly = s->rules[y].layer ? s->rules[y].layer : 0x7FFF;
            if (ly < lx || (ly == lx && (sy < sx || (sy == sx && y < x)))) break;
            matched[j + 1] = y;
        }
        matched[j + 1] = x;
    }
    return nm;
}

int CssApplyPseudo(const CssSheet *s, const CssElem *chain, int n, int pe, const CssStyle *parent, CssStyle *st)
{
    int matched[MAX_MATCH], nm, i, j, phase, imp;

    if (s == NULL || n <= 0) {
        return 0;
    }
    g_wantPe = pe;
    nm = MatchedRules(s, chain, n, matched);
    g_wantPe = 0;
    g_curVars = st->vars;               /* 要素から受け継いだカスタムプロパティ */
    for (phase = 0; phase < 2; phase++) {
        for (imp = 0; imp < 2; imp++) {
            for (i = 0; i < nm; i++) {
                const Rule *r = &s->rules[matched[i]];
                for (j = 0; j < r->nd; j++) {
                    const Decl *d = &s->decls[r->d0 + j];
                    if (d->imp == imp && IsFontProp(d->prop) == (phase == 0)) {
                        ApplyDecl(s, d->prop, d->val, parent, st);
                    }
                }
            }
        }
    }
    g_curVars = NULL;
    return nm > 0;
}

void CssApply(const CssSheet *s, const CssElem *chain, int n, const char *inl,
              const CssStyle *parent, CssStyle *st)
{
    int     matched[MAX_MATCH], nm, i, j, phase, imp;
    InlDecl idecl[MAX_INLINE];
    char    ibuf[4096];
    InlCtx  ic;

    if (s == NULL) {
        return;
    }
    g_varChain = chain;
    g_varN     = n;
    g_varInl   = inl;
    nm = MatchedRules(s, chain, n, matched);
    ic.d = idecl;
    ic.n = 0;
    ic.buf = ibuf;
    ic.used = 0;
    if (inl != NULL && *inl) {
        ParseDeclText(inl, inl + strlen(inl), InlineDecl, &ic);
    }
    /* カスタムプロパティ: 合ったルールの --x を詳細度の順に、style 属性のものを最後に、親の並びの前に足す
       (後で足したものが勝つ) */
    for (imp = 0; imp < 2; imp++) {
        for (i = 0; i < nm; i++) {
            const Rule *r = &s->rules[matched[i]];
            for (j = 0; j < r->nd; j++) {
                const Decl *d = &s->decls[r->d0 + j];
                if (d->prop == P_VAR && d->imp == imp) {
                    VarPush(st, d->name, d->val);
                }
            }
        }
    }
    if (inl != NULL && strstr(inl, "--") != NULL) {
        ParseDeclText(inl, inl + strlen(inl), InlineVarDecl, st);
    }
    g_curVars = st->vars;
    g_uaDisplay = st->display;
    /* 先に文字の大きさを決めてから (em の基準になる) 他を適用する */
    for (phase = 0; phase < 2; phase++) {
        for (imp = 0; imp < 2; imp++) {
            for (i = 0; i < nm; i++) {
                const Rule *r = &s->rules[matched[i]];
                for (j = 0; j < r->nd; j++) {
                    const Decl *d = &s->decls[r->d0 + j];
                    if (d->imp == imp && IsFontProp(d->prop) == (phase == 0)) {
                        ApplyDecl(s, d->prop, d->val, parent, st);
                    }
                }
            }
            for (i = 0; i < ic.n; i++) {
                if (idecl[i].imp == imp && IsFontProp(idecl[i].prop) == (phase == 0)) {
                    ApplyDecl(s, idecl[i].prop, idecl[i].val, parent, st);
                }
            }
        }
    }
    if (st->animName != NULL && st->animFwd && !st->animInf) {
        /* animation: 動きは描かず、終わった状態 (最後のコマ。fill-mode が forwards / both のもの) を当てる */
        for (i = 0; i < s->nKfs; i++) {
            if (strcmp(s->kfs[i].name, st->animName) == 0) {
                for (j = 0; j < s->kfs[i].nd; j++) {
                    const Decl *d = &s->decls[s->kfs[i].d0 + j];
                    if (d->prop != P_OTHER) {
                        ApplyDecl(s, d->prop, d->val, parent, st);
                    }
                }
                break;
            }
        }
    }
    /* 画面外に出したり切り取ったりして隠している要素 (スクリーンリーダー向けの文字など) */
    if (st->posAbs && (st->offscreen || st->clipped
                       || (st->width != CSS_UNSET && !st->widthPct && st->width <= 1
                           && st->height != CSS_UNSET && !st->heightPct && st->height <= 1))) {
        st->display = CSS_D_NONE;
    }
    if (st->contents) {
        /* display: contents: 自分の箱は作らない (子は親の箱の中に並ぶ)。箱の書式を外したインラインにする */
        int s4;
        st->display      = CSS_D_INLINE;
        st->floatSide    = 0;
        st->pos          = CSS_P_STATIC;
        st->sticky       = 0;
        st->bg           = CSS_NOCOLOR;
        st->bgImg        = 0;
        st->bgGrad       = NULL;
        st->bgList       = NULL;
        st->biImg        = 0;
        st->boxShOn      = 0;
        st->boxShList    = NULL;
        st->outlineStyle = 0;
        st->opacityT     = 0;
        st->hasXf        = 0;
        st->hasFilt      = 0;
        st->maskOn       = 0;
        st->maskImg      = 0;
        st->clipSet      = 0;
        st->overflow     = st->overflowX = 0;
        st->width = st->height = st->minWidth = st->minHeight = st->maxWidth = st->maxHeight = CSS_UNSET;
        for (s4 = 0; s4 < 4; s4++) {
            st->margin[s4]    = st->padding[s4] = 0;
            st->marginPct[s4] = st->padPct[s4] = 0;
            st->bstyle[s4]    = CSS_B_NONE;
        }
    }
    if (st->bgClip == 3) {
        /* background-clip: text: 文字の形に切り抜けないので背景は描かない。文字を透明にしていれば、
           背景 (グラデーションなら最初の色) を文字の色にして読めるようにする */
        if (st->colorClear && st->bg != CSS_NOCOLOR) {
            st->color      = st->bg;
            st->colorT     = 0;
            st->colorClear = 0;
        }
        st->bg     = CSS_NOCOLOR;
        st->bgT    = 0;
        st->bgImg  = 0;
        st->bgGrad = NULL;
        st->bgList = NULL;
    }
    if (st->containerType && st->width != CSS_UNSET && !st->widthPct && st->width > 0) {
        /* container の基準の幅 (子孫の cqw の単位に使う) */
        int w = st->width - (st->borderBox ? st->padding[CSS_LEFT] + st->padding[CSS_RIGHT]
                                             + CssBorder(st, CSS_LEFT) + CssBorder(st, CSS_RIGHT) : 0);
        st->cqW = (short)(w > 0 ? (w > 30000 ? 30000 : w) : 0);
    }
    g_varChain = NULL;
    g_varInl   = NULL;
    g_curVars  = NULL;
}
