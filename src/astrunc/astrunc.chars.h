#ifndef __ASTRUNC_CHARS_H__
#define __ASTRUNC_CHARS_H__

/****
 * @CopyRight (C) 石正贤（Shizhengxian)
 **/

#include <iostream>
#include <string>
#include <vector>


/** Namespace astrunc */
namespace astrunc {
    /** Class chars */
    class chars {
        public :
            /** Nothing */
            chars(): pos__( 0) {};
            ~chars(){};

        public :
            /** class static function */
            bool next( const std::string &__s, std::string &__cs);

            /** Reset pos */
            void reset( void);

        /** No attributes */
        public :
            std::size_t pos__;
    };
};


/**
 * @Brief: Get char string from string，the input string must utf-8 code.
 *
 * @Param: __s , Input sentence string.
 * @Param: __sc, Output char string.
 *
 * @Return: Ok-> 0, Other-> -1.
 **/
bool astrunc::chars::next( const std::string &__s, std::string &__cs)
{
    bool rc = false;
    __cs.clear();

    if ( !__s.empty() && (this->pos__ < __s.size())) {
        std::size_t i      = this->pos__;
        std::size_t c_size = 0;

        unsigned char c = (unsigned char )__s[ i];

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

        for ( std::size_t j = 0; j < c_size; j++) {
            __cs.push_back( __s[ i + j]);
        }

        this->pos__ += c_size;

        rc = true;
    }

    return rc;
}/// astrunc::chars::next


/**
 * @Brief: Reset object.
 *
 * @Param: Nothing.
 *
 * @Return: Nothing.
 **/
void astrunc::chars::reset( void) 
{
    this->pos__ = 0;
}/// astrunc::chars::reset


#endif /** __ASTRUNC_CHARS_H__ */

