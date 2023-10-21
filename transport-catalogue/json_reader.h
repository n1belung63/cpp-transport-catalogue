#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <variant>
#include <tuple>
#include <unordered_map>

#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json.h"

namespace transport_catalogue { 
    struct Query {
        QueryType type;
        json::Dict query_body;
    };

    class JsonReader {
    public:
        JsonReader() = default;

        void ParseRequests(std::istream& inp);

        using BaseRequest = std::variant<Bus, Stop>;
        BaseRequest GetBaseRequest(size_t i);
        size_t GetBaseRequestCount() const;

        using StatRequest = std::tuple<QueryType,int,std::string_view>;
        StatRequest GetStatRequest(size_t i);
        size_t GetStatRequestCount() const;

        RenderSettings GetRendererSettings();

        void AddResponse(const std::tuple<int,std::variant<std::string, BusInfo, StopInfo>>& resp);

        void PrintResponses(std::ostream& out);
    private:
        std::deque<Query> base_requests_storage_;
        std::deque<std::tuple<QueryType,int,std::string>> stat_requests_storage_;
        json::Dict settings_;
        json::Array resp_;

        std::unordered_map<int, QueryType> query_id_to_type_;

        Query ResolveQuery(const json::Dict& query);      
        Stop ParseStop(const json::Dict& query);
        Bus ParseBus(const json::Dict& query);
        RenderSettings ParseRendererSettings(const json::Dict& settings);
    };
}