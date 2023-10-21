#include "transport_catalogue.h"

#include <string>
#include <set>
#include <string_view>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace transport_catalogue {
    using namespace std::string_literals;

    TransportCatalogue::TransportCatalogue() { }

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

    StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) {
        Stop stop = TransportCatalogue::FindStop(stopname);

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

        return stop_pair_to_distance_[{from_ref, to_ref}]; 
    }

    BusInfo TransportCatalogue::GetBusInfo(std::string_view busnum) {
        Bus bus = TransportCatalogue::FindBus(busnum);

        BusInfo businfo;
        businfo.num = busnum;
        businfo.stops_count = bus.is_circular_route ? bus.stopnames.size() : 2 * bus.stopnames.size() - 1;

        Bus* bus_ref = busname_to_bus_.at(busnum);

        std::set<Stop*> unique_stops;
        for (const auto& stop_name : bus_ref->stopnames) {
            unique_stops.insert(stopname_to_stop_.at(stop_name));
        }
        businfo.unique_stops_count = unique_stops.size();

        double direct_route_length = 0.0;

        for (size_t from=0, to=1; to<bus.stopnames.size(); ++from, ++to) {
            SetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
            businfo.route_length += GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
            direct_route_length += ComputeDistance(stopname_to_stop_.at(bus_ref->stopnames[from])->coords, stopname_to_stop_.at(bus_ref->stopnames[to])->coords);
        }

        if (!bus.is_circular_route) {
            for (int from=bus.stopnames.size()-1, to=bus.stopnames.size()-2; to>=0; from--, to--) {
                SetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
                businfo.route_length += GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
                direct_route_length += ComputeDistance(stopname_to_stop_.at(bus_ref->stopnames[from])->coords, stopname_to_stop_.at(bus_ref->stopnames[to])->coords);
            }
        }

        businfo.route_curvature = businfo.route_length / direct_route_length;

        return businfo;
    }

    BusExtendedInfo TransportCatalogue::GetBusExtendedInfo(std::string_view busnum) {
        Bus bus = TransportCatalogue::FindBus(busnum);

        BusExtendedInfo businfo;

        businfo.name = bus.num;
        businfo.is_circular_route = bus.is_circular_route;
        businfo.stops_and_coordinates.reserve(bus.stopnames.size());

        for (const auto&  stop_name : bus.stopnames) {
            const Stop stop = TransportCatalogue::FindStop(stop_name);
            businfo.stops_and_coordinates.push_back({stop_name, stop.coords});
        }

        return businfo;
    }
}