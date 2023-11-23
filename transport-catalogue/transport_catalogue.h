#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

#include "domain.h"
#include "router.h"

namespace transport_catalogue {
    class TransportCatalogue {
    public:
        explicit TransportCatalogue();

        void AddStop(const Stop& stop);
        StopInfo GetStopInfo(std::string_view stopname);

        void AddBus(const Bus& bus);
        BusInfo GetBusInfo(std::string_view busnum);

        BusExtendedInfo GetBusExtendedInfo(std::string_view busnum);

        void BuildGraph(const RoutingSettings& routing_settings);

        RouteInfo GetRoute(std::tuple<std::string_view,std::string_view> from_to);

    private:
        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
        std::deque<Bus> buses_;
        std::unordered_map<std::string_view, Bus*> busname_to_bus_;

        RoutingSettings routing_settings_;

        // using StopVertex = std::pair<RouteItemType, std::string>;
        graph::DirectedWeightedGraph<double> graph_;
        graph::Router<double> router_;

        struct StopItemPair { 
            graph::VertexId wait_on_stop_id;
            graph::VertexId stop_id;
        };

        std::map<std::string_view, StopItemPair> stopname_to_vertex_id_pair_;
        std::map<graph::VertexId, RouteItem> edge_id_to_route_item_;

        struct StopPairHasher {
            size_t operator() (std::pair<Stop*, Stop*> pair) const {          
                size_t h_first = u_hasher_((uint64_t)pair.first);
                size_t h_second = u_hasher_((uint64_t)pair.second);        
                return h_first + h_second * 37;
            }
        private:
            std::hash<uint64_t> u_hasher_;
        };

        std::unordered_map<std::pair<Stop*, Stop*>, double, StopPairHasher> stop_pair_to_distance_;
        std::unordered_map<Stop*, std::unordered_set<Bus*>> stop_to_buses_;

        Stop FindStop(std::string_view stopname);
        Stop* FindStopV2(std::string_view stopname);
        Bus FindBus(std::string_view busnum);
        Bus* FindBusV2(std::string_view busnum);
        void AddDummyStop(std::string_view stopname);
        void SetDistance(std::string_view from_stop_name, std::string_view to_stop_name, double distance=0);
        double GetDistance(std::string_view from_stop_name, std::string_view to_stop_name);
    };
}

