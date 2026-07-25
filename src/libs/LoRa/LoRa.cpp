
#include <LoRa.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include "hardware/gpio.h"
#include <pico/time.h>

// Routine d'interruption sur message LoRa.
// Volative specifier, because this variable is used in interrupt
volatile uint8_t rx_done_flag = 0;

void rxLoRa_callback(uint gpio, uint32_t events)
{
	//	rx_done_flag = 1;
	rx_done_flag += 1;
}

LoRaClass::LoRaClass() : //_spiSettings(LORA_DEFAULT_SPI_FREQUENCY, MSBFIRST, SPI_MODE0),
						 //_spi(&LORA_DEFAULT_SPI),
						 //_ss(LORA_DEFAULT_SS_PIN),
						 _frequency(0),
						 _packetIndex(0),
						 _implicitHeaderMode(0)
{

	// Init pour l'IT via D0.
	//------------------------
	gpio_init(LORA_DEFAULT_DO_INT_PIN);
	gpio_set_dir(LORA_DEFAULT_DO_INT_PIN, GPIO_IN);
	gpio_pull_up(LORA_DEFAULT_DO_INT_PIN);

	// Déclaration de l'interruption
	gpio_set_irq_enabled_with_callback(LORA_DEFAULT_DO_INT_PIN,
									   GPIO_IRQ_EDGE_RISE, // front descendant
									   true,			   // activer
									   &rxLoRa_callback	   // fonction appelée
	);

	// Initialiser SPI0 à 1 MHz.
	//--------------------------
	spi_init(spi0, 1000 * 1000);

	// Associer les GPIO à la fonction SPIcf tableau p 590
	gpio_set_function(16, GPIO_FUNC_SPI); // GPIO 16 MISO SPI0 RX pin 21

	// gpio_set_function(17, GPIO_FUNC_SPI); // GPIO 17 CSn SPI0 CSn pin 22
	gpio_init(LORA_DEFAULT_SS_PIN);
	gpio_set_dir(LORA_DEFAULT_SS_PIN, GPIO_OUT);
	gpio_put(LORA_DEFAULT_SS_PIN, 1); // On le met à l'état haut par défaut (repos)

	gpio_set_function(18, GPIO_FUNC_SPI); // GPIO 18 SCK SPI0 SCK pin 24
	gpio_set_function(19, GPIO_FUNC_SPI); // GPIO 19 MOSI SPI0 TX pin 25

	// Configure SPI avec MSB firste et SPI_MODE0, mode master par defaut.
	spi_set_format(spi0,
				   8,			 // data_bits
				   SPI_CPOL_0,	 // cpol (repos bas)
				   SPI_CPHA_0,	 // cpha (échantillon sur premier front)
				   SPI_MSB_FIRST // ordre des bits
	);

	// Mode master dèjà par defaut mais plus clair.
	spi_set_slave(spi0, false);

	// Pin pour reset.
	//----------------
	gpio_init(LORA_DEFAULT_RESET_PIN);
	gpio_set_dir(LORA_DEFAULT_RESET_PIN, GPIO_OUT);
}

int LoRaClass::begin(long frequency)
{

	// Reset du Lora, on ressort en STANDBY.
	reset();
	// put in standby mode. Inutile mais plus clair.
	idle();

	// start SPI
	//	_spi.begin();

	// check version

	uint8_t version = readRegister(REG_VERSION);
	if (version != 0x12)
	{
		printf("Version Lora @ 0x%x != 0x12 0x%x\n ", REG_VERSION, version);
		return ((int)REG_VERSION);
	}
	printf("Version Lora @ 0x%x = 0x12 0x%x\n ", REG_VERSION, version);

	// put in sleep mode
	sleep(); //??Ne sert pas pour config??

	// set frequency
	setFrequency(frequency);

	// set base addresses
	// Fifo partagée en 2 comme init par defaut donc inutile.
	// MAX_PKT_LENGTH est du coup fixé à 127.
	writeRegister(REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_ADDR);
	writeRegister(REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_ADDR);

	// set LNA boost
	writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

	// set auto AGC
	writeRegister(REG_MODEM_CONFIG_3, 0x04);

	// set output power to 17 dBm
	setTxPower(17);

	// put in standby mode
	idle(); //??Dejà dans ce mode suite au reset, a cause sleep.

	return 1;
}

