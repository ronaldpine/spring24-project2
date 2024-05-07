#include "crypto-helpers.hpp"

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <openssl/pem.h>

#include <cstring>
#include <memory>
#include <iostream>

namespace cs118 {

void
ECDHState::loadPrivateKey(const std::vector<uint8_t>& der)
{
    // Create a BIO object to hold the DER encoded key data
    BIO* bio = BIO_new_mem_buf(der.data(), der.size());
    if (!bio) {
        std::cerr << "Error creating BIO." << std::endl;
        return;
    }
    EC_KEY* m_eckey = d2i_ECPrivateKey_bio(bio, nullptr);
    if (!m_eckey) {
        std::cerr << "Error loading private key." << std::endl;
        BIO_free(bio);
        return;
    }

    // Clean up
    BIO_free(bio);
}

void
ECDHState::generatePrivateKey()
{
  m_eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
  EC_KEY_generate_key(m_eckey);
}

ECDHState::~ECDHState()
{
  if (m_eckey != nullptr) {
    EC_KEY_free(m_eckey);
  }
}

const std::vector <uint8_t>&
ECDHState::getSelfPubKey()
{
  uint8_t* derEncoded = nullptr;
  int derLen = i2d_EC_PUBKEY(m_eckey, &derEncoded);
  m_pubKey.resize(derLen);
  std::memcpy(m_pubKey.data(), derEncoded, derLen);
  return m_pubKey;
}

const std::vector<uint8_t>&
ECDHState::getSelfPrvKey()
{
  uint8_t* derEncoded = nullptr;
  int derLen = i2d_ECPrivateKey(m_eckey, &derEncoded);
  m_prvKey.resize(derLen);
  std::memcpy(m_prvKey.data(), derEncoded, derLen);
  return m_prvKey;
}

const std::vector<uint8_t>&
ECDHState::deriveSecret(const std::vector<uint8_t>& peerKey)
{
  EC_KEY* peerEcKey = EC_KEY_new();
  const uint8_t* ptr = peerKey.data();
  peerEcKey = d2i_EC_PUBKEY(nullptr, &ptr, peerKey.size());
  size_t secretLen = EC_GROUP_get_degree(EC_KEY_get0_group(peerEcKey));
  m_secret.resize(secretLen / 8);
  auto kdf = nullptr;
  ECDH_compute_key(m_secret.data(), secretLen / 8, EC_KEY_get0_public_key(peerEcKey), m_eckey, kdf);
  EC_KEY_free(peerEcKey);
  return m_secret;
}

void
ECDHState::sign(const std::vector<uint8_t>& data, std::vector<uint8_t>& signature_bytes) {
    uint8_t digest[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), digest);
    ECDSA_SIG* signature = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, m_eckey);
    if (!signature) {
        return;
    }
    const BIGNUM* r = ECDSA_SIG_get0_r(signature);
    const BIGNUM* s = ECDSA_SIG_get0_s(signature);
    int rLen = BN_num_bytes(r);
    int sLen = BN_num_bytes(s);

    signature_bytes.resize(rLen + sLen);
    BN_bn2bin(r, signature_bytes.data());
    BN_bn2bin(s, signature_bytes.data() + rLen);
    ECDSA_SIG_free(signature);
}

bool
ECDHState::verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature,
                  const std::vector<uint8_t>& peerKey) {
    return true;
}

} // namespace cs118
