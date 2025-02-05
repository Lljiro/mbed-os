/**
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @addtogroup LoRaWAN
 * Mbed OS LoRaWAN Stack
 *  @{
 */

#ifndef LORAWANINTERFACE_H_
#define LORAWANINTERFACE_H_

#include "platform/Callback.h"
#include "platform/ScopedLock.h"
#include "events/EventQueue.h"
#include "LoRaWANStack.h"
#include "LoRaRadio.h"
#include "lorawan_types.h"

// Forward declaration of LoRaPHY class
class LoRaPHY;

/** LoRaWANInterface Class
 * A network interface for LoRaWAN
 */
class LoRaWANInterface {

public:

    /** Constructs a LoRaWANInterface using the LoRaWANStack instance underneath.
     *
     * Currently, LoRaWANStack is a singleton and you should only
     * construct a single instance of LoRaWANInterface.
     *
     * LoRaWANInterface will construct PHY based on "lora.phy" setting in mbed_app.json.
     *
     * @param radio A reference to radio object
     */
    LoRaWANInterface(LoRaRadio &radio);

    /** Constructs a LoRaWANInterface using the user provided PHY object.
     * @param radio A reference to radio object
     * @param phy   A reference to PHY object
     */
    LoRaWANInterface(LoRaRadio &radio, LoRaPHY &phy);

    /**
     * Destructor
     */
    ~LoRaWANInterface();

    /** Initialize the LoRa stack.
     *
     * You must call this before using the LoRa stack.
     *
     * @param queue A pointer to EventQueue provided by the application.
     *
     * @return         LORAWAN_STATUS_OK on success, a negative error code on failure:
     *                 LORAWAN_STATUS_PARAMETER_INVALID is NULL queue is given.
     */
    lorawan_status_t initialize(events::EventQueue *queue);

    /** Sets up UTC system time
     *
     * This API provides a convenience utility to setup UTC system time.
     * Please note that device level synchronization does not require any conversion
     * from GPS time. That's why any application level or stack level APIs involved
     * in time synchronization should always use 'get_current_gps_time()' and
     * 'set_current_gps_time(time)' APIs. 'set_system_time_utc(...)' API can be used
     * for other application purposes where acquisition of UTC time is important.
     * In addition to that it should be taken into account that the internal network
     * assisted GPS time acquisition may not be 100% accurate. It involves local monotonic
     * network provided time-stamp for GPS time may also involve inaccuracies owing to the
     * ticks (in ms) which is a direct function of CPU ticks and can be inaccurate. The
     * fact that the device will never know at what instant the time-stamp was taken and hence
     * cannot compensate for it.
     *
     * 'set_system_time_utc(...)' API utilizes stored network assisted GPS time
     * always ahead of GPS time by 19 seconds, whereas in 2019 TAI is ahead of
     * to convert for UTC time. The Temps Atomique International (TAI) time is
     * UTC by 37 seconds. This difference between TAI and UTC must be provided
     * by the user because this number is subject to change (to compensate for leap
     * seconds).
     *
     * @param tai_utc_diff    Number of seconds TAI is ahead of UTC time.
     *
     * @return LORAWAN_STATUS_OK if system time is set, negative error code
     *         otherwise.
     */
    lorawan_status_t set_system_time_utc(unsigned int tai_utc_diff);

