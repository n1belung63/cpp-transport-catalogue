#include "request_handler.h"

namespace transport_catalogue {
    using namespace std::string_literals;

    std::ostream& operator<<(std::ostream& out, const BusInfo& businfo) {
        out << businfo.stops_count << " stops on route, "s 
            << businfo.unique_stops_count << " unique stops, "s
            << businfo.route_length << " route length, "s
            << businfo.route_curvature << " curvature"s;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const StopInfo& stopinfo) {
        out << "buses"s;
        for (const auto& bus : stopinfo.buses) {
            out << " " << bus;
        }
        return out;
    }

    void RequestHandler::BaseRequest(std::variant<Bus, Stop> base_request) {
        if (std::holds_alternative<Bus>(base_request)) {
            tc_.AddBus(std::get<Bus>(base_request));
            buses_names_.emplace_back(std::get<Bus>(base_request).num);
        }
        else if (std::holds_alternative<Stop>(base_request)) {
            tc_.AddStop(std::get<Stop>(base_request));
        }
    }

    RequestHandler::StatResponse RequestHandler::StatRequest(std::tuple<QueryType,int,std::string_view> stat_request) {
        try {
            switch (std::get<QueryType>(stat_request))
            {
            case QueryType::GetStopInfo:
                return { std::get<int>(stat_request), tc_.GetStopInfo(std::get<std::string_view>(stat_request)) };
                break;
            case QueryType::GetBusInfo:
                return { std::get<int>(stat_request), tc_.GetBusInfo(std::get<std::string_view>(stat_request)) };
                break;
            case QueryType::GetMap: {
                    std::stringstream ss;
                    SendDataToRenderer();    
                    renderer_.Render(ss);
                    return { std::get<int>(stat_request), ss.str() };
                }
                break;    
            default:
                return { std::get<int>(stat_request), "No resp"s };
                break;
            }
        }
        catch (const std::exception& e) {
            std::string err_msg = e.what();
            return { std::get<int>(stat_request), err_msg };
        }
    }

    void RequestHandler::SendDataToRenderer() {
        for (const auto& busnum : buses_names_) {
            renderer_.SetBusExtendedInfo(tc_.GetBusExtendedInfo(busnum));
        }        
    }
}