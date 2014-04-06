#include "jsonpp.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <cassert>
#include <cctype>
#include <algorithm>

std::vector<std::string> const tests = {
   "{}", /* Support Empty Object */
   R"({ "v":"1"})", /* Support simple Object String value */
   "{ \"v\":\"1\"\r\n}", /* Space Tester */
   R"({ "v":1})", /* Support simple Object int value */
   R"({ "v":"ab'c"})", /* Support simple Quote in String */
   R"({ "PI":3.141E-10})", /* Support simple Object float value */
   R"({ "PI":3.141e-10})", /* Support lowcase float value */
   R"({ "v":12345123456789})", /* Long number support */
   R"({ "v":123456789123456789123456789})", /* Bigint number support */
   R"([ { }, { },[]])", /* Array of empty Object */
   //   R"({ "v":"\\u2000\\u20ff"})", /* Support lowercase Unicode Text */
   //   R"({ "v":"\\u2000\\u20FF"})", /* Support uppercase Unicode Text */
   R"({ "a":"hp://foo"})", /* Support non protected / text */
   R"({ "a":null})", /* Support null */
   R"({ "a":true})", /* Support boolean */
   R"({ "a" : true })", /* Support non trimed data */
   R"({ "v":1.7976931348623157E308})", /* Double precision floating point */
   R"({ "a":{ "b":[0,1,2,333,5], "www":"私" }, "c":[ { "d":0 } ] })"
};


bool compareIgnoringWhitespace(std::string const& a, std::string const& b)
{
   std::string aa = a;
   std::string bb = b;
   aa.erase(std::remove_if(aa.begin(), aa.end(), isspace), aa.end());
   bb.erase(std::remove_if(bb.begin(), bb.end(), isspace), bb.end());
   return aa == bb;
}

int main(int argc, char** argv)
{
   // Test initializations and assignment
   for(std::string const& str : tests)
   {
      json::Value value = json::Value::parse(str);
      assert(compareIgnoringWhitespace(value.toString(), str));

      json::Value copy(value);
      assert(compareIgnoringWhitespace(copy.toString(), str));
      assert(compareIgnoringWhitespace(value.toString(), str));

      json::Value copy2;
      copy2 = copy;
      assert(compareIgnoringWhitespace(copy2.toString(), str));
      assert(compareIgnoringWhitespace(copy.toString(), str));

      json::Value moved(std::move(copy));
      assert(compareIgnoringWhitespace(moved.toString(), str));

      json::Value moved2;
      moved2 = std::move(moved);
      assert(compareIgnoringWhitespace(moved2.toString(), str));
   }

   // Test array operations
   {
      json::Value array = json::Value::array();
      assert(array.size() == 0);
      json::Value values[] = {1, 2, true, "yay!"};
      for(json::Value const& value : values)
      {
         array.append(value);
      }
      assert(array.size() == 4);

      assert(array.at(0).longValue() == 1);
      assert(array.at(1).longValue() == 2);
      assert(array.at(2).booleanValue() == true);
      assert(array.at(3).stringValue() == "yay!");

      json::Value array2 = {1, 1.0, true, json::null(), {1, 2, 3}};
      assert(array2.size() == 5);
      assert(array2.at(0).longValue() == 1);
      assert(array2.at(1).doubleValue() < 1.1 && array2.at(1).doubleValue() > 0.9);
      assert(array2.at(2).booleanValue() == true);
      assert(array2.at(3).isNull());
      assert(array2.at(4).at(0).longValue() == 1);
      assert(array2.at(4).at(1).longValue() == 2);
      assert(array2.at(4).at(2).longValue() == 3);
   }

   // Test object operations
   {
      json::Value object = json::object({
            {"foo", 1},
            {"bar", false},
            {"bag", "<o/"}
            });
      assert(object.has("foo"));
      assert(object.has("bar"));
      assert(object.has("bag"));

      assert(!object.has("baz"));

      assert(object.get("foo").longValue() == 1);
      assert(object.get("bar").booleanValue() == false);
      assert(object.get("bag").stringValue() == "<o/");
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