    /** Connect OTAA or ABP using the Mbed OS config system
     *
     * Connect by Over The Air Activation or Activation By Personalization.
     * You need to configure the connection properly using the Mbed OS configuration system.
     *
     * When connecting through OTAA, the return code for success (LORAWAN_STATUS_CONNECT_IN_PROGRESS)
     * is negative. However, this is not a real error. It tells you that the connection is in progress,
     * and an event will notify you of the completion. By default, after the Join Accept message is
     * received, base stations may provide the node with a CF-List that replaces all user-configured
     * channels except the Join/Default channels. A CF-List can configure a maximum of five channels
     * other than the default channels.
     *
     * To configure more channels, we recommend that you use the `set_channel_plan()` API after the connection.
     * By default, the PHY layers configure only the mandatory Join channels. The retransmission back-off
     * restrictions on these channels are severe, and you may experience long delays or even failures
     * in the confirmed traffic. If you add more channels, the aggregated duty cycle becomes much more
     * relaxed as compared to the Join (default) channels only.
     *
     * **NOTES ON RECONNECTION:**
     * Currently, the Mbed OS LoRaWAN implementation does not support non-volatile memory storage.
     * Therefore, the state and frame counters cannot be restored after a power cycle. However,
     * if you use the `disconnect()` API to shut down the LoRaWAN protocol, the state and frame
     * counters are saved. Connecting again restores the previous session. According to the LoRaWAN
     * 1.0.2 specification, the frame counters are always reset to 0 for OTAA, and a new Join request
     * lets the network server know that the counters need a reset. The same is said about the ABP,
     * but there is no way to convey this information to the network server. For a network server,
     * an ABP device is always connected. That's why storing the frame counters is important for ABP.
     * That's why we restore frame counters from session information after a disconnection.
     *
     * @return    Common:   LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_PARAMETER_INVALID if connection parameters are invalid.
     *
     *            For ABP:  If everything goes well, LORAWAN_STATUS_OK is returned for first call
     *                      followed by a 'CONNECTED' event. Otherwise a negative error code is returned:
     *                      Any subsequent call will return LORAWAN_STATUS_ALREADY_CONNECTED and no event follows.
     *
     *            For OTAA: When a JoinRequest is sent, LORAWAN_STATUS_CONNECT_IN_PROGRESS is returned for
     *                      the first call. Any subsequent call will return either LORAWAN_STATUS_BUSY
     *                      (if the previous request for connection is still underway) or
     *                      LORAWAN_STATUS_ALREADY_CONNECTED (if a network was already joined successfully).
     *                      A 'CONNECTED' event is sent to the application when the JoinAccept is received.
     */
    lorawan_status_t connect();

    /** Connect OTAA or ABP with parameters
     *
     * All connection parameters are chosen by you and provided in the data structure passed down.
     *
     * When connecting using OTAA, the return code for success (LORAWAN_STATUS_CONNECT_IN_PROGRESS)
     * is negative. However, this is not a real error. It tells you that connection is in progress,
     * and an event will notify you of completion. By default, after Join Accept message is received,
     * base stations may provide the node with a CF-List that replaces all user-configured channels
     * except the Join/Default channels. A CF-List can configure a maximum of five channels other
     * than the default channels.
     *
     * To configure more channels, we recommend that you use the `set_channel_plan()` API after
     * the connection. By default, the PHY layers configure only the mandatory Join channels.
     * The retransmission back-off restrictions on these channels are severe, and you may experience
     * long delays or even failures in the confirmed traffic. If you add more channels, the aggregated
     * duty cycle becomes much more relaxed as compared to the Join (default) channels only.
     *
     * **NOTES ON RECONNECTION:**
     * Currently, the Mbed OS LoRaWAN implementation does not support non-volatile memory storage.
     * Therefore, the state and frame counters cannot be restored after a power cycle. However,
     * if you use the `disconnect()` API to shut down the LoRaWAN protocol, the state and frame
     * counters are saved. Connecting again restores the previous session. According to the LoRaWAN
     * 1.0.2 specification, the frame counters are always reset to zero for OTAA, and a new Join
     * request lets the network server know that the counters need a reset. The same is said about
     * the ABP, but there is no way to convey this information to the network server. For a network
     * server, an ABP device is always connected. That's why storing the frame counters is important
     * for ABP. That's why we restore frame counters from session information after a disconnection.
     *
     * @param connect  Options for an end device connection to the gateway.
     *
     * @return    Common:   LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_PARAMETER_INVALID if connection parameters are invalid.
     *
     *            For ABP:  If everything goes well, LORAWAN_STATUS_OK is returned for first call followed
     *                      by a 'CONNECTED' event. Otherwise a negative error code is returned.
     *                      Any subsequent call will return LORAWAN_STATUS_ALREADY_CONNECTED and no event follows.
     *
     *            For OTAA: When a JoinRequest is sent, LORAWAN_STATUS_CONNECT_IN_PROGRESS is returned for the
     *                      first call. Any subsequent call will return either LORAWAN_STATUS_BUSY
     *                      (if the previous request for connection is still underway) or LORAWAN_STATUS_ALREADY_CONNECTED
     *                      (if a network was already joined successfully).
     *                      A 'CONNECTED' event is sent to the application when the JoinAccept is received.
     */
    lorawan_status_t connect(const lorawan_connect_t &connect);

