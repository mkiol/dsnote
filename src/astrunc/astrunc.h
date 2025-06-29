#ifndef __ASTRUNC_H__
#define __ASTRUNC_H__

/****
 * @CopyRight (C) 石正贤（Shizhengxian)
 **/


#include <iostream>
#include <string>
#include <vector>

#include "astrunc.chars.h"
#include "astrunc.common.h"


/** astrunc namespace */
namespace astrunc {
class access {
   public:
    /** langauge varibale */
    enum lang_t {
        NONE = 0, /** 默认         */
        AF,       /** Dutch         - 荷兰语       */
        AR,       /** Arab          - 阿拉伯       */
        AZ,       /** Azerbaijan    - 阿塞拜疆     */
        BE,       /** Belarus       - 白俄罗斯     */
        BG,       /** Bulgaria      - 保加利亚     */
        BO,       /** Tibetan       - 藏语         */
        BN,       /** Bengali                     */
        CA,       /** Agatha Ronnie - 嘉泰罗尼亚   */
        CS,       /** Czech         - 捷克         */
        DA,       /** Danish        - 丹麦文       */
        DE,       /** German        - 德语         */
        DIV,      /** Maldives      - 马尔代夫     */
        EE,       /** Estonia       - 爱沙尼亚     */
        EL,       /** Greece        - 希腊         */
        EN,       /** English       - 英语         */
        ES,       /** Spain         - 西班牙       */
        ET,       /** Ethiopia      - 埃塞俄比亚   */
        EU,       /** Basque        - 巴斯克       */
        FA,       /** Persia        - 波斯语       */
        FO,       /** Faroe group   - 法罗群岛     */
        FI,       /** Finland       - 芬兰         */
        FR,       /** French        - 法语         */
        GL,       /** Galicia       - 加利西亚     */
        GU,       /** gujarat       - 古吉拉特     */
        HE,       /** Hebrew        - 希伯来       */
        HI,       /** North India   - 北印度       */
        HY,       /** Armenia       - 亚美尼亚     */
        HR,       /** Croatia       - 克罗埃西亚   */
        HU,       /** Hungary       - 匈牙利       */
        ID,       /** Indonesia     - 印尼         */
        IS,       /** Iceland       - 冰岛         */
        IT,       /** Italy         - 意大利       */
        JA,       /** Japanese      - 日语         */
        KA,       /** Georgia       - 格鲁吉亚州   */
        KN,       /** Kannada       - 卡纳达       */
        KK,       /** Kazakh        - 哈萨克       */
        KO,       /** Korean        - 韩语         */
        KY,       /** Kyrgyz        - 吉尔吉斯     */
        LA,       /** Latin         - 拉丁         */
        LV,       /** Latvia        - 拉脱维亚     */
        MK,       /** Macedonia     - 马其顿       */
        ML,       /** Malayalam                   */
        MS,       /** Malay         - 马来         */
        MR,       /** Mara          - 马拉地       */
        MN,       /** Mongolia      - 蒙古         */
        NE,       /** Nepali                      */
        NL,       /** Netherlands   - 荷兰         */
        NO,       /** Norway        - 挪威         */
        PL,       /** poland        - 波兰         */
        PT,       /** Portugal      - 葡萄牙       */
        PA,       /** India         - 印度         */
        RO,       /** Romania       - 罗马尼亚     */
        RU,       /** Russian       - 俄语         */
        RZ,       /** Uzbekistan    - 乌兹别克斯坦 */
        SA,       /** Sanskrit      - 梵文         */
        SQ,       /** Albania       - 阿尔巴尼亚   */
        SK,       /** Slovakia      - 斯洛伐克     */
        SL,       /** Slovenia      - 斯洛文尼亚   */
        SR,       /** Serbia        - 塞尔维亚     */
        SW,       /** Swahili       - 斯瓦希里     */
        SV,       /** Sweden        - 瑞典         */
        SY,       /** Ancient Syria - 叙利亚       */
        TA,       /** Tamil         - 泰米尔       */
        TE,       /** Telugu        - 泰卢固       */
        TH,       /** Thai          - 泰语         */
        TR,       /** Turkey        - 土耳其       */
        TT,       /** Tartar        - 鞑靼         */
        TW,       /** Chinese-tw    - 中文繁体     */
        UK,       /** Ukraine       - 乌克兰       */
        UG,       /** Uygur         - 维吾尔       */
        UR,       /** Urdu          - 乌尔都       */
        VI,       /** Vietnam       - 越南         */
        ZH,       /** Chinese       - 中文         */
        MAX,
    };

   public:
    /** Nothing */
    access(){};
    ~access(){};

    /** Split segment by astruncs */
    static int split(std::vector<std::string> &__vs, const std::string &__seg,
                     astrunc::access::lang_t __lang, std::size_t __nchars);

   private:
    /** Split segment syntax with west langauge */
    static int split_west(std::vector<std::string> &__vs,
                          const std::string &__seg, std::size_t __nchars);

    /** Split segment syntax with esst langauge */
    static int split_east(std::vector<std::string> &__vs,
                          const std::string &__seg, std::size_t __nchars);

    /** Split segment syntax with armenia langauge */
    static int split_armenia(std::vector<std::string> &__vs,
                             const std::string &__seg, std::size_t __nchars);

    /** Split segment syntax with india langauge */
    static int split_india(std::vector<std::string> &__vs,
                           const std::string &__seg, std::size_t __nchars);

    /** astrunc static symbols */
   private:
    /** newline char */
    static const std::string newline;

