#include <stdio.h>
#include <string.h>
#include <psa/crypto.h>
#include "bip39.h"

static CONFIDENTIAL char mnemonic[24 * 10] = {0};

const char *mnemonic_from_data(const uint8_t *data, int strength)
{
	if (strength % 32 || strength < 128 || strength > 256)
	{
		return "";
	}

	int len = strength / 8;

	uint8_t bits[32 + 1] = {0};

	psa_status_t status = psa_crypto_init();

	if (status == PSA_SUCCESS)
	{
		size_t hash_length;
		status = psa_hash_compute(PSA_ALG_SHA_256, data, len, bits, sizeof(bits), &hash_length);
	}

	if (status != PSA_SUCCESS)
	{
		return "";
	}

	char *output_ptr = mnemonic;

	bits[len] = bits[0];

	memcpy(bits, data, len);

	int total_words = len * 3 / 4;

	int current_word = 0, bit_position = 0, word_index = 0;

	for (current_word = 0; current_word < total_words; current_word++)
	{
		word_index = 0;
		for (bit_position = 0; bit_position < 11; bit_position++)
		{
			word_index <<= 1;
			word_index += (bits[(current_word * 11 + bit_position) / 8] & (1 << (7 - ((current_word * 11 + bit_position) % 8)))) > 0;
		}
		strcpy(output_ptr, BIP39_WORD_LIST_ENGLISH[word_index]);
		output_ptr += strlen(BIP39_WORD_LIST_ENGLISH[word_index]);
		*output_ptr = (current_word < total_words - 1) ? ' ' : 0;
		output_ptr++;
	}

	return mnemonic;
}

int mnemonic_to_seed(const char *mnemonic, const char *passphrase,
		     uint8_t seed[512 / 8])
{
	// Init salt
	uint8_t salt[8 + 256] = {"mnemonic"};
	// Add use salt
	memcpy(salt + 8, passphrase, strlen(passphrase));

	// Init PAS Crypto
	psa_status_t status = psa_crypto_init();
	// Init PSA operation
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

	// Setup add alg
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_setup(&operation, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_512));
	}
	// Add rounds
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_input_integer(&operation, PSA_KEY_DERIVATION_INPUT_COST, BIP39_PBKDF2_ROUNDS);
	}
	// Add salt
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT,
							salt, strlen(salt));
	}
	// Add mnemonic
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
							mnemonic, strlen(mnemonic));
	}
	// Add maximum capacity
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_set_capacity(&operation, 512 / 8);
	}
	// Generate seed
	if (status == PSA_SUCCESS)
	{
		status = psa_key_derivation_output_bytes(&operation, seed, 512 / 8);
	}

	return 0;
}