    /** Disconnect the current session.
     *
     * @return         LORAWAN_STATUS_DEVICE_OFF on success, a negative error code on failure:
     *                 LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     */
    lorawan_status_t disconnect();

    /** Validate the connectivity with the network.
     *
     * Application may use this API to submit a request to the stack for validation of its connectivity
     * to a Network Server. Under the hood, this API schedules a Link Check Request command (LinkCheckReq)
     * for the network server and once the response, i.e., LinkCheckAns MAC command is received from
     * the Network Server, user provided method is called.
     *
     * One way to use this API may be the validation of connectivity after a long deep sleep.
     * Mbed LoRaWANStack follows the MAC commands with data frame payload, so the application needs
     * to send something, and the Network Server may respond during the RX slots.
     *
     * This API is usable only when the application sets the 'link_check_resp' callback.
     * See add_lora_app_callbacks API. If the above mentioned callback is not set,
     * a LORAWAN_STATUS_PARAMETER_INVALID error is thrown.
     *
     * The first parameter to callback function is the demodulation margin, and the second parameter
     * is the number of gateways that successfully received the last request.
     *
     * A 'Link Check Request' MAC command remains set for every subsequent transmission, until/unless
     * the application explicitly turns it off using the remove_link_check_request() API.
     *
     * @return          LORAWAN_STATUS_OK on successfully queuing a request, or
     *                  a negative error code on failure:
     *                  LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                  LORAWAN_STATUS_PARAMETER_INVALID if link_check_resp callback method is not set.
     *
     */
    lorawan_status_t add_link_check_request();

    /** Removes link check request sticky MAC command.
     *
     * Any already queued request may still be completed. However, no new requests will be made.
     */
    void remove_link_check_request();

    /** Request the network for current time and date.
     *
     * In response to the user request for acquisition of current time and date
     * from the network, the stack generates a 'DeviceTimeReq' MAC command.
     * This MAC command is not sticky, and the user may request again if no
     * response was indicated by the stack.
     *
     * The stack will generate an event 'DEVICE_TIME_SYNCHED' if a response from
     * the network server was received and applied to the device.
     * At this point user may check the current time and date using standard
     * time() API.
     *
     * @return          LORAWAN_STATUS_OK on successfully queuing a request, or
     *                  a negative error code on failure:
     *                  LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                  LORAWAN_STATUS_NO_NETWORK_JOINED if the device is not connected to the NS
     *                  LORAWAN_STATUS_LENGTH_ERROR      if the MAC command could not fit in the outgoing message this time around
     *                  LORAWAN_STATUS_UNSUPPORTED       if the stack version is less than v1.0.3
     */
    lorawan_status_t add_device_time_request();

    /** Removes device time request sticky command.
     *
     * Any already queued request may still be completed. However, no new requests will be made.
     */
    virtual void remove_device_time_request();

    /** Sets up a particular data rate
     *
     * @param data_rate   The intended data rate, for example DR_0 or DR_1.
     *                    Please note that the macro DR_* can mean different things in different regions.
     * @return            LORAWAN_STATUS_OK if everything goes well, otherwise a negative error code:
     *                    LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                    LORAWAN_STATUS_PARAMETER_INVALID if ADR is enabled or invalid data rate is given
     */
    lorawan_status_t set_datarate(uint8_t data_rate);

