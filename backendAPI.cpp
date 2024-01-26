#include <crow.h>
#include <crow/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "openssl/err.h"
#include <openssl/evp.h>
#include <openssl/err.h>

struct Snippet {
    int id;
    std::string language;
    std::string code;           // Existing code field
};


// Function to encrypt a string using AES-256-CBC with OpenSSL
std::string encrypt(const std::string &plaintext, const unsigned char *key, const unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    int len;
    std::string ciphertext(plaintext.size() + AES_BLOCK_SIZE, 0);

    EVP_EncryptUpdate(ctx, (unsigned char*)&ciphertext[0], &len, (const unsigned char*)plaintext.c_str(), plaintext.size());
    int ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, (unsigned char*)&ciphertext[ciphertext_len], &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext;
}

// Function to decrypt the encrypted string using AES-256-CBC with OpenSSL
std::string decrypt(const std::string &ciphertext, const unsigned char *key, const unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    int len;
    std::string decryptedtext(ciphertext.size(), 0);

    EVP_DecryptUpdate(ctx, (unsigned char*)&decryptedtext[0], &len, (const unsigned char*)ciphertext.c_str(), ciphertext.size());
    int decryptedtext_len = len;

    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)&decryptedtext[decryptedtext_len], &len) != 1) {
        // Handle decryption error
        std::cerr << "Decryption error: EVP_DecryptFinal_ex" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    decryptedtext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Resize the buffer to the actual length of the decrypted text
    decryptedtext.resize(decryptedtext_len);

    return decryptedtext;
}


// Function to read Key and Iv from file
std::vector<unsigned char> readFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file:" << filename << std::endl;
        exit(1);
    }

    // Read the contents of the file into a vector
    std::vector<unsigned char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return content;
}



int main() {
  
  // Read key and IV from files
    std::vector<unsigned char> key = readFromFile("key.bin");
    std::vector<unsigned char> iv = readFromFile("iv.bin");

    std::string message = "hello world";
    std::string encryptMessage = encrypt(message, key.data(), iv.data());
    std::string decryptMessage = decrypt(encryptMessage, key.data(), iv.data());

    std::cout << encryptMessage << std::endl;
    std::cout << decryptMessage << std::endl;


  // Creates an instance of the SimpleApp class from Crow, setting up the web application.
  crow::SimpleApp app;
  std::vector<Snippet> snippets;

  // Opens/Reads the seedData.json file
  std::ifstream seedDataFile("seedData.json");
  if(!seedDataFile.is_open()) {
     return 1;
  }

     // Read the contents of the file into a string
    std::string seedDataString((std::istreambuf_iterator<char>(seedDataFile)), std::istreambuf_iterator<char>());

    // Close the file
    seedDataFile.close();

    // Parse JSON string using RapidJSON
    rapidjson::Document jsonDocument;
    jsonDocument.Parse(seedDataString.c_str());

    // Check for parsing errors
    if (jsonDocument.HasParseError()) {
        // Handle parsing errors
        return 1;
    }

    // Populate the snippets vector from the parsed JSON
    for (const auto& snippetJson : jsonDocument.GetArray()) {
        int id = snippetJson["id"].GetInt();
        std::string language = snippetJson["language"].GetString();
        std::string code = snippetJson["code"].GetString();

        std::cout << "Code: " << code << std::endl;

        
        // Encrypt the "code" field
        std::string encryptedCode = encrypt(code, key.data(), iv.data());

        std::cout << "Encryption: " << encryptedCode << std::endl;        
        std::cout << "Decryption: " << decrypt(encryptedCode, key.data(), iv.data()) << std::endl;

        snippets.push_back({id, language, encryptedCode});
    }

//     for (const auto& snippet : snippets) {
//     std::cout << "Snippet ID: " << snippet.id << std::endl;
//     std::cout << "Language: " << snippet.language << std::endl;
//     std::cout << "Encrypted Code: " << snippet.code << std::endl;
//     std::cout << "Decrypted Code: " << decrypt(snippet.code, key.data(), iv.data()) << std::endl;
//     // Add any other fields you want to print

//     // Add a separator for better readability
//     std::cout << "------------------------" << std::endl;
// }


  // ROUTES!!!
   CROW_ROUTE(app, "/snippets")
    .methods("GET"_method)
    ([&snippets, &key, &iv](const crow::request& req, crow::response& res){
        // Handle GET request to retrieve all snippets
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        // Write all snippets to the response
        writer.StartArray();
        for (const auto& snippet : snippets) {
            writer.StartObject();
            writer.Key("id");
            writer.Int(snippet.id);
            writer.Key("language");
            writer.String(snippet.language.c_str());
            
            // Use the decryptedCode if available, otherwise use the original "code" field
            writer.Key("code");
            writer.String(snippet.code.c_str());            
            writer.EndObject();
        }
        writer.EndArray();

        // Set the response body
        res.body = std::string(buffer.GetString());
        res.end();
    });


  app.port(8080).multithreaded().run();

  return 0;
}
