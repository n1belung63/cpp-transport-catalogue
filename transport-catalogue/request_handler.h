#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <optional>
#include <variant>
#include <tuple>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "domain.h"

namespace transport_catalogue {
    std::ostream& operator<<(std::ostream& out, const BusInfo& businfo);
    std::ostream& operator<<(std::ostream& out, const StopInfo& stopinfo);

    class RequestHandler {
    public:
        RequestHandler(TransportCatalogue& db, transport_catalogue::MapRenderer& renderer) : tc_(db), renderer_(renderer) { }
        void BaseRequest(BaseRequestDTO base_request);
        StatResponseDTO StatRequest(const StatRequestDTO& stat_request);
        void BuildGraph(const RoutingSettings& routing_settings);
    private:
        TransportCatalogue& tc_;
        MapRenderer& renderer_;
        std::deque<std::string> buses_names_;

        std::string RenderMap();
    };

}