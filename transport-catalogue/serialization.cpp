#include "serialization.h"
#include "graph.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

namespace transport_catalogue {
    using namespace std::string_literals;


    void SerializeTransportCatalogue(
        TransportCatalogue& tc, 
        const RenderSettings& renderer_settings, 
        const TransportRouter& transport_router, 
        std::ostream& output)
    {
        transport_catalogue_serialize::TransportCatalogue tc_ser;

        std::unordered_map<std::string, uint32_t> stop_name_to_stop_id;
        uint32_t stop_id = 0;
        for (std::string_view stop_name : tc.GetStopList()) {
            stop_name_to_stop_id[std::string(stop_name)] = stop_id++;
        }
        for (std::string_view stop_name : tc.GetStopList()) {
            const Stop stop = tc.FindStop(stop_name);

            transport_catalogue_serialize::Stop stop_ser;

            stop_ser.set_name(stop.name);
            stop_ser.mutable_coords()->set_lat(stop.coords.lat);
            stop_ser.mutable_coords()->set_lon(stop.coords.lng);

            for (const auto& [another_stop_name, distance] : stop.range_to_other_stop) {
                transport_catalogue_serialize::StopIdAndDistancePair pair;
                pair.set_key(stop_name_to_stop_id.at(another_stop_name));
                pair.set_value(distance);

                *stop_ser.add_another_stop_id_to_distance() = pair;
                pair.Clear();
            }

            *tc_ser.add_stop() = stop_ser;
            stop_ser.Clear();
        }


        for (const std::string_view bus_name : tc.GetBusList()) {
            const Bus bus = tc.FindBus(bus_name);

            transport_catalogue_serialize::Bus bus_ser;

            bus_ser.set_name(bus.num);
            bus_ser.set_is_circular_route(bus.is_circular_route);

            for (const auto& stop_name : bus.stopnames) {
                bus_ser.add_stop_id(stop_name_to_stop_id.at(stop_name));
            }

            *tc_ser.add_bus() = bus_ser;
            bus_ser.Clear();
        }


        tc_ser.mutable_renderer_settings()->set_width(renderer_settings.width);
        tc_ser.mutable_renderer_settings()->set_height(renderer_settings.height);
        tc_ser.mutable_renderer_settings()->set_padding(renderer_settings.padding);
        tc_ser.mutable_renderer_settings()->set_stop_radius(renderer_settings.stop_radius);
        tc_ser.mutable_renderer_settings()->set_line_width(renderer_settings.line_width);
        tc_ser.mutable_renderer_settings()->set_bus_label_font_size(renderer_settings.bus_label_font_size);
        for (const double item : renderer_settings.bus_label_offset) {
            tc_ser.mutable_renderer_settings()->add_bus_label_offset(item);
        }
        tc_ser.mutable_renderer_settings()->set_stop_label_font_size(renderer_settings.stop_label_font_size);
        for (const double item : renderer_settings.stop_label_offset) {
            tc_ser.mutable_renderer_settings()->add_stop_label_offset(item);
        }
        tc_ser.mutable_renderer_settings()->set_underlayer_color(renderer_settings.underlayer_color);
        tc_ser.mutable_renderer_settings()->set_underlayer_width(renderer_settings.underlayer_width);
        for (const std::string& item : renderer_settings.color_palette) {
            tc_ser.mutable_renderer_settings()->add_color_palette(item);
        }

        const RoutingSettings routing_settins = transport_router.GetRoutingSettings();
        tc_ser.mutable_router()->mutable_routing_settings()->set_bus_velocity(routing_settins.bus_velocity);
        tc_ser.mutable_router()->mutable_routing_settings()->set_bus_wait_time(routing_settins.bus_wait_time);
        transport_catalogue_serialize::Edge edge_ser;
        transport_catalogue_serialize::Graph graph_ser;
        for (graph::EdgeId i = 0; i < transport_router.GetGraph().GetEdgeCount(); ++i) {
            auto edge = transport_router.GetGraph().GetEdge(i);
            edge_ser.set_from(edge.from);
            edge_ser.set_to(edge.to);
            edge_ser.set_weight(edge.weight);

            *graph_ser.add_edge() = edge_ser;
            edge_ser.Clear();
        }
        *tc_ser.mutable_router()->mutable_graph() = graph_ser;
        transport_catalogue_serialize::StopnameAndStopItemPairPair stopname_to_stop_item_pair_pair_ser;
        for (const auto& [key, value] : transport_router.GetStopNameToVertexIdPair()) {
            stopname_to_stop_item_pair_pair_ser.set_key(std::string(key));
            stopname_to_stop_item_pair_pair_ser.mutable_value()->set_wait_on_stop_id(value.wait_on_stop_id);
            stopname_to_stop_item_pair_pair_ser.mutable_value()->set_stop_id(value.stop_id);

            *tc_ser.mutable_router()->add_stopname_to_vertex_id_pair() = stopname_to_stop_item_pair_pair_ser;
            stopname_to_stop_item_pair_pair_ser.Clear();
        }
        transport_catalogue_serialize::EdgeIdAndRouteItemPair edge_id_and_route_item_pair_ser;
        for (const auto& [key, value] : transport_router.GetEdgeIdToRouteItem()) {
            edge_id_and_route_item_pair_ser.set_key(key);
            transport_catalogue_serialize::RouteItem route_item_ser;
            if (std::holds_alternative<WaitItem>(value)) {
                const auto& wait_item = std::get<WaitItem>(value);
                route_item_ser.mutable_wait_item()->set_stop_name(wait_item.stop_name);
                route_item_ser.mutable_wait_item()->set_time(wait_item.time);
            } else if (std::holds_alternative<BusItem>(value)) {
                const auto& bus_item = std::get<BusItem>(value);
                route_item_ser.mutable_bus_item()->set_bus_name(bus_item.bus);
                route_item_ser.mutable_bus_item()->set_span_count(bus_item.span_count);
                route_item_ser.mutable_bus_item()->set_time(bus_item.time);
            }
            *edge_id_and_route_item_pair_ser.mutable_value() = route_item_ser;
            *tc_ser.mutable_router()->add_edge_id_to_route_item() = edge_id_and_route_item_pair_ser;
            edge_id_and_route_item_pair_ser.Clear();
        }

        tc_ser.SerializeToOstream(&output);
    }

