#include <openvdb/math/Coord.h>
#include <openvdb/openvdb.h>
#include <iostream>
#include <string>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/ChangeBackground.h>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <algorithm>
#include <thread>
#include <fstream>

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

vector<float> getModelColors(string path) 
{
    using boost::split, boost::is_any_of;
    vector<string> strs;
    split(strs, path, is_any_of("-"));
    path = strs[1].substr(0, strs[1].size()-4);
    split(strs, path, is_any_of(","));
    vector<float> color(strs.size());
    std::transform(strs.begin(), strs.end(), color.begin(), [](string i) {return std::stof(i);});
    return color;
}

void vdb2Raw(openvdb::FloatGrid::Ptr grid, string output)
{
    openvdb::FloatGrid::ConstAccessor accessor = grid->getConstAccessor();
    auto bound_box = grid->evalActiveVoxelBoundingBox();
    std::ofstream outputFile(output + std::to_string(bound_box.dim().x()) + "," +
                             std::to_string(bound_box.dim().y()) + "," +
                             std::to_string(bound_box.dim().z()) + ".bin", std::ios::binary);
    if (!outputFile)
    {
        std::cout << "Can't write to file" << std::endl;
        return;
    }

    for (auto i = grid->evalActiveVoxelBoundingBox().beginXYZ(); i; ++i)
    {
        uint8_t value = static_cast<char>(accessor.getValue(*i) * 255);
        outputFile << value;
    }
}

int main()
{
    // Initialize the OpenVDB library
    openvdb::initialize();
    string model_dir = "./model_vdbc";
    string output_dir = "./model_combined_raw";

    vector<openvdb::FloatGrid::Ptr> red_grids;
    vector<openvdb::FloatGrid::Ptr> blue_grids;
    vector<openvdb::FloatGrid::Ptr> green_grids;
    for (const auto & entry : fs::directory_iterator(model_dir))
    {
        // Parse filename to get color
        vector<float> color = getModelColors(entry.path());

        // Read file and make r, g, and b grids
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
    
    // // Save to file
    // openvdb::io::File("result.vdb").write({accumulator});
    
    vdb2Raw(red, output_dir + "/red");    
    vdb2Raw(green, output_dir + "/green");    
    vdb2Raw(blue, output_dir + "/blue");    
}