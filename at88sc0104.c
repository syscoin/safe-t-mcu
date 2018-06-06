/*
 * Low level driver for cryptomem AT88SC0104CA
 *
 * based on https://github.com/xiaoguigit/at88sc and
 * https://github.com/crewrktablets/rk30_kernel/blob/master/drivers/misc/at88scxx.c
 * and the description of the crypto algorithm in
 * Flavio D. Garcia, Peter van Rossum, Roel Verdult, Ronny Wichers Schreur:
 * "Dismantling SecureMemory, CryptoMemory and Crypto". https://eprint.iacr.org/2010/169.pdf and
 * https://www.esat.kuleuven.be/cosic/publications/article-2113.pdf)
 * */
#if CRYPTOMEM
#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>

#include "at88sc0104.h"

#include "util.h" // delay()
#include "timer.h"
#include "memzero.h"

/***********************************************************************
          Algorithm part of the definition and function statement
************************************************************************/

// Basic Definitions (if not available elsewhere)
#ifndef FALSE
#define FALSE       (0)
#define TRUE        (!FALSE)
#endif


// Global variables
static uint8_t CM_UserZone;
static uint8_t CM_AntiTearing;
static uint8_t CM_Encrypt;
static uint8_t CM_Authenticate;



#define CM_PWRON_CLKS (15)

/*****************************************************************
           I2C Address/command definition
 ****************************************************************/

#define CM_CMD_WRUSERZONE	0xB0
#define CM_CMD_RANDOMREAD	0xB1
#define CM_CMD_RDUSERZONE	0xB2
#define CM_CMD_SYSTEMWR		0xB4
#define CM_CMD_SYSTEMRD		0xB6
#define CM_CMD_VERIFYCRYPTO	0xB8
#define CM_CMD_VERIFYPWD	0xBA



/* Crypto algorithm part */
static void cm_ResetCrypto(void);
static uint8_t cm_GPAGen(uint8_t Datain);
static uint8_t cm_GPAGenN(uint8_t Datain, uint8_t Count);
static void cm_CalChecksum(uint8_t *Ck_sum);
static void cm_AuthEncryptCalc(uint8_t *Ci, uint8_t *G_Sk, uint8_t *Q, uint8_t *Ch);
static void cm_GPAcmd2(uint8_t *InsBuff);
static void cm_GPAdecrypt(uint8_t Encrypt, uint8_t *Buffer, uint8_t Count);
static void cm_GPAencrypt(uint8_t Encrypt, uint8_t *Buffer, uint8_t Count);

/* low level interface */

static uint8_t cm_SendCommand(uint8_t *InsBuff);
static uint8_t cm_ReceiveData(uint8_t *ReceiveData, uint8_t Length);
static uint8_t cm_SendData(const uint8_t *SendData, uint8_t Length);
static uint8_t cm_WaitAckPolling(uint8_t timeout);
static uint8_t cm_SendCmdByte(uint8_t Command);

static uint8_t cm_ReadCommand(uint8_t *InsBuff, uint8_t *RetVal, uint8_t Len);
static uint8_t cm_WriteCommand(uint8_t *InsBuff, const uint8_t *SendVal, uint8_t Len);


static uint8_t cm_AuthEncrypt(uint8_t KeySet, uint8_t AddrCi, uint8_t *Ci,
		uint8_t * G_Sk, const uint8_t * Random)
{
	uint8_t Return;
	uint8_t Ci2[8];
	uint8_t Q_Ch[16];
	uint8_t CmdVerifyCrypto[4] = { CM_CMD_VERIFYCRYPTO, KeySet, 0x00, 0x10 };

	/* Get a random number */
	if (!Random)
		return CM_FAILED;

	memcpy(Q_Ch, Random, 8);

	cm_AuthEncryptCalc(Ci, G_Sk, Q_Ch, &Q_Ch[8]);

	/* Initiate certification */
	if ((Return = cm_WriteCommand(CmdVerifyCrypto, Q_Ch, 16)) != CM_SUCCESS) {
		return Return;
	}

	/* On chip calculation needs some time - see table 8-2 */
	if ((Return = cm_WaitAckPolling(10)) != CM_SUCCESS)
		return Return;

	/* Read the calibration results */
	if ((Return = cm_ReadConfigZone(AddrCi, Ci2, 8)) != CM_SUCCESS) {
		return Return;
	}

	/* Compare */
	if (memcmp(Ci, Ci2, 8)) {
		return CM_FAILED;
	}

	return CM_SUCCESS;
}