int LoRaClass::init()
{
	// Reset du Lora, on ressort en STANDBY.
	reset();
	// check version
	uint8_t version = readRegister(REG_VERSION);
	if (version != 0x12)
	{
		printf("Version Lora @ 0x%x != 0x12 0x%x\n ", REG_VERSION, version);
		return (-1);
	}
	printf("Version Lora @ 0x%x = 0x12 0x%x\n ", REG_VERSION, version);

	// put in sleep mode
	sleep(); //??Ne sert pas pour config??

	// Passe en mode Lora.
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE);

	// set frequency
	setFrequency(FREQUENCY);
	// set base addresses
	// Fifo partagée en 2 comme init par defaut donc inutile.
	// MAX_PKT_LENGTH est du coup fixé à 127.
	writeRegister(REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_ADDR);
	writeRegister(REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_ADDR);

	// set LNA boost
	writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

	// set auto AGC
	// Datasheet page: 114
	// RegModemConfig3: 7-4 unused, 3 LowDataRateOptimize, 2 AgcAutoOn, 1-0 reserved
	writeRegister(REG_MODEM_CONFIG_3, 0x04);

	// Map DIO0 to RX_DONE irq
	writeRegister(REG_DIO_MAPPING_1, 0x00);

	setBandwidth(BANDWIDTH);
	setSpreadingFactor(SPREADING_FACTOR);

	setCodingRate(CODING_RATE);

	// Active le CRC en fin de message.
	writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) | 0b00000100));

	// set output power to 17 dBm
	setTxPower(17);

	// put in standby mode
	idle(); //??Dejà dans ce mode suite au reset, a cause sleep.

	sleep_ms(50);

	return 1;
}

void LoRaClass::end()
{
	// put in sleep mode
	sleep();
}

/**
 * Récupère le SNR limite pour un SF donné
 * en prenant compte de la marge MARGE_SNR.
 * Retourne -25.0f (valeur de sécurité) si le SF est hors plage
 */
int LoRaClass::getMinSnrForSF(uint8_t sf)
{
	if (sf < 6 || sf > 12)
		return (-25 + MARGE_SNR);
	return (int)(ceil(LORA_SF_LIMITS[sf - 6].minSnr + MARGE_SNR));
}

/**
 * Retourne la sensibilité en dBm pour un couple SF/BW donné
 *  en prenant compte de la marge MARGE_RSSI.
 * Si le SF est hors plage, retourne celui de SF6, sensibilité min.
 * Si le BW est hors plage, retourne celui de 500k, sensibilité min.
 */
int LoRaClass::getMinSensitivity(uint8_t sf, int bw)
{
	if (sf < 6 || sf > 12)
	{
		sf = 6;
	}
	if (bw < 0 || bw >= NUMBER_OF_BANDWIDTH)
	{
		bw = BANDWIDTH_500_KHZ;
	}
	// Indexation directe : SF6 est à l'index 0
	return (LORA_SENSITIVITY_TABLE[sf - 6].sensByBw[static_cast<uint8_t>(bw)] + MARGE_RSSI);
}

