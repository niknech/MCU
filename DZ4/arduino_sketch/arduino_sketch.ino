#define LED_pin 13
#define BTN_pin 2

volatile bool btn_pressed = false;

uint8_t rxData[3] = {0};
uint8_t txData[3] = {0};

void btnISR()
{
    btn_pressed = true;
}

void setup()
{
    txData[0] = 0x66;
    txData[1] = crc_mb(&txData[0], 1) >> 8;
    txData[2] = crc_mb(&txData[0], 1);

    pinMode(LED_pin, OUTPUT);
    pinMode(BTN_pin, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(BTN_pin),
        btnISR,
        FALLING
    );

    Serial.begin(115200);
}

void loop()
{
    if (btn_pressed)
    {
        Serial.write(txData[0]);
        Serial.write(txData[1]);
        Serial.write(txData[2]);

        btn_pressed = false;
    }

    if (Serial.available() >= 3)
    {
        rxData[0] = Serial.read();
        rxData[1] = Serial.read();
        rxData[2] = Serial.read();

        uint16_t crc = crc_mb(&rxData[0], 1);

        if ((rxData[1] == (uint8_t)(crc >> 8)) &&
            (rxData[2] == (uint8_t)(crc & 0xFF)))
        {
            if (rxData[0] == 0x77)
                digitalWrite(LED_pin, !digitalRead(LED_pin));
        }
    }
}

uint16_t crc_mb(uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFF;

    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];

        for (int i = 8; i != 0; i--)
        {
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}