/*!
 *
 * \brief Activate Security
 *
 * \note When calling this function:
 * reads the current cryptogram (Ci) of the key set,
 * computes the next cryptogram (Ci+1) based on the secret key Key (GCi) and the random number selected,
 * sends the (Ci+1) and the random number to the CryptoMemory device,
 * computes (Ci+2) and compares the computed value to the new cryptogram of the key set (read from the chip).
 * If (Ci+2) matches the new cryptogram of the key set, authentication was successful.
 * In addition, if Encrypt is TRUE, the function:
 * 	computes the new session key (Ci+3) and a challenge,
 * 	sends the new session key and the challenge to the CryptoMemory device,
 * If the new session key and the challenge are correctly related, encryption is activated.
 *
 * \param KeySet: select key set 0..3
 * \param Key: secret seed S_k - will be overwritten by updated key S_k_i+1
 * \param Random: 8/16 bytes of random data used to compute the challenge (8 for authentication, 8 for encryption)
 * \param Encrypt: if !=0 then start protocol to enable encryption
 *
 * \retval 0 on success
 */

uint8_t cm_ActivateSecurity(uint8_t KeySet, uint8_t * Key, uint8_t * Random,
		uint8_t Encrypt)
{
	uint8_t AddrCi;
	uint8_t Return;
	uint8_t G_Sk[8];
	uint8_t Ci[8];

	if (!Random)
		return CM_FAILED;

	/* Read Ci  */
	AddrCi = CM_CI_ADDR + (KeySet << 4);
	if ((Return = cm_ReadConfigZone(AddrCi, Ci, 8)) != CM_SUCCESS) {
		return Return;
	}

	/* Gc */
	memcpy(G_Sk, Key, 8);

	/* Activate certification */
	if ((Return = cm_AuthEncrypt(KeySet, AddrCi, Ci, G_Sk, Random))
			!= CM_SUCCESS) {
		return Return;
	}

	CM_Authenticate = TRUE;

	/* If cryptographic authentication is required, activate cryptographic authentication */
	if (Encrypt) {
		if ((Return = cm_AuthEncrypt(KeySet + 0x10, AddrCi, Ci, G_Sk,
				Random + 8)) != CM_SUCCESS) {
			return Return;
		}
		CM_Encrypt = TRUE;
	}

	/* The certification is successful */
	return CM_SUCCESS;
}


/* Chip communication test function */
uint8_t cm_aCommunicationTest(void)
{
    uint8_t write_data[2] = {0x55, 0xaa};//Test Data
    uint8_t read_data[2];
    cm_WriteConfigZone(0xa, write_data, 2, FALSE);
    cm_ReadConfigZone(0xa, read_data, 2);
    if(memcmp(write_data, read_data, 2) != 0){
        return CM_FAILED;
    }
    return CM_SUCCESS;
}


/*************************************************************************
                              Bus communication
**************************************************************************/

/*************************************************************************
                        GPIO Macros - I2C like bit banging
*************************************************************************/

/* CLK on PB6, DATA on PB7 */

#define CM_CLK_OUT 		gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO6)
#define CM_CLK_HI		gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO6)
#define CM_CLK_LO   	{ gpio_clear(GPIOB, GPIO6); CM_CLK_OUT; }

#define CM_DATA_OUT 	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO7)
#define CM_DATA_IN    	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO7)
#define CM_DATA_HI		CM_DATA_IN
#define CM_DATA_LO  	{ gpio_clear(GPIOB, GPIO7); CM_DATA_OUT; }
#define CM_DATA_RD  	gpio_get(GPIOB, GPIO7)

/*************************************************************************
                                       Basic bus communication
**************************************************************************/
static void cm_Delay(uint8_t Delay)
{
    delay(Delay);
}

 // Half a clock cycle high
static void cm_Clockhigh(void)
{
    cm_Delay(1);
    CM_CLK_HI;
    cm_Delay(1);
}

