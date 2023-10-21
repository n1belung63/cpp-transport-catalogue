#include "map_renderer.h"

namespace transport_catalogue {

using namespace std::string_literals;

bool IsItZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::SetBusExtendedInfo(const BusExtendedInfo& bus_info) {
    buses_info_.push_back(bus_info);
    for (const auto& [name, coords] : bus_info.stops_and_coordinates) {
        stop_name_and_coords_.push_back({name, coords});
        stop_coords_.push_back(coords);
    }
}

void MapRenderer::SetUp(const RenderSettings& settings) {
    settings_ = settings;
}

void MapRenderer::AddRoutesLayer(svg::Document& doc, const transport_catalogue::SphereProjector& proj) {
    const size_t color_palette_size = settings_.color_palette.size();

    for (size_t i=0; i<buses_info_.size(); ++i) {
        svg::Polyline bus_route = svg::Polyline();

        for (const auto& [stop, coords] : buses_info_.at(i).stops_and_coordinates) {
            bus_route.AddPoint(proj(coords));
        }
        if (!buses_info_.at(i).is_circular_route) {
            size_t stops_count = buses_info_.at(i).stops_and_coordinates.size();
            for (size_t j=1; j<stops_count; j++) {
                bus_route.AddPoint(proj( std::get<geo::Coordinates>( buses_info_.at(i).stops_and_coordinates.at(stops_count-1-j) ) ));
            }  
        }

        doc.Add(bus_route
            .SetFillColor("none"s)
            .SetStrokeColor(settings_.color_palette.at(i % color_palette_size))
            .SetStrokeWidth(static_cast<double>(settings_.line_width))
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
    }
}

void MapRenderer::AddRoutesNamesLayer(svg::Document& doc, const transport_catalogue::SphereProjector& proj) {
    auto text_back { [this](svg::Document& doc, const transport_catalogue::SphereProjector& proj, size_t index, size_t stop_index) {
        doc.Add(svg::Text()
            .SetPosition(proj( std::get<geo::Coordinates>( buses_info_.at(index).stops_and_coordinates.at(stop_index) ) ))
            .SetOffset({ settings_.bus_label_offset[0], settings_.bus_label_offset[1] })
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold"s)
            .SetData(buses_info_.at(index).name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
        } };

    auto text { [this](svg::Document& doc, const transport_catalogue::SphereProjector& proj, size_t index, size_t stop_index) {
        const size_t color_palette_size = settings_.color_palette.size();
        doc.Add(svg::Text()
            .SetPosition(proj( std::get<geo::Coordinates>( buses_info_.at(index).stops_and_coordinates.at(stop_index) ) ))
            .SetOffset({ settings_.bus_label_offset[0], settings_.bus_label_offset[1] })
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold"s)
            .SetData(buses_info_.at(index).name)
            .SetFillColor(settings_.color_palette.at(index % color_palette_size)));
        } };

    for (size_t i=0; i<buses_info_.size(); ++i) {
        text_back(doc, proj, i, 0);
        text(doc, proj, i, 0);
        if (!buses_info_.at(i).is_circular_route) {
            size_t stops_count = buses_info_.at(i).stops_and_coordinates.size();
            if (std::get<std::string>(buses_info_.at(i).stops_and_coordinates.at(0)) != std::get<std::string>(buses_info_.at(i).stops_and_coordinates.at(stops_count-1))) {
                text_back(doc, proj, i, stops_count-1);
                text(doc, proj, i, stops_count-1);
            }
        }
    }
}

void MapRenderer::AddStopsLayer(svg::Document& doc, const transport_catalogue::SphereProjector& proj) {
    for (const auto& [name, coords] : stop_name_and_coords_) {
        doc.Add(svg::Circle()
            .SetCenter(proj(coords))
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white"s));
    }
}

void MapRenderer::AddStopsNamesLayer(svg::Document& doc, const transport_catalogue::SphereProjector& proj) {
    for (const auto& [name, coords] : stop_name_and_coords_) {
        doc.Add(svg::Text()
            .SetPosition(proj(coords))
            .SetOffset({ settings_.stop_label_offset[0], settings_.stop_label_offset[1] })
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
        
        doc.Add(svg::Text()
            .SetPosition(proj(coords))
            .SetOffset({ settings_.stop_label_offset[0], settings_.stop_label_offset[1] })
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(name)
            .SetFillColor("black"s));
    }
}

void MapRenderer::Render(std::ostream& out) {
    std::sort(
        buses_info_.begin(), 
        buses_info_.end(), 
        [this](const BusExtendedInfo& left, const BusExtendedInfo& right){ 
            return left.name < right.name;
        });
    
    std::sort(
        stop_name_and_coords_.begin(),
        stop_name_and_coords_.end(),
        [this](const std::tuple<std::string, geo::Coordinates>& left, const std::tuple<std::string, geo::Coordinates>& right) { 
            return std::get<std::string>(left) < std::get<std::string>(right);
        });
    stop_name_and_coords_.erase( std::unique( stop_name_and_coords_.begin(), stop_name_and_coords_.end() ), stop_name_and_coords_.end() );

    const transport_catalogue::SphereProjector proj{
        stop_coords_.begin(), 
        stop_coords_.end(), 
        static_cast<double>(settings_.width), 
        static_cast<double>(settings_.height), 
        static_cast<double>(settings_.padding)
    };

    svg::Document doc;
    AddRoutesLayer(doc, proj);
    AddRoutesNamesLayer(doc, proj);
    AddStopsLayer(doc, proj);
    AddStopsNamesLayer(doc, proj);
    doc.Render(out);
}

}