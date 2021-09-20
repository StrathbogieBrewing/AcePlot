#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "AceBMS.h"
#include "AceDump.h"
#include "AceGrid.h"
#include "AceMPPT.h"
#include "AcePlot.h"
#include "cgi.h"
#include "log.h"
#include "tinux.h"
#include "udpBroadcast.h"

#define kProgramName (0)
#define kSerialDevice (1)
#define kLogPath (2)
#define kBufferSize (1024)

static sig_name_t sigNames[] = {ACEBMS_NAMES, ACELOG_NAMES, ACEMPPT_NAMES,
                                ACEDUMP_NAMES, ACEGRID_NAMES};
static const int sigCount = (sizeof(sigNames) / sizeof(sig_name_t));
static volatile int keepRunning = 1;

static void intHandler(int dummy) { keepRunning = 0; }

char *ltoa(char *str, uint64_t value) {
  const int bufsize = 64;
  char buf[bufsize];
  char *dest = buf + bufsize;
  while (value) {
    *--dest = '0' + (value % 10LL);
    value /= 10LL;
  }
  memcpy(str, dest, buf + bufsize - dest);
  return str + (buf + bufsize - dest);
}

int frameToJson(tinframe_t *frame, uint64_t time, char *str) {
  *str++ = '{';
  int found = 0;
  int index = 0;
  while (index < sigCount) {
    int16_t value;
    fmt_t format =
        sig_decode((msg_t *)frame->data, sigNames[index].sig, &value);
    if (format != FMT_NULL) {
      if (found++ == 0) {
        *str++ = '"';
        *str++ = 't';
        *str++ = '"';
        *str++ = ':';
        str = ltoa(str, time);
      }
      *str++ = ',';
      char valueBuffer[FMT_MAXSTRLEN] = {0};
      sig_toString((msg_t *)frame->data, sigNames[index].sig, valueBuffer);
      *str++ = '"';
      strcpy(str, sigNames[index].name);
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
  return found;
}

void hexDump(char *buffer, int size) {
  int i = 0;
  fprintf(stdout, "HEX : ");
  while (i < size) {
    fprintf(stdout, "%2.2x ", (unsigned char)buffer[i++]);
  }
  fprintf(stdout, "\n");
}

bool logData(log_t *logger, tinframe_t *frame) {
  // remove unnecessary / excessive log data from bms
  int16_t value;
  fmt_t format = sig_decode((msg_t *)frame->data, ACEBMS_RQST, &value);
  if ((format != FMT_NULL) && (value & 0x0003)) {
    return false;
  }
  // hexDump(frame->data, tinframe_kDataSize);
  // commit to log
  uint64_t t = log_commit(logger, frame);
  // decode recieved data and send to stdout
  char str[kBufferSize] = {0};
  int found = frameToJson(frame, t, str);
  if (found) {
    fprintf(stdout, "%s\n", str);
  } else {
    hexDump((char *)frame, sizeof(tinframe_t));
  }
  return true;
}

void logError(log_t *logger, unsigned char error) {
  tinframe_t frame;
  msg_t *msg = (msg_t *)frame.data;
  sig_encode(msg, ACELOG_ERROR, error);
  logData(logger, &frame);
}

int putLog(const char *path, const char *port) {
  log_t logger;
  if (tinux_open(port) == -1) {
    return EXIT_FAILURE;
  }
  udpBroadcast_open(31415);
  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
  log_begin(&logger, path, sizeof(tinframe_t));
  while (keepRunning) {
    tinframe_t rxFrame;
    int result = tinux_read(&rxFrame);
    if (result == tinux_kOK) {
      logData(&logger, &rxFrame);
      udpBroadcast_send((unsigned char *)&rxFrame, tinframe_kFrameSize);
    } else if (result == tinux_kReadCRCError) {
      fprintf(stdout, "CRC Error\n");
      logError(&logger, result);
    } else if (result == tinux_kReadOverunError) {
      fprintf(stdout, "Overun Error\n");
      logError(&logger, result);
    }

    // unsigned char data[udp_kBufferSize];
    // int bytesRead;
    // if((bytesRead = udp_read(data, udp_kBufferSize)) > 0){
    //   hexDump(data, bytesRead);
    // }
  }
  // teardown port and close log
  udpBroadcast_close();
  tinux_close();
  log_end(&logger);
  return EXIT_SUCCESS;
}

int getLog(const char *path, time_t startTime, time_t endTime) {
  log_t logger;
  log_begin(&logger, path, sizeof(tinframe_t));
  struct timespec ts = {0};
  ts.tv_sec = startTime;
  fprintf(stdout, "Content-Type: application/json\r\n\r\n");
  fprintf(stdout, "{\"TimeSeries\":[");
  int lines = 0;
  while (ts.tv_sec <= endTime) {
    tinframe_t data;
    int bytesRead = log_read(&logger, &ts, &data);
    if (bytesRead) {
      char str[kBufferSize] = {0};
      frameToJson(&data, log_millis(&ts), str);
      if (lines++) {
        fprintf(stdout, ",%s", str);
      } else {
        fprintf(stdout, "%s", str);
      }
    }
  }
  fprintf(stdout, "]}");
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

  int returnValue = EXIT_SUCCESS;
  if (loggerSerial != NULL) {
    if (daemonize) {
      daemon(1, 0);
    }
    returnValue = putLog(loggerPath, loggerSerial);
  } else {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    time_t duration = (time_t)cgi_getLongInt("duration");
    if (duration == 0)
      duration = 3600L;
    time_t startTime = (time_t)cgi_getLongInt("start");
    if (startTime == 0)
      startTime = now.tv_sec - duration;
    time_t endTime = startTime + duration;
    returnValue = getLog(loggerPath, startTime, endTime);
  }
  return returnValue;
}
