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

namespace transport_catalogue {
    enum class QueryType {
        None,
        AddStop,
        FindStop,
        GetStopInfo,
        AddBus,
        FindBus,
        GetBusInfo,
        GetMap
    };

    std::ostream& operator<<(std::ostream& out, const BusInfo& businfo);
    std::ostream& operator<<(std::ostream& out, const StopInfo& stopinfo);

    class RequestHandler {
    public:
        RequestHandler(TransportCatalogue& db, transport_catalogue::MapRenderer& renderer) : tc_(db), renderer_(renderer) { }

        void BaseRequest(std::variant<Bus, Stop> base_request);

        using StatResponse = std::tuple<int,std::variant<std::string, BusInfo, StopInfo>>;
        StatResponse StatRequest(std::tuple<QueryType,int,std::string_view> stat_request);
    private:
        TransportCatalogue& tc_;
        MapRenderer& renderer_;
        std::deque<std::string> buses_names_;

        void SendDataToRenderer();
    };

}