// .hpp
// 2D Binary Space Partitioning
// by Kyle Furey

#pragma once
#include <vector>
#include <memory>
#include <utility>

namespace geo {
    struct v2 final {
        float x;
        float y;

        v2 operator+(v2 v) const;
        v2 operator-(v2 v) const;
        v2 operator*(float f) const;
        v2 operator/(float f) const;

        // Square magnitude
        float sqrmag() const;
    };

    struct line final {
        enum class side {
            // Behind
            NEGATIVE = -1,
            // On top
            COLLINEAR = 0,
            // In front
            POSITIVE = 1,
            // Through
            SPANNING = 2,
        };

        v2 start;
        v2 end;

        // Direction
        v2 dir() const;
        // Signed distance
        float sdist(v2 v) const;
        // Compare sidedness
        side compare(line l) const;
        // Split a line
        std::pair<line, line> split(line l) const;
    };

    struct bsp final {
        // Splitting line
        line partition;
        // On top
        std::vector<line> collinear;
        // In front
        std::unique_ptr<bsp> positive;
        // Behind
        std::unique_ptr<bsp> negative;

        // Size of BSP
        size_t size() const;
        // BSP generation
        static std::unique_ptr<bsp> generate(const std::vector<line> &lines, size_t partition);
    };
}
