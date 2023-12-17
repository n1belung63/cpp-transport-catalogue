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
            stopname_to_stop_.at(stop.name)->range_to_other_stop.insert(
                stop.range_to_other_stop.begin(), stop.range_to_other_stop.end()
            );
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

            if (stop_pair_to_distance_.count({to_ref, from_ref}) == 0) {
                stop_pair_to_distance_[{to_ref, from_ref}] = stop_pair_to_distance_.at({from_ref, to_ref});
            }
        }
    }

    const Stop TransportCatalogue::FindStop(std::string_view stopname) const {
        if (stopname_to_stop_.count(stopname)) {
            return *stopname_to_stop_.at(stopname);
        }
        else {
            throw std::out_of_range("not found"s);
        }
    }

    const Stop* TransportCatalogue::FindStopRef(std::string_view stopname) const {
        if (stopname_to_stop_.count(stopname)) {
            return stopname_to_stop_.at(stopname);
        }
        else {
            throw std::out_of_range("not found"s);
        }
    }

    StopInfo TransportCatalogue::GetStopInfo(std::string_view stopname) {
        // TransportCatalogue::FindStopRef(stopname);
        if (stopname_to_stop_.count(stopname) < 1) {
            throw std::out_of_range("not found"s);
        }

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

    const Bus TransportCatalogue::FindBus(std::string_view busnum) const {
        if (busname_to_bus_.count(busnum)) {
            return *busname_to_bus_.at(busnum);
        }
        else {
            throw std::out_of_range("not found"s);
        } 
    }

    const Bus* TransportCatalogue::FindBusRef(std::string_view busnum) const {
        if (busname_to_bus_.count(busnum)) {
            return busname_to_bus_.at(busnum);
        }
        else {
            throw std::out_of_range("not found"s);
        } 
    }

    double TransportCatalogue::GetDistance(std::string_view from_stop_name, std::string_view to_stop_name) const {
        auto from_ref = stopname_to_stop_.at(from_stop_name);
        auto to_ref = stopname_to_stop_.at(to_stop_name);

        return stop_pair_to_distance_.at({from_ref, to_ref}); 
    }

    BusInfo TransportCatalogue::GetBusInfo(std::string_view busnum) {
        const Bus* bus_ref = TransportCatalogue::FindBusRef(busnum);

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
            double d_route = GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
            businfo.route_length += d_route;

            direct_route_length += ComputeDistance(
                stopname_to_stop_.at(bus_ref->stopnames[from])->coords, 
                stopname_to_stop_.at(bus_ref->stopnames[to])->coords
            );
        }

        if (!bus_ref->is_circular_route) {
            for (int from=bus_ref->stopnames.size()-1, to=bus_ref->stopnames.size()-2; to>=0; from--, to--) {
                double d_route = GetDistance(bus_ref->stopnames[from], bus_ref->stopnames[to]);
                businfo.route_length += d_route;

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
        const Bus* bus_ref = TransportCatalogue::FindBusRef(busnum);

        BusExtendedInfo businfo;

        businfo.name = bus_ref->num;
        businfo.is_circular_route = bus_ref->is_circular_route;
        businfo.stops_and_coordinates.reserve(bus_ref->stopnames.size());

        for (const auto&  stop_name : bus_ref->stopnames) {
            const Stop* stop_ref = TransportCatalogue::FindStopRef(stop_name);
            businfo.stops_and_coordinates.push_back({stop_name, stop_ref->coords});
        }

        return businfo;
    }

    const std::vector<std::string_view> TransportCatalogue::GetBusList() const {
        std::vector<std::string_view> bus_names;
        bus_names.reserve(buses_.size());

        for (const auto& bus : buses_) {
            bus_names.emplace_back(bus.num);
        }

        return bus_names;
    }

    const std::vector<std::string_view> TransportCatalogue::GetStopList() const {
        std::vector<std::string_view> stop_names;
        stop_names.reserve(stops_.size());

        for (const auto& stop : stops_) {
            stop_names.emplace_back(stop.name);
        }

        return stop_names;
    }
}
  