/*
 Si   SNR  < (LIMITE_SNR_POUR_SF+MARGE_SNR)
   ou RSSI < (LIMITE_RSSI_POUR_SF_BW+MARGE_RSSI)
alors puissance à la hausse
sinon
	 si   SNR  > (LIMITE_SNR_POUR_SF+MARGE_SNR)
	   et RSSI > (LIMITE_RSSI_POUR_SF_BW+MARGE_RSSI)
	alors puissance à la baisse
*/
int LoRaClass::computeNextTxPower(int precedentSNR, int precedentRSSI, int currentTxPower)
{
	printf("[--------------------\n");
	int nextTxPower; // entre 2 et 17 dBm.
	int deltaSnr = precedentSNR - getMinSnrForSF(SPREADING_FACTOR);
	int deltaRssi = precedentRSSI - getMinSensitivity(SPREADING_FACTOR, BANDWIDTH);
	printf(" precedentSNR = %d deltaSNR=%d\n", precedentSNR, deltaSnr);
	printf(" precedentRSSI = %d deltaRssi=%d\n", precedentRSSI, deltaRssi);
	printf(" currentTxPower = %d \n", currentTxPower);

	if ((deltaSnr < 0) || (deltaRssi < 0))
	{
		// On augmente la puissance.
		if (deltaSnr < deltaRssi)
		{
			nextTxPower = ceil(currentTxPower - deltaSnr * 1.3);
		}
		else
		{
			nextTxPower = ceil(currentTxPower - deltaRssi * 1.3);
		}
		printf(" On augmente la puissance nextTxPower= %d \n", nextTxPower);
	}
	else
	{
		// On diminue la puissance.
		if (deltaSnr < deltaRssi)
		{
			nextTxPower = ceil(currentTxPower - deltaSnr * 0.5);
		}
		else
		{
			nextTxPower = ceil(currentTxPower - deltaRssi * 0.5);
		}
		printf(" On diminue la puissance nextTxPower= %d \n", nextTxPower);
	}
	nextTxPower = std::clamp(nextTxPower, 2, 17);
	// Si l'ecart de correction est trop petit, ne fait rien.
	if (abs(nextTxPower - currentTxPower) <= 2)
	{
		nextTxPower = currentTxPower;
	}
	printf("--------------------]\n");

	return nextTxPower;
}


int LoRaClass::beginPacket(int implicitHeader)
{
	printf("beginPacket.1\n");
	if (isTransmitting())
	{
		printf("beginPacket.2\n");
		return 0;
	}

	printf("beginPacket.3\n");
	// put in standby mode
	idle();

	printf("beginPacket.4\n");
	if (implicitHeader)
	{
		printf("beginPacket.5\n");
		implicitHeaderMode();
	}
	else
	{
		printf("beginPacket.6\n");
		explicitHeaderMode();
	}

	printf("beginPacket.7\n");
	// reset FIFO address and payload length
	writeRegister(REG_FIFO_ADDR_PTR, 0);
	writeRegister(REG_FIFO_TX_BASE_ADDR, 0);

	writeRegister(REG_PAYLOAD_LENGTH, 1); // 0 pas authoris�.

	printf("beginPacket.8\n");
	return 1;
}

int LoRaClass::endPacket(bool async)
{
	printf("endPacket.3\n");
	// put in TX mode
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

	if (!async)
	{
		// wait for TX done
		while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
		{
			printf("endPacket.4\n");

			//			yield();
		}
		// clear IRQ's
		printf("endPacket.5\n");
		writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
	}
	printf("endPacket.6\n");

	return 1;
}

