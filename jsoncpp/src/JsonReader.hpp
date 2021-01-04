#include <math.h>
#ifndef ARDUINO
#include <cinttypes>
#include <string.h>
#endif
#include "ArduinoCommon.h"
#include "LexSource.hpp"
#include "JsonUtility.hpp"
#include "JsonTree.hpp"

namespace json {
#define JSON_ERROR_NO_ERROR 0
#define JSON_ERROR_UNTERMINATED_OBJECT 1
const char JSON_ERROR_UNTERMINATED_OBJECT_MSG[] PROGMEM =  "Unterminated object";
#define JSON_ERROR_UNTERMINATED_ARRAY 2
const char JSON_ERROR_UNTERMINATED_ARRAY_MSG[] PROGMEM =  "Unterminated array";
#define JSON_ERROR_UNTERMINATED_STRING 3
const char JSON_ERROR_UNTERMINATED_STRING_MSG[] PROGMEM =  "Unterminated string";
#define JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY 4
const char JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY_MSG[] PROGMEM =  "Unterminated object or array";
#define JSON_ERROR_FIELD_NO_VALUE 5
const char JSON_ERROR_FIELD_NO_VALUE_MSG[] PROGMEM =  "Field has no value";
#define JSON_ERROR_UNEXPECTED_VALUE 6
const char JSON_ERROR_UNEXPECTED_VALUE_MSG[] PROGMEM =  "Unexpected value";
#define JSON_ERROR_UNKNOWN_STATE 7
const char JSON_ERROR_UNKNOWN_STATE_MSG[] PROGMEM =  "Unknown state";
#define JSON_ERROR_OUT_OF_MEMORY 8
const char JSON_ERROR_OUT_OF_MEMORY_MSG[] PROGMEM =  "Out of memory";
#define JSON_ERROR_INVALID_ARGUMENT 9
const char JSON_ERROR_INVALID_ARGUMENT_MSG[] PROGMEM =  "Invalid argument";
#define JSON_ERROR_END_OF_DOCUMENT 10
const char JSON_ERROR_END_OF_DOCUMENT_MSG[] PROGMEM =  "End of document";
#define JSON_ERROR_NO_DATA 11
const char JSON_ERROR_NO_DATA_MSG[] PROGMEM =  "No data";
#define JSON_ERROR_FIELD_NOT_SUPPORTED 12
const char JSON_ERROR_FIELD_NOT_SUPPORTED_MSG[] PROGMEM =  "Individual fields cannot be returned";
#define JSON_ERROR_UNEXPECTED_FIELD 13
const char JSON_ERROR_UNEXPECTED_FIELD_MSG[] PROGMEM =  "Unexpected field";
#define JSON_ERROR_IO_ERROR 14
const char JSON_ERROR_IO_ERROR_MSG[] PROGMEM =  "I/O Error";
#define JSON_ERROR_INVALID_VALUE 15
const char JSON_ERROR_INVALID_VALUE_MSG[] PROGMEM = "Invalid value";
#define JSON_ERROR_FIELD_TOO_LONG 16
const char JSON_ERROR_FIELD_TOO_LONG_MSG[] PROGMEM = "Field name too long. Field names cannot be streamed.";
#define JSON_ERROR(x) error(JSON_ERROR_ ## x,JSON_ERROR_ ## x ## _MSG)

    class JsonReader {
    public:
        // the initial node
        static const int8_t Initial = -1;
        // the end document/final node
        static const int8_t EndDocument = -2;
        // an error node
        static const int8_t Error = -3;
        // a value node
        static const int8_t Value = 0;
        // a field node
        static const int8_t Field = 1;
        // an array node
        static const int8_t Array = 2;
        // an end array node
        static const int8_t EndArray = 3;
        // an object node
        static const int8_t Object = 4;
        // an end object node
        static const int8_t EndObject = 5;
        // a partial value - for values too big to load into memory at once
        static const int8_t ValuePart = 6;
        // the end of a partial value
        static const int8_t EndValuePart = 7;

        // the type is undefined. this indicates an error in reading the value
        static const int8_t Undefined = -1;
        // this type is a null
        static const int8_t Null = 0;
        // this type is a string
        static const int8_t String = 1;
        // this type is a real number (floating point)
        static const int8_t Real = 2;
        // this type is an integer (not part of the JSON spec)
        static const int8_t Integer = 3;
        // this type is a boolean
        static const int8_t Boolean = 4;

        // this axis is a fast forward search that ignores heirarchy
        static const int8_t Forward = 0;
        // this axis retrieves sibling fields
        static const int8_t Siblings = 1;
        // this axis retrieves descendant fields
        static const int8_t Descendants = 2;

    private:
        int8_t m_state ;
        int8_t m_valueType ;
        JsonLexState m_lexState;
        uint8_t m_lastError;
        lex::LexSource& m_lc;
        unsigned long int m_objectDepth;

