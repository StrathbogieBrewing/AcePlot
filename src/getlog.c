#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>

#include "log.h"


// maximum log record string
#define kMaxStrLen (1024)

// maximum log file buffer size
#define kMaxLogFileSize (kMaxStrLen * kMaxStrLen)

// listening port
#define kUDPPort (12345)

// default logging directory is log
static char *loggerDirectory = "log";

// default log retrieval is 5 minute
static char *getDuration = "5";
static char *getSource = NULL;
static char *getList = NULL;
static bool daemonize = false;

static char sourceCGI[kMaxStrLen];
static char durationCGI[kMaxStrLen];
static char *logDirectoryCGI = "/home/johny/log";

#define kQueryValueStringLengeth (256)

typedef struct {
  const char *fieldName;
  char fieldValue[kQueryValueStringLengeth];
} query_t;

void parseQueryString(query_t queryItems[], const char *queryString) {
  int item = 0;
  const char *fieldName;
  while ((fieldName = queryItems[item].fieldName) != NULL) {
    int fieldNameLength = strlen(fieldName);
    char *fieldNameStart = strstr(queryString, fieldName);
    int index = 0;
    if (fieldNameStart) {
      const char *value = fieldNameStart + fieldNameLength;
      if (*value++ == '=') {
        while ((*value != ';') && (*value != '&') && (*value != 0)) {
          queryItems[item].fieldValue[index++] = *value++;
          if (index >= kQueryValueStringLengeth)
            index = 0;
        }
      } else {
        // assign default value of 0
        queryItems[item].fieldValue[index++] = '0';
      }
    }
    queryItems[item++].fieldValue[index] = 0;
  }
}

static volatile int keepRunning = 1;
void intHandler(int dummy) { keepRunning = 0; }

int main(int argc, char *argv[]) {

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);

  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "p:t:s:d")) != -1)
    switch (opt) {
    case 'p':
      loggerDirectory = optarg;
      break;
    case 't':
      getDuration = optarg;
      break;
    case 's':
      getSource = optarg;
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
      abort();
    }

  // check if this is a cgi call with a valid query string
  query_t queryItems[] = {{"source", {0}}, {"duration", {0}}, {"list", {0}}, {NULL, {0}}};
  char *queryString = getenv("QUERY_STRING");
  if (queryString != NULL) {
    loggerDirectory = logDirectoryCGI;
    parseQueryString(queryItems, queryString);
    if (queryItems[0].fieldValue[0]) {
      getSource = queryItems[0].fieldValue;
    }
    if (queryItems[1].fieldValue[0]) {
      getDuration = queryItems[1].fieldValue;
    }
    if (queryItems[2].fieldValue[0]) {
      getList = queryItems[2].fieldValue;
    }
  }

  time_t secondsEnd, secondsStart;

  if(getDuration != NULL){
    unsigned int minutes;
    int params = sscanf(getDuration, "%u", &minutes);
    if (params != 1) {
      fprintf(stderr, "Duration requires a number of minutes\n");
      return EXIT_FAILURE;
    }

    // get time now
    struct timeval tv;
    gettimeofday(&tv, NULL);
    secondsEnd = tv.tv_sec;
    secondsStart = secondsEnd - (60 * minutes);
  }

  if (getSource != NULL) {
    // retieve data from log
    if (queryString != NULL) {
      fprintf(stdout, "Content-Type: application/json\r\n\r\n");
    }
    log_initialise(loggerDirectory);
    log_read(getSource, secondsStart, secondsEnd);
    log_terminate();
    return EXIT_SUCCESS;
  }

  if(getList != NULL){
    if (queryString != NULL) {
      fprintf(stdout, "Content-Type: application/json\r\n\r\n");
    }
    log_initialise(loggerDirectory);
    log_readSourceList(secondsStart, secondsEnd);
    log_terminate();
    return EXIT_SUCCESS;
  }

  if (daemonize) {
    daemon(1, 0);
    if (udp_open(kUDPPort) == -1) {
      fprintf(stderr, "Log UDP Open Error\n");
      return EXIT_FAILURE;
    }
    log_initialise(loggerDirectory);
    while (keepRunning) {
      // process udp packets with blocking read
      char rxBuffer[udp_kBufferSize];
      int recievedBytes = udp_read(rxBuffer);
      if (recievedBytes == tinbus_kFrameSize) {
        log_commit((tinbus_frame_t*)rxBuffer);
      } else {
        // record bad datagram
        fprintf(stderr, "Log UDP Packet Error\n");
      }
    }
    // tear down log and udp port
    log_terminate();
    udp_close();
  }

  return EXIT_SUCCESS;
}