    /** West language */
    static const std::string west_comma;
    static const std::string west_single_l;
    static const std::string west_single_r;
    static const std::string west_double_l;
    static const std::string west_double_r;
    static const std::string west_sbracket_l;
    static const std::string west_sbracket_r;
    static const std::string west_bracket_l;
    static const std::string west_bracket_r;
    static const std::string west_braces_l;
    static const std::string west_braces_r;
    static const std::string west_book_l;
    static const std::string west_book_r;
    static const std::string west_semicolon;
    static const std::string west_question;
    static const std::string west_excla;
    static const std::string west_dot;
    ;
    /** East language */;
    static const std::string east_comma;
    static const std::string east_double_l;
    static const std::string east_double_r;
    static const std::string east_angle_l;
    static const std::string east_angle_r;
    static const std::string east_sbracket_l;
    static const std::string east_sbracket_r;
    static const std::string east_bracket_l;
    static const std::string east_bracket_r;
    static const std::string east_braces_l;
    static const std::string east_braces_r;
    static const std::string east_semicolon;
    static const std::string east_question;
    static const std::string east_excla;
    static const std::string east_dot;
    ;
    /** India language */;
    static const std::string india_comma;
    static const std::string india_double_l;
    static const std::string india_double_r;
    static const std::string india_angle_l;
    static const std::string india_angle_r;
    static const std::string india_sbracket_l;
    static const std::string india_sbracket_r;
    static const std::string india_bracket_l;
    static const std::string india_bracket_r;
    static const std::string india_braces_l;
    static const std::string india_braces_r;
    static const std::string india_semicolon;
    static const std::string india_question;
    static const std::string india_excla;
    static const std::string india_dot;
    static const std::string india_end;
    ;
    /** Armenia language */;
    static const std::string armenia_comma;
    static const std::string armenia_double_l;
    static const std::string armenia_double_r;
    static const std::string armenia_angle_l;
    static const std::string armenia_angle_r;
    static const std::string armenia_sbracket_l;
    static const std::string armenia_sbracket_r;
    static const std::string armenia_bracket_l;
    static const std::string armenia_bracket_r;
    static const std::string armenia_braces_l;
    static const std::string armenia_braces_r;
    static const std::string armenia_dot;
    static const std::string armenia_question;
    static const std::string armenia_excla;
    static const std::string armenia_end;

    /** End */
}; /** class access  */
}; /** namespace astrunc */


/** newline char */
const std::string astrunc::access::newline = "\n";

/** West language */
const std::string astrunc::access::west_comma      = "，";
const std::string astrunc::access::west_single_l   = "‘";
const std::string astrunc::access::west_single_r   = "’";
const std::string astrunc::access::west_double_l   = "“";
const std::string astrunc::access::west_double_r   = "”";
const std::string astrunc::access::west_sbracket_l = "（";
const std::string astrunc::access::west_sbracket_r = "）";
const std::string astrunc::access::west_bracket_l  = "【";
const std::string astrunc::access::west_bracket_r  = "】";
const std::string astrunc::access::west_braces_l   = "｛";
const std::string astrunc::access::west_braces_r   = "｝";
const std::string astrunc::access::west_book_l     = "《";
const std::string astrunc::access::west_book_r     = "》";
const std::string astrunc::access::west_semicolon  = "；";
const std::string astrunc::access::west_question   = "？";
const std::string astrunc::access::west_excla      = "！";
const std::string astrunc::access::west_dot        = "。";

/** East language */
const std::string astrunc::access::east_comma      = ",";
const std::string astrunc::access::east_double_l   = "\"";
const std::string astrunc::access::east_double_r   = "\"";
const std::string astrunc::access::east_angle_l    = "<";
const std::string astrunc::access::east_angle_r    = ">";
const std::string astrunc::access::east_sbracket_l = "(";
const std::string astrunc::access::east_sbracket_r = ")";
const std::string astrunc::access::east_bracket_l  = "[";
const std::string astrunc::access::east_bracket_r  = "]";
const std::string astrunc::access::east_braces_l   = "{";
const std::string astrunc::access::east_braces_r   = "}";
const std::string astrunc::access::east_semicolon  = ";";
const std::string astrunc::access::east_question   = "?";
const std::string astrunc::access::east_excla      = "!";
const std::string astrunc::access::east_dot        = ".";

/** India language */
const std::string astrunc::access::india_comma      = ",";
const std::string astrunc::access::india_double_l   = "\"";
const std::string astrunc::access::india_double_r   = "\"";
const std::string astrunc::access::india_angle_l    = "<";
const std::string astrunc::access::india_angle_r    = ">";
const std::string astrunc::access::india_sbracket_l = "(";
const std::string astrunc::access::india_sbracket_r = ")";
const std::string astrunc::access::india_bracket_l  = "[";
const std::string astrunc::access::india_bracket_r  = "]";
const std::string astrunc::access::india_braces_l   = "{";
const std::string astrunc::access::india_braces_r   = "}";
const std::string astrunc::access::india_semicolon  = ";";
const std::string astrunc::access::india_question   = "?";
const std::string astrunc::access::india_excla      = "!";
const std::string astrunc::access::india_dot        = ".";
const std::string astrunc::access::india_end        = "।";

/** Armenia language */
const std::string astrunc::access::armenia_comma      = ",";
const std::string astrunc::access::armenia_double_l   = "«";
const std::string astrunc::access::armenia_double_r   = "»";
const std::string astrunc::access::armenia_angle_l    = "<";
const std::string astrunc::access::armenia_angle_r    = ">";
const std::string astrunc::access::armenia_sbracket_l = "(";
const std::string astrunc::access::armenia_sbracket_r = ")";
const std::string astrunc::access::armenia_bracket_l  = "[";
const std::string astrunc::access::armenia_bracket_r  = "]";
const std::string astrunc::access::armenia_braces_l   = "{";
const std::string astrunc::access::armenia_braces_r   = "}";
const std::string astrunc::access::armenia_dot        = ".";
const std::string astrunc::access::armenia_question   = "՞";
const std::string astrunc::access::armenia_excla      = "՛";
const std::string astrunc::access::armenia_end        = ":";


/**
 * @Brief: Split segment content by astruncs.
 *
 * @Param: __vs,     Ouptut astruncs.
 * @Param: __seg,    The segment content.
 * @Param: __lang,   Specify language.
 * @Param: __nchars, Max chars of astruncs.
 *
 * @Return: Ok->0, Other->-1.
 **/
