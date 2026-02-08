# LoRa Library for Raspberry Pi Pico 2

## Overview

This LoRa library is designed for the Raspberry Pi Pico 2 using the official Pico SDK. It provides a complete implementation for LoRa communication using the hardware SPI interface.

## Hardware Connections

### Default Pin Configuration

```
LoRa Module    | Pico 2 GPIO | Pico 2 Pin | Function
---------------|-------------|-----------|----------
NSS (CS)       | GPIO 17     | Pin 22    | SPI0 CSn
SCK            | GPIO 18     | Pin 24    | SPI0 SCK
MOSI           | GPIO 19     | Pin 25    | SPI0 TX
MISO           | GPIO 16     | Pin 21    | SPI0 RX
RESET          | GPIO 7      | Pin 10    | Reset
DIO0 (IRQ)     | GPIO 6      | Pin 9     | Interrupt
```

### SPI Configuration

- **SPI Instance**: SPI0
- **Frequency**: 1 MHz (default, configurable)
- **Mode**: SPI_MODE0 (CPOL=0, CPHA=0)
- **Bit Order**: MSB First

## Dependencies

### Required Libraries

- **Raspberry Pi Pico SDK** (included via CMake)
  - `hardware/spi.h` - SPI interface
  - `hardware/gpio.h` - GPIO control
  - `pico/time.h` - Timing functions
  - `pico/stdlib.h` - Standard library functions

### CMake Configuration

The library is built as a static library `lora_common` and linked to your main executable. Include paths are automatically configured for:
- `common/interface/` - Protocol headers
- `common/LoRa/` - LoRa library source
- Pico SDK headers

## Usage Examples

### Basic Initialization

```cpp
#include "LoRa.h"

LoRaClass lora;

int main() {
    // Initialize LoRa with default frequency (433 MHz)
    if (!lora.begin(433E6)) {
        printf("LoRa initialization failed!\n");
        return 1;
    }
    
    printf("LoRa initialized successfully\n");
    
    // Your application code here
    
    return 0;
}
```

### Sending Data

```cpp
uint8_t data[] = "Hello, LoRa!";
int result = lora.sendPacketBlocking(data, sizeof(data));

if (result > 0) {
    printf("Sent %d bytes successfully\n", result);
} else {
    printf("Send failed with error code: %d\n", result);
}
```

### Receiving Data

```cpp
uint8_t buffer[128];
int received = lora.receivePacketBlocking(buffer, sizeof(buffer));

if (received > 0) {
    printf("Received %d bytes: %.*s\n", received, received, buffer);
} else if (received == -1) {
    printf("Receive timeout\n");
} else if (received == -2) {
    printf("CRC error\n");
} else if (received == -3) {
    printf("Packet too large\n");
}
```

### Configuration Options

```cpp
// Set frequency (Hz)
lora.setFrequency(868E6);  // 868 MHz

// Set spreading factor (7-12)
lora.setSpreadingFactor(SF12);

// Set bandwidth
lora.setBandwidth(BANDWIDTH_125_KHZ);

// Set coding rate
lora.setCodingRate(CODING_RATE_4_5);

// Set transmit power (2-20 dBm)
lora.setTxPower(17);

// Enable/disable CRC
lora.enableCrc();  // or lora.disableCrc()
```

## Error Codes

### Return Values

- **Positive values**: Success (often byte count)
- **-1**: Transmission/reception in progress or timeout
- **-2**: CRC error
- **-3**: Packet size error (too large or invalid)
- **Other negative**: Specific error conditions

## Advanced Features

### Interrupt-Driven Reception

The library supports interrupt-driven reception on GPIO 6 (DIO0). The interrupt is configured for falling edge triggers.

### Register Access

For debugging and advanced configuration, you can directly access LoRa registers:

```cpp
// Read a register
uint8_t value = lora.readRegister(REG_VERSION);

// Write a register
lora.writeRegister(REG_FIFO, 0x42);

// Dump all registers for debugging
lora.dumpRegisters();
```

### Power Management

```cpp
// Put LoRa in sleep mode
lora.sleep();

// Wake up (idle mode)
lora.idle();

// Continuous receive mode
lora.rxContinuous();
```

## Configuration Constants

### Default Settings (in LoRa.h)

```cpp
#define LORA_DEFAULT_SPI_FREQUENCY (1000*1000)  // 1 MHz
#define LORA_DEFAULT_SS_PIN        17           // GPIO 17
#define LORA_DEFAULT_RESET_PIN     7            // GPIO 7
#define LORA_DEFAULT_DO_INT_PIN    6            // GPIO 6

#define SPREADING_FACTOR          SF7
#define CODING_RATE               CODING_RATE_4_5
#define BANDWIDTH                 BANDWIDTH_125_KHZ
#define FREQUENCY                 433E6          // 433 MHz
```

