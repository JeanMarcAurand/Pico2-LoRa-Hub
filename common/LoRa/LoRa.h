
/**
 * @file LoRa.h
 * @brief LoRa communication library for Raspberry Pi Pico 2
 *
 * This library provides a complete implementation for LoRa communication
 * using the hardware SPI interface of the Raspberry Pi Pico 2.
 *
 * @author Jean-
 * @date 2026
 */

#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include <cstdint>

#include "hardware/spi.h"

//=================== CONFIG ===================
/** @def LORA_DEFAULT_SPI_FREQUENCY
 * @brief Default SPI frequency in Hz (1 MHz)
 */
#define LORA_DEFAULT_SPI_FREQUENCY (1000*1000)

/** @def LORA_DEFAULT_SS_PIN
 * @brief Default slave select (CS) GPIO pin (GPIO 17, pin 22)
 */
#define LORA_DEFAULT_SS_PIN        17

/** @def LORA_DEFAULT_RESET_PIN
 * @brief Default reset GPIO pin (GPIO 7, pin 10)
 */
#define LORA_DEFAULT_RESET_PIN     7

/** @def LORA_DEFAULT_DO_INT_PIN
 * @brief Default interrupt GPIO pin (GPIO 6, pin 9)
 */
#define LORA_DEFAULT_DO_INT_PIN    6

/** @def CODING_RATE
 * @brief Default coding rate (4/5)
 */
#define CODING_RATE			CODING_RATE_4_5

/** @def BANDWIDTH
 * @brief Default bandwidth (125 kHz)
 * 
 * ATTENTION!! Si la durée d'un symbole dépasse 16 ms, il faut mettre Low Data Rate Optimization à 1.
 * cf p 114 et 132 datasheet LORA.
 * 
 * Cette durée se calcule avec la relation: 
 * durée d'un symbole = 2**SF / BW 
 * 	SF = SPREADING_FACTOR
 * 	BW = BANDWIDTH en Hz
 * 
 * Exemple SF=7, BW = 125kHz
 * 	durée d'un symbole = 1ms < 16ms, Low Data Rate Optimization=0 (valeur par defaut).
 */
#define BANDWIDTH			BANDWIDTH_125_KHZ
/** @def SPREADING_FACTOR
 * @brief Default spreading factor (SF7)
 * 
 * ATTENTION!! Si la durée d'un symbole dépasse 16 ms, il faut mettre Low Data Rate Optimization à 1.
 * cf p 114 et 132 datasheet LORA.
 * 
 * Cette durée se calcule avec la relation: 
 * durée d'un symbole = 2**SF / BW 
 * 	SF = SPREADING_FACTOR
 * 	BW = BANDWIDTH en Hz
 * 
 * Exemple SF=7, BW = 125kHz
 * 	durée d'un symbole = 1ms < 16ms, Low Data Rate Optimization=0 (valeur par defaut).
 */
#define SPREADING_FACTOR		SF7


/** @def FREQUENCY
 * @brief Default operating frequency (433 MHz)
 */
#define FREQUENCY			433E6

//==============================================

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1



// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_RSSI_VALUE           0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40
#define IRQ_RX_TIMEOUT_MASK		   0x80

#define RF_MID_BAND_THRESHOLD    525E6
#define RSSI_OFFSET_HF_PORT      157
#define RSSI_OFFSET_LF_PORT      164

//Coding rate
#define CODING_RATE_4_5			0b001
#define CODING_RATE_4_6			0b010
#define CODING_RATE_4_7			0b011
#define CODING_RATE_4_8			0b100

//Bandwidth
#define BANDWIDTH_7_8_KHZ		0b0000
#define BANDWIDTH_10_4_KHZ		0b0001
#define BANDWIDTH_15_6_KHZ		0b0010
#define BANDWIDTH_20_8_KHZ		0b0011
#define BANDWIDTH_31_25_KHZ		0b0100
#define BANDWIDTH_41_7_KHZ		0b0101
#define BANDWIDTH_62_5_KHZ		0b0110
#define BANDWIDTH_125_KHZ		0b0111
#define BANDWIDTH_250_KHZ		0b1000
#define BANDWIDTH_500_KHZ		0b1001

//Spreading factor
// #define SF6		6 Pas utilisable en mode explicit et config particuliere cf p27 
/** @def SF7
 * @brief Spreading factor 7
 */
#define SF7		7
/** @def SF8
 * @brief Spreading factor 8
 */
#define SF8		8
/** @def SF9
 * @brief Spreading factor 9
 */
#define SF9		9
/** @def SF10
 * @brief Spreading factor 10
 */
#define SF10	10
/** @def SF11
 * @brief Spreading factor 11
 */
#define SF11	11
/** @def SF12
 * @brief Spreading factor 12
 */
#define SF12	12

/** @def MAX_PKT_LENGTH
 * @brief Maximum packet length (127 bytes)
 */
