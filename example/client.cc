#include <httplib.h>
#include <iostream>

#define CA_CERT_FILE "./ca-bundle.crt"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: No file path provided." << std::endl;
    return 1;
  }

  std::string filePath = argv[1];

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file '" << filePath << "'."
              << std::endl;
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string fileContents = buffer.str();
  file.close();

  // std::cout << "Contents of '" << filePath << "':" << std::endl;
  // std::cout << fileContents << std::endl;

  const char *host_env = std::getenv("FILE_UPLOAD_HOST");
  const char *host_port = std::getenv("FILE_UPLOAD_PORT");
  const char *host_endpoint = std::getenv("FILE_UPLOAD_ENDPOINT");
  std::string host = (host_env) ? host_env : "localhost";
  std::string port = (host_port) ? host_port : "8080";
  std::string endpoint = (host_endpoint) ? host_endpoint : "/upload";

  std::string url = "http://" + host + ":" + port;
  httplib::Client client(url);

  auto res = client.Get("/token");

  if (res && res->status == 200)
  {
      std::string response_body = res->body;

      httplib::Headers headers = {
          {"X-CSRF-TOKEN", response_body} };

      auto res = client.Post(endpoint, headers, fileContents, "text/plain");

      if (res.error() == httplib::Error::Success)
      {
          std::cout << "Successfully posted the file contents to " + endpoint
              << std::endl;
      }
      else
      {
          std::cerr << "Error posting to " + endpoint << httplib::to_string(res.error())
              << std::endl;
          return - 1;
      }
  }
  else
  {
      // Print error message
      std::cerr << "Error: " << (res ? res->status : -1) << std::endl;
      return -1;
  }

  return 0;
}
