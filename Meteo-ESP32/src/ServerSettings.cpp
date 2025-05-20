#include "ServerSettings.h"

// Настройки FTP сервера
const char* FTP_SERVER = "ftpcouch.couchdrop.io";
const char* FTP_USER = "couchuser";
const char* FTP_PASS = "lqKJ4MAUy8g0g06IB8rn";
const char* HTTP_SERVER = "172.30.56.14";
// const char* FTP_USER = "meteoesp";
// const char* FTP_PASS = "mD7t_i@J?J8}";
// const char* FTP_SERVER = "66.220.9.50";
// const char* FTP_USER = "mrkamalov001";
// const char* FTP_PASS = "PSc2q@UVSn5rayc";

const char* FILE_NAME = "firmware.bin";  // Загружаемый файл
const char* CONFIG_FILE = "config.txt";
// Расположение файла прошивки в SPIFFS
const char* LOCAL_FILE = "/firmware.bin";
const int FW_VERSION_ADDR  = 114;
const int FW_VERSION_SIZE = 8;
const char* VERSION_FILE = "version.txt";