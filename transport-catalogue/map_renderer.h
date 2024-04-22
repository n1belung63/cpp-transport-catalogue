#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <deque>

namespace transport_catalogue {

inline const double EPSILON = 1e-6;
bool IsItZero(double value);

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin,PointInputIt points_end,double max_width,double max_height,double padding) 
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsItZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsItZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

struct RenderSettings {
    double width;
    double height;
    double padding;
    double stop_radius;
    double line_width;
    int bus_label_font_size;
    std::vector<double> bus_label_offset;
    int stop_label_font_size;
    std::vector<double> stop_label_offset;
    std::string underlayer_color;
    double underlayer_width;
    std::vector<std::string> color_palette;
};

class MapRenderer {
public:
    MapRenderer() = default;

    void SetUp(const RenderSettings& settings);

    const RenderSettings GetRendererSettings() const;

    void Render(std::ostream& out);

    void SetBusExtendedInfo(const BusExtendedInfo& bus_info);

private:
    RenderSettings settings_;
    std::deque<BusExtendedInfo> buses_info_;
    std::deque<geo::Coordinates> stop_coords_;
    std::deque<std::tuple<std::string, geo::Coordinates>> stop_name_and_coords_;

    void AddRoutesLayer(svg::Document& doc, const SphereProjector& proj);
    void AddRoutesNamesLayer(svg::Document& doc, const SphereProjector& proj);
    void AddStopsLayer(svg::Document& doc, const SphereProjector& proj);
    void AddStopsNamesLayer(svg::Document& doc, const SphereProjector& proj);
};

} // transport_catalogue