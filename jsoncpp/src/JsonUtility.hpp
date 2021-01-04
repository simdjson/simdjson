#include <math.h>
#ifndef ARDUINO
#include <cstdint>
#endif
#include "LexSource.hpp"
namespace json {
    union Convert32 {
        //uint8_t bytes[4];
        uint16_t words[2];
        uint32_t dword;
    };
    struct JsonLexState {
        struct {
            unsigned int state : 4;
            unsigned int accept : 2;
            unsigned int negative : 1;
            unsigned int overflow : 1;
            unsigned int fracPart : 2;
            unsigned int fracCount : 6;
            unsigned int eCount : 16;
        } flags;
        double real;
        long long int integer;
    };
    struct JsonUtility {
        static uint8_t fromHexChar(char hex) {
            if (':' > hex && '/' < hex)
                return (uint8_t)(hex - '0');
            if ('G' > hex && '@' < hex)
                return (uint8_t)(hex - '7'); // 'A'-10
            return (uint8_t)(hex - 'W'); // 'a'-10
        }
        static bool isHexChar(char hex) {
            return (':' > hex && '/' < hex) ||
                ('G' > hex && '@' < hex) ||
                ('g' > hex && '`' < hex);
        }
        static bool decodeUnicodeEscape(lex::LexSource& ls,int32_t& codepoint) {
            // nibble 1
            if(!isHexChar((char)ls.current())) {
                // ERROR invalid escape sequence
                return false;
            }
            codepoint = fromHexChar((char)ls.current());
            // nibble 2
            codepoint *= 16;
            if (!ls.advance()) {
                // ERROR unterminated string
                return false;
            }
            if(!isHexChar((char)ls.current())) {
                // ERROR invalid escape sequence
                return false;
            }
            codepoint |= fromHexChar((char)ls.current());
            // nibble 3
            codepoint *= 16;
            if (!ls.advance()) {
                // ERROR unterminated string
                return false;
            }
            if(!isHexChar((char)ls.current())) {
                // ERROR invalid escape sequence
                return false;
            }
            codepoint|= fromHexChar((char)ls.current());
            // nibble 4
            codepoint*=16;
            if (!ls.advance()) {
                // ERROR unterminated string
                return false;
            }
            if(!isHexChar((char)ls.current())) {
                // ERROR invalid escape sequence
                return false;
            }
            codepoint|=fromHexChar((char)ls.current());

            if (0 == codepoint) {
                codepoint = '?';
            }
            ls.advance();
            return true;
        }