    /** Enables adaptive data rate (ADR)
     *
     * The underlying LoRaPHY and LoRaMac layers handle the data rate automatically
     * based on the radio conditions (network congestion).
     *
     * @return             LORAWAN_STATUS_OK on success, negative error code on failure:
     *                     LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize()
     */
    lorawan_status_t enable_adaptive_datarate();

    /** Disables adaptive data rate
     *
     * When adaptive data rate (ADR) is disabled, either you can set a certain
     * data rate, or the MAC layer selects a default value.
     *
     * @return             LORAWAN_STATUS_OK on success, negative error code on failure:
     *                     LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize()
     */
    lorawan_status_t disable_adaptive_datarate();

    /** Sets up the retry counter for confirmed messages.
     *
     * Valid for confirmed messages only.
     *
     * The number of trials to transmit the frame, if the LoRaMAC layer did not receive an
     * acknowledgment. The MAC performs a data rate adaptation as in the LoRaWAN Specification
     * V1.0.2, chapter 18.4, table on page 64.
     *
     * Note that if the number of retries is set to 1 or 2, MAC does not decrease the data rate,
     * if the LoRaMAC layer did not receive an acknowledgment.
     *
     * @param count     The number of retries for confirmed messages.
     *
     * @return          LORAWAN_STATUS_OK or a negative error code on failure:
     *                  LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize()
     *                  LORAWAN_STATUS_PARAMETER_INVALID if count >= 255
     */
    lorawan_status_t set_confirmed_msg_retries(uint8_t count);

    /** Sets the channel plan.
     *
     * You can provide a list of channels with appropriate parameters filled in. However,
     * this list is not absolute. The stack applies a CF-List whenever available, which means
     * that the network can overwrite your channel frequency settings right after Join Accept
     * is received. You may try to set up any channel or channels after that, and if the channel
     * requested is already active, the request is silently ignored. A negative error code is
     * returned if there is any problem with parameters.
     *
     * Please note that you can also use this API to add a single channel to the existing channel plan.
     *
     * There is no reverse mechanism in the 1.0.2 specification for a node to request a particular
     * channel. Only the network server can initiate such a request.
     * You need to ensure that the corresponding base station supports the channel or channels being added.
     *
     * If your list includes a default channel (a channel where Join Requests are received),
     * you cannot fully configure the channel parameters. Either leave the channel settings to default,
     * or check your corresponding PHY layer implementation. For example, LoRaPHYE868.
     *
     * @param channel_plan      The channel plan to set.
     *
     * @return              LORAWAN_STATUS_OK on success, a negative error code on failure:
     *                      LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_PARAMETER_INVALID if number of channels is exceeding the PHY limit,
     *                      LORAWAN_STATUS_DATARATE_INVALID  if invalid data rate is given,
     *                      LORAWAN_STATUS_FREQUENCY_INVALID if invalid frequency is given,
     *                      LORAWAN_STATUS_FREQ_AND_DR_INVALID if invalid data rate and freqency are given,
     *                      LORAWAN_STATUS_BUSY              if TX currently ongoing,
     *                      LORAWAN_STATUS_SERVICE_UNKNOWN   if custom channel plans are disabled in PHY
     */
    lorawan_status_t set_channel_plan(const lorawan_channelplan_t &channel_plan);

    /** Gets the channel plans from the LoRa stack.
     *
     * Once you have selected a particular PHY layer, a set of channels is automatically activated.
     * Right after connecting, you can use this API to see the current plan. Otherwise, this API
     * returns the channel plan that you have set using `set_channel_plan()`.
     *
     * @param  channel_plan     The current channel plan information.
     *
     * @return              LORAWAN_STATUS_OK on success, a negative error code on failure:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_SERVICE_UNKNOWN if custom channel plans are disabled in PHY
     */
    lorawan_status_t get_channel_plan(lorawan_channelplan_t &channel_plan);

    /** Removes an active channel plan.
     *
     * You cannot remove default channels (the channels the base stations are listening to).
     * When a plan is abolished, only the non-default channels are removed.
     *
     * @return              LORAWAN_STATUS_OK on success, negative error code on failure
     *                      LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_BUSY              if TX currently ongoing,
     *                      LORAWAN_STATUS_SERVICE_UNKNOWN   if custom channel plans are disabled in PHY
     */
    lorawan_status_t remove_channel_plan();

