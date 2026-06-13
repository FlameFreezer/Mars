#include "jsonparser.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <set>

namespace JSON {
    static const std::set<char> whitespace = {' ', '\n', 13, '\t'};

    Value::Value(bool b) noexcept {
        mBoolean = b;
        if(b) mType = Type::jtrue;
        else mType = Type::jfalse;
    }
    Value::Value(Number n) noexcept : mType(Type::jnumber), mNumber{n} {}
    Value::Value(const std::string& str) noexcept : mType(Type::jstring), mString(str) {}
    Value::Value(std::string&& str) noexcept : mType(Type::jstring), mString(std::move(str)) {}
    Value::Value(const Array& a) noexcept : mType(Type::jarray), mArray(a) {}
    Value::Value(Array&& a) noexcept : mType(Type::jarray), mArray(std::move(a)) {}
    Value::Value(const Object& o) noexcept : mType(Type::jobject), mObject(o) {}
    Value::Value(Object&& o) noexcept : mType(Type::jobject), mObject(std::move(o)) {}
    Value::Value(const Value& other) noexcept : mType(other.mType) {
        switch(mType) {
            case Type::jnull: mBoolean = false; break;
            case Type::jfalse: mBoolean = false; break;
            case Type::jtrue: mBoolean = true; break;
            case Type::jnumber: mNumber = other.mNumber; break;
            case Type::jstring: 
                new (&mString) std::string{other.mString};
                break;
            case Type::jarray: 
                new (&mArray) Array{other.mArray};
                break;
            case Type::jobject: 
                new (&mObject) Object{other.mObject};
                break;
        }
    }
    Value::Value(Value&& other) noexcept : mType(other.mType) {
        switch(mType) {
            case Type::jnull: mBoolean = false; break;
            case Type::jfalse: mBoolean = false; break;
            case Type::jtrue: mBoolean = true; break;
            case Type::jnumber: mNumber = other.mNumber; break;
            case Type::jstring: 
                new (&mString) std::string{std::move(other.mString)};
                break;
            case Type::jarray: 
                new (&mArray) Array{std::move(other.mArray)};
                break;
            case Type::jobject: 
                new (&mObject) Object{std::move(other.mObject)};
                break;
        }
        other.mType = Type::jnull;
    }
    Value& Value::operator=(const Value& rhs) noexcept {
        if(mType != rhs.mType) this->~Value();
        switch(rhs.mType) {
            case Type::jnull: mBoolean = false; break;
            case Type::jfalse: mBoolean = false; break;
            case Type::jtrue: mBoolean = true; break;
            case Type::jnumber: mNumber = rhs.mNumber; break;
            case Type::jstring: 
                new (&mString) std::string{rhs.mString};
                break;
            case Type::jarray: 
                new (&mArray) Array{rhs.mArray};
                break;
            case Type::jobject: 
                new (&mObject) Object{rhs.mObject};
                break;
        }
        mType = rhs.mType;
        return *this;
    }
    Value& Value::operator=(Value&& rhs) noexcept {
        if(mType != rhs.mType) this->~Value();
        switch(rhs.mType) {
            case Type::jnull: mBoolean = false; break;
            case Type::jfalse: mBoolean = false; break;
            case Type::jtrue: mBoolean = true; break;
            case Type::jnumber: mNumber = rhs.mNumber; break;
            case Type::jstring: 
                new (&mString) std::string{std::move(rhs.mString)};
                break;
            case Type::jarray: 
                new (&mArray) Array{std::move(rhs.mArray)};
                break;
            case Type::jobject: 
                new (&mObject) Object{std::move(rhs.mObject)};
                break;
        }
        mType = rhs.mType;
        rhs.mType = Type::jnull;
        return *this;
    }
    Value::~Value() noexcept {
        switch(mType) {
            case Type::jstring: 
                mString.~basic_string();
                break;
            case Type::jobject: 
                mObject.~Object();
                break;
            case Type::jarray: 
                mArray.~Array();
                break;
            default: break;
        }
    }
    Type Value::getType() const noexcept {
        return mType;
    }
    Error<bool> Value::getBool() const noexcept {
        if(mType != Type::jtrue and mType != Type::jfalse) {
            FATAL(std::format("Tried to get true or false from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mBoolean;
    }
    Error<const Number&> Value::getNumber() const noexcept {
        if(mType != Type::jnumber) {
            FATAL(std::format("Tried to get a number from a JSON value, but the type was {}", typeToString(mType)));
        }

        return mNumber;
    }
    Error<Number&> Value::getNumber() noexcept {
        if(mType != Type::jnumber) {
            FATAL(std::format("Tried to get a number from a JSON value, but the type was {}", typeToString(mType)));
        }

        return mNumber;

    }
    Error<const std::string&> Value::getString() const noexcept {
        if(mType != Type::jstring) {
            FATAL(std::format("Tried to get a string from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mString;
    }
    Error<std::string&> Value::getString() noexcept {
        if(mType != Type::jstring) {
            FATAL(std::format("Tried to get a string from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mString;

    }
    Error<const Array&> Value::getArray() const noexcept {
        if(mType != Type::jarray) {
            FATAL(std::format("Tried to get an array from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mArray;
    }
    Error<Array&> Value::getArray() noexcept {
        if(mType != Type::jarray) {
            FATAL(std::format("Tried to get an array from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mArray;
    }

    Error<const Object&> Value::getObject() const noexcept {
        if(mType != Type::jobject) {
            FATAL(std::format("Tried to get an object from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mObject;
    }
    Error<Object&> Value::getObject() noexcept {
        if(mType != Type::jobject) {
            FATAL(std::format("Tried to get an object from a JSON value, but the type was {}", typeToString(mType)));
        }
        return mObject;
    }

    Error<Value> parse(std::istringstream& txt) noexcept;

    Error<Value> parse(const std::string& text) noexcept {
        std::istringstream jsontxt(text);
        TRY_RETURN(parse(jsontxt));
    }

    template<>
    Error<bool> valueTo(const Value& value) noexcept {
        TRY_RETURN(value.getBool());
    }
    template<>
    Error<std::string> valueTo(const Value& value) noexcept {
        Error<const std::string&> res = value.getString();
        if (!res.okay()) {
            APPEND_SOURCE_INFO(res);
            return MOVE_ERROR(res);
        }
        else return res.value();
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
        if (txt.eof()) {
            FATAL("Tried to parse at EOF!");
        }
        switch(c) {
        case '{': 
            TRY_RETURN(parseObject(txt));
        case '[': 
            TRY_RETURN(parseArray(txt));
        case '\"': {
            Error<std::string> str = parseString(txt);
            if (!str.okay()) {
                APPEND_SOURCE_INFO(str);
                return MOVE_ERROR(str);
            }
            return Value{str.moveValue()};
        }
        case 'n': 
            TRY_RETURN(parseNull(txt));
        case 't': 
            TRY_RETURN(parseTrue(txt));
        case 'f': 
            TRY_RETURN(parseFalse(txt));
        default:
            if(whitespace.contains(c)) {
                //Parse whitespace
                parseWhitespace(txt);
                //Try to parse the value again
                TRY_RETURN(parse(txt));
            }
            else {
                return parseNumber(txt);
            }
        }
    }
    Error<Value> parseNull(std::istringstream& txt) noexcept {
        static const char* nullstr = "null";
        static const std::size_t len = strlen(nullstr);
        std::string buff{};
        buff.reserve(len);
        txt.read(buff.data(), buff.capacity());
        if (txt.gcount() < len) {
            FATAL("Failed to parse null");
        }
        if (strcmp(buff.data(), nullstr) != 0) {
            FATAL(std::format("Failed to parse null: \"{}\" is not null", std::string_view(buff)));
        }
        return Value{}; 
    }
    Error<Value> parseTrue(std::istringstream& txt) noexcept {
        static const char* truestr = "true";
        static const std::size_t len = strlen(truestr);
        std::string buff{};
        buff.reserve(len);
        txt.read(buff.data(), buff.capacity());
        if (txt.gcount() < len) {
            FATAL("Failed to parse true");
        }
        if (strcmp(buff.data(), truestr) != 0) {
            FATAL(std::format("Failed to parse true: \"{}\" is not true", std::string_view(buff)));
        }
        return Value{true}; 
    }
    Error<Value> parseFalse(std::istringstream& txt) noexcept {
        static const char* falsestr = "false";
        static const std::size_t len = strlen(falsestr);
        std::string buff{};
        buff.reserve(len);
        txt.read(buff.data(), buff.capacity());
        if (txt.gcount() < len) {
            FATAL("Failed to parse false");
        }
        if (strcmp(buff.data(), falsestr) != 0) {
            FATAL(std::format("Failed to parse false: \"{}\" is not false", std::string_view(buff)));
        }
        return Value{false}; 
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
            if (txt.eof()) {
                FATAL("Failed to parse object: reached EOF");
            }
            switch(c) {
            default: 
                FATAL(std::format("Failed to parse object: found unexpected character \'{}\'", c));
            //End of object
            case '}': 
                txt.ignore();
                parseWhitespace(txt);
                return Value{std::move(obj)};
            //Deliniates consecutive values
            case ',': 
                txt.ignore();
                parseWhitespace(txt);
                //We know after a comma is another field name, so the fallthrough is intentional
                [[fallthrough]];
            //String identifier for a field
            case '\"': 
                TRY_INIT(std::string, fieldName, parseString(txt));
                //Objects cannot have duplicate field names
                if (obj.contains(fieldName)) {
                    FATAL(std::format("Failed to parse object: had duplicate field name \"{}\"", fieldName));
                }
                //Skip until reaching the colon
                parseWhitespace(txt);
                if (txt.peek() != ':') {
                    FATAL(std::format("Failed to parse object: expected \':\', got \'{}\'", txt.peek()));
                }
                //Next character is a colon
                txt.ignore();
                Error<Value> value = parse(txt);
                if (!value.okay()) {
                    APPEND_SOURCE_INFO(value);
                    return MOVE_ERROR(value);
                }
                else {
                    obj[fieldName] = Value{value.moveValue()};
                }
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
            if (txt.eof()) {
                FATAL("Failed to parse array: reached EOF");
            }
            //End of array
            switch(c) {
            case ']':
                txt.ignore();
                arr.shrink_to_fit();
                parseWhitespace(txt);
                return Value{std::move(arr)};
            case ',':
                txt.ignore();
                //We know that after a comma is another value, so fallthrough is intended
                [[fallthrough]];
            default: 
                Error<Value> value = parse(txt);
                if (!value.okay()) {
                    APPEND_SOURCE_INFO(value);
                    return MOVE_ERROR(value);
                }
                else arr.push_back(Value{ value.moveValue() });
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
            if (txt.eof()) {
                FATAL("Failed to parse string: reached EOF");
            }
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
        switch(v.getType()) {
        case Type::jnull: 
            str << "null";
            break;
        case Type::jfalse:
            str << "false";
            break;
        case Type::jtrue:
            str << "true";
            break;
        case Type::jnumber: {
            const Number& n = v.getNumber().value();
            if(n.sign == -1) str << '-';
            str << n.whole;
            if(n.part > 0) {
                u64 place = n.partPlace;
                u64 part = n.part;
                while(part % place == part) {
                    str << '0';
                    place /= 10;
                }
                str << '.' << n.part;
            }
            if(n.exponent != 0) {
                str << 'e';
                if(n.exponentSign == -1) str << '-';
                str << n.exponent;
            }
            break;
        }
        case Type::jstring:
            str << "\"" << v.getString().value() << "\"";
            break;
        case Type::jobject:
            str << "\n";
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "{\n";
            index = 0;
            for(const auto [fieldname, value] : v.getObject().value()) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                str << "\"" << fieldname << "\" : ";
                serializeValue(str, value, indentCount + 1);
                if(index++ < v.getObject().value().size()) {
                    str << ",";
                }
                str << "\n";
            }
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "}";
            break;
        case Type::jarray:
            str << "\n";
            for(int i = 0; i < indentCount - 1; i++) {
                str << "\t";
            }
            str << "[\n";
            index = 0;
            for(const Value& value : v.getArray().value()) {
                for(int i = 0; i < indentCount; i++) {
                    str << "\t";
                }
                serializeValue(str, value, indentCount + 1);
                if(index++ < v.getArray().value().size()) {
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