// Half clock cycle is low
static void cm_Clocklow(void)
{
    cm_Delay(1);
    CM_CLK_LO;
    cm_Delay(1);
}

// A complete clock
static void cm_ClockCycle(void)
{
    cm_Delay(1);
    CM_CLK_LO;
    cm_Delay(2);
    CM_CLK_HI;
    cm_Delay(1);
}

// n complete clock cycles
static void cm_ClockCycles(uint8_t Count)
{
    uint8_t i;
    for (i = 0; i < Count; ++i) {
        cm_ClockCycle();
    }
}

// Start condition
static void cm_Start(void)
{
    CM_DATA_HI;
    cm_Delay(4);
    cm_Clockhigh();
    cm_Delay(4);
    CM_DATA_LO;
    cm_Delay(8);
    cm_Clocklow();
    cm_Delay(8);
}

// Stop condition
static void cm_Stop(void)
{
    cm_Clocklow();
    CM_DATA_LO;
    cm_Delay(4);
    cm_Clockhigh();
    cm_Delay(8);
    CM_DATA_HI;
    cm_Delay(4);
}

// Write a byte on the bus
// return 0 on success
static uint8_t cm_Write(uint8_t Data)
{
    uint8_t i;

    for(i=0; i<8; i++) {
        cm_Clocklow();
        if (Data&0x80) {
            CM_DATA_HI;
        } else {
            CM_DATA_LO;
        }
        cm_Clockhigh();
        Data = Data<<1;
    }
    cm_Clocklow();

    // Wait for ACK
    CM_DATA_HI;
    cm_Delay(8);
    cm_Clockhigh();

    while(i > 1) {
        cm_Delay(2);
        if (CM_DATA_RD)
            i--;
        else {
            i = 0;    //ACK
            break;
        }
    }
    cm_Clocklow();

    return i;
}

// Send ACK or NACK
static void cm_AckNak(uint8_t Ack)
{
	CM_DATA_HI;
    cm_Clocklow();
    if (Ack) {
        CM_DATA_LO;               // ACK
    } else {
        CM_DATA_HI;               //  NACK
    }
    cm_Delay(2);
    cm_Clockhigh();
    cm_Delay(8);
    cm_Clocklow();
}


// Reads one byte from the bus
static uint8_t cm_Read(void)
{
    uint8_t i;
    uint8_t rByte = 0;

    CM_DATA_HI;
    for(i=0x80; i; i=i>>1)
    {
        cm_ClockCycle();
        if (CM_DATA_RD) rByte |= i;
        cm_Clocklow();
    }
    CM_DATA_HI;
    return rByte;
}

// Wait for ACK polling sequence - using command 0xB6 (System Read)
//
// timeout in ms
static uint8_t cm_WaitAckPolling(uint8_t timeout)
{
	uint32_t start = timer_ms();

	while (!timer_expired(start + timeout)) {
		cm_Start();
		if (cm_Write(CM_CMD_SYSTEMRD) == 0)
			return CM_SUCCESS;
	}
	return CM_ACK_POLL_FAIL;
}


// Send a command (4 bytes)
static uint8_t cm_SendCommand(uint8_t *InsBuff)
{
    uint8_t i, Cmd;

    i = 100;
    Cmd = (InsBuff[0]&0x0F) | 0xb0;

    while (i) {
        cm_Start();
        if (cm_Write(Cmd) == 0)
            break;

        if (--i == 0)
            return CM_FAIL_CMDSTART;
    }

    for(i = 1; i< 4; i++) {
        if (cm_Write(InsBuff[i]) != 0)
            return CM_FAIL_CMDSEND;
    }

    return CM_SUCCESS;
}


//  Receive data
static uint8_t cm_ReceiveData(uint8_t *RecBuf, uint8_t Len)
{
    int i;
    for(i = 0; i < (Len-1); i++) {
        RecBuf[i] = cm_Read();
        cm_AckNak(TRUE);
    }
    RecBuf[i] = cm_Read();
    cm_AckNak(FALSE);
    cm_Stop();
    return CM_SUCCESS;
}

