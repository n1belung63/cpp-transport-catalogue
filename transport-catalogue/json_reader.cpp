#include "json_reader.h"
#include "json_builder.h"

namespace transport_catalogue {
    using namespace std::string_literals;

    void JsonReader::ParseRequests(std::istream& inp) {
        json::Document doc = json::Load(inp);
        const json::Dict& json_obj = doc.GetRoot().AsMap();

        if (json_obj.count("base_requests"s)) {
            for (const auto& raw_query : json_obj.at("base_requests"s).AsArray()) {         
                const json::Dict& query = raw_query.AsMap();
                Query query_resolved = ResolveQuery(query);
                base_requests_storage_.emplace_back(query_resolved);
            }
        }   

        if (json_obj.count("stat_requests"s)) {
            for (const auto& raw_query : json_obj.at("stat_requests"s).AsArray()) {         
                const json::Dict& query = raw_query.AsMap();
                Query query_resolved = ResolveQuery(query);
                std::string name = (query_resolved.query_body.count("name"s)) ? query_resolved.query_body.at("name").AsString() : ""s;
                stat_requests_storage_.push_back({
                    query_resolved.type, 
                    query_resolved.query_body.at("id"s).AsInt(), 
                    name
                });
                query_id_to_type_[query_resolved.query_body.at("id"s).AsInt()] = query_resolved.type;
            }
        }       

        if (json_obj.count("render_settings"s)) {
            settings_ = json_obj.at("render_settings"s).AsMap();
        }    
    }

    size_t JsonReader::GetBaseRequestCount() const {
        return base_requests_storage_.size();
    }

    JsonReader::BaseRequest JsonReader::GetBaseRequest(size_t i) {
        const auto base_request = base_requests_storage_.at(i);

        switch (base_request.type) {
        case QueryType::AddStop:
            return ParseStop(base_request.query_body);
            break;
        case QueryType::AddBus:
            return ParseBus(base_request.query_body);
            break;
        default:
            return {};
            break;
        }
    }

    size_t JsonReader::GetStatRequestCount() const {
        return stat_requests_storage_.size();
    }

    JsonReader::StatRequest JsonReader::GetStatRequest(size_t i) {
        return { 
            std::get<QueryType>(stat_requests_storage_.at(i)), 
            std::get<int>(stat_requests_storage_.at(i)), 
            std::get<std::string>(stat_requests_storage_.at(i))
        };
    }

    void JsonReader::AddResponse(const std::tuple<int,std::variant<std::string, BusInfo, StopInfo>>& resp) {
        const int request_id = std::get<int>(resp);
        const auto info = std::get<std::variant<std::string, BusInfo, StopInfo>>(resp);

        if (query_id_to_type_.at(request_id) == QueryType::GetBusInfo) {
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
        else if (query_id_to_type_.at(request_id) == QueryType::GetStopInfo) {
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
        else if (query_id_to_type_.at(request_id) == QueryType::GetMap) {
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
        return ParseRendererSettings(settings_);
    }

    Stop JsonReader::ParseStop(const json::Dict& query) {
        Stop stop;
        stop.name = query.at("name"s).AsString();
        stop.coords = { query.at("latitude"s).AsDouble(), query.at("longitude"s).AsDouble() };
        for (const auto& [key, value] : query.at("road_distances"s).AsMap()) {
            stop.range_to_other_stop[key] = value.AsDouble();
        }
        return stop;
    }

    Bus JsonReader::ParseBus(const json::Dict& query) {
        Bus bus;
        bus.num = query.at("name"s).AsString();
        bus.is_circular_route = query.at("is_roundtrip"s).AsBool();
        json::Array stops = query.at("stops"s).AsArray();
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

    Query JsonReader::ResolveQuery(const json::Dict& query) {
        const std::string& type_str = query.at("type"s).AsString();
        if (type_str == "Bus"s) {
            if (query.count("id"s)) {
                return { QueryType::GetBusInfo, query };
            }
            else {
                return { QueryType::AddBus, query };
            }      
        }
        else if (type_str == "Stop"s) {
            if (query.count("id"s)) {
                return { QueryType::GetStopInfo, query };
            }
            else {
                return { QueryType::AddStop, query };
            }     
        }
        else if (type_str == "Map"s) {
            return { QueryType::GetMap, query };     
        }
        else {
            return { QueryType::None, query };
        }
    }
}