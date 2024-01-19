#include <crow.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <fstream>
#include <streambuf>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "openssl/err.h"

// Function to encrypt a string using AES-256-CBC
std::string encrypt(const std::string &plaintext, const unsigned char *key, const unsigned char *iv)
{

  // Initialize OpenSSL to open algorithms and error strings. It's necessary before performing cryptographic operations.
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();

  // Allocates and initializes AES cipher contexts(another word for container)
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  // Initializes cipher context with AES-256-CBC algorithm, key, and initialization vector (IV)
  EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

  // empty string to store encrypted result
  std::string ciphertext;

  // A buffer(or temporary storage) is allocated in the ciphertext string to store the encrypted data. The size is set to accommodate the original plaintext and potential padding.
  ciphertext.resize(plaintext.size() + AES_BLOCK_SIZE);

  // EVP_EncryptUpdate function is used to perform the actual encryption. It updates the cipher context with the input plaintext and produces encrypted output, which is stored in the ciphertext buffer.
  int len;
  EVP_EncryptUpdate(ctx, (unsigned char *)&ciphertext[0], &len, (const unsigned char *)plaintext.c_str(), plaintext.size());

  // Is assigned the length of the encrypted data obtained from the previous EVP_EncryptUpdate call.
  int ciphertext_len = len;

  // Finalize the encryption by adding any remaining encrypted data to the ciphertext buffer.
  EVP_EncryptFinal_ex(ctx, (unsigned char *)&ciphertext[ciphertext_len], &len);
  ciphertext_len += len;

  // The cipher context is freed to release resources.
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext;
}

// Functin to decrypt the encrypted string
std::string decrypt(const std::string &ciphertext, const unsigned char *key, const unsigned char *iv)
{
  // Initialize OpenSSL
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();

  // Allocates and initializ AES cipher context
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  // Initialize cipher context with AES-256-CBC algorithm, key, and initialization vector (IV)
  EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

  std::string decryptedtext;

  // Allocates buffer for decrpyted data
  decryptedtext.resize(ciphertext.size());

  // Decrypt the data
  int len;
  EVP_DecryptUpdate(ctx, (unsigned char *)&decryptedtext[0], &len, (const unsigned char *)ciphertext.c_str(), ciphertext.size());
  int decryptedtext_len = len;

  // Finalize the decryption
  EVP_DecryptFinal(ctx, (unsigned char *)&decryptedtext[decryptedtext_len], &len);
  decryptedtext_len += len;

  // The cipher context is freed to release resources.
  EVP_CIPHER_CTX_free(ctx);

  // Remove the padding from the decrypted text
  decryptedtext.resize(decryptedtext_len);

  return decryptedtext;
}

// Function to generate key and iv

void generateKeyAndIv(unsigned char* key, unsigned char* iv) {
  // Generates a random key
  RAND_bytes(key, EVP_MAX_KEY_LENGTH);
  // Generates a random iv
  RAND_bytes(iv, EVP_MAX_IV_LENGTH);
}


int main() {

  // Creates an instance of the SimpleApp class from Crow, setting up the web application.
  crow::SimpleApp app;

  // Generate key and IV
  unsigned char key[EVP_MAX_KEY_LENGTH];
  unsigned char iv[EVP_MAX_IV_LENGTH];
  generateKeyAndIv(key, iv);

  // Opens/Reads the seedData.json file
  std::ifstream ifs("seedData.json");

  // This line reads the entire contents of the file "ifs" and stores them as a string in the variable "content". It uses "iterators" to traverse the file's contents character by character, capturing them within a string, effectively loading the file's content into the string variable "content".
  std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  // Creates empty object
  rapidjson::Document jsonDocument;

  // Parse the JSON string content into jsonDocument object. The Parse function requires a C-style string as input. This method of rapidjson::Document attempts to parse the JSON data provided in the C-style string. It interprets the string as JSON and attempts to construct a structured representation of that JSON data inside the "jsonDocument".
  jsonDocument.Parse(content.c_str());

  
  
  app.port(8081).multithreaded().run();

  return 0;
}

