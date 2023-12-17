#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "transport_catalogue.h"
#include "serialization.h"

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <fstream>

using namespace std;
using namespace transport_catalogue;

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    // const std::string_view mode = "make_base"sv;
    // const std::string_view mode = "process_requests"sv;

    TransportCatalogue transport_catalogue;
    TransportRouter transport_router(transport_catalogue);
    MapRenderer map_renderer;

    RequestHandler request_handler(transport_catalogue, map_renderer, transport_router);
    TransportCatalogueSerializer transport_catalogue_serializer(transport_catalogue, map_renderer, transport_router);

    JsonReader reader;

    reader.ParseRequests(std::cin);
    if (mode == "make_base"sv) {
        for (size_t i=0; i < reader.GetBaseRequestCount(); i++) {
            request_handler.BaseRequest(reader.GetBaseRequest(i));
        }
        map_renderer.SetUp(reader.GetRendererSettings());
        transport_router.SetUp(reader.GetRoutingSettings());

        std::ofstream ofs(reader.GetSerializationFilePath(), ios::binary);
        transport_catalogue_serializer.SerializeToOstream(ofs);
    } else if (mode == "process_requests"sv) {
        std::ifstream ifs(reader.GetSerializationFilePath(), ios::binary);
        transport_catalogue_serializer.DeserializeFromIstream(ifs);

        for (size_t i=0; i < reader.GetStatRequestCount(); i++) {
            reader.AddResponse(request_handler.StatRequest(reader.GetStatRequest(i)));
        }
        reader.PrintResponses(std::cout); 
    } else {
        PrintUsage();
        return 1;
    }
}