// send data
static uint8_t cm_SendData(const uint8_t *SendBuf, uint8_t Len)
{
    int i;
    for(i = 0; i< Len; i++) {
        if (cm_Write(SendBuf[i]) != CM_SUCCESS)
            return CM_FAIL_WRDATA;
    }
    cm_Stop();

    return CM_SUCCESS;
}


// Send a byte command
static uint8_t cm_SendCmdByte(uint8_t Command)
{
    uint8_t i, Cmd;

    i = 100;

    Cmd = (Command & 0x0F) | 0xb0;
    while (i) {
      cm_Start();
        if (cm_Write(Cmd) == 0)
            break;
        if (--i == 0)
            return CM_FAIL_CMDSTART;
    }

    return CM_SUCCESS;
}

void cm_PowerOn(void)
{
    cm_ResetCrypto();
    CM_UserZone = CM_AntiTearing = 0;

    // Initialize the bus
    CM_DATA_OUT;
    CM_CLK_OUT;
    CM_CLK_LO;
    CM_DATA_HI;
    // Give a certain number of clocks to initialize the bus
    cm_ClockCycles(CM_PWRON_CLKS);
}


// Start reading command
static uint8_t cm_ReadCommand(uint8_t * InsBuff, uint8_t * RetVal, uint8_t Len)
{
	uint8_t Return;
	int i;

	for (i = 0; i < 20; i++) {
		if ((Return = cm_SendCommand(InsBuff)) != CM_SUCCESS)
			continue;
		else
			break;
	}

	if (i >= 20) {
		return Return;
	}

	return cm_ReceiveData(RetVal, Len);
}




// Initiate write command
static uint8_t cm_WriteCommand(uint8_t * InsBuff, const uint8_t * SendVal,
		uint8_t Len)
{
	uint8_t Return;
	if ((Return = cm_SendCommand(InsBuff)) != CM_SUCCESS) {
		return Return;
	}

	return cm_SendData(SendVal, Len);
}






/****************************************************************************
                                         interface functions
****************************************************************************/

// Set (select) the user area
uint8_t cm_SetUserZone(uint8_t ZoneNumber, uint8_t AntiTearing)
{
	uint8_t Return;
	uint8_t CmdSetUserZone[4] = { CM_CMD_SYSTEMWR, 0x03, ZoneNumber, 0x00 };

	if (AntiTearing)
		CmdSetUserZone[1] = 0x0b;

	cm_GPAGen(ZoneNumber);

	if ((Return = cm_SendCommand(CmdSetUserZone)) != CM_SUCCESS) {
		return Return;
	}

	// Save the global variables
	CM_UserZone = ZoneNumber;
	CM_AntiTearing = AntiTearing;

	return CM_SUCCESS;
}



// Burn fuse
uint8_t cm_BurnFuse(uint8_t Fuse)
{
	uint8_t Return;
	uint8_t CmdWrFuse[4] = { CM_CMD_SYSTEMWR, 0x01, Fuse, 0x00 };

	if ((Return = cm_SendCommand(CmdWrFuse)) != CM_SUCCESS) {
		return Return;
	}

	/* ACK polling for 5ms - see table 8-2 */
	if ((Return = cm_WaitAckPolling(5)) != CM_SUCCESS)
		return Return;

	return CM_SUCCESS;
}


// Read the configuration area
uint8_t cm_ReadConfigZone(uint8_t CryptoAddr, uint8_t * Buffer, uint8_t Count)
{
	uint8_t Return, Encrypt;
	uint8_t CmdReadConfigZone[4] = { CM_CMD_SYSTEMRD, 0x00, CryptoAddr, Count };

	// The parameters need to be added to the polynomial
	cm_GPAcmd2(CmdReadConfigZone);

	// Read operation
	if ((Return = cm_ReadCommand(CmdReadConfigZone, Buffer, Count))
			!= CM_SUCCESS) {
		return Return;
	}

	// Password area is encrypted, read out need to decrypt
	Encrypt = ((CryptoAddr >= CM_PSW_ADDR) && CM_Encrypt);

	// Decrypt
	cm_GPAdecrypt(Encrypt, Buffer, Count);

	return CM_SUCCESS;
}



