#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

namespace transport_catalogue {
class TransportCatalogueSerializer {
public:
    explicit TransportCatalogueSerializer(TransportCatalogue& tc, MapRenderer& map_renderer, TransportRouter& transport_router) 
        : tc_(tc), map_renderer_(map_renderer), transport_router_(transport_router) { }

    void SerializeToOstream(std::ostream& output);
    void DeserializeFromIstream(std::istream& input);

private:
    transport_catalogue_serialize::TransportCatalogue tc_ser_;

    TransportCatalogue& tc_;
    MapRenderer& map_renderer_;
    TransportRouter& transport_router_;

    void SerializeTransportCatalogue();
    void SerializeRenderer();
    void SerializeTransportRouter();

    void DeserializeTransportCatalogue();
    void DeserializeRenderer();
    void DeserializeTransportRouter();  
};
}
