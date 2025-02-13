#include "ExtendedHttpClient.h"

bool ExtendedHttpClient::responseBodyStream(Stream &output) {
    int bodyLength = contentLength();
    String response;
    int writedBytes = 0;

    // if (bodyLength > 0)
    // {
    //     // try to reserve bodyLength bytes
    //     if (response.reserve(bodyLength) == 0) {
    //         // String reserve failed
    //         return String((const char*)NULL);
    //     }
    // }

    // keep on timedRead'ing, until:
    //  - we have a content length: body length equals consumed or no bytes
    //                              available
    //  - no content length:        no bytes are available
    while (iBodyLengthConsumed != bodyLength)
    {
        int c = timedRead();

        if (c == -1) {
            // read timed out, done
            break;
        }

        output.write(c);
        writedBytes++;
        // if (!response.concat((char)c)) {
        //     // adding char failed
        //     return String((const char*)NULL);
        // }
    }

    if (bodyLength > 0 && bodyLength!=writedBytes){    //(unsigned int)bodyLength != response.length()) {
        // failure, we did not read in response content length bytes
        return false;//String((const char*)NULL);
    }

    return true;//response;
}