    /** Removes a single channel.
     *
     * You cannot remove default channels (the channels the base stations are listening to).
     *
     * @param    index      The channel index.
     *
     * @return              LORAWAN_STATUS_OK on success, negative error code on failure:
     *                      LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_PARAMETER_INVALID if invalid channel index is given,
     *                      LORAWAN_STATUS_BUSY              if TX currently ongoing,
     *                      LORAWAN_STATUS_SERVICE_UNKNOWN   if custom channel plans are disabled in PHY
     */
    lorawan_status_t remove_channel(uint8_t index);

    /** Send message to gateway
     *
     * @param port          The application port number. Port numbers 0 and 224 are reserved,
     *                      whereas port numbers from 1 to 223 (0x01 to 0xDF) are valid port numbers.
     *                      Anything out of this range is illegal.
     *
     * @param data          A pointer to the data being sent. The ownership of the buffer is not transferred.
     *                      The data is copied to the internal buffers.
     *
     * @param length        The size of data in bytes.
     *
     * @param flags         A flag used to determine what type of message is being sent, for example:
     *
     *                      MSG_UNCONFIRMED_FLAG = 0x01
     *                      MSG_CONFIRMED_FLAG   = 0x02
     *                      MSG_MULTICAST_FLAG   = 0x04
     *                      MSG_PROPRIETARY_FLAG = 0x08
     *
     *                      All flags are mutually exclusive, and MSG_MULTICAST_FLAG cannot be set.
     *
     * @return              The number of bytes sent, or a negative error code on failure:
     *                      LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_NO_ACTIVE_SESSIONS if connection is not open,
     *                      LORAWAN_STATUS_WOULD_BLOCK       if another TX is ongoing,
     *                      LORAWAN_STATUS_PORT_INVALID      if trying to send to an invalid port (e.g. to 0)
     *                      LORAWAN_STATUS_PARAMETER_INVALID if NULL data pointer is given or flags are invalid.
     */
    int16_t send(uint8_t port, const uint8_t *data, uint16_t length, int flags);

    /** Receives a message from the Network Server on a specific port.
     *
     * @param port          The application port number. Port numbers 0 and 224 are reserved,
     *                      whereas port numbers from 1 to 223 (0x01 to 0xDF) are valid port numbers.
     *                      Anything out of this range is illegal.
     *
     * @param data          A pointer to buffer where the received data will be stored.
     *
     * @param length        The size of data in bytes.
     *
     * @param flags         A flag is used to determine what type of message is being sent, for example:
     *
     *                      MSG_UNCONFIRMED_FLAG = 0x01
     *                      MSG_CONFIRMED_FLAG   = 0x02
     *                      MSG_MULTICAST_FLAG   = 0x04
     *                      MSG_PROPRIETARY_FLAG = 0x08
     *
     *                      All flags can be used in conjunction with one another depending on the intended
     *                      use case or reception expectation.
     *
     *                      For example, MSG_CONFIRMED_FLAG and MSG_UNCONFIRMED_FLAG are
     *                      not mutually exclusive. In other words, the user can subscribe to
     *                      receive both CONFIRMED AND UNCONFIRMED messages at the same time.
     *
     * @return              It could be one of these:
     *                       i)   0 if there is nothing else to read.
     *                       ii)  Number of bytes written to user buffer.
     *                       iii) A negative error code on failure:
     *                       LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                       LORAWAN_STATUS_NO_ACTIVE_SESSIONS if connection is not open,
     *                       LORAWAN_STATUS_WOULD_BLOCK        if there is nothing available to read at the moment,
     *                       LORAWAN_STATUS_PARAMETER_INVALID  if NULL data or length is given,
     *                       LORAWAN_STATUS_WOULD_BLOCK        if incorrect port or flags are given,
     */
    int16_t receive(uint8_t port, uint8_t *data, uint16_t length, int flags);

