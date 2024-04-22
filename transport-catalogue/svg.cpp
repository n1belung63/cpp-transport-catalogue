#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    bool first_one = true;
    out << "<polyline points=\""sv;
    for (const Point& point : points_) {
        if (first_one) {
            first_one = false;
        }
        else {
            out  << " "sv;
        }
        out << point.x << ","sv << point.y;     
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(const std::string& font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(const std::string& font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(const std::string& data) {
    data_ = data;
    return *this;
}

void Text::EscapeString(const RenderContext& context) const {
    auto& out = context.out;
    for (char c : data_) {
        switch (c) {
        case '\"':
            out << "&quot;";
            break;
        case '\'':
            out << "&apos;";
            break;
        case '<':
            out << "&lt;";
            break;
        case '>':
            out << "&gt;";
            break;
        case '&':
            out << "&amp;";
            break;
        default:
            out << c;
        }
    }
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << font_size_ << "\""sv;
    if (font_family_ != "") {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (font_weight_ != "") {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">";
    Text::EscapeString(context);
    out << "</text>"sv;
}

// ---------- Document ------------------
void Document::AddPtr(std::unique_ptr<Object>&& object_ptr) {
    objects_.emplace_back(std::move(object_ptr));
}

void Document::Render(std::ostream& out) const {
    RenderContext ctx(out, 2, 2);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (const auto& obj : objects_) {
        obj.get()->Render(ctx);
    }
    out << "</svg>"sv;
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch(line_cap)
    {
        case StrokeLineCap::BUTT : return out << "butt"sv ;
        case StrokeLineCap::ROUND : return out << "round"sv ;
        case StrokeLineCap::SQUARE : return out << "square"sv ;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch(line_join)
    {
        case StrokeLineJoin::ARCS : return out << "arcs"sv ;
        case StrokeLineJoin::BEVEL : return out << "bevel"sv ;
        case StrokeLineJoin::MITER : return out << "miter"sv ;
        case StrokeLineJoin::MITER_CLIP : return out << "miter-clip"sv ;
        case StrokeLineJoin::ROUND : return out << "round"sv ;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, Rgb rgb) {
    out << "rgb("sv << unsigned(rgb.red) << ","sv << unsigned(rgb.green) << ","sv << unsigned(rgb.blue) << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, Rgba rgba) {
    out << "rgba("sv << unsigned(rgba.red) << ","sv << unsigned(rgba.green) << ","sv << unsigned(rgba.blue) << ","sv << rgba.opacity << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, std::monostate) {
    out << "none"sv;
    return out;
}

}  // namespace svg