int astrunc::access::split( std::vector< std::string > &__vs, const std::string &__seg, astrunc::access::lang_t __lang, std::size_t __nchars)
{
    int rc = 0;
    if ( 0 >= __nchars) { __nchars = 256; }

    if ( !__seg.empty()) {
        if ( ( astrunc::access::JA == __lang ) ||
             ( astrunc::access::TW == __lang ) ||
             ( astrunc::access::ZH == __lang )
           ) {
            /** West language */
            rc = astrunc::access::split_west( __vs, __seg, __nchars);

        } else if ( (astrunc::access::EN  == __lang) ||
                    (astrunc::access::AF  == __lang) ||
                    (astrunc::access::AZ  == __lang) ||
                    (astrunc::access::BE  == __lang) ||
                    (astrunc::access::BG  == __lang) ||
                    (astrunc::access::CA  == __lang) ||
                    (astrunc::access::CS  == __lang) ||
                    (astrunc::access::DA  == __lang) ||
                    (astrunc::access::DE  == __lang) ||
                    (astrunc::access::DIV == __lang) ||
                    (astrunc::access::EE  == __lang) ||
                    (astrunc::access::EL  == __lang) ||
                    (astrunc::access::ES  == __lang) ||
                    (astrunc::access::ET  == __lang) ||
                    (astrunc::access::EU  == __lang) ||
                    (astrunc::access::FO  == __lang) ||
                    (astrunc::access::FI  == __lang) ||
                    (astrunc::access::FR  == __lang) ||
                    (astrunc::access::GL  == __lang) ||
                    (astrunc::access::GU  == __lang) ||
                    (astrunc::access::HE  == __lang) ||
                    (astrunc::access::HR  == __lang) ||
                    (astrunc::access::HU  == __lang) ||
                    (astrunc::access::ID  == __lang) ||
                    (astrunc::access::IS  == __lang) ||
                    (astrunc::access::IT  == __lang) ||
                    (astrunc::access::KA  == __lang) ||
                    (astrunc::access::KN  == __lang) ||
                    (astrunc::access::KK  == __lang) ||
                    (astrunc::access::KO  == __lang) ||
                    (astrunc::access::KY  == __lang) ||
                    (astrunc::access::LA  == __lang) ||
                    (astrunc::access::LV  == __lang) ||
                    (astrunc::access::MK  == __lang) ||
                    (astrunc::access::ML  == __lang) ||
                    (astrunc::access::MS  == __lang) ||
                    (astrunc::access::MR  == __lang) ||
                    (astrunc::access::NL  == __lang) ||
                    (astrunc::access::NO  == __lang) ||
                    (astrunc::access::PL  == __lang) ||
                    (astrunc::access::PT  == __lang) ||
                    (astrunc::access::RO  == __lang) ||
                    (astrunc::access::RU  == __lang) ||
                    (astrunc::access::RZ  == __lang) ||
                    (astrunc::access::SA  == __lang) ||
                    (astrunc::access::SK  == __lang) ||
                    (astrunc::access::SL  == __lang) ||
                    (astrunc::access::SR  == __lang) ||
                    (astrunc::access::SW  == __lang) ||
                    (astrunc::access::SV  == __lang) ||
                    (astrunc::access::SY  == __lang) ||
                    (astrunc::access::TA  == __lang) ||
                    (astrunc::access::TE  == __lang) ||
                    (astrunc::access::TR  == __lang) ||
                    (astrunc::access::TT  == __lang) ||
                    (astrunc::access::UK  == __lang) ||
                    (astrunc::access::VI  == __lang)
                  ) {
            /** East language */
            rc = astrunc::access::split_east( __vs, __seg, __nchars);

        } else if ( astrunc::access::BO == __lang) {
            /** Tibetan - 藏语 */

        } else if ( (astrunc::access::AR == __lang) ||
                    (astrunc::access::FA == __lang) ||
                    (astrunc::access::UG == __lang) ||
                    (astrunc::access::UR == __lang)
                  ) {
            /** Arab - 阿拉伯, Persia - 波斯语，Uygur - 维语，Urdu - 乌尔都 */
            rc = astrunc::access::split_east( __vs, __seg, __nchars);

        } else if ((astrunc::access::HI == __lang) ||
                   (astrunc::access::PA == __lang) ||
                   (astrunc::access::SA == __lang) ||
                   (astrunc::access::NE == __lang) ||
                   (astrunc::access::BN == __lang)) {
            /** North india - 北印度，India - 印度，Sanskrit - 梵文 */
            rc = astrunc::access::split_india( __vs, __seg, __nchars);

        } else if (astrunc::access::HY == __lang) {
            /** Armenia - 亚美尼亚 */
            rc = astrunc::access::split_armenia( __vs, __seg, __nchars);

        } else if (astrunc::access::TH == __lang) {
            /** Thai - 泰语 */
            rc = astrunc::access::split_east( __vs, __seg, __nchars);

        } else {
            /** Default west langauge split */
            rc = astrunc::access::split_west( __vs, __seg, __nchars);
        }
    }


    return rc;
}/// astrunc::access::split


/**
 * @Brief: Split segment content by astruncs with west language.
 *
 * @Param: __vs,     Ouptut astruncs.
 * @Param: __seg,    The segment content.
 * @Param: __nchars, Max chars of astruncs.
 *
 * @Return: Ok->0, Other->-1.
 **/
