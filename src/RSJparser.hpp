/** **************************************************************************************
*                                                                                        *
*    A Ridiculously Simple JSON Parser for C++ (RSJp-cpp)                                *
*    Version 2.x                                                                         *
*    ----------------------------------------------------------                          *
*    Copyright (C) 2018  Subhrajit Bhattacharya                                          *
*                                                                                        *
*    This program is free software: you can redistribute it and/or modify                *
*    it under the terms of the GNU General Public License as published by                *
*    the Free Software Foundation, either version 3 of the License, or                   *
*    (at your option) any later version.                                                 *
*                                                                                        *
*    This program is distributed in the hope that it will be useful,                     *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of                      *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                       *
*    GNU General Public License for more details <http://www.gnu.org/licenses/>.         *
*                                                                                        *
*                                                                                        *
*    Contact:  subhrajit@gmail.com                                                       *
*              https://www.lehigh.edu/~sub216/ , http://subhrajit.net/                   *
*                                                                                        *
*                                                                                        *
*************************************************************************************** **/

#ifndef __DOSL_RSJPARSE_TCC
#define __DOSL_RSJPARSE_TCC

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <climits>
#include <fstream>


static char const* RSJobjectbrackets = "{}";
static char const* RSJarraybrackets = "[]";
static char RSJobjectassignment = ':';
static char RSJarraydelimiter = ',';

static std::vector<char const*> RSJbrackets = {RSJobjectbrackets, RSJarraybrackets};
static std::vector<char const*> RSJstringquotes = {"\"\"", "''"};
static char RSJcharescape = '\\';
static std::string RSJlinecommentstart = "//";

static std::string RSJprinttab = "    ";

enum RSJresourceType { RSJ_UNINITIATED, RSJ_UNKNOWN, RSJ_OBJECT, RSJ_ARRAY, RSJ_LEAF };

// ============================================================
// Direct string manipulation functions

inline 
std::string to_string (RSJresourceType rt) {
    switch (rt) {
        case RSJ_UNINITIATED: return("RSJ_UNINITIATED");
        case RSJ_UNKNOWN: return("RSJ_UNKNOWN");
        case RSJ_OBJECT: return("RSJ_OBJECT");
        case RSJ_ARRAY: return("RSJ_ARRAY");
        case RSJ_LEAF: return("RSJ_LEAF");
    }
}

enum StrTrimDir { STRTRIM_L=1, STRTRIM_R=2, STRTRIM_LR=3 };

inline 
std::string strtrim (std::string str, std::string chars=" \t\n\r", int max_count=-1, StrTrimDir dirs=STRTRIM_LR) {
    if (str.empty()) return(str);
    if (max_count<0) max_count = str.length();
    
    if (dirs & STRTRIM_L) { // left trim
        int p;
        for (p=0; p<max_count; ++p)
            if (chars.find(str[p])==std::string::npos) break;
        str.erase (0, p); // const_str::l_erase(p)
    }
    
    if (dirs & STRTRIM_R) { // right trim
        int q, strlenm1=str.length()-1;
        for (q=0; q<max_count; ++q)
            if (chars.find(str[strlenm1-q])==std::string::npos) break;
        str.erase (str.length()-q, q); // const_str::r_erase(q)
    }
    
    return (str);
}

inline 
std::string strip_outer_quotes (std::string str, char* qq=NULL) {
    str = strtrim (str);
    
    std::string ret = strtrim (str, "\"");
    if (ret.length()==str.length()) {
        ret = strtrim (str, "'");
        if (qq && ret!=str) *qq = '\'';
    }
    else if (qq)
        *qq = '"';
    
    return (ret);
}

// ----------------

inline 
int is_bracket (char c, std::vector<char const*>& bracks, int indx=0) {
    for (int b=0; b<bracks.size(); ++b)
        if (c==bracks[b][indx]) 
            return (b);
    return (-1);
}

