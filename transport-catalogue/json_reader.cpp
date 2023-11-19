#include "json_reader.h"
#include "json_builder.h"

namespace transport_catalogue {
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    void JsonReader::ParseRequests(std::istream& inp) {
        json::Document doc = json::Load(inp);
        const json::Dict& json_obj = doc.GetRoot().AsMap();

        if (json_obj.count("base_requests"s)) {
            for (const auto& raw_request : json_obj.at("base_requests"s).AsArray()) {
                base_requests_storage_.push_back(ResolveBaseRequest(raw_request.AsMap()));
            }
        }   

        if (json_obj.count("stat_requests"s)) {
            for (const auto& raw_request : json_obj.at("stat_requests"s).AsArray()) {         
                const JsonReader::StatRequest stat_request = ResolveStatRequest(raw_request.AsMap());
                stat_requests_storage_.push_back(stat_request);
                request_id_to_type_[stat_request.id] = stat_request.type;
            }
        }       

        if (json_obj.count("render_settings"s)) {
            render_settings_ = json_obj.at("render_settings"s).AsMap();
        }

        if (json_obj.count("routing_settings"s)) {
            routing_settings_ = json_obj.at("routing_settings"s).AsMap();
        }  
    }

    size_t JsonReader::GetBaseRequestCount() const {
        return base_requests_storage_.size();
    }

    BaseRequestDTO JsonReader::GetBaseRequest(size_t i) {
        const auto base_request = base_requests_storage_.at(i);

        switch (base_request.type) {
        case RequestType::AddStop:
            return ParseStop(base_request.raw_request_body.value());
            break;
        case RequestType::AddBus:
            return ParseBus(base_request.raw_request_body.value());
            break;
        default:
            return std::nullopt;
            break;
        }
    }

    size_t JsonReader::GetStatRequestCount() const {
        return stat_requests_storage_.size();
    }

    StatRequestDTO JsonReader::GetStatRequest(size_t i) {
        const auto stat_request = stat_requests_storage_.at(i);
        switch (stat_request.type) {
        case RequestType::GetStopInfo:
            return std::tuple{ stat_request.id, 
                stat_request.type, 
                std::get<std::string>(stat_request.request_body.value()) 
            };
            break;
        case RequestType::GetBusInfo:
            return std::tuple{ stat_request.id,
                stat_request.type,
                std::get<std::string>(stat_request.request_body.value())
            };
            break;
        case RequestType::GetMap:
            return std::tuple{ stat_request.id, stat_request.type, std::nullopt };
            break;
        case RequestType::GetRoute:
            return std::tuple{ stat_request.id, stat_request.type, std::nullopt };
            break; 
        default:
            return std::nullopt;
            break;
        }     
    }

    void JsonReader::AddResponse(const StatResponseDTO& resp_opt) {
        if (!resp_opt.has_value()) {
            return;
        }

        const auto& resp = resp_opt.value();
        const int request_id = std::get<int>(resp);

        if (request_id_to_type_.count(request_id) < 1) {
            return;
        }
   
        const auto info = std::get<std::variant<std::monostate, std::string, BusInfo, StopInfo>>(resp);
        
        if (request_id_to_type_.at(request_id) == RequestType::GetBusInfo) {
            if (std::holds_alternative<BusInfo>(info)) {
                const auto bus_info = std::get<BusInfo>(info);
                json::Node dict = json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(request_id)
                        .Key("curvature"s).Value(bus_info.route_curvature)
                        .Key("route_length"s).Value(static_cast<int>(bus_info.route_length))
                        .Key("stop_count"s).Value(bus_info.stops_count)
                        .Key("unique_stop_count"s).Value(bus_info.unique_stops_count)
                    .EndDict()
                    .Build();
                resp_.emplace_back(dict);
            }
            else {
                json::Node dict = json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(request_id)
                        .Key("error_message"s).Value(std::get<std::string>(info))
                    .EndDict()
                    .Build();
                resp_.emplace_back(dict);
            } 
        }
        else if (request_id_to_type_.at(request_id) == RequestType::GetStopInfo) {
            if (std::holds_alternative<StopInfo>(info)) {
                const auto stop_info = std::get<StopInfo>(info);
                json::Array buses;
                for (std::string bus : stop_info.buses) {
                    buses.emplace_back(bus);
                }
                json::Node dict = json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(request_id)
                        .Key("buses"s).Value(buses)
                    .EndDict()
                    .Build();
                resp_.emplace_back(dict);
            }
            else {
                json::Node dict = json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(request_id)
                        .Key("error_message"s).Value(std::get<std::string>(info))
                    .EndDict()
                    .Build();
                resp_.emplace_back(dict);
            }
        }
        else if (request_id_to_type_.at(request_id) == RequestType::GetMap) {
            json::Node dict = json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(request_id)
                        .Key("map"s).Value(std::get<std::string>(info))
                    .EndDict()
                    .Build();
            resp_.emplace_back(dict);
        }        
    }

