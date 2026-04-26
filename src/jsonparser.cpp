module;

#include <cctype>
#include <cstring>
#include <exception>
#include <sstream>
#include <set>

module json;

namespace JSON {
    static const std::set<char> whitespace = {' ', '\n', 13, '\t'};

    void deletePtr(Value* ptr) {
        delete[] ptr;
    }

    Value parse(std::istringstream& txt);

    Value parse(const std::string& text) {
        std::istringstream jsontxt(text);
        return parse(jsontxt);
    }

    Value parseObject(std::istringstream& txt);
    Value parseArray(std::istringstream& txt);
    std::string parseString(std::istringstream& txt);
    void parseWhitespace(std::istringstream& txt);
    Value parseNumber(std::istringstream& txt);
    Value parseNull(std::istringstream& txt);
    Value parseTrue(std::istringstream& txt);
    Value parseFalse(std::istringstream& txt);

    Value parse(std::istringstream& txt) {
        char c = txt.peek();
        if(txt.eof()) throw std::exception();
        switch(c) {
        case '{':
            return parseObject(txt);
        case '[':
            return parseArray(txt);
        case '\"':
            return Value{parseString(txt)};
        case 'n':
            return parseNull(txt);
        case 't':
            return parseTrue(txt);
        case 'f':
            return parseFalse(txt);
        default:
            if(whitespace.contains(c)) {
                //Parse whitespace
                parseWhitespace(txt);
                //Try to parse the value again
                return parse(txt);
            }
            else {
                return parseNumber(txt);
            }
        }
    }
    Value parseNull(std::istringstream& txt) {
        static const char* nullstr = "null";
        static const std::size_t len = strlen(nullstr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) throw std::exception();
        if(strcmp(buff, nullstr) != 0) throw std::exception();
        return Value(); 
    }
    Value parseTrue(std::istringstream& txt) {
        static const char* truestr = "true";
        static const std::size_t len = strlen(truestr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) throw std::exception();
        if(strcmp(buff, truestr) != 0) throw std::exception();
        return Value(true); 
    }
    Value parseFalse(std::istringstream& txt) {
        static const char* falsestr = "false";
        static const std::size_t len = strlen(falsestr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) throw std::exception();
        if(strcmp(buff, falsestr) != 0) throw std::exception();
        return Value(false); 
    }

    void parseWhitespace(std::istringstream& txt) {
        while(true) {
            char c = txt.peek();
            if(txt.eof() or !whitespace.contains(c)) break;
            else txt.ignore();
        }
    }
    Value parseObject(std::istringstream& txt) {
        //We know first character is '{'
        txt.ignore();
        parseWhitespace(txt);
        Object obj;
        //Go until reaching the closing bracket
        while(true) {
            char c = txt.peek();
            if(txt.eof()) throw std::exception();
            switch(c) {
            default: throw std::exception();
            //End of object
            case '}':
                txt.ignore();
                parseWhitespace(txt);
                return Value{std::move(obj)};
            //Deliniates consecutive values
            case ',': 
                txt.ignore();
                parseWhitespace(txt); //fallthrough is intended
            //String identifier for a field
            case '\"':
                std::string fieldName = parseString(txt);
                //Objects cannot have duplicate field names
                if(obj.contains(fieldName)) throw std::exception();
                //Skip until reaching the colon
                parseWhitespace(txt);
                if(txt.peek() != ':') throw std::exception();
                //Next character is a colon
                txt.ignore();
                obj[fieldName] = new Value(parse(txt));
                break;
            }
        }
    }
    Value parseArray(std::istringstream& txt) {
        //We know first character is '['
        txt.ignore();
        parseWhitespace(txt);
        Array arr;
        while(true) {
            char c = txt.peek();
            if(txt.eof()) throw std::exception();
            //End of array
            switch(c) {
            case ']':
                txt.ignore();
                arr.shrink_to_fit();
                parseWhitespace(txt);
                return Value{std::move(arr)};
            case ',':
                txt.ignore(); //fallthrough is intended
            default:
                arr.push_back(new Value(parse(txt)));
            }
        }
    }
    std::string parseString(std::istringstream& txt) {
        //We know first character is '\"'
        txt.ignore();
        std::string str;
        while(true) {
            char c = txt.get();
            if(txt.eof()) return str;
            switch(c) {
            //Close quote
            case '\"':
            parseWhitespace(txt);
                return str;
            //Escape sequence
            case '\\':
                c = txt.get();
                switch(c) {
                    case 't': str.push_back('\t'); break;
                    case 'r': str.push_back(13); break;
                    case 'n': str.push_back('\n'); break;
                    case 'f': str.push_back(12); break;
                    case 'b': str.push_back(8); break;
                    //Don't care!
                    case 'u': break;
                    default: str.push_back(c); break;
                }
                break;
            //Any other character
            default: 
                str.push_back(c);
            }
        }
    }
    Value parseNumber(std::istringstream& txt) {
        int sign = 1, whole = 0, part = 0, partPlace = 1;
        while(true) {
            char c = txt.get();
            if(txt.eof()) goto EndParseNumber;
            switch(c) {
                case '-': sign = -1; break;
                default:
                    if(!std::isdigit(c)) {
                        txt.putback(c);
                        goto EndParseWhole;
                    }
                    whole *= 10;
                    whole += c - '0';
            }
        }
        EndParseWhole:
        while(true) {
            char c = txt.get();
            if(txt.eof()) goto EndParseNumber;
            if(c == '.') continue;
            else {
                if(!std::isdigit(c)) {
                    txt.putback(c);
                    goto EndParseNumber;
                }
                part *= 10;
                partPlace *= 10;
                part += c - '0';
            }
        }
        EndParseNumber:
        parseWhitespace(txt);
        return Value{sign * (whole + static_cast<double>(part) / partPlace)};
    }

    void serializeValue(std::ostringstream& str, const Value& v, int indentCount = 1);

    std::string serialize(const Value& v) {
        std::ostringstream output;
        serializeValue(output, v);
        return output.str();
    }
    void serializeValue(std::ostringstream& str, const Value& v, int indentCount) {
        int index = 0;
        switch(v.getTag()) {
        case ValueTag::jnull: 
            str << "null";
            break;
        case ValueTag::jfalse:
            str << "false";
            break;
        case ValueTag::jtrue:
            str << "true";
            break;
        case ValueTag::jnumber:
            str << std::to_string(v.getData().number);
            break;
        case ValueTag::jstring:
            str << "\"" << v.getData().str << "\"";
            break;
        case ValueTag::jobject:
            str << "\n";
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "{\n";
            index = 0;
            for(const auto [fieldname, value] : v.getData().object) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                str << "\"" << fieldname << "\" : ";
                serializeValue(str, *value, indentCount + 1);
                if(index++ < v.getData().object.size()) {
                    str << ",";
                }
                str << "\n";
            }
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "}";
            break;
        case ValueTag::jarray:
            str << "\n";
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "[\n";
            index = 0;
            for(const Value* value : v.getData().array) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                serializeValue(str, *value, indentCount + 1);
                if(index++ < v.getData().array.size()) {
                    str << ",";
                }
                str << "\n";
            }
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "]";
            break;
        }
    }
}