    /** Receives a message from the Network Server on any port.
     *
     * @param data          A pointer to buffer where the received data will be stored.
     *
     * @param length        The size of data in bytes
     *
     * @param port          Return the number of port from which message was received.
     *
     * @param flags         Return flags to determine what type of message was received.
     *                      MSG_UNCONFIRMED_FLAG = 0x01
     *                      MSG_CONFIRMED_FLAG   = 0x02
     *                      MSG_MULTICAST_FLAG   = 0x04
     *                      MSG_PROPRIETARY_FLAG = 0x08
     *
     * @return              It could be one of these:
     *                       i)   0 if there is nothing else to read.
     *                       ii)  Number of bytes written to user buffer.
     *                       iii) A negative error code on failure:
     *                       LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                       LORAWAN_STATUS_NO_ACTIVE_SESSIONS if connection is not open,
     *                       LORAWAN_STATUS_PARAMETER_INVALID if NULL data or length is given,
     *                       LORAWAN_STATUS_WOULD_BLOCK if there is nothing available to read at the moment.
     */
    int16_t receive(uint8_t *data, uint16_t length, uint8_t &port, int &flags);

    /** Add application callbacks to the stack.
     *
     * An example of using this API with a latch onto 'lorawan_events' could be:
     *
     *\code
     *  LoRaWANInterface lorawan(radio);
     *  lorawan_app_callbacks_t cbs;
     *  static void my_event_handler();
     *
     *  int main()
     *  {
     *      lorawan.initialize();
     *      cbs.lorawan_events = mbed::callback(my_event_handler);
     *      lorawan.add_app_callbacks(&cbs);
     *      lorawan.connect();
     *  }
     *
     *  static void my_event_handler(lorawan_event_t event)
     *  {
     *      switch(event) {
     *          case CONNECTED:
     *              //do something
     *              break;
     *          case DISCONNECTED:
     *              //do something
     *              break;
     *          case TX_DONE:
     *              //do something
     *              break;
     *          default:
     *              break;
     *      }
     *  }
     *
     *\endcode
     *
     * @param callbacks     A pointer to the structure containing application callbacks.
     *
     * @return              LORAWAN_STATUS_OK on success, a negative error code on failure:
     *                      LORAWAN_STATUS_NOT_INITIALIZED   if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_PARAMETER_INVALID if events callback is not set
     */
    lorawan_status_t add_app_callbacks(lorawan_app_callbacks_t *callbacks);

    /** Change device class
     *
     *  Change current device class.
     *
     * **Preconditions to switch to Class B**
     *   - Stack version must be greater than v1.0.2
     *   - "lora.class-b" set to true in mbed_app.json
     *   - Network beacon found (See enable_beacon_acquisition for details)
     *
     * @param    device_class   The device class
     *
     *
     * @return              LORAWAN_STATUS_OK on success or other negative error code if request failed:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_UNSUPPORTED if requested class is not supported
     *                      LORAWAN_STATUS_NO_BEACON_FOUND if class B requested and not beacon locked
     */
    lorawan_status_t set_device_class(device_class_t device_class);

    /** Get hold of TX meta-data
     *
     * Use this method to acquire any TX meta-data related to previous transmission.
     * TX meta-data is only available right after the transmission is completed.
     * In other words, you can check for TX meta-data right after receiving the TX_DONE event.
     *
     * @param    metadata   the inbound structure that will be filled if the meta-data is available.
     *
     * @return              LORAWAN_STATUS_OK if the meta-data is available,
     *                      otherwise other negative error code if request failed:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_METADATA_NOT_AVAILABLE if the meta-data is not available
     */
    lorawan_status_t get_tx_metadata(lorawan_tx_metadata &metadata);

    /** Get hold of RX meta-data
     *
     * Use this method to acquire any RX meta-data related to current reception.
     * RX meta-data is only available right after the reception is completed.
     * In other words, you can check for RX meta-data right after receiving the RX_DONE event.
     *
     * @param    metadata   the inbound structure that will be filled if the meta-data is available.
     *
     * @return              LORAWAN_STATUS_OK if the meta-data is available,
     *                      otherwise other negative error code if request failed:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_METADATA_NOT_AVAILABLE if the meta-data is not available
     */
    lorawan_status_t get_rx_metadata(lorawan_rx_metadata &metadata);

