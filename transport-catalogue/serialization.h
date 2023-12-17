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

namespace transport_catalogue {
    void SerializeTransportCatalogue(TransportCatalogue& tc, const RenderSettings& renderer_settings, const TransportRouter& transport_router, std::ostream& output);

    void DeserializeTransportCatalogue(std::istream& input, TransportCatalogue& tc, MapRenderer& map_renderer, TransportRouter& transport_router);
}