bool LoRaClass::isTransmitting()
{
	bool returnValue;
	if ((readRegister(REG_OP_MODE) & MODE_TX) == MODE_TX)
	{
		/* Transmission en cours.*/
		returnValue = true;
	}
	else
	{
		/* Pas de transmission en cours.*/
		if (readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK)
		{
			// clear IRQ's
			writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
		}
		returnValue = false;
	}
	return returnValue;
}
/*-------------------------------------------------------------------------------------------------------
sendPacketBlocking(const uint8_t *buffer, size_t size)
Attente bloquante que le transfert soit fini
size taille nombre de data dans buffer à envoyer.
valeur de retour:
 -1 emission en cours,
 -3 taille à envoyer  > MAX_PKT_LENGTH ou <= 0
 taille envoyée.

*/
int LoRaClass::sendPacketBlocking(const uint8_t *buffer, size_t size)
{
	// Test si une transmission est déjà en cours.
	if ((readRegister(REG_OP_MODE) & MODE_TX) == MODE_TX)
	{
		/* Transmission en cours.*/
		return (-1);
	}
	// printf("pas d emission en cours: %x\n", readRegister(REG_OP_MODE) & MODE_TX);
	// printf("REG_OP_MODE: %x\n", readRegister(REG_OP_MODE));

	// Vérifie la taille du message a envoyer.
	if ((size > MAX_PKT_LENGTH) || (size <= 0))
	{
		return (-3);
	}

	// put in standby mode
	// printf("REG_OP_MODE: %x\n", readRegister(REG_OP_MODE));
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
	// printf("MODE_LONG_RANGE_MODE | MODE_STDBY: %x REG_OP_MODE: %x\n", MODE_LONG_RANGE_MODE | MODE_STDBY, readRegister(REG_OP_MODE));

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

	/* Passe en mode explicit: un header est automatiquement ajouté/géré par Lora.*/
	writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);

	// reset FIFO address and payload length
	writeRegister(REG_FIFO_ADDR_PTR, FIFO_TX_BASE_ADDR);
	writeRegister(REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_ADDR);

	// Ecrit les données dans la fifo.
	// printf("Taille message: %d\n", size);
	for (size_t i = 0; i < size; i++)
	{
		writeRegister(REG_FIFO, buffer[i]);
		//	printf("%c", buffer[i]);
	}
	// printf("\n");
	writeRegister(REG_PAYLOAD_LENGTH, size);

	// Démarre l'émission.
	// printf("REG_OP_MODE: %x\n", readRegister(REG_OP_MODE));
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
	// printf("MODE_LONG_RANGE_MODE | MODE_TX: %x REG_OP_MODE: %x\n", MODE_LONG_RANGE_MODE | MODE_TX, readRegister(REG_OP_MODE));

	// Attend fin d'émission des données
	while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
	{
		//		printf("REG_OP_MODE: %x\n", readRegister(REG_OP_MODE));
		//		printf("Attend fin d'emission des donnees\n");
		//		printf("irq=%x mask=%x\n", readRegister(REG_IRQ_FLAGS), IRQ_TX_DONE_MASK);
		sleep_ms(10);
	}

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);

	return (size);
}

