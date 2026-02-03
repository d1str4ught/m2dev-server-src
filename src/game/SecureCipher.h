#pragma once

#include <sodium.h>
#include <cstdint>
#include <cstring>

class SecureCipher {
public:
    // libsodium constants
    static constexpr size_t PK_SIZE = crypto_kx_PUBLICKEYBYTES;           // 32
    static constexpr size_t SK_SIZE = crypto_kx_SECRETKEYBYTES;           // 32
    static constexpr size_t KEY_SIZE = crypto_kx_SESSIONKEYBYTES;         // 32
    static constexpr size_t NONCE_SIZE = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;  // 24
    static constexpr size_t TAG_SIZE = crypto_aead_xchacha20poly1305_ietf_ABYTES;       // 16
    static constexpr size_t CHALLENGE_SIZE = 32;
    static constexpr size_t SESSION_TOKEN_SIZE = 32;
    static constexpr size_t HMAC_SIZE = crypto_auth_BYTES;                // 32

    SecureCipher();
    ~SecureCipher();

    // Initialization - generates keypair
    bool Initialize();
    void CleanUp();

    // Key exchange
    void GetPublicKey(uint8_t* out_pk) const;

    // Client computes session keys from server's public key
    bool ComputeClientKeys(const uint8_t* server_pk);

    // Server computes session keys from client's public key
    bool ComputeServerKeys(const uint8_t* client_pk);

    // Challenge-response for authentication
    void GenerateChallenge(uint8_t* out_challenge);
    void ComputeChallengeResponse(const uint8_t* challenge, uint8_t* out_response);
    bool VerifyChallengeResponse(const uint8_t* challenge, const uint8_t* response);

    // AEAD encryption - output is len + TAG_SIZE bytes
    // Returns actual ciphertext length (plaintext_len + TAG_SIZE)
    size_t Encrypt(const void* plaintext, size_t plaintext_len, void* ciphertext);

    // AEAD decryption - input must be ciphertext_len bytes (includes TAG_SIZE)
    // Returns actual plaintext length, or 0 on failure
    size_t Decrypt(const void* ciphertext, size_t ciphertext_len, void* plaintext);

    // In-place stream encryption for network buffers (XChaCha20, no tag overhead)
    // Same length in/out. Nonce counter prevents replay.
    void EncryptInPlace(void* buffer, size_t len);
    void DecryptInPlace(void* buffer, size_t len);

    // Encrypt a single token with explicit nonce (for KeyComplete packet)
    bool EncryptToken(const uint8_t* plaintext, size_t len,
                      uint8_t* ciphertext, uint8_t* nonce_out);
    bool DecryptToken(const uint8_t* ciphertext, size_t len,
                      const uint8_t* nonce, uint8_t* plaintext);

    // State
    bool IsActivated() const { return m_activated; }
    void SetActivated(bool value) { m_activated = value; }
    bool IsInitialized() const { return m_initialized; }

    // Session token management
    void SetSessionToken(const uint8_t* token);
    const uint8_t* GetSessionToken() const { return m_session_token; }

    // Get current nonce counters (for debugging/logging)
    uint64_t GetTxNonce() const { return m_tx_nonce; }
    uint64_t GetRxNonce() const { return m_rx_nonce; }

private:
    bool m_initialized = false;
    bool m_activated = false;

    // X25519 keypair
    uint8_t m_pk[PK_SIZE];
    uint8_t m_sk[SK_SIZE];

    // Session keys derived from key exchange
    uint8_t m_tx_key[KEY_SIZE];  // Key for encrypting outgoing packets
    uint8_t m_rx_key[KEY_SIZE];  // Key for decrypting incoming packets

    // Nonce counters - prevent replay attacks
    uint64_t m_tx_nonce = 0;
    uint64_t m_rx_nonce = 0;

    // Server-generated session token
    uint8_t m_session_token[SESSION_TOKEN_SIZE];

    // Build 24-byte nonce from counter
    void BuildNonce(uint8_t* nonce, uint64_t counter, bool is_tx);
};
