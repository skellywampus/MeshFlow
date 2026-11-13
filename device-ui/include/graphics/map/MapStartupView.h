#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace MapStartupView
{
constexpr double kPi = 3.14159265358979323846;

struct Selection {
    bool hasCenter = false;
    double latitude = 0.0;
    double longitude = 0.0;
    uint8_t zoom = 3;
};

inline Selection fromNodeCoordinates(const std::vector<std::pair<int32_t, int32_t>> &coordinates)
{
    if (coordinates.empty())
        return {};

    std::vector<int32_t> latitudes;
    std::vector<double> longitudes;
    latitudes.reserve(coordinates.size());
    longitudes.reserve(coordinates.size());
    for (const auto &coordinate : coordinates) {
        latitudes.push_back(coordinate.first);
        double longitude = coordinate.second * 1e-7;
        longitude = std::fmod(longitude, 360.0);
        if (longitude < 0.0)
            longitude += 360.0;
        longitudes.push_back(longitude);
    }
    std::sort(latitudes.begin(), latitudes.end());
    std::sort(longitudes.begin(), longitudes.end());

    const size_t count = coordinates.size();
    if (count == 1) {
        return {true, latitudes[0] * 1e-7, coordinates[0].second * 1e-7, 13};
    }

    // Unwrap longitude across the smallest circular interval so nodes on
    // opposite sides of the date line stay adjacent instead of averaging at 0°.
    size_t largestGapAfter = count - 1;
    double largestGap = longitudes[0] + 360.0 - longitudes[count - 1];
    for (size_t i = 0; i + 1 < count; ++i) {
        const double gap = longitudes[i + 1] - longitudes[i];
        if (gap > largestGap) {
            largestGap = gap;
            largestGapAfter = i;
        }
    }
    const size_t longitudeStart = (largestGapAfter + 1) % count;
    const double arcStart = longitudes[longitudeStart];
    std::vector<double> unwrappedLongitudes;
    unwrappedLongitudes.reserve(count);
    for (size_t offset = 0; offset < count; ++offset) {
        const double longitude = longitudes[(longitudeStart + offset) % count];
        unwrappedLongitudes.push_back(longitude < arcStart ? longitude + 360.0 : longitude);
    }

    // Keep the central 60% when there are enough nodes to trim outliers.
    size_t first = count >= 5 ? count / 5 : 0;
    size_t end = count >= 5 ? (count * 4) / 5 : count;
    if (end <= first) {
        first = 0;
        end = count;
    }

    int64_t latitudeTotal = 0;
    double longitudeTotal = 0.0;
    for (size_t i = first; i < end; ++i) {
        latitudeTotal += latitudes[i];
        longitudeTotal += unwrappedLongitudes[i];
    }

    const double latitude = (static_cast<double>(latitudeTotal) / (end - first)) * 1e-7;
    double longitude = longitudeTotal / (end - first);
    if (longitude >= 360.0)
        longitude -= 360.0;
    if (longitude > 180.0)
        longitude -= 360.0;
    const double latitudeSpan = (latitudes[end - 1] - static_cast<double>(latitudes[first])) * 1e-7;
    const double longitudeSpan = unwrappedLongitudes[end - 1] - unwrappedLongitudes[first];
    const double latitudeCosine = std::abs(std::cos(latitudes[count / 2] * 1e-7 * kPi / 180.0));
    const double spanKm = std::max(std::abs(latitudeSpan), std::abs(longitudeSpan) * latitudeCosine) * 111.32;

    uint8_t zoom = 13;
    if (spanKm > 0.001) {
        const double estimate = std::sqrt(156.543034 / spanKm * std::max(latitudeCosine, 0.01) * 256.0) + 1.0;
        zoom = static_cast<uint8_t>(std::clamp(estimate, 3.0, 17.0));
    }
    return {true, latitude, longitude, zoom};
}
} // namespace MapStartupView
