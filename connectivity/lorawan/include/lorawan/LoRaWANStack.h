/**
 * \file      LoRaWANStack.h
 *
 * \brief     LoRaWAN stack layer implementation
 *
 * \copyright Revised BSD License, see LICENSE.TXT file include in the project
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \defgroup  LoRaWAN stack layer that controls MAC layer underneath
 *
 * License: Revised BSD License, see LICENSE.TXT file include in the project
 *
 * Copyright (c) 2017, Arm Limited and affiliates.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LORAWANSTACK_H_
#define LORAWANSTACK_H_

#include <stdint.h>
#include "events/EventQueue.h"
#include "platform/mbed_atomic.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"
#include "platform/ScopedLock.h"

#include "lorastack/mac/LoRaMac.h"
#include "system/LoRaWANTimer.h"
#include "system/lorawan_data_structures.h"
#include "LoRaRadio.h"

class LoRaPHY;

/** LoRaWANStack Class
 * A controller layer for LoRaWAN MAC and PHY
 */
class LoRaWANStack: private mbed::NonCopyable<LoRaWANStack> {

public:
    LoRaWANStack();

    /** Binds PHY layer and radio driver to stack.
     *
     * MAC layer is totally detached from the PHY layer so the stack layer
     * needs to play the role of an arbitrator.
     * This API sets the PHY layer object to stack and bind the radio driver
     * object from the application to the PHY layer.
     * Also initialises radio callback handles which the radio driver will
     * use in order to report events.
     *
     * @param radio            LoRaRadio object, i.e., the radio driver
     * @param phy              LoRaPHY object.
     *
     */
    void bind_phy_and_radio_driver(LoRaRadio &radio, LoRaPHY &phy);

    /** End device initialization.
     * @param queue            A pointer to an EventQueue passed from the application.
     * @return                 LORAWAN_STATUS_OK on success, a negative error code on failure.
     */
    lorawan_status_t initialize_mac_layer(events::EventQueue *queue);

    /** Sets all callbacks for the application.
     *
     * @param callbacks        A pointer to the structure carrying callbacks.
     * @return                 LORAWAN_STATUS_OK on success, a negative error code on failure.
     */
    lorawan_status_t set_lora_callbacks(const lorawan_app_callbacks_t *callbacks);

    /** Connect OTAA or ABP using Mbed-OS config system
     *
     * @return    For ABP:  If everything goes well, LORAWAN_STATUS_OK is returned for first call followed by
     *                      a 'CONNECTED' event. Otherwise a negative error code is returned.
     *                      Any subsequent call will return LORAWAN_STATUS_ALREADY_CONNECTED and no event follows.
     *
     *            For OTAA: When a JoinRequest is sent, LORAWAN_STATUS_CONNECT_IN_PROGRESS is returned for the first call.
     *                      Any subsequent call will return either LORAWAN_STATUS_BUSY (if the previous request for connection
     *                      is still underway) or LORAWAN_STATUS_ALREADY_CONNECTED (if a network was already joined successfully).
     *                      A 'CONNECTED' event is sent to the application when the JoinAccept is received.
     */
    lorawan_status_t connect();

    /** Connect OTAA or ABP with parameters
     *
     * @param connect  Options for an end device connection to the gateway.
     *
     * @return    For ABP:  If everything goes well, LORAWAN_STATUS_OK is returned for first call followed by
     *                      a 'CONNECTED' event. Otherwise a negative error code is returned.
     *                      Any subsequent call will return LORAWAN_STATUS_ALREADY_CONNECTED and no event follows.
     *
     *            For OTAA: When a JoinRequest is sent, LORAWAN_STATUS_CONNECT_IN_PROGRESS is returned for the first call.
     *                      Any subsequent call will return either LORAWAN_STATUS_BUSY (if the previous request for connection
     *                      is still underway) or LORAWAN_STATUS_ALREADY_CONNECTED (if a network was already joined successfully).
     *                      A 'CONNECTED' event is sent to the application when the JoinAccept is received.
     */
    lorawan_status_t connect(const lorawan_connect_t &connect);

    /** Adds channels to use.
     *
     * You can provide a list of channels with appropriate parameters filled
     * in. However, this list is not absolute. In some regions, a CF
     * list gets implemented by default, which means that the network can overwrite your channel
     * frequency settings right after receiving a Join Accept. You may try
     * to set up any channel or channels after that and if the channel requested
     * is already active, the request is silently ignored. A negative error
     * code is returned if there is any problem with parameters.
     *
     * You need to ensure that the base station nearby supports the channel or channels being added.
     *
     * If your list includes a default channel (a channel where Join Requests
     * are received) you cannot fully configure the channel parameters.
     * Either leave the channel settings to default or check your
     * corresponding PHY layer implementation. For example, LoRaPHYE868.
     *
     * @param  channel_plan     A list of channels or a single channel.
     *
     * @return                  LORAWAN_STATUS_OK on success, a negative error
     *                          code on failure.
     */
    lorawan_status_t add_channels(const lorawan_channelplan_t &channel_plan);

