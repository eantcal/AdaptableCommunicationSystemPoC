//
// This file is part of tSIPd
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#pragma once

#define SIP_SERVER_CFGFNAME "acsgw.cfg"

#define SIP_SERVER_PORT 15060
#define SIP_SERVER_NAME "acsgw"
#define SIP_SERVER_MAJ_V 1
#define SIP_SERVER_MIN_V 0
#define SIP_SERVER_TX_BUF_SIZE 0x100000
#define SIP_SERVER_BACKLOG SOMAXCONN
#define SIP_CONN_TIMEOUT 120 //secs
#define SIP_PROTO "SIP/2.0"
