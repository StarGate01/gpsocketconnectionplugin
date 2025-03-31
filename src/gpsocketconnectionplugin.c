/* Copyright (c) 2025, Christoph Honal  
 * Copyright (c) 2009, Karsten Ohme
 *  This file is part of GlobalPlatform.
 *
 *  GlobalPlatform is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GlobalPlatform is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with GlobalPlatform.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <errno.h>

#include <globalplatform/debug.h>
#include <globalplatform/error.h>
#include <globalplatform/errorcodes.h>
#include <globalplatform/unicode.h>
#include <globalplatform/stringify.h>
#include <globalplatform/connectionplugin.h>

#include "gpsocketconnectionplugin.h"
#include "util.h"


#define CHECK_CARD_INFO_INITIALIZATION(cardInfo, status) \
    if (cardInfo.librarySpecific == NULL) { \
        OPGP_ERROR_CREATE_ERROR(status, OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED, OPGP_PL_stringify_error(OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED)); \
        goto end; \
    }

#define GET_SOCKET_CARD_INFO_SPECIFIC(cardInfo) ((SOCKET_CARD_INFO_SPECIFIC *)(cardInfo.librarySpecific))

#define HANDLE_STATUS(status, result) if (result != SCARD_S_SUCCESS) {\
	OPGP_ERROR_CREATE_ERROR(status,result,OPGP_PL_stringify_error((DWORD)result));\
	}\
	else {\
	OPGP_ERROR_CREATE_NO_ERROR(status);\
	}


OPGP_ERROR_STATUS OPGP_PL_establish_context(OPGP_CARD_CONTEXT *cardContext) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_ERROR_CREATE_NO_ERROR(status);
    return status;
}

OPGP_ERROR_STATUS OPGP_PL_release_context(OPGP_CARD_CONTEXT *cardContext) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_ERROR_CREATE_NO_ERROR(status);
    return status;
}

OPGP_ERROR_STATUS OPGP_PL_list_readers(OPGP_CARD_CONTEXT cardContext, OPGP_STRING readerNames, PDWORD readerNamesLength) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_LOG_START(_T("OPGP_PL_list_readers"));
    const TCHAR *fakeName = _T("Unix Socket Reader");
    DWORD size = (_tcslen(fakeName) + 2) * sizeof(TCHAR); // includes two null terminators
    if (readerNames == NULL) {
        *readerNamesLength = size;
    } else {
        _tcscpy(readerNames, fakeName);
        readerNames[_tcslen(fakeName) + 1] = _T('\0'); // add double null terminator
        *readerNamesLength = size;
    }
    OPGP_ERROR_CREATE_NO_ERROR(status);
    OPGP_LOG_END(_T("OPGP_PL_list_readers"), status);
    return status;
}

OPGP_ERROR_STATUS OPGP_PL_card_connect(OPGP_CARD_CONTEXT cardContext, OPGP_CSTRING socketFd, OPGP_CARD_INFO *cardInfo, DWORD protocol) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_LOG_START(_T("OPGP_PL_card_connect"));
    
    int fd = atoi(socketFd);
    if (fd <= 0) {
        OPGP_ERROR_CREATE_ERROR(status, EINVAL, _T("Invalid file descriptor input"));
        goto end;
    }

    struct sockaddr_un addr;
    socklen_t addr_len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr*)&addr, &addr_len) < 0) {
        OPGP_ERROR_CREATE_ERROR(status, errno, _T("Failed to get socket name"));
        close(fd);
        goto end;
    }

    int sock_type = 0;
    socklen_t opt_len = sizeof(sock_type);
    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &sock_type, &opt_len) < 0) {
        OPGP_ERROR_CREATE_ERROR(status, errno, _T("Failed to get socket type"));
        close(fd);
        goto end;
    }

    if (sock_type != SOCK_SEQPACKET || addr.sun_family != AF_UNIX) {
        OPGP_ERROR_CREATE_ERROR(status, EPROTOTYPE, _T("Socket must be AF_UNIX and SOCK_SEQPACKET"));
        close(fd);
        goto end;
    }

    SOCKET_CARD_INFO_SPECIFIC* info = malloc(sizeof(SOCKET_CARD_INFO_SPECIFIC));
    memset(info, 0, sizeof(SOCKET_CARD_INFO_SPECIFIC));
    info->sock_fd = fd;
    cardInfo->librarySpecific = info;
    cardInfo->logicalChannel = 0;

    ssize_t atrLen = recv(fd, cardInfo->ATR, sizeof(cardInfo->ATR), 0);
    if (atrLen > 0) {
        cardInfo->ATRLength = (DWORD)atrLen;
    } else {
        cardInfo->ATRLength = 0;
    }

    OPGP_ERROR_CREATE_NO_ERROR(status);

end:
    OPGP_LOG_END(_T("OPGP_PL_card_connect"), status);
    return status;
}

OPGP_ERROR_STATUS OPGP_PL_card_disconnect(OPGP_CARD_CONTEXT cardContext, OPGP_CARD_INFO *cardInfo) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_LOG_START(_T("OPGP_PL_card_disconnect"));
    CHECK_CARD_INFO_INITIALIZATION((*cardInfo), status)
    free(cardInfo->librarySpecific);
    cardInfo->librarySpecific = NULL;
    cardInfo->ATRLength = 0;
    OPGP_ERROR_CREATE_NO_ERROR(status);
end:
    OPGP_LOG_END(_T("OPGP_PL_card_disconnect"), status);
    return status;
}

OPGP_ERROR_STATUS OPGP_PL_send_APDU(OPGP_CARD_CONTEXT cardContext, OPGP_CARD_INFO cardInfo,
        PBYTE capdu, DWORD capduLength, PBYTE rapdu, PDWORD rapduLength) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_LOG_START(_T("OPGP_PL_send_APDU"));
    CHECK_CARD_INFO_INITIALIZATION(cardInfo, status)

    int sock = GET_SOCKET_CARD_INFO_SPECIFIC(cardInfo)->sock_fd;
    DWORD offset = 0, tempDataLength = 0;
    BYTE caseAPDU, lc, le, la;
    DWORD maxLen = *rapduLength;

    PBYTE responseData = (PBYTE)malloc(maxLen);
    if (!responseData) {
        HANDLE_STATUS(status, ENOMEM);
        goto end;
    }

    // Determine APDU case type (1, 2, 3, 4) and extract Lc/Le
    if (parse_apdu_case(capdu, capduLength, &caseAPDU, &lc, &le)) {
        HANDLE_STATUS(status, OPGP_ERROR_UNRECOGNIZED_APDU_COMMAND);
        goto end;
    }

    // For Case 4, remove the trailing Le byte before first send
    if (caseAPDU == 4) capduLength--;

    // Initial APDU send via socket
    if (send(sock, capdu, capduLength, 0) != (ssize_t)capduLength) {
        OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
        goto end;
    }

    // Initial receive from socket
    ssize_t rlen = recv(sock, responseData, maxLen, 0);
    if (rlen < 0) {
        OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
        goto end;
    }
    DWORD responseDataLength = (DWORD)rlen;
    offset += responseDataLength - 2;

    // Handle APDU chaining based on response status (0x61, 0x6C, 0x9F)
    switch (caseAPDU) {
        case 2:
        case 4:
            while (responseData[offset] == 0x61 || responseData[offset] == 0x6C || responseData[offset] == 0x9F) {

                // If card returns SW1=0x6C, it rejected our Le and tells us the correct one (SW2)
                if (responseData[offset] == 0x6C) {
                    la = responseData[offset + 1];
                    capdu[capduLength - 1] = la; // Adjust Le to correct value
                } else {
                    // Card returns SW1=0x61 or 0x9F — more data available
                    la = responseData[offset + 1];

                    // Build GET RESPONSE command (CLA=00 INS=C0 P1=00 P2=00 Le=la)
                    capdu[0] = 0x00;
                    capdu[1] = 0xC0;
                    capdu[2] = 0x00;
                    capdu[3] = 0x00;
                    capdu[4] = la;
                    capduLength = 5;
                }

                le = la;

                // Send the follow-up APDU (either corrected command or GET RESPONSE)
                if (send(sock, capdu, capduLength, 0) != (ssize_t)capduLength) {
                    OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
                    goto end;
                }

                rlen = recv(sock, responseData + offset, maxLen - offset, 0);
                if (rlen < 0) {
                    OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
                    goto end;
                }

                // If La > Le, only keep Le bytes of data and copy back the SW1SW2
                if (convert_byte(le) < convert_byte(la)) {
                    memmove(responseData + offset + convert_byte(le),
                            responseData + offset + rlen - 2,
                            2); // copy SW1SW2
                    offset += convert_byte(le);
                    break;
                }

                offset += (DWORD)rlen - 2;

                // Special fallback for broken ISO cards: they respond with 0x6E00 or 0x6D00 to GET RESPONSE on 0x9000
                if (caseAPDU == 4 && get_short(responseData + offset, 2) == 0x6E00) {
                    memcpy(responseData, rapdu, tempDataLength);
                    responseDataLength = tempDataLength;
                    break;
                }
            }
            break;
    }

    // Fallback for case 3 acting like case 4: final SW1 = 0x61 indicates trailing response
    if (responseData[offset] == 0x61) {
        la = responseData[offset + 1];

        // Build GET RESPONSE command again
        capdu[0] = 0x00;
        capdu[1] = 0xC0;
        capdu[2] = 0x00;
        capdu[3] = 0x00;
        capdu[4] = la;
        capduLength = 5;
        le = la;

        // Send GET RESPONSE
        if (send(sock, capdu, capduLength, 0) != (ssize_t)capduLength) {
            OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
            goto end;
        }

        rlen = recv(sock, responseData + offset, maxLen - offset, 0);
        if (rlen < 0) {
            OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
            goto end;
        }

        offset += (DWORD)rlen - 2;
    }

    // Copy accumulated response to output buffer
    memcpy(rapdu, responseData, offset + 2);
    *rapduLength = offset + 2;

    // Extract status word SW1SW2 and map to error code
    DWORD sw = get_short(rapdu, *rapduLength - 2);
    OPGP_ERROR_CREATE_NO_ERROR_WITH_CODE(status, OPGP_ISO7816_ERROR_PREFIX | sw, OPGP_stringify_error(OPGP_ISO7816_ERROR_PREFIX | sw));

end:
    if (responseData) free(responseData);
    OPGP_LOG_END(_T("OPGP_PL_send_APDU"), status);
    return status;
}

OPGP_STRING OPGP_PL_stringify_error(DWORD errorCode) {
    if (errorCode == OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED) {
        return (OPGP_STRING)_T("Socket plugin is not initialized. A card connection must be created first.");
    }
    return OPGP_stringify_error(errorCode);
}