inline 
std::vector<std::string> split_RSJ_array (const std::string& str) { // TODO: Make efficient. This function is speed bottleneck.
    // splits, while respecting brackets and escapes
    std::vector<std::string> ret;
    
    std::string current;
    std::vector<int> bracket_stack;
    std::vector<int> quote_stack;
    bool escape_active = false;
    int bi;
    
    for (int a=0; a<str.length(); ++a) { // *
        
        // delimiter
        if ( bracket_stack.size()==0  &&  quote_stack.size()==0  &&  str[a]==RSJarraydelimiter ) {
            ret.push_back (current);
            current.clear(); bracket_stack.clear(); quote_stack.clear(); escape_active = false;
            continue; // to *
        }
        
        // ------------------------------------
        // checks for string
        
        if (quote_stack.size() > 0) { // already inside string
            if (str[a]==RSJcharescape)  // an escape character
                escape_active = !escape_active;
            else if (!escape_active  &&  str[a]==RSJstringquotes[quote_stack.back()][1] ) { // close quote
                quote_stack.pop_back();
                escape_active = false;
            }
            else
                escape_active = false;
            
            current.push_back (str[a]);
            continue; // to *
        }
        
        if (quote_stack.size()==0) { // check for start of string
            if ((bi = is_bracket (str[a], RSJstringquotes)) >= 0) {
                quote_stack.push_back (bi);
                current.push_back (str[a]);
                continue; // to *
            }
        }
        
        // ------------------------------------
        // checks for comments
        
        if (quote_stack.size()==0) { // comment cannot start inside string
            
            // single-line commenst
            if (str.compare (a, RSJlinecommentstart.length(), RSJlinecommentstart) == 0) {
                // ignore until end of line
                int newline_pos = str.find ("\n", a);
                if (newline_pos == std::string::npos)
                    newline_pos = str.find ("\r", a);
                
                if (newline_pos != std::string::npos)
                    a = newline_pos; // point to the newline character (a will be incremented)
                else // the comment continues until EOF
                    a = str.length();
                continue;
            }
        }
        
        // ------------------------------------
        // checks for brackets
        
        if ( bracket_stack.size()>0  &&  str[a]==RSJbrackets[bracket_stack.back()][1] ) { // check for closing bracket
            bracket_stack.pop_back();
            current.push_back (str[a]);
            continue;
        }
        
        if ((bi = is_bracket (str[a], RSJbrackets)) >= 0) {
            bracket_stack.push_back (bi);
            current.push_back (str[a]);
            continue; // to *
        }
        
        // ------------------------------------
        // otherwise
        current.push_back (str[a]);
    }
    
    if (current.length() > 0)
        ret.push_back (current);
    
    return (ret);
}

inline 
std::string insert_tab_after_newlines (std::string str) {
    for (int a=0; a<str.length(); ++a)
        if (str[a]=='\n') {
            str.insert (a+1, RSJprinttab);
            a += RSJprinttab.length();
        }
    return (str);
}


// ============================================================

// forward declarations
class RSJparsedData;
class RSJresource;

// Objet and array typedefs
typedef std::unordered_map <std::string,RSJresource>    RSJobject;
typedef std::vector <RSJresource>                       RSJarray;

// ------------------------------------
// Main classes

class RSJresource {
/* Use: RSJresource("RSJ_string_data").as<RSJobject>()["keyName"].as<RSJarray>()[2].as<int>()
        RSJresource("RSJ_string_data")["keyName"][2].as<int>()  */
private:
    // main data
    std::string data; // can be object, vector or leaf data
    bool _exists;      // whether the RSJ resource exists.
    
    // parsed data
    RSJparsedData* parsed_data_p;
    
public:
    // constructor
    RSJresource () : _exists (false), parsed_data_p (NULL) { } // no data field.
    
    RSJresource (std::string str) : data (str), _exists (true), parsed_data_p (NULL) { }
    RSJresource (const char* str) : RSJresource(std::string(str)) { }
    
    // other convertion
    template <class dataType>
    RSJresource (dataType d) : RSJresource(std::to_string(d)) { }
    
    // read from file and stream
    RSJresource (std::istream& is) : _exists (true), parsed_data_p (NULL) {
        data = std::string ( (std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>()) );
    }
    RSJresource (std::ifstream& ifs) : _exists (true), parsed_data_p (NULL) {
        std::istream& is = ifs;
        data = std::string ( (std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>()) );
    }
    
