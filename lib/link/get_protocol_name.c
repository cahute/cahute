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
 * Get the name of a link protocol.
 *
 * @param protocol Protocol identifier, as a constant.
 * @return Textual name of the protocol.
 */
CAHUTE_INTERNAL(char const *) cahute_get_protocol_name(int protocol) {
    switch (protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_NONE:
        return "Generic (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS:
        return "CASIOLINK (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS40:
        return "CAS40 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS50:
        return "CAS50 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS100:
        return "CAS100 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CAS300:
        return "CAS300 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        return "Protocol 7.00 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP:
        return "Protocol 7.00 Screenstreaming (serial)";
    case CAHUTE_LINK_PROTOCOL_USB_NONE:
        return "Generic (USB)";
    case CAHUTE_LINK_PROTOCOL_USB_CAS300:
        return "CAS300 (USB)";
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return "Protocol 7.00 (USB)";
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP:
        return "Protocol 7.00 Screenstreaming (USB)";
    case CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE:
        return "USB Mass Storage";
    default:
        return "(unknown)";
    }
}
