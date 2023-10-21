#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
using namespace transport_catalogue;

int main()
{
    TransportCatalogue transport_catalogue;
    MapRenderer map_renderer;
    
    RequestHandler request_handler(transport_catalogue, map_renderer);

    JsonReader reader;
    reader.ParseRequests(std::cin);

    for (size_t i=0; i < reader.GetBaseRequestCount(); i++) {
        request_handler.BaseRequest(reader.GetBaseRequest(i));
    }

    map_renderer.SetUp(reader.GetRendererSettings());

    for (size_t i=0; i < reader.GetStatRequestCount(); i++) {
        reader.AddResponse(request_handler.StatRequest(reader.GetStatRequest(i)));
    }
    reader.PrintResponses(std::cout);
}