// Read checksum
//
// may drop authentication (depending on DCR UCR)
uint8_t cm_ReadChecksum(uint8_t * ChkSum)
{
	uint8_t DCR[1];
	uint8_t Return;
	uint8_t CmdRdChk[4] = { CM_CMD_SYSTEMRD, 0x02, 0x00, 0x02 };

	cm_GPAGenN(0, 20);

	// Read operation
	if ((Return = cm_ReadCommand(CmdRdChk, ChkSum, 2)) != CM_SUCCESS) {
		return Return;
	}

	// Check if the read checksum is limited
	if ((Return = cm_ReadConfigZone(DCR_ADDR, DCR, 1)) != CM_SUCCESS) {
		return Return;
	}

	if ((DCR[0] & DCR_UCR)) {
		cm_ResetCrypto();
	}

	return CM_SUCCESS;
}

// Read user space
// cm_ReadUserZone  device for reading user space less than 256 Bytes (at88sc0104)

uint8_t cm_ReadUserZone(uint8_t CryptoAddr, uint8_t * Buffer, uint8_t Count)
{
	uint8_t Return;
	uint8_t CmdReadUserZone[4] = { CM_CMD_RDUSERZONE, 0x00, CryptoAddr, Count };

	//Add polynomial parameters
	cm_GPAcmd2(CmdReadUserZone);

	//Read operation
	if ((Return = cm_ReadCommand(CmdReadUserZone, Buffer, Count))
			!= CM_SUCCESS) {
		return Return;
	}

	//If encrypted, it needs to be decrypted
	cm_GPAdecrypt(CM_Encrypt, Buffer, Count);

	return CM_SUCCESS;
}


// write user space
// cm_WriteUserZone device for writing user space less than 256 Bytes (at88sc0104)

uint8_t cm_WriteUserZone(uint8_t CryptoAddr, const uint8_t * Buffer, uint8_t Count)
{
	uint8_t Return;
	uint8_t CmdWriteUserZone[4] = { CM_CMD_WRUSERZONE, 0x00, CryptoAddr, Count };

	//Add polynomial parameters
	cm_GPAcmd2(CmdWriteUserZone);

	// cm_GPAencrypt() modifies the buffer potentially, make a copy
	uint8_t buf2[Count];
	memcpy( buf2, Buffer, Count);
	//If we encrypt, you need to encrypt the data
	cm_GPAencrypt(CM_Encrypt, buf2, Count);

	// Write data
	Return = cm_WriteCommand(CmdWriteUserZone, buf2, Count);

	memzero(buf2, Count);
	/* ACK polling for 5ms - see table 8-2 */
	/* If anti-tearing is used, you need to wait 20ms */
	if ((Return = cm_WaitAckPolling(CM_AntiTearing ? 20 : 5)) != CM_SUCCESS)
		return Return;

	return Return;
}


// Read the fuse
uint8_t cm_ReadFuse(uint8_t * Fuse)
{
	uint8_t Return;
	uint8_t CmdRdFuse[4] = { CM_CMD_SYSTEMRD, 0x01, 0x00, 0x01 };

	cm_GPAGenN(0, 11);
	cm_GPAGen(0x01);

	if ((Return = cm_ReadCommand(CmdRdFuse, Fuse, 1)) != CM_SUCCESS) {
		return Return;
	}

	cm_GPAGen(*Fuse);
	cm_GPAGenN(0, 5);

	return CM_SUCCESS;

}

/* check the available password access attempts, 0 means locked */
uint8_t cm_CheckPAC(uint8_t Set, uint8_t rw, uint8_t *attempts)
{
	uint8_t PAC;
	uint8_t Return;

	if ((Return = cm_ReadConfigZone(
			CM_PSW_ADDR + (Set << 3) + ((rw != CM_PWWRITE) ? 4 : 0), &PAC, 1))
			!= CM_SUCCESS) {
		return Return;
	}

	if (!attempts)
		return CM_FAILED;

	/* decode PAC for the usual 4 attempts possible (assume DCR bit ETA not set) */
	switch (PAC) {
	case 0xFF:
		*attempts = 4;
		break;
	case 0xEE:
		*attempts = 3;
		break;
	case 0xCC:
		*attempts = 2;
		break;
	case 0x88:
		*attempts = 1;
		break;
	case 0x00:
		*attempts = 0;
		break;
	default:
		// weird PAC usually means that we lost the authentication/encryption
		return CM_DEAUTH;
	}
	return CM_SUCCESS;
}

