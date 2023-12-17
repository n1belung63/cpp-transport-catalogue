#include "request_handler.h"

#include <iostream>

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

    void RequestHandler::BaseRequest(BaseRequestDTO base_request) {
        if (base_request.has_value()) {
            if (std::holds_alternative<Bus>(base_request.value())) {
                tc_.AddBus(std::get<Bus>(base_request.value()));
                buses_names_.emplace_back(std::get<Bus>(base_request.value()).num);
            }
            else if (std::holds_alternative<Stop>(base_request.value())) {
                tc_.AddStop(std::get<Stop>(base_request.value()));
            }
        } else {
            // todo: raise error
        }
        
    }

    StatResponseDTO RequestHandler::StatRequest(const StatRequestDTO& stat_request_opt) {
        if (!stat_request_opt.has_value()) {
            return std::nullopt;
        }
        const auto& stat_request = stat_request_opt.value();
        try {
            const auto request_body = std::get<std::optional<StatRequestBodyDTO>>(stat_request);
            switch (std::get<RequestType>(stat_request))
            {
            case RequestType::GetStopInfo:
                return std::tuple{ std::get<int>(stat_request), 
                    tc_.GetStopInfo(std::get<std::string_view>(request_body.value()))
                };
                break;
            case RequestType::GetBusInfo:
                return std::tuple{ std::get<int>(stat_request), 
                    tc_.GetBusInfo(std::get<std::string_view>(request_body.value()))
                };
                break;
            case RequestType::GetMap:
                return std::tuple{ std::get<int>(stat_request), RenderMap() };
                break;
            case RequestType::GetRoute:
                return std::tuple{ std::get<int>(stat_request),
                    transport_router_.GetRoute(std::get<std::tuple<std::string_view,std::string_view>>(request_body.value()))
                };
                break;
            default:
                return std::nullopt;
                break;
            }
        }
        catch (const std::exception& e) {
            std::string err_msg = e.what();
            return std::tuple{ std::get<int>(stat_request), err_msg };
        }
    }

    std::string RequestHandler::RenderMap() {
        for (const auto busnum : tc_.GetBusList()) {
            renderer_.SetBusExtendedInfo(tc_.GetBusExtendedInfo(busnum));
        }   
        std::stringstream ss;
        renderer_.Render(ss);
        return ss.str();  
    }
}