#define MAX_PKT_LENGTH           127
/** @def FIFO_TX_BASE_ADDR
 * @brief TX FIFO base address
 */
#define FIFO_TX_BASE_ADDR			 0x80
/** @def FIFO_RX_BASE_ADDR
 * @brief RX FIFO base address
 */
#define FIFO_RX_BASE_ADDR			 0x0

#define ISR_PREFIX

/**
 * @class LoRaClass
 * @brief Main class for LoRa communication on Raspberry Pi Pico 2
 *
 * This class provides an interface for LoRa communication with SX1278 using
 * the Raspberry Pi Pico 2 hardware SPI interface.
 */
class LoRaClass {
	public:
	/**
	 * @brief Constructor for LoRaClass
	 *
	 * Initializes the LoRa module with default settings and configures
	 * SPI and GPIO interfaces.
	 */
	LoRaClass();

	/**
	 * @brief Initialize LoRa with specified frequency
	 *
	 * @param frequency The operating frequency in Hz (e.g., 433E6 for 433 MHz)
	 * @return 1 on success, else version number 
	 */
	int begin(long frequency);

	/**
	 * @brief Initialize LoRa with default settings
	 *
	 * Configures LoRa module with default frequency, spreading factor,
	 * bandwidth, and other parameters.
	 *
	 * @return 1 on success, -1 on failure
	 */
	int init(void);

	/**
	 * @brief End LoRa operation and put module to sleep
	 *
	 * Puts the LoRa module in sleep mode to conserve power.
	 */
	void end();

	/**
	 * @brief Begin packet transmission
	 *
	 * Prepares the LoRa module for sending a packet.
	 *
	 * @param implicitHeader If true, uses implicit header mode
	 * @return 1 on success, 0 if transmission is already in progress
	 */
	int beginPacket(int implicitHeader = false);

	/**
	 * @brief End packet transmission
	 *
	 * Completes the packet transmission process.
	 *
	 * @param async If true, transmission is asynchronous
	 * @return 1 on success
	 */
	int endPacket(bool async = false);

	/**
	 * @brief Send packet with blocking wait
	 *
	 * Sends a packet and waits for transmission to complete.
	 *
	 * @param buffer Pointer to data buffer
	 * @param size Size of data to send
	 * @return Number of bytes sent on success, negative error code on failure
	 * @retval -1 Transmission already in progress
	 * @retval -3 Invalid packet size
	 */
	int sendPacketBlocking(const uint8_t *buffer, size_t size);

	/**
	 * @brief Parse incoming packet
	 *
	 * Checks for incoming packets and prepares for reading.
	 *
	 * @param size Expected packet size (0 for explicit header mode)
	 * @return IRQ flags status
	 */
	int parsePacket(int size = 0);

	/**
	 * @brief Receive packet with blocking wait
	 *
	 * Receives a packet and waits for completion.
	 *
	 * @param buffer Pointer to receive buffer
	 * @param size Maximum size of receive buffer
	 * @return Number of bytes received on success, negative error code on failure
	 * @retval -1 Receive timeout
	 * @retval -2 CRC error
	 * @retval -3 Packet too large for buffer
	 */
	int receivePacketBlocking(const uint8_t *buffer, size_t size);

	/**
	 * @brief Check for LoRa events (non-blocking)
	 *
	 * Checks if a packet has been received using interrupt-driven approach.
	 *
	 * @param receivedData Pointer to buffer for received data
	 * @return Number of bytes received on success, negative error code on failure
	 * @retval -1 No packet received
	 * @retval -2 CRC error
	 * @retval -3 Packet too large
	 */
	int lora_event(uint8_t* receivedData);

	/**
	 * @brief Get RSSI of last received packet
	 *
	 * @return RSSI value in dBm
	 */
	int packetRssi();

	/**
	 * @brief Get SNR of last received packet
	 *
	 * @return SNR value in dB
	 */
	float packetSnr();

	/**
	 * @brief Get frequency error of last received packet
	 *
	 * @return Frequency error in Hz
	 */
	long packetFrequencyError();

	/**
	 * @brief Get current RSSI
	 *
	 * @return Current RSSI value in dBm
	 */
	int rssi();

	/**
	 * @brief Write single byte to packet
	 *
	 * @param byte Byte to write
	 * @return Number of bytes written
	 */
	virtual size_t write(uint8_t byte);

	/**
	 * @brief Write buffer to packet
	 *
	 * @param buffer Pointer to data buffer
	 * @param size Number of bytes to write
	 * @return Number of bytes written
	 */
	virtual size_t write(const uint8_t *buffer, size_t size);

	/**
	 * @brief Get number of bytes available to read
	 *
	 * @return Number of bytes available
	 */
	virtual int available();

	/**
	 * @brief Read single byte from packet
	 *
	 * @return Byte read, or -1 if no data available
	 */
	virtual int read();

