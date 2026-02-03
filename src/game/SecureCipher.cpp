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
    sodium_memzero(m_tx_stream_nonce, sizeof(m_tx_stream_nonce));
    sodium_memzero(m_rx_stream_nonce, sizeof(m_rx_stream_nonce));
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
    sodium_memzero(m_tx_stream_nonce, sizeof(m_tx_stream_nonce));
    sodium_memzero(m_rx_stream_nonce, sizeof(m_rx_stream_nonce));
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

    // Set up fixed stream nonces per direction
    // client->server = 0x02, server->client = 0x01
    sodium_memzero(m_tx_stream_nonce, NONCE_SIZE);
    m_tx_stream_nonce[0] = 0x02;
    sodium_memzero(m_rx_stream_nonce, NONCE_SIZE);
    m_rx_stream_nonce[0] = 0x01;

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

    // Set up fixed stream nonces per direction
    // server->client = 0x01, client->server = 0x02
    sodium_memzero(m_tx_stream_nonce, NONCE_SIZE);
    m_tx_stream_nonce[0] = 0x01;
    sodium_memzero(m_rx_stream_nonce, NONCE_SIZE);
    m_rx_stream_nonce[0] = 0x02;

    return true;
}

void SecureCipher::GenerateChallenge(uint8_t* out_challenge)
{
    randombytes_buf(out_challenge, CHALLENGE_SIZE);
}

void SecureCipher::ComputeChallengeResponse(const uint8_t* challenge, uint8_t* out_response)
{
    // HMAC the challenge using our tx_key as the authentication key
    // Client tx_key == Server rx_key, so the server can verify with its rx_key
    crypto_auth(out_response, challenge, CHALLENGE_SIZE, m_tx_key);
}

bool SecureCipher::VerifyChallengeResponse(const uint8_t* challenge, const uint8_t* response)
{
    // Verify the HMAC - peer should have used their tx_key (our rx_key) to compute it
    return crypto_auth_verify(response, challenge, CHALLENGE_SIZE, m_rx_key) == 0;
}

void SecureCipher::ApplyStreamCipher(void* buffer, size_t len,
                                     const uint8_t* key, uint64_t& byte_counter,
                                     const uint8_t* stream_nonce)
{
    uint8_t* p = (uint8_t*)buffer;

    // Handle partial leading block (if byte_counter isn't block-aligned)
    uint32_t offset = (uint32_t)(byte_counter % 64);
    if (offset != 0 && len > 0)
    {
        // Generate full keystream block, use only the portion we need
        uint8_t ks[64];
        sodium_memzero(ks, 64);
        crypto_stream_xchacha20_xor_ic(ks, ks, 64, stream_nonce, byte_counter / 64, key);

        size_t use = len < (64 - offset) ? len : (64 - offset);
        for (size_t i = 0; i < use; ++i)
            p[i] ^= ks[offset + i];

        p += use;
        len -= use;
        byte_counter += use;
    }

    // Handle remaining data (starts at a block boundary)
    if (len > 0)
    {
        crypto_stream_xchacha20_xor_ic(p, p, (unsigned long long)len,
                                       stream_nonce, byte_counter / 64, key);
        byte_counter += len;
    }
}

size_t SecureCipher::Encrypt(const void* plaintext, size_t plaintext_len, void* ciphertext)
{
    if (!m_activated)
    {
        return 0;
    }

    // AEAD encryption uses a random nonce (not the stream nonce)
    uint8_t nonce[NONCE_SIZE];
    randombytes_buf(nonce, NONCE_SIZE);

    unsigned long long ciphertext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            (uint8_t*)ciphertext, &ciphertext_len,
            (const uint8_t*)plaintext, plaintext_len,
            nullptr, 0,
            nullptr,
            nonce,
            m_tx_key) != 0)
    {
        return 0;
    }

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
    randombytes_buf(nonce, NONCE_SIZE);

    unsigned long long plaintext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            (uint8_t*)plaintext, &plaintext_len,
            nullptr,
            (const uint8_t*)ciphertext, ciphertext_len,
            nullptr, 0,
            nonce,
            m_rx_key) != 0)
    {
        return 0;
    }

    return (size_t)plaintext_len;
}

void SecureCipher::EncryptInPlace(void* buffer, size_t len)
{
    if (!m_activated || len == 0)
        return;

    ApplyStreamCipher(buffer, len, m_tx_key, m_tx_nonce, m_tx_stream_nonce);
}

void SecureCipher::DecryptInPlace(void* buffer, size_t len)
{
    if (!m_activated || len == 0)
        return;

    ApplyStreamCipher(buffer, len, m_rx_key, m_rx_nonce, m_rx_stream_nonce);
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
