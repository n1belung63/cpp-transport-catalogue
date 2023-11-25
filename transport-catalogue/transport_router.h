#pragma once

#include <map>
#include <string>

#include "domain.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue {
class TransportRouter {
public:
    explicit TransportRouter(const TransportCatalogue& db) 
        : tc_(db), graph_(graph::DirectedWeightedGraph<double>(0)), router_(graph::Router<double>(graph_)) { }

    void SetUp(const RoutingSettings& routing_settings);
    RouteInfo GetRoute(std::tuple<std::string_view,std::string_view> from_to);

private:
    RoutingSettings routing_settings_;

    const TransportCatalogue& tc_;
    graph::DirectedWeightedGraph<double> graph_;
    graph::Router<double> router_;

    struct StopItemPair { 
        graph::VertexId wait_on_stop_id;
        graph::VertexId stop_id;
    };

    std::map<std::string_view, StopItemPair> stopname_to_vertex_id_pair_;
    std::map<graph::VertexId, RouteItem> edge_id_to_route_item_;
};
}

