#include "stdafx.h"
#include "SecureCipher.h"

// Static initialization flag for libsodium
static bool s_sodiumInitialized = false;

static bool EnsureSodiumInit()
{
    if (!s_sodiumInitialized)
    {
        if (sodium_init() < 0)
        {
            return false;
        }
        s_sodiumInitialized = true;
    }
    return true;
}

SecureCipher::SecureCipher()
{
    sodium_memzero(m_pk, sizeof(m_pk));
    sodium_memzero(m_sk, sizeof(m_sk));
    sodium_memzero(m_tx_key, sizeof(m_tx_key));
    sodium_memzero(m_rx_key, sizeof(m_rx_key));
    sodium_memzero(m_session_token, sizeof(m_session_token));
}

SecureCipher::~SecureCipher()
{
    CleanUp();
}

bool SecureCipher::Initialize()
{
    if (!EnsureSodiumInit())
    {
        return false;
    }

    // Generate X25519 keypair
    if (crypto_kx_keypair(m_pk, m_sk) != 0)
    {
        return false;
    }

    m_tx_nonce = 0;
    m_rx_nonce = 0;
    m_initialized = true;
    m_activated = false;

    return true;
}

void SecureCipher::CleanUp()
{
    // Securely erase all sensitive key material
    sodium_memzero(m_sk, sizeof(m_sk));
    sodium_memzero(m_tx_key, sizeof(m_tx_key));
    sodium_memzero(m_rx_key, sizeof(m_rx_key));
    sodium_memzero(m_session_token, sizeof(m_session_token));

    m_initialized = false;
    m_activated = false;
    m_tx_nonce = 0;
    m_rx_nonce = 0;
}

void SecureCipher::GetPublicKey(uint8_t* out_pk) const
{
    memcpy(out_pk, m_pk, PK_SIZE);
}

bool SecureCipher::ComputeClientKeys(const uint8_t* server_pk)
{
    if (!m_initialized)
    {
        return false;
    }

    // Client: tx_key is for sending TO server, rx_key is for receiving FROM server
    if (crypto_kx_client_session_keys(m_rx_key, m_tx_key, m_pk, m_sk, server_pk) != 0)
    {
        return false;
    }

    return true;
}

bool SecureCipher::ComputeServerKeys(const uint8_t* client_pk)
{
    if (!m_initialized)
    {
        return false;
    }

    // Server: tx_key is for sending TO client, rx_key is for receiving FROM client
    if (crypto_kx_server_session_keys(m_rx_key, m_tx_key, m_pk, m_sk, client_pk) != 0)
    {
        return false;
    }

    return true;
}

void SecureCipher::GenerateChallenge(uint8_t* out_challenge)
{
    randombytes_buf(out_challenge, CHALLENGE_SIZE);
}

void SecureCipher::ComputeChallengeResponse(const uint8_t* challenge, uint8_t* out_response)
{
    // HMAC the challenge using our rx_key as the authentication key
    // This proves we derived the correct shared secret
    crypto_auth(out_response, challenge, CHALLENGE_SIZE, m_rx_key);
}

bool SecureCipher::VerifyChallengeResponse(const uint8_t* challenge, const uint8_t* response)
{
    // Verify the HMAC - peer should have used their tx_key (our rx_key) to compute it
    return crypto_auth_verify(response, challenge, CHALLENGE_SIZE, m_rx_key) == 0;
}

void SecureCipher::BuildNonce(uint8_t* nonce, uint64_t counter, bool is_tx)
{
    // 24-byte nonce structure:
    // [0]:     direction flag (0x01 for tx, 0x02 for rx)
    // [1-7]:   reserved/zero
    // [8-15]:  64-bit counter (little-endian)
    // [16-23]: reserved/zero

    sodium_memzero(nonce, NONCE_SIZE);
    nonce[0] = is_tx ? 0x01 : 0x02;

    // Store counter in little-endian at offset 8
    for (int i = 0; i < 8; ++i)
    {
        nonce[8 + i] = (uint8_t)(counter >> (i * 8));
    }
}

size_t SecureCipher::Encrypt(const void* plaintext, size_t plaintext_len, void* ciphertext)
{
    if (!m_activated)
    {
        return 0;
    }

    uint8_t nonce[NONCE_SIZE];
    BuildNonce(nonce, m_tx_nonce, true);

    unsigned long long ciphertext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            (uint8_t*)ciphertext, &ciphertext_len,
            (const uint8_t*)plaintext, plaintext_len,
            nullptr, 0,  // No additional data
            nullptr,     // No secret nonce
            nonce,
            m_tx_key) != 0)
    {
        return 0;
    }

    ++m_tx_nonce;
    return (size_t)ciphertext_len;
}

size_t SecureCipher::Decrypt(const void* ciphertext, size_t ciphertext_len, void* plaintext)
{
    if (!m_activated)
    {
        return 0;
    }

    if (ciphertext_len < TAG_SIZE)
    {
        return 0;
    }

    uint8_t nonce[NONCE_SIZE];
    BuildNonce(nonce, m_rx_nonce, false);

    unsigned long long plaintext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            (uint8_t*)plaintext, &plaintext_len,
            nullptr,  // No secret nonce output
            (const uint8_t*)ciphertext, ciphertext_len,
            nullptr, 0,  // No additional data
            nonce,
            m_rx_key) != 0)
    {
        // Decryption failed - either wrong key, tampered data, or replay attack
        return 0;
    }

    ++m_rx_nonce;
    return (size_t)plaintext_len;
}

void SecureCipher::EncryptInPlace(void* buffer, size_t len)
{
    if (!m_activated || len == 0)
        return;

    uint8_t nonce[NONCE_SIZE];
    BuildNonce(nonce, m_tx_nonce, true);

    crypto_stream_xchacha20_xor_ic(
        (uint8_t*)buffer,
        (const uint8_t*)buffer,
        (unsigned long long)len,
        nonce,
        0,
        m_tx_key);

    ++m_tx_nonce;
}

void SecureCipher::DecryptInPlace(void* buffer, size_t len)
{
    if (!m_activated || len == 0)
        return;

    uint8_t nonce[NONCE_SIZE];
    BuildNonce(nonce, m_rx_nonce, false);

    crypto_stream_xchacha20_xor_ic(
        (uint8_t*)buffer,
        (const uint8_t*)buffer,
        (unsigned long long)len,
        nonce,
        0,
        m_rx_key);

    ++m_rx_nonce;
}

bool SecureCipher::EncryptToken(const uint8_t* plaintext, size_t len,
                                 uint8_t* ciphertext, uint8_t* nonce_out)
{
    if (!m_initialized)
    {
        return false;
    }

    // Generate random nonce for this one-time encryption
    randombytes_buf(nonce_out, NONCE_SIZE);

    unsigned long long ciphertext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext, &ciphertext_len,
            plaintext, len,
            nullptr, 0,
            nullptr,
            nonce_out,
            m_tx_key) != 0)
    {
        return false;
    }

    return true;
}

bool SecureCipher::DecryptToken(const uint8_t* ciphertext, size_t len,
                                 const uint8_t* nonce, uint8_t* plaintext)
{
    if (!m_initialized)
    {
        return false;
    }

    unsigned long long plaintext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext, &plaintext_len,
            nullptr,
            ciphertext, len,
            nullptr, 0,
            nonce,
            m_rx_key) != 0)
    {
        return false;
    }

    return true;
}

void SecureCipher::SetSessionToken(const uint8_t* token)
{
    memcpy(m_session_token, token, SESSION_TOKEN_SIZE);
}