// Verification secure code (Write 7 Password)
uint8_t cm_VerifySecurePasswd(const uint8_t *passwd)
{
	uint8_t CmdVerifySecPW[4] = { CM_CMD_VERIFYPWD, 0x07, 0x00, 0x03 }; /* Write 7 Password */
	uint8_t Return;

	if ((Return = cm_WriteCommand(CmdVerifySecPW, passwd, 3)) != CM_SUCCESS)
		return Return;

	/* ACK polling for 10ms - see table 8-2 */
	Return = cm_WaitAckPolling(10);

	if (Return == CM_SUCCESS) {
		uint8_t attempts;
		if ((Return = cm_CheckPAC(7, CM_PWWRITE, &attempts)) != CM_SUCCESS)
			return Return;
		if (attempts != 4) {
			Return = CM_FAIL_PwD_NOK;
		}
	}

	return Return;

}


// Password Authentication
uint8_t cm_VerifyPassword(const uint8_t * Password, uint8_t Set, uint8_t rw)
{
	uint8_t j;
	uint8_t Return;
	uint8_t CmdPassword[4] = { CM_CMD_VERIFYPWD, Set, 0x00, 0x03 };
	uint8_t pwd[3];

	// Processing encryption
	if (CM_Authenticate) {
		for (j = 0; j < 3; j++) {
			pwd[j] = cm_GPAGenN(Password[j], 5);
		}
	} else {
		// Do not use encryption
		memcpy(pwd, Password, 3);
	}

	// Initiate certification

	if ((Return = cm_WriteCommand(CmdPassword, pwd, 3)) != CM_SUCCESS) {
		if (Return == CM_FAIL_CMDSEND)
			// if a section is already blocked, because PAC=0, we get CM_FAIL_CMDSEND here
			return CM_PWD_NOK_LOCKED;
		return Return;
	}
	/* ACK polling for 10ms - see table 8-2 */
	Return = cm_WaitAckPolling(10);

	// Read PAC
	if (Return == CM_SUCCESS) {
		uint8_t attempts;
		Return = cm_CheckPAC(Set, rw, &attempts);
		if (Return == CM_DEAUTH) {
			// sometimes reading the PAC yields useless data. Turn off security and retry
			if (CM_Authenticate) {
				cm_DeactivateSecurity();
			}
			if ((Return = cm_CheckPAC(Set, rw, &attempts) ) != CM_SUCCESS)
					return Return;
		} else if (Return != CM_SUCCESS)
			return Return;
		if (attempts == 0) {
			Return = CM_PWD_NOK_LOCKED;
		} else if (attempts != 4) {
			Return = CM_FAIL_PwD_NOK;
		}
	}

	if (CM_Authenticate && (Return != CM_SUCCESS)) {
		cm_ResetCrypto();
	}

	return Return;
}


// Write configuration area
uint8_t cm_WriteConfigZone(uint8_t CryptoAddr, const uint8_t * Buffer, uint8_t Count,
		uint8_t AntiTearing)
{
	uint8_t Return, Encrypt;
	uint8_t CmdWriteConfigZone[4] = { CM_CMD_SYSTEMWR, 0x00, CryptoAddr, Count };

	if (AntiTearing)
		CmdWriteConfigZone[1] = 0x08;

	// The parameter is added to the polynomial
	cm_GPAcmd2(CmdWriteConfigZone);

	// The password area is encrypted
	Encrypt = ((CryptoAddr >= CM_PSW_ADDR) && CM_Encrypt);

	// cm_GPAencrypt() modifies the buffer potentially, make a copy
	uint8_t buf2[Count];
	memcpy( buf2, Buffer, Count);
	// Encrypt data
	cm_GPAencrypt(Encrypt, buf2, Count);

	// Write operation
	Return = cm_WriteCommand(CmdWriteConfigZone, buf2, Count);

	memzero( buf2, Count);

	if (Return != CM_SUCCESS)
		return Return;

	/* ACK polling for 5ms - see table 8-2 */
	/* If anti-tearing is used, you need to wait 20ms */
	if ((Return = cm_WaitAckPolling(AntiTearing ? 20 : 5)) != CM_SUCCESS)
		return Return;

	return Return;
}