    /** Get hold of backoff time
     *
     * In the TX path, because of automatic duty cycling, the transmission is delayed by a certain
     * amount of time, which is the backoff time. While the system schedules application data to be sent,
     * the application can inquire about how much time is left in the actual transmission to happen.
     *
     * The system will provide you with a backoff time only if the application data is in the TX pipe.
     * If however, the event is already queued for the transmission, this API returns a
     * LORAWAN_STATUS_METADATA_NOT_AVAILABLE error code.
     *
     * @param    backoff    the inbound integer that will carry the backoff time if it is available.
     *
     * @return              LORAWAN_STATUS_OK if the meta-data is available,
     *                      otherwise other negative error code if request failed:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_METADATA_NOT_AVAILABLE if the meta-data is not available
     */
    lorawan_status_t get_backoff_metadata(int &backoff);

    /** Cancel outgoing transmission
     *
     * This API is used to cancel any outstanding transmission in the TX pipe.
     * If an event for transmission is not already queued at the end of backoff timer,
     * the system can cancel the outstanding outgoing packet. Otherwise, the system is
     * busy sending and can't be held back. The system will not try to resend if the
     * outgoing message was a CONFIRMED message even if the ack is not received.
     *
     * @return              LORAWAN_STATUS_OK if the sending is canceled, otherwise
     *                      other negative error code if request failed:
     *                      LORAWAN_STATUS_NOT_INITIALIZED if system is not initialized with initialize(),
     *                      LORAWAN_STATUS_BUSY if the send cannot be canceled
     *                      LORAWAN_STATUS_NO_OP if the operation cannot be completed (nothing to cancel)
     */
    lorawan_status_t cancel_sending(void);

    /** Get network provided GPS time
     *
     * Network provides a time-stamp for the device synchronization on demand
     * using GPS time base. The request may originate from Application layer
     * Clock Synchronization protocol (for v1.0.2), stack level DevTimeReq MAC
     * command (for v.10.3 and above), or from received network beacons when
     * beacon tracking is enabled. If the request originated from the application layer,
     * application is responsible for relaying the information to the stack. In case of
     * DevTimeReq MAC command or network beacons, the stack will take care of it automatically.
     *
     * The API looks up the stored GPS time and the monotonic tick time-stamp taken
     * at the moment of storing GPS time, and returns GPS time + difference of stored
     * tick time and current tick time.
     * If the GPS time was not set by the network yet, the API returns zero.
     *
     * @return Current GPS time in milliseconds
     *         Or 0 if the GPS time is not yet set by the network
     */
    lorawan_gps_time_t get_current_gps_time(void);

    /** Set GPS time received from the network
     *
     * It is important that the caller relays the network provided GPS time (in milliseconds)
     * as it is without adjustment (for leap seconds or conversions to TAI/UTC etc).
     * The rationale here is that we are not setting system time here. This time base
     * is used only for device level synchronization with network.
     *
     * @param gps_time   Current GPS time provided by the network (seconds) */
    void set_current_gps_time(lorawan_gps_time_t gps_time);