    /** Removes a channel from the list.
     *
     * @param channel_id        Index of the channel being removed
     *
     * @return                  LORAWAN_STATUS_OK on success, a negative error
     *                          code on failure.
     */
    lorawan_status_t remove_a_channel(uint8_t channel_id);

    /** Removes a previously set channel plan.
     *
     * @return                  LORAWAN_STATUS_OK on success, a negative error
     *                          code on failure.
     */
    lorawan_status_t drop_channel_list();

    /** Gets a list of currently enabled channels .
     *
     * @param channel_plan      The channel plan structure to store final result.
     *
     * @return                  LORAWAN_STATUS_OK on success, a negative error
     *                          code on failure.
     */
    lorawan_status_t get_enabled_channels(lorawan_channelplan_t &channel_plan);

    /** Sets up a retry counter for confirmed messages.
     *
     * Valid only for confirmed messages. This API sets the number of times the
     * stack will retry a CONFIRMED message before giving up and reporting an
     * error.
     *
     * @param count             The number of retries for confirmed messages.
     *
     * @return                  LORAWAN_STATUS_OK or a negative error code.
     */
    lorawan_status_t set_confirmed_msg_retry(uint8_t count);

    /** Sets up the data rate.
     *
     * `set_datarate()` first verifies whether the data rate given is valid or not.
     * If it is valid, the system sets the given data rate to the channel.
     *
     * @param data_rate   The intended data rate, for example DR_0 or DR_1.
     *                    Note that the macro DR_* can mean different
     *                    things in different regions.
     *
     * @return            LORAWAN_STATUS_OK if everything goes well, otherwise
     *                    a negative error code.
     */
    lorawan_status_t set_channel_data_rate(uint8_t data_rate);

    /** Enables ADR.
     *
     * @param adr_enabled       0 ADR disabled, 1 ADR enabled.
     *
     * @return                  LORAWAN_STATUS_OK on success, a negative error
     *                          code on failure.
     */
    lorawan_status_t enable_adaptive_datarate(bool adr_enabled);

    /** Send message to gateway
     *
     * @param port              The application port number. Port numbers 0 and 224
     *                          are reserved, whereas port numbers from 1 to 223
     *                          (0x01 to 0xDF) are valid port numbers.
     *                          Anything out of this range is illegal.
     *
     * @param data              A pointer to the data being sent. The ownership of the
     *                          buffer is not transferred. The data is copied to the
     *                          internal buffers.
     *
     * @param length            The size of data in bytes.
     *
     * @param flags             A flag used to determine what type of
     *                          message is being sent, for example:
     *
     *                          MSG_UNCONFIRMED_FLAG = 0x01
     *                          MSG_CONFIRMED_FLAG = 0x02
     *                          MSG_MULTICAST_FLAG = 0x04
     *                          MSG_PROPRIETARY_FLAG = 0x08
     *                          MSG_MULTICAST_FLAG and MSG_PROPRIETARY_FLAG can be
     *                          used in conjunction with MSG_UNCONFIRMED_FLAG and
     *                          MSG_CONFIRMED_FLAG depending on the intended use.
     *
     *                          MSG_PROPRIETARY_FLAG|MSG_CONFIRMED_FLAG mask will set
     *                          a confirmed message flag for a proprietary message.
     *                          MSG_CONFIRMED_FLAG and MSG_UNCONFIRMED_FLAG are
     *                          mutually exclusive.
     *
     * @param null_allowed      Internal use only. Needed for sending empty packet
     *                          having CONFIRMED bit on.
     *
     * @param allow_port_0      Internal use only. Needed for flushing MAC commands.
     *
     * @return                  The number of bytes sent, or
     *                          LORAWAN_STATUS_WOULD_BLOCK if another TX is
     *                          ongoing, or a negative error code on failure.
     */
    int16_t handle_tx(uint8_t port, const uint8_t *data,
                      uint16_t length, uint8_t flags,
                      bool null_allowed = false, bool allow_port_0 = false);

