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