        void error(uint8_t code,const char* msg) {
            m_lastError = code;
            size_t c = strlen(msg);
            if(c<m_lc.captureCapacity()) {
                STRNCPYP(m_lc.captureBuffer(),msg,c+1);
            } else {
                STRNCPYP(m_lc.captureBuffer(),msg,m_lc.captureCapacity());
            }
        }
        void error(const lex::LexSource& src) {
            if(!src.hasError())
                return;
            switch (src.error())
            {
            case lex::LexSource::IOError:
                JSON_ERROR(IO_ERROR);
                m_state = Error; // unrecoverable
                break;
            case lex::LexSource::OutOfMemoryError:
                JSON_ERROR(OUT_OF_MEMORY);
                break;
            default:
                break;
            }
        }
        bool clearError() {
            if(Error==m_state)
                return false;
            m_lastError = JSON_ERROR_NO_ERROR;
            return true;
        }
        bool recoverValue() {
            uint32_t cp;
            // recover the cursor
            m_state = Value;
            while(m_lc.advance() && ']'!=(cp=m_lc.current())&&'}'!=cp&&','!=cp);
            if(m_lc.hasError()) {
                m_state = Error; // unrecoverable
                error(m_lc);
                return false;
            }
            return true;
        }
        bool parseSubtreeImpl(MemoryPool& pool,JsonElement* pelem,bool skipFinalRead=false) {
            size_t c;
            JsonElement e;
            char* sz;
            char* sz2;
            switch (m_state)
            {
                case EndDocument:
                    JSON_ERROR(END_OF_DOCUMENT);
                    return false;
                case Initial:
                    if (!read()) {
                        JSON_ERROR(NO_DATA);
                        return false;
                    }
                    return parseSubtreeImpl(pool,pelem,skipFinalRead);
                case ValuePart:
                    switch(valueType()) {
                        case Null:
                            while(read() && m_valueType!=EndValuePart);
                            if(hasError())
                                return false;
                            e.null(nullptr);
                            break;

                        case Real:
                            while(read() && m_valueType!=EndValuePart);
                            if(hasError())
                                return false;
                            e.real(realValue());
                            break;

                        case Integer:
                            while(read() && m_valueType!=EndValuePart);
                            if(hasError())
                                return false;
                            e.integer(integerValue());
                            break;

                        break;
                        case Boolean:
                            while(read() && m_valueType!=EndValuePart);
                            if(hasError())
                                return false;
                            e.boolean(booleanValue());
                            break;

                        case String:
                            c=strlen(value());
                            sz =(char*)pool.alloc(c);
                            if(nullptr==sz) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;
                            }
                            memcpy(sz,value(),c);

                            while(read() && m_state!=EndValuePart) {
                                c=strlen(value());
                                sz2 =(char*)pool.alloc(c);
                                if(nullptr==sz2) {
                                    JSON_ERROR(OUT_OF_MEMORY);
                                    return false;
                                }
                                memcpy(sz2,value(),c);

                            }
                            if(hasError() || EndValuePart!=m_state)
                                return false;
                            sz2=(char*)pool.alloc(1);
                            if(nullptr==sz2) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;
                            }
                            *sz2=0;
                            e.string(sz);
                            break;
                    }
                    if(!skipFinalRead)
                        read();
                    else {
                        if(!JsonUtility::skipWhiteSpace(m_lc)) {
                            error(m_lc);
                            return false;
                        }
                        if(!skipIfComma())
                            return false;
                        m_state = Field;
                    }
                    break;
                case Value:
                    switch(valueType()) {
                        case Null:
                            e.null(nullptr);
                            break;

                        case Real:
                            e.real(realValue());
                            break;

                        case Integer:
                            e.integer(integerValue());
                            break;

                        break;
                        case Boolean:
                            e.boolean(booleanValue());
                            break;

                        case String:
                            if(!e.allocString(pool,value())) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;
                            }
                            break;
                    }
                    if(!skipFinalRead)
                        read();
                    else {
                        if(!JsonUtility::skipWhiteSpace(m_lc)) {
                            error(m_lc);
                            return false;
                        }
                        if(!skipIfComma())
                            return false;
                        m_state = Field;
                    }
                    break;
                case Field:
                    // we have no structure with which to return a field
                    JSON_ERROR(FIELD_NOT_SUPPORTED);
                    return false;