	/**
	 * @brief Peek at next byte without consuming it
	 *
	 * @return Next byte, or -1 if no data available
	 */
	virtual int peek();

	/**
	 * @brief Flush receive buffer
	 */
	virtual void flush();

	/**
	 * @brief Reset LoRa module
	 *
	 * Performs hardware reset of the LoRa module.
	 */
	void reset();

	/**
	 * @brief Put LoRa in idle mode
	 *
	 * Puts the module in standby mode.
	 */
	void idle();

	/**
	 * @brief Put LoRa in sleep mode
	 *
	 * Puts the module in low-power sleep mode.
	 */
	void sleep();

	/**
	 * @brief Put LoRa in continuous receive mode
	 *
	 * Configures the module to continuously listen for incoming packets.
	 */
	void rxContinuous();

	/**
	 * @brief Set transmit power
	 *
	 * @param level Power level (2-20 dBm)
	 * @param outputPin Output pin (PA_OUTPUT_RFO_PIN or PA_OUTPUT_PA_BOOST_PIN)
	 */
	void setTxPower(int level, int outputPin = PA_OUTPUT_PA_BOOST_PIN);

	/**
	 * @brief Set operating frequency
	 *
	 * @param frequency Frequency in Hz
	 */
	void setFrequency(long frequency);

	/**
	 * @brief Set spreading factor
	 *
	 * @param sf Spreading factor (7-12)
	 */
	void setSpreadingFactor(int sf);

	/**
	 * @brief Set bandwidth using predefined constants
	 *
	 * @param mode Bandwidth mode constant (e.g., BANDWIDTH_125_KHZ)
	 */
	void setBandwidth(uint8_t mode);

	/**
	 * @brief Set bandwidth using frequency value
	 *
	 * @param sbw Bandwidth in Hz
	 */
	void setSignalBandwidth(long sbw);

	/**
	 * @brief Set coding rate using predefined constants
	 *
	 * @param rate Coding rate constant (e.g., CODING_RATE_4_5)
	 */
	void setCodingRate(uint8_t rate);

	/**
	 * @brief Set coding rate using denominator
	 *
	 * @param denominator Coding rate denominator (5-8)
	 */
	void setCodingRate4(int denominator);
	
	/**
	 * @brief Set preamble length
	 *
	 * @param length Preamble length in symbols
	 */
	void setPreambleLength(long length);

	/**
	 * @brief Set sync word
	 *
	 * @param sw Sync word value
	 */
	void setSyncWord(int sw);

	/**
	 * @brief Enable CRC checking
	 */
	void enableCrc();

	/**
	 * @brief Disable CRC checking
	 */
	void disableCrc();

	/**
	 * @brief Enable IQ inversion
	 */
	void enableInvertIQ();

	/**
	 * @brief Disable IQ inversion
	 */
	void disableInvertIQ();
	
	/**
	 * @brief Set over-current protection
	 *
	 * @param mA Current limit in mA
	 */
	void setOCP(uint8_t mA);

	/**
	 * @brief Set LNA gain
	 *
	 * @param gain Gain value (0-6, where 0 enables AGC)
	 */
	void setGain(uint8_t gain);


	/**
	 * @brief Dump all registers for debugging
	 *
	 * Prints the contents of all LoRa registers to stdout.
	 */
	void dumpRegisters(void);

	private:
	/**
	 * @brief Set explicit header mode
	 */
	void explicitHeaderMode();

	/**
	 * @brief Set implicit header mode
	 */
	void implicitHeaderMode();

	/**
	 * @brief Handle DIO0 interrupt
	 */
	void handleDio0Rise();

	/**
	 * @brief Check if transmission is in progress
	 *
	 * @return true if transmitting, false otherwise
	 */
	bool isTransmitting();

	/**
	 * @brief Get current spreading factor
	 *
	 * @return Current spreading factor
	 */
	int getSpreadingFactor();

	/**
	 * @brief Get current signal bandwidth
	 *
	 * @return Current bandwidth in Hz
	 */
	long getSignalBandwidth();

	/**
	 * @brief Set LDO flag based on symbol duration
	 */
	void setLdoFlag();

	public:
	/**
	 * @brief Read from LoRa register
	 *
	 * @param address Register address
	 * @return Register value
	 */
	uint8_t readRegister(uint8_t address);
	
	private:
	/**
	 * @brief Write to LoRa register
	 *
	 * @param address Register address
	 * @param value Value to write
	 */
	void writeRegister(uint8_t address, uint8_t value);

	/**
	 * @brief Perform single SPI transfer
	 *
	 * @param address Register address with R/W bit
	 * @param value Value to write (for write operations)
	 * @return Response from LoRa module
	 */
	uint8_t singleTransfer(uint8_t address, uint8_t value);

	private:
	long _frequency; /**< Current operating frequency */
	int _packetIndex; /**< Current packet index */
	int _implicitHeaderMode; /**< Header mode flag */
};


#endif