// Send checksum
uint8_t cm_SendChecksum(uint8_t * ChkSum)
{
	uint8_t Return;
	uint8_t localChkSum[2];
	uint8_t CmdWrChk[4] = { CM_CMD_SYSTEMWR, 0x02, 0x00, 0x02 };
	// Get the checksum
	if (ChkSum == NULL) {
		cm_CalChecksum(localChkSum);
	} else {
		localChkSum[0] = *ChkSum++;
		localChkSum[1] = *ChkSum;
	}

	// send
	Return = cm_WriteCommand(CmdWrChk, localChkSum, 2);

	/* ACK polling for 5ms - see table 8-2 */
	/* If anti-tearing is used, you need to wait 20ms */
	if ((Return = cm_WaitAckPolling(CM_AntiTearing ? 20 : 5)) != CM_SUCCESS)
		return Return;

	return Return;
}




// Deactivate authentication and encryption and reset crypto
uint8_t cm_DeactivateSecurity(void)
{
	uint8_t Return;

	if ((Return = cm_SendCmdByte(CM_CMD_VERIFYCRYPTO)) != CM_SUCCESS)
		return Return;
	cm_WaitAckPolling(10);

	cm_ResetCrypto();

	return CM_SUCCESS;
}




/****************************************************************************
                                             CryptoMemory low level interface
****************************************************************************/

/* state machine registers implementing the crypto */
CONFIDENTIAL static struct {
	uint8_t	R[7];
	uint8_t S[7];
	uint8_t T[5];
	uint8_t out;
} Gpa;


// Reset Password
uint8_t cm_ResetPassword(void)
{
	cm_SendCmdByte(CM_CMD_VERIFYPWD);
	cm_WaitAckPolling(10);
	return CM_SUCCESS;
}


/***************************************************************************
            Algorithm-related functions
****************************************************************************/

// Reset algorithm related variables
static void cm_ResetCrypto(void)
{
	memzero(&Gpa, sizeof(Gpa));
	CM_Encrypt = CM_Authenticate = FALSE;
}

// Generate next value
static uint8_t cm_GPAGen(uint8_t Datain)
{
	uint8_t Din_gpa;
	uint8_t Ri, Si, Ti;
	uint8_t R_sum, S_sum, T_sum;

#define CM_MOD_R (0x1F)
#define CM_MOD_T (0x1F)
#define CM_MOD_S (0x7F)

#define cm_Mod(x,y,m) ( (x+y)>m ?(x+y-m) : (x+y) )
#define cm_RotR(x)    ( ((x & 0x0F)<<1) | ((x & 0x10)>>4) )
#define cm_RotS(x)    ( ((x & 0x3F)<<1) | ((x & 0x40)>>6) )
#define cm_RotT(x)    ( ((x & 0x0F)<<1) | ((x & 0x10)>>4) )

	// Input Character
	Din_gpa = Datain ^ Gpa.out;
	Ri = Din_gpa & 0x1F;			 	// Ri[4:0] = Din_gpa[4:0]
	Si = ((Din_gpa & 0x0F) << 3) | ((Din_gpa & 0xE0) >> 5); // Si[6:0] = { Din_gpa[3:0], Din_gpa[7:5] }
	Ti = (Din_gpa & 0xF8) >> 3; 		// Ti[4:0] = Din_gpa[7:3];

	//R polynomial
	R_sum = cm_Mod(Gpa.R[3], cm_RotR(Gpa.R[6]), CM_MOD_R);
	memmove(&Gpa.R[1], &Gpa.R[0], 6);
	Gpa.R[3] ^= Ri;
	Gpa.R[0] = R_sum;

	//S polynomial
	S_sum = cm_Mod(Gpa.S[5], cm_RotS(Gpa.S[6]), CM_MOD_S);
	memmove(&Gpa.S[1], &Gpa.S[0], 6);
	Gpa.S[5] ^= Si;
	Gpa.S[0] = S_sum;

	//T polynomial
	T_sum = cm_Mod(Gpa.T[4], Gpa.T[2], CM_MOD_T);
	memmove(&Gpa.T[1], &Gpa.T[0], 4);
	Gpa.T[2] ^= Ti;
	Gpa.T[0] = T_sum;

	// Output Stage
	Gpa.out = ((Gpa.out << 4) & 0xF0) |	// shift previous nibble left
			(((((Gpa.R[0] ^ Gpa.R[4]) & 0x0F) & (~Gpa.S[0]))
					| ((((Gpa.T[0] ^ Gpa.T[3]) & 0x0F) & Gpa.S[0]))) & 0x0F); // and concatenate 4 new bits selected by Si
	return Gpa.out;
}


