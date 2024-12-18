/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <psa/crypto.h>

#include "test_vectors.h"

uint8_t key[32] = {
	0x31, 0x4b, 0x66, 0x77, 0x19, 0x39, 0x73, 0x17, 0x7d, 0xaf, 0x98,
	0x3d, 0x5d, 0x31, 0x26, 0x02, 0x31, 0x4b, 0x66, 0x77, 0x19, 0x39,
	0x73, 0x17, 0x7d, 0xaf, 0x98, 0x3d, 0x5d, 0x31, 0x26, 0x02,
};

uint8_t ciphertext[sizeof(plaintext)];
uint8_t decrypted[sizeof(ciphertext)];
size_t ciphertext_len;
size_t decrypted_len;

ZTEST(psa_crypto_test, test_cipher_aes_cbc_256)
{
	psa_key_id_t key_id;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t alg = PSA_ALG_CBC_NO_PADDING;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	uint8_t iv[PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)];
	size_t iv_len;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, alg);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);
	if (IS_ENABLED(TEST_WRAPPED_KEYS)) {
		psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							  PSA_KEY_PERSISTENCE_VOLATILE, 1));
		zassert_equal(psa_generate_key(&attributes, &key_id), PSA_SUCCESS,
			      "Failed to generate key");
	} else {
		zassert_equal(psa_import_key(&attributes, key, sizeof(key), &key_id), PSA_SUCCESS,
			      "Failed to import key");
	}
	psa_reset_key_attributes(&attributes);

	zassert_equal(psa_cipher_encrypt_setup(&operation, key_id, alg), PSA_SUCCESS,
		      "Failed to begin encrypt operation");
	zassert_equal(psa_cipher_generate_iv(&operation, iv, sizeof(iv), &iv_len), PSA_SUCCESS,
		      "Failed to generate IV");
	zassert_equal(psa_cipher_update(&operation, plaintext, sizeof(plaintext), ciphertext,
					sizeof(ciphertext), &ciphertext_len),
		      PSA_SUCCESS, "Failed to update encrypt operation");
	zassert_equal(psa_cipher_finish(&operation, ciphertext + ciphertext_len,
					sizeof(ciphertext) - ciphertext_len, &ciphertext_len),
		      PSA_SUCCESS, "Failed to finish encrypt operation");

	psa_cipher_abort(&operation);

	zassert(memcmp(ciphertext, plaintext, sizeof(plaintext)) != 0,
		"Ciphertext identical to plaintext");

	zassert_equal(psa_cipher_decrypt_setup(&operation, key_id, alg), PSA_SUCCESS,
		      "Failed to begin decrypt operation");
	zassert_equal(psa_cipher_set_iv(&operation, iv, sizeof(iv)), PSA_SUCCESS,
		      "Failed to set IV");
	zassert_equal(psa_cipher_update(&operation, ciphertext, sizeof(ciphertext), decrypted,
					sizeof(decrypted), &decrypted_len),
		      PSA_SUCCESS, "Failed to update decrypt operation");
	zassert_equal(psa_cipher_finish(&operation, decrypted + decrypted_len,
					sizeof(decrypted) - decrypted_len, &decrypted_len),
		      PSA_SUCCESS, "Failed to finish decrypt operation");

	psa_cipher_abort(&operation);
	psa_destroy_key(key_id);

	zassert_mem_equal(decrypted, plaintext, sizeof(plaintext));
}