// typedef struct {
//   const char *str;
//   int len;
// } token_t;

// bool parseQueryString(token_t *query, const char *fieldName) {
//   int fieldNameLength = strlen(fieldName);
//   char *fieldNameStart = strstr(query->str, fieldName);
//   if (fieldNameStart) {
//     query->len = 0;
//     const char *value = fieldNameStart + fieldNameLength;
//     if (*value == '=') {
//       query->str = ++value;
//       while ((*value != ';') && (*value != '&') && (*value != 0)) {
//         value++;
//         query->len++;
//       }
//       return true;
//     }
//     if ((*value == ';') || (*value == '&') || (*value == 0)) {
//       query->str = NULL;
//       return true;
//     }
//   }
//   return false;
// }

// struct timeval tv;
// gettimeofday(&tv, NULL);
// suseconds_t ms = tv.tv_usec / 1000L;
// struct tm tm = *gmtime(&tv.tv_sec);

// printf("%s%4.4ld/%2.2ld/%2.2ld/%2.2ld/%2.2ld\n", loggerDirectory,
//        1900L + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour,
//        tm.tm_min);

// char path[256];
// sprintf(path, "%s%4.4ld/%2.2ld/%2.2ld/%2.2ld", loggerDirectory,
//         1900L + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour);
//
// printf("%s\n", path);
// build(path);

// return 0;

// char rxBuffer[udp_kBufferSize] = {0};
//
// if (argc != 2) {
//   fprintf(stdout, "Usage: %s <logpath>\n", argv[0]);
//   return EXIT_FAILURE;
// }
// if (strlen(argv[1]) > LOG_MAX_STRING_LEN - 16) {
//   fprintf(stdout, "Path too long\n");
//   return EXIT_FAILURE;
// }
//
// strncpy(filePath, argv[1], LOG_MAX_STRING_LEN);
//
// return 0;
//
// fileBuffer = (char *)malloc(kMaxLogFileSize);
// if (fileBuffer == NULL) {
//   fprintf(stderr, "Could not allocate required memory\n");
//   return EXIT_FAILURE;
// }
//
// if (udp_open(kUDPPort) == -1) {
//   perror(__FILE__ " Line " LINE);
//   return EXIT_FAILURE;
// }
//
// while (keepRunning) {
//   // process udp packets with blocking read
//   int rc = udp_read(rxBuffer);
//   if (rc > 0) {
//     // remove cr / lf and terminate
//     int len = strlen(rxBuffer);
//     int i = 0;
//     while (i < len) {
//       if (rxBuffer[i] == '\r')
//         rxBuffer[i] = '\0';
//       if (rxBuffer[i] == '\n')
//         rxBuffer[i] = '\0';
//       i++;
//     }
//     putLog(rxBuffer);
//   } else if (rc < 0) {
//     // record bad datagram
//     fprintf(stderr, "Log UDP Packet Error\n");
//   }
// }
//
// // tear down db and udp port
// putLog(NULL);
// udp_close();

// return EXIT_SUCCESS;

// log_commit("ab=123456,d1=10.24,df=987.833,as=1,df=-98.33,ef=987.833");
// log_commit("ef=-987.833");

// bcd_t bcd;
// char buffer[16];
//
// // bcd_parseNumber(bcd, "-12.3452");
// // bcd_printNumber(buffer, bcd);
// char test[] = "sds:dss";
//
// bcd_parseID(bcd, test);
// bcd_printID(buffer, bcd);
//
// printf("%s 0x%2.2X : 0x%2.2X : 0x%2.2X : 0x%2.2X, %s\n", test, bcd[0],
// bcd[1],
//        bcd[2], bcd[3], buffer);
//
// return EXIT_SUCCESS;
