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

## ⚙️ Configure
Configuration file of `chttpd` uses PL2BK DSL. A sample configuration looks like:
```
listen-address 127.0.0.1
listen-port 3080
max-pending 5
cgi-timeout 3

GET /           STATIC ./src/index.html
GET /index.html STATIC ./src/index.html
GET /robots.txt STATIC ./src/robots.txt
GET /api/login  DCGI   ./dcgi/liblogin.so
```

The first four lines (`listen-address`, `listen-port`, `max-pending`, `cgi-timeout`) configures
how the HTTP server works. When not appointed, chttpd will use default value for them
(see `src/config.c`). Among these attributes, I don't really know what `max-pending` means
(this is a parameter of POSIX socket listening).

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
> Document incomplete by this time

### 🌵 By using CGI
> Not implemented by this time

### 🥈 By using AgNO3
> Not implemented by this time
