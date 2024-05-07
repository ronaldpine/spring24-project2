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

} // namespace cs118
