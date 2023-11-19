#pragma once

#include "geo.h"

#include <vector>
#include <map>
#include <string>
#include <variant>
#include <optional>

namespace transport_catalogue {
    enum class RequestType {
        None,
        AddStop,
        FindStop,
        GetStopInfo,
        AddBus,
        FindBus,
        GetBusInfo,
        GetMap,
        GetRoute
    };

    struct Stop {
        std::string name;
        geo::Coordinates coords;
        std::map<std::string, double> range_to_other_stop;
    };

    struct Bus {
        bool is_circular_route = false;
        std::string num;
        std::vector<std::string> stopnames;
    };

    struct StopInfo {
        std::string name;
        std::vector<std::string> buses;
    };

    struct BusInfo {
        std::string num;
        int stops_count = 0;
        int unique_stops_count = 0;
        double route_length = 0.0;
        double route_curvature = 1.0;
    };

    struct BusExtendedInfo {
        bool is_circular_route = false;
        std::string name;
        std::vector<std::tuple<std::string, geo::Coordinates>> stops_and_coordinates;
    };

    struct RoutingSettings {
        double bus_velocity;
        int bus_wait_time;
    };
    
    using RequestBodyDTO = std::variant<std::monostate,std::string_view,std::pair<std::string_view,std::string_view>>;

    using BaseRequestDTO = std::optional<std::variant<std::monostate, Bus, Stop>>;
    using StatRequestDTO = std::optional<std::tuple<int,RequestType,std::optional<RequestBodyDTO>>>;
    using StatResponseDTO = std::optional<std::tuple<int,std::variant<std::monostate, std::string, BusInfo, StopInfo>>>;
}