## Troubleshooting

### Common Issues

1. **SPI Communication Failures**:
   - Verify all SPI connections (MOSI, MISO, SCK, CS)
   - Check that the LoRa module is properly powered (3.3V)
   - Ensure no other device is using the same SPI bus

2. **No Reception**:
   - Verify frequency matches between sender and receiver
   - Check spreading factor and bandwidth settings
   - Ensure antennas are properly connected
   - Check for interference on the selected frequency

3. **CRC Errors**:
   - May indicate weak signal or interference
   - Try reducing the data rate (lower bandwidth, higher spreading factor)
   - Check antenna connections

### Debugging Tips

```cpp
// Check LoRa version register
uint8_t version = lora.readRegister(REG_VERSION);
printf("LoRa version: 0x%02X (expected: 0x12)\n", version);

// Dump all registers for diagnostic
lora.dumpRegisters();

// Check RSSI (Received Signal Strength Indicator)
int rssi = lora.rssi();
printf("Current RSSI: %d dBm\n", rssi);
```

## Performance Considerations

### Range vs. Data Rate Trade-offs

- **Higher Spreading Factors (SF11, SF12)**: Better range, lower data rate, more resistant to interference
- **Lower Spreading Factors (SF7, SF8)**: Shorter range, higher data rate, less resistant to interference
- **Narrower Bandwidths**: Better sensitivity, lower data rate
- **Wider Bandwidths**: Higher data rate, less sensitive

### Power Consumption

- Sleep mode consumes minimal power
- Transmission is the most power-intensive operation
- Continuous receive mode consumes significant power
- Consider duty cycling for battery-powered applications

## Legal Considerations

### Frequency Regulations

Different countries have different regulations for LoRa frequencies:

- **Europe (EU868)**: 863-870 MHz
- **North America (US915)**: 902-928 MHz
- **Asia (AS923)**: 923 MHz
- **Other regions**: Check local regulations

Always ensure you're operating within legal frequency bands and power limits for your region.

### Duty Cycle Limitations

Some frequency bands have duty cycle limitations (e.g., 1% in EU868). The library doesn't enforce these - it's your responsibility to comply with local regulations.

## Example Applications

### Simple Chat System

See `tests/LoRa/main.cpp` for a complete example of a bidirectional communication system.

### Sensor Network Node

```cpp
#include "LoRa.h"
#include "hardware/adc.h"

LoRaClass lora;

int main() {
    lora.begin(868E6);
    
    // Initialize ADC for temperature sensor
    adc_init();
    adc_gpio_init(26);  // ADC0 on GPIO 26
    adc_select_input(0);
    
    while (true) {
        // Read temperature (simplified example)
        uint16_t adc_value = adc_read();
        float voltage = adc_value * 3.3f / 4095.0f;
        float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
        
        // Send temperature data
        uint8_t data[4];
        memcpy(data, &temperature, sizeof(temperature));
        lora.sendPacketBlocking(data, sizeof(data));
        
        sleep_ms(60000);  // Send every minute
    }
    
    return 0;
}
```

### Gateway Node

```cpp
#include "LoRa.h"

LoRaClass lora;

int main() {
    lora.begin(868E6);
    lora.rxContinuous();
    
    uint8_t buffer[128];
    
    while (true) {
        int received = lora.lora_event(buffer);
        
        if (received > 0) {
            printf("Received packet (%d bytes): ", received);
            for (int i = 0; i < received; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
            
            // Process or forward the packet
            // ...
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
```

## Future Enhancements

Potential improvements that could be added:

1. **Multiple SPI Instance Support**: Allow selection of SPI0 or SPI1
2. **Configurable Pin Mapping**: Make all pins configurable at runtime
3. **FEC (Forward Error Correction)**: Additional error correction
4. **Encryption**: Built-in payload encryption
5. **Automatic ACK**: Automatic acknowledgment protocol
6. **Frequency Hopping**: Spread spectrum techniques
7. **Adaptive Data Rate**: Automatic adjustment based on signal quality

## References

- **SX1276/77/78/79 Datasheet**: Semtech documentation for the LoRa chip
- **Raspberry Pi Pico SDK**: Official documentation for hardware interfaces
- **LoRa Alliance**: Standards and specifications for LoRaWAN

## License

This library is provided as-is for use with Raspberry Pi Pico projects. Check the main project license for distribution terms.

## Support

For issues or questions:
1. Check the example code in `tests/LoRa/main.cpp`
2. Review the register definitions and constants in `LoRa.h`
3. Consult the SX1276 datasheet for low-level details
4. Examine the Pico SDK documentation for hardware interface details