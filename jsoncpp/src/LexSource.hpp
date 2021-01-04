#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_LEXSOURCE_HPP
#define HTCW_LEXSOURCE_HPP

#ifndef ARDUINO
#include <type_traits>
#include <cinttypes>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "MemoryPool.hpp"
#endif

namespace lex {

class LexSource {
        int8_t m_state;
        int32_t m_current;
        /* Keep this in for now for when I add simd
        union {
            char bytes[4];
            uint32_t word;
        } m_buf;
        */

        unsigned long long m_position;
    public:
        static const int8_t IOError = -3;
        static const int8_t OutOfMemoryError=-4;
    private:
        LexSource(LexSource& rhs) = delete;
        LexSource(LexSource&& rhs) = delete;
        LexSource& operator=(LexSource& rhs) = delete;

        const int8_t Initial = -5;
        bool advanceImpl() {
            int16_t ch;

            /* I'm keeping this in for now so I can try to make it simd later
            int e=0;
            char* sz=decodeUtf8Padded4(m_buf.bytes,m_current,e);
            if(0!=e) {
                m_state = IOError;
                return false;
            }
            int len = sz-m_buf.bytes;
            while(0!=len) {
                m_buf.word>>=8;
                ch=read();
                m_state = ch;
                m_buf.bytes[3]=(-1<ch)*ch;
                --len;
            }
            switch(m_current*(-1<m_state)*(128>m_current)) {
                case '\t': // tab
                    m_column+=TabWidth;
                    break;
                case '\n': // newline
                    ++m_line;
                    // fall through
                case '\r': // carriage return
                    m_column=0;
                    break;
                default:
                    ++m_column;
                    break;
            }
            m_position+=(-1<m_state);
            return -1<m_state;
            */
            /*if(0>(ch=read())) {
                m_state = (int8_t)ch;
                return false;
            }*/
            int16_t i = read();
            if(0>i) {
                m_state = i;
                m_current = 0;
                return false;
            }
            m_current = i;
            m_state = 0;

            ++m_position;
            if(m_current<128) {
                return -1<m_state;
            }


            if (0xf0 == (0xf8 & m_current)) {
                // 4 byte utf8 codepoint
                m_current = (0x07 & m_current) << 18;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= (0x3f & ch) << 12;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= (0x3f & ch) << 6;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= 0x3f & ch;
                ++m_position;
                return true;
            }
            if (0xe0 == (0xf0 & m_current)) {
                // 3 byte utf8 codepoint
                m_current = (0x0f & m_current) << 12;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= (0x3f & ch) << 6;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= 0x3f & ch;
                ++m_position;
                return true;
            }
            //if (0xc0 == (0xe0 & ch)) {
                // 2 byte utf8 codepoint
                m_current = (0x1f & m_current) << 6;
                ch = read();
                if(EndOfInput==ch||Closed==ch||IOError==ch) {
                    m_state = IOError;
                    return false;
                }
                m_current |= 0x3f & ch;
                ++m_position;
                return true;
            //}
        }
        // Keeping this here for when I add simd support
        // buf is the buffer to convert (must be length multiple of 4), c is the output codepoint, and e holds any error
        /*
        inline static char *decodeUtf8Padded4(char *buf, uint32_t& c, int& e)
        {
            static const char lengths[] = {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
            };
            static const int masks[]  = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
            static const uint32_t mins[] = {4194304, 0, 128, 2048, 65536};
            static const int shiftc[] = {0, 18, 12, 6, 0};
            static const int shifte[] = {0, 6, 4, 2, 0};

            unsigned char *s = (unsigned char*)buf;
            int len = lengths[s[0] >> 3];

            // Compute the pointer to the next character early so that the next
            // iteration can start working on the next character. Neither Clang
            // nor GCC figure out this reordering on their own.

            unsigned char *next = s + len + !len;

            // Assume a four-byte character and load four bytes. Unused bits are
            // shifted out.

            c  = (uint32_t)(s[0] & masks[len]) << 18;
            c |= (uint32_t)(s[1] & 0x3f) << 12;
            c |= (uint32_t)(s[2] & 0x3f) <<  6;
            c |= (uint32_t)(s[3] & 0x3f) <<  0;
            c >>= shiftc[len];

            // Accumulate the various error conditions.
            e  = (c < mins[len]) << 6; // non-canonical encoding
            e |= ((c >> 11) == 0x1b) << 7;  // surrogate half?
            e |= (c > 0x10FFFF) << 8;  // out of range?
            e |= (s[1] & 0xc0) >> 2;
            e |= (s[2] & 0xc0) >> 4;
            e |= (s[3]       ) >> 6;
            e ^= 0x2a; // top two bits of each tail byte correct?
            e >>= shifte[len];

            return (char*)next;
        }
        */
    protected:
        static const int8_t EndOfInput=-1;
        static const int8_t Closed=-2;
        virtual int16_t read()=0;

