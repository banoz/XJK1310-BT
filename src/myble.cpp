#include <bluefruit.h>

#include "main.h"
#include "myble.h"

BLEDis bledis; // DIS (Device Information Service) helper class instance
BLEBas blebas; // BAS (Battery Service) helper class instance

BLEService ss = BLEService(UUID16_SVC_SCALES);
BLECharacteristic rc = BLECharacteristic(UUID16_CHR_SCALES_READ);
BLECharacteristic wc = BLECharacteristic(UUID16_CHR_SCALES_WRITE);

bool _isConnected = false;
bool _isNotifyEnabled = false;

bool isConnected(void) { return _isConnected; }
bool isNotifyEnabled(void) { return _isNotifyEnabled; }

void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection *connection = Bluefruit.Connection(conn_handle);

    char central_name[32] = {0};
    connection->getPeerName(central_name, sizeof(central_name));

    _isConnected = true;

    DEBUG_PRINT("Connected to ");
    DEBUG_PRINTLN(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void)conn_handle;
    (void)reason;

    _isConnected = false;
    _isNotifyEnabled = false;

    DEBUG_PRINT("Disconnected, reason = 0x");
    DEBUG_PRINTLNF(reason, HEX);
    DEBUG_PRINTLN("Advertising!");
}

bool _isInTare = false;
bool _isInTimer = false;
uint8_t timerState = 0;

bool isInTare(void) { return _isInTare; }
bool isInTimer(void) { return _isInTimer; }

void write_callback(uint16_t conn_hdl, BLECharacteristic *chr, uint8_t *data, uint16_t len)
{
    if (len == 7)
    {
        if (data[0] == 0x03)
        {
            if (data[1] == 0x0F)
            {
                if (!_isInTare)
                {
                    _isInTare = true;
                }
            }
            else if (data[1] == 0x0B)
            {
                if (!_isInTimer)
                {
                    timerState = data[2];
                    _isInTimer = true;
                }
            }
        }
    }
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic *chr, uint16_t cccd_value)
{
    // Display the raw request packet
    DEBUG_PRINT("CCCD Updated: ");
    // Serial.printBuffer(request->data, request->len);
    DEBUG_PRINTLN(cccd_value);

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr->uuid == wc.uuid)
    {
        if (chr->notifyEnabled(conn_hdl))
        {
            _isNotifyEnabled = true;
            // TODO            loadcell.power_up();
            DEBUG_PRINTLN("Weight Measurement 'Notify' enabled");
        }
        else
        {
            _isNotifyEnabled = false;
            // TODO            loadcell.power_down();
            DEBUG_PRINTLN("Weight Measurement 'Notify' disabled");
        }
    }
}

err_t setupWS(void)
{
    VERIFY_STATUS(ss.begin());

    wc.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);

    wc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    wc.setFixedLen(7);

    wc.setCccdWriteCallback(cccd_callback); // Optionally capture CCCD updates
    VERIFY_STATUS(wc.begin());

    rc.setProperties(CHR_PROPS_WRITE_WO_RESP);

    rc.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    rc.setFixedLen(7);

    rc.setWriteCallback(write_callback);
    VERIFY_STATUS(rc.begin());

    return ERROR_NONE;
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include WS Service UUID
    Bluefruit.Advertising.addService(ss);

    // Bluefruit.Advertising.addService(bleuart);

    // Include Name
    Bluefruit.Advertising.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
    Bluefruit.Advertising.start(0);             // 0 = Don't stop advertising after n seconds
}

bool setup_ble(void)
{
    // Initialise the Bluefruit module
    if (!Bluefruit.begin())
    {
        return false;
    }

    Bluefruit.setName(SCALES_NAME);

    // Set the connect/disconnect callback handlers
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    Bluefruit.setConnLedInterval(250);

    // Configure and Start the Device Information Service
    DEBUG_PRINTLN("Configuring the Device Information Service");
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel(SCALES_NAME);
    VERIFY_STATUS(bledis.begin(), false);

    VERIFY_STATUS(blebas.begin(), false);
    blebas.write(100);

    // Setup the Scales service using
    // BLEService and BLECharacteristic classes
    DEBUG_PRINTLN("Configuring the Scales Service");
    setupWS();

    // Setup the advertising packet(s)
    DEBUG_PRINTLN("Setting up the advertising payload(s)");
    startAdv();

    DEBUG_PRINTLN("Ready Player One!!!");
    DEBUG_PRINTLN("\nAdvertising");

    return true;
}

void setBattery(uint8_t battery)
{
    blebas.write(battery);
}

void disconnect(void)
{
    Bluefruit.disconnect(Bluefruit.connHandle());
}

unsigned long clockMillisOffset = 0;

void notifyWeight(int16_t weight)
{
    DEBUG_PRINTLN("---");
    DEBUG_PRINTLNF(highByte(weight), BIN);
    DEBUG_PRINTLNF(lowByte(weight), BIN);
    DEBUG_PRINTLN(weight);

    // unsigned long currentMillis = millis() - clockMillisOffset;

    // uint8_t minutesOn = currentMillis / 60000;
    // uint8_t secondsOn = (currentMillis - (minutesOn * 60000)) / 1000;
    // uint8_t millis100On = (currentMillis - (minutesOn * 60000) - (secondsOn * 1000)) / 100;

    // DEBUG_PRINT(minutesOn);
    // DEBUG_PRINT(":");
    // DEBUG_PRINT(secondsOn);
    // DEBUG_PRINT(":");
    // DEBUG_PRINTLN(millis100On);

    // uint8_t wsdata[10] = {0x03, 0xCA, highByte(weight), lowByte(weight), minutesOn, secondsOn, millis100On, 0, 0, 0};
    // wsdata[9] = wsdata[0] ^ wsdata[1] ^ wsdata[2] ^ wsdata[3] ^ wsdata[4] ^ wsdata[5] ^ wsdata[6] ^ wsdata[7] ^ wsdata[8];

    uint8_t wsdata[7] = {0x03, 0xCA, highByte(weight), lowByte(weight), 0, 0, 0};
    wsdata[6] = wsdata[0] ^ wsdata[1] ^ wsdata[2] ^ wsdata[3] ^ wsdata[4] ^ wsdata[5];

    if (_isNotifyEnabled)
    {
        wc.notify(wsdata, sizeof(wsdata));
    }
}

uint8_t tareCounter = 0;

void notifyTareDone(void)
{
    if (_isInTare == false)
    {
        return;
    }

    uint8_t wsdata[7] = {0x03, 0x0F, ++tareCounter, 0, 0, 0xFE, 0};
    wsdata[6] = wsdata[0] ^ wsdata[1] ^ wsdata[2] ^ wsdata[3] ^ wsdata[4] ^ wsdata[5];

    if (_isNotifyEnabled)
    {
        wc.notify(wsdata, sizeof(wsdata));
    }

    _isInTare = false;
}