#include "jsonparser.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <set>

namespace JSON {
    static const std::set<char> whitespace = {' ', '\n', 13, '\t'};

    Value::Value(bool b) noexcept {
        mBoolean = b;
        if(b) mTag = ValueTag::jtrue;
        else mTag = ValueTag::jfalse;
    }
    Value::Value(Number n) noexcept : mTag(ValueTag::jnumber), mNumber{n} {}
    Value::Value(std::string&& str) noexcept : mTag(ValueTag::jstring), mString(str) {}
    Value::Value(Array&& a) noexcept : mTag(ValueTag::jarray), mArray(a) {}
    Value::Value(Object&& o) noexcept : mTag(ValueTag::jobject), mObject(o) {}
    Value::Value(Value&& other) noexcept : mTag(other.mTag) {
        switch(mTag) {
            case ValueTag::jnull: mBoolean = false; break;
            case ValueTag::jfalse: mBoolean = false; break;
            case ValueTag::jtrue: mBoolean = true; break;
            case ValueTag::jnumber: mNumber = other.mNumber; break;
            case ValueTag::jstring: 
                new (&mString) std::string{std::move(other.mString)};
                break;
            case ValueTag::jarray: 
                new (&mArray) Array{std::move(other.mArray)};
                break;
            case ValueTag::jobject: 
                new (&mObject) Object{std::move(other.mObject)};
                break;
        }
        other.mTag = ValueTag::jnull;
    }
    Value& Value::operator=(Value&& rhs) noexcept {
        this->~Value();
        switch(rhs.mTag) {
            case ValueTag::jnull: mBoolean = false; break;
            case ValueTag::jfalse: mBoolean = false; break;
            case ValueTag::jtrue: mBoolean = true; break;
            case ValueTag::jnumber: mNumber = rhs.mNumber; break;
            case ValueTag::jstring: 
                new (&mString) std::string{std::move(rhs.mString)};
                break;
            case ValueTag::jarray: 
                new (&mArray) Array{std::move(rhs.mArray)};
                break;
            case ValueTag::jobject: 
                new (&mObject) Object{std::move(rhs.mObject)};
                break;
        }
        mTag = rhs.mTag;
        rhs.mTag = ValueTag::jnull;
        return *this;
    }
    Value::~Value() noexcept {
        switch(mTag) {
            case ValueTag::jstring: 
                mString.~basic_string();
                break;
            case ValueTag::jobject: 
                for(auto [key, v] : mObject) delete v;
                mObject.~Object();
                break;
            case ValueTag::jarray: 
                for(Value* v : mArray) delete v;
                mArray.~Array();
                break;
            default: break;
        }
    }
    ValueTag Value::getTag() const noexcept {
        return mTag;
    }
    bool Value::getBool() const noexcept {
        return mBoolean;
    }
    const Number& Value::getNumber() const noexcept {
        return mNumber;
    }
    const std::string& Value::getString() const noexcept {
        return mString;
    }
    std::string&& Value::moveString() noexcept {
        mTag = ValueTag::jnull;
        return std::move(mString);
    }
    const Array& Value::getArray() const noexcept {
        return mArray;
    }
    Array&& Value::moveArray() noexcept {
        mTag = ValueTag::jnull;
        return std::move(mArray);
    }
    const Object& Value::getObject() const noexcept {
        return mObject;
    }
    Object&& Value::moveObject() noexcept {
        mTag = ValueTag::jnull;
        return std::move(mObject);
    }

    Error<Value> parse(std::istringstream& txt) noexcept;

    Error<Value> parse(const std::string& text) noexcept {
        std::istringstream jsontxt(text);
        return parse(jsontxt);
    }

    Error<Value> parseObject(std::istringstream& txt) noexcept;
    Error<Value> parseArray(std::istringstream& txt) noexcept;
    Error<std::string> parseString(std::istringstream& txt) noexcept;
    void parseWhitespace(std::istringstream& txt) noexcept;
    Value parseNumber(std::istringstream& txt) noexcept;
    Error<Value> parseNull(std::istringstream& txt) noexcept;
    Error<Value> parseTrue(std::istringstream& txt) noexcept;
    Error<Value> parseFalse(std::istringstream& txt) noexcept;