        static const char* decodeUtf8(const char* sz, int32_t& codepoint) {
            char ch = *sz;
            codepoint=0;
            if (0xf0 == (0xf8 & ch)) {
                // 4 byte utf8 codepoint
                // TODO: verify this!!
                Convert32 cvt;
                cvt.dword = 0;
                cvt.words[0]=(0x07 & ch)<<2;
                //codepoint = (0x07 & ch) << 18;
                codepoint=cvt.dword;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= (0x3f & ch) << 12;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= (0x3f & ch) << 6;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= 0x3f & ch;
                return sz;
            }
            if (0xe0 == (0xf0 & ch)) {
                // 3 byte utf8 codepoint
                codepoint = (0x0f & ch) << 12;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= (0x3f & ch) << 6;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= 0x3f & ch;
                ++sz;
                return sz;
            }
            if (0xc0 == (0xe0 & ch)) {
                // 2 byte utf8 codepoint
                codepoint = (0x1f & ch) << 6;
                ch = *(++sz);
                if(0==ch) {
                    return nullptr;
                }
                codepoint |= 0x3f & ch;
                ++sz;
                return sz;
            }
            // 1 byte utf8 codepoint otherwise
            codepoint = (char)ch;
            ++sz;
            return sz;
        }
        static bool undecorate(lex::LexSource& ls, int32_t& codepoint,bool capture) {
            if(!ls.more() || '\n'==ls.current() || '\r'==ls.current()||0==ls.current())
                return false;
            if('\"'==ls.current())
                return false; // empty string

            if(ls.current()>0x7F) {
                if(capture) {
                    if(!ls.capture(ls.current()))
                        // ERROR out of memory
                        return false;
                }
                codepoint = ls.current();
                ls.advance();
                return true;
            }

            switch((char)(ls.current() & 0x7f)) {
                case '\\':
                    if(!ls.advance()) {
                        // ERROR unterminated string
                        return false;
                    }
                    switch(ls.current()) {
                        case '\\':
                        case '\"':
                        case '/':
                            codepoint = ls.current();
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;
                        case 'r':
                            codepoint='\r';
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;
                        case 'n':
                            codepoint='\n';
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;
                        case 't':
                            codepoint='\t';
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;
                        case 'f':
                            codepoint='\f';
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;
                        case 'b':
                            codepoint='\b';
                            if(capture && !ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            ls.advance();
                            return true;

                        case 'u':
                            if (!ls.advance()) {
                                // ERROR unterminated string
                                return false;
                            }
                            if(!decodeUnicodeEscape(ls,codepoint)) {
                                // ERROR invalid unicode sequence
                                return false;
                            }

                            //U+D800 to U+DBFF is high surrogate.
                            if(codepoint>0xD7FF && codepoint<0xDC00) {
                                // look for the second escape;
                                if('\\'!=ls.current() || !ls.advance()) {
                                    // ERROR invalid unicode surrogate
                                    return false;
                                }
                                if('u'!=ls.current() || !ls.advance()) {
                                    // ERROR invalid unicode surrogate
                                    return false;
                                }
                                int32_t cp2;
                                if(!decodeUnicodeEscape(ls,cp2)) {
                                    // ERROR invalid unicode sequence
                                    return false;
                                }
                                codepoint=(codepoint << 10) + cp2 - 0x35fdc00;
                                ls.advance();

                            }
                            if(capture&&!ls.capture(codepoint)) {
                                // ERROR out of memory
                                return false;
                            }
                            return true;
                        default:
                            // ERROR invalid escape sequence
                            return false;
                    }
                    return true;
                default:
                    codepoint = ls.current();
                    if(capture && !ls.capture(codepoint)) {
                        // ERROR out of memory
                        return false;
                    }
                    ls.advance();
                    return true;
            }
            return false;
        }

        inline static bool skipWhiteSpace(lex::LexSource& ls) {
            static const char ws[]= {0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
            int32_t cp = ls.current();
            if(-1<cp && cp<33 && 1==ws[cp]) {
                while(ls.advance() && (-1<(cp=ls.current()) && cp<33 && 1==ws[cp]));
            }
            return !ls.hasError();
        }

        static bool skipStringPart(lex::LexSource& ls, bool skipFinalQuote = true)
        {
            if(!ls.ensureStarted())
                return false;
            if (ls.current() == '\"') {
                if(skipFinalQuote)
                    ls.advance();
                return true;
            }
            char ch = ls.skipToAny("\"\\");
            while(0!=ch) {
                switch(ch) {
                    case '\\':
                        if(!ls.advance()) {
                            return false;
                        }
                        if(!ls.advance()) {
                            return false;
                        }
                        break;
                    case '\"':
                        if (skipFinalQuote)
                            if(!ls.advance())
                                return false;
                        return true;
                }
                ch=ls.skipToAny("\"\\");
            }
            if('\"'==ch) {
                if (skipFinalQuote)
                        if(!ls.advance())
                            return false;
                return true;
            }
            return false;

        }

        static bool lexNumber(lex::LexSource& ls,JsonLexState& st) {
            int32_t cp;
            if(!ls.ensureStarted())
                return false; // error

            switch(st.flags.state) {
                case 0:
                    st.flags.accept=0;
                    st.flags.eCount=0;
                    st.flags.fracCount=0;
                    st.flags.fracPart=0;
                    st.flags.negative=0;
                    st.flags.overflow=0;
                    st.integer = 0;
                    st.real = 0;
                    if(ls.more()) {
                        if ((cp=ls.current()) == '-') {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            st.flags.negative =1;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 1;
                            return true;
                        }
                        if (cp == '0') {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 2;
                            return true;
                        }
                        if (((cp >= '1')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            st.integer=cp;
                            st.real = cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 8;
                            return true;
                        }
                    }
                    goto error;
                case 1:
                    if(ls.more()) {
                        if ((cp=ls.current()) == '0') {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            if(0!=st.flags.negative)
                                    cp=-cp;
                            st.real=cp;
                            st.integer=cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 2;
                            return true;
                        }
                        if (((cp >= '1')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            if(0!=st.flags.negative)
                                    cp=-cp;
                            st.real=cp;
                            st.integer=cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 8;
                            return true;
                        }
                    }
                    goto error;
                case 2:
                    if(ls.more()) {
                        if ((((cp=ls.current())== 'E')
                                    || (cp == 'e'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            st.flags.fracPart = 2;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 5;
                            return true;
                        }
                        if (cp == '.') {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            ++st.flags.fracCount;
                            st.flags.fracPart=1;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 3;
                            return true;
                        }
                    }
                    st.flags.accept=1;
                    return false;
                case 3:
                    if(ls.more()) {
                        if ((((cp=ls.current()) >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            if(st.flags.fracCount<63) {
                                cp-='0';
                                if(0!=st.flags.negative)
                                        cp=-cp;
                                st.real+=(cp*(pow(10, (st.flags.fracCount * -1))));
                                ++st.flags.fracCount;
                            }
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 4;
                            return true;
                        }
                    }
                    goto error;
                case 4:
                    if(ls.more()) {
                        if ((((cp=ls.current())== 'E')
                                    || (cp == 'e'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 5;
                            return true;
                        }
                        if (((cp >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            if(st.flags.fracCount<63) {
                                cp-='0';
                                if(0!=st.flags.negative)
                                        cp=-cp;
                                st.real+=(cp*(pow(10,-st.flags.fracCount)));
                                ++st.flags.fracCount;
                            }
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 4;
                            return true;
                        }
                    }
                    st.flags.accept=2;
                    return false; // orig
                case 5:
                    if(ls.more()) {
                        if ((((cp=ls.current()) == '+')
                                    || (cp == '-'))) {
                            if('-'==cp)
                                st.flags.fracPart = 3; // indicates neg exponent
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 6;
                            return true;
                        }
                        if (((cp >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            if(st.flags.eCount>6553) {
                                st.flags.overflow = 1;
                            }
                            st.flags.eCount=st.flags.eCount*10+cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 7;
                            return true;
                        }
                    }
                    goto error;
                case 6:
                    if(ls.more()) {
                        if ((((cp=ls.current()) >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            if(st.flags.eCount>6553) {
                                st.flags.overflow = 1;
                            }
                            st.flags.eCount=st.flags.eCount*10+cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 7;
                            return true;
                        }
                    }
                    goto error;
                case 7:
                    if(ls.more()) {
                        if ((((cp=ls.current()) >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp-='0';
                            if(st.flags.eCount>6553) {
                                st.flags.overflow = 1;
                            }
                            st.flags.eCount=st.flags.eCount*10+cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;

                            }
                            st.flags.state = 7;
                            return true;
                        }
                    }
                    st.flags.accept=2;
                    return false; // orig
                case 8:
                    if(ls.more()) {
                        if ((((cp=ls.current())== 'E')
                                    || (cp == 'e'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            st.flags.fracPart = 2;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 5;
                            return true;
                        }
                        if (((cp >= '0')
                                    && (cp <= '9'))) {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            cp=cp-'0';
                            if(st.flags.negative)
                                cp=-cp;
                            st.integer=st.integer*10+cp;
                            if((0>st.integer && 0==st.flags.negative)||(0<st.integer && 0!=st.flags.negative)) {
                                st.flags.overflow = 1;
                            }
                            st.real = floor(st.real*10.0+0.5)+cp;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 8;
                            return true;
                        }
                        if (cp == '.') {
                            if(!ls.capture(cp)) {
                                return false;
                            }
                            ++st.flags.fracCount;
                            st.flags.fracPart=1;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 3;
                            return true;
                        }
                    }
                    st.flags.accept =1;
                    return false;
            }
        error:
            // TODO: figure out why not consistent capture on error
            ls.capture(ls.current());
            /*if(!ls.advance()) {
                return false;
            }*/
            return false;
        }
        static bool lexLiteral(lex::LexSource& ls,JsonLexState& st) {
            int32_t cp;
            switch(st.flags.state) {
                case 0:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'n') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 1;
                            return true;
                        }
                        if ((cp=ls.current()) == 't') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 5;
                            return true;
                        }
                        if ((cp=ls.current()) == 'f') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 9;
                            return true;
                        }
                    }
                    goto error;
                case 1:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'u') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 2;
                            return true;
                        }
                    }
                    goto error;
                case 2:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'l') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 3;
                            return true;
                        }
                    }
                    goto error;
                case 3:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'l') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 4;
                            return true;
                        }
                    }
                    goto error;
                case 4:
                    st.flags.accept= 1;
                    return false;
                case 5:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'r') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 6;
                            return true;
                        }
                    }
                    goto error;
                case 6:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'u') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 7;
                            return true;
                        }
                    }
                    goto error;
                case 7:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'e') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 8;
                            return true;
                        }
                    }
                    goto error;
                case 8:
                    st.flags.accept=2;
                    return false;
                case 9:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'a') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 10;
                            return true;
                        }
                    }
                    goto error;
                case 10:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'l') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 11;
                            return true;
                        }
                    }
                    goto error;
                case 11:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 's') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 12;
                            return true;
                        }
                    }
                    goto error;
                case 12:
                    if(ls.more()) {
                        if ((cp=ls.current()) == 'e') {
                            if(!ls.capture(cp))
                                return false;
                            if(!ls.advance()) {
                                if(ls.hasError())
                                    return false;
                            }
                            st.flags.state = 13;
                            return true;
                        }
                    }
                    goto error;
                case 13:
                    st.flags.accept=3;
                    return false;
            }
        error:
            ls.capture(ls.current());
                    /*if(!ls.advance()) {
                        return false;
                    }*/
            return false;

        }
    };
}