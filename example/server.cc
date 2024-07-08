//
//  sample.cc
//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

#include "httplib.h"
#include "json.hpp"
#include <chrono>
#include <cstdio>

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"

using namespace httplib;
using json = nlohmann::json;

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
  // todo make this bulletproof by adding utc time to generations
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

const std::string log_prefix = "Server: ";

void info(std::string input) { std::cout << log_prefix << input << std::endl; }

int main(void) {

  info("Started!");
  const char *host_env = std::getenv("FILE_UPLOAD_HOST");
  const char *host_port = std::getenv("FILE_UPLOAD_PORT");
  const char *host_endpoint = std::getenv("FILE_UPLOAD_ENDPOINT");

  Server svr;

  svr.Get("/dump", [](const Request &req, Response &res) {
    res.set_content(dump_headers(req.headers), "text/plain");
  });

  svr.Get("/stop",
          [&](const Request & /*req*/, Response & /*res*/) { svr.stop(); });

  svr.Post("/calc", [](const httplib::Request &req, httplib::Response &res) {
    if (req.has_header("Content-Type") &&
        req.get_header_value("Content-Type") == "application/json") {

      const std::string random_filename = generate_random_hash();
      const std::string received_data_loation =
          "/home/" + random_filename + "_in.json";
      const std::string output_data_loation =
          "/home/" + random_filename + "_in_out.json";
      const std::string calculator_loation = "/home/bin/vrp_capacity";
      const std::string detailed_calculator_loation = "/home/bin/cvrptw";
      const std::string client_loation = "/home/bin/client";
      const std::string result_loation =
          "/home/" + random_filename + "_log.txt";

      std::ofstream outfile(received_data_loation,
                            std::ios::out | std::ios::trunc);
      if (outfile.is_open()) {

        outfile << req.body;
        outfile.close();

        json j = json::parse(req.body);
        auto indata = j["datas"];

        std::string loation = detailed_calculator_loation;
        if (indata["time_matrix"].is_null())
        {
            loation = calculator_loation;
        }

        const std::string calculateCommandStr =
            loation + " --input_filepath=" + received_data_loation +
            " &> " + result_loation;

        info("calculation started: " + calculateCommandStr);

        const int status = system(calculateCommandStr.c_str());
        res.status = 200;
        res.set_content(random_filename, "text/plain");

        if (status == 1) {
          printf("Error running system command.\n");
        } else {
          const std::string clientCommandStr =
              client_loation + " " + output_data_loation;

          info("sending data back: " + clientCommandStr);

          const int clientStatus = system(clientCommandStr.c_str());

          if (clientStatus == -1) {
            info("Failed!");
          } else {
            info("Done!");
          }
        }

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

  // svr.set_logger([](const Request &req, const Response &res) {
  //   printf("%s", log(req, res).c_str());
  // });
  std::string loaclhost = "0.0.0.0";
  auto port = 8080;

  svr.listen(loaclhost, port);

  return 0;
}
