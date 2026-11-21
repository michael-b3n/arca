#include <arcstd/web/web_request.hpp>

#include <boost/asio.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

namespace arcstd::web
{
namespace
{

using boost::asio::ip::tcp;

///
/// HTTP server on a free port of the loopback address, it answers every request with one response.
///
class local_server final
{
  // Variables
  const std::string response_;
  boost::asio::io_context context_;
  tcp::acceptor acceptor_;
  std::jthread thread_;

public: // Structors
  explicit local_server(std::string response);
  ~local_server() noexcept;

  local_server(const local_server&) = delete;
  auto operator=(const local_server&) -> local_server& = delete;

public: // Accessors
  ///
  /// \return the url of \p path on this server
  ///
  [[nodiscard]] auto url(std::string_view path = "/") const -> std::string;

private: // Implementation
  auto accept() -> void;
};

///
///
local_server::local_server(std::string response)
  : response_{std::move(response)}
  , acceptor_{context_, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0)}
{
  accept();
  thread_ = std::jthread{[this] { context_.run(); }};
}

///
///
// NOLINTNEXTLINE(bugprone-exception-escape) io_context::stop throws nothing
local_server::~local_server() noexcept
{
  context_.stop();
}

///
///
auto local_server::url(const std::string_view path) const -> std::string
{
  return std::format("http://127.0.0.1:{}{}", acceptor_.local_endpoint().port(), path);
}

///
///
auto local_server::accept() -> void
{
  acceptor_.async_accept(
    [this](const boost::system::error_code& error, tcp::socket accepted)
    {
      if(error)
      {
        return;
      }
      const auto socket = std::make_shared<tcp::socket>(std::move(accepted));
      const auto request = std::make_shared<std::string>();
      boost::asio::async_read_until(
        *socket,
        boost::asio::dynamic_buffer(*request),
        "\r\n\r\n",
        [this, socket, request](const boost::system::error_code& read_error, std::size_t)
        {
          if(!read_error)
          {
            boost::asio::async_write(
              *socket,
              boost::asio::buffer(response_),
              [socket](const boost::system::error_code&, std::size_t)
              {
                auto ignored = boost::system::error_code{};
                std::ignore = socket->shutdown(tcp::socket::shutdown_both, ignored);
              }
            );
          }
        }
      );
      accept();
    }
  );
}

///
/// \return an HTTP response with \p status and \p body
///
[[nodiscard]] auto response(const std::string_view status, const std::string_view body = {})
  -> std::string
{
  return std::format(
    "HTTP/1.1 {}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", status, body.size(), body
  );
}

} // namespace

TEST_CASE("web_request_fetches_a_page", "[web]")
{
  const auto server = local_server{response("200 OK", "<p>page</p>")};
  CHECK(web_request::fetch(server.url("/page.html")) == "<p>page</p>");
  // Asked again, answered again
  CHECK(web_request::fetch(server.url()) == "<p>page</p>");
}

TEST_CASE("web_request_tells_a_missing_page_from_a_failure", "[web]")
{
  using error_code = web_request::error_code;
  const auto missing = local_server{response("404 Not Found")};
  CHECK(web_request::fetch(missing.url()) == std::unexpected{error_code::not_found});
  const auto gone = local_server{response("410 Gone")};
  CHECK(web_request::fetch(gone.url()) == std::unexpected{error_code::not_found});
  const auto failing = local_server{response("500 Internal Server Error")};
  CHECK(web_request::fetch(failing.url()) == std::unexpected{error_code::request_failed});
}

TEST_CASE("web_request_fetches_web_pages_only", "[web]")
{
  using error_code = web_request::error_code;
  CHECK(
    web_request::fetch("file:///C:/Windows/win.ini") == std::unexpected{error_code::request_failed}
  );
  // Not even when redirected
  const auto redirect = local_server{
    "HTTP/1.1 302 Found\r\nLocation: file:///C:/Windows/win.ini\r\nContent-Length: 0\r\n"
    "Connection: close\r\n\r\n"
  };
  CHECK(web_request::fetch(redirect.url()) == std::unexpected{error_code::request_failed});
}

TEST_CASE("web_request_fails_without_a_server", "[web]")
{
  using error_code = web_request::error_code;
  const auto url = local_server{response("200 OK")}.url();
  CHECK(web_request::fetch(url) == std::unexpected{error_code::request_failed});
  CHECK(web_request::fetch("not a url") == std::unexpected{error_code::invalid_url});
  // A url curl takes for none
  CHECK(web_request::fetch("http://") == std::unexpected{error_code::invalid_url});
}

} // namespace arcstd::web