    /** Receives a message from the Network Server.
     *
     * @param data              A pointer to buffer where the received data will be
     *                          stored.
     *
     * @param length            The size of data in bytes
     *
     * @param port              The application port number. Port numbers 0 and 224
     *                          are reserved, whereas port numbers from 1 to 223
     *                          (0x01 to 0xDF) are valid port numbers.
     *                          Anything out of this range is illegal.
     *
     *                          In return will contain the number of port to which
     *                          message was received.
     *
     * @param flags             A flag is used to determine what type of
     *                          message is being received, for example:
     *
     *                          MSG_UNCONFIRMED_FLAG = 0x01,
     *                          MSG_CONFIRMED_FLAG = 0x02
     *                          MSG_MULTICAST_FLAG = 0x04,
     *                          MSG_PROPRIETARY_FLAG = 0x08
     *
     *                          MSG_MULTICAST_FLAG and MSG_PROPRIETARY_FLAG can be
     *                          used in conjunction with MSG_UNCONFIRMED_FLAG and
     *                          MSG_CONFIRMED_FLAG depending on the intended use.
     *
     *                          MSG_PROPRIETARY_FLAG|MSG_CONFIRMED_FLAG mask will set
     *                          a confirmed message flag for a proprietary message.
     *
     *                          MSG_CONFIRMED_FLAG and MSG_UNCONFIRMED_FLAG are
     *                          not mutually exclusive, i.e., the user can subscribe to
     *                          receive both CONFIRMED AND UNCONFIRMED messages at
     *                          the same time.
     *
     *                          In return will contain the flags to determine what kind
     *                          of message was received.
     *
     * @param validate_params   If set to true, the given port and flags values will be checked
     *                          against the values received with the message. If values do not
     *                          match, LORAWAN_STATUS_WOULD_BLOCK will be returned.
     *
     * @return                  It could be one of these:
     *                             i)   0 if there is nothing else to read.
     *                             ii)  Number of bytes written to user buffer.
     *                             iii) LORAWAN_STATUS_WOULD_BLOCK if there is
     *                                  nothing available to read at the moment.
     *                             iv)  A negative error code on failure.
     */
    int16_t handle_rx(uint8_t *data, uint16_t length, uint8_t &port, int &flags, bool validate_params);

    /** Send Link Check Request MAC command.
     *
     *
     * This API schedules a Link Check Request command (LinkCheckReq) for the network
     * server and once the response, i.e., LinkCheckAns MAC command is received
     * from the Network Server, an event is generated.
     *
     * A callback function for the link check response must be set prior to using
     * this API, otherwise a LORAWAN_STATUS_PARAMETER_INVALID error is thrown.
     *
     * @return          LORAWAN_STATUS_OK on successfully queuing a request, or
     *                  a negative error code on failure.
     *
     */
    lorawan_status_t set_link_check_request();

    /** Removes link check request sticky MAC command.
     *
     * Any already queued request may still get entertained. However, no new
     * requests will be made.
     */
    void remove_link_check_request();

    /** Sends DeviceTimeReq MAC command.
     *
     * This API schedules a DeviceTimeReq command for the network
     * server and once the response, i.e., DeviceTimeAns MAC command is received
     * from the Network Server, an event is generated.
     *
     * @return          LORAWAN_STATUS_OK on successfully queuing a request, or
     *                  a negative error code on failure.
     *
     */
    lorawan_status_t set_device_time_request();

    /** Shuts down the LoRaWAN protocol.
     *
     * In response to the user call for disconnection, the stack shuts down itself.
     *
     * @return          LORAWAN_STATUS_DEVICE_OFF on successfully shutdown.
     */
    lorawan_status_t shutdown();

    /** Change device class
     *
     * Change current device class.
     *
     * @param    device_class   The device class
     *
     * @return                  LORAWAN_STATUS_OK on success,
     *                          LORAWAN_STATUS_UNSUPPORTED is requested class is not supported,
     *                          or other negative error code if request failed.
     */
    lorawan_status_t set_device_class(const device_class_t &device_class);

    /** Acquire TX meta-data
     *
     * Upon successful transmission, TX meta-data will be made available
     *
     * @param    metadata    A reference to the inbound structure which will be
     *                       filled with any TX meta-data if available.
     *
     * @return               LORAWAN_STATUS_OK if successful,
     *                       LORAWAN_STATUS_METADATA_NOT_AVAILABLE otherwise
     */
    lorawan_status_t acquire_tx_metadata(lorawan_tx_metadata &metadata);

    /** Acquire RX meta-data
     *
     * Upon successful reception, RX meta-data will be made available
     *
     * @param    metadata    A reference to the inbound structure which will be
     *                       filled with any RX meta-data if available.
     *
     * @return               LORAWAN_STATUS_OK if successful,
     *                       LORAWAN_STATUS_METADATA_NOT_AVAILABLE otherwise
     */
    lorawan_status_t acquire_rx_metadata(lorawan_rx_metadata &metadata);