/*-------------------------------------------------------------------------------------------------------
receivePacketBlocking(const uint8_t *buffer, size_t size)
Attente bloquante que le transfert soit fini
size taille si en mode implicit sinon max pour buffer pour ranger les données reçues.

On ne cherche pas si une reception est en cours, on configure et on on passe en reception.
valeur de retour:
 -1 rien recu, time-out,
 -2 erreur de CRC,
 -3 taille reçu > size

 taille recue.

*/
int LoRaClass::receivePacketBlocking(const uint8_t *buffer, size_t size)
{
	int returnValue;
	int irqFlags = readRegister(REG_IRQ_FLAGS);
	size_t packetLength = 0;
	//	printf("irq=%x mask=%x\n",irqFlags, IRQ_RX_DONE_MASK);
	//	printf("1\n");

	// put in standby mode
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, readRegister(REG_IRQ_FLAGS));

	/* Passe en mode explicit: un header est automatiquement ajouté/géré par Lora.*/
	writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);

	// put in single RX mode
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
	//	printf("5\n");
	//	printf("irq=%x\n",(irqFlags & IRQ_RX_DONE_MASK));
	// Attente de la fin de réception.
	do
	{
		//		printf("6\n");
		irqFlags = readRegister(REG_IRQ_FLAGS);
		//		printf("irq=%x mask=%x\n",irqFlags, IRQ_RX_DONE_MASK);
	} while (((irqFlags & IRQ_RX_DONE_MASK) == 0) && ((irqFlags & IRQ_RX_TIMEOUT_MASK) == 0));
	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, irqFlags);

	//	printf("7\n");
	if ((irqFlags & IRQ_RX_TIMEOUT_MASK) != 0)
	{
		/* Pas de reception, sortie en time-out.*/
		returnValue = -1;
	}
	else
	{
		if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) != 0)
		{
			//	printf("8\n");
			// Problème de reception, erreur de CRC, ne prend pas en compte.
			returnValue = -2;
		}
		else
		{
			// read packet length
			packetLength = readRegister(REG_RX_NB_BYTES);

			// Verif de la taille reçue.
			if (packetLength > size)
			{
				//	printf("11\n");
				// Message reçu trop grand.
				returnValue = -3;
			}
			else
			{
				//	printf("12\n");
				returnValue = packetLength;
				// Range les données.
				uint8_t *pt = (uint8_t *)buffer;
				// set FIFO address to current RX address
				writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

				for (int indiceLecture = packetLength; indiceLecture > 0; indiceLecture--)
				{
					*pt++ = readRegister(REG_FIFO);
				}
			}
		}
	}
	return returnValue;
}
/*-------------------------------------------------------------------------------------------------------
int LoRaClass::lora_event( uint8_t** receiveData, int* nbDeData )
Méthode qui regarde si on reçu un message.Non bloquant.

size taille si en mode implicit sinon max pour buffer pour ranger les donn�es re�ues.

valeur de retour:
 -1 rien recu,
 -2 erreur de CRC,
 -3 taille reçu > size
 >=0 taille recue.

*/
int LoRaClass::lora_event(uint8_t *receivedData)
{
	// Datasheet page 39
	// 1. Mode request STAND-BY
	// 2. RX init
	// 3. Request mode rx continues
	// 4. Wait for IRQ
	// 5. RxDone -> IRQ error, back to 4
	// 6. Read rx data
	// 7. New mode request

	int returnValue = -1;

	if ((readRegister(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK) != 0)
	{
		uint8_t len;
		uint8_t irq = readRegister(REG_IRQ_FLAGS);

		//		if (!rx_done_flag)
		{
			//			printf(" lora_event ???? rx_done_flag = %d\n", rx_done_flag);
		}

		// Clear irq status
		writeRegister(REG_IRQ_FLAGS, irq);

		// Check if crc error occur
		if ((irq & IRQ_PAYLOAD_CRC_ERROR_MASK))
		{
			returnValue = -2;
		}
		else if ((irq & IRQ_RX_DONE_MASK))
		{
			// Check how many data arrived
			len = readRegister(REG_RX_NB_BYTES);
			// Verif de la taille reçue.
			if (len > MAX_PKT_LENGTH)
			{
				// Message reçu trop grand.
				returnValue = -3;
			}
			else
			{
				returnValue = len;
				// Set FIFO address to begining of the last received packet.
				writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

				// Read FIFO to buffer
				for (uint8_t i = 0; i < len; i++)
					receivedData[i] = readRegister(REG_FIFO);
			}
		}
		rx_done_flag = 0;
	}
	return (returnValue);
}
int LoRaClass::parsePacket(int size)
{
	int packetLength = 0;
	int irqFlags = readRegister(REG_IRQ_FLAGS);

	if (size > 0)
	{
		implicitHeaderMode();

		writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
	}
	else
	{
		explicitHeaderMode();
	}

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, irqFlags);

	if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)
	{
		// received a packet
		_packetIndex = 0;

		// read packet length
		if (_implicitHeaderMode)
		{
			packetLength = readRegister(REG_PAYLOAD_LENGTH);
		}
		else
		{
			packetLength = readRegister(REG_RX_NB_BYTES);
		}

		// set FIFO address to current RX address
		writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

		// put in standby mode
		idle();
	}
	else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE))
	{
		// not currently in RX mode

		// reset FIFO address
		writeRegister(REG_FIFO_ADDR_PTR, 0);

		// put in single RX mode
		writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
	}

	//	return packetLength;
	return (irqFlags);
}

