#pragma once

#ifdef HAVE_LIBCRYPTOPP
#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/des.h>
#include <cryptopp/aes.h>
#include <cryptopp/cast.h>
#include <cryptopp/tea.h>
#include <cryptopp/3way.h>
#include <cryptopp/blowfish.h>
#include <cryptopp/twofish.h>
#include <cryptopp/rc2.h>
#include <cryptopp/arc4.h>
#include <cryptopp/serpent.h>
#include <cryptopp/gost.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#endif

#define TRANSMITTED_IV_SIZE     128     /* size of IV to transmit - must be as big as largest IV needed for any crypto algorithm */

/********************* ENCRYPTION TYPES ****************/

#define ENCRYPT_NONE            0       /* no encryption */
#define ENCRYPT_XOR             1       /* not really encrypted, just obfuscated */

#ifdef HAVE_LIBCRYPTOPP
#define ENCRYPT_DES             2       /* DES */
#define ENCRYPT_3DES            3       /* 3DES or Triple DES */
#define ENCRYPT_CAST128         4       /* CAST-128 */
#define ENCRYPT_CAST256         5       /* CAST-256 */
#define ENCRYPT_XTEA            6       /* xTEA */
#define ENCRYPT_3WAY            7       /* 3-WAY */
#define ENCRYPT_BLOWFISH        8       /* SKIPJACK */
#define ENCRYPT_TWOFISH         9       /* TWOFISH */
#define ENCRYPT_LOKI97          10      /* LOKI97 */
#define ENCRYPT_RC2             11      /* RC2 */
#define ENCRYPT_ARCFOUR         12      /* RC4 */
#define ENCRYPT_RC6             13      /* RC6 */            /* UNUSED */
#define ENCRYPT_RIJNDAEL128     14      /* RIJNDAEL-128 */
#define ENCRYPT_RIJNDAEL192     15      /* RIJNDAEL-192 */
#define ENCRYPT_RIJNDAEL256     16      /* RIJNDAEL-256 */
#define ENCRYPT_MARS            17      /* MARS */           /* UNUSED */
#define ENCRYPT_PANAMA          18      /* PANAMA */         /* UNUSED */
#define ENCRYPT_WAKE            19      /* WAKE */
#define ENCRYPT_SERPENT         20      /* SERPENT */
#define ENCRYPT_IDEA            21      /* IDEA */           /* UNUSED */
#define ENCRYPT_ENIGMA          22      /* ENIGMA (Unix crypt) */
#define ENCRYPT_GOST            23      /* GOST */
#define ENCRYPT_SAFER64         24      /* SAFER-sk64 */
#define ENCRYPT_SAFER128        25      /* SAFER-sk128 */
#define ENCRYPT_SAFERPLUS       26      /* SAFER+ */
#endif
#define LAST_ENCRYPTION_ID 26

namespace nsca {
	class nsca_encrypt {
	public:
		class encryption_exception {
			std::wstring msg_;
		public:
			encryption_exception() {}
			encryption_exception(std::wstring msg) : msg_(msg) {}
			std::wstring getMessage() const { return msg_; }

		};
		class any_encryption {
		public:
			virtual void init(std::string password, std::string iv) = 0;
			virtual void encrypt(std::string &buffer) = 0;
			virtual void decrypt(std::string &buffer) = 0;
			virtual std::wstring getName() = 0;
		};
#ifdef HAVE_LIBCRYPTOPP
		template <class TMethod>
		class cryptopp_encryption : public any_encryption {
		private:
			typedef CryptoPP::CFB_Mode_ExternalCipher::Encryption TEncryption;
			typedef typename TMethod::Encryption TCipher;
			TEncryption crypto_;
			TCipher cipher_;
			int keysize_;
		public:
			cryptopp_encryption() : keysize_(TMethod::DEFAULT_KEYLENGTH) {}
			cryptopp_encryption(int keysize) : keysize_(keysize) {}
			int get_keySize() {
				return keysize_;
			}
			int get_blockSize() {
				return TMethod::BLOCKSIZE;
			}

