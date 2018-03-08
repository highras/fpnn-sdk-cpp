#include <string.h>
#include "FPLog.h"
#include "md5.h"
#include "sha256.h"
#include "KeyExchange.h"

using namespace fpnn;

bool ECCKeysMaker::setCurve(const std::string& curve)
{
	if (curve == "secp256k1")
	{
		_curve = uECC_secp256k1();
		_secertLen = 32;
	}
	else if (curve == "secp256r1")
	{
		_curve = uECC_secp256r1();
		_secertLen = 32;
	}
	else if (curve == "secp224r1")
	{
		_curve = uECC_secp224r1();
		_secertLen = 28;
	}
	else if (curve == "secp192r1")
	{
		_curve = uECC_secp192r1();
		_secertLen = 24;
	}
	else
		return false;

	_publicKey.clear();
	_privateKey.clear();

	return true;
}

std::string ECCKeysMaker::publicKey(bool reGen)
{
	if (!_curve)
	{
		LOG_FATAL("ECC Private Key Config ERROR.");
		return std::string();
	}

	if (_publicKey.empty() || reGen)
	{
		uint8_t privateKey[32];
		uint8_t publicKey[64];

		if (uECC_make_key(publicKey, privateKey, _curve))
		{
			_publicKey.assign((char*)publicKey, _secertLen * 2);
			_privateKey.assign((char*)privateKey, _secertLen);
		}
		else
		{
			LOG_ERROR("Gen public key & private key failed.");
			return std::string();
		}
	}

	return _publicKey;
}

bool ECCKeysMaker::calcKey(uint8_t* key, uint8_t* iv, int keylen)
{
	if (!_curve)
	{
		LOG_FATAL("ECC Private Key Config ERROR.");
		return false;
	}
	if ((int)_peerPublicKey.length() != _secertLen * 2)
	{
		LOG_ERROR("Peer public key length missmatched.");
		return false;
	}

	uint8_t secret[32];
	int r = uECC_shared_secret((uint8_t*)_peerPublicKey.data(), (uint8_t*)_privateKey.data(), secret, _curve);
	if (r == 0)
	{
		LOG_ERROR("Cacluate shared secret failed.");
		return false;
	}

	if (keylen == 16)
	{
		memcpy(key, secret, 16);
	}
	else if (keylen == 32)
	{
		if (_secertLen == 32)
			memcpy(key, secret, 32);
		else
			sha256_checksum(key, secret, _secertLen);
	}
	else
	{
		LOG_ERROR("ECC Key Exchange: key len error.");
		return false;
	}

	md5_checksum(iv, secret, _secertLen);
	return true;
}
