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

    enum class RouteItemType { Wait, Stop };

    struct WaitItem {
        std::string stop_name;
        double time;
    };

    struct BusItem {
        std::string bus;
        size_t span_count;
        double time;
    };
    
    using RouteItem = std::variant<std::monostate, WaitItem, BusItem>;

    struct RouteInfo {
        double total_time;
        std::vector<RouteItem> items;
    };

        
    using StatRequestBodyDTO = std::variant<std::monostate,std::string_view,std::tuple<std::string_view,std::string_view>>;
    using StatResponseBodyDTO = std::variant<std::monostate, std::string, BusInfo, StopInfo, RouteInfo>;

    using BaseRequestDTO = std::optional<std::variant<std::monostate, Bus, Stop>>;
    using StatRequestDTO = std::optional<std::tuple<int,RequestType,std::optional<StatRequestBodyDTO>>>;
    using StatResponseDTO = std::optional<std::tuple<int,StatResponseBodyDTO>>;
}