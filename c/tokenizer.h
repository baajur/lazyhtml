#ifndef HTML_TOKENIZER_H
#define HTML_TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern const int html_state_error;
extern const int html_state_Data;
extern const int html_state_RCData;
extern const int html_state_RawText;
extern const int html_state_PlainText;
extern const int html_state_ScriptData;

typedef struct {
    const char *data;
    size_t length;
} TokenizerString;

typedef struct {
    bool has_value;
    TokenizerString value;
} TokenizerOptionalString;

typedef enum {
    token_none,
    token_character,
    token_comment,
    token_start_tag,
    token_end_tag,
    token_doc_type
} TokenType;

typedef enum {
    token_character_raw,
    token_character_data,
    token_character_rcdata,
    token_character_cdata,
    token_character_safe
} TokenCharacterKind;

typedef struct {
    TokenCharacterKind kind;
    TokenizerString value;
} TokenCharacter;

typedef struct {
    TokenizerString value;
} TokenComment;

typedef struct {
    TokenizerString name;
    TokenizerString value;
} Attribute;

#define MAX_ATTR_COUNT 256

typedef struct {
    size_t count;
    Attribute items[MAX_ATTR_COUNT];
} TokenAttributes;

typedef enum {
    HTML_TAG_A = 1,
    HTML_TAG_ABBR = 34898,
    HTML_TAG_ADDRESS = 1212749427,
    HTML_TAG_AREA = 51361,
    HTML_TAG_ARTICLE = 1698991493,
    HTML_TAG_ASIDE = 1680517,
    HTML_TAG_AUDIO = 1741103,
    HTML_TAG_B = 2,
    HTML_TAG_BASE = 67173,
    HTML_TAG_BDI = 2185,
    HTML_TAG_BDO = 2191,
    HTML_TAG_BLOCKQUOTE = 84081888640645,
    HTML_TAG_BODY = 81049,
    HTML_TAG_BR = 82,
    HTML_TAG_BUTTON = 89805294,
    HTML_TAG_CANVAS = 102193203,
    HTML_TAG_CAPTION = 3272222190,
    HTML_TAG_CITE = 108165,
    HTML_TAG_CODE = 113797,
    HTML_TAG_COL = 3564,
    HTML_TAG_COLGROUP = 119595941552,
    HTML_TAG_DATA = 132737,
    HTML_TAG_DATALIST = 139185235572,
    HTML_TAG_DD = 132,
    HTML_TAG_DEL = 4268,
    HTML_TAG_DETAILS = 4483753363,
    HTML_TAG_DFN = 4302,
    HTML_TAG_DIALOG = 143700455,
    HTML_TAG_DIV = 4406,
    HTML_TAG_DL = 140,
    HTML_TAG_DT = 148,
    HTML_TAG_EM = 173,
    HTML_TAG_EMBED = 5671076,
    HTML_TAG_FIELDSET = 216002612404,
    HTML_TAG_FIGCAPTION = 221245627573742,
    HTML_TAG_FIGURE = 211015237,
    HTML_TAG_FOOTER = 217567410,
    HTML_TAG_FORM = 212557,
    HTML_TAG_HEAD = 267300,
    HTML_TAG_HEADER = 273715378,
    HTML_TAG_HGROUP = 276381360,
    HTML_TAG_HR = 274,
    HTML_TAG_HTML = 283052,
    HTML_TAG_I = 9,
    HTML_TAG_IFRAME = 308872613,
    HTML_TAG_IMG = 9639,
    HTML_TAG_INPUT = 9913012,
    HTML_TAG_INS = 9683,
    HTML_TAG_KBD = 11332,
    HTML_TAG_KEYGEN = 375168174,
    HTML_TAG_LABEL = 12617900,
    HTML_TAG_LEGEND = 408131012,
    HTML_TAG_LI = 393,
    HTML_TAG_LINK = 402891,
    HTML_TAG_MAIN = 427310,
    HTML_TAG_MAP = 13360,
    HTML_TAG_MARK = 427595,
    HTML_TAG_MATH = 427656,
    HTML_TAG_MENU = 431573,
    HTML_TAG_MENUITEM = 452537405613,
    HTML_TAG_META = 431745,
    HTML_TAG_METER = 13815986,
    HTML_TAG_NAV = 14390,
    HTML_TAG_NOSCRIPT = 497783744020,
    HTML_TAG_OBJECT = 505746548,
    HTML_TAG_OL = 492,
    HTML_TAG_OPTGROUP = 533254979248,
    HTML_TAG_OPTION = 520758766,
    HTML_TAG_OUTPUT = 526009012,
    HTML_TAG_P = 16,
    HTML_TAG_PARAM = 16828461,
    HTML_TAG_PICTURE = 17485682245,
    HTML_TAG_PRE = 16965,
    HTML_TAG_PROGRESS = 569594418803,
    HTML_TAG_Q = 17,
    HTML_TAG_RP = 592,
    HTML_TAG_RT = 596,
    HTML_TAG_RUBY = 611417,
    HTML_TAG_S = 19,
    HTML_TAG_SAMP = 624048,
    HTML_TAG_SCRIPT = 641279508,
    HTML_TAG_SECTION = 20572677614,
    HTML_TAG_SELECT = 643175540,
    HTML_TAG_SLOT = 635380,
    HTML_TAG_SMALL = 20350348,
    HTML_TAG_SOURCE = 653969509,
    HTML_TAG_SPAN = 639022,
    HTML_TAG_STRONG = 659111367,
    HTML_TAG_STYLE = 20604293,
    HTML_TAG_SUB = 20130,
    HTML_TAG_SUMMARY = 21119796825,
    HTML_TAG_SUP = 20144,
    HTML_TAG_SVG = 20167,
    HTML_TAG_TABLE = 21006725,
    HTML_TAG_TBODY = 21052569,
    HTML_TAG_TD = 644,
    HTML_TAG_TEMPLATE = 693016856197,
    HTML_TAG_TEXTAREA = 693389805729,
    HTML_TAG_TFOOT = 21183988,
    HTML_TAG_TH = 648,
    HTML_TAG_THEAD = 21238820,
    HTML_TAG_TIME = 664997,
    HTML_TAG_TITLE = 21287301,
    HTML_TAG_TR = 658,
    HTML_TAG_TRACK = 21562475,
    HTML_TAG_U = 21,
    HTML_TAG_UL = 684,
    HTML_TAG_VAR = 22578,
    HTML_TAG_VIDEO = 23367855,
    HTML_TAG_WBR = 23634
} HtmlTagType;

