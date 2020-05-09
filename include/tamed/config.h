#pragma once

#include <array>
#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/string_body.hpp>


namespace tamed {

    /**
     *  Server configuration options
     */
    template <typename body = boost::beast::http::string_body, typename executor = boost::asio::io_context::executor_type, boost::beast::http::verb... verbs>
    struct config
    {
        /**
         *  The body type to use for incoming requests
         */
        using request_body_type = body;

        /**
         *  The executor type to use for registering
         *  asynchronous events
         */
        using executor_type = executor;

        /**
         *  The HTTP methods that are supported by the server
         */
        constexpr const static std::array<boost::beast::http::verb, sizeof...(verbs)> methods{ verbs... };

        /**
         *  Select a different body type for incoming
         *  requests
         */
        template <typename body_type>
        using with_body_type = config<body_type, executor, verbs...>;

        /**
         *  Select a different executor type
         *  for registering asynchronous events
         */
        template <typename executor_type>
        using with_executor_type = config<body, executor_type, verbs...>;

        /**
         *  Select a different set of supported
         *  request methods
         */
        template <boost::beast::http::verb... methods>
        using with_methods = config<body, executor, methods...>;
    };

    /**
     *  Pre-defined server config for a simple REST server
     */
    using rest_config = config<>
        ::with_body_type<boost::beast::http::string_body>
        ::with_executor_type<boost::asio::io_context::executor_type>
        ::with_methods<
            boost::beast::http::verb::get,
            boost::beast::http::verb::post,
            boost::beast::http::verb::put,
            boost::beast::http::verb::delete_
        >;

    /**
     *  Pre-defined server config for a WebDAV server
     */
    using webdav_config = config<>
        ::with_body_type<boost::beast::http::string_body>
        ::with_executor_type<boost::asio::io_context::executor_type>
        ::with_methods<
            boost::beast::http::verb::get,
            boost::beast::http::verb::head,
            boost::beast::http::verb::post,
            boost::beast::http::verb::put,
            boost::beast::http::verb::delete_,
            boost::beast::http::verb::move,
            boost::beast::http::verb::copy,
            boost::beast::http::verb::lock,
            boost::beast::http::verb::unlock,
            boost::beast::http::verb::mkcol,
            boost::beast::http::verb::options,
            boost::beast::http::verb::propfind,
            boost::beast::http::verb::proppatch
        >;

}