    // free allocated memory for parsed data
    ~RSJresource();
    
    // deep copy
    RSJresource (const RSJresource& r);
    RSJresource& operator= (const RSJresource& r);
    
    // ------------------------------------
    // parsers (old)
    RSJresourceType parse (bool force=false);
    void parse_full (bool force=false, int max_depth=INT_MAX, int* parse_count_for_verbose_p=NULL); // recursively parse the entire JSON text
    // parser (new)
    void fast_parse (std::string* str_p=NULL, bool copy_string=false, int max_depth=INT_MAX, int* parse_start_str_pos=NULL); // TODO: finish.
    
    RSJobject& as_object (bool force=false);
    RSJarray& as_array (bool force=false);
    
    // ------------------------------------
    
    // access raw data and other attributes
    int size(void);
    std::string& raw_data (void) { return (data); }
    bool exists (void) { return (_exists); }
    bool is_parsed (void) { return (parsed_data_p!=NULL); }
    RSJresourceType type (void);
    // emitter
    std::string as_str (bool print_comments=false, bool update_data=true);
    void print (bool print_comments=false, bool update_data=true) 
        { std::cout << as_str(print_comments,update_data) << std::endl; }
    
    // opertor[]
    RSJresource& operator[] (std::string key); // object
    RSJresource& operator[] (int indx); // array
    
    // ------------------------------------
    
    // as
    template <class dataType>
    dataType as (const dataType& def = dataType()) { // specialized outside class declaration
        if (!exists()) return (def);
        return dataType (data); // default behavior for unknown types: invoke 'dataType(std::string)'
    }
    
    // as_vector
    template <class dataType, class vectorType=std::vector<dataType> > // vectorType should have push_back method
    vectorType as_vector (const vectorType& def = vectorType());
    
    // as_map
    template <class dataType, class mapType=std::unordered_map<std::string,dataType> > // mapType should have operator[] defined
    mapType as_map (const mapType& def = mapType());    
};

// ------------------------------------------------------------

class RSJparsedData {
public:
    RSJobject object;
    RSJarray array;
    
    RSJresourceType type;
    RSJparsedData() : type(RSJ_UNKNOWN) {}
    
    // parser (single-level)
    void parse (const std::string& data, RSJresourceType typ = RSJ_UNKNOWN) {
        std::string content = strtrim(data);
        
        if (typ==RSJ_OBJECT || typ==RSJ_UNKNOWN) {
            // parse as object:
            content = strtrim (strtrim (content, "{", 1, STRTRIM_L ), "}", 1, STRTRIM_R );
            if (content.length() != data.length()) { // a valid object
                std::vector<std::string> nvPairs = split_RSJ_array (content);
                for (int a=0; a<nvPairs.size(); ++a) {
                    std::size_t assignmentPos = nvPairs[a].find (RSJobjectassignment);
                    object.insert (make_pair( 
                                        strip_outer_quotes (nvPairs[a].substr (0,assignmentPos) ) ,
                                        RSJresource (strtrim (nvPairs[a].substr (assignmentPos+1) ) )
                               ) );
                }
                if (object.size() > 0) {
                    type = RSJ_OBJECT;
                    return;
                }
            }
        }
        
        if (typ==RSJ_ARRAY || typ==RSJ_UNKNOWN) {
            // parse as array
            content = strtrim (strtrim (content, "[", 1, STRTRIM_L ), "]", 1, STRTRIM_R );
            if (content.length() != data.length()) { // a valid array
                std::vector<std::string> nvPairs = split_RSJ_array (content);
                for (int a=0; a<nvPairs.size(); ++a) 
                    array.push_back (RSJresource (strtrim (nvPairs[a]) ) );
                if (array.size() > 0) {
                    type = RSJ_ARRAY;
                    return;
                }
            }
        }
        
        if (typ==RSJ_UNKNOWN)
            type = RSJ_LEAF;
    }
    
    
    // remove non-existing items inserted due to accessing
    int cleanup(void) {
    
        if (type==RSJ_OBJECT) {
            bool found = true;
            while (found) {
                found = false;
                for (auto it=object.begin(); it!=object.end(); ++it)
                    if (!(it->second.exists())) {
                        object.erase(it);
                        found = true;
                        break; // break for loop since it is now invalid
                    }
            }
            return (object.size());
        }
        
        if (type==RSJ_ARRAY) { // erases only the non-existent elements at the tail
            while (!(array[array.size()-1].exists()))
                array.pop_back();
            return (array.size());
        }
        
        if (type==RSJ_LEAF)
            return (1);
        
        return (0);
    }
    