int astrunc::access::split_west( std::vector< std::string > &__vs, const std::string &__seg, std::size_t __nchars)
{
    int rc = 0;

    if ( !__seg.empty()) {
        enum state_t {
            AST_START     = 0,
            AST_CONTENT   ,
            AST_SINGLE_L  ,    /** "‘"    */
            AST_SINGLE_W  ,    /** word   */
            AST_SINGLE_R  ,    /** "’"    */
            AST_DOUBLE_L  ,    /** "“"    */
            AST_DOUBLE_W  ,    /** word   */
            AST_DOUBLE_R  ,    /** "”"    */
            AST_SBRACKET_L,    /** "（"   */
            AST_SBRACKET_W,    /** word   */
            AST_SBRACKET_R,    /** "）"   */
            AST_BRACKET_L ,    /** "【"   */
            AST_BRACKET_W ,    /** word   */
            AST_BRACKET_R ,    /** "】"   */
            AST_BRACES_L  ,    /** "｛"   */
            AST_BRACES_W  ,    /** word   */
            AST_BRACES_R  ,    /** "｝"   */
            AST_BOOK_L    ,    /** "《"   */
            AST_BOOK_W    ,    /** word   */
            AST_BOOK_R    ,    /** "》"   */
            AST_SEMI      ,    /** "；"   */
            AST_QUESTION  ,    /** "？"   */
            AST_EXCLA     ,    /** "！"   */
            AST_DOT       ,    /** "。"   */
            AST_SUSP_DOT  ,    /** "。。" */
            AST_NEWLINE   ,    /** "\n"   */
            AST_TRUNC     ,
        } ast_state_v = AST_START;

        /** Get next state */
        auto ast_next_state = []( const std::string &__cs, state_t __ostate) -> state_t {
            state_t rc_state = AST_START;

            if ( __cs == astrunc::access::west_single_l ) {
                rc_state = AST_SINGLE_L;

            } else if ( __cs == astrunc::access::west_double_l) {
                rc_state = AST_DOUBLE_L;

            } else if ( __cs == astrunc::access::west_sbracket_l ) {
                rc_state = AST_SBRACKET_L;

            } else if ( __cs == astrunc::access::west_bracket_l ) { 
                rc_state = AST_BRACKET_L;

            } else if ( __cs == astrunc::access::west_braces_l ) {
                rc_state = AST_BRACES_L;

            } else if ( __cs == astrunc::access::west_book_l ) {
                rc_state = AST_BOOK_L;

            } else if ( __cs == astrunc::access::west_semicolon ) {
                rc_state = AST_SEMI;

            } else if ( __cs == astrunc::access::west_question ) {
                rc_state = AST_QUESTION;

            } else if ( __cs == astrunc::access::west_excla ) {
                rc_state = AST_EXCLA;

            } else if ( __cs == astrunc::access::west_dot ) {
                if ( (AST_DOT == __ostate) || (AST_SUSP_DOT == __ostate)) {
                    rc_state = AST_SUSP_DOT;
                } else {
                    rc_state = AST_DOT;
                }

            } else if ( __cs == astrunc::access::newline ) {
                rc_state = AST_NEWLINE;

            } else {
                rc_state = AST_CONTENT;
            }

            return rc_state;
        };

        /** split east language sentence */
        std::string sentence_s; 
        try {
            /** alloced sentence cache */
            sentence_s.reserve( 256);

        } catch ( std::bad_alloc &__e) {
            /** Nothing */
        }

        std::string cs; {
            cs.reserve( 8);
            cs.clear();
        }

        int area_level = 0;

        astrunc::chars chars_context;
        while ( chars_context.next( __seg, cs)) {
            switch ( ast_state_v ) {
                case AST_START : 
                case AST_CONTENT :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;
                        ast_state_v = ast_next_state( cs, ast_state_v);

                        if ( AST_CONTENT == ast_state_v)  {
                            if ( __nchars < sentence_s.size()) {
                                if ( cs == astrunc::access::west_comma ) {
                                    ast_state_v = AST_TRUNC;
                                }
                            }
                        }

                    } break;
                case AST_SINGLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_single_r) {
                            ast_state_v = AST_SINGLE_R;

                        } else if ( cs == astrunc::access::west_single_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_SINGLE_W;
                        }
                    } break;
                case AST_SINGLE_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_single_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_SINGLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::west_single_l) {
                            ast_state_v = AST_SINGLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_SINGLE_R :
                    {
                        area_level -= 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_single_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_single_l) {
                                ast_state_v = AST_SINGLE_L;

                            } else {
                                ast_state_v = AST_SINGLE_W;
                            }
                        }
                    } break;
                case AST_DOUBLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_double_r) {
                            ast_state_v = AST_DOUBLE_R;

                        } else if ( cs == astrunc::access::west_double_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_DOUBLE_W;
                        }
                    } break;
                case AST_DOUBLE_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_double_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::west_double_l) {
                            ast_state_v = AST_DOUBLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_DOUBLE_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_double_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_double_l) {
                                ast_state_v = AST_DOUBLE_L;

                            } else {
                                ast_state_v = AST_DOUBLE_W;
                            }
                        }
                    } break;
                case AST_SBRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_sbracket_r) {
                            ast_state_v = AST_SBRACKET_R;

                        } else if ( cs == astrunc::access::west_sbracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_SBRACKET_W;
                        }
                    } break;
                case AST_SBRACKET_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_sbracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::west_sbracket_l) {
                            ast_state_v = AST_SBRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_SBRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_sbracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_sbracket_l) {
                                ast_state_v = AST_SBRACKET_L;

                            } else {
                                ast_state_v = AST_SBRACKET_W;
                            }
                        }
                    } break;
                case AST_BRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_bracket_r) {
                            ast_state_v = AST_BRACKET_R;

                        } else if ( cs == astrunc::access::west_bracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACKET_W;
                        }
                    } break;
                case AST_BRACKET_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_bracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACKET_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::west_bracket_l) {
                            ast_state_v = AST_BRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_bracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_bracket_l) {
                                ast_state_v = AST_BRACKET_L;

                            } else {
                                ast_state_v = AST_BRACKET_W;
                            }
                        }
                    } break;
                case AST_BRACES_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_braces_r) {
                            ast_state_v = AST_BRACES_R;

                        } else if ( cs == astrunc::access::west_braces_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACES_W;
                        }
                    } break;
                case AST_BRACES_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_braces_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACES_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::west_braces_l) {
                            ast_state_v = AST_BRACES_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACES_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_braces_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_braces_l) {
                                ast_state_v = AST_BRACES_L;

                            } else {
                                ast_state_v = AST_BRACES_W;
                            }
                        }
                    } break;
                case AST_BOOK_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_book_r) {
                            ast_state_v = AST_BOOK_R;

                        } else if ( cs == astrunc::access::west_book_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BOOK_W;
                        }
                    } break;
                case AST_BOOK_W :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_book_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BOOK_R;
                            } else {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::west_book_l) {
                            ast_state_v = AST_BOOK_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BOOK_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::west_book_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::west_book_l) {
                                ast_state_v = AST_BOOK_L;

                            } else {
                                ast_state_v = AST_BOOK_W;
                            }
                        }
                    } break;
                case AST_DOT :
                    {
                        if ( !(cs == astrunc::access::west_dot) ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        }

                        /** Next */
                        {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_SUSP_DOT :
                    {
                        if ( cs == astrunc::access::west_dot ) {
                            sentence_s += cs;

                        } else {
                            {
                                /** Push output vector< string > */
                                try {
                                    __vs.push_back( sentence_s);

                                } catch ( std::bad_alloc &__e) {
                                    /** Alloced failed */
                                    rc = -1;
                                }

                                /** Clear sentence string */
                                sentence_s.clear();
                            }
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_SEMI :
                    {
                        if ( !(cs == astrunc::access::west_semicolon) ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } 
                        
                        /** Goto next state */
                        {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_QUESTION :
                    {
                        if ( !(cs == astrunc::access::west_question) ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } 
                        
                        /** Goto next state */
                        {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_EXCLA :
                    {
                        if ( !(cs == astrunc::access::west_excla ) ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } 
                        
                        /** Goto next state */
                        {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_NEWLINE :
                    {
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();
                        sentence_s += cs;

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    } break;
                case AST_TRUNC :
                    {
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();
                        sentence_s += cs;

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    }
                default : break;
            }

            if ( 0 != rc) { break; }
        }/// for ( auto const &c : vec_chars)

        if ( 0 == rc) {
            if ( !sentence_s.empty()) {
                try {
                    __vs.push_back( sentence_s);

                } catch ( std::bad_alloc &__e) {
                    rc = -1;
                }
            }

            /** Clear */
            sentence_s.clear();
        }
    }

    return rc;
}/// astrunc::astrunc::split_west


/**
 * @Brief: Split segment content by astruncs with east language.
 *
 * @Param: __vs,     Ouptut astruncs.
 * @Param: __seg,    The segment content.
 * @Param: __nchars, Max chars of astruncs.
 *
 * @Return: Ok->0, Other->-1.
 **/
int astrunc::access::split_east( std::vector< std::string > &__vs, const std::string &__seg, std::size_t __nchars)
{
    int rc = 0;

    if ( !__seg.empty()) {
        enum state_t {
            AST_START   = 0,
            AST_CONTENT   ,
            AST_DOUBLE_L  ,    /** "\""  */
            AST_DOUBLE_C  ,    /** char   */
            AST_DOUBLE_R  ,    /** "\""  */
            AST_ANGLE_L   ,    /** "<"   */
            AST_ANGLE_C   ,    /** char   */
            AST_ANGLE_R   ,    /** ">"   */
            AST_SBRACKET_L,    /** "("   */
            AST_SBRACKET_C,    /** char   */
            AST_SBRACKET_R,    /** ")"   */
            AST_BRACKET_L ,    /** "["   */
            AST_BRACKET_C ,    /** char   */
            AST_BRACKET_R ,    /** "]"   */
            AST_BRACES_L  ,    /** "{"   */
            AST_BRACES_C  ,    /** char   */
            AST_BRACES_R  ,    /** "}"   */
            AST_SEMI      ,    /** ";"   */
            AST_DOT       ,    /** "."   */
            AST_DOT_BLANK ,    /** " "   */
            AST_QUESTION  ,    /** "?"   */
            AST_EXCLA     ,    /** "!"   */
            AST_SUSP_DOT  ,    /** "..." */
            AST_NEWLINE   ,    /** "\n"  */
            AST_TRUNC     ,
        } ast_state_v = AST_START;

        /** Get next state */
        auto ast_next_state = []( const std::string &__cs, state_t __ostate) -> state_t {
            state_t rc_state = AST_START;

            if ( __cs == astrunc::access::east_double_l) {
                rc_state = AST_DOUBLE_L;

            } else if ( __cs == astrunc::access::east_angle_l ) {
                rc_state = AST_ANGLE_L;

            } else if ( __cs == astrunc::access::east_sbracket_l ) {
                rc_state = AST_SBRACKET_L;

            } else if ( __cs == astrunc::access::east_bracket_l ) { 
                rc_state = AST_BRACKET_L;

            } else if ( __cs == astrunc::access::east_braces_l ) {
                rc_state = AST_BRACES_L;

            } else if ( __cs == astrunc::access::east_semicolon ) {
                rc_state = AST_SEMI;

            } else if ( __cs == astrunc::access::east_question ) {
                rc_state = AST_QUESTION;

            } else if ( __cs == astrunc::access::east_excla ) {
                rc_state = AST_EXCLA;

            } else if ( __cs == astrunc::access::east_dot ) {
                if ( (AST_DOT == __ostate) || (AST_SUSP_DOT == __ostate)) {
                    rc_state = AST_SUSP_DOT;
                } else {
                    rc_state = AST_DOT;
                }
            } else if ( __cs == astrunc::access::newline ) {
                rc_state = AST_NEWLINE;

            } else {
                rc_state = AST_CONTENT;
            }

            return rc_state;
        };

        /** split east language sentence */
        std::string sentence_s; 
        try {
            /** alloced sentence cache */
            sentence_s.reserve( 256);

        } catch ( std::bad_alloc &__e) {
            /** Nothing */
        }

        std::string cs; {
            cs.reserve( 8);
            cs.clear();
        }

        int area_level = 0;

        astrunc::chars chars_context;
        while ( chars_context.next( __seg, cs)) {
            switch ( ast_state_v ) {
                case AST_START : 
                case AST_CONTENT :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;
                        ast_state_v = ast_next_state( cs, ast_state_v);

                        if ( AST_CONTENT == ast_state_v) {
                            if ( __nchars < sentence_s.size()) {
                                if ( cs == astrunc::access::east_comma ) {
                                    ast_state_v = AST_TRUNC;
                                }
                            }
                        }

                    } break;
                case AST_DOUBLE_L :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_double_r) {
                            ast_state_v = AST_DOUBLE_R;

                        } else {
                            ast_state_v = AST_DOUBLE_C;
                        }
                    } break;
                case AST_DOUBLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_double_r) {
                            ast_state_v = AST_CONTENT;
                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_DOUBLE_R :
                    {
                        /** Nothing */
                    } break;
                case AST_ANGLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_angle_r) {
                            ast_state_v = AST_ANGLE_R;

                        } else if ( cs == astrunc::access::east_angle_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_ANGLE_C;
                        }
                    } break;
                case AST_ANGLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_angle_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_ANGLE_R;
                            } else {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::east_angle_l) {
                            ast_state_v = AST_ANGLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_ANGLE_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_angle_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::east_angle_l) {
                                ast_state_v = AST_ANGLE_L;

                            } else {
                                ast_state_v = AST_ANGLE_C;
                            }
                        }
                    } break;
                case AST_SBRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_sbracket_r) {
                            ast_state_v = AST_SBRACKET_R;

                        } else if ( cs == astrunc::access::east_sbracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_SBRACKET_C;
                        }
                    } break;
                case AST_SBRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_sbracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::east_sbracket_l) {
                            ast_state_v = AST_SBRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_SBRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_sbracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::east_sbracket_l) {
                                ast_state_v = AST_SBRACKET_L;

                            } else {
                                ast_state_v = AST_SBRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_bracket_r) {
                            ast_state_v = AST_BRACKET_R;

                        } else if ( cs == astrunc::access::east_bracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACKET_C;
                        }
                    } break;
                case AST_BRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_bracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACKET_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::east_bracket_l) {
                            ast_state_v = AST_BRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_bracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::east_bracket_l) {
                                ast_state_v = AST_BRACKET_L;

                            } else {
                                ast_state_v = AST_BRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACES_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_braces_r) {
                            ast_state_v = AST_BRACES_R;

                        } else if ( cs == astrunc::access::east_braces_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACES_C;
                        }
                    } break;
                case AST_BRACES_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_braces_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACES_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::east_braces_l) {
                            ast_state_v = AST_BRACES_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACES_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::east_braces_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::east_braces_l) {
                                ast_state_v = AST_BRACES_L;

                            } else {
                                ast_state_v = AST_BRACES_C;
                            }
                        }
                    } break;
                case AST_DOT :
                    {
                        if ( (cs == " ") || (cs == "\t") || (cs == "\v")) {
                            sentence_s += cs;

                            ast_state_v = AST_DOT_BLANK;
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_DOT_BLANK :
                    {
                        if (std::isupper(cs[0]) || !std::islower(cs[0])) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();

                            sentence_s += cs;
                            ast_state_v = ast_next_state( cs, ast_state_v);
                        } else {
                            sentence_s += cs;

                            if ( (cs == " ") || (cs == "\t") || (cs == "\v")) {
                                /** Pass */
                            } else {
                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        }
                    } break;
                case AST_SUSP_DOT :
                    {
                        if ( cs == astrunc::access::east_dot ) {
                            sentence_s += cs;

                        } else {
                            if ( (cs == " " ) || (cs == "\t") || (cs == "\v")) {
                                sentence_s += cs;

                                ast_state_v = AST_DOT_BLANK;
                            } else {
                                sentence_s += cs;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        }
                    } break;
                case AST_SEMI     :
                case AST_QUESTION :
                case AST_EXCLA    :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_NEWLINE :
                    {
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();
                        sentence_s += cs;

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    } break;
                case AST_TRUNC :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    }
                default : break;
            }

            if ( 0 != rc) { break; }
        }/// while next word-char

        if ( 0 == rc) {
            if ( !sentence_s.empty()) {
                try {
                    __vs.push_back( sentence_s);

                } catch ( std::bad_alloc &__e) {
                    rc = -1;
                }
            }

            /** Clear */
            sentence_s.clear();
        }
    }

    return rc;
}/// astrunc::access::split_east