                case Array:
                    e.parray(nullptr);
                    if(!read())
                        return false;
                    while (EndArray != m_state) {
                        JsonElement je;
                        JsonElement* pje = (JsonElement*)pool.alloc(sizeof(JsonElement));
                        *pje=je;
                        //printf("Before pool used: %d\r\n",(int)pool.used());
                        if(false==(parseSubtreeImpl(pool,pje)) || !e.addItem(pool,pje)) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                        //printf("After pool used: %d\r\n",(int)pool.used());
                    }
                    if(!skipFinalRead) {
                        if(EndArray==m_state && !read())
                            return false;
                    } else {
                        if(!skipIfComma())
                            return false;
                    }
                    break;
                case EndArray:
                case EndObject:
                    // we have no data to return
                    JSON_ERROR(NO_DATA);
                    return false;
                case Object:// begin object
                    e.pobject(nullptr);
                    if(!read()) {
                        return false;
                    }
                    //printf("Before pool used: %d\r\n",(int)pool.used());
                    while (!hasError() && EndObject != m_state) {
                        c=strlen(value())+1;
                        char *fn=(char*)pool.alloc(c);
                        if(nullptr==fn) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                        strcpy(fn,value());
                        if(!read()) {
                            if(hasError())
                                return false;
                            JSON_ERROR(FIELD_NO_VALUE);
                            return false;
                        }
                        JsonElement je;
                        JsonElement* pje = (JsonElement*)pool.alloc(sizeof(JsonElement));
                        if(nullptr==pje) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                        *pje=je;
                        if(nullptr==fn || false==(parseSubtreeImpl(pool,pje)) || !e.addFieldPooled(pool,fn,pje)) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                    }
                    if(!skipFinalRead) {
                        if(EndObject==m_state && !read())
                            if(EndDocument!=m_state)
                                return false;
                    } else {
                        if(!skipIfComma())
                            return false;
                    }
                    break;
                default:
                    JSON_ERROR(UNKNOWN_STATE);
                    return false;
            }
            if(!e.undefined()) {
                *pelem= e;
                return true;
            }
            return false;
        }
        bool extractImpl(MemoryPool& pool,JsonExtractor& extraction,bool subcall) {
            //if(m_state==EndObject) asm("int $3");
            if(0==extraction.count) {
                if(m_state==Object || m_state==Value || m_state==ValuePart || m_state==Array || m_state==Initial) {
                    if(!parseSubtreeImpl(pool,extraction.presult,subcall))
                        return false;
                    return true;
                }
                return false;
            }
            int matched;
            size_t c;
            size_t idx;
            unsigned long int depth;
            switch(m_state) {
                case JsonReader::Initial:
                    if(!read())
                        return false;
                    return extractImpl(pool,extraction,false);

                case JsonReader::Array:
                    if(extraction.pindices==nullptr) {
                        // we're not on an array extraction.
                        return false;
                    }
                    c= extraction.count;
                    idx = 0;
                    depth = m_objectDepth;
                    while(!hasError() && m_state!=EndArray) {
                        matched = -1;
                        for(size_t i = 0;i<extraction.count;++i) {
                            if(extraction.pindices[i]==idx) {
                                matched = i;
                                break;
                            }
                        }
                        if(-1!=matched) {
                            if(nullptr!=extraction.pchildren) {
                                if(!read())
                                    return false;
                                if(EndArray==m_state)
                                    break;
                                // extract the array value
                                if(!extractImpl(pool,extraction.pchildren[matched],true))
                                    return false;
                            } else {
                                JSON_ERROR(INVALID_ARGUMENT);
                                if(!skipToEndArray())
                                    m_state = Error; // can't recover unless we have a known good location
                                return false;
                            }
                            --c;
                           if(0==c) {
                                if(!skipToEndArray()) {
                                    return false;
                                }
                                break;
                            }
                        } else {
                            if(!skipObjectOrArrayOrValuePart())
                                return false;
                            if(!skipIfComma())
                                return false;
                            ++idx;
                        }

                    }
                    if(m_state!=EndArray)
                        return false;
                    skipIfComma();
                    return !hasError();
                case JsonReader::Object:
                    if(extraction.pfields==nullptr) {
                        // we're not on an object extraction.
                        return false;
                    }
                    matched=-1;
                    c= extraction.count;
                    // prepare our allocated space by copying our field pointers
                    depth = m_objectDepth;
                    while(!hasError() && ((m_state!=EndObject || (m_objectDepth>=depth)||'\"'==m_lc.current()))) {
                        // we need space for extra string pointers for scanMatchFields()
                        // scanMatchFields() is destructive to the pointers we give it.
                        // so if we haven't already, allocate it
                        if(nullptr==extraction.palloced) {
                            extraction.palloced = (const void**)pool.alloc(extraction.count*sizeof(char*));
                            if(nullptr==extraction.palloced) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;
                            }
                        }
                        // copy fresh field string pointers for scanMatchFields()
                        memcpy(extraction.palloced,extraction.pfields,extraction.count*sizeof(char*));

                        matched = scanMatchFields((const char**)extraction.palloced,extraction.count);
                        // free what we just used since we can't later. it's just better to
                        // use the pool temporarily like this, since we can't call unalloc() after
                        // this pointer gets "stale" (meaning another alloc() happens since)
                        pool.unalloc(extraction.count*sizeof(char*));
                        extraction.palloced = nullptr;
                        if(-1!=matched) {
                            if(nullptr!=extraction.pchildren) {
                                // read to the field value
                                if(!read()) {
                                    return false;
                                }
                                if(!this->extractImpl(pool,extraction.pchildren[matched],true))
                                        return false;
                                if(EndObject==m_state) {
                                    depth=m_objectDepth;
                                    skipIfComma();
                                    m_state=Value;
                                }
                            }
                            --c;
                            if(0==c) {
                                if(!this->skipToEndObject()) {
                                    return false;
                                }
                                break;
                            }
                        }/* else {
                            if(m_lc.position()==646)
                                asm("int $3");
                        } */
                    }
                    if(m_state!=EndObject)
                        return false;
                    depth=m_objectDepth;
                    skipIfComma();
                    return !hasError() ;//&& ((subcall || depth>m_objectDepth) || read());
            }
            return false;
        }
        bool scanMatchField(const char* field) {
            bool match = false;
            const char* sz;
            int32_t ch;
            if(!m_lc.more() || '\"'!=m_lc.current()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                JSON_ERROR(UNEXPECTED_VALUE);
                return false;
            }
            if (!m_lc.advance()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                JSON_ERROR(UNTERMINATED_STRING);
                m_state = Error; // unrecoverable
                return false;
            }
            match = true;
            sz=field;
            while(m_lc.more() && m_lc.current()!='\"')
            {
                if(0==(*sz))
                    break;
                if(!JsonUtility::undecorate(m_lc,ch,false)) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                    JSON_ERROR(INVALID_VALUE);
                    return false;
                }
                if(0==ch) break;
                if(ch!=*sz) {
                    match=false;
                    break;
                }
                ++sz;
            }
            if(m_lc.hasError()) {
                error(m_lc);
                return false;
            }
            // read to the end of the field so the reader isn't broken
            if(m_lc.more() && '\"'==m_lc.current()) {
                if(!m_lc.advance()) {
                    return false;
                }
                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                    error(m_lc);
                    return false;
                }
                if(m_lc.current()==':') { // it's a field. This is a match!
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = Field;
                    if(match && 0==(*sz))
                        return !hasError();
                }
            } else {
                // nothing to be done if it fails - i/o error or badly formed document
                // can't recover our cursor

                if(!JsonUtility::skipStringPart(m_lc,true)) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                    JSON_ERROR(UNTERMINATED_STRING);
                    m_state = Error;
                    return false;
                }
                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                    error(m_lc);
                    return false;
                }
                if(':'==m_lc.current()) { // it's a field. This is a match!
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = Field;
                    return false;
                } else {
                    JSON_ERROR(FIELD_NO_VALUE);
                    return false;
                }
            }
            return false;
        }
        int scanMatchFields(const char** fields,size_t fieldCount) {
            int result = -1;
            int32_t ch;
            if('\"'!=m_lc.current()) {
                JSON_ERROR(UNEXPECTED_VALUE);
                return -1;
            }
            if (!m_lc.advance()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                JSON_ERROR(UNTERMINATED_STRING);
                m_state = Error;
                return -1;
            }
            size_t fc = fieldCount;
            while(0<fc && m_lc.more() && '\"'!=m_lc.current())
            {
                if(!JsonUtility::undecorate(m_lc,ch,false)) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                    JSON_ERROR(UNTERMINATED_STRING);
                    m_state = Error;
                    return -1;
                }

                int32_t cpcmp;
                for(size_t i = 0;i<fieldCount;++i) {
                    const char *sz = fields[i];
                    if(nullptr!=sz) {
                        sz=JsonUtility::decodeUtf8(fields[i],cpcmp);
                        if(ch!=cpcmp) {
                            // disqualify this entry
                            fields[i]=nullptr;
                            --fc;
                            continue;
                        }
                        fields[i]=sz;
                    }
                }
            }
            if('\"'==m_lc.current()) {
                for(size_t i = 0;i<fieldCount;++i) {
                    if(fields[i] && 0==(*fields[i])) {
                        result = i;
                        break;
                    }
                }
                // if result != -1 there is potentially a match
                if(!m_lc.advance()) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                }
                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                    error(m_lc);
                    return false;
                }
                if(':'==m_lc.current()) { // it's a field. This is a match!
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return -1;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = Field;
                    // if we didn't match, skip past this field's value.
                    if(result==-1) {
                        if(!skipObjectOrArrayOrValuePart())
                            return -1;
                        if(!skipCommaOrEndObjectOrEndArray())
                            return -1;
                        if(EndObject!=m_state)
                            m_state = Field;
                        return -1;
                    }
                    return result;
                } else {
                    JSON_ERROR(FIELD_NO_VALUE);
                    return -1;
                }
            } else {
                // read to the end of the field so the reader isn't broken
                // nothing to be done if it fails - i/o error or badly formed document
                // can't recover our cursor
                if(!JsonUtility::skipStringPart(m_lc,true)) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                    JSON_ERROR(UNTERMINATED_STRING);
                    return -1;
                }
                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                    error(m_lc);
                    return false;
                }
                if(m_lc.current()==':') {
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(FIELD_NO_VALUE);
                        return -1;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    // skip past the field's value
                    if(!skipObjectOrArrayOrValuePart())
                        return -1;
                    if(!skipCommaOrEndObjectOrEndArray())
                        return -1;
                    if(EndObject!=m_state)
                        m_state = Field;
                    return -1;
                }
            }
            return -1;
        }
        bool scanMatchSiblings(const char* field) {
            while(true) {
                bool matched = scanMatchField(field);
                if(hasError())
                    return false;
                if(matched) {
                    return true;
                } else {
                    if(!skipObjectOrArrayOrValuePart())
                        return false;
                    if(!skipIfComma())
                        return false;
                }
            }
            return false;
        }
        bool readAnyOpen(bool allowFields=false) {
            int32_t cp;
            m_valueType = Undefined;
            switch(m_lc.current()) {
                case '[':
                    if(!m_lc.advance()) {
                        error(m_lc);
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(!m_lc.more()) {
                        JSON_ERROR(UNTERMINATED_ARRAY);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = Array;
                    return true;

                case '{':
                    if(!m_lc.advance()) {
                        error(m_lc);
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(m_lc.eof()) {
                        JSON_ERROR(UNTERMINATED_OBJECT);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = Object;
                    ++m_objectDepth;
                    return true;
                case '-':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    m_valueType=Integer;
                    m_lexState.flags.state=0;
                    m_lc.clearCapture();

                    while(JsonUtility::lexNumber(m_lc,m_lexState));

                    if(lex::LexSource::OutOfMemoryError==m_lc.error())  {
                        m_state = ValuePart;
                        return true;
                    }
                    if(m_lc.hasError()) {
                        error(m_lc);
                        recoverValue();
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(0!=m_lexState.flags.accept && !m_lc.hasError() && (m_lc.eof()||']'==(cp=m_lc.current())||'}'==cp||','==cp)) {
                        m_state = Value;
                        if(m_lexState.flags.fracPart>1) {
                            double p = pow(10,m_lexState.flags.eCount);
                            if(3==m_lexState.flags.fracPart) {
                                m_lexState.real/=p;
                                m_lexState.integer/=p;
                            } else {
                                m_lexState.real*=p;
                                m_lexState.integer*=p;
                                if((0>m_lexState.integer && 0==m_lexState.flags.negative)||(0<m_lexState.integer && 0!=m_lexState.flags.negative)) {
                                    m_lexState.flags.overflow = 1;
                                }
                            }
                        }
                        if(m_lexState.flags.fracPart!=0 || m_lexState.flags.overflow) {
                            m_valueType = Real;
                        } else {
                            m_valueType = Integer;
                        }
                        return true;
                    }
                    //recoverValue();
                    if(hasError())
                        return false;
                    JSON_ERROR(INVALID_VALUE);
                    return false;
                case 't':
                case 'f':
                case 'n':
                    m_valueType=('n'==m_lc.current())?Null:Boolean;
                    m_lexState.flags.state=0;
                    m_lexState.flags.accept=0;
                    m_lc.clearCapture();
                    while(JsonUtility::lexLiteral(m_lc,m_lexState));
                    if(lex::LexSource::OutOfMemoryError==m_lc.error())  {
                        m_state = ValuePart;
                        return true;
                    }
                    if(m_lc.hasError()) {
                        error(m_lc);
                        recoverValue();
                        return false;
                    }
                    JsonUtility::skipWhiteSpace(m_lc);
                    if(0!=m_lexState.flags.accept && !m_lc.hasError() && (']'==(cp=m_lc.current())||'}'==cp||','==cp||m_lc.eof())) {
                        m_state = Value;
                        return true;
                    }
                    recoverValue();
                    JSON_ERROR(INVALID_VALUE);
                    return false;
                case '\"':
                    m_state = Value;
                    m_lc.clearCapture();
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(UNTERMINATED_STRING);
                        return false;
                    }
                    while(JsonUtility::undecorate(m_lc,cp,true));
                    if(m_lc.hasError()) {
                        if(lex::LexSource::OutOfMemoryError== m_lc.error()) {
                            m_state = ValuePart;
                            m_lexState.flags.accept = 0;
                            m_valueType = String;
                            return true;
                        }
                        error(m_lc);
                        return false;
                    }

                    if('\"'!=m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_STRING);
                        return false;
                    }
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    m_valueType = String;
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(m_lc.more() && ':'==m_lc.current()) {
                        if(!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                        }
                        if(!JsonUtility::skipWhiteSpace(m_lc)) {
                            error(m_lc);
                            return false;
                        }
                        if(!allowFields) {
                            JSON_ERROR(UNEXPECTED_FIELD);
                            m_state = Error; // TODO: we can recover this but I haven't written the code for it
                            return false;
                        }
                        m_state = Field;
                    }
                    return true;

                default:

                    JSON_ERROR(INVALID_VALUE);
                    recoverValue();
                    return false;
            }
        }
        bool readValuePart() {
            int32_t cp;
            switch(m_valueType) {
                case String:

                    m_lc.clearCapture();
                    if(m_lexState.flags.accept==1) {
                        m_state = EndValuePart;
                        return true;
                    }
                    while(m_lc.captureCapacity()-m_lc.captureSize()>=4 && JsonUtility::undecorate(m_lc,cp,true));

                    if(m_lc.captureCapacity()-m_lc.captureSize()<4)
                        return true;

                    if(m_lc.hasError()) {
                        if(lex::LexSource::OutOfMemoryError==m_lc.error())
                            return true;
                        error(m_lc);
                        return false;
                    }

                    if('\"'!=m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_STRING);
                        return false;
                    }
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    m_lexState.flags.accept = 1;
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(m_lc.more() && ':'==m_lc.current()) {
                        JSON_ERROR(FIELD_TOO_LONG);
                        m_state = Error; // TODO: we can recover this but I haven't written the code for it yet
                        return false;
                    }
                    if(0==m_lc.captureSize())
                        m_state = EndValuePart;

                    return true;
                case Real:
                case Integer:
                    m_lc.clearCapture();
                    while(JsonUtility::lexNumber(m_lc,m_lexState));
                    if(lex::LexSource::OutOfMemoryError==m_lc.error())  {
                        m_state = ValuePart;
                        return true;
                    }
                    if(m_lc.hasError()) {
                        error(m_lc);
                        recoverValue();
                        return false;
                    }
                    JsonUtility::skipWhiteSpace(m_lc);
                    if(0!=m_lexState.flags.accept && !m_lc.hasError() && (']'==(cp=m_lc.current())||'}'==cp||','==cp||m_lc.eof())) {
                        m_state = EndValuePart;
                        if(m_lexState.flags.fracPart>1) {
                            double p = pow(10,m_lexState.flags.eCount);
                            if(3==m_lexState.flags.fracPart) {
                                m_lexState.real/=p;
                                m_lexState.integer/=p;
                            } else {
                                m_lexState.real*=p;
                                m_lexState.integer*=p;
                                if((0>m_lexState.integer && 0==m_lexState.flags.negative)||(0<m_lexState.integer && 0!=m_lexState.flags.negative)) {
                                    m_lexState.flags.overflow = 1;
                                }
                            }
                        }
                        if(m_lexState.flags.fracPart!=0 || m_lexState.flags.overflow) {
                            m_valueType = Real;
                        } else {
                            m_valueType = Integer;
                        }
                        return true;
                    }
                    recoverValue();
                    JSON_ERROR(INVALID_VALUE);
                    return false;
                case Null:
                case Boolean:
                    m_lc.clearCapture();
                    while(JsonUtility::lexLiteral(m_lc,m_lexState));
                    if(lex::LexSource::OutOfMemoryError==m_lc.error())  {
                        m_state = ValuePart;
                        return true;
                    }
                    if(m_lc.hasError()) {
                        error(m_lc);
                        recoverValue();
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(0!=m_lexState.flags.accept && !m_lc.hasError() && (']'==(cp=m_lc.current())||'}'==cp||','==cp||m_lc.eof())) {
                        m_state = EndValuePart;
                        return true;
                    }
                    recoverValue();
                    JSON_ERROR(INVALID_VALUE);
                    return false;
            }
            JSON_ERROR(INVALID_VALUE);
            // recover the cursor
            m_state = Value;
            while(m_lc.advance() && ']'!=(cp=m_lc.current())&&'}'!=cp&&','!=cp);
            if(m_lc.hasError()) {
                m_state = Error; // unrecoverable
                error(m_lc);
            }
            return false;
        }
        bool readAny() {
            if(!m_lc.more())
                return false;
            switch(m_lc.current()) {
                case ']':
                    return readEndArray();

                case '}':
                    return readEndObject();

            }
            if(!skipIfComma())
                return false;
            return readAnyOpen(true);
        }
        bool skipCommaOrEndObjectOrEndArray() {
            if(!m_lc.more())
                return false;
            switch(m_lc.current()) {
                case ',':
                    m_lc.advance();
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(!m_lc.more()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(UNTERMINATED_OBJECT_OR_ARRAY);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    break;
                case ']':
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(UNTERMINATED_ARRAY);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = EndArray;
                    break;
                case '}':
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        JSON_ERROR(UNTERMINATED_OBJECT);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    --m_objectDepth;
                    m_state = EndObject;
                    break;
                default:
                    return false;
            }
            return true;
        }
        bool skipIfComma() {
            if(m_lc.more() && ','==m_lc.current()) {
                if(!m_lc.advance()) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                }
                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                    error(m_lc);
                    return false;
                }
                if(m_lc.eof()) {
                    JSON_ERROR(UNTERMINATED_OBJECT_OR_ARRAY);
                    m_state = Error; // unrecoverable
                    return false;
                }
            }
            return true;
        }

        bool readEndObject() {
            if(!m_lc.more() || '}'!= m_lc.current())
                return false;
            if(!m_lc.advance()) {
                error(m_lc);
                return !hasError();
            }
            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                error(m_lc);
                return false;
            }

            m_state = EndObject;
            return true;
        }
        bool readEndArray() {
            if(!m_lc.more() || ']'!=m_lc.current())
                return false;
            if(!m_lc.advance()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
            }
            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                error(m_lc);
                return false;
            }
            m_state = EndArray;
            return true;
        }
        bool readFieldOrEndObject() {
            switch(m_lc.current()) {
                case '}':
                    --m_objectDepth;
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = EndObject;
                    return true;
                case '\"':
                    if(!readAnyOpen(true))
                        return false;
                    if(Field!=m_state ) {
                        JSON_ERROR(FIELD_TOO_LONG);
                        return false;
                    }

                    return true;
            }
            return false;
        }
        bool readValueOrEndArray() {
            switch(m_lc.current()) {
                case ']':
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    m_state = EndArray;
                    return true;

                default:
                    if(!readAnyOpen(false))
                        return false;
                    return true;
            }
        }
        // skips scalar values only
        bool skipValuePart() {
            clearError();
            if(!m_lc.more())
                return false;
            //int32_t cp;
            switch(m_lc.current()) {
                case '\"':
                    if(!skipString())
                        return false;
                    return true;
                default:
                    if(!m_lc.skipToAny(",]}")) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                        m_state = EndDocument;
                        return false;
                    }
                    m_state = Value;
                    return true;
            }
        }
        bool skipObjectOrArrayOrValuePart() {
            if(!m_lc.more()) return false;
            switch(m_lc.current()) {
                case '[':
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(m_lc.eof()) {
                        JSON_ERROR(UNTERMINATED_ARRAY);
                        m_state = Error;
                        return false;
                    }
                    if(!skipArrayPart())
                        return false;
                    break;
                case '{':
                    if(!m_lc.advance()) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        if(m_lc.hasError()) {
                            error(m_lc);
                            return false;
                        }
                    }
                    if(m_lc.eof()) {
                        JSON_ERROR(UNTERMINATED_OBJECT);
                        m_state = Error;
                        return false;
                    }
                    if(!skipObjectPart())
                        return false;
                    break;
                default:
                    if(!skipValuePart())
                        return false;
                    break;
            }
            return true;
        }
        bool skipString() {
            clearError();
            if ('\"' != m_lc.current()) {
                JSON_ERROR(UNTERMINATED_STRING);
                m_state=Error; // unrecoverable
                return false;
            }
            if(!m_lc.advance()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                JSON_ERROR(UNTERMINATED_STRING);
                m_state=Error; // unrecoverable
                return false;
            }

            if(!JsonUtility::skipStringPart(m_lc,true)) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                JSON_ERROR(UNTERMINATED_STRING);
                m_state=Error; // unrecoverable
                return false;
            }
            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
            }
            m_state = Value;
            return true;
        }
        bool skipObjectPart(int depth=1)
        {
            clearError();
            // TODO: this is so much faster when i don't have to track arrays
            // for some reason though, that breaks extract() and/or parseSubtree()
            char ch = m_lc.skipToAny("\"{}[]");
            while(0!=ch) {
                switch(ch) {
                    case '\"':
                        if(!skipString())
                            return false;
                        break;

                    case '{':
                        ++m_objectDepth;
                        ++depth;
                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                    break;

                    case ']':
                        --depth;
                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                        break;
                    case '[':
                        ++depth;
                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                        break;

                    case '}':
                        --depth;
                        --m_objectDepth;
                        if (0>=depth)
                        {
                            m_state=(m_lc.eof())?EndDocument:EndObject;
                            if(!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                            }
                            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                error(m_lc);
                                return false;
                            }
                            return true;
                        }
                        if(!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                        }

                        break;
                }

                ch=m_lc.skipToAny("\"{}[]");
            }

            if(m_lc.hasError()) {
                error(m_lc);
                return false;

            }
            return false;
        }

        bool skipArrayPart(int depth=1)
        {
            clearError();
            char ch = m_lc.skipToAny("\"{}[]");
            while(0!=ch) {
                switch (m_lc.current())
                {
                    case '\"':
                        if(!skipString())
                            return false;
                        break;

                    case '{':

                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                        ++m_objectDepth;

                        ++depth;
                        break;
                    case '}':

                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                        --m_objectDepth;

                        --depth;
                        break;

                    case '[':
                        if (!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                        ++depth;
                        break;


                    case ']':
                        if(!m_lc.advance()) {
                            if(m_lc.hasError()) {
                                error(m_lc);
                                return false;
                            }
                        }
                        --depth;
                        if (0>=depth) {
                            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                error(m_lc);
                                return false;
                            }
                            m_state=(m_lc.eof())?EndDocument:EndArray;
                            return true;
                        }

                    break;


                }
                ch = m_lc.skipToAny("\"{}[]");
            }

            return false;
        }
        JsonReader()=delete;
        JsonReader(const JsonReader& rhs) = delete;
        JsonReader(const JsonReader&& rhs) = delete;
        JsonReader& operator=(const JsonReader& rhs) = delete;
        JsonReader& operator=(const JsonReader&& rhs) = delete;
    public:
        // constructs an instance
        JsonReader(lex::LexSource& source) : m_state(Initial),m_valueType(Undefined),m_lastError(0), m_lc(source),m_objectDepth(0) {

        }
        // destroys an instance
        ~JsonReader() {

        }
        void reset() {
            m_state=Initial;
            m_objectDepth = 0;
            m_lastError = 0;
            m_valueType = Undefined;
            m_lc.reset();
        }
        // provides access to the LexSource being read from and captured to
        lex::LexSource& source() const { return m_lc; }
        // indicates whether there's an error
        bool hasError() const {
            return Error==m_state || JSON_ERROR_NO_ERROR!=m_lastError;
        }
        int8_t error() const {
            return m_lastError;
        }
        // executes a step of the parse
        bool read() {
            if(!m_lc.ensureStarted()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                m_state = EndDocument;
                return false;
            }
            if(Error==m_state)
                return false;
            if(Initial!=m_state && !m_lc.more() && lex::LexSource::OutOfMemoryError != m_lc.error()) {
                m_state = EndDocument;
                return false;
            }
            clearError();
            switch(m_state) {
                case Initial:
                    m_objectDepth=0;

                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                        error(m_lc);
                        return false;
                    }
                    if(!readAnyOpen(false))
                        return false;
                    break;

                case Value:
                    if(!readAny())
                        return false;
                    break;
                case ValuePart:
                    if(!readValuePart())
                        return false;
                    break;
                case EndValuePart:
                    if(!readAny())
                        return false;
                    break;
                case Object:
                    if(!readFieldOrEndObject())
                        return false;
                    break;

                case Field:
                    // handle field special by not breaking (doesn't change anything in current code)
                    return readAnyOpen(false);

                case EndObject:
                    if(!readAny())
                        return false;
                    break;

                case Array:
                    if(!readValueOrEndArray())
                        return false;
                    break;

                case EndArray:
                    if(!readAny())
                        return false;
                    break;

                case EndDocument:
                case Error:
                    return false;
                default:
                    JSON_ERROR(UNKNOWN_STATE);
                    return false;
            }
            return true;
        }
        // skips the substree under the cursor
        bool skipSubtree()
        {
            clearError();
            switch (m_state)
            {
            case JsonReader::Error:
            case JsonReader::EndDocument: // eos
                return false;
            case JsonReader::Initial: // initial
                if(!m_lc.ensureStarted()) {
                    if(m_lc.hasError()) {
                        error(m_lc);
                        return false;
                    }
                    m_state = EndDocument;
                    return false;
                }

                if (read() && Error!=nodeType())
                    return skipSubtree();
                return false;
            case JsonReader::Value: // value
                if(!skipObjectOrArrayOrValuePart())
                    return false;
                if(!read() || Error==m_state)
                    return false;
                return true;
            case JsonReader::Field: // field
                // we're doing this here to avoid loading values
                // if we just call read() to advance, the field's
                // value will be loaded. This is undesirable if
                // we don't care about the field because it uses
                // lexcontext space
                // instead we're advancing manually. We start right
                // after the field's ':' delimiter
                if(!skipObjectOrArrayOrValuePart())
                    return false;
                if(!read() || Error==m_state)
                    return false;
                return true;
            case JsonReader::Array:// begin array
                if(!skipArrayPart())
                    return false;
                if(m_state!=JsonReader::EndArray || !read() || Error==nodeType())
                    return false;
                return true;
            case JsonReader::Object:// begin object
                if(!skipObjectPart())
                    return false;
                if(m_state!=EndDocument && (m_state!=JsonReader::EndObject || !read() || Error==nodeType()))
                    return m_state==EndDocument;
                return true;
            case JsonReader::EndArray: // end array
            case JsonReader::EndObject: // end object
                if(Error==nodeType())
                    return false;
                return true;
            }
            return false;
        }
        bool skipToFieldValue(const char* field, int8_t axis, unsigned long int *pdepth = nullptr) {
            return skipToField(field,axis,pdepth) && read();
        }
        bool skipToField(const char* field, int8_t axis, unsigned long int *pdepth = nullptr) {
            clearError();
            if(!m_lc.ensureStarted()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                m_state = EndDocument;
                return false;
            }
            m_lc.clearCapture();
            if(!m_lc.more() ) {
                error(m_lc);
                return false;
            }
            if(!field) {
                JSON_ERROR(INVALID_ARGUMENT);
                return false;
            }
            bool match;
            int32_t ch;
            const char*sz;
            switch(axis) {
            case Siblings:
                switch(m_state) {
                    case Initial:
                        if(!read()||Object!=m_state)
                            return false;
                        // fall through
                    case Object:
                        if(scanMatchSiblings(field)) {
                            return true;
                        }
                        if(hasError())
                            return false;
                        break;

                    case Field:
                        if(!skipObjectOrArrayOrValuePart())
                            return false;
                        if(!skipIfComma())
                            return false;

                        if(scanMatchSiblings(field)) {
                            return true;
                        }
                        if(hasError())
                            return false;
                        break;
                }
                return false;
            break;
            case Descendants:
                if(nullptr==pdepth) {
                    JSON_ERROR(INVALID_ARGUMENT);
                    return false;
                }
                if(Initial==m_state) {
                    if(!read())
                        return false;
                }

                if(0==(*pdepth))
                    *pdepth=1;
                // fall through
            case Forward:
                if(!m_lc.ensureStarted()) {
                    error(m_lc);
                    return false;
                }
                int16_t sch;
                while (0!=(sch=m_lc.skipToAny("\"{}[]"))) {
                    switch (sch) {
                        case '\"':
                            if (!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                JSON_ERROR(UNTERMINATED_STRING);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            match = true;
                            sz=field;
                            while(m_lc.more() && m_lc.current()!='\"')
                            {
                                int32_t cp;
                                // could do this faster if we only decoded once and held it around
                                if(nullptr==(sz=JsonUtility::decodeUtf8(sz,cp)))
                                    break;
                                if(!JsonUtility::undecorate(m_lc,ch,false))
                                    break;
                                if(ch!=cp) {
                                    match=false;
                                    break;
                                }
                            }
                            // read to the end of the field so the reader isn't broken
                            if(m_lc.more() && '\"'==m_lc.current()) {
                                if(!m_lc.advance()) {
                                    if(m_lc.hasError()) {
                                        error(m_lc);
                                        return false;
                                    }
                                }
                                if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                    error(m_lc);
                                    return false;
                                }
                                if(m_lc.more() && ':'==m_lc.current()) { // it's a field. This is a match!
                                    if(!m_lc.advance()) {
                                        if(m_lc.hasError()) {
                                            error(m_lc);
                                            return false;
                                        }
                                        JSON_ERROR(FIELD_NO_VALUE);
                                        m_state = Error; // unrecoverable
                                        return false;
                                    }
                                    if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                        error(m_lc);
                                        return false;
                                    }
                                    m_state = Field;
                                    if(match && !(*sz))
                                        return !hasError();
                                }
                            } else if(!JsonUtility::skipStringPart(m_lc,true)) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                JSON_ERROR(UNTERMINATED_STRING);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            break;

                        case '{':
                            if(!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                JSON_ERROR(UNTERMINATED_OBJECT);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            if(pdepth)
                                ++(*pdepth);
                            ++m_objectDepth;
                            break;

                        case '[':
                            if(!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                JSON_ERROR(UNTERMINATED_ARRAY);
                                return false;
                            }
                            if(pdepth)
                                ++(*pdepth);
                            break;

                        case '}':
                            --m_objectDepth;
                            if(!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                return false;
                            }
                            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                            }
                            if(pdepth) {
                                --*pdepth;
                                if(!*pdepth) {
                                    m_state = EndObject;
                                    return false;
                                }
                            }
                            break;
                        case ']':
                            if(!m_lc.advance()) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                                return false;
                            }
                            if(!JsonUtility::skipWhiteSpace(m_lc)) {
                                if(m_lc.hasError()) {
                                    error(m_lc);
                                    return false;
                                }
                            }
                            if(pdepth) {
                                --*pdepth;
                                if(!*pdepth) {
                                    m_state = EndArray;
                                    return false;
                                }
                            }
                            break;

                        }
                    }
                    return false;
                default:
                    JSON_ERROR(INVALID_ARGUMENT);
                    return false;

            }
            return false;
        }
        bool skipToIndex(size_t index) {
            clearError();
            if(!m_lc.ensureStarted()) {
                if(m_lc.hasError()) {
                    error(m_lc);
                    return false;
                }
                m_state = EndDocument;
                return false;
            }
            if (Initial == m_state || Field == m_state) // initial or field
                if (!read() || Error==nodeType())
                    return false;
            if (Array == m_state) { // array start
                if (0 == index) {
                    if (!read() || Error==nodeType())
                        return false;
                }
                else {
                    for (size_t i = 0; i < index; ++i) {
                        if (EndArray == m_state) // end array
                            return false;
                        if (!skipObjectOrArrayOrValuePart())
                            return false;
                        if(!skipIfComma())
                            return false;
                    }
                    if(!read())
                        return false;
                }
                return true;
            }
            return false;
        }
        bool skipToEndObject() {
            if(EndObject==m_state)
                return true;
            clearError();
            if(!skipObjectPart(0))
                return false;
            return true;
        }
        bool skipToEndArray() {
            if(EndArray==m_state)
                return true;
            clearError();
            if(m_state==Error) return false;
            if(!skipArrayPart(0))
                return false;
            return true;
        }
        bool parseSubtree(MemoryPool& pool,JsonElement* pelem) {
            if(nullptr==pelem) {
                JSON_ERROR(INVALID_ARGUMENT);
                return false;
            }
            if(parseSubtreeImpl(pool,pelem))
                return true;
            return false;
        }
        bool extract(MemoryPool& pool,JsonExtractor& extraction) {
            if( extractImpl(pool,extraction,false)) {
                return read() || EndDocument==m_state;
            }
            return false;
        }
        bool hasValue() const {
            switch(m_state) {
                case Field:
                case Value:
                case ValuePart:
                case Error:
                    return true;
                case EndValuePart:
                    return m_valueType == Boolean || m_valueType==Integer || m_valueType == Real;
            }
            return hasError();
        }
        int8_t valueType() {
            if(hasValue())
                return m_valueType;
            return Undefined;
        }
        bool booleanValue() {
            return Boolean==valueType()&&2==m_lexState.flags.accept;
        }
        char* value() const {
            if(hasValue())
                return m_lc.captureBuffer();
            return nullptr;
        }
        long long int integerValue() const {
            if(hasError() || !hasValue()||(Integer!=m_valueType &&Real!=m_valueType))
                return 0;
            return m_lexState.integer;
        }
        double realValue() const {
            if(hasError() || !hasValue()||(Integer!=m_valueType &&Real!=m_valueType))
                return 0;
            return m_lexState.real;
        }
        int8_t nodeType() const {
            return m_state;
        }
    };
}
