#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "msg_solar.h"
#include "tinux.h"

#define kProgramName (0)
#define kSerialDevice (1)
#define kLogPath (2)
#define kBufferSize (1024)

static msg_name_t msgNames[] = MSG_NAMES;
static const int msgCount = (sizeof(msgNames) / sizeof(msg_name_t));
static volatile int keepRunning = 1;

static void intHandler(int dummy) { keepRunning = 0; }

char *ltoa(char *str, long unsigned int value) {
  const int bufsize = 64;
  char buf[bufsize];
  char *dest = buf + bufsize;
  while (value) {
    *--dest = '0' + (value % 10);
    value /= 10;
  }
  memcpy(str, dest, buf + bufsize - dest);
  return str + (buf + bufsize - dest);
}

void frameToJson(tinframe_t *frame, long int time, char *str) {
  *str++ = '{';
  int found = 0;
  int index = 0;
  while (index < msgCount) {
    int value;
    int format = msg_unpack((msg_data_t *)&frame->data[MSG_ID_OFFSET],
                            msgNames[index].pack, &value);
    if (format != MSG_NULL) {
      if (found++ == 0) {
        *str++ = '"';
        *str++ = 't';
        *str++ = '"';
        *str++ = ':';
        str = ltoa(str, time);
      } else {
      }
      *str++ = ',';
      char valueBuffer[kBufferSize] = {0};
      msg_format((msg_data_t *)&frame->data[MSG_ID_OFFSET],
                 msgNames[index].pack, valueBuffer);
      *str++ = '"';
      strcpy(str, msgNames[index].name);
      while (*str)
        str++;
      *str++ = '"';
      *str++ = ':';
      strcpy(str, valueBuffer);
      while (*str)
        str++;
    }
    index++;
  }
  *str++ = '}';
  *str++ = '\0';
}

int putLog(char *port, char *path) {
  log_t logger;
  if (tinux_open(port) == -1) {
    return EXIT_FAILURE;
  }
  log_begin(&logger, path, sizeof(tinframe_t));
  while (keepRunning) {
    tinframe_t rxFrame;
    int result = tinux_read(&rxFrame);
    if (result == tinux_kOK) {
      long int t = log_commit(&logger, &rxFrame);
      // decode recieved data and send to stdout
      char str[kBufferSize] = {0};
      frameToJson(&rxFrame, t, str);
      fprintf(stdout, "%s\n", str);
    } else if (result == tinux_kReadCRCError) {
      fprintf(stdout, "crc error\n");
    } else if (result == tinux_kReadOverunError) {
      fprintf(stdout, "overun error\n");
    }
  }
  // teardown port and close log
  tinux_close();
  log_end(&logger);
  return EXIT_SUCCESS;
}

int getLog(char *path, time_t startTime, time_t endTime) {
  log_t logger;
  log_begin(&logger, path, sizeof(tinframe_t));
  struct timeval tv = {0};
  tv.tv_sec = startTime;
  while (tv.tv_sec < endTime) {
    unsigned char data[sizeof(tinframe_t)];
    int bytesRead = log_read(&logger, &tv, data);
    if (bytesRead) {

    } else {
      // all log data has been read
    }
  }
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stdout, "Usage: %s <serial device> <log path>\n",
            argv[kProgramName]);
    return EXIT_FAILURE;
  }

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);

  int err = putLog(argv[kSerialDevice], argv[kLogPath]);

  fprintf(stdout, "\nExit %s\n", argv[0]);
  return err;
}
