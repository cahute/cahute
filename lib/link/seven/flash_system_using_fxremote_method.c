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
 * Flash a sector using the fxRemote method.
 *
 * This method sends the data using 0x70 commands, by buffers of 0x3FC bytes
 * (size of the buffer with fxRemote), and once the whole buffer is
 * copied at address 0x88030000, request a copy to the real flash location
 * using command 0x71.
 *
 * @param link Link to the device.
 * @param addr Base address of the sector to write.
 * @param data Data to write to the sector.
 * @param size Size of the data to write to the sector.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_flash_sector_using_fxremote_method(
    cahute_link *link,
    unsigned long addr,
    cahute_u8 const *data,
    size_t size
) {
    cahute_u8 buf[0x404];
    unsigned long upload_offset = 0x88030000;
    unsigned long upload_left = size;
    int err;

    /* Send data using the 0x3FC sized buffer. */
    for (; upload_left >= 0x3FC;
         upload_offset += 0x3FC, upload_left -= 0x3FC, data += 0x3FC) {
        buf[0] = (upload_offset >> 24) & 255;
        buf[1] = (upload_offset >> 16) & 255;
        buf[2] = (upload_offset >> 8) & 255;
        buf[3] = upload_offset & 255;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0x03;
        buf[7] = 0xFC;
        memcpy(&buf[8], data, 0x3FC);

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x70,
            buf,
            0x404,
            TIMEOUT_PACKET_TIMEOUT
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Leftover data (% 0x3FC). */
    if (upload_left) {
        buf[0] = (upload_offset >> 24) & 255;
        buf[1] = (upload_offset >> 16) & 255;
        buf[2] = (upload_offset >> 8) & 255;
        buf[3] = upload_offset & 255;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = (upload_left >> 8) & 255;
        buf[7] = upload_left & 255;
        memcpy(&buf[8], data, upload_left);

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x70,
            buf,
            8 + upload_left,
            TIMEOUT_PACKET_TIMEOUT
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Copy data from the buffer to the flash. */
    buf[0] = (addr >> 24) & 255;
    buf[1] = (addr >> 16) & 255;
    buf[2] = (addr >> 8) & 255;
    buf[3] = addr & 255;
    buf[4] = (size >> 24) & 255;
    buf[5] = (size >> 16) & 255;
    buf[6] = (size >> 8) & 255;
    buf[7] = size & 255;
    buf[8] = 0x88;
    buf[9] = 0x03;
    buf[10] = 0x00;
    buf[11] = 0x00;

    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        0x71,
        buf,
        12,
        TIMEOUT_PACKET_TIMEOUT
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    return CAHUTE_OK;
}

/**
 * Flash an image on the calculator.
 *
 * This uses command 0x76 to get information regarding the calculator model
 * from the bootloader, clears all sectors related to the ROM using
 * command 0x72, then copy all sectors from 0xA0020000 to 0xA0280000 excluded,
 * then the initial system sector at 0xA0010000, and send the final
 * 0x78 command.
 *
 * See :ref:`flash-the-calculator-using-fxremote` for more information.
 *
 * @param link Link to the device.
 * @param flags Flags.
 * @param system System to flash on the device.
 * @param system_size Size of the system to flash on the device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_INTERNAL(int)
cahute_seven_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
) {
    cahute_u8 const *initial_sector;
    unsigned long addr, initial_sector_size;
    unsigned long bootloader_size;
    unsigned long max_addr;
    int err;

    /* Use fxRemote-specific command 76 to get special data.
     * While we don't want to exploit it, we want to still do it for
     * the Update.EXE to recognize us as fxRemote. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, 0x76);
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    /* The previous packet sends both an ACK and a data packet. */
    err = cahute_seven_receive(link, TIMEOUT_PACKET_START);
    if (err)
        return err;

    /* Clear all system-related sectors of the flash. */
    if (flags & CAHUTE_FLASH_FLAG_RESET_SMEM)
        max_addr = 0xA0400000;
    else
        max_addr = 0xA0280000;

    for (addr = 0xA0010000; addr < max_addr; addr += 0x10000) {
        cahute_u8 buf[4];

        buf[0] = (addr >> 24) & 255;
        buf[1] = (addr >> 16) & 255;
        buf[2] = (addr >> 8) & 255;
        buf[3] = addr & 255;

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x72,
            buf,
            4,
            TIMEOUT_PACKET_TIMEOUT
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Skip the bootloader section. */
    bootloader_size = system_size >= 0x10000 ? 0x10000 : system_size;
    system += bootloader_size;
    system_size -= bootloader_size;

    /* Write all sectors. */
    initial_sector = system;
    initial_sector_size = system_size > 0x10000 ? 0x10000 : system_size;

    system += initial_sector_size;
    system_size -= initial_sector_size;

    for (addr = 0xA0020000; system_size; addr += 0x10000) {
        unsigned long sector_size =
            system_size > 0x10000 ? 0x10000 : system_size;

        err = cahute_seven_flash_sector_using_fxremote_method(
            link,
            addr,
            system,
            sector_size
        );
        if (err)
            return err;

        system += sector_size;
        system_size -= sector_size;
    }

    if (initial_sector_size) {
        err = cahute_seven_flash_sector_using_fxremote_method(
            link,
            0xA0010000,
            initial_sector,
            initial_sector_size
        );
        if (err)
            return err;
    }

    /* We can request termination. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, 0x78);
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    /* Everything should be good! */
    return CAHUTE_OK;
}
