#ifndef __AT88SC0104_H__
#define __AT88SC0104_H__

/*****************************************************************
 Return value definition
 ****************************************************************/
enum { CM_SUCCESS = 0, CM_FAILED=1, CM_FAIL_CMDSTART, CM_FAIL_CMDSEND, CM_FAIL_WRDATA, CM_FAIL_RDDATA, CM_FAIL_PwD_NOK, CM_ACK_POLL_FAIL };

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

uint8_t cm_init_manufacturing(uint8_t seed[4][8]);


#endif    /* __AT88SC0104_H__ */
