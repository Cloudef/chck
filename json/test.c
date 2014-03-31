#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

void errorcb(chckJsonDecoder *decoder, unsigned int line, unsigned int position, const char *linePtr, chckJsonError code, const char *message)
{
   printf("[%u, %u]: %s (%d)\n", line, position, message, code);

   if (linePtr) {
      printf("%s\n", linePtr);
      printf("%*s%c\n", position - 1, " ", '^');
   }

   void *userdata = chckJsonDecoderGetUserdata(decoder);
   *(int*)userdata = 0;
}

int compareNoWhitespace(const char *a, const char *b)
{
   while (1) {
      while (*a && isspace(*a)) ++a;
      while (*b && isspace(*b)) ++b;
      if (*a != *b)
         return 0;
      else if (!*a || !*b)
         break;
      ++a, ++b;
   }
   return (*a == *b);
}

int main(void)
{
   /* TEST: decoding JSON standard */
   {
      /* http://code.google.com/p/json-smart/wiki/FeaturesTests */
      const char *tests[] = {
         "{}", /* Support Empty Object */
         "{ \"v\":\"1\"}", /* Support simple Object String value */
         "{ \"v\":\"1\"\r\n}", /* Space Tester */
         "{ \"v\":1}", /* Support simple Object int value */
         "{ \"v\":\"ab'c\"}", /* Support simple Quote in String */
         "{ \"PI\":3.141E-10}", /* Support simple Object float value */
         "{ \"PI\":3.141e-10}", /* Support lowcase float value */
         "{ \"v\":12345123456789}", /* Long number support */
         "{ \"v\":123456789123456789123456789}", /* Bigint number support */
         "[ { }, { },[]]", /* Array of empty Object */
         "{ \"v\":\"\\u2000\\u20ff\"}", /* Support lowercase Unicode Text */
         "{ \"v\":\"\\u2000\\u20FF\"}", /* Support uppercase Unicode Text */
         "{ \"a\":\"hp://foo\"}", /* Support non protected / text */
         "{ \"a\":null}", /* Support null */
         "{ \"a\":true}", /* Support boolean */
         "{ \"a\" : true }", /* Support non trimed data */
         "{ \"v\":1.7976931348623157E308}", /* Double precision floating point */
         "{ \"a\":{ \"b\":[0,1,2,333,5], \"www\":\"私\" }, \"c\":[ { \"d\":0 } ] }",
         NULL
      };

      int result = 1;
      chckJsonDecoder *decoder = chckJsonDecoderNew();
      assert(decoder != NULL);
      chckJsonDecoderUserdata(decoder, &result);
      chckJsonDecoderErrorCallback(decoder, errorcb);

      int i;
      for (i = 0; tests[i]; ++i) {
         result = 1;
         const char *json = tests[i];
         printf("%s\n", json);
         chckJson *djson = chckJsonDecoderDecode(decoder, json);

         /* don't run tests on jsons that contain unicode.
          * we don't encode unicode to hexdecimal for size reasons. */
         if (strstr(json, "\\u") == 0) {
            char *encode = chckJsonEncode(djson, NULL);
            assert(compareNoWhitespace(encode, json) == 1);
            free(encode);
         }

         if (djson) chckJsonFreeAll(djson);
         assert(result == 1 && "Standard JSON tests should pass!");
      }

      chckJsonDecoderFree(decoder);
   }

   /* TEST: decoding non RFC 4627 standard JSON */
   {
      /* NON RFC 4627 Tests */
      const char *tests[] = {
         "{ \"v\":'ab\"c'}", /* Support Double Quote in Simple Quoted Text */
         "{ \"v\":str}", /* Support non protected String value */
         "{ \"v\":It's'Work}", /* Support simple quote in non protected string value */
         "{ a:1234}", /* Support non protected keys */
         "{ \"v\":s1 s2}", /* Support non protected String value with space */
         "{ \"v\":s1 s2 }", /* Support non protected String value having special spaces */
         "{ \"a\":\"foo.bar\"}#toto", /* Support gardbage tailling comment */
         "{ 'value':'string'}", /* Support Simple Quote Stings */
         "{v:15-55}", /* Support non protected start like text value */
         "{v:15%}", /* Support non protected start like text value */
         "{v:15.06%}", /* Support non protected start like text value */
         "{} /* C comment */", /* Support a C comment */
         "{} // C++ comment", /* Support a C++ comment */

         /* Trap tests */
         "{ \"v\":s1' s2}", /* Support non protected String value having quote 1 */
         "{ \"v\":s1\" \"s2}", /* Support non protected String value having quote 2 */
         "{ \"NaN\":NaN}", /* NaN Test Value */
         "{ a,b:123}", /* Support non protected keys contains , */
         "{ a]b:123}", /* Support non protected keys contains ] */
         "{\"X\":\"s", /* Trucated value */
         "{\"X", /* Trucated key */
         NULL
      };

      int result = 1;
      chckJsonDecoder *decoder = chckJsonDecoderNew();
      assert(decoder != NULL);
      chckJsonDecoderUserdata(decoder, &result);
      chckJsonDecoderErrorCallback(decoder, errorcb);

      int i;
      for (i = 0; tests[i]; ++i) {
         result = 1;
         const char *json = tests[i];
         printf("%s\n", json);
         chckJson *djson = chckJsonDecoderDecode(decoder, json);
         if (djson) chckJsonFreeAll(djson);
         assert(result == 0 && "Non standard JSON tests should fail!");
      }

      chckJsonDecoderFree(decoder);
   }

   /* TEST: decoding using comments extension */
   {
      const char *tests[] = {
         "/* this is a C comment */ { /* this is a C comment 2 */ } /* this is a C comment 3 */",
         "{} // This is a C++ comment",
         "// C++ comment that blocks rest { \"object\":0 }",
         "{ \"object\": { \"number\": 1 /* C Comment */ } }",
         NULL
      };

      int result = 1;
      chckJsonDecoder *decoder = chckJsonDecoderNew();
      assert(decoder != NULL);
      chckJsonDecoderUserdata(decoder, &result);
      chckJsonDecoderAllowComments(decoder, 1);
      chckJsonDecoderErrorCallback(decoder, errorcb);

      int i;
      for (i = 0; tests[i]; ++i) {
         result = 1;
         const char *json = tests[i];
         printf("%s\n", json);
         chckJson *djson = chckJsonDecoderDecode(decoder, json);
         if (djson) chckJsonFreeAll(djson);
         assert(result == 1 && "Comments extension should pass!");
      }

      chckJsonDecoderFree(decoder);
   }

   /* TEST: decoding bad comments using comments extension */
   {
      const char *tests[] = {
         "/* Unclosed C comment",
         NULL
      };

      int result = 1;
      chckJsonDecoder *decoder = chckJsonDecoderNew();
      assert(decoder != NULL);
      chckJsonDecoderUserdata(decoder, &result);
      chckJsonDecoderAllowComments(decoder, 1);
      chckJsonDecoderErrorCallback(decoder, errorcb);

      int i;
      for (i = 0; tests[i]; ++i) {
         result = 1;
         const char *json = tests[i];
         printf("%s\n", json);
         chckJson *djson = chckJsonDecoderDecode(decoder, json);
         if (djson) chckJsonFreeAll(djson);
         assert(result == 0 && "Bad comments should not pass!");
      }

      chckJsonDecoderFree(decoder);
   }

   /* TEST: building and encoding a json tree */
   {
      const char *json = "{\"a\":256, \"b\":{[\"c\":[]]}, \"t\":true, \"f\":false, \"n\":null}";

      chckJson *r = chckJsonNew(CHCK_JSON_TYPE_OBJECT);
      chckJson *a = chckJsonNewString("a");
      chckJsonChild(r, a);

      chckJson *n256 = chckJsonNewNumberLong(256);
      chckJsonChild(a, n256);

      chckJson *b = chckJsonNewStringf("%c", 'b');
      chckJsonNext(a, b);

      chckJson *bc = chckJsonNew(CHCK_JSON_TYPE_OBJECT);
      chckJsonChild(b, bc);

      chckJson *bca = chckJsonNew(CHCK_JSON_TYPE_ARRAY);
      chckJsonChild(bc, bca);

      chckJson *c = chckJsonNewString("c");
      chckJsonChild(bca, c);

      chckJson *ca = chckJsonNew(CHCK_JSON_TYPE_ARRAY);
      chckJsonChild(c, ca);

      chckJson *t = chckJsonNewString("t");
      chckJsonChild(t, chckJsonNewBool(1));
      chckJsonNext(b, t);

      chckJson *f = chckJsonNewString("f");
      chckJsonChild(f, chckJsonNewBool(0));
      chckJsonNext(t, f);

      chckJson *n = chckJsonNewString("n");
      chckJsonChild(n, chckJsonNew(CHCK_JSON_TYPE_NULL));
      chckJsonNext(f, n);

      char *encode = chckJsonEncode(r, NULL);
      printf("%s\n", encode);
      assert(compareNoWhitespace(encode, json) == 1);
      free(encode);
      chckJsonFreeAll(r);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