			virtual void init(std::string password, std::string iv) {
				init(password, (unsigned char*)&*iv.begin(), iv.size());

			}
			void init(std::string password, unsigned char *transmitted_iv, int iv_size) {
				/* generate an encryption/description key using the password */
				unsigned int keysize=get_keySize();

				unsigned char *key = new unsigned char[keysize+1];
				if (key == NULL){
					throw encryption_exception(_T("Could not allocate memory for encryption/decryption key"));
				}
				ZeroMemory(key,keysize*sizeof(unsigned char));
				strncpy(reinterpret_cast<char*>(key),password.c_str(),min(keysize,password.length()));


				/* determine size of IV buffer for this algorithm */
				int blocksize = get_blockSize();
				if(blocksize>iv_size){
					throw encryption_exception(_T("IV size for crypto algorithm exceeds limits"));
				}

				/* allocate memory for IV buffer */
				unsigned char *iv = new unsigned char[blocksize+1];
				if (iv == NULL){
					throw encryption_exception(_T("Could not allocate memory for IV buffer"));
				}

				/* fill IV buffer with first bytes of IV that is going to be used to crypt (determined by server) */
				memcpy(iv, transmitted_iv, sizeof(unsigned char)*blocksize);

				try {
					cipher_.SetKey(key, keysize);
					crypto_.SetCipherWithIV(cipher_, iv, 1);
				} catch (...) {
					throw encryption_exception(_T("Unknown exception when trying to setup crypto"));
				}
				delete [] iv;
				delete [] key;
			}
			void encrypt(std::string &buffer) {
				encrypt((unsigned char*)&*buffer.begin(), buffer.size());
			}
			void encrypt(unsigned char *buffer, int buffer_size) {
				/* encrypt each byte of buffer, one byte at a time (CFB mode) */
				try {
					for(int x=0;x<buffer_size;x++)
						crypto_.ProcessData(&buffer[x], &buffer[x], 1);
				} catch (...) {
					throw encryption_exception(_T("Unknown exception when trying to setup crypto"));
				}
			}
			void decrypt(std::string &buffer) {
				decrypt((unsigned char*)&*buffer.begin(), buffer.size());
			}
			void decrypt(unsigned char *buffer, int buffer_size) {
				throw encryption_exception(_T("Decryption not supported"));
			}
			std::wstring getName() {
				return strEx::string_to_wstring(TMethod::StaticAlgorithmName());
			}

		};
#endif
		class no_encryption : public any_encryption {
		public:
			static int get_keySize() {
				return 0;
			}
			static int get_blockSize() {
				return 1;
			}
			void init(std::string password, std::string iv) {}
			void encrypt(std::string &buffer) { std::cout << "USING NO ENCRYPTION * * * " << std::endl;}
			void decrypt(std::string &buffer) {}
			std::wstring getName() {
				return _T("No Encryption (not safe)");
			}
		};
		class xor_encryption : public any_encryption {
		private:
			std::string iv_;
			std::string password_;
		public:
			xor_encryption() {}
			~xor_encryption() {}
			static int get_keySize() {
				return 0;
			}
			static int get_blockSize() {
				return 1;
			}
			void init(std::string password, std::string iv) {
				iv_ = iv;
				password_ = password;
			}
			void encrypt(std::string &buffer) {
				/* rotate over IV we received from the server... */
				unsigned int buf_len =  buffer.size();
				unsigned int iv_len = iv_.size();
				unsigned int pwd_len = password_.size();
				for (int y=0,x=0,z=0;y<buf_len;y++,x++,z++) {
					/* keep rotating over IV */
					if (x >= iv_len)
						x = 0;
					buffer[y] ^= iv_[x];
					/* keep rotating over Password */
					if (z >= pwd_len)
						z = 0;
					buffer[y] ^= password_[z];
				}
			}
			void decrypt(std::string &buffer) {
				throw encryption_exception(_T("Decryption not supported"));
			}
			std::wstring getName() {
				return _T("XOR (not safe)");
			}
		};

	private:
		any_encryption *core_;
	public:

		nsca_encrypt() : core_(NULL) {}
		~nsca_encrypt() {
			delete core_;
		}