    /** Acquire backoff meta-data
     *
     * Get hold of backoff time after which the transmission will take place.
     *
     * @param    backoff     A reference to the inbound integer which will be
     *                       filled with any backoff meta-data if available.
     *
     * @return               LORAWAN_STATUS_OK if successful,
     *                       LORAWAN_STATUS_METADATA_NOT_AVAILABLE otherwise
     */
    lorawan_status_t acquire_backoff_metadata(int &backoff);

    /** Stops sending
     *
     * Stop sending any outstanding messages if they are not yet queued for
     * transmission, i.e., if the backoff timer is nhot elapsed yet.
     *
     * @return               LORAWAN_STATUS_OK if the transmission is cancelled.
     *                       LORAWAN_STATUS_BUSY otherwise.
     */
    lorawan_status_t stop_sending(void);

    /** Gives access to  GPS time
     *
     * Returns stored gps time + difference in stored and current tick time
     *
     * @return Current GPS time in milliseconds
     *         Or 0 if the GPS time is not yet set by the network
     */
    lorawan_gps_time_t get_current_gps_time(void);

    /** Store GPS time received from the network
     *
     * Once the GPS time is received from the network, we store it along-with the
     * current CPU monotonic tick. Having taken snapshot of the current tick would
     * enable us to have a reference to calculate the time difference given the stored
     * GPS time and the forthcoming CPU tick. Please refer to `get_current_gps_time()` API
     * documentation for more information.
     *
     * It is important that the caller relays the network provided GPS time (in milliseconds)
     * as it is without adjustment (for leap seconds or conversions to TAI/UTC etc).
     * The rationale here is that we are not setting system time here. This time base
     * is used only for device level synchronization with network.
     *
     * @param gps_time              Current GPS time provided by the network (seconds)
     */
    void set_current_gps_time(lorawan_gps_time_t gps_time);

    /** Send Ping Slot Info Request MAC command.
     *
     * Schedule a Ping Slot Info Request command (PingSlotInfoReq) for
     * the network server and once the response, i.e., PingSlotInfoAns MAC command
     * is received from the Network Server, an event is generated.
     *
     * @param  periodicity  Peridocity range is 0 to 7 encoding the current ping slot
     *                      period. pingNb     = 2^(7-periodicity) and
     *                              pingPeriod = 2^(5+periodicity)
     *
     *
     * @return          LORAWAN_STATUS_OK on successfully queuing a request, or
     *                  a negative error code on failure.
     *
     */
    lorawan_status_t add_ping_slot_info_request(uint8_t periodicity);

    /** Removes ping slot info request sticky MAC command.
     *
     * Any already queued request may still get entertained. However, no new
     * requests will be made.
     */
    void remove_ping_slot_info_request();

    /** Removes device time request sticky MAC command.
     *
     * Any already queued request may still get entertained. However, no new
     * requests will be made.
     */
    void remove_device_time_request();

    /** Enable Network beacon acquisition
     *
     * Enable Network beacon acquistion and tracking.
     *
     * @return          LORAWAN_STATUS_OK if beacon acquisition enabled, or
     *                  a negative error code on failure
     *
     */
    lorawan_status_t enable_beacon_acquisition();

    /**
     * @brief Get the last received beacon contents
     *
     * @param beacon
     * @return LORAWAN_STATUS_OK if beacon received
     *         LORAWAN_STATUS_NO_BEACON_FOUND if no beacon received
     */
    lorawan_status_t get_last_rx_beacon(loramac_beacon_t &beacon);

    /** Lock resource
     *
     * Provides mutual exclusion.
     */
    void lock(void)
    {
        _loramac.lock();
    }

    /** Unlock resource
     *
     * Release resource.
     */
    void unlock(void)
    {
        _loramac.unlock();
    }

private:
    typedef mbed::ScopedLock<LoRaWANStack> Lock;
    /**
     * Checks if the user provided port is valid or not
     */
    bool is_port_valid(uint8_t port, bool allow_port_0 = false);

    /**
     * State machine for stack controller layer.
     */
    lorawan_status_t state_controller(device_states_t new_state);


    /**
     * Send Reset indication (only in ABP & LW1.1)
     * LoRaWAN 1.1 specification mandates ABP device to send
     * ResetInd MAC command until ResetConf is received.
     *
     */
    void set_reset_indication();

    /**
     * Send Rekey indication (only in OTAA & LW1.1)
     *
     */
    void set_rekey_indication();

