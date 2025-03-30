/*! \file
 * This file defines all socket connection related type definitions.
 */

#ifndef OPGP_SOCKET_CONNECTION_PLUGIN_H
#define OPGP_SOCKET_CONNECTION_PLUGIN_H

#include <globalplatform/library.h>


#define MAX_APDU_LEN 4096

/**
 * Socket specific card information. Used in OPGP_CARD_INFO.librarySpecific.
 */
typedef struct {
    int sock_fd; //!< File descriptor for the connected Unix domain socket.
} SOCKET_CARD_INFO_SPECIFIC;

/**
 * \brief Stringifies an error code.
 */
OPGP_NO_API
OPGP_STRING OPGP_PL_stringify_error(DWORD errorCode);

#endif