    Error<Value> parse(std::istringstream& txt) noexcept {
        char c = txt.peek();
        if(txt.eof()) return fatal<Value>("Tried to parse at EOF!");
        switch(c) {
        case '{':
            return parseObject(txt);
        case '[':
            return parseArray(txt);
        case '\"': {
            Error<std::string> str = parseString(txt);
            if(!str) return str.moveError<Value>();
            return Value{str.moveData()};
        }
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
    Error<Value> parseNull(std::istringstream& txt) noexcept {
        static const char* nullstr = "null";
        static const std::size_t len = strlen(nullstr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) return fatal<Value>("Failed to parse null");
        if(strcmp(buff, nullstr) != 0) return fatal<Value>(std::format("Failed to parse null: \"{}\" is not null", std::string_view(buff, len)));
        return Value{}; 
    }
    Error<Value> parseTrue(std::istringstream& txt) noexcept {
        static const char* truestr = "true";
        static const std::size_t len = strlen(truestr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) return fatal<Value>("Failed to parse true");
        if(strcmp(buff, truestr) != 0) return fatal<Value>(std::format("Failed to parse true: \"{}\" is not true", std::string_view(buff, len)));
        return Value(true); 
    }
    Error<Value> parseFalse(std::istringstream& txt) noexcept {
        static const char* falsestr = "false";
        static const std::size_t len = strlen(falsestr);
        char buff[len];
        txt.read(buff, len);
        if(txt.gcount() < len) return fatal<Value>("Failed to parse false");
        if(strcmp(buff, falsestr) != 0) return fatal<Value>(std::format("Failed to parse false: \"{}\" is not false", std::string_view(buff, len)));
        return Value(false); 
    }

    void parseWhitespace(std::istringstream& txt) noexcept {
        while(true) {
            char c = txt.peek();
            if(txt.eof() or !whitespace.contains(c)) break;
            else txt.ignore();
        }
    }
    Error<Value> parseObject(std::istringstream& txt) noexcept {
        //We know first character is '{'
        txt.ignore();
        parseWhitespace(txt);
        Object obj;
        //Go until reaching the closing bracket
        while(true) {
            char c = txt.peek();
            if(txt.eof()) return fatal<Value>("Failed to parse object: reached EOF");
            switch(c) {
            default: return fatal<Value>(std::format("Failed to parse object: found unexpected character \'{}\'", c));
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
                if(obj.contains(fieldName)) return fatal<Value>(std::format("Failed to parse object: had duplicate field name \"{}\"", fieldName));
                //Skip until reaching the colon
                parseWhitespace(txt);
                if(txt.peek() != ':') return fatal<Value>(std::format("Failed to parse object: expected \':\', got \'{}\'", txt.peek()));
                //Next character is a colon
                txt.ignore();
                Error<Value> value = parse(txt);
                if(value) obj[fieldName] = new Value(value.moveData());
                else return value;
                break;
            }
        }
    }
    Error<Value> parseArray(std::istringstream& txt) noexcept {
        //We know first character is '['
        txt.ignore();
        parseWhitespace(txt);
        Array arr;
        while(true) {
            char c = txt.peek();
            if(txt.eof()) return fatal<Value>("Failed to parse array: reached EOF");
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
                auto value = parse(txt);
                if(value) arr.push_back(new Value(value.moveData()));
                else return value;
                break;
            }
        }
    }
    Error<std::string> parseString(std::istringstream& txt) noexcept {
        //We know first character is '\"'
        txt.ignore();
        std::string str;
        while(true) {
            char c = txt.get();
            if(txt.eof()) return fatal<std::string>("Failed to parse string: reached EOF");
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
    Value parseNumber(std::istringstream& txt) noexcept {
        Number result;
        while(true) {
            char c = txt.get();
            if(txt.eof()) goto EndParseNumber;
            switch(c) {
                case '-': result.sign = -1; break;
                case '.': goto EndParseWhole;
                default:
                    if(!std::isdigit(c)) {
                        txt.putback(c);
                        goto EndParseWhole;
                    }
                    result.whole *= 10;
                    result.whole += c - '0';
            }
        }
        EndParseWhole:
        while(true) {
            char c = txt.get();
            if(txt.eof()) goto EndParseNumber;
            switch(c) {
                case 'E':
                case 'e': goto EndParsePart;
                default:
                    if(!std::isdigit(c)) {
                        txt.putback(c);
                        goto EndParsePart;
                    }
                    result.part *= 10;
                    result.part += c - '0';
                    result.partPlace *= 10;
            }
        }
        EndParsePart:
        while(true) {
            char c = txt.get();
            if(txt.eof()) goto EndParseNumber;
            switch(c) {
                case '-': result.exponentSign = -1; break;
                default:
                    if(!std::isdigit(c)) {
                        txt.putback(c);
                        goto EndParseNumber;
                    }
                    result.exponent *= 10;
                    result.exponent += c - '0';
            }
        }
        EndParseNumber:
        parseWhitespace(txt);
        return Value{result};
    }

    void serializeValue(std::ostringstream& str, const Value& v, int indentCount = 1) noexcept;

    std::string serialize(const Value& v) noexcept {
        std::ostringstream output;
        serializeValue(output, v);
        return output.str();
    }
    void serializeValue(std::ostringstream& str, const Value& v, int indentCount) noexcept {
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
            if(v.getNumber().sign == -1) str << '-';
            str << v.getNumber().whole;
            if(v.getNumber().part > 0) {
                u64 place = v.getNumber().partPlace;
                u64 part = v.getNumber().part;
                while(part % place == part) {
                    str << '0';
                    place /= 10;
                }
                str << '.' << v.getNumber().part;
            }
            if(v.getNumber().exponent != 0) {
                str << 'e';
                if(v.getNumber().exponentSign == -1) str << '-';
                str << v.getNumber().exponent;
            }
            break;
        case ValueTag::jstring:
            str << "\"" << v.getString() << "\"";
            break;
        case ValueTag::jobject:
            str << "\n";
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "{\n";
            index = 0;
            for(const auto [fieldname, value] : v.getObject()) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                str << "\"" << fieldname << "\" : ";
                serializeValue(str, *value, indentCount + 1);
                if(index++ < v.getObject().size()) {
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
            for(const Value* value : v.getArray()) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                serializeValue(str, *value, indentCount + 1);
                if(index++ < v.getArray().size()) {
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
