#include <openvdb/math/Coord.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/ChangeBackground.h>
#include <openvdb/tools/ValueTransformer.h>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <algorithm>
#include <thread>
#include <fstream>

namespace fs = std::filesystem;
using std::string, std::vector;

openvdb::FloatGrid::Ptr swapValueCopy(openvdb::FloatGrid::Ptr input, float scale)
{
    // struct Local {
    //     float scale;
    //     Local(float neki): scale(neki) {}
    //     inline void op(const openvdb::FloatGrid::ValueAllIter& iter) {
    //         iter.setValue(*iter * scale);
    //     }
    // };
    openvdb::FloatGrid::Ptr output = input->deepCopy();
    for (auto i = output->beginValueOn(); i; ++i) {
        i.setValue(*i * scale);
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
                             std::to_string(bound_box.dim().z()) + ".raw", std::ios::binary);
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
    string model_dir = "./vdb";
    string output_dir = "./raw/";
    fs::create_directory(output_dir);
    int frame = 0;

    for (const auto & entry : fs::directory_iterator(model_dir))
    {
        if (!entry.is_directory()) continue;
        std::cout << "Parsing frame " << entry.path().filename() << std::endl;
        
        vector<openvdb::FloatGrid::Ptr> red_grids;
        vector<openvdb::FloatGrid::Ptr> blue_grids;
        vector<openvdb::FloatGrid::Ptr> green_grids;
        for (const auto & frame_dir : fs::directory_iterator(entry.path()))
        {
            // Parse filename to get color
            vector<float> color = getModelColors(frame_dir.path());

            // Read file and make r, g, and b grids
            openvdb::io::File file(frame_dir.path());
            file.open();
            openvdb::GridBase::Ptr grid = file.readGrid("density");
            file.close();
            openvdb::FloatGrid::Ptr volume = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
            // std::cout << color[0] << "," << color[1] << "," << color[2] << std::endl;
            // std::stringstream name("./test");
            // name << frame_dir.path().filename() << volume->evalActiveVoxelBoundingBox().dim() << ".raw";
            // vdb2Raw(volume, name.str());

            // red_grids.push_back(volume);
            red_grids.push_back(swapValueCopy(volume, color[0]));
            green_grids.push_back(swapValueCopy(volume, color[1]));
            blue_grids.push_back(swapValueCopy(volume, color[2]));
        }

        // Combine grids into accumulator
        openvdb::FloatGrid::Ptr red = combineGrids(red_grids);
        openvdb::FloatGrid::Ptr green = combineGrids(green_grids);
        openvdb::FloatGrid::Ptr blue = combineGrids(blue_grids);

        float kernel[3][3][3] = {
                      {{0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037}},
                      {{0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037}},
                      {{0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037},
                       {0.037037037, 0.037037037, 0.037037037}}
                      };

        auto red_acc = red->getConstAccessor();
        auto green_acc = green->getConstAccessor();
        auto blue_acc = blue->getConstAccessor();
        auto red_out = red->deepCopy();
        auto green_out = green->deepCopy();
        auto blue_out = blue->deepCopy();
        auto red_new = red_out->getAccessor();
        auto green_new = green_out->getAccessor();
        auto blue_new = blue_out->getAccessor();
        for (auto a = red->evalActiveVoxelBoundingBox().beginXYZ(); a; ++a) {
            auto r = red_acc.getValue(*a);
            auto g = green_acc.getValue(*a);
            auto b = blue_acc.getValue(*a);
            float red_bin = 0.0;
            float green_bin = 0.0;
            float blue_bin = 0.0;
            for(int i=-1; i<=1; i++){
                for(int j=-1; j<=1; j++){
                    for(int k=-1; k<=1; k++) {
                        auto current = *a + openvdb::v10_0::math::Coord(i, j, k);
                        red_bin += red_acc.getValue(current) * kernel[i+1][j+1][k+1];
                        green_bin += green_acc.getValue(current) * kernel[i+1][j+1][k+1];
                        blue_bin += blue_acc.getValue(current) * kernel[i+1][j+1][k+1];
                    }
                }
            }
            red_new.setValue(*a, red_bin);
            green_new.setValue(*a, green_bin);
            blue_new.setValue(*a, blue_bin);
        }
        
        string frame_dir = output_dir + entry.path().filename().string();
        fs::create_directory(frame_dir);
        vdb2Raw(red_out, frame_dir + "/red");    
        vdb2Raw(green_out, frame_dir + "/green");    
        vdb2Raw(blue_out, frame_dir + "/blue");    
    }
}