#pragma once
#include <string>
#include <vector>
#include <utility>

namespace WLLog {

    inline std::vector<std::pair<std::string, std::string>> _lines;
    inline int _headerWidth = 0;
    inline int _longestLineWidth = 0;

    inline void resetLog() {
        _lines.clear();
        _headerWidth = 0;
        _longestLineWidth = 0;
    }

    inline void AddLine(const std::string &header, const std::string &value = "") {
        _lines.emplace_back(header, value);
    }

    inline const std::vector<std::pair<std::string, std::string>> &getLines() {
        return _lines;
    }

    inline void setHeaderWidth(int width) {
        if (width > _headerWidth) _headerWidth = width;
    }

    inline void setLongestLineWidth(int width) {
        if (width > _longestLineWidth) _longestLineWidth = width;
    }

    inline int getHeaderWidth() { return _headerWidth; }
    inline int getLongestLineWidth() { return _longestLineWidth; }

}
