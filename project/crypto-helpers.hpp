#include <openssl/evp.h>
#include <vector>

namespace cs118 {

class ECDHState
{
public:
  ~ECDHState();

  void
  loadPrivateKey(const std::vector<uint8_t>& der);

  void
  generatePrivateKey();

  const std::vector<uint8_t>&
  deriveSecret(const std::vector<uint8_t>& peerkey);

  const std::vector<uint8_t>&
  getSelfPubKey();

  const std::vector<uint8_t>&
  getSelfPrvKey();

  void
  sign(const std::vector<uint8_t>& data, std::vector<uint8_t>& signature);

  bool
  verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature,
         const std::vector<uint8_t>& peerKey);

private:
  EC_KEY* m_eckey = nullptr;
  std::vector<uint8_t> m_pubKey;
  std::vector<uint8_t> m_prvKey;
  std::vector<uint8_t> m_secret;
};


size_t
hkdf(const uint8_t* secret, size_t secretLen,
     const uint8_t* salt, size_t saltLen,
     uint8_t* output, size_t outputLen,
     const uint8_t* info = nullptr, size_t infoLen = 0);


void
hmacSha256(const uint8_t* data, size_t dataLen,
           const uint8_t* key, size_t keyLen,
           uint8_t* result);

void
aesCbc256Encrypt(uint8_t* plaintext, int plaintextLen,
                 const uint8_t* key, const uint8_t* iv,
                 uint8_t* ciphertext, int* ciphertextLen);

void
aesCbc256Decrypt(const uint8_t* ciphertext, int ciphertextLen,
                 const uint8_t* key, const uint8_t* iv,
                 uint8_t* plaintext, int* plaintextLen);

} // namespace cs118
