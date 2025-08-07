#ifndef SIM800_UPDATER_H
#define SIM800_UPDATER_H

#include "deviceConfig.h"
#include <Arduino.h>

class Sim800Updater {
public:
    Sim800Updater();

    void updateFirmwareViaGPRS();
    void begin();   

private:    
    uint32_t calculateFileCRC32(const char* filePath);
    bool initSpiffs();
    void performFirmwareUpdate(String& newVersion);
    void sendCommand(const String& command, int delayMs = 500);
    bool waitForResponse(const String& expected, int timeout);
    //bool downloadFirmware();
    bool downloadFileFromFtp(const String& remoteFilename, int partSize, bool isLastPart);
    //bool downloadFileFromFTP(const String& url, const char* path);
    bool fetchConfigFromFTP(String &config);//String &version, uint32_t &remoteCRC, int &partsCount, int &partSize
    String readLocalVersion();
    void saveVersionToEEPROM(const String& version);    
    bool checkForUpdates(String &version, uint32_t &remoteCRC, int& partsCount, int& partSize);
    bool isValidVersionFormat(const String& version);
    bool mergeFirmwareParts(int totalParts, const char *outputFile);  
    bool parseConfigContent(const String& configContent, String& version, uint32_t& totalCrc, int& partCount, int& partSize);

    char FTP_SERVER[32] = {0};
    char FTP_USER[32] = {0};
    char FTP_PASS[32] = {0};
    

    // Внутреннее состояние загрузки
    enum UpdateState {
        IDLE,
        LOAD_CONFIG_INITIAL,
        DOWNLOAD_PART,
        VERIFY_CRC,
        UPDATE_DONE
    };

    UpdateState updateState = IDLE;

    // Текущая информация о прошивке
    String fwVersion = "";
    uint32_t fwCRC = 0;
    int fwParts = 0;
    int fwPartSize = 0;
    int currentPart = 0;
    #define MAX_PARTS 40 // Максимальное количество частей    
    uint32_t partCRCs[MAX_PARTS] = {0}; // Массив для хранения CRC частей прошивки
    int downloadAttempts = 0; // Счетчик попыток загрузки
};

#endif