    // size
    int size(void) { return (cleanup()); }
};


// ------------------------------------------------------------
// RSJresource member functions

inline 
RSJresource::~RSJresource (){
    if (parsed_data_p) delete parsed_data_p;
}

inline 
RSJresource::RSJresource (const RSJresource& r) {
    data=r.data;
    _exists = r._exists;
    if(r.parsed_data_p) parsed_data_p = new RSJparsedData(*(r.parsed_data_p));
    else parsed_data_p = NULL;
}

inline 
RSJresource& RSJresource::operator= (const RSJresource& r) {
    data=r.data;
    _exists = r._exists;
    if(r.parsed_data_p) parsed_data_p = new RSJparsedData(*(r.parsed_data_p));
    else parsed_data_p = NULL;
    return *this;
}

inline 
int RSJresource::size (void) {
    if (!exists()) return (0);
    parse(); // parse if not parsed
    return (parsed_data_p->size());
}

inline 
RSJresourceType RSJresource::type (void) {
    if (!exists()) return (RSJ_UNINITIATED);
    parse(); // parse if not parsed
    return (parsed_data_p->type);
}

inline 
std::string RSJresource::as_str (bool print_comments, bool update_data) {
    if (exists()) {
        std::string ret;
        parse(); // parse if not parsed
        parsed_data_p->cleanup();
        
        if (parsed_data_p->type==RSJ_OBJECT) {
            ret = "{\n";
            for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it) {
                ret += RSJprinttab + "'" + it->first + "': " + insert_tab_after_newlines( it->second.as_str (print_comments, update_data) );
                if (std::next(it) != parsed_data_p->object.end()) ret += ",";
                if (print_comments)
                    ret += " // " + to_string(it->second.type());
                ret += "\n";
            }
            ret += "}";
        }
        else if (parsed_data_p->type==RSJ_ARRAY) {
            ret = "[\n";
            for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it) {
                ret += RSJprinttab + insert_tab_after_newlines( it->as_str (print_comments, update_data) );
                if (std::next(it) != parsed_data_p->array.end()) ret += ",";
                if (print_comments)
                    ret += " // " + to_string(it->type());
                ret += "\n";
            }
            ret += "]";
        }
        else // RSJ_LEAF or RSJ_UNKNOWN
             ret = strtrim (data);
        
        if (update_data) data = ret;
        return (ret);
    }
    else
        return ("");
}

// Parsers

inline 
RSJresourceType RSJresource::parse (bool force) {
    if (!parsed_data_p)  parsed_data_p = new RSJparsedData;
    if (parsed_data_p->type==RSJ_UNKNOWN || force)  parsed_data_p->parse (data, RSJ_UNKNOWN);
    return (parsed_data_p->type);
}

inline 
void RSJresource::parse_full (bool force, int max_depth, int* parse_count_for_verbose_p) { // recursive parsing (slow)
    if (max_depth==0) return;
    if (!parsed_data_p)  parsed_data_p = new RSJparsedData;
    if (parsed_data_p->type==RSJ_UNKNOWN || force)  parsed_data_p->parse (data, RSJ_UNKNOWN);
    // verbose
    if (parse_count_for_verbose_p) {
        (*parse_count_for_verbose_p)++;
        if ( (*parse_count_for_verbose_p) % 100 == 0)
            std::cout << "parse_full: " << (*parse_count_for_verbose_p) << " calls." << std::endl;
    }
    // recursive parse children if not already parsed
    if (parsed_data_p->type==RSJ_OBJECT) 
        for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it)
            it->second.parse_full (force, max_depth-1, parse_count_for_verbose_p);
    else if (parsed_data_p->type==RSJ_ARRAY)
        for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it) 
            it->parse_full (force, max_depth-1, parse_count_for_verbose_p);
}

