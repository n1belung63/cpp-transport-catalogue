#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <optional>

#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json.h"

namespace transport_catalogue {
    class JsonReader {
    private:
        using RequestBody = std::variant<std::monostate,std::string,std::tuple<std::string,std::string>>;

        struct BaseRequest {
            RequestType type;
            std::optional<json::Dict> raw_request_body;
        };

        struct StatRequest {
            int id;
            RequestType type;
            std::optional<RequestBody> request_body;
        };

    public:
        JsonReader() = default;

        void ParseRequests(std::istream& inp);

        BaseRequestDTO GetBaseRequest(size_t i);
        size_t GetBaseRequestCount() const;

        StatRequestDTO GetStatRequest(size_t i);
        size_t GetStatRequestCount() const;

        RenderSettings GetRendererSettings();
        RoutingSettings GetRoutingSettings();

        void AddResponse(const StatResponseDTO& resp);

        void PrintResponses(std::ostream& out);
    private:
        std::deque<BaseRequest> base_requests_storage_;
        std::deque<StatRequest> stat_requests_storage_;
        json::Dict render_settings_;
        json::Dict routing_settings_;
        json::Array resp_;

        std::unordered_map<int, RequestType> request_id_to_type_;

        BaseRequest ResolveBaseRequest(const json::Dict& request); 
        StatRequest ResolveStatRequest(const json::Dict& request);
          
        Stop ParseStop(const json::Dict& request);
        Bus ParseBus(const json::Dict& request);
        RenderSettings ParseRendererSettings(const json::Dict& settings);
    };
}