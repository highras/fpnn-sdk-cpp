#ifndef FPNN_KeyExchange_h
#define FPNN_KeyExchange_h

#include <string>
#include <stdint.h>
#include "micro-ecc/uECC.h"

namespace fpnn
{
	/**
	 *	ECCKeyExchange is only server using.
	 * 	This is a fake class for UDP.v2 unused server side codes.
	 */
	class ECCKeyExchange
	{
	public:
		bool calcKey(uint8_t* key, uint8_t* iv, int keylen, const std::string& peerPublicKey)
		{
			(void)key;
			(void)iv;
			(void)keylen;
			(void)peerPublicKey;

			return false;
		}
	};

	class ECCKeysMaker
	{
		int _secertLen;
		uECC_Curve _curve;
		std::string _privateKey;
		std::string _publicKey;
		std::string _peerPublicKey;

	public:
		ECCKeysMaker(): _secertLen(0), _curve(NULL) {}
		void setPeerPublicKey(const std::string& peerPublicKey)	//-- in binary format. No't base64 or hex.
		{
			_peerPublicKey = peerPublicKey;
		}

		/*
			curve:
				secp256k1
				secp256r1
				secp224r1
				secp192r1
		*/
		bool setCurve(const std::string& curve);
		std::string publicKey(bool reGen = false);

		/*
			key: OUT. Key buffer length is equal to keylen.
			iv: OUT. iv buffer length is 16 bytes.
			keylen: IN. 16 or  32.
		*/
		bool calcKey(uint8_t* key, uint8_t* iv, int keylen);
	};
}

#endif
