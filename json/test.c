#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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
         printf("%s\n", tests[i]);
         chckJsonDecoderDecode(decoder, json);
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
         chckJsonDecoderDecode(decoder, json);
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
         chckJsonDecoderDecode(decoder, json);
         assert(result == 1 && "Comments extension should pass!");
      }

      chckJsonDecoderFree(decoder);
   }

   return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=3 tw=0 :*/