    void JsonReader::PrintResponses(std::ostream& out) {
        json::Print(json::Document{resp_}, out);
    }

    RenderSettings JsonReader::GetRendererSettings() {
        return ParseRendererSettings(render_settings_);
    }

    RoutingSettings JsonReader::GetRoutingSettings() {
        return { 
            routing_settings_.at("bus_velocity").AsDouble(),
            routing_settings_.at("bus_wait_time").AsInt()
        };
    }

    Stop JsonReader::ParseStop(const json::Dict& request) {
        Stop stop;
        stop.name = request.at("name"s).AsString();
        stop.coords = { request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble() };
        for (const auto& [key, value] : request.at("road_distances"s).AsMap()) {
            stop.range_to_other_stop[key] = value.AsDouble();
        }
        return stop;
    }

    Bus JsonReader::ParseBus(const json::Dict& request) {
        Bus bus;
        bus.num = request.at("name"s).AsString();
        bus.is_circular_route = request.at("is_roundtrip"s).AsBool();
        json::Array stops = request.at("stops"s).AsArray();
        bus.stopnames.reserve(stops.size());
        for (const auto& stop : stops) {
            bus.stopnames.push_back(stop.AsString());
        }
        return bus;
    }

    std::string ColorNodeToString(const json::Node& color_node) {
        if (color_node.IsArray()) {
            json::Array color = color_node.AsArray();
            std::stringstream ss;
            if (color.size() == 4) {
                ss << svg::Rgba(color.at(0).AsInt(), color.at(1).AsInt(), color.at(2).AsInt(), color.at(3).AsDouble());
            }
            else if (color.size() == 3) {
                ss << svg::Rgb(color.at(0).AsInt(), color.at(1).AsInt(), color.at(2).AsInt());
            }
            return ss.str();
        }
        else if (color_node.IsString()) {
            return color_node.AsString();
        }
        else {
            return ""s;
        }    
    }

    RenderSettings JsonReader::ParseRendererSettings(const json::Dict& settings) {
        RenderSettings out;

        out.width = settings.at("width").AsDouble();
        out.height = settings.at("height").AsDouble();
        out.padding = settings.at("padding").AsDouble();
        out.stop_radius = settings.at("stop_radius").AsDouble();
        out.line_width = settings.at("line_width").AsDouble();
        out.bus_label_font_size = settings.at("bus_label_font_size").AsInt();

        json::Array bus_label_offset = settings.at("bus_label_offset").AsArray();
        out.bus_label_offset.clear();
        out.bus_label_offset.reserve(bus_label_offset.size());
        for (const auto& item : bus_label_offset) {
            out.bus_label_offset.push_back(item.AsDouble());
        }   

        out.stop_label_font_size = settings.at("stop_label_font_size").AsInt();

        json::Array stop_label_offset = settings.at("stop_label_offset").AsArray();
        out.stop_label_offset.clear();
        out.stop_label_offset.reserve(stop_label_offset.size());
        for (const auto& item : stop_label_offset) {
            out.stop_label_offset.push_back(item.AsDouble());
        }

        out.underlayer_color = ColorNodeToString(settings.at("underlayer_color"));
        out.underlayer_width = settings.at("underlayer_width").AsDouble();
        
        json::Array color_palette = settings.at("color_palette").AsArray();
        out.color_palette.clear();
        out.color_palette.reserve(color_palette.size());
        for (const auto& node : color_palette) {
            out.color_palette.push_back(ColorNodeToString(node));
        }
        
        return out;
    }

    JsonReader::BaseRequest JsonReader::ResolveBaseRequest(const json::Dict& request) {
        const std::string& type_str = request.at("type"s).AsString();
        if (type_str == "Bus"s) {
            return { RequestType::AddBus, request };  
        }
        else if (type_str == "Stop"s) {
            return { RequestType::AddStop, request };    
        }
        else {
            return { RequestType::None, std::nullopt };
        }
    }

    JsonReader::StatRequest JsonReader::ResolveStatRequest(const json::Dict& request) {
        const std::string& type_str = request.at("type"s).AsString();
        if (type_str == "Bus"s) {
            return { request.at("id"s).AsInt(), RequestType::GetBusInfo, request.at("name"s).AsString() };   
        }
        else if (type_str == "Stop"s) {
            return { request.at("id"s).AsInt(), RequestType::GetStopInfo, request.at("name"s).AsString() };     
        }
        else if (type_str == "Map"s) {
            return { request.at("id"s).AsInt(), RequestType::GetMap, std::nullopt };     
        }
        else if (type_str == "Route"s) {
            return { request.at("id"s).AsInt(), RequestType::GetRoute, std::pair{ request.at("from"s).AsString(), request.at("to"s).AsString()} };     
        }
        else {
            return { -1, RequestType::None, std::nullopt };
        }
    }
}