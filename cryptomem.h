/*
 * cryptomem.h
 *
 *  Interface to cryptomem
 *
 */

#ifndef CRYPTOMEM_H_
#define CRYPTOMEM_H_

enum { CM_SUCCESS = 0, CM_FAILED=1, CM_FAIL_CMDSTART, CM_FAIL_CMDSEND, CM_FAIL_WRDATA, CM_FAIL_RDDATA,
	CM_FAIL_PwD_NOK, CM_ACK_POLL_FAIL, CM_ALREADY_PGMD };

uint8_t cm_init_manufacturing(uint8_t seed[4][8]);
uint8_t cm_check_programming(uint8_t seed[4][8]);
uint8_t cm_prodtest(void);

uint8_t cm_get_seed_in_OTP(uint8_t *seed, uint8_t index );
uint8_t cm_store_seed_in_OTP(uint8_t seed[4][8]);

#endif /* CRYPTOMEM_H_ */
