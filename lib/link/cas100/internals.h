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

#ifndef LINK_CAS100_INTERNALS_H
#define LINK_CAS100_INTERNALS_H 1
#include "../internals.h"

#define PACKET_TYPE_ACK       0x06
#define PACKET_TYPE_CORRUPTED 0x2B
#define PACKET_TYPE_HEADER    0x3A

CAHUTE_INTERNAL(int)
cahute_cas100_determine_data_description(
    cahute_context *context,
    cahute_u8 const *data,
    cahute_casiolink_data_description *desc
);

CAHUTE_INTERNAL(int)
cahute_cas100_decode_data_using_description(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long offset,
    cahute_u8 const *header,
    struct cahute_casiolink_data_description *desc
);

CAHUTE_INTERNAL(int)
cahute_cas100_receive_raw_data(
    cahute_link *link,
    cahute_u8 const *header,
    unsigned long timeout,
    struct cahute_casiolink_data_description *desc
);

CAHUTE_INTERNAL(int) cahute_cas100_send_model_information(cahute_link *link);

#endif /* LINK_CAS100_INTERNALS_H */