		static bool hasEncryption(int encryption_method) {
			switch(encryption_method) {
			case ENCRYPT_NONE:
			case ENCRYPT_XOR:
#ifdef HAVE_LIBCRYPTOPP
			case ENCRYPT_DES:
			case ENCRYPT_3DES:
			case ENCRYPT_CAST128:
			case ENCRYPT_XTEA:
			case ENCRYPT_BLOWFISH:
			case ENCRYPT_TWOFISH:
			case ENCRYPT_RC2:
			case ENCRYPT_RIJNDAEL128:
			case ENCRYPT_SERPENT:
			case ENCRYPT_GOST:
#endif
				return true;

				// UNdefined
#ifdef HAVE_LIBCRYPTOPP
			case ENCRYPT_3WAY:
			case ENCRYPT_ARCFOUR:
			case ENCRYPT_CAST256:
			case ENCRYPT_LOKI97:
			case ENCRYPT_WAKE:
			case ENCRYPT_ENIGMA:
			case ENCRYPT_RIJNDAEL192:
			case ENCRYPT_RIJNDAEL256:
			case ENCRYPT_SAFER64:
			case ENCRYPT_SAFER128:
			case ENCRYPT_SAFERPLUS:
#endif
			default:
				return false;
			}
		}


		static any_encryption* get_encryption_core(int encryption_method) {
			switch(encryption_method) {
	case ENCRYPT_NONE:
		return new no_encryption();
	case ENCRYPT_XOR:
		return new xor_encryption();
#ifdef HAVE_LIBCRYPTOPP
	case ENCRYPT_DES:
		return new cryptopp_encryption<CryptoPP::DES>();
	case ENCRYPT_3DES:
		return new cryptopp_encryption<CryptoPP::DES_EDE3>();
	case ENCRYPT_CAST128:
		return new cryptopp_encryption<CryptoPP::CAST128>();
	case ENCRYPT_XTEA:
		return new cryptopp_encryption<CryptoPP::XTEA>();
	case ENCRYPT_3WAY:
		return new cryptopp_encryption<CryptoPP::ThreeWay>();
	case ENCRYPT_BLOWFISH:
		return new cryptopp_encryption<CryptoPP::Blowfish>(56);
	case ENCRYPT_TWOFISH:
		return new cryptopp_encryption<CryptoPP::Twofish>(32);
	case ENCRYPT_RC2:
		return new cryptopp_encryption<CryptoPP::RC2>(128);
	case ENCRYPT_RIJNDAEL128:
		return new cryptopp_encryption<CryptoPP::AES>(32);
	case ENCRYPT_SERPENT:
		return new cryptopp_encryption<CryptoPP::Serpent>(32);
	case ENCRYPT_GOST:
		return new cryptopp_encryption<CryptoPP::GOST>();
#endif
	default:
		return NULL;
			}
		}
		static std::string generate_transmitted_iv(unsigned int len = TRANSMITTED_IV_SIZE){
			std::string buffer;
			buffer.resize(len);

			/*********************************************************/
			/* fill IV buffer with data that's as random as possible */ 
			/*********************************************************/

			/* else fall back to using the current time as the seed */
			int seed=(int)time(NULL);

			/* generate pseudo-random IV */
			srand(seed);
			for(unsigned int x=0;x<len;x++)
				buffer[x]=(int)((256.0*rand())/(RAND_MAX+1.0));
			return buffer;
		}

		/* initializes encryption routines */
		void encrypt_init(std::string password, int encryption_method, std::string received_iv){
			delete core_;
			core_ = get_encryption_core(encryption_method);
			if (core_ == NULL)
				throw encryption_exception(_T("Failed to get encryption core!"));

			/* server generates IV used for encryption */
			if (received_iv.empty()) {
				std::string iv = generate_transmitted_iv();
				core_->init(password, iv);
			} else	/* client receives IV from server */
				core_->init(password, received_iv);
		}

		/* encrypt a buffer */
		void encrypt_buffer(std::string &buffer) {
			if (core_ == NULL)
				throw encryption_exception(_T("No encryption core!"));
			core_->encrypt(buffer);
		}
		std::string get_rand_buffer(int length) {
			std::string buffer; buffer.resize(length);
			//unsigned char * buffer = new unsigned char[length+1];
#if HAVE_LIBCRYPTOPP
			CryptoPP::AutoSeededRandomPool rng;
			rng.GenerateBlock((byte*)&*buffer.begin(), length);
#endif
			return buffer;
		}
	};
}
