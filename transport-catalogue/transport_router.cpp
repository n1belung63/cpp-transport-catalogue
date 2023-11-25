#include "transport_router.h"

#include <string>
#include <set>
#include <string_view>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace transport_catalogue {
    using namespace std::string_literals;

    void TransportRouter::SetUp(const RoutingSettings& routing_settings) {
        constexpr double SECONDS_IN_MINUTE = 60.0;
        constexpr double METERS_IN_KILOMETER = 1000.0;

        routing_settings_ = routing_settings;

        const auto stop_names = tc_.GetStopList();
        const auto bus_names = tc_.GetBusList();
        graph_.SetVertexCount(stop_names.size() * 2);

        const double meters_to_minutes_multiplier = 1.0 / METERS_IN_KILOMETER / routing_settings_.bus_velocity * SECONDS_IN_MINUTE;
        
        auto update_stopname_to_vertex_id_pair {
            [this](const std::string& stop_name, graph::VertexId & id) {
                stopname_to_vertex_id_pair_[stop_name] = { id, id + 1 };
                id += 2;
            } 
        };

        auto abs_of_size_t_diff {[](auto a, auto b) { return (a > b) ? a - b : b - a; } };

        graph::VertexId id = 0;
        for (const auto& bus_num : bus_names) {
            const Bus* bus_ref = tc_.FindBusV2(bus_num);

            for (const auto& stop_name : bus_ref->stopnames) {
                if (stopname_to_vertex_id_pair_.count(stop_name) < 1) {
                    update_stopname_to_vertex_id_pair(stop_name, id);
                    graph::EdgeId edge_id = graph_.AddEdge({ 
                        stopname_to_vertex_id_pair_.at(stop_name).wait_on_stop_id,
                        stopname_to_vertex_id_pair_.at(stop_name).stop_id,
                        static_cast<double>(routing_settings_.bus_wait_time)
                    });
                    edge_id_to_route_item_[edge_id] = WaitItem{ stop_name, static_cast<double>(routing_settings_.bus_wait_time) }; 
                }
            }

            size_t stops_count = bus_ref->stopnames.size();
            size_t total_stops_count = (bus_ref->is_circular_route) ? stops_count : 2 * stops_count - 1;

            size_t from, to, prev_to;
            size_t from_transparent = 0;
            while (from_transparent < total_stops_count - 1) {
                size_t to_transparent = from_transparent + 1;
                from = (from_transparent < stops_count) ? from_transparent : total_stops_count - 1 - from_transparent;

                double time = 0.0;
                double running_error = 0.0;

                while (to_transparent < total_stops_count) {
                    to = (to_transparent < stops_count) ? to_transparent : total_stops_count - 1 - to_transparent;
                    prev_to = (to_transparent < stops_count) ? to - 1 : to + 1;

                    double difference = (tc_.GetDistance(bus_ref->stopnames[prev_to], bus_ref->stopnames[to]) * meters_to_minutes_multiplier) - running_error;
                    double temp = time + difference;
                    running_error = (temp - time) - difference;
                    time = temp;
                    
                    // time += d_time;

                    graph::EdgeId edge_id = graph_.AddEdge({ 
                        stopname_to_vertex_id_pair_.at(bus_ref->stopnames[from]).stop_id,
                        stopname_to_vertex_id_pair_.at(bus_ref->stopnames[to]).wait_on_stop_id,
                        time
                    });
                    size_t span_count = abs_of_size_t_diff(to, from);
                    edge_id_to_route_item_[edge_id] = BusItem{ bus_ref->num, span_count, time };

                    ++to_transparent;
                }
                ++from_transparent;
            }
        }
        router_.Update();
    }

    RouteInfo TransportRouter::GetRoute(std::tuple<std::string_view,std::string_view> from_to) {
        if (stopname_to_vertex_id_pair_.count(std::get<0>(from_to)) < 1 || stopname_to_vertex_id_pair_.count(std::get<1>(from_to)) < 1) {
            throw std::out_of_range("not found"s);
        }

        graph::VertexId from = stopname_to_vertex_id_pair_[std::get<0>(from_to)].wait_on_stop_id;
        graph::VertexId to = stopname_to_vertex_id_pair_[std::get<1>(from_to)].wait_on_stop_id;
        const auto raw_route_info = router_.BuildRoute(from, to);

        RouteInfo route_info;
        if (raw_route_info.has_value()) {
            route_info.total_time = raw_route_info.value().weight;

            route_info.items.reserve(raw_route_info.value().edges.size());
            for (graph::EdgeId edge_id : raw_route_info.value().edges) {
                route_info.items.emplace_back(edge_id_to_route_item_.at(edge_id));         
            }
        } else {
            throw std::out_of_range("not found"s);
        }

        return route_info;
    }
}