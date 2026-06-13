#pragma once

#include <cmath>
#include <format>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mars_types.h"
#include "error.h"

namespace JSON {
    enum class Type : u32 {
        jnull,
        jtrue,
        jfalse,
        jnumber,
        jstring,
        jarray,
        jobject
    };

    constexpr std::string typeToString(Type tag) noexcept {
        switch(tag) {
            case Type::jnull: return "null";
            case Type::jtrue: return "true";
            case Type::jfalse: return "false";
            case Type::jnumber: return "number";
            case Type::jstring: return "string";
            case Type::jarray: return "array";
            case Type::jobject: return "object";
        }
        std::unreachable();
    }


    struct Number {
        u64 whole = 0;
        u64 part = 0;
        u64 partPlace = 1;
        u64 exponent = 0;
        i32 sign = 1;
        i32 exponentSign = 1;
        template<typename T> requires std::is_arithmetic<T>::value
        T to() const noexcept {
            return (sign * (whole + static_cast<double>(part) / partPlace)) * std::pow(10, static_cast<i64>(exponent * exponentSign));
        }
    };

    class Value {
        using Array = std::vector<Value>;
        using Object = std::unordered_map<std::string, Value>;
        Type mType = Type::jnull;
        union {
            bool mBoolean {false};
            std::string mString;
            Number mNumber;
            Array mArray;
            Object mObject;
        };
        public:
        Value() noexcept = default;
        explicit Value(bool b) noexcept;        
        explicit Value(int i) noexcept;
        explicit Value(Number n) noexcept;
        explicit Value(const std::string& str) noexcept;
        explicit Value(std::string&& str) noexcept;        
        explicit Value(const Array& a) noexcept;
        explicit Value(Array&& a) noexcept;
        explicit Value(const Object& o) noexcept;
        explicit Value(Object&& o) noexcept;
        Value(const Value& other) noexcept;
        Value(Value&& other) noexcept;        
        Value& operator=(const Value& rhs) noexcept;
        Value& operator=(Value&& rhs) noexcept;        
        ~Value() noexcept;        
        Type getType() const noexcept;        
        Error<bool> getBool() const noexcept;
        template<typename T> requires std::is_arithmetic<T>::value
        Error<T> getNumberAs() const noexcept {
            if(mType != Type::jnumber) {
                FATAL(std::format("Tried to get a number from a JSON value, but the type was {}", typeToString(mType)));
            }
            return mNumber.to<T>();
        }
        Error<const Number&> getNumber() const noexcept;
        Error<Number&> getNumber() noexcept;
        Error<const std::string&> getString() const noexcept;
        Error<std::string&> getString() noexcept;
        Error<const Array&> getArray() const noexcept;
        Error<Array&> getArray() noexcept;
        Error<const Object&> getObject() const noexcept;
        Error<Object&> getObject() noexcept;
    };

    template<class T>
    Error<T> valueTo(const Value& value) noexcept;

    template<>
    Error<bool> valueTo(const Value& value) noexcept;

    template<>
    Error<std::string> valueTo(const Value& value) noexcept;

    template<class T> requires std::is_arithmetic<T>::value
    Error<T> valueTo(const Value& value) noexcept {
        TRY_RETURN(value.getNumberAs<T>());
    }

    using Object = std::unordered_map<std::string, Value>;
    using Array = std::vector<Value>;

    Error<Value> parse(const std::string& text) noexcept;
    std::string serialize(const Value& v) noexcept;
}