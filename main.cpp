#include <cttpd/Server.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <signal.h>
#include <string>
#include <sys/stat.h>

#ifndef DOMAIN
#error Please define a DOMAIN to build CName301
#endif

#define xstr(s) _str(s)
#define _str(s) #s

#define _DOMAIN xstr(DOMAIN)


const std::string ourdomain = _DOMAIN;

inline int hexchr2bin(const char hex) {
  if (hex >= '0' && hex <= '9') {
    return hex - '0';
  } else if (hex >= 'A' && hex <= 'F') {
    return hex - 'A' + 10;
  } else if (hex >= 'a' && hex <= 'f') {
    return hex - 'a' + 10;
  }
  return -1;
}

inline std::string from_base16(const std::string_view &b16text) {
  std::string out;
  for (auto chr : b16text) {
    int decchr = hexchr2bin(chr);
    if (decchr < 0)
      return "";

    out += decchr & 0xff;
  }
  return out;
}

inline bool file_exists(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

inline bool ends_with(std::string const &value, std::string const &ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string ResolveContentType(const std::string &path) {
  if (ends_with(path, ".html"))
    return "text/html; charset=utf-8";
  if (ends_with(path, ".js"))
    return "application/javascript";
  if (ends_with(path, ".css"))
    return "text/css";
  return "";
}

size_t fileLoader([[maybe_unused]] const Request &req, Response &resp,
                  std::string uripath) {
  resp.SetHeader("Cache-Control", "private, max-age=120");

  // Path is already normalized
  std::string path("public/" + uripath);
  std::ifstream inFile;
  inFile.open(path);
  // Empty/Non-existant file
  if (!inFile.good() || inFile.eof() || !file_exists(path)) {
    return 404;
  }

  std::stringstream strStream;
  strStream << inFile.rdbuf(); // read the file
  resp.SetBody(strStream.str());

  // Cache is determined by the body
  resp.SetHeader("Content-type", ResolveContentType(uripath));
  return 200;
}

void fallbackHandler(const Request &req, Response &resp) {
  // size_t status = fileLoader(req, resp, "/index.html");
  // resp.SetResponseCode(status);
  // TODO: if GetPath ends with / append index.html
  size_t status = fileLoader(req, resp, req.GetRequestURI()->GetPath());
  if (status == 404) {
    resp.SetBody("<html><body><center><h2>404 File not "
                 "found</h2></center></body></html>");
  }
  resp.SetResponseCode(status);
}

void indexHandler(const Request &req, Response &resp) {
  auto domain = req.GetHeader("Host");
  if (domain == ourdomain) {
    return fallbackHandler(req, resp);
  }
  
  // Not our domain, lets build up a redirect link

  // Remove port suffix if it exists
  auto port_offset = domain.find(':');
  if (port_offset != domain.npos)
	domain.remove_suffix(domain.size() - port_offset);

  // Remove our domain suffix
  domain.remove_suffix(ourdomain.length() + 1); // Remove our domain name plus the '.' before it

  std::string _domain = std::string(domain);
  if (domain.find('.') == domain.npos) {
    // No TLD, assume Base16
    _domain = from_base16(domain);
    if (_domain.empty()) {
      resp.SetResponseCode(400);
      resp.SetBody("Could not decode hex-encoded URL, please check your settings");
    }
  }

  resp.SetHeader("Location", "http://" + _domain);
  resp.SetResponseCode(301);
}

// =====Scaffolding=====
void process_mem_usage(double &vm_usage, double &resident_set) {
  vm_usage = 0.0;
  resident_set = 0.0;

  // the two fields we want
  unsigned long vsize;
  long rss;
  {
    std::string ignore;
    std::ifstream ifs("/proc/self/stat", std::ios_base::in);
    ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
        ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
        ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
        ignore >> vsize >> rss;
  }

  long page_size_kb = sysconf(_SC_PAGE_SIZE) /
                      1024; // in case x86-64 is configured to use 2MB pages
  vm_usage = vsize / 1024.0;
  resident_set = rss * page_size_kb;
}

int main() {
  Server s(
      8080,
      {{std::regex("/"), indexHandler}, {std::regex("/.*"), fallbackHandler}},
      "server.log");

  double vm_u = 0, rss = 0;
  uint32_t cnter = 0;
  while (true) {
    s.PollAccept(10000);
    s.PollGC();
    if (cnter % 300 == 0) {
      process_mem_usage(vm_u, rss);
      // TODO: create status page instead of this
      std::cout << "Virtual Memory: " << vm_u << "\nResident set size: " << rss
                << std::endl;
    }
    cnter++;
  }
}