/**
 * @Brief: Split segment content by astruncs with india language.
 *
 * @Param: __vs,     Ouptut astruncs.
 * @Param: __seg,    The segment content.
 * @Param: __nchars, Max chars of astruncs.
 *
 * @Return: Ok->0, Other->-1.
 **/
int astrunc::access::split_india( std::vector< std::string > &__vs, const std::string &__seg, std::size_t __nchars)
{
    int rc = 0;

    if ( !__seg.empty()) {
        enum state_t {
            AST_START   = 0,
            AST_CONTENT   ,
            AST_DOUBLE_L  ,    /** "\""  */
            AST_DOUBLE_C  ,    /** char   */
            AST_DOUBLE_R  ,    /** "\""  */
            AST_ANGLE_L   ,    /** "<"   */
            AST_ANGLE_C   ,    /** char   */
            AST_ANGLE_R   ,    /** ">"   */
            AST_SBRACKET_L,    /** "("   */
            AST_SBRACKET_C,    /** char   */
            AST_SBRACKET_R,    /** ")"   */
            AST_BRACKET_L ,    /** "["   */
            AST_BRACKET_C ,    /** char   */
            AST_BRACKET_R ,    /** "]"   */
            AST_BRACES_L  ,    /** "{"   */
            AST_BRACES_C  ,    /** char   */
            AST_BRACES_R  ,    /** "}"   */
            AST_SEMI      ,    /** ";"   */
            AST_DOT       ,    /** "."   */
            AST_QUESTION  ,    /** "?"   */
            AST_EXCLA     ,    /** "!"   */
            AST_SUSP_DOT  ,    /** "..." */
            AST_NEWLINE   ,    /** "\n"  */
            AST_END       ,    /** End   */
            AST_TRUNC     ,
        } ast_state_v = AST_START;

        /** Get next state */
        auto ast_next_state = []( const std::string &__cs, state_t __ostate) -> state_t {
            state_t rc_state = AST_START;

            if ( __cs == astrunc::access::india_double_l) {
                rc_state = AST_DOUBLE_L;

            } else if ( __cs == astrunc::access::india_angle_l ) {
                rc_state = AST_ANGLE_L;

            } else if ( __cs == astrunc::access::india_sbracket_l ) {
                rc_state = AST_SBRACKET_L;

            } else if ( __cs == astrunc::access::india_bracket_l ) { 
                rc_state = AST_BRACKET_L;

            } else if ( __cs == astrunc::access::india_braces_l ) {
                rc_state = AST_BRACES_L;

            } else if ( __cs == astrunc::access::india_semicolon ) {
                rc_state = AST_SEMI;

            } else if ( __cs == astrunc::access::india_question ) {
                rc_state = AST_QUESTION;

            } else if ( __cs == astrunc::access::india_excla ) {
                rc_state = AST_EXCLA;

            } else if ( __cs == astrunc::access::india_dot ) {
                if ( (AST_DOT == __ostate) || (AST_SUSP_DOT == __ostate)) {
                    rc_state = AST_SUSP_DOT;
                } else {
                    rc_state = AST_DOT;
                }

            } else if ( __cs == astrunc::access::india_end ) {
                rc_state = AST_END;

            } else if ( __cs == astrunc::access::newline ) {
                rc_state = AST_NEWLINE;

            } else {
                rc_state = AST_CONTENT;
            }

            return rc_state;
        };

        /** split east language sentence */
        std::string sentence_s; 
        try {
            /** alloced sentence cache */
            sentence_s.reserve( 256);

        } catch ( std::bad_alloc &__e) {
            /** Nothing */
        }

        std::string cs; {
            cs.reserve( 8);
            cs.clear();
        }

        int area_level = 0;

        astrunc::chars chars_context;
        while ( chars_context.next( __seg, cs)) {
            switch ( ast_state_v ) {
                case AST_START : 
                case AST_CONTENT :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;
                        ast_state_v = ast_next_state( cs, ast_state_v);

                        if ( AST_CONTENT == ast_state_v) {
                            if ( __nchars < sentence_s.size()) {
                                if ( cs == astrunc::access::india_comma ) {
                                    ast_state_v = AST_TRUNC;
                                }
                            }
                        }

                    } break;
                case AST_DOUBLE_L :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_double_r) {
                            ast_state_v = AST_DOUBLE_R;

                        } else {
                            ast_state_v = AST_DOUBLE_C;
                        }
                    } break;
                case AST_DOUBLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_double_r) {
                            ast_state_v = AST_CONTENT;
                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_DOUBLE_R :
                    {
                        /** Nothing */
                    } break;
                case AST_ANGLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_angle_r) {
                            ast_state_v = AST_ANGLE_R;

                        } else if ( cs == astrunc::access::india_angle_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_ANGLE_C;
                        }
                    } break;
                case AST_ANGLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_angle_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_ANGLE_R;
                            } else {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::india_angle_l) {
                            ast_state_v = AST_ANGLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_ANGLE_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_angle_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::india_angle_l) {
                                ast_state_v = AST_ANGLE_L;

                            } else {
                                ast_state_v = AST_ANGLE_C;
                            }
                        }
                    } break;
                case AST_SBRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_sbracket_r) {
                            ast_state_v = AST_SBRACKET_R;

                        } else if ( cs == astrunc::access::india_sbracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_SBRACKET_C;
                        }
                    } break;
                case AST_SBRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_sbracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::india_sbracket_l) {
                            ast_state_v = AST_SBRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_SBRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_sbracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::india_sbracket_l) {
                                ast_state_v = AST_SBRACKET_L;

                            } else {
                                ast_state_v = AST_SBRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_bracket_r) {
                            ast_state_v = AST_BRACKET_R;

                        } else if ( cs == astrunc::access::india_bracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACKET_C;
                        }
                    } break;
                case AST_BRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_bracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACKET_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::india_bracket_l) {
                            ast_state_v = AST_BRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_bracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::india_bracket_l) {
                                ast_state_v = AST_BRACKET_L;

                            } else {
                                ast_state_v = AST_BRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACES_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_braces_r) {
                            ast_state_v = AST_BRACES_R;

                        } else if ( cs == astrunc::access::india_braces_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACES_C;
                        }
                    } break;
                case AST_BRACES_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_braces_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACES_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::india_braces_l) {
                            ast_state_v = AST_BRACES_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACES_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::india_braces_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::india_braces_l) {
                                ast_state_v = AST_BRACES_L;

                            } else {
                                ast_state_v = AST_BRACES_C;
                            }
                        }
                    } break;
                case AST_DOT :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_SUSP_DOT :
                    {
                        if ( cs == astrunc::access::india_dot ) {
                            sentence_s += cs;

                        } else {
                            if ( cs == " " ) {
                                /** Push output vector< string > */
                                try {
                                    __vs.push_back( sentence_s);

                                } catch ( std::bad_alloc &__e) {
                                    /** Alloced failed */
                                    rc = -1;
                                }

                                /** Clear sentence string */
                                sentence_s.clear();
                            }
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_SEMI     :
                case AST_QUESTION :
                case AST_EXCLA    :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_END     :
                case AST_NEWLINE :
                    {
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();
                        sentence_s += cs;

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    } break;
                case AST_TRUNC :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    }
                default : break;
            }

            if ( 0 != rc) { break; }
        }/// for ( auto const &c : vec_chars)

        if ( 0 == rc) {
            if ( !sentence_s.empty()) {
                try {
                    __vs.push_back( sentence_s);

                } catch ( std::bad_alloc &__e) {
                    rc = -1;
                }
            }

            /** Clear */
            sentence_s.clear();
        }
    }

    return rc;
}/// astrunc::access::split_india