    /** Enable network beacon acquisition
     *
     * To switch from Class A to Class B, a network beacon must first be received
     * to align timing with the network.
     *
     * Beacons are transmitted synchronously by network gateways every 128 seconds.
     * In certain regions, the beacon is transmitted on a single channel, while in others it is
     * transmitted following a frequency hopping pattern. To accelerate beacon discovery, the
     * application may use the DeviceTimeReq MAC command to set acquire the GPS time.
     *
     * When beacon acquisition is enabled and GPS time is not set the stack will open a continuous
     * receive window, listening on the beacon channel. For regions where the beacon hops frequencies,
     * the first beacon channel is selected. If the GPS time is set the stack will open a short receive
     * window at the beacon time.
     *
     * The application is notified of acquisition result via an MLME_BEACON_ACQUISITION event.
     *     If beacon is found the status is set to LORAMAC_EVENT_INFO_STATUS_OK
     *     If beacon is not found the status is set to LORAMAC_EVENT_INFO_BEACON_NOT_FOUND
     *
     * "beacon-acquisition-nb-trials" sets the number of acquistion attempts to run.
     *  The default number of attempts is 8.
     *
     * Once a beacon has been found the device can switch to Class B.
     *
     * To stay synchronized to network time, a beacon receive window is opened at every beacon time.
     * If a beacon is received a BEACON_LOCK event is delivered to the application, otherwise a
     * BEACON_MISS event is sent.
     *
     * In the event of beacon loss, the device will stay in Class B for 120 minutes after receiving
     * the last beacon. This period is called Class B beacon-less operation. During this time interval
     * the reception of a beacon will extend Class B operation by 120 minutes. After 120 minutes of
     * beacon-less Class B operation the device will switch back to Class A and a SWTICH_CLASS_B_TO_A
     * event will be sent to the application.
     *
     * @return          LORAWAN_STATUS_OK if beacon acquisition enabled, or
     *                  a negative error code on failure
     */
    virtual lorawan_status_t enable_beacon_acquisition();

    /** Get last received beacon data
     *
     * Allows the application to inspect last received beacon frame contents.
     *
     * @return    LORAWAN_STATUS_OK if beacon found
     *            LORAWAN_STATUS_NO_BEACON_FOUND when no beacon found
     */
    virtual lorawan_status_t get_last_rx_beacon(loramac_beacon_t &beacon);

    /** Set unicast ping slot period
     *
     * This API is used by the application to inform the network of its ping slot period.
     * A PING_SLOT_INFO_SYNCHED event is sent to the application when the request is acknowledged
     * by the network.  Ping slot period can only be changed when in Class A.
     *
     * In Class B mode the device periodically opens receive windows, called ping slots,
     * that the network can use to send downlinks. The number of ping slots opened
     * per beacon period (128 seconds) is determined by the configurd ping slot periodicity:
     *
     *    Periodicity  Ping Slot Period
     *    0            Every second
     *    1            Once every 2 seconds
     *    2            Once evry 4 seconds
     *    3            Once every 8 seconds
     *    4            Once every 16 seconds
     *    5            Once every 32 seconds
     *    6            Once every 64 seconds
     *    7            Once every 128 seconds
     *
     * The default ping slot period is 7. To change the default add "lora.ping-slot-periodicity": <PERIOD>
     * to mbed_app.json.
     *
     * The application will need to use this API when the ping slot period is set to a
     * value different than its configuration known by the network server.
     *
     * @return  LORAWAN_STATUS_OK on successfully queuing of the request, or
     *          a negative error code on failure:
     */
    virtual lorawan_status_t add_ping_slot_info_request(uint8_t periodicity);

    /** Stop transmitting ping slot info request
     *
     * Any already queued request may still be completed. However, no new requests will be made.
     */
    virtual void remove_ping_slot_info_request();


    /** Provides exclusive access to the stack.
     *
     * Use only if the stack is being run in it's own separate thread.
     */
    void lock(void)
    {
        _lw_stack.lock();
    }

    /** Releases exclusive access to the stack.
     *
     * Use only if the stack is being run in it's own separate thread.
     */
    void unlock(void)
    {
        _lw_stack.unlock();
    }

private:
    /** ScopedLock object
     *
     * RAII style exclusive access
     */
    typedef mbed::ScopedLock<LoRaWANInterface> Lock;

    /** LoRaWANStack object
     *
     * Handle for the LoRaWANStack class
     */
    LoRaWANStack _lw_stack;

    /** PHY object if created by LoRaWANInterface
     *
     * PHY object if LoRaWANInterface has created it.
     * If PHY object is provided by the application, this pointer is NULL.
     */
    LoRaPHY *_default_phy;
};

#endif /* LORAWANINTERFACE_H_ */
/** @}*/
