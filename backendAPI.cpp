#include <crow.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <fstream>
#include <streambuf>

struct Post {
  int id;
  std::string language;
  std::string code;
};

int main() {
    // Creates an instance of the SimpleApp class from Crow, setting up the web application.
    crow::SimpleApp app;

    // Opens/Reads the seedData.json file
    std::ifstream ifs("seedData.json");
    
    // This line reads the entire contents of the file "ifs" and stores them as a string in the variable "content". It uses "iterators" to traverse the file's contents character by character, capturing them within a string, effectively loading the file's content into the string variable "content".
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    
    // Creates empty object
    rapidjson::Document jsonDocument;
    
    // Parse the JSON string content into jsonDocument object. The Parse function requires a C-style string as input. This method of rapidjson::Document attempts to parse the JSON data provided in the C-style string. It interprets the string as JSON and attempts to construct a structured representation of that JSON data inside the "jsonDocument".
    jsonDocument.Parse(content.c_str());

    
    // ROUTES!!!
    
    // *GET* All Snippets 
    
    // Defines a route using Crow's CROW_ROUTE macro. It associates this route with the "app" instance of SimpleApp. This route is set for the path "/snippet".
    CROW_ROUTE(app, "/snippet")
    
    // Specifies that this route should handle only HTTP GET requests. The "_method" is a Crow way to specify the HTTP method for the route.
    .methods("GET"_method)
    
    // Starts a lambda function that handles the GET request when this route is accessed.The lambda function captures the "jsonDocument" variable by reference (&jsonDocument). This allows the function to access the jsonDocument object defined outside the lambda scope.
    ([&jsonDocument]() {
        
        // Creates a string buffer to hold the JSON data.
        rapidjson::StringBuffer buffer;
        
        // Creates a JSON writer that writes to the buffer.
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        
        // Writes the contents of "jsonDocument" (parsed JSON data) to the string buffer using the writer.
        jsonDocument.Accept(writer);
        
        // Retrieves the string representation of the JSON data from the buffer (buffer.GetString()).Creates a Crow response object using crow::response and returns it.This response contains the JSON data obtained from jsonDocument in response to a GET request to /snippet.
        return crow::response(buffer.GetString());
    });


    // *GET* Snippet by ID
    
    // Defines a route for handling GET requests at the /snippet/<int> endpoint, where <int> represents the ID parameter. 
    CROW_ROUTE(app, "/snippet/<int>")
    // Specifies that this route will only respond to GET requests.
    .methods("GET"_method)
    // "&jsonDocument" captures the "jsonDocument" variable from the outer scope, allowing access to the JSON data."int id" captures the integer value from the URL, representing the requested ID.
    ([&jsonDocument](int id) {
      
      // Initializes an iterator itr at the beginning of the JSON array in jsonDocument.
      rapidjson::Value::ConstValueIterator itr = jsonDocument.Begin();

      // Initializes an iterator itr at the beginning of the JSON array in jsonDocument.
      while (itr != jsonDocument.End()) {
        
        // Checks if the "id" field of the current object in the JSON array matches the provided id.
        if ((*itr)["id"].GetInt() == id) {
          //Creates a rapidjson::StringBuffer to hold the JSON data.
          rapidjson::StringBuffer buffer;
          // Initializes a rapidjson::Writer with the buffer to write the JSON data.
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

          // Uses itr->Accept(writer) to write the current object to the buffer.
          itr->Accept(writer);

          // Constructs a response containing the JSON data for the specific ID. 
          return crow::response(buffer.GetString());

        }

        // Moves the iterator to the next object in the JSON array if no match is found.
        ++itr;
      }

      // Returns a 404 response indicating that the data was not found if the loop completes without finding a matching ID.
      return crow::response(404, "Data not found");

    });

    CROW_ROUTE(app, "/snippet")
.methods("POST"_method)
([](const crow::request& req) {
    // Parse the JSON from the request body
    rapidjson::Document jsonDocument;
    jsonDocument.Parse(req.body.c_str());

    // Check if parsing was successful and if the request body contains required fields
    if (!jsonDocument.IsObject() || !jsonDocument.HasMember("id") || !jsonDocument.HasMember("language") || !jsonDocument.HasMember("code")) {
        return crow::response(400, "Invalid JSON or missing fields");
    }

    // Access the fields from the request body
    int id = jsonDocument["id"].GetInt();
    std::string language = jsonDocument["language"].GetString();
    std::string code = jsonDocument["code"].GetString();

    // Load the existing JSON data from seedData.json
    std::ifstream ifs("seedData.json");
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    rapidjson::Document existingData;
    existingData.Parse(content.c_str());

    int maxId = 0;
    for (rapidjson::Value::ConstValueIterator itr = existingData.Begin(); itr != existingData.End(); ++itr) {
        int id = (*itr)["id"].GetInt();
        if (id > maxId) {
            maxId = id;
        }
    }

    // Set the ID for the new post to be one greater than the maximum ID in the existing data
    int newId = maxId + 1;

    // Create a rapidjson::Value for the new post
    rapidjson::Value newPost(rapidjson::kObjectType);
    newPost.SetObject();
    newPost.AddMember("id", newId, existingData.GetAllocator());
    newPost.AddMember("language", rapidjson::Value().SetString(language.c_str(), language.size(), existingData.GetAllocator()), existingData.GetAllocator());
    newPost.AddMember("code", rapidjson::Value().SetString(code.c_str(), code.size(), existingData.GetAllocator()), existingData.GetAllocator());

    // Append the new post to the existing JSON array
    existingData.PushBack(newPost, existingData.GetAllocator());

    // Save the updated content back to seedData.json
    std::ofstream ofs("seedData.json");
    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    existingData.Accept(writer);

    return crow::response(200, "POST request received and data added");
});
   

    app.port(8080).multithreaded().run();

    return 0;
}




