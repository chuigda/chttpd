# chttpd
Toy HTTP server implementation, with tons of toys used

## ⚠️⚠️⚠️ WARNING ⚠️⚠️⚠️
This project is totally a toy, implemented together with tons of toys by myself.
Codes are not guaranteed to work, and not guaranteed to be safe, of course. Except for the
fact that this project should not be used in real-world production (you may want OpenResty instead),
the C programming language used is not of good practice. Don't read this project code if you're
beginner, in order not to be misleaded.

## ⚠️⚠️⚠️ KNOWN ISSUES ⚠️⚠️⚠️
 - Minor memory leak from `getdelim` function

## 🔨 Build
This project supoorts POSIX platform in theory. Tested on the following platforms:
  - x86_64-unknown-linux-gnu
  - i386-unknown-linux-gnu
  - i386-unknown-linux-musl

To build, just clone the code and run `make` in the project directory. Seems that no much
dependencies are required.

For release build, the following `CFLAGS` setup is recommended:
```shell
CFLAGS="-O2 -g -fstack-protector-strong" make
```

## ⚙️ Configure
Configuration file of `chttpd` uses PL2BK DSL. A sample configuration looks like:
```
listen-address 127.0.0.1
listen-port 3080
max-pending 5

GET /           STATIC ./src/index.html
GET /index.html STATIC ./src/index.html
GET /robots.txt STATIC ./src/robots.txt
GET /api/login  DCGI   ./dcgi/liblogin.so
```

The first four lines (`listen-address`, `listen-port`, `max-pending`) configures how the HTTP
server works. When not appointed, chttpd will use default value for them (see `src/config.c`).
Among these attributes, I don't really know what `max-pending` means (this is a parameter of
POSIX socket listening, see `src/main.c`).

The following four lines are routes. A route has the following format:
```
HTTP-METHOD request-path HANDLER-TYPE handler-path
```

By this time, `STATIC` handler (for serving static files) and `DCGI` handler (for serving dynamic
contents) are supported. For more informations about these handlers please refer to the following
sections. Note that if one of the `handler-path`s is incorrect, `chttpd` does not immediately
figure out your mistake, but give you a `500` when that route gets used.

PL2 is another toy of mine. See [PL2 infrared missile](https://github.com/PL2-Lang/PL2)

## 📂 Serving static files
By using `STATIC` handler you can serve static files. Unfortunately, by this time `chttpd` is not
capable of handling directory structures (since it uses accurate path matching), so user must
explicitly appoint every single file path.

`chttpd` supports very limited mime guessing. See `src/static.c` for more information.

## 🔄 Serving dynamic contents

### 🗡️ By using DCGI
`DCGI` (Dynamic Common Gateway Interface) is a interface exploiting dynamic library utilities. To
use `DCGI`, you need to:
  - Write a C module, define a `dcgi_main` function, write handing mechanism within that function.
  - Compile the C module as shared object (`libxxx.so`)
  - Add route to that library

A `dcgi_main` function looks like:

```c
int dcgi_main(/* input */  const char *queryPath,      /* 1 */
              /* input */  const StringPair headers[], /* 2 */
              /* input */  const StringPair params[],  /* 3 */
              /* input */  const char *body,           /* 4 */
              /* output */ StringPair **headerDest,    /* 5 */
              /* output */ char **dataDest,            /* 6 */
              /* output */ char **errDest) {           /* 7 */
  /* your handling code */
  return your_return_code;                /* 8 */
}
```

Explainations
  1. `queryPath`: The path part of HTTP request, not including query parameters. 
     - example:
       - `/index.html`
       - `/robots.txt`
       - `/api/login`
  2. `headers`: Array of header pairs. the `first` part of `StringPair` is the header name, and 
     the `second` part of `StringPair` is the header value. No escape conversions is performed.
     The whole array ends with a `{ .first = NULL, .second = NULL }` pair.
     - example:
       ```
       {
         (StringPair) {"Content-Type", "text/plain"},
         (StringPair) {"Content-Length", "114514"},
         (StringPair) {"Content-Encoding", "identity"},
         (StringPair) {"User-Agent", "..."},
         (StringPair) {NULL, NULL}
       }
       ```
  3. `params`: Similar to `headers`, but stores query parameters.
     - example:
       ```
       {
         (StringPair) {"blogId", "114514"},
         (StringPair) {"reply", "1919810"},
         (StringPair) {NULL, NULL}
       }
       ```
  4. `body`: Null terminated string, presents if `Content-Length` is not zero, `NULL` otherwise.
  5. `headerDest`: Used for `dcgi_main` to output headers. Keep it untouched if no output header.
     Use "null-terminated array" structure, and make sure every string is on the heap.
     - example:
       ```c
       /* This should work with UTF-8, but not guaranteed to work with UTF16 or so */
       static char *copyString(const char *src) {
         size_t length = strlen(src);
         char *ret = (char*)malloc(length + 1);
         strcpy(ret, src);
         return ret;
       }
       
       typedef char* (StringPair)[2]; /* Use this to avoid including src/util.h in your module*/
       
       /* ... */
       *headerDest = (StringPair*)malloc(sizeof(StringPair) * 2);
       (*headerDest)[0][0] = copyString("Content-Type");
       (*headerDest)[0][1] = copyString("text/plain; charset=utf-8");
       (*headerDest)[1][0] = NULL;
       (*headerDest)[1][1] = NULL;
       ```
  6. `dataDest`: Used for `dcgi_main` to output response body. Keep it untouched if no response
     body. Use null-terminated string, and make sure it's on the heap.
     - example:
       ```c
       *dataDest = copyString("Excuse you!");
       ```
  7. `errDest`: Used for `dcgi_main` to output error information. Keep it untouched if no error.
  8. Return value: Return `0` or `200` to indicate successful response, any othre value to
     indicate failed response.

### 🥈 By using AgNO3
> Not implemented by this time
