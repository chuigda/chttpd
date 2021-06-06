#include "static.h"

#include <config.h>
#include <errno.h>
#include <string.h>
#include "file_util.h"
#include "util.h"

static const char *mimeGuess(const char *filePath);

void handleStatic(const char *filePath,
                  FILE *fp,
                  Error *error) {
  FILE *fpFile = fopen(filePath, "r");
  if (fpFile == NULL) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot open file: %s",
                 filePath);
    return;
  }

  ssize_t fileSize = readAll(fpFile, NULL, 0);
  if (fileSize < 0) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot get size of file: %s",
                 filePath);
    goto close_fp_ret;
  }

  char *buffer = (char*)malloc(fileSize);
  if (buffer == NULL) {
    QUICK_ERROR2(error, 500, "handleStatic: cannot allocate %zi bytes",
                 fileSize);
    goto close_fp_ret;
  }

  ssize_t bytesRead = readAll(fpFile, buffer, fileSize);
  if (bytesRead != fileSize) {
    QUICK_ERROR2(error, 500,
                 "handleStatic: cannot read %zi bytes from %s",
                 fileSize, filePath);
    goto free_buf_ret;
  }

  fprintf(fp,
          "HTTP/1.1 200 OK\r\n"
          "Server: %s\r\n"
          "Connection: close\r\n"
          "Content-Encoding: identity\r\n"
          "Content-Type: %s\r\n"
          "Content-Length: %zi\r\n\r\n",
          CHTTPD_SERVER_NAME, mimeGuess(filePath), fileSize);
  if (errno != 0) {
    LOG_WARN("failed to write response header: %d", errno);
    goto free_buf_ret;
  }

  ssize_t bytesWrite = fwrite(buffer, 1, fileSize, fp);
  if (bytesWrite != fileSize) {
    LOG_WARN("failed to respond %zi bytes: %d", fileSize, errno);
  }

free_buf_ret:
  free(buffer);

close_fp_ret:
  fclose(fpFile);
}

static const char *mimeGuess(const char *filePath) {
  const char *postfix = strrchr(filePath, '.');
  if (postfix == NULL) {
    return "application/octet-stream";
  }

  if (strcmp_icase(postfix, ".html")) {
    return "text/html";
  } else if (strcmp_icase(postfix, ".js")) {
    return "text/javascript";
  } else if (strcmp_icase(postfix, ".css")) {
    return "text/css";
  } else if (strcmp_icase(postfix, ".json")) {
    return "application/json";
  } else if (strcmp_icase(postfix, ".xml")) {
    return "application/xml";
  } else if (strcmp_icase(postfix, ".txt")) {
    return "text/plain";
  }
  
  return "application/octet-stream";
}