int LoRaClass::packetRssi()
{
	return (readRegister(REG_PKT_RSSI_VALUE) - (_frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

float LoRaClass::packetSnr()
{
	return ((int8_t)readRegister(REG_PKT_SNR_VALUE)) * 0.25;
}

long LoRaClass::packetFrequencyError()
{
	int32_t freqError = 0;
	freqError = static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MSB) & 0b00000111);
	freqError <<= 8L;
	freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MID));
	freqError <<= 8L;
	freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_LSB));

	if (readRegister(REG_FREQ_ERROR_MSB) & 0b00001000)
	{						 // Sign bit is on
		freqError -= 524288; // B1000'0000'0000'0000'0000
	}

	const float fXtal = 32E6;																						  // FXOSC: crystal oscillator (XTAL) frequency (2.5. Chip Specification, p. 14)
	const float fError = ((static_cast<float>(freqError) * (1L << 24)) / fXtal) * (getSignalBandwidth() / 500000.0f); // p. 37

	return static_cast<long>(fError);
}

int LoRaClass::rssi()
{
	return (readRegister(REG_RSSI_VALUE) - (_frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

size_t LoRaClass::write(uint8_t byte)
{
	return write(&byte, sizeof(byte));
}

size_t LoRaClass::write(const uint8_t *buffer, size_t size)
{
	int currentLength = readRegister(REG_PAYLOAD_LENGTH);

	// check size
	if ((currentLength + size) > MAX_PKT_LENGTH)
	{
		size = MAX_PKT_LENGTH - currentLength;
	}

	// write data
	for (size_t i = 0; i < size; i++)
	{
		writeRegister(REG_FIFO, buffer[i]);
	}

	// update length
	writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);

	return size;
}

int LoRaClass::available()
{
	printf("_packetIndex = %d REG_RX_NB_BYTES = %d\n", _packetIndex, readRegister(REG_RX_NB_BYTES));

	return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

int LoRaClass::read()
{
	if (!available())
	{
		return -1;
	}

	_packetIndex++;

	return readRegister(REG_FIFO);
}

int LoRaClass::peek()
{
	if (!available())
	{
		return -1;
	}

	// store current FIFO address
	int currentAddress = readRegister(REG_FIFO_ADDR_PTR);

	// read
	uint8_t b = readRegister(REG_FIFO);

	// restore FIFO address
	writeRegister(REG_FIFO_ADDR_PTR, currentAddress);

	return b;
}

void LoRaClass::flush()
{
}

void LoRaClass::reset()
{
	// perform reset
	gpio_put(LORA_DEFAULT_RESET_PIN, 0);
	sleep_ms(10);
	gpio_put(LORA_DEFAULT_RESET_PIN, 1);
	sleep_ms(10);
}

// void LoRaClass::interruptInit()
//{
//	cli(); // D�sactive l'interruption globale

//// Passe en entree et r�sistance de pullup.
// pinMode( 8,INPUT);
// digitalWrite(8, HIGH);

// Set INT0 to rising edge
//	EICRA |= (1 << ISC01) | (1 << ISC00);
// Allow INT0 to trigger
//	EIMSK |= (1 << INT0);

//	sei(); // Active l'interruption globale
//}

void LoRaClass::idle()
{
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void LoRaClass::sleep()
{
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void LoRaClass::rxContinuous()
{
	// put in standby mode
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

	// clear IRQ's
	writeRegister(REG_IRQ_FLAGS, readRegister(REG_IRQ_FLAGS));

	/* Passe en mode explicit: un header est automatiquement ajouté/géré par Lora.*/
	writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);

	// Passe en mode Rx continuous
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

void LoRaClass::setTxPower(int level, int outputPin)
{
	if (PA_OUTPUT_RFO_PIN == outputPin)
	{
		// RFO
		if (level < 0)
		{
			level = 0;
		}
		else if (level > 14)
		{
			level = 14;
		}

		writeRegister(REG_PA_CONFIG, 0x70 | level);
	}
	else
	{
		// PA BOOST
		if (level > 17)
		{
			if (level > 20)
			{
				level = 20;
			}

			// subtract 3 from level, so 18 - 20 maps to 15 - 17
			level -= 3;

			// High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
			writeRegister(REG_PA_DAC, 0x87);
			setOCP(140);
		}
		else
		{
			if (level < 2)
			{
				level = 2;
			}
			// Default value PA_HF/LF or +17dBm
			writeRegister(REG_PA_DAC, 0x84);
			setOCP(100);
		}

		writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
	}
}

void LoRaClass::setFrequency(long frequency)
{
	_frequency = frequency;

	// cf paragraphe 5.3.3
	uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

	writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
	writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
	writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

int LoRaClass::getSpreadingFactor()
{
	return readRegister(REG_MODEM_CONFIG_2) >> 4;
}

void LoRaClass::setSpreadingFactor(int sf)
{
	if (sf < 6)
	{
		sf = 6;
	}
	else if (sf > 12)
	{
		sf = 12;
	}

	if (sf == 6)
	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
	}
	else
	{
		writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
		writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
	}

	writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
	setLdoFlag();
}

long LoRaClass::getSignalBandwidth()
{
	uint8_t bw = (readRegister(REG_MODEM_CONFIG_1) >> 4);

	switch (bw)
	{
	case 0:
		return 7.8E3;
	case 1:
		return 10.4E3;
	case 2:
		return 15.6E3;
	case 3:
		return 20.8E3;
	case 4:
		return 31.25E3;
	case 5:
		return 41.7E3;
	case 6:
		return 62.5E3;
	case 7:
		return 125E3;
	case 8:
		return 250E3;
	case 9:
		return 500E3;
	}

	return -1;
}

void LoRaClass::setBandwidth(uint8_t mode)
{
	// RegModemConfig1: 7-4 Bandwidth, 3-1 CodingRate, 0 ImplicitHeaderModeOn
	// Datasheet page 112
	writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (mode << 4));
}

void LoRaClass::setSignalBandwidth(long sbw)
{
	int bw;

	if (sbw <= 7.8E3)
	{
		bw = 0;
	}
	else if (sbw <= 10.4E3)
	{
		bw = 1;
	}
	else if (sbw <= 15.6E3)
	{
		bw = 2;
	}
	else if (sbw <= 20.8E3)
	{
		bw = 3;
	}
	else if (sbw <= 31.25E3)
	{
		bw = 4;
	}
	else if (sbw <= 41.7E3)
	{
		bw = 5;
	}
	else if (sbw <= 62.5E3)
	{
		bw = 6;
	}
	else if (sbw <= 125E3)
	{
		bw = 7;
	}
	else if (sbw <= 250E3)
	{
		bw = 8;
	}
	else /*if (sbw <= 250E3)*/
	{
		bw = 9;
	}

	writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
	setLdoFlag();
}
void LoRaClass::setCodingRate(uint8_t rate)
{
	// Datasheet page 27

	// Datasheet page 112
	// RegModemConfig1: 7-4 Bandwidth, 3-1 CodingRate, 0 ImplicitHeaderModeOn

	writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0b11110001) | (rate << 1));
}

void LoRaClass::setLdoFlag()
{
	// Section 4.1.1.5
	long symbolDuration = 1000 / (getSignalBandwidth() / (1L << getSpreadingFactor()));

	// Section 4.1.1.6
	// boolean ldoOn = symbolDuration > 16;

	uint8_t config3 = readRegister(REG_MODEM_CONFIG_3);
	// bitWrite(config3, 3, ldoOn);
	if (symbolDuration > 16)
	{
		config3 = (config3 | (1 << 3));
	}
	writeRegister(REG_MODEM_CONFIG_3, config3);
}

void LoRaClass::setCodingRate4(int denominator)
{
	if (denominator < 5)
	{
		denominator = 5;
	}
	else if (denominator > 8)
	{
		denominator = 8;
	}

	int cr = denominator - 4;

	writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

void LoRaClass::setPreambleLength(long length)
{
	writeRegister(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
	writeRegister(REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

void LoRaClass::setSyncWord(int sw)
{
	writeRegister(REG_SYNC_WORD, sw);
}

void LoRaClass::enableCrc()
{
	writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) | 0x04);
}

void LoRaClass::disableCrc()
{
	writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) & 0xfb);
}

void LoRaClass::enableInvertIQ()
{
	writeRegister(REG_INVERTIQ, 0x66);
	writeRegister(REG_INVERTIQ2, 0x19);
}

void LoRaClass::disableInvertIQ()
{
	writeRegister(REG_INVERTIQ, 0x27);
	writeRegister(REG_INVERTIQ2, 0x1d);
}

void LoRaClass::setOCP(uint8_t mA)
{
	uint8_t ocpTrim = 27;

	if (mA <= 120)
	{
		ocpTrim = (mA - 45) / 5;
	}
	else if (mA <= 240)
	{
		ocpTrim = (mA + 30) / 10;
	}

	writeRegister(REG_OCP, 0x20 | (0x1F & ocpTrim));
}

void LoRaClass::setGain(uint8_t gain)
{
	// check allowed range
	if (gain > 6)
	{
		gain = 6;
	}

	// set to standby
	idle();

	// set gain
	if (gain == 0)
	{
		// if gain = 0, enable AGC
		writeRegister(REG_MODEM_CONFIG_3, 0x04);
	}
	else
	{
		// disable AGC
		writeRegister(REG_MODEM_CONFIG_3, 0x00);

		// clear Gain and set LNA boost
		writeRegister(REG_LNA, 0x03);

		// set gain
		writeRegister(REG_LNA, readRegister(REG_LNA) | (gain << 5));
	}
}

#if 0
uint8_t LoRaClass::random()
{
	return readRegister(REG_RSSI_WIDEBAND);
}
#endif

#if 0
void LoRaClass::setSPI(SPIClass& spi)
{
	_spi = &spi;
}

void LoRaClass::setSPIFrequency(uint32_t frequency)
{
	_spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
}
#endif

// void LoRaClass::dumpRegisters(Stream& out)
void LoRaClass::dumpRegisters(void)
{
	for (int i = 0; i < 128; i++)
	{
		printf("0x%x: 0x%x\n", i, readRegister(i));
		// out.print("0x");
		// out.print(i, HEX);
		// out.print(": 0x");
		// out.println(readRegister(i), HEX);
	}
}

void LoRaClass::explicitHeaderMode()
{
	/* Il y a un header automatiquement ajouté par Lora.*/
	_implicitHeaderMode = 0;

	writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

void LoRaClass::implicitHeaderMode()
{
	/* Il n'y a pas de header ajouté par Lora.*/
	_implicitHeaderMode = 1;

	writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
}

uint8_t LoRaClass::readRegister(uint8_t address)
{
	return singleTransfer(address & 0x7f, 0x00);
}

void LoRaClass::writeRegister(uint8_t address, uint8_t value)
{
	singleTransfer(address | 0x80, value);
}
inline void lora_select()
{
	gpio_put(LORA_DEFAULT_SS_PIN, 0);
	// On assure le t_nsetup (30ns).
	// Sur un RP2350 à 150MHz, 5 cycles = 33ns.
	busy_wait_at_least_cycles(10);
}

inline void lora_deselect()
{
	// On assure le t_nhold (100ns) AVANT de remonter le CS
	// Sur un RP2350 à 150MHz, 15 cycles = 106ns.
	busy_wait_at_least_cycles(15);
	gpio_put(LORA_DEFAULT_SS_PIN, 1);
}

uint8_t LoRaClass::singleTransfer(uint8_t address, uint8_t value)
{
	uint8_t response;
	lora_select();
	spi_write_blocking(spi0, &address, 1);		  // On envoie l'adresse
	spi_read_blocking(spi0, value, &response, 1); // On lit la réponse et ecrit la valeur
	lora_deselect();
	return response;
}
