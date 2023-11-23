#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <fstream>

using namespace std;
using namespace transport_catalogue;

int main()
{
    // const std::string path_to_file = "C:\\Users\\SRD\\Desktop\\Yandex C++\\cpp-transport-catalogue\\";
    // std::ifstream ifile(path_to_file + "s12_final_opentest_2.json");
    // std::ofstream ofile(path_to_file + "s12_final_opentest_2_result.json");

    TransportCatalogue transport_catalogue;
    MapRenderer map_renderer;
    
    RequestHandler request_handler(transport_catalogue, map_renderer);

    JsonReader reader;

    // reader.ParseRequests(ifile);

    reader.ParseRequests(std::cin);

    for (size_t i=0; i < reader.GetBaseRequestCount(); i++) {
        request_handler.BaseRequest(reader.GetBaseRequest(i));
    }

    map_renderer.SetUp(reader.GetRendererSettings());
    request_handler.BuildGraph(reader.GetRoutingSettings());

    for (size_t i=0; i < reader.GetStatRequestCount(); i++) {
        reader.AddResponse(request_handler.StatRequest(reader.GetStatRequest(i)));
    }
    reader.PrintResponses(std::cout);
    // reader.PrintResponses(ofile);
}