/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Example project configuration file for CHIP.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general CHIP features.
 *
 */

#pragma once

/**
 * CHIP_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY
 *
 * Enables the use of a hard-coded default Chip device id and credentials if no device id
 * is found in Chip NV storage.
 *
 * This option is for testing only and should be disabled in production releases.
 */
#define CHIP_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY 34

// Use a default setup PIN code if one hasn't been provisioned in flash.
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_PIN_CODE 20202021
#define CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR 0xF00

// Use a default pairing code if one hasn't been provisioned in flash.
#define CHIP_DEVICE_CONFIG_USE_TEST_PAIRING_CODE "CHIPUS"

// For convenience, enable Chip Security Test Mode and disable the requirement for
// authentication in various protocols.
//
//    WARNING: These options make it possible to circumvent basic Chip security functionality,
//    including message encryption. Because of this they MUST NEVER BE ENABLED IN PRODUCTION BUILDS.
//
#define CHIP_CONFIG_SECURITY_TEST_MODE 1
#define CHIP_CONFIG_REQUIRE_AUTH 0

/**
 * CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID
 *
 * 0x235A: Chip's Vendor Id.
 */
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0x235A

/**
 * CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * 0x514C: qpg lighting-app
 */
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0x514C

/**
 * CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION
 *
 * The product revision number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software revisions.
 */
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION 1

/**
 * CHIP_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION_STRING
 *
 * A string identifying the firmware revision running on the device.
 * CHIP service currently expects the firmware version to be in the format
 * {MAJOR_VERSION}.0d{MINOR_VERSION}
 */
#ifndef CHIP_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION_STRING
#define CHIP_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION_STRING "0.1ALPHA"
#endif
/**
 * CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
 *
 * Enable support for Chip-over-BLE (CHIPoBLE).
 */
#define CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE 1

/**
 * CHIP_DEVICE_CONFIG_ENABLE_CHIP_TIME_SERVICE_TIME_SYNC
 *
 * Enables synchronizing the device's real time clock with a remote Chip Time service
 * using the Chip Time Sync protocol.
 */
#define CHIP_DEVICE_CONFIG_ENABLE_CHIP_TIME_SERVICE_TIME_SYNC 0

/**
 * CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER
 *
 * Enables the use of a hard-coded default serial number if none
 * is found in Chip NV storage.
 */
#define CHIP_DEVICE_CONFIG_TEST_SERIAL_NUMBER "TEST_SN"

/**
 * CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS
 *
 * Enable recording UTC timestamps.
 */
#define CHIP_CONFIG_EVENT_LOGGING_UTC_TIMESTAMPS 1

/**
 * CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE
 *
 * A size, in bytes, of the individual debug event logging buffer.
 */
#define CHIP_DEVICE_CONFIG_EVENT_LOGGING_DEBUG_BUFFER_SIZE (512)

/**
 * CHIP_DEVICE_CONFIG_THREAD_FTD
 *
 * Disable Full Thread Device features
 */
#define CHIP_DEVICE_CONFIG_THREAD_FTD 0

/**
 * CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
 *
 * Enable Thread CLI interface at initialisation.
 */
#define CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI 1

/**
 *  @def CHIP_CONFIG_MAX_DEVICE_ADMINS
 *
 *  @brief
 *    Maximum number of administrators that can provision the device. Each admin
 *    can provision the device with their unique operational credentials and manage
 *    their access control lists.
 */
#define CHIP_CONFIG_MAX_DEVICE_ADMINS 4 // 3 fabrics + 1 for rotation slack

/**
 *  @name Interaction Model object pool configuration.
 *
 *  @brief
 *    The following definitions sets the maximum number of corresponding interaction model object pool size.
 *
 *      * #CHIP_IM_MAX_NUM_COMMAND_HANDLER
 *      * #CHIP_IM_MAX_NUM_COMMAND_SENDER
 *      * #CHIP_IM_MAX_NUM_READ_HANDLER
 *      * #CHIP_IM_MAX_NUM_READ_CLIENT
 *      * #CHIP_IM_MAX_REPORTS_IN_FLIGHT
 *      * #CHIP_IM_SERVER_MAX_NUM_PATH_GROUPS
 *      * #CHIP_IM_MAX_NUM_WRITE_HANDLER
 *      * #CHIP_IM_MAX_NUM_WRITE_CLIENT
 *
 *  @{
 */

/**
 * @def CHIP_IM_MAX_NUM_COMMAND_HANDLER
 *
 * @brief Defines the maximum number of CommandHandler, limits the number of active commands transactions on server.
 */
#define CHIP_IM_MAX_NUM_COMMAND_HANDLER 2

/**
 * @def CHIP_IM_MAX_NUM_COMMAND_SENDER
 *
 * @brief Defines the maximum number of CommandSender, limits the number of active command transactions on client.
 */
#define CHIP_IM_MAX_NUM_COMMAND_SENDER 2

/**
 * @def CHIP_IM_MAX_NUM_READ_HANDLER
 *
 * @brief Defines the maximum number of ReadHandler, limits the number of active read transactions on server.
 */
#define CHIP_IM_MAX_NUM_READ_HANDLER 3

/**
 * @def CHIP_IM_MAX_NUM_READ_CLIENT
 *
 * @brief Defines the maximum number of ReadClient, limits the number of active read transactions on client.
 */
#define CHIP_IM_MAX_NUM_READ_CLIENT 2

/**
 * @def CHIP_IM_MAX_REPORTS_IN_FLIGHT
 *
 * @brief Defines the maximum number of Reports, limits the traffic of read and subscription transactions.
 */
#define CHIP_IM_MAX_REPORTS_IN_FLIGHT 2

/**
 * @def CHIP_IM_SERVER_MAX_NUM_PATH_GROUPS
 *
 * @brief Defines the maximum number of path objects, limits the number of attributes being read or subscribed at the same time.
 */
#define CHIP_IM_SERVER_MAX_NUM_PATH_GROUPS 9

/**
 * @def CHIP_IM_MAX_NUM_WRITE_HANDLER
 *
 * @brief Defines the maximum number of WriteHandler, limits the number of active write transactions on server.
 */
#define CHIP_IM_MAX_NUM_WRITE_HANDLER 2

/**
 * @def CHIP_IM_MAX_NUM_WRITE_CLIENT
 *
 * @brief Defines the maximum number of WriteClient, limits the number of active write transactions on client.
 */
#define CHIP_IM_MAX_NUM_WRITE_CLIENT 2
