/* ****************************************************************************
 * Copyright (C) 2024-2025 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#ifndef LINK_CAS300_INTERNALS_H
#define LINK_CAS300_INTERNALS_H 1
#include "../internals.h"

#define PACKET_TYPE_COMMAND 0x01
#define PACKET_TYPE_DATA    0x02
#define PACKET_TYPE_ACK     0x06
#define PACKET_TYPE_ORDER   0x15
#define PACKET_TYPE_TERM    0x18

#define TIMEOUT_ACK             1000
#define TIMEOUT_PACKET_CONTENTS 500

CAHUTE_INTERNAL(int)
cahute_cas300_send_command(
    cahute_link *link,
    unsigned int command,
    cahute_u8 const *payload,
    size_t payload_size
);

CAHUTE_INTERNAL(int)
cahute_cas300_send_data_packet(
    cahute_link *link,
    unsigned int command,
    cahute_u8 const *payload,
    size_t payload_size
);

#endif /* LINK_CAS300_INTERNALS_H */