// ------------------------------------------------------------
// ============================================================
// FAST PARSER (Under construction. DO NOT use the following functions in your application.)

inline 
int seek_next (std::string* str_p, int start_pos, char character) {
    
}

inline 
void RSJresource::fast_parse (std::string* str_p, bool copy_string, int max_depth, int* parse_start_str_pos) {
    // TODO: UNDER CONSTRUCTION...
    
    if (!str_p)
        str_p = &data;
    std::string& str = *str_p;
    
    // splits, while respecting brackets and escapes
    //std::vector<std::string> ret;
    
    //std::string current;
    std::vector<int> bracket_stack;
    std::vector<int> quote_stack;
    bool escape_active = false;
    int bi;
    
    bool initial_whitespaces = true;
    bool isroot = false;
    
    if (!parse_start_str_pos) {
        parse_start_str_pos = new int;
        *parse_start_str_pos = 0;
        isroot = true;
    }
    
    int a = *parse_start_str_pos;
    
    while (*parse_start_str_pos < str_p->length()) { // *
        
        // initial whitespace characters
        if (initial_whitespaces) {
            if (str[a] == ' ' || str[a] == '\n' || str[a] == '\r' || str[a] == '\t' ) {
                ++a;
                continue;
            }
            else {
                if (str[a] == '{') // start of object
                    // ... TODO: seek_next ':'
                
                initial_whitespaces = false;
            }
        }
        
        
        // delimiter
        if ( bracket_stack.size()==0  &&  quote_stack.size()==0  &&  str[a]==RSJarraydelimiter ) {
            //ret.push_back (current);
            
            //current.clear();
            bracket_stack.clear(); quote_stack.clear(); escape_active = false;
            continue; // to *
        }
        
        // ------------------------------------
        // checks for string
        
        if (quote_stack.size() > 0) { // already inside string
            if (str[a]==RSJcharescape)  // an escape character
                escape_active = !escape_active;
            else if (!escape_active  &&  str[a]==RSJstringquotes[quote_stack.back()][1] ) { // close quote
                quote_stack.pop_back();
                escape_active = false;
            }
            else
                escape_active = false;
            
            //current.push_back (str[a]);
            continue; // to *
        }
        
        if (quote_stack.size()==0) { // check for start of string
            if ((bi = is_bracket (str[a], RSJstringquotes)) >= 0) {
                quote_stack.push_back (bi);
                //current.push_back (str[a]);
                continue; // to *
            }
        }
        
        // ------------------------------------
        // checks for comments
        
        if (quote_stack.size()==0) { // comment cannot start inside string
            
            // single-line commenst
            if (str.compare (a, RSJlinecommentstart.length(), RSJlinecommentstart) == 0) {
                // ignore until end of line
                int newline_pos = str.find ("\n", a);
                if (newline_pos == std::string::npos)
                    newline_pos = str.find ("\r", a);
                
                if (newline_pos != std::string::npos)
                    a = newline_pos; // point to the newline character (a will be incremented)
                else // the comment continues until EOF
                    a = str.length();
                continue;
            }
        }
        
        // ------------------------------------
        // checks for brackets
        
        if ( bracket_stack.size()>0  &&  str[a]==RSJbrackets[bracket_stack.back()][1] ) { // check for closing bracket
            bracket_stack.pop_back();
            //current.push_back (str[a]);
            continue;
        }
        
        if ((bi = is_bracket (str[a], RSJbrackets)) >= 0) {
            bracket_stack.push_back (bi);
            //current.push_back (str[a]);
            continue; // to *
        }
        
        // ------------------------------------
        // otherwise
        //current.push_back (str[a]);
    }
    
    /*if (current.length() > 0)
        ret.push_back (current); */
    
    if (isroot)
        delete parse_start_str_pos;
    
    // return (ret);
}

// ============================================================

// ------------------------------------------------------------

inline 
RSJobject& RSJresource::as_object (bool force) {
    if (!parsed_data_p)  parsed_data_p = new RSJparsedData;
    if (parsed_data_p->type==RSJ_UNKNOWN || force)  parsed_data_p->parse (data, RSJ_OBJECT);
    return (parsed_data_p->object);
}

