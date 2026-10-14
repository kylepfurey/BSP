// .cpp
// 2D Binary Space Partitioning
// by Kyle Furey

#include "bsp.hpp"
#include <cfloat>

namespace geo {
    v2 v2::operator+(v2 v) const {
        return {x + v.x, y + v.y};
    }

    v2 v2::operator-(v2 v) const {
        return {x - v.x, y - v.y};
    }

    v2 v2::operator*(float f) const {
        return {x * f, y * f};
    }

    v2 v2::operator/(float f) const {
        return {x / f, y / f};
    }

    float v2::sqrmag() const {
        return x * x + y * y;
    }

    v2 line::dir() const {
        return end - start;
    }

    float line::sdist(v2 v) const {
        const v2 dir1 = end - start;
        const v2 dir2 = v - start;
        return dir1.x * dir2.y - dir1.y * dir2.x;
    }

    line::side line::compare(line l) const {
        float sqrep = FLT_EPSILON * FLT_EPSILON * dir().sqrmag();
        auto signof = [sqrep](float f)-> int {
            if (f * f <= sqrep) {
                return 0;
            }
            return f > 0.0f ? 1 : -1;
        };
        int startsign = signof(sdist(l.start));
        int endsign = signof(sdist(l.end));
        if (startsign * endsign < 0) {
            return side::SPANNING;
        }
        if (startsign > 0 || endsign > 0) {
            return side::POSITIVE;
        }
        if (startsign < 0 || endsign < 0) {
            return side::NEGATIVE;
        }
        return side::COLLINEAR;
    }

    std::pair<line, line> line::split(line l) const {
        float startdist = sdist(l.start);
        float enddist = sdist(l.end);
        float len = startdist / (startdist - enddist);
        v2 p = l.start + (l.end - l.start) * len;
        line first = l;
        line second = l;
        first.end = p;
        second.start = p;
        return {first, second};
    }

    static size_t rsize(const bsp &b) {
        size_t count = 1;
        if (b.positive != nullptr) {
            count += rsize(*b.positive);
        }
        if (b.negative != nullptr) {
            count += rsize(*b.negative);
        }
        return count;
    }

    size_t bsp::size() const {
        return rsize(*this);
    }

    static size_t partition(const std::vector<line> &lines) {
        size_t best = 0;
        float best_score = FLT_MAX;
        for (size_t i = 0; i < lines.size(); ++i) {
            int positive = 0;
            int negative = 0;
            int spanning = 0;
            for (size_t j = 0; j < lines.size(); ++j) {
                if (i == j) continue;
                switch (lines[i].compare(lines[j])) {
                    case line::side::POSITIVE:
                        ++positive;
                        break;
                    case line::side::NEGATIVE:
                        ++negative;
                        break;
                    case line::side::SPANNING:
                        ++spanning;
                        break;
                    case line::side::COLLINEAR:
                        break;
                }
            }
            int diff = positive - negative;
            float score = 8.0f * static_cast<float>(spanning) + static_cast<float>(diff > 0 ? diff : -diff);
            if (score < best_score) {
                best_score = score;
                best = i;
            }
        }
        return best;
    }

    std::unique_ptr<bsp> bsp::generate(const std::vector<line> &lines, size_t partition) {
        if (partition >= lines.size()) {
            return nullptr;
        }
        auto root = std::make_unique<bsp>();
        line part = lines[partition];
        root->partition = part;
        auto append = [](std::vector<line> &v, line l) {
            if (l.dir().sqrmag() > FLT_EPSILON * FLT_EPSILON) {
                v.push_back(l);
            }
        };
        std::vector<line> positive;
        std::vector<line> negative;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i == partition) {
                continue;
            }
            line l = lines[i];
            switch (part.compare(l)) {
                case line::side::COLLINEAR:
                    root->collinear.push_back(l);
                    break;
                case line::side::POSITIVE:
                    append(positive, l);
                    break;
                case line::side::NEGATIVE:
                    append(negative, l);
                    break;
                case line::side::SPANNING: {
                    auto split = part.split(l);
                    bool startpos = part.sdist(l.start) > 0.0f;
                    append(startpos ? positive : negative, split.first);
                    append(startpos ? negative : positive, split.second);
                    break;
                }
            }
        }
        size_t pospart = geo::partition(positive);
        size_t negpart = geo::partition(negative);
        root->positive = generate(positive, pospart);
        root->negative = generate(negative, negpart);
        return root;
    }
}
