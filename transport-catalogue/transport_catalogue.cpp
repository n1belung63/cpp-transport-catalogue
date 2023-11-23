#include "transport_catalogue.h"

#include <string>
#include <set>
#include <string_view>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace transport_catalogue {
    using namespace std::string_literals;

    TransportCatalogue::TransportCatalogue() : router_(graph_) { }

    void TransportCatalogue::AddDummyStop(std::string_view stopname) {
        Stop stop{};
        stop.name = stopname;
        stops_.emplace_back(stop);
        stopname_to_stop_[stops_.back().name] = &stops_.back();
    }

    void TransportCatalogue::AddStop(const Stop& stop) {
        if (stopname_to_stop_.count(stop.name)) {
            stopname_to_stop_.at(stop.name)->coords = { stop.coords.lat, stop.coords.lng };
        }
        else {
            stops_.emplace_back(stop);
            stopname_to_stop_[stops_.back().name] = &stops_.back();
        }

        for (const auto & [stopname, range] : stop.range_to_other_stop) {
            auto from_ref = stopname_to_stop_.at(stop.name);
            if (stopname_to_stop_.count(stopname) == 0) {
                AddDummyStop(stopname);
            }
            auto to_ref = stopname_to_stop_.at(stopname);

            stop_pair_to_distance_[{from_ref,to_ref}] = static_cast<double>(range);
        }
    }

    Stop TransportCatalogue::FindStop(std::string_view stopname) {
        if (stopname_to_stop_.count(stopname)) {
            return *stopname_to_stop_.at(stopname);
        }
        else {
            throw std::out_of_range("not found"s);
        }
    }

    Stop* TransportCatalogue::FindStopV2(std::string_view stopname) {
        if (stopname_to_stop_.count(stopname)) {
            return stopname_to_stop_.at(stopname);
        }
        else {
            throw std::out_of_range("not found"s);
        }
    }

    StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) {
        TransportCatalogue::FindStopV2(stopname);

        StopInfo stopinfo;
        stopinfo.name = stopname;

        if (stop_to_buses_.count(stopname_to_stop_.at(stopinfo.name)) == 0) {
            return stopinfo;
        }

        stopinfo.buses.reserve(stop_to_buses_.at(stopname_to_stop_.at(stopinfo.name)).size());

        for (const auto& bus_ref : stop_to_buses_.at(stopname_to_stop_.at(stopinfo.name))) {
            stopinfo.buses.emplace_back(bus_ref->num);
        }

        std::sort(stopinfo.buses.begin(), stopinfo.buses.end());

        return stopinfo;
    }
    
    void TransportCatalogue::AddBus(const Bus& bus) {
        buses_.emplace_back(bus);
        busname_to_bus_[buses_.back().num] = &buses_.back();

        for (std::string_view stopname : bus.stopnames) {
            if (stopname_to_stop_.count(stopname) == 0) {
                AddDummyStop(stopname);
            }

            stop_to_buses_[stopname_to_stop_.at(stopname)].insert(&buses_.back());
        }
    }

    Bus TransportCatalogue::FindBus(std::string_view busnum) {
        if (busname_to_bus_.count(busnum)) {
            return *busname_to_bus_.at(busnum);
        }
        else {
            throw std::out_of_range("not found"s);
        } 
    }

    Bus* TransportCatalogue::FindBusV2(std::string_view busnum) {
        if (busname_to_bus_.count(busnum)) {
            return busname_to_bus_.at(busnum);
        }
        else {
            throw std::out_of_range("not found"s);
        } 
    }

    void TransportCatalogue::SetDistance(std::string_view from_stop_name, std::string_view to_stop_name, double distance) {
        auto from_ref = stopname_to_stop_.at(from_stop_name);
        auto to_ref = stopname_to_stop_.at(to_stop_name);

        if (distance != 0) {
            stop_pair_to_distance_[{from_ref, to_ref}] = distance;
        }
        else {
            if (stop_pair_to_distance_.count({from_ref, to_ref}) == 0) {
                if (stop_pair_to_distance_.count({to_ref, from_ref}) == 0) {
                    stop_pair_to_distance_[{from_ref, to_ref}] = ComputeDistance(
                        from_ref->coords,
                        to_ref->coords
                    );
                }
                else {
                    stop_pair_to_distance_[{from_ref, to_ref}] = stop_pair_to_distance_[{to_ref, from_ref}];
                }
            }
        }  
    }

    double TransportCatalogue::GetDistance(std::string_view from_stop_name, std::string_view to_stop_name) {
        auto from_ref = stopname_to_stop_.at(from_stop_name);
        auto to_ref = stopname_to_stop_.at(to_stop_name);

        return stop_pair_to_distance_.at({from_ref, to_ref}); 
    }

    BusInfo TransportCatalogue::GetBusInfo(std::string_view busnum) {
        Bus* bus_ref = TransportCatalogue::FindBusV2(busnum);

        BusInfo businfo;
        businfo.num = busnum;
        businfo.stops_count = bus_ref->is_circular_route ? bus_ref->stopnames.size() : 2 * bus_ref->stopnames.size() - 1;

        std::set<Stop*> unique_stops;
        for (const auto& stop_name : bus_ref->stopnames) {           
            unique_stops.insert(stopname_to_stop_.at(stop_name));
        }
        businfo.unique_stops_count = unique_stops.size();

        double direct_route_length = 0.0;

        for (size_t from=0, to=1; to<bus_ref->stopnames.size(); ++from, ++to) {
            SetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
            businfo.route_length += GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);

            direct_route_length += ComputeDistance(
                stopname_to_stop_.at(bus_ref->stopnames[from])->coords, 
                stopname_to_stop_.at(bus_ref->stopnames[to])->coords
            );
        }

        if (!bus_ref->is_circular_route) {
            for (int from=bus_ref->stopnames.size()-1, to=bus_ref->stopnames.size()-2; to>=0; from--, to--) {
                SetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
                businfo.route_length += GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);

                direct_route_length += ComputeDistance(
                    stopname_to_stop_.at(bus_ref->stopnames[from])->coords,
                    stopname_to_stop_.at(bus_ref->stopnames[to])->coords
                );
            }
        }

        businfo.route_curvature = businfo.route_length / direct_route_length;

        return businfo;
    }

    BusExtendedInfo TransportCatalogue::GetBusExtendedInfo(std::string_view busnum) {
        Bus* bus_ref = TransportCatalogue::FindBusV2(busnum);

        BusExtendedInfo businfo;

        businfo.name = bus_ref->num;
        businfo.is_circular_route = bus_ref->is_circular_route;
        businfo.stops_and_coordinates.reserve(bus_ref->stopnames.size());

        for (const auto&  stop_name : bus_ref->stopnames) {
            Stop* stop_ref = TransportCatalogue::FindStopV2(stop_name);
            businfo.stops_and_coordinates.push_back({stop_name, stop_ref->coords});
        }

        return businfo;
    }

    void TransportCatalogue::BuildGraph(const RoutingSettings& routing_settings) {
        constexpr double SECONDS_IN_MINUTE = 60.0;
        constexpr double METERS_IN_KILOMETER = 1000.0;

        routing_settings_ = routing_settings;
        graph_.SetVertexCount(stops_.size() * 2);

        const double meters_to_minutes_multiplier = 1.0 / METERS_IN_KILOMETER / routing_settings_.bus_velocity * SECONDS_IN_MINUTE;
        
        auto update_stopname_to_vertex_id_pair {
            [this](const std::string& stop_name, graph::VertexId & id) {
                stopname_to_vertex_id_pair_[stop_name] = { id, id + 1 };
                id += 2;
            } 
        };

        auto abs_of_size_t_diff {[](auto a, auto b) { return (a > b) ? a - b : b - a; } };

        graph::VertexId id = 0;
        for (const Bus& bus : buses_) {
            Bus* bus_ref = busname_to_bus_.at(bus.num);

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

            size_t stops_count = bus.stopnames.size();
            size_t total_stops_count = (bus.is_circular_route) ? stops_count : 2 * stops_count - 1;

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

                    SetDistance(bus_ref->stopnames[prev_to], bus_ref->stopnames[to]);

                    // double difference = (GetDistance(bus_ref->stopnames[prev_to], bus_ref->stopnames[to])) - running_error;
                    // double temp = time + difference;
                    // running_error = (temp - time) - difference;
                    // time = temp;

                    time += GetDistance(bus_ref->stopnames[prev_to], bus_ref->stopnames[to]);

                    graph::EdgeId edge_id = graph_.AddEdge({ 
                        stopname_to_vertex_id_pair_.at(bus_ref->stopnames[from]).stop_id,
                        stopname_to_vertex_id_pair_.at(bus_ref->stopnames[to]).wait_on_stop_id,
                        time * meters_to_minutes_multiplier
                    });
                    size_t span_count = abs_of_size_t_diff(to, from);
                    edge_id_to_route_item_[edge_id] = BusItem{ bus.num, span_count, time * meters_to_minutes_multiplier };

                    ++to_transparent;
                }
                ++from_transparent;
            }
        }
        router_.Update();
    }

    RouteInfo TransportCatalogue::GetRoute(std::tuple<std::string_view,std::string_view> from_to) {
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
  