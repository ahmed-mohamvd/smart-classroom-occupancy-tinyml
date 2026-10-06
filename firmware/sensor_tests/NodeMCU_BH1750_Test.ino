#include <Wire.h>

/* NodeMCU ESP8266 default I2C pins */
#define I2C_SDA D2  // GPIO4
#define I2C_SCL D1  // GPIO5

#define BH1750_ADDR_LOW   0x23  // ADDR connected to GND
#define BH1750_ADDR_HIGH  0x5C  // ADDR connected to 3.3V

#define BH1750_POWER_ON            0x01
#define BH1750_RESET               0x07
#define BH1750_CONT_HIGH_RES_MODE  0x10

uint8_t bh1750Address = 0;

bool i2cAddressResponds(uint8_t address)
{
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

void scanI2C()
{
  uint8_t found = 0;

  Serial.println();
  Serial.println("========================================");
  Serial.println("Scanning I2C bus on NodeMCU...");
  Serial.println("SDA=D2/GPIO4, SCL=D1/GPIO5");

  for (uint8_t address = 0x08; address <= 0x77; address++)
  {
    if (i2cAddressResponds(address))
    {
      found++;
      Serial.print("FOUND: 0x");
      if (address < 0x10)
      {
        Serial.print('0');
      }
      Serial.print(address, HEX);

      if (address == 0x23)
      {
        Serial.print(" -> BH1750 (ADDR=GND)");
      }
      else if (address == 0x5C)
      {
        Serial.print(" -> BH1750 (ADDR=3.3V)");
      }
      else if ((address == 0x3C) || (address == 0x3D))
      {
        Serial.print(" -> OLED");
      }

      Serial.println();
    }
  }

  Serial.print("RESULT: ");
  Serial.print(found);
  Serial.println(" I2C device(s) found.");
  Serial.println("========================================");
}

bool detectBH1750()
{
  if (i2cAddressResponds(BH1750_ADDR_LOW))
  {
    bh1750Address = BH1750_ADDR_LOW;
    return true;
  }

  if (i2cAddressResponds(BH1750_ADDR_HIGH))
  {
    bh1750Address = BH1750_ADDR_HIGH;
    return true;
  }

  bh1750Address = 0;
  return false;
}

bool sendBH1750Command(uint8_t command)
{
  if (bh1750Address == 0)
  {
    return false;
  }

  Wire.beginTransmission(bh1750Address);
  Wire.write(command);
  return (Wire.endTransmission() == 0);
}

bool startBH1750()
{
  if (!detectBH1750())
  {
    return false;
  }

  if (!sendBH1750Command(BH1750_POWER_ON))
  {
    return false;
  }
  delay(10);

  if (!sendBH1750Command(BH1750_RESET))
  {
    return false;
  }
  delay(10);

  if (!sendBH1750Command(BH1750_CONT_HIGH_RES_MODE))
  {
    return false;
  }

  delay(180);
  return true;
}

bool readBH1750(float &lux, uint16_t &rawValue)
{
  if (bh1750Address == 0)
  {
    return false;
  }

  uint8_t received = Wire.requestFrom(bh1750Address, (uint8_t)2);
  if ((received != 2) || (Wire.available() < 2))
  {
    return false;
  }

  rawValue = ((uint16_t)Wire.read() << 8) | (uint8_t)Wire.read();
  lux = rawValue / 1.2f;
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // BH1750-safe I2C speed: 100 kHz

  Serial.println();
  Serial.println("=== NodeMCU BH1750 TEST ===");

  scanI2C();

  if (startBH1750())
  {
    Serial.print("BH1750 started at address 0x");
    Serial.println(bh1750Address, HEX);
  }
  else
  {
    Serial.println("BH1750 NOT FOUND at 0x23 or 0x5C.");
  }
}

void loop()
{
  float lux;
  uint16_t rawValue;

  if (bh1750Address == 0)
  {
    Serial.println("Retrying BH1750 detection...");
    if (startBH1750())
    {
      Serial.print("BH1750 started at address 0x");
      Serial.println(bh1750Address, HEX);
    }
    else
    {
      Serial.println("Still not found. Check VCC/GND/SDA/SCL/ADDR.");
    }
  }
  else if (readBH1750(lux, rawValue))
  {
    Serial.print("Raw: ");
    Serial.print(rawValue);
    Serial.print(" | Light: ");
    Serial.print(lux, 1);
    Serial.println(" lux");
  }
  else
  {
    Serial.println("BH1750 read failed; detecting again...");
    bh1750Address = 0;
  }

  delay(1000);
}
