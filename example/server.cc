//
//  sample.cc
//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <chrono>
#include <cstdio>
#include <httplib.h>

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"

using namespace httplib;

std::string dump_headers(const Headers &headers) {
  std::string s;
  char buf[BUFSIZ];

  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

std::string log(const Request &req, const Response &res) {
  std::string s;
  char buf[BUFSIZ];

  s += "================================\n";

  snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
           req.version.c_str(), req.path.c_str());
  s += buf;

  std::string query;
  for (auto it = req.params.begin(); it != req.params.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%c%s=%s",
             (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
             x.second.c_str());
    query += buf;
  }
  snprintf(buf, sizeof(buf), "%s\n", query.c_str());
  s += buf;

  s += dump_headers(req.headers);

  s += "--------------------------------\n";

  snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
  s += buf;
  s += dump_headers(res.headers);
  s += "\n";

  if (!res.body.empty()) { s += res.body; }

  s += "\n";

  return s;
}

std::string generate_random_hash(int length = 16) {
  static const char alphanum[] = "0123456789abcdef";
  std::random_device rd;  // Seed for random number generation
  std::mt19937 gen(rd()); // Mersenne Twister random number generator
  std::uniform_int_distribution<> dis(0, 15);

  std::stringstream ss;
  for (int i = 0; i < length; ++i) {
    ss << alphanum[dis(gen)];
  }
  return ss.str();
}

int main(void) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
#else
  Server svr;
#endif

  if (!svr.is_valid()) {
    printf("server has an error...\n");
    return -1;
  }

  svr.Get("/dump", [](const Request &req, Response &res) {
    res.set_content(dump_headers(req.headers), "text/plain");
  });

  svr.Get("/stop",
          [&](const Request & /*req*/, Response & /*res*/) { svr.stop(); });

  svr.Post("/calc", [](const httplib::Request &req, httplib::Response &res) {
    if (req.has_header("Content-Type") &&
        req.get_header_value("Content-Type") == "application/json") {

      std::string received_data_loation = "/home/received_data.json";
      std::string calculator_loation = "/home/alpine-build/bin/vrp_capacity";
      std::string random_filename = generate_random_hash() + ".txt";
      std::string result_loation = "/home/"+ random_filename;

      std::ofstream outfile(received_data_loation,
                            std::ios::out | std::ios::trunc);
      if (outfile.is_open()) {

        outfile << req.body;
        outfile.close();

        std::string commandStr =
            calculator_loation
            + " --input_filepath="+ received_data_loation
            + " &> " + result_loation;

        system(commandStr.c_str());

        res.status = 200;
        res.set_content(random_filename, "text/plain");
      } else {
        res.status = 500;
        res.set_content("Failed to open the file for writing.", "text/plain");
      }
    } else {
      res.status = 415; // Unsupported media type
      res.set_content("Expected Content-Type: application/json.", "text/plain");
    }
  });

  svr.set_error_handler([](const Request & /*req*/, Response &res) {
    const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), fmt, res.status);
    res.set_content(buf, "text/html");
  });

  svr.set_logger([](const Request &req, const Response &res) {
    printf("%s", log(req, res).c_str());
  });

  printf("server started\n");
  svr.listen("localhost", 8080);

  return 0;
}
