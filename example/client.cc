#include "httplib.h"
#include <iostream>

#define CA_CERT_FILE "./ca-bundle.crt"

using namespace std;

const std::string log_prefix = "Client: ";

void info(std::string input) { std::cout << log_prefix << input << std::endl; }
void error(std::string input) { std::cerr << log_prefix << input << std::endl; }
std::string dump_headers(const httplib::Headers &headers) {
  std::string s;
  char buf[BUFSIZ];

  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

int main(int argc, char *argv[]) {

  info("Started");
  const char *host_env = std::getenv("FILE_UPLOAD_HOST");
  const char *host_port = std::getenv("FILE_UPLOAD_PORT");
  const char *host_endpoint = std::getenv("FILE_UPLOAD_ENDPOINT");

  //info("Host: ");
  //info(host_env);
  //info("Port: ");
  //info(host_port);
  //info("Endpoint: ");
  //info(host_endpoint);

  std::string host = (host_env) ? host_env : "localhost";
  std::string port = (host_port) ? host_port : "8080";
  std::string endpoint = (host_endpoint) ? host_endpoint : "upload";

  info("Url to post: http://" + host + ":" + port + "/" + endpoint);

  if (argc < 2) {
    error("Error: No file path provided.");
    return 1;
  }

  std::string filePath = argv[1];
  info("Parameters: " + filePath);

  std::ifstream file(filePath);
  if (!file.is_open()) {
    error("Error: Unable to open file '" + filePath + "'");
    return 1;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string fileContents = buffer.str();
  file.close();

  std::string url = "http://" + host + ":" + port;
  httplib::Client client(url);

  auto res = client.Get("/token");

  if (res && res->status == 200) {
    std::string response_body = res->body;

    std::string laravel_session_cookie;
    for (const auto &header : res->headers) {
      if (header.first == "Set-Cookie") {
        auto set_cookie_header = header.second;
        auto pos = set_cookie_header.find("laravel_session=");
        if (pos != std::string::npos) {
          auto end_pos = set_cookie_header.find(';', pos);
          laravel_session_cookie = set_cookie_header.substr(pos, end_pos - pos);
        }
      }
    }

    httplib::Headers headers = {
        {"Content-Type", "application/json"},
        {"Connection", "keep-alive"},
        {"Cookie", laravel_session_cookie},
        {"X-CSRF-TOKEN", response_body},
        {"Content-Length", std::to_string(fileContents.size())}};

    auto res2 =
        client.Post("/" + endpoint, headers, fileContents, "application/json");

    if (res2 && res2->status == 200) {

      info("Response: " + res2->body);
    } else {
      error("Error-response: " + res2->body);
      error(httplib::to_string(res2.error()));
      return -1;
    }
  } else {

    error("Error getting: " + endpoint);
    error(httplib::to_string(res.error()));
    return -1;
  }

  return 0;
}