    /**
     * Send Device mode indication (only in OTAA & LW1.1)
     *
     */
    void set_device_mode_indication();

    /**
     * Helpers for state controller
     */
    void process_uninitialized_state(lorawan_status_t &op_status);
    void process_idle_state(lorawan_status_t &op_status);
    void process_connected_state();
    void process_connecting_state(lorawan_status_t &op_status);
    void process_joining_state(lorawan_status_t &op_status);
    void process_scheduling_state(lorawan_status_t &op_status);
    void process_status_check_state();
    void process_shutdown_state(lorawan_status_t &op_status);
    void state_machine_run_to_completion(void);

    /**
     * Handles MLME indications
     */
    void mlme_indication_handler(void);

    /**
     * Handles an MLME confirmation
     */
    void mlme_confirm_handler(loramac_mlme_confirm_t &mlme_confirm);

    /**
     * Handles an MCPS confirmation
     */
    void mcps_confirm_handler(void);

    /**
     * Handles an MCPS indication
     */
    void mcps_indication_handler(void);

    /**
     * Sets up user application port
     */
    lorawan_status_t set_application_port(uint8_t port, bool allow_port_0 = false);

    /**
     * Handles connection internally
     */
    lorawan_status_t handle_connect(bool is_otaa);


    /** Send event to application.
     *
     * @param  event            The event to be sent.
     */
    void send_event_to_application(const lorawan_event_t event) const;

    /** Send empty uplink message to network.
     *
     * Sends an empty confirmed message to gateway.
     *
     * @param  port            The event to be sent.
     */
    void send_automatic_uplink_message(uint8_t port);

    /**
     * TX interrupt handlers and corresponding processors
     */
    void tx_interrupt_handler(void);
    void tx_timeout_interrupt_handler(void);
    void process_transmission(void);
    void process_transmission_timeout(void);

    /**
     * RX interrupt handlers and corresponding processors
     */
    void rx_interrupt_handler(const uint8_t *payload, uint16_t size, int16_t rssi,
                              int8_t snr);
    void rx_timeout_interrupt_handler(void);
    void rx_error_interrupt_handler(void);
    void process_reception(const uint8_t *payload, uint16_t size, int16_t rssi,
                           int8_t snr);
    void process_reception_timeout(bool is_timeout);

    int convert_to_msg_flag(const mcps_type_t type);

    void make_tx_metadata_available(void);
    void make_rx_metadata_available(void);

    void handle_scheduling_failure(void);

    void post_process_tx_with_reception(void);
    void post_process_tx_no_reception(void);

    void process_rejoin(join_req_type_t rejoin_type, bool is_forced);
    void poll_rejoin(void);
    void reset_forced_rejoin();

    void forced_timer_expiry();
    void process_rejoin_type0();

    void handle_device_time_sync_event(lorawan_gps_time_t gps_time);

    void process_beacon_event(loramac_beacon_status_t status, const loramac_beacon_t *beacon);

private:
    LoRaMac _loramac;

    radio_events_t radio_events;
    device_states_t _device_current_state;
    lorawan_app_callbacks_t _callbacks;
    lorawan_session_t _lw_session;
    loramac_tx_message_t _tx_msg;
    loramac_rx_message_t _rx_msg;
    lorawan_tx_metadata _tx_metadata;
    lorawan_rx_metadata _rx_metadata;
    uint8_t _num_retry;
    uint8_t _qos_cnt;
    uint32_t _ctrl_flags;
    uint8_t _app_port;
    bool _link_check_requested;
    bool _reset_ind_requested;
    bool _rekey_ind_needed;
    uint8_t _rekey_ind_counter;
    bool _device_mode_ind_needed;
    bool _device_mode_ind_ongoing;
    uint8_t _new_class_type;
    bool _automatic_uplink_ongoing;
    core_util_atomic_flag _rx_payload_in_use;
    uint8_t _rx_payload[LORAMAC_PHY_MAXPAYLOAD];
    events::EventQueue *_queue;
    lorawan_time_t _tx_timestamp;
    lorawan_time_t _rx_timestamp;
    uint32_t _rejoin_type1_send_period;
    uint32_t _rejoin_type1_stamp;
    timer_event_t _rejoin_type0_timer;
    uint32_t _rejoin_type0_counter;

    uint8_t _forced_datarate;
    uint32_t _forced_period;
    uint8_t _forced_retry_count;
    join_req_type_t _forced_rejoin_type;
    uint8_t _forced_counter;
    timer_event_t _forced_timer;

    bool _ping_slot_info_requested;
    bool _device_time_requested;
    lorawan_time_t _last_beacon_rx_time;
};

#endif /* LORAWANSTACK_H_ */
