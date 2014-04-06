#ifndef JSONPP_H
#define JSONPP_H

#include "json.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <thread>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <iostream>

namespace json
{
   namespace internal
   {
      class Shared;

      std::weak_ptr<Shared> _sharedRef;
      std::shared_ptr<Shared> getShared()
      {
         std::shared_ptr<Shared> result = _sharedRef.lock();
         if(!result)
         {
            result = std::make_shared<Shared>();
            _sharedRef = result;
         }
         return result;
      }


      struct Local
      {
         Local(std::weak_ptr<Shared> shared) : shared(shared.lock()), decoder(chckJsonDecoderNew())
         {

         }

         ~Local()
         {
            chckJsonDecoderFree(decoder);
         }

         std::shared_ptr<Shared> shared;
         chckJsonDecoder* decoder;
      };

      class Shared
      {
      public:
         Shared() : _prevId(0), _prevLocal(), _locals()
         {

         }

         ~Shared()
         {

         }

         std::shared_ptr<Local> getLocal()
         {
            std::shared_ptr<Local> result;
            std::thread::id id = std::this_thread::get_id();
            if(id == _prevId)
            {
               result = _prevLocal.lock();
            }

            if(!result)
            {
               result = _locals[id].lock();

               if(!result)
               {
                  result = std::make_shared<Local>(getShared());
                  _locals.insert({id, std::weak_ptr<Local>(result)});
               }

               _prevId = id;
               _prevLocal = result;
            }

            return result;
         }

      private:
         std::thread::id _prevId;
         std::weak_ptr<Local> _prevLocal;
         std::unordered_map<std::thread::id, std::weak_ptr<Local>> _locals;
      };

   }

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

      Value(chckJson* value = nullptr) : _value(value),
         _local(getLocal())
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

      Value(Value const& other) : _value(other._value != nullptr ? chckJsonCopy(other._value) : nullptr),
         _local(getLocal())
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
         auto shared = internal::getShared();
         auto local = shared->getLocal();
         chckJsonDecoder* decoder = local->decoder;
         return chckJsonDecoderDecode(decoder, str.data());
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

   private:
      static std::shared_ptr<internal::Local> getLocal()
      {
         return internal::getShared()->getLocal();
      }

      std::shared_ptr<internal::Local> _local;

      chckJson* _value;

   };

   Value object(std::unordered_map<std::string, Value> properties = {})
   {
      return Value::object(properties);
   }
   Value null()
   {
      return Value::null();
   }

   Value array(std::initializer_list<Value> values = {})
   {
      return Value::array(values);
   }
}
#endif // JSONPP_H

/* vim: set ts=8 sw=3 tw=0 :*/