        virtual bool skipToAny(const char* characters7bit,unsigned long long& position,int16_t& match,int8_t& error) {
            error = 0;
            const char *sz;
            if(current()>-1&&current()<128) {
                match=(int16_t)current();
                sz = strchr(characters7bit,match);
                if(nullptr!=sz) {
                    match=*sz;
                    return true;
                }
            }
            while(-1<(match=read())) {
                ++position;
                if(match>127)
                    continue;
                sz = strchr(characters7bit,(char)match);
                if(nullptr!=sz) {
                    match=*sz;
                    return true;
                }

            }
            error = match;
            return false;
        }
        virtual bool appendCapture(char ch)=0;
        void clearError() {m_state = 0;}

    public:
        LexSource()  {
            reset();
        }

        virtual char* captureBuffer()=0;
        virtual size_t captureCapacity()  const=0;
        virtual size_t captureSize() const=0 ;
        virtual void clearCapture()=0;

        virtual void reset() {
            m_state = Initial;
            m_position = 0;
            m_current = 0;
        }

        char skipToAny(const char* characters7bit) {
            int16_t match;
            unsigned long long int pos=m_position;
            int8_t error=0;
            if(!skipToAny(characters7bit,pos,match,error)) {
                if(0>error)
                    m_state=error;
                m_position=pos;
                return false;
            }
            m_current = match;
            //printf("Advanced %d characters\r\n",(int)(pos-m_position));
            m_position=pos;
            return match;
        }