inline 
RSJresource& RSJresource::operator[] (std::string key) { // returns reference
    return ( (as_object())[key] ); // will return empty resource (with _exists==false) if 
                                            // either this resource does not exist, is not an object, or the key does not exist
}

inline 
RSJarray& RSJresource::as_array (bool force) {
    if (!parsed_data_p)  parsed_data_p = new RSJparsedData;
    if (parsed_data_p->type==RSJ_UNKNOWN || force)  parsed_data_p->parse (data, RSJ_ARRAY);
    return (parsed_data_p->array);
}

inline 
RSJresource& RSJresource::operator[] (int indx) { // returns reference
    as_array();
    if (indx >= parsed_data_p->array.size())
        parsed_data_p->array.resize(indx+1); // insert empty resources
    return (parsed_data_p->array[indx]); // will return empty resource (with _exists==false) if 
                                            // either this resource does not exist, is not an object, or the key does not exist
}

// ------------------------------------------------------------
// special 'as':

template <class dataType, class vectorType> inline 
vectorType RSJresource::as_vector (const vectorType& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    vectorType ret;
    as_array();
    for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it)
        ret.push_back (it->as<dataType>());
    return (ret);
}

template <class dataType, class mapType> inline 
mapType RSJresource::as_map (const mapType& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    mapType ret;
    as_object();
    for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it)
        ret[it->first] = it->second.as<dataType>();
    return (ret);
}

// ============================================================
// Specialized .as() member functions

// Helper preprocessor directives
#define rsjObject  as<RSJobject>()
#define rsjArray   as<RSJarray>()
#define rsjAs(t)   as<t>()


// RSJobject
template <> inline 
RSJobject RSJresource::as<RSJobject> (const RSJobject& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    return (as_object());
}

// RSJarray
template <> inline 
RSJarray  RSJresource::as<RSJarray> (const RSJarray& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    return (as_array());
}

// ------------------------------------
// Elementary types

// String
template <> inline 
std::string  RSJresource::as<std::string> (const std::string& def) {
    if (!exists()) return (def);
    
    char qq = '\0';
    std::string ret = strip_outer_quotes (data, &qq);
    
    std::vector< std::vector<std::string> > escapes = { {"\\n","\n"}, {"\\r","\r"}, {"\\t","\t"}, {"\\\\","\\"} };
    if (qq=='"')
        escapes.push_back ({"\\\"","\""});
    else if (qq=='\'')
        escapes.push_back ({"\\'","'"});
    
    for (int a=0; a<escapes.size(); ++a)
        for ( std::size_t start_pos=ret.find(escapes[a][0]); start_pos!=std::string::npos; start_pos=ret.find(escapes[a][0],start_pos) ) {
            ret.replace (start_pos, escapes[a][0].length(), escapes[a][1]);
            start_pos += escapes[a][1].length();
        }
    
    return (ret);
}

// integer
template <> inline 
int  RSJresource::as<int> (const int& def) {
    if (!exists()) return (def);
    return (atoi (strip_outer_quotes(data).c_str() ) );
}

// double
template <> inline 
double  RSJresource::as<double> (const double& def) {
    if (!exists()) return (def);
    return (atof (strip_outer_quotes(data).c_str() ) );
}

// bool
template <> inline 
bool  RSJresource::as<bool> (const bool& def) {
    if (!exists()) return (def);
    std::string cleanData = strip_outer_quotes (data);
    if (cleanData=="true" || cleanData=="TRUE" || cleanData=="True" || atoi(cleanData.c_str())!=0) return (true);
    return (false);
}

// ------------------------------------
// Other types

/*template <> template <dataType> inline 
bool  RSJresource::as< std::vector<dataType> > (const std::vector<dataType>& def) {
    return as_vector<dataType> (def);
}

template <> template <dataType> inline 
std::unordered_map<std::string,dataType>  RSJresource::as< std::unordered_map<std::string,dataType> > 
                                                    (const std::unordered_map<std::string,dataType>& def) {
    return as_map<dataType> (def);
}*/

#endif
