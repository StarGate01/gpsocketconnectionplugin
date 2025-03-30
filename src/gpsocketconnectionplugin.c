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


#define CHECK_CARD_INFO_INITIALIZATION(cardInfo, status) \
    if (cardInfo.librarySpecific == NULL) { \
        OPGP_ERROR_CREATE_ERROR(status, OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED, OPGP_PL_stringify_error(OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED)); \
        goto end; \
    }

#define GET_SOCKET_CARD_INFO_SPECIFIC(cardInfo) ((SOCKET_CARD_INFO_SPECIFIC *)(cardInfo.librarySpecific))

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

OPGP_ERROR_STATUS OPGP_PL_card_connect(OPGP_CARD_CONTEXT cardContext, OPGP_CSTRING socketPath, OPGP_CARD_INFO *cardInfo, DWORD protocol) {
    OPGP_ERROR_STATUS status;
    memset(&status, 0, sizeof(OPGP_ERROR_STATUS));
    OPGP_LOG_START(_T("OPGP_PL_card_connect"));

    int fd = open(socketPath, O_RDWR);
    if (fd < 0) {
        OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
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

    struct iovec iov = { .iov_base = cardInfo->ATR, .iov_len = sizeof(cardInfo->ATR) };
    struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };

    ssize_t atrLen = recvmsg(fd, &msg, MSG_WAITALL);
    if (atrLen > 0 && (msg.msg_flags & MSG_EOR)) {
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
    close(GET_SOCKET_CARD_INFO_SPECIFIC((*cardInfo))->sock_fd);
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

    struct iovec send_iov = { .iov_base = capdu, .iov_len = capduLength };
    struct msghdr send_msg = { .msg_iov = &send_iov, .msg_iovlen = 1 };

    if (sendmsg(sock, &send_msg, MSG_EOR) != (ssize_t)capduLength) {
        OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
        goto end;
    }

    struct iovec recv_iov = { .iov_base = rapdu, .iov_len = *rapduLength };
    struct msghdr recv_msg = { .msg_iov = &recv_iov, .msg_iovlen = 1 };

    ssize_t msg_len = recvmsg(sock, &recv_msg, MSG_WAITALL);
    if (msg_len < 0) {
        OPGP_ERROR_CREATE_ERROR(status, errno, OPGP_stringify_error(errno));
        goto end;
    }

    if (!(recv_msg.msg_flags & MSG_EOR)) {
        OPGP_ERROR_CREATE_ERROR(status, EMSGSIZE, _T("Incomplete RAPDU received (missing MSG_EOR)"));
        goto end;
    }

    *rapduLength = (DWORD)msg_len;
    OPGP_ERROR_CREATE_NO_ERROR(status);

end:
    OPGP_LOG_END(_T("OPGP_PL_send_APDU"), status);
    return status;
}

OPGP_STRING OPGP_PL_stringify_error(DWORD errorCode) {
    if (errorCode == OPGP_PL_ERROR_NO_CARD_INFO_INITIALIZED) {
        return (OPGP_STRING)_T("Socket plugin is not initialized. A card connection must be created first.");
    }
    return OPGP_stringify_error(errorCode);
}