/**
 * @Brief: Split segment content by astruncs with armenia language.
 *
 * @Param: __vs,     Ouptut astruncs.
 * @Param: __seg,    The segment content.
 * @Param: __nchars, Max chars of astruncs.
 *
 * @Return: Ok->0, Other->-1.
 **/
int astrunc::access::split_armenia( std::vector< std::string > &__vs, const std::string &__seg, std::size_t __nchars)
{
    int rc = 0;

    if ( !__seg.empty()) {
        enum state_t {
            AST_START   = 0,
            AST_CONTENT   ,
            AST_DOUBLE_L  ,    /** "«"   */
            AST_DOUBLE_C  ,    /** char   */
            AST_DOUBLE_R  ,    /** "»"   */
            AST_ANGLE_L   ,    /** "<"   */
            AST_ANGLE_C   ,    /** char   */
            AST_ANGLE_R   ,    /** ">"   */
            AST_SBRACKET_L,    /** "("   */
            AST_SBRACKET_C,    /** char   */
            AST_SBRACKET_R,    /** ")"   */
            AST_BRACKET_L ,    /** "["   */
            AST_BRACKET_C ,    /** char   */
            AST_BRACKET_R ,    /** "]"   */
            AST_BRACES_L  ,    /** "{"   */
            AST_BRACES_C  ,    /** char   */
            AST_BRACES_R  ,    /** "}"   */
            AST_DOT       ,    /** "."   */
            AST_QUESTION  ,    /** "՞"   */
            AST_EXCLA     ,    /** "՛"   */
            AST_SUSP_DOT  ,    /** "..." */
            AST_END       ,    /** ":"   */
            AST_NEWLINE   ,    /** "\n"  */
            AST_TRUNC     ,
        } ast_state_v = AST_START;

        /** Get next state */
        auto ast_next_state = []( const std::string &__cs, state_t __ostate) -> state_t {
            state_t rc_state = AST_START;

            if ( __cs == astrunc::access::armenia_double_l) {
                rc_state = AST_DOUBLE_L;

            } else if ( __cs == astrunc::access::east_angle_l ) {
                rc_state = AST_ANGLE_L;

            } else if ( __cs == astrunc::access::armenia_sbracket_l ) {
                rc_state = AST_SBRACKET_L;

            } else if ( __cs == astrunc::access::armenia_bracket_l ) { 
                rc_state = AST_BRACKET_L;

            } else if ( __cs == astrunc::access::armenia_braces_l ) {
                rc_state = AST_BRACES_L;

            } else if ( __cs == astrunc::access::armenia_question ) {
                rc_state = AST_QUESTION;

            } else if ( __cs == astrunc::access::armenia_excla ) {
                rc_state = AST_EXCLA;

            } else if ( __cs == astrunc::access::armenia_dot ) {
                if ( (AST_DOT == __ostate) || (AST_SUSP_DOT == __ostate)) {
                    rc_state = AST_SUSP_DOT;
                } else {
                    rc_state = AST_DOT;
                }

            } else if ( __cs == astrunc::access::armenia_end ) {
                rc_state = AST_END;

            } else if ( __cs == astrunc::access::newline ) {
                rc_state = AST_NEWLINE;

            } else {
                rc_state = AST_CONTENT;
            }

            return rc_state;
        };

        /** split east language sentence */
        std::string sentence_s; 
        try {
            /** alloced sentence cache */
            sentence_s.reserve( 256);

        } catch ( std::bad_alloc &__e) {
            /** Nothing */
        }

        std::string cs; {
            cs.reserve( 8);
            cs.clear();
        }

        int area_level = 0;

        astrunc::chars chars_context;
        while ( chars_context.next( __seg, cs)) {
            switch ( ast_state_v ) {
                case AST_START : 
                case AST_CONTENT :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;
                        ast_state_v = ast_next_state( cs, ast_state_v);

                        if ( AST_CONTENT == ast_state_v) {
                            if ( __nchars < sentence_s.size()) {
                                if ( cs == astrunc::access::armenia_comma ) {
                                    ast_state_v = AST_TRUNC;
                                }
                            }
                        }

                    } break;
                case AST_DOUBLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_double_r) {
                            ast_state_v = AST_DOUBLE_R;

                        } else if ( cs == astrunc::access::armenia_double_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_DOUBLE_C;
                        }
                    } break;
                case AST_DOUBLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_double_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::armenia_double_l) {
                            ast_state_v = AST_DOUBLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_DOUBLE_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_double_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::armenia_double_l) {
                                ast_state_v = AST_DOUBLE_L;

                            } else {
                                ast_state_v = AST_DOUBLE_C;
                            }
                        }
                    } break;
                case AST_ANGLE_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_angle_r) {
                            ast_state_v = AST_ANGLE_R;

                        } else if ( cs == astrunc::access::armenia_angle_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_ANGLE_C;
                        }
                    } break;
                case AST_ANGLE_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_angle_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_ANGLE_R;
                            } else {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::armenia_angle_l) {
                            ast_state_v = AST_ANGLE_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_ANGLE_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_angle_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::armenia_angle_l) {
                                ast_state_v = AST_ANGLE_L;

                            } else {
                                ast_state_v = AST_ANGLE_C;
                            }
                        }
                    } break;
                case AST_SBRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_sbracket_r) {
                            ast_state_v = AST_SBRACKET_R;

                        } else if ( cs == astrunc::access::armenia_sbracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_SBRACKET_C;
                        }
                    } break;
                case AST_SBRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_sbracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_DOUBLE_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else if ( cs == astrunc::access::armenia_sbracket_l) {
                            ast_state_v = AST_SBRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_SBRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_sbracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::armenia_sbracket_l) {
                                ast_state_v = AST_SBRACKET_L;

                            } else {
                                ast_state_v = AST_SBRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACKET_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_bracket_r) {
                            ast_state_v = AST_BRACKET_R;

                        } else if ( cs == astrunc::access::armenia_bracket_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACKET_C;
                        }
                    } break;
                case AST_BRACKET_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_bracket_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACKET_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::armenia_bracket_l) {
                            ast_state_v = AST_BRACKET_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACKET_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_bracket_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::armenia_bracket_l) {
                                ast_state_v = AST_BRACKET_L;

                            } else {
                                ast_state_v = AST_BRACKET_C;
                            }
                        }
                    } break;
                case AST_BRACES_L :
                    {
                        area_level += 1;

                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_braces_r) {
                            ast_state_v = AST_BRACES_R;

                        } else if ( cs == astrunc::access::armenia_braces_l) {
                            /** Pass */
                        } else {
                            ast_state_v = AST_BRACES_C;
                        }
                    } break;
                case AST_BRACES_C :
                    {
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_braces_r) {
                            area_level -= 1;

                            if ( 0 < area_level) {
                                ast_state_v = AST_BRACES_R;

                            } else {
                                area_level  = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }

                        } else if ( cs == astrunc::access::armenia_braces_l) {
                            ast_state_v = AST_BRACES_L;

                        } else {
                            /** Pass */
                        }
                    } break;
                case AST_BRACES_R :
                    {
                        /** Push back sentence content */
                        sentence_s += cs;

                        if ( cs == astrunc::access::armenia_braces_r) {
                            area_level -= 1;

                            if ( 0 >= area_level) {
                                area_level = 0;

                                ast_state_v = ast_next_state( cs, ast_state_v);
                            }
                        } else {
                            if ( cs == astrunc::access::armenia_braces_l) {
                                ast_state_v = AST_BRACES_L;

                            } else {
                                ast_state_v = AST_BRACES_C;
                            }
                        }
                    } break;
                case AST_DOT :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_SUSP_DOT :
                    {
                        if ( cs == astrunc::access::armenia_dot ) {
                            sentence_s += cs;

                        } else {
                            if ( cs == " " ) {
                                /** Push output vector< string > */
                                try {
                                    __vs.push_back( sentence_s);

                                } catch ( std::bad_alloc &__e) {
                                    /** Alloced failed */
                                    rc = -1;
                                }

                                /** Clear sentence string */
                                sentence_s.clear();
                            }
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    } break;
                case AST_QUESTION :
                case AST_EXCLA    :
                    {
                        sentence_s += cs;
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    } break;
                case AST_END     :
                case AST_NEWLINE :
                    {
                        /** Push output vector< string > */
                        try {
                            __vs.push_back( sentence_s);

                        } catch ( std::bad_alloc &__e) {
                            /** Alloced failed */
                            rc = -1;
                        }

                        /** Clear sentence string */
                        sentence_s.clear();
                        sentence_s += cs;

                        ast_state_v = ast_next_state( cs, ast_state_v);
                    } break;
                case AST_TRUNC :
                    {
                        if ( cs == " " ) {
                            /** Push output vector< string > */
                            try {
                                __vs.push_back( sentence_s);

                            } catch ( std::bad_alloc &__e) {
                                /** Alloced failed */
                                rc = -1;
                            }

                            /** Clear sentence string */
                            sentence_s.clear();
                        } else {
                            sentence_s += cs;

                            ast_state_v = ast_next_state( cs, ast_state_v);
                        }
                    }
                default : break;
            } /// switch

            if ( 0 != rc) { break; }
        }/// while ( chars_context.next( __seg, cs))

        if ( 0 == rc) {
            if ( !sentence_s.empty()) {
                try {
                    __vs.push_back( sentence_s);

                } catch ( std::bad_alloc &__e) {
                    rc = -1;
                }
            }

            /** Clear */
            sentence_s.clear();
        }
    }

    return rc;
}/// astrunc::access::split_armenia


#endif /** __ASTRUNC_H__ */


