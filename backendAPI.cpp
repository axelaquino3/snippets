#include <crow.h>
#include <crow/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <fstream>
#include <vector>

struct Snippet {
  int id;
  std::string language;
  std::string code;
};

int main() {
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

        snippets.push_back({id, language, code});
    }

  // ROUTES!!!
 
 CROW_ROUTE(app, "/snippets/<int>")
    .methods("GET"_method)
    ([&snippets](const crow::request& req, crow::response& res, int snippetId){
        // Handle GET request to retrieve a specific snippet by ID
        auto it = std::find_if(snippets.begin(), snippets.end(),
                               [snippetId](const Snippet& snippet) {
                                   return snippet.id == snippetId;
                               });

        if (it != snippets.end()) {
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);

            // Write the specific snippet to the response
            writer.StartObject();
            writer.Key("id");
            writer.Int(it->id);
            writer.Key("language");
            writer.String(it->language.c_str());
            writer.Key("code");
            writer.String(it->code.c_str());
            writer.EndObject();

            // Set the response body
            res.body = std::string(s.GetString());
            res.end();
        } else {
            // Handle snippet not found (return 404 response)
            res.code = 404;
            res.body = "Snippet not found";
            res.end();
        }
    });

  
   CROW_ROUTE(app, "/snippets")
    .methods("GET"_method)
    ([&](const crow::request& req, crow::response& res) -> void{
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
            writer.Key("code");
            writer.String(snippet.code.c_str());
            writer.EndObject();
        }
        writer.EndArray();

        // Set the response body
        res.body = std::string(buffer.GetString());
        res.end();
    });

  CROW_ROUTE(app, "/snippets")
    .methods("POST"_method)
    ([&snippets](const crow::request& req, crow::response& res) {
        // Handle the POST request to add a new snippet
        crow::json::rvalue jsonPayload = crow::json::load(req.body);
        if (!jsonPayload) {
            res.code = 400; // Bad Request
            res.body = "Invalid JSON payload";
            res.end();
            return;
        }

        // Extract data from jsonPayload
        int newId = snippets.size() + 1;
        std::string language = jsonPayload["language"].s();
        std::string code = jsonPayload["code"].s();

        // Add the new snippet to the snippets vector
        snippets.push_back({newId, language, code});

        // Set response status and body
        res.code = 201; // Created
        res.body = "Snippet created successfully";
        res.end();
    });



  app.port(8080).multithreaded().run();

  return 0;
}
