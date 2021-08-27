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
 - `SIGPIPE`s are simply ignored. No proof on negative effects by this time, though.
 - Suspicious crash (`SIGSEGV`?) after running for several days.

## 🔨 Build
This project supports all POSIX platforms in theory. Tested on the following platforms:
  - x86_64-unknown-linux-gnu
  - i386-unknown-linux-gnu
  - i386-unknown-linux-musl
  - cygwin x86_64
  - msys2

To build, just clone the code and run `make` in the project directory. Seems that no much
dependencies are required.

For release build (if you really want), the following `CFLAGS` setup is recommended:
```shell
CFLAGS="-O2 -g -fstack-protector-strong" make
```

There is a plan of porting `chttpd` to Windows platform, but no timetable. Don't rely
on this.

## ⚙️ Configure
Configuration file of `chttpd` uses PL2BK DSL. A sample configuration looks like:
```
listen-address 127.0.0.1
listen-port 3080
max-pending 5
cache-time 1800
preload true
case-ignore true

GET  !/robots.txt STATIC ./src/robots.txt
POST !/api/login  DCGI   ./dcgi/liblogin.so
GET  /            STATIC ./src/index.html
POST /            INTERN 403
```

The first 5 lines (`listen-address`, `listen-port`, `max-pending`) configures how the HTTP
server works. When not appointed, chttpd will use default value for them (see `src/config.c`).
Among these attributes, I don't really know what `max-pending` means (this is a parameter of
POSIX socket listening, see `src/main.c`).

`cache-time` controls the caching mechanism of HTTP. If `cache-time` was set to a non-negative
value, a corresponding `Cache-Control` will be added when serving static files.

`preload` controls the loading mechanism of DCGI. When set to `true`, chttpd loads all DCGI
libraries ahead of time, and keep them alive all the time; when `false`, chttpd load a DCGI
library before handling one request, and unloads that library after finishing the request.

`case-ignore` controls whether the router cares about letter cases.

The following 4 lines are routes. A route has the following format:
```
HTTP-METHOD request-path HANDLER-TYPE handler-path
```

By this time, `STATIC` handler (for serving static files), `DCGI` handler (for serving dynamic
contents) and `INTERN` handler (for sending internal error pages) are supported. For more
informations about these handlers please refer to the following sections. 

The if threre's an exclaimation mark (`!`) at the commence of the `request-path`, then this route
is expected to be matched exactly. Otherwise it will be matched in some wildcard way. For
example, `/` will match `/index`, `/login` or so.

Note that if one of the `handler-path`s is incorrect, `chttpd` does not always immediately
figure out your mistake, but may give you a `500` when that route gets used.

PL2 is another toy of mine. See [PL2 infrared missile](https://github.com/PL2-Lang/PL2)

## 🛠️ Sending internal pages
By using `INTERN` handler you can send an error page to client. By this time, HTTP errors
`403`, `404` and `500` are supported.

## 📂 Serving static files
By using `STATIC` handler you can serve static files. Unfortunately, by this time `chttpd` is not
capable of handling directory structures (since it uses accurate path matching), so user must
explicitly appoint every single file path.

`chttpd` supports very limited mime guessing. See `src/static.c` for more information.

## 🔄 Serving dynamic contents by DCGI
`DCGI` (Dynamic Common Gateway Interface) is a interface exploiting dynamic library utilities. To
use `DCGI`, you need to:
  - Write a C module, define a `dcgi_main` function, write handing mechanism within that function.
  - Compile the C module as shared object (`libxxx.so`)
  - Add route to that library

A `dcgi_main` function looks like:

```c
int dcgi_main(/* input */  int method, /* 1 */
              /* input */  const char *queryPath,      /* 2 */
              /* input */  const StringPair headers[], /* 3 */
              /* input */  const StringPair params[],  /* 4 */
              /* input */  const char *body,           /* 5 */
              /* output */ StringPair **headerDest,    /* 6 */
              /* output */ char **dataDest,            /* 7 */
              /* output */ char **errDest) {           /* 8 */
  /* your handling code */
  return your_return_code;                /* 9 */
}
```

Explainations
  1. `method`: The HTTP method of HTTP request, 0 for `GET` and 1 for `POST`.
  2. `queryPath`: The path part of HTTP request, not including query parameters. 
     - example:
       - `/index.html`
       - `/robots.txt`
       - `/api/login`
  3. `headers`: Array of header pairs. the `first` part of `StringPair` is the header name, and 
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
  4. `params`: Similar to `headers`, but stores query parameters.
     - example:
       ```
       {
         (StringPair) {"blogId", "114514"},
         (StringPair) {"reply", "1919810"},
         (StringPair) {NULL, NULL}
       }
       ```
  5. `body`: Null terminated string, presents if `Content-Length` is not zero, `NULL` otherwise.
  6. `headerDest`: Used for `dcgi_main` to output headers. Keep it untouched if no output header.
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
  7. `dataDest`: Used for `dcgi_main` to output response body. Keep it untouched if no response
     body. Use null-terminated string, and make sure it's on the heap.
     - example:
       ```c
       *dataDest = copyString("Excuse you!");
       ```
  8. `errDest`: Used for `dcgi_main` to output error information. Keep it untouched if no error.
  9. Return value: Return `200` to indicate successful response, any othre value to indicate
     failed response. If returned code is not `5xx`, the response body will be delivered as-is;
     if so, chttpd will send a internal page containing the response.