typedef struct {
    TokenizerString name;
    HtmlTagType type;
    bool self_closing;
    TokenAttributes attributes;
} TokenStartTag;

typedef struct {
    TokenizerString name;
    HtmlTagType type;
} TokenEndTag;

typedef struct {
    TokenizerOptionalString name;
    TokenizerOptionalString public_id;
    TokenizerOptionalString system_id;
    bool force_quirks;
} TokenDocType;

typedef struct {
    TokenType type;
    union {
        TokenCharacter character;
        TokenComment comment;
        TokenStartTag start_tag;
        TokenEndTag end_tag;
        TokenDocType doc_type;
    };
    TokenizerString raw;
    void *extra;
} Token;

typedef void (*TokenHandler)(const Token *token);

typedef struct {
    bool allow_cdata;
    TokenHandler emit_token;
    TokenizerString last_start_tag_name;
    char quote;
    Token token;
    Attribute *attribute;
    const char *start_slice;
    const char *mark;
    const char *appropriate_end_tag_offset;
    char *buffer;
    char *buffer_pos;
    const char *buffer_end;
    int cs;
} TokenizerState;

typedef struct {
    bool allow_cdata;
    TokenHandler on_token;
    TokenizerString last_start_tag_name;
    int initial_state;
    char *buffer;
    size_t buffer_size;
    void *extra;
} TokenizerOpts;

void html_tokenizer_init(TokenizerState *state, const TokenizerOpts *options);
int html_tokenizer_feed(TokenizerState *state, const TokenizerString *chunk);

#endif
