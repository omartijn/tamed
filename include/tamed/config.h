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

}
