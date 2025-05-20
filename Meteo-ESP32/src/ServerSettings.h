#pragma once

// Настройки FTP сервера
// const char* FTP_SERVER = "66.220.9.50";
// const char* FTP_USER = "mrkamalov001";
// const char* FTP_PASS = "PSc2q@UVSn5rayc";
extern const char* FTP_SERVER;
extern const char* FTP_USER;
extern const char* FTP_PASS;
extern const char* HTTP_SERVER;

extern const char* FILE_NAME;  // Загружаемый файл
extern const char* CONFIG_FILE;
// Расположение файла прошивки в SPIFFS
extern const char* LOCAL_FILE;
extern const int FW_VERSION_ADDR;
extern const int FW_VERSION_SIZE;
extern const char* VERSION_FILE;