        inline bool advance() {
            /*if (EndOfInput == m_state || IOError==m_state) {
                m_current = 0;
                return false;
            }*/
            return advanceImpl();
        }
        bool advance(bool capture) {
            if (EndOfInput == m_state || IOError==m_state) {
                m_current = 0;
                return false;
            }

            if(capture) {
                if(captureSize()>captureCapacity()-4) {
                    m_state=OutOfMemoryError;
                    m_current=0;
                    return false;
                }
                return advanceImpl() && this->capture(m_current);
            }
            return advanceImpl();
        }
        inline int32_t current() const {
            return m_current;
        }
        bool ensureStarted() {
            if(m_state==Initial) {
                /* Keep this in for now for when i add simd
                int16_t ch;
                for(int i=0;i<4 && -1<(ch=(int8_t)read());++i) {
                    m_buf.bytes[i]=(char)ch;
                }

                m_state=(int8_t)ch;
                */
                return advanceImpl();
            }
            return true;
        }
        inline unsigned long long position() const { return m_position; }
        inline bool more() const {
            return -1<m_state||Initial==m_state;
        }
        inline bool eof() const {return EndOfInput==m_state;};
        inline bool hasError() const {return -1> m_state;};
        inline int8_t error() const {if(hasError()) return m_state; return 0;}
        bool capture(int32_t codepoint) {
            size_t n = captureCapacity()-captureSize();
            if(n<4) {
                m_state = OutOfMemoryError;
                return false;
            }
            if (0 == ((int32_t)0xffffff80 & codepoint)) {
                // 1-byte/7-bit ascii
                // (0b0xxxxxxx)
                /*if (n < 1) {
                    m_state = OutOfMemoryError;
                    return false;
                }*/
                appendCapture(codepoint);
                return true;
            }
            if (0 == ((int32_t)0xfffff800 & codepoint)) {
                // 2-byte/11-bit utf8 code point
                // (0b110xxxxx 0b10xxxxxx)
                /*if (n < 2) {
                    m_state = OutOfMemoryError;
                    return false;
                }*/
                appendCapture(0xc0 | (char)(codepoint >> 6));
                appendCapture(0x80 | (char)(codepoint & 0x3f));
                return true;
            }
            if (0 == ((int32_t)0xffff0000 & codepoint)) {
                // 3-byte/16-bit utf8 code point
                // (0b1110xxxx 0b10xxxxxx 0b10xxxxxx)
                /*if (n < 3) {
                    m_state = OutOfMemoryError;
                    return false;
                }*/
                appendCapture(0xe0 | (char)(codepoint >> 12));
                appendCapture(0x80 | (char)((codepoint >> 6) & 0x3f));
                appendCapture(0x80 | (char)(codepoint & 0x3f));
                return true;
            }
            // if (0 == ((int)0xffe00000 & chr)) {
            // 4-byte/21-bit utf8 code point
            // (0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx)
            /*if (n < 4) {
                m_state = OutOfMemoryError;
                return false;
            }*/
            appendCapture(0xf0 | (char)(codepoint >> 18));
            appendCapture(0x80 | (char)((codepoint >> 12) & 0x3f));
            appendCapture(0x80 | (char)((codepoint >> 6) & 0x3f));
            appendCapture(0x80 | (char)(codepoint & 0x3f));
            return true;
        }
        virtual ~LexSource() {

        }
};
template<size_t TCapacity> class StaticLexSource : virtual public LexSource {
    static const int8_t Initial=-5;
    // only certain platforms support
    // certain C++ features =(
#if !defined ARDUINO && !defined ESP8266
    // to safely capture a complete utf8 encoded Unicode codepoint
    // plus the null terminator we need at least 5 bytes
    static_assert(TCapacity>4,"Capacity must be 5 bytes or greater");
#endif

    char m_capture[TCapacity];
    size_t m_captureSize;

protected:
    bool appendCapture(char ch) final {
        char*sz = m_capture+(m_captureSize++);
        *(sz++)=ch;
        *sz=0;
        return true;
    }
public:

    void reset() final{
        LexSource::reset();
        m_captureSize=0;
        *m_capture=0;
    }

    char* captureBuffer() final {return m_capture;};
    size_t captureCapacity() const final {return TCapacity-1;};
    size_t captureSize() const final {return m_captureSize;}
    void clearCapture() final {
        m_captureSize=0;
        *m_capture=0;
        // clear out of memory errors
        if(OutOfMemoryError==error())
            clearError();
    }
};
#ifndef ARDUINO
class FileLexSource : public virtual LexSource {
    FILE* m_pfile;
    FileLexSource(FileLexSource& rhs)=delete;
    FileLexSource(FileLexSource&& rhs)=delete;
    FileLexSource& operator=(FileLexSource& rhs)=delete;

protected:
    int16_t read() final {
        if(!m_pfile)
            return LexSource::Closed;
        int16_t result = fgetc(m_pfile);
        if(-1==result)
            return LexSource::EndOfInput;
        return result;
    }
public:
    FileLexSource() {
        m_pfile = nullptr;
    }
    ~FileLexSource() override {}
    bool open(const char* filename) {
        if(nullptr!=m_pfile)
            return false;
        m_pfile=fopen(filename,"rb");
        if(nullptr==m_pfile)
            return false;
        reset();
        return true;
    }
    bool attach(FILE* pfile) {
        if(nullptr==pfile)
            return false;
        if(nullptr!=m_pfile)
            return false;
        reset();
        m_pfile = pfile;
        return true;
    }
    bool detach() {
        if(nullptr==m_pfile)
            return false;
        m_pfile = nullptr;
        return true;
    }
    void close() {
        if(nullptr!=m_pfile) {
            fclose(m_pfile);
            m_pfile = nullptr;
        }
    }
};
#endif
class SZLexSource : public virtual LexSource {
    const char * m_sz;
    SZLexSource(SZLexSource& rhs)=delete;
    SZLexSource(SZLexSource&& rhs)=delete;
    SZLexSource& operator=(SZLexSource& rhs)=delete;

protected:
    int16_t read() final {
        if(nullptr==m_sz)
            return LexSource::Closed;
        if(0==(*m_sz))
            return LexSource::EndOfInput;
        int16_t result = (unsigned char)*(m_sz++);
        if(0==result)
            return LexSource::EndOfInput;
        return result;
    }
    bool skipToAny(const char* characters7bit,unsigned long long& position,int16_t& match,int8_t& error) final {
        if(nullptr==m_sz){
            match = 0;
            error=Closed;
            return false;
        }

        const char*sz=strpbrk(m_sz-1, characters7bit);
        if(nullptr==sz) {
            // advance everything to the end since we found jack
            size_t c = strlen(m_sz);
            position+=c;
            m_sz +=c;
            error = EndOfInput;
            return false;
        }

        match = *sz;
        position += (sz - m_sz)+1;
        m_sz = sz+1;
        error=0;
        return true;
    }

public:
    SZLexSource() {
        m_sz = nullptr;
    }
    ~SZLexSource() override {}
    bool attach(const char* sz) {
        if(nullptr==sz)
            return false;
        if(nullptr!=m_sz)
            return false;
        reset();
        m_sz = sz;
        return true;
    }
    bool detach() {
        if(nullptr==m_sz)
            return false;
        m_sz = nullptr;
        return true;
    }
};
#if defined ARDUINO
template<size_t TCapacity> class ArduinoLexSource : public StaticLexSource<TCapacity>, public virtual LexSource {
    Stream* m_pstream;
    protected:
        int16_t read() final {
            if(nullptr==m_pstream)
                return Closed;
            return (int16_t)m_pstream->read();
        }
    public:
        bool begin(Stream& stream) {
            m_pstream = &stream;
            reset();
            return true;
        }
};
#endif

#ifndef ARDUINO
template<size_t TCapacity> class StaticFileLexSource : public StaticLexSource<TCapacity>, public virtual FileLexSource {

};
#endif
template<size_t TCapacity> class StaticSZLexSource : public StaticLexSource<TCapacity>, public virtual SZLexSource {

};
} // namespace lex
#endif