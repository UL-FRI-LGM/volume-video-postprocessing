#include <openvdb/openvdb.h>
#include <iostream>
#include <string>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/ChangeBackground.h>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;
using std::string, std::vector;

openvdb::FloatGrid::Ptr swapValueCopy(openvdb::FloatGrid::Ptr input, float from, float to)
{
    openvdb::FloatGrid::Ptr output = input->deepCopy();
    for (auto i = input->beginValueOn(); i.test(); ++i)
    {
        if (i.getValue() == from)
        {
            i.setValue(to);
        }
    }
    return output;
}

openvdb::FloatGrid::Ptr combineGrids(vector<openvdb::FloatGrid::Ptr> grids)
{
    return std::accumulate(
        grids.begin() + 1,
        grids.end(),
        (*grids.begin())->deepCopy(),
        [](auto a, auto b) 
        {
            return openvdb::tools::csgUnionCopy(*a,*b);
        });
}

int main()
{
    // Initialize the OpenVDB library
    openvdb::initialize();
    string path = "./model_vdbc";

    vector<openvdb::FloatGrid::Ptr> red_grids;
    vector<openvdb::FloatGrid::Ptr> blue_grids;
    vector<openvdb::FloatGrid::Ptr> green_grids;
    for (const auto & entry : fs::directory_iterator(path))
    {
        // Parse the filename
        using boost::split, boost::is_any_of;
        string path = entry.path();
        vector<string> strs;
        split(strs, path, is_any_of("-"));
        path = strs[1].substr(0, strs[1].size()-4);
        split(strs, path, is_any_of(","));
        vector<float> color(strs.size());
        std::transform(strs.begin(), strs.end(), color.begin(), [](string i) {return std::stof(i);});

        // Read files
        openvdb::io::File file(entry.path());
        file.open();
        openvdb::GridBase::Ptr grid = file.readGrid("density");
        file.close();
        openvdb::FloatGrid::Ptr volume = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
        red_grids.push_back(swapValueCopy(volume, 1.0, color[0]));
        green_grids.push_back(swapValueCopy(volume, 1.0, color[1]));
        blue_grids.push_back(swapValueCopy(volume, 1.0, color[2]));
    }

    // Combine grids into accumulator
    openvdb::FloatGrid::Ptr red = combineGrids(red_grids);
    openvdb::FloatGrid::Ptr green = combineGrids(green_grids);
    openvdb::FloatGrid::Ptr blue = combineGrids(blue_grids);
    
    // Save to file
    openvdb::io::File("result.vdb").write({accumulator});
}