#ifndef __AT88SC0104_H__
#define __AT88SC0104_H__

/*****************************************************************
 Return value definition
 ****************************************************************/
enum { CM_SUCCESS = 0, CM_FAILED=1, CM_FAIL_CMDSTART, CM_FAIL_CMDSEND, CM_FAIL_WRDATA, CM_FAIL_RDDATA,
	CM_FAIL_PwD_NOK, CM_DEAUTH, CM_ACK_POLL_FAIL, CM_ALREADY_PGMD, CM_DEFAULT_Pw_NOK };

/*****************************************************************
 interface functions
 ****************************************************************/
uint8_t cm_ActivateSecurity(uint8_t KeySet, uint8_t *Key, uint8_t *Random, uint8_t Encrypt);
uint8_t cm_DeactivateSecurity(void);
uint8_t cm_VerifyPassword(const uint8_t *Password, uint8_t Set, uint8_t RW);
uint8_t cm_CheckPAC(uint8_t Set, uint8_t rw, uint8_t *attemtps);
uint8_t cm_VerifySecurePasswd(const uint8_t *passwd);
uint8_t cm_ResetPassword(void);
uint8_t cm_ReadConfigZone(uint8_t CryptoAddr, uint8_t *Buffer, uint8_t Count);
uint8_t cm_WriteConfigZone(uint8_t CryptoAddr, const uint8_t *Buffer, uint8_t Count, uint8_t AntiTearing);
uint8_t cm_SetUserZone(uint8_t ZoneNumber, uint8_t AntiTearing);
uint8_t cm_ReadUserZone(uint8_t CryptoAddr, uint8_t *Buffer, uint8_t Count);
uint8_t cm_WriteUserZone(uint8_t CryptoAddr, const uint8_t *Buffer, uint8_t Count);
uint8_t cm_SendChecksum(uint8_t *ChkSum);
uint8_t cm_ReadChecksum(uint8_t *ChkSum);
uint8_t cm_ReadFuse(uint8_t *Fuze);
uint8_t cm_BurnFuse(uint8_t Fuze);
uint8_t cm_aCommunicationTest(void);
void cm_PowerOn(void);

enum { CM_PWREAD = 1, CM_PWWRITE = 0 };

/*****************************************************************
           Partial Configuration Area Register Definition
 ****************************************************************/
#define DCR_ADDR      (0x18)
#define DCR_SME       (0x80)
#define DCR_UCR       (0x40)
#define DCR_UAT       (0x20)
#define DCR_ETA       (0x10)
#define DCR_CS        (0x0F)

#define CM_AR_ADDR	  (0x20)
#define CM_PR_ADDR	  (0x21)

#define CM_CI_ADDR    (0x50)
#define CM_Sk_ADDR    (0x58)
#define CM_G_ADDR     (0x90)

#define CM_FAB        (0x06)
#define CM_CMA        (0x04)
#define CM_PER        (0x00)

#define CM_PSW_ADDR   (0xB0)


#endif    /* __AT88SC0104_H__ */
