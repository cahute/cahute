/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
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
 * Emit an SCSI request on a link transport, and receive data.
 *
 * NOTE: ``*statusp`` may be NULL.
 *
 * @param link Link with the transport to SCSI request from.
 * @param command Command to emit to the device, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the device.
 * @param buf Buffer to fill with the command's result.
 * @param buf_size Buffer capacity to not go past.
 * @param statusp Pointer to the SCSI status to set to the received one.
 */
CAHUTE_INTERNAL(int)
cahute_scsi_request_from_link_transport(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
) {
    int err, status = 0;

    if (link->transport != CAHUTE_LINK_TRANSPORT_UMS)
        CAHUTE_RETURN_IMPL(
            link->context,
            "Cannot SCSI request from a non-UMS link."
        );

    err = (*link->transport_scsi_request_from_func)(
        link->context,
        link->transport_cookie,
        command,
        command_size,
        buf,
        buf_size,
        &status
    );
    if (statusp)
        *statusp = status;

    switch (err) {
    case CAHUTE_ERROR_GONE:
        link->flags |= CAHUTE_LINK_FLAG_GONE;
        /* FALLTHRU */

    default:
        return err;
    }
}