    void DeserializeTransportCatalogue(
        std::istream& input, 
        TransportCatalogue& tc, 
        MapRenderer& map_renderer,
        TransportRouter& transport_router)
    {
        transport_catalogue_serialize::TransportCatalogue tc_ser;
        tc_ser.ParseFromIstream(&input);

        std::unordered_map<uint32_t, std::string> stop_id_to_stop_name;
        uint32_t stop_id = 0;
        for (const auto& s : tc_ser.stop()) {
            stop_id_to_stop_name[stop_id++] = s.name();
        }
        for (const auto& s : tc_ser.stop()) {
            Stop stop;
            stop.name = s.name();
            stop.coords = { s.coords().lat(), s.coords().lon() };

            for (const auto& as : s.another_stop_id_to_distance()) {
                stop.range_to_other_stop[stop_id_to_stop_name.at(as.key())] = as.value();
            }

            tc.AddStop(stop);
        }


        for (const auto& b : tc_ser.bus()) {
            Bus bus;
            bus.is_circular_route = b.is_circular_route();
            bus.num = b.name();

            bus.stopnames.reserve(b.stop_id_size());
            for (const auto id : b.stop_id()) {
                bus.stopnames.push_back(tc_ser.stop(id).name());
            }

            tc.AddBus(bus);
        }


        RenderSettings renderer_settings;
        renderer_settings.width = tc_ser.renderer_settings().width();
        renderer_settings.height = tc_ser.renderer_settings().height();
        renderer_settings.padding = tc_ser.renderer_settings().padding();
        renderer_settings.stop_radius = tc_ser.renderer_settings().stop_radius();
        renderer_settings.line_width = tc_ser.renderer_settings().line_width();
        renderer_settings.bus_label_font_size = tc_ser.renderer_settings().bus_label_font_size();
        renderer_settings.bus_label_offset.reserve(tc_ser.renderer_settings().bus_label_offset_size());
        for (const double item : tc_ser.renderer_settings().bus_label_offset()) {
            renderer_settings.bus_label_offset.push_back(item);
        }
        renderer_settings.stop_label_font_size = tc_ser.renderer_settings().stop_label_font_size();
        renderer_settings.stop_label_offset.reserve(tc_ser.renderer_settings().stop_label_offset_size());
        for (const double item : tc_ser.renderer_settings().stop_label_offset()) {
            renderer_settings.stop_label_offset.push_back(item);
        }
        renderer_settings.underlayer_color = tc_ser.renderer_settings().underlayer_color();
        renderer_settings.underlayer_width = tc_ser.renderer_settings().underlayer_width();
        renderer_settings.color_palette.reserve(tc_ser.renderer_settings().color_palette_size());
        for (const std::string& item : tc_ser.renderer_settings().color_palette()) {
            renderer_settings.color_palette.push_back(item);
        }
        map_renderer.SetUp(renderer_settings);

        RoutingSettings routing_settings;
        routing_settings.bus_velocity = tc_ser.router().routing_settings().bus_velocity();
        routing_settings.bus_wait_time = tc_ser.router().routing_settings().bus_wait_time();
        transport_router.SetRoutingSettings(routing_settings);
        transport_router.GetGraph().SetVertexCount(tc_ser.stop_size() * 2); // todo: upd
        for (const auto& item : tc_ser.router().graph().edge()) {
            graph::Edge<double> edge = { item.from(), item.to(), item.weight() };
            transport_router.GetGraph().AddEdge(edge);
        }
        transport_router.UpdateRouter();
        std::map<std::string, TransportRouter::StopItemPair> stopname_to_vertex_id_pair;
        for (const auto& item : tc_ser.router().stopname_to_vertex_id_pair()) {
            stopname_to_vertex_id_pair.insert({ item.key(), TransportRouter::StopItemPair{ item.value().wait_on_stop_id(), item.value().stop_id() }});
        }
        transport_router.SetStopnameToVertexIdPair(stopname_to_vertex_id_pair);
        std::map<graph::VertexId, RouteItem> edge_id_to_route_item;
        for (const auto& item : tc_ser.router().edge_id_to_route_item()) {
            if (item.value().has_wait_item()) {
                edge_id_to_route_item.insert({ item.key(), WaitItem{ item.value().wait_item().stop_name(), item.value().wait_item().time() }});
            } else if (item.value().has_bus_item()) {
                edge_id_to_route_item.insert({ item.key(), BusItem{ item.value().bus_item().bus_name(), item.value().bus_item().span_count(), item.value().bus_item().time() }});
            }
        }
        transport_router.SetEdgeIdToRouteItem(edge_id_to_route_item);
    }


}