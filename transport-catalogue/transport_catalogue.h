#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

#include "domain.h"

namespace transport_catalogue {
    class TransportCatalogue {
    public:
        explicit TransportCatalogue();

        void AddStop(const Stop& stop);
        StopInfo GetStopInfo(std::string_view stopname);
        void AddBus(const Bus& bus);
        BusInfo GetBusInfo(std::string_view busnum);
        BusExtendedInfo GetBusExtendedInfo(std::string_view busnum);

    private:
        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
        std::deque<Bus> buses_;
        std::unordered_map<std::string_view, Bus*> busname_to_bus_;

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
        Bus FindBus(std::string_view busnum);
        void AddDummyStop(std::string_view stopname);
        void SetDistance(std::string_view from_stop_name, std::string_view to_stop_name, double distance=0);
        double GetDistance(std::string_view from_stop_name, std::string_view to_stop_name);
    };

    namespace detail {
        std::vector<std::string_view> SplitStopNames(std::string_view str, char delimeter);
    }
}

