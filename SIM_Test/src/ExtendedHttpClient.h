#ifndef EXTENDED_HTTP_CLIENT_H
#define EXTENDED_HTTP_CLIENT_H

#include <ArduinoHttpClient.h>
#include <Stream.h>

class ExtendedHttpClient : public HttpClient {
public:
    using HttpClient::HttpClient;  // Наследуем конструкторы

    // Новый метод для сохранения данных в поток (SPIFFS или буфер)
    bool responseBodyStream(Stream &output);
};

#endif  // EXTENDED_HTTP_CLIENT_H
