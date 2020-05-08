#include <iostream>

#include <tamed/server.h>
#include <boost/asio/ip/tcp.hpp>


void handle_slash(tamed::connection connection, boost::beast::http::request<boost::beast::http::string_body>&& request)
{
    boost::beast::http::response<boost::beast::http::string_body>   response{ boost::beast::http::status::ok, request.version() };

    response.body().assign("Hello, world!");
    connection.send(std::move(response));
}

void handle_not_found(tamed::connection connection, boost::beast::http::request<boost::beast::http::string_body>&& request)
{
    boost::beast::http::response<boost::beast::http::string_body>   response{ boost::beast::http::status::not_found, request.version() };

    response.body().assign("The requested resource was not found");
    connection.send(std::move(response));
}


int main()
{
    using server_type = tamed::server<boost::beast::http::string_body, boost::asio::executor,
        boost::beast::http::verb::get,
        boost::beast::http::verb::post,
        boost::beast::http::verb::put
    >;

    boost::asio::io_context         io_context  {                                                   };
    server_type                     server      { io_context.get_executor()                         };
    boost::asio::ip::tcp::endpoint  endpoint    { boost::asio::ip::make_address("127.0.0.1"), 8080  };

    server.add<handle_slash>(boost::beast::http::verb::get, "/");
    server.add<handle_slash>(boost::beast::http::verb::get, "");

    server.set_not_found<handle_not_found>();

    server.listen(endpoint);
    io_context.run();

    return 0;
}
