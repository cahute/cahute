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

#include "internals.h"

/**
 * Receive data.
 *
 * @param link Link to the device.
 * @param datap Data to allocate.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_INTERNAL(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
) {
    cahute_u8 buf[50];
    int byte, err;

    do {
        err = cahute_casiolink_receive_first_byte(link, &byte, timeout);
        if (err)
            return err;

        if (byte == 0x01 || byte == 0x02 || byte == 0x18) {
            err = cahute_cas300_receive_data(link, datap, byte, timeout);
            goto data_decoded;
        }

        if (byte != 0x3A) {
            msg(link->context, ll_error, "Unknown packet type 0x%02X.", byte);
            return CAHUTE_ERROR_UNKNOWN;
        }

        buf[0] = byte;
        err = cahute_receive_on_link_transport(
            link,
            &buf[1],
            39,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS,
            CASIOLINK_TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;

        switch (cahute_casiolink_determine_header_variant(buf)) {
        case VARIANT_CAS100:
            err = cahute_cas100_receive_data(link, datap, buf, 0);
            break;

        case VARIANT_CAS50:
            err = cahute_receive_on_link_transport(
                link,
                &buf[40],
                10,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS,
                CASIOLINK_TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                return err;

            err = cahute_cas50_receive_data(link, datap, buf, 0);
            break;

        default:
            err = cahute_cas40_receive_data(link, datap, buf, 0);
        }

data_decoded:
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_IMPL)
            return err;
    } while (1);

    return CAHUTE_OK;
}
