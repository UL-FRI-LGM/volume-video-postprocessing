#include <math.h>
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

string getModelName(string path) 
{
    using boost::split, boost::is_any_of;
    vector<string> strs;
    split(strs, path, is_any_of("/"));
    path = strs.back();
    split(strs, path, is_any_of("-"));
    return strs[0];
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

openvdb::FloatGrid::Ptr convolveX(openvdb::FloatGrid::Ptr grid, vector<float> kernel) {
    auto const_grid = grid->getConstAccessor();
    auto output = grid->deepCopy();
    auto output_grid = output->getAccessor();
    int size = kernel.size();
    int pol = -floor(size*0.5);
    for (auto a = grid->evalActiveVoxelBoundingBox().beginXYZ(); a; ++a) {
        float acc = 0;
        for (int i = 0; i < size; i++) {
            acc += kernel[i] * const_grid.getValue(*a + openvdb::v10_0::math::Coord(i + pol, 0, 0));
        }
        output_grid.setValue(*a, acc);
    }
    // tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads());
    // auto output = grid->deepCopy();
    // int size = kernel.size();
    // int pol = -floor(size * 0.5);
    // tbb::parallel_for_each(grid->evalActiveVoxelBoundingBox().beginXYZ(), [&](auto a) {
    //     auto const_grid = grid->getConstAccessor();
    //     auto output_grid = output->getAccessor();
    //     float acc = 0;
    //     for (int i = 0; i < size; i++) {
    //         acc += kernel[i] * const_grid.getValue(*a + openvdb::v10_0::math::Coord(i + pol, 0, 0));
    //     }
    //     output_grid.setValue(*a, acc);
        
    // });
    return output;
}

openvdb::FloatGrid::Ptr convolveY(openvdb::FloatGrid::Ptr grid, vector<float> kernel) {
    auto const_grid = grid->getConstAccessor();
    auto output = grid->deepCopy();
    auto output_grid = output->getAccessor();
    int size = kernel.size();
    int pol = -floor(size*0.5);
    for (auto a = grid->evalActiveVoxelBoundingBox().beginXYZ(); a; ++a) {
        float acc = 0;
        for (int i = 0; i < size; i++) {
            acc += kernel[i] * const_grid.getValue(*a + openvdb::v10_0::math::Coord(0, i + pol, 0));
        }
        output_grid.setValue(*a, acc);
    }
    return output;
}

openvdb::FloatGrid::Ptr convolveZ(openvdb::FloatGrid::Ptr grid, vector<float> kernel) {
    auto const_grid = grid->getConstAccessor();
    auto output = grid->deepCopy();
    auto output_grid = output->getAccessor();
    int size = kernel.size();
    int pol = -floor(size*0.5);
    for (auto a = grid->evalActiveVoxelBoundingBox().beginXYZ(); a; ++a) {
        float acc = 0;
        for (int i = 0; i < size; i++) {
            acc += kernel[i] * const_grid.getValue(*a + openvdb::v10_0::math::Coord(0, 0, i + pol));
        }
        output_grid.setValue(*a, acc);
    }
    return output;
}

openvdb::FloatGrid::Ptr DoG(openvdb::FloatGrid::Ptr grid) {
    vector<float> small_gauss = {0.05399113, 0.24197145, 0.39894347, 0.24197145, 0.05399113};
    vector<float> big_gauss = {0.0010282, 0.0075974, 0.03599435, 0.10934117, 0.21296753, 0.26596426, 0.21296753, 0.10934117, 0.03599435, 0.0075974, 0.0010282};
    auto small_blur = convolveZ(convolveY(convolveX(grid, small_gauss), small_gauss), small_gauss);
    auto big_blur = convolveZ(convolveY(convolveX(grid, big_gauss), big_gauss), big_gauss);
    
    auto out = big_blur->deepCopy();
    struct Local {
        static inline void diff(const float& a, const float& b, float& result) {
            result = a - b;
        }
    };
    out->tree().combine(small_blur->deepCopy()->tree(), Local::diff);
    return out;
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
        vector<openvdb::FloatGrid::Ptr> alpha_grids;
        openvdb::FloatGrid::Ptr cutoff;
        for (const auto & frame_dir : fs::directory_iterator(entry.path()))
        {
            // Parse filename to get color
            vector<float> color = getModelColors(frame_dir.path());
        	    // std::cout << getModelName(frame_dir.path()) << std::endl;

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
	    if (getModelName(frame_dir.path()) == "capture_range") {
                cutoff = volume;
	    }

            // red_grids.push_back(volume);
            red_grids.push_back(swapValueCopy(volume, color[0]));
            green_grids.push_back(swapValueCopy(volume, color[1]));
            blue_grids.push_back(swapValueCopy(volume, color[2]));
            alpha_grids.push_back(swapValueCopy(volume, color[3]));
        }

        // Combine grids into accumulator
        // openvdb::FloatGrid::Ptr red = openvdb::tools::csgIntersectionCopy(*combineGrids(red_grids), *cutoff);
        // openvdb::FloatGrid::Ptr green = openvdb::tools::csgIntersectionCopy(*combineGrids(green_grids), *cutoff);
        // openvdb::FloatGrid::Ptr blue = openvdb::tools::csgIntersectionCopy(*combineGrids(blue_grids), *cutoff);
        // openvdb::FloatGrid::Ptr alpha = openvdb::tools::csgIntersectionCopy(*combineGrids(alpha_grids), *cutoff);
        openvdb::FloatGrid::Ptr red = combineGrids(red_grids);
        openvdb::FloatGrid::Ptr green = combineGrids(green_grids);
        openvdb::FloatGrid::Ptr blue = combineGrids(blue_grids);
        openvdb::FloatGrid::Ptr alpha = combineGrids(alpha_grids);

        // Change background
        // float colour[4] = {0.1, 0.0, 0.3, 0.1};
        // openvdb::FloatGrid::Accessor red_acc = red->getAccessor();
        // openvdb::FloatGrid::Accessor green_acc = green->getAccessor();
        // openvdb::FloatGrid::Accessor blue_acc = blue->getAccessor();
        // openvdb::FloatGrid::Accessor alpha_acc = alpha->getAccessor();
        // for (auto i = alpha->evalActiveVoxelBoundingBox().beginXYZ(); i; ++i) {
        //     auto r = red_acc.getValue(*i);
        //     auto g = green_acc.getValue(*i);
        //     auto b = blue_acc.getValue(*i);
        //     if (alpha_acc.getValue(*i) < 0.11) {
        //         red_acc.setValue(*i, colour[0]);
        //         green_acc.setValue(*i, colour[1]);
        //         blue_acc.setValue(*i, colour[2]);
        //         alpha_acc.setValue(*i, colour[3]);
        //     }
        // }

        // Blur
        // string frame_dir = output_dir + entry.path().filename().string();
        // fs::create_directory(frame_dir);

        // vector<float> gauss = {0.12098749, 0.17603576, 0.19947465, 0.17603576, 0.12098749};
        // vdb2Raw(convolveZ(convolveY(convolveX(red, gauss), gauss), gauss), frame_dir + "/red");
        // vdb2Raw(convolveZ(convolveY(convolveX(green, gauss), gauss), gauss), frame_dir + "/green");
        // vdb2Raw(convolveZ(convolveY(convolveX(blue, gauss), gauss), gauss), frame_dir + "/blue");
        // vdb2Raw(convolveZ(convolveY(convolveX(alpha, gauss), gauss), gauss), frame_dir + "/alpha");

        // Detect surface
        string frame_dir = output_dir + entry.path().filename().string();
        fs::create_directory(frame_dir);
        vdb2Raw(DoG(red), frame_dir + "/red");
        vdb2Raw(DoG(green), frame_dir + "/green");
        vdb2Raw(DoG(blue), frame_dir + "/blue");
        vdb2Raw(DoG(alpha), frame_dir + "/alpha");

        // Normal
        // string frame_dir = output_dir + entry.path().filename().string();
        // fs::create_directory(frame_dir);
        // vdb2Raw(red, frame_dir + "/red");
        // vdb2Raw(green, frame_dir + "/green");
        // vdb2Raw(blue, frame_dir + "/blue");
        // vdb2Raw(alpha, frame_dir + "/alpha");
    }
}
