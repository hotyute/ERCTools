#pragma once

#include "core/common.h"

struct PopulatedPlace
{
    std::wstring name;
    std::wstring country;
    double latitude = 0.0;
    double longitude = 0.0;
    double population = 0.0;
};

std::vector<PopulatedPlace> ParsePopulatedPlacesGeoJson(const std::string& body, std::wstring* errorOut = nullptr);
std::vector<PopulatedPlace> ParseSupplementalPopulatedPlacesJson(const std::string& body, std::wstring* errorOut = nullptr);
void MergePopulatedPlaces(std::vector<PopulatedPlace>& places, std::vector<PopulatedPlace> supplementalPlaces);
std::string SerializePopulatedPlacesCache(const std::vector<PopulatedPlace>& places, std::wstring* errorOut = nullptr);
bool PopulatedPlacesSupportPopulationFilter(const std::vector<PopulatedPlace>& places);
bool LoadPopulatedPlacesFromFile(const std::filesystem::path& path, std::vector<PopulatedPlace>& placesOut, std::wstring* errorOut = nullptr);
bool SavePopulatedPlacesToFile(const std::filesystem::path& path, const std::string& body, std::wstring* errorOut = nullptr);
double HaversineMiles(double lat1, double lon1, double lat2, double lon2);
bool FindNearestPopulatedPlace(
    const std::vector<PopulatedPlace>& places,
    double latitude,
    double longitude,
    double maxDistanceMiles,
    double minimumPopulation = 1.0,
    PopulatedPlace* nearestOut = nullptr,
    double* distanceMilesOut = nullptr);
