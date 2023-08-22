#ifndef __ASTRUNC_COMMON_H__
#define __ASTRUNC_COMMON_H__

/****
 * @CopyRight (C) 石正贤（Shizhengxian)
 **/

#include <iostream>
#include <string>
#include <vector>

namespace astrunc {
    class common {
        public :
            /** Nothing */
            common(){};
            ~common(){};

        public :
            /** class static function */
            static int chars( std::vector< std::string> &__chars, const std::string &__s);

        /** No attributes */
    };
};


/**
 * @Brief: Get chars list from string，the input string must utf-8 code.
 *       :  eg: __s = "你好啊", output chars table is :
 *       :      __chars[ 0] = "你"
 *       :      __chars[ 1] = "好"
 *       :      __chars[ 2] = "啊"
 *
 *       :  eg: __s = "how are", output chars table is :
 *       :      __chars[ 0] = "h"
 *       :      __chars[ 1] = "o"
 *       :      __chars[ 2] = "w"
 *       :      __chars[ 3] = " "
 *       :      __chars[ 4] = "a"
 *       :      __chars[ 5] = "r"
 *       :      __chars[ 6] = "e"
 *
 * @Param: __chars, Output chars tables.
 * @Param: __s    , Input sentence string.
 *
 * @Return: Ok-> 0, Other-> -1.
 **/
int astrunc::common::chars( std::vector< std::string> &__chars, const std::string &__s)
{
    int rc = 0;

    if ( !__s.empty()) {
        std::size_t i = 0;

        for( i = 0; i < __s.size(); ) {
            unsigned char c = (unsigned char )__s[ i];

            std::size_t c_size = 0;
            if ( (0x00 <= c) && (0xC0 > c)) {
                c_size = 1;

            } else if ( (0xC0 <= c) && (0xE0 > c)) {
                c_size = 2;

            } else if ( (0xe0 <= c) && (0xF0 > c)) {
                c_size = 3;

            } else if ( (0xF0 <= c) && (0xF8 > c)) {
                c_size = 4;

            } else if ( (0xF8 <= c) && (0xFC > c)) {
                c_size = 5;

            } else if ( (0xFE <= c) && (0xFE > c)) {
                c_size = 6;

            } else {
                c_size = 7;
            }

            std::string tmp_word;
            for ( std::size_t j = 0; j < c_size; j++) {
                tmp_word.push_back( __s[ i + j]);
            }

            try {
                __chars.push_back( tmp_word);
            } catch ( std::bad_alloc &__e) {
                rc = -1;

                break;
            }

            i += c_size;
        }
    }

    return rc;
}/// astrunc::common::chars


#endif /** __ASTRUNC_COMMON_H__ */

