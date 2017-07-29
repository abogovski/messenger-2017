#include "request_handler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "reply.hpp"
#include "request.hpp"

namespace http {
namespace server {

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  const std::string strget = "/get/";
  // Request path must be absolute and not contain ".." or and match pattern /get/<file_name>
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos
      || request_path[request_path.size() - 1] == '/'
      || std::mismatch(strget.begin(), strget.end(), request_path.begin()).first != strget.end())
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Open the file to send back.
  std::string fname = "./" + request_path.substr(strget.size());
  std::ifstream is(fname.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  { 
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, is.gcount());
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "application/octet-stream";
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server
} // namespace http