/*!
 *
 * \brief 	Do authenticate/encrypt challenge encryption
 *
 * \note Ci and G_Sk are updated
 *
 * \param	Ci: cryptogram C_i - code will compute C_i+1
 * \param	G_Sk: session encryption key S_k
 * \param	Q: 8 random bytes
 * \param	Ch: 8 bytes computed challenge from
 */
static void cm_AuthEncryptCalc(uint8_t *Ci, uint8_t *G_Sk, uint8_t *Q,
		uint8_t *Ch)
{
	uint8_t i, j;

	// Reset all registers
	memzero(&Gpa, sizeof(Gpa));

	// Setup the cryptographic registers
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++)
			cm_GPAGen(Ci[2 * j]);
		for (i = 0; i < 3; i++)
			cm_GPAGen(Ci[2 * j + 1]);
		cm_GPAGen(Q[j]);
	}

	for (j = 0; j < 4; j++) {
		for (i = 0; i < 3; i++)
			cm_GPAGen(G_Sk[2 * j]);
		for (i = 0; i < 3; i++)
			cm_GPAGen(G_Sk[2 * j + 1]);
		cm_GPAGen(Q[j + 4]);
	}

	// begin to generate Ch
	Ch[0] = cm_GPAGenN(0, 6);		// 6 0x00s

	for (j = 1; j < 8; j++) {
		Ch[j] = cm_GPAGenN(0, 7);	// 7 0x00s
	}

	// then calculate new Ci and Sk, to compare with the new Ci and Sk read from eeprom
	Ci[0] = 0xff;					// reset AAC
	for (j = 1; j < 8; j++) {
		Ci[j] = cm_GPAGenN(0, 2);	// 2 0x00s
	}

	for (j = 0; j < 8; j++) {
		G_Sk[j] = cm_GPAGenN(0, 2);	// 2 0x00s
	}

	cm_GPAGenN(0, 3);				// 3 0x00s
}

// Calculate Checksum
static void cm_CalChecksum(uint8_t *Ck_sum)
{
	Ck_sum[0] = cm_GPAGenN(0, 15);  // 15 0x00s
	Ck_sum[1] = cm_GPAGenN(0, 5);   // 5 0x00s
}

// Clock some data into the state machine
static uint8_t cm_GPAGenN(uint8_t Datain, uint8_t Count)
{
	while (--Count)
		cm_GPAGen(Datain);
	return cm_GPAGen(Datain);
}

// Include 2 bytes of a command into the polynomials
static void cm_GPAcmd2(uint8_t *InsBuff)
{
	cm_GPAGenN(0, 5);
	cm_GPAGen(InsBuff[2]);
	cm_GPAGenN(0, 5);
	cm_GPAGen(InsBuff[3]);
}

// Include the data in the polynomials and decrypt if required
static void cm_GPAdecrypt(uint8_t Encrypt, uint8_t *Buffer, uint8_t Count)
{
	uint8_t i;

	for (i = 0; i < Count; ++i) {
		if (Encrypt)
			Buffer[i] = Buffer[i] ^ Gpa.out;
		cm_GPAGen(Buffer[i]);
		cm_GPAGenN(0, 5);        // 5 clocks with 0x00 data
	}
}

// Include the data in the polynomials and encrypt if required
static void cm_GPAencrypt(uint8_t Encrypt, uint8_t *Buffer, uint8_t Count)
{
	uint8_t i, Data;

	for (i = 0; i < Count; i++) {
		cm_GPAGenN(0, 5);                          // 5 0x00s
		Data = Buffer[i];
		if (Encrypt)
			Buffer[i] = Buffer[i] ^ Gpa.out;
		cm_GPAGen(Data);
	}
}

#endif /* CRYPTOMEM */

