#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>

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

int putLog(const char *path, const char *port) {
  log_t logger;
  if (tinux_open(port) == -1) {
    return EXIT_FAILURE;
  }
  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
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

int getLog(const char *path, time_t startTime, time_t endTime) {
  log_t logger;
  log_begin(&logger, path, sizeof(tinframe_t));
  struct timespec ts = {0};
  ts.tv_sec = startTime;
  fprintf(stdout, "{[\n");
  while (ts.tv_sec <= endTime){
    tinframe_t data;
    int bytesRead = log_read(&logger, &ts, &data);
    // fprintf(stdout, "Read bytes %d, time %ld\n", bytesRead, ts.tv_sec);
    if (bytesRead) {
      char str[kBufferSize] = {0};
      frameToJson(&data, log_millis(&ts), str);
      fprintf(stdout, "%s,\n", str);
    }
  }
  fprintf(stdout, "]}\n");
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  static char *loggerPath = "log";
  static char *loggerSerial = NULL;
  static bool daemonize = false;

  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "p:s:d")) != -1)
    switch (opt) {
    case 'p':
      loggerPath = optarg;
      break;
    case 's':
      loggerSerial = optarg;
      break;
    case 'd':
      daemonize = true;
      break;
    case '?':
      if (optopt == 'p')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      return EXIT_FAILURE;
    default:
      fprintf(stderr, "Invalid option.\n");
      abort();
      return EXIT_FAILURE;
    }

  int returnValue  = EXIT_SUCCESS;
  if(loggerSerial != NULL){
    if (daemonize) {
      daemon(1, 0);
    }
    returnValue = putLog(loggerPath, loggerSerial);
  } else {
    struct timespec now;
    clock_gettime (CLOCK_REALTIME, &now);
    time_t startTime = now.tv_sec - 3600LL;
    time_t endTime = now.tv_sec;
    returnValue = getLog(loggerPath, startTime, endTime);
  }

  fprintf(stdout, "\nExit %s\n", argv[0]);
  return returnValue;
}
