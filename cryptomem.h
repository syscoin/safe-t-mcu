/*
 * cryptomem.h
 *
 *  Interface to cryptomem
 *
 */

#ifndef CRYPTOMEM_H_
#define CRYPTOMEM_H_

enum { CM_SUCCESS = 0, CM_FAILED=1, CM_FAIL_CMDSTART, CM_FAIL_CMDSEND, CM_FAIL_WRDATA, CM_FAIL_RDDATA,
	CM_FAIL_PwD_NOK, CM_ACK_POLL_FAIL, CM_ALREADY_PGMD, CM_DEFAULT_Pw_NOK };

#define CM_DEFAULT_PW 0xFFFFFF
uint8_t cm_init_manufacturing(uint8_t seed[4][8]);
uint8_t cm_check_programming(uint8_t seed[4][8]);
uint8_t cm_prodtest_communication_test(void);
uint8_t cm_prodtest_initialization(void);

void cm_init( void );

uint8_t cm_get_seed_in_OTP(uint8_t *seed, uint8_t index );
uint8_t cm_store_seed_in_OTP(uint8_t seed[4][8]);

int8_t cm_get_aes_key( uint8_t *key );
int8_t cm_set_PIN(uint32_t pw);
int8_t cm_open_zone(uint32_t pw);
int8_t cm_initialize_new_zone( void );
int8_t cm_deactivate_security( void );
int8_t cm_get_remaining_PIN_attempts(void);
#endif /* CRYPTOMEM_H_ */
