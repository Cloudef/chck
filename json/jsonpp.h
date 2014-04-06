#ifndef JSONPP_H
#define JSONPP_H

#include "json.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <iostream>

namespace json
{
   class Value
   {
   public:
      enum class Type {
         NONE,
         NULL_JSON,
         BOOLEAN,
         NUMBER,
         STRING,
         ARRAY,
         OBJECT,
      };

      class ValueException : public std::runtime_error
      {
      public:
         ValueException(std::string const& reason) : std::runtime_error(reason)
         {
         }
      };

      static Value object(std::unordered_map<std::string, Value> properties = {})
      {
         Value v(chckJsonNew(CHCK_JSON_TYPE_OBJECT));
         for(std::pair<std::string const, Value> const& prop : properties)
         {
            v.set(prop.first, prop.second);
         }
         return v;
      }

      static Value null()
      {
         return Value(chckJsonNew(CHCK_JSON_TYPE_NULL));
      }

      static Value array(std::initializer_list<Value> values = {})
      {
         return Value(values);
      }

      Value(chckJson* value = nullptr) : _value(value)
      {

      }

      Value(bool value) : Value(chckJsonNewBool(value))
      {

      }

      Value(int value) : Value(chckJsonNewNumberLong(value))
      {

      }

      Value(long value) : Value(chckJsonNewNumberLong(value))
      {

      }

      Value(double value) : Value(chckJsonNewNumberDouble(value))
      {

      }

      Value(char const* value) : Value(chckJsonNewString(value))
      {

      }

      Value(std::string const& value) : Value(chckJsonNewString(value.data()))
      {

      }

      Value(std::initializer_list<Value> values) : Value(chckJsonNew(CHCK_JSON_TYPE_ARRAY))
      {
         for(Value const& value : values)
         {
            append(value);
         }
      }

      Value(Value const& other) : Value(other._value != nullptr ? chckJsonCopy(other._value) : nullptr)
      {

      }

      Value(Value&& other) : Value(other._value)
      {
         other._value = nullptr;
      }

      ~Value()
      {
         if(_value != nullptr)
         {
            chckJsonFreeAll(_value);
         }
      }

      Value& operator=(Value const& other)
      {
         if(this != &other)
         {
            _value = other._value != nullptr ? chckJsonCopy(other._value) : nullptr;
         }
         return *this;
      }

      Value& operator=(Value&& other)
      {
         if(this != &other)
         {
            _value = other._value;
            other._value = nullptr;
         }
         return *this;
      }

      Type type() const
      {
         Type type = Type::NONE;
         switch(chckJsonGetType(_value))
         {
            case CHCK_JSON_TYPE_NULL:
            {
               type = Type::NULL_JSON;
               break;
            }
            case CHCK_JSON_TYPE_BOOL:
            {
               type = Type::BOOLEAN;
               break;
            }
            case CHCK_JSON_TYPE_NUMBER:
            {
               type = Type::NUMBER;
               break;
            }
            case CHCK_JSON_TYPE_STRING:
            {
               type = Type::STRING;
               break;
            }
            case CHCK_JSON_TYPE_ARRAY:
            {
               type = Type::ARRAY;
               break;
            }
            case CHCK_JSON_TYPE_OBJECT:
            {
               type = Type::OBJECT;
               break;
            }
            default:
            {
               throw ValueException("Unknown value type!");
            }
         }
         return type;
      }

      bool isNull() const
      {
         return type() == Type::NULL_JSON;
      }
      bool booleanValue() const
      {
         if(type() != Type::BOOLEAN)
            throw ValueException("Value is not a boolean");

         return chckJsonGetLong(_value);
      }
      long longValue() const
      {
         if(type() != Type::NUMBER)
            throw ValueException("Value is not a number");

         return chckJsonGetLong(_value);
      }
      double doubleValue() const
      {
         if(type() != Type::NUMBER)
            throw ValueException("Value is not a number");

         return chckJsonGetDouble(_value);
      }

      std::string stringValue() const
      {
         if(type() != Type::STRING)
            throw ValueException("Value is not a string");

         return chckJsonGetString(_value);
      }

      unsigned int size() const
      {
         if(type() != Type::ARRAY)
            throw ValueException("Value is not an array");

         unsigned int result = 0;
         chckJsonGetChild(_value, &result);
         return result;
      }

      Value at(unsigned int index) const
      {
         if(type() != Type::ARRAY)
            throw ValueException("Value is not an array");

         return Value(chckJsonCopy(chckJsonGetChildAt(_value, index)));
      }
      Value& append(Value const& value)
      {
         if(type() != Type::ARRAY)
            throw ValueException("Value is not an array");

         chckJsonChildAppend(_value, chckJsonCopy(value._value));
         return *this;
      }

      Value get(std::string const& property) const
      {
         if(type() != Type::OBJECT)
            throw ValueException("Value is not an object");

         return Value(chckJsonCopy(chckJsonGetProperty(_value, property.data())));
      }

      bool has(std::string const& property) const
      {
         if(type() != Type::OBJECT)
            throw ValueException("Value is not an object");

         return chckJsonGetProperty(_value, property.data()) != nullptr;
      }
      Value& set(std::string const& property, Value const& value)
      {
         if(type() != Type::OBJECT)
            throw ValueException("Value is not an object");

         chckJsonProperty(_value, property.data(), chckJsonCopy(value._value));
         return *this;
      }

      std::vector<std::string> properties() const
      {
         if(type() != Type::OBJECT)
            throw ValueException("Value is not an object");

         std::vector<std::string> result;
         for(chckJson* prop = chckJsonGetChild(_value, nullptr); prop != nullptr; prop = chckJsonGetNext(prop))
         {
            result.push_back(chckJsonGetString(prop));
         }
         return result;
      }

      static Value parse(std::string const& str)
      {
         chckJsonDecoder* decoder = chckJsonDecoderNew();
         chckJson* result = chckJsonDecoderDecode(decoder, str.data());
         chckJsonDecoderFree(decoder);
         return result;
      }

      static Value parse(std::istream& stream)
      {
         std::string str;
         stream.seekg(0, std::ios::end);
         str.reserve(stream.tellg());
         stream.seekg(0, std::ios::beg);

         str.assign((std::istreambuf_iterator<char>(stream)),
                    std::istreambuf_iterator<char>());

         return parse(str);

      }

      static Value parseFile(std::string const& path)
      {
         std::ifstream stream(path);
         return parse(stream);
      }

      std::string toString() const
      {
         char* chars = chckJsonEncode(_value, NULL);
         std::string result(chars);
         free(chars);
         return result;
      }

      chckJson* extract()
      {
         chckJson* result = _value;
         _value = nullptr;
         return result;
      }

   private:
      chckJson* _value;

   };
}
#endif // JSONPP_H

/* vim: set ts=8 sw=3 tw=0 :*/
