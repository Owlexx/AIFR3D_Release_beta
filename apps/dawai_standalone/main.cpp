#include "dawai/engine.hpp"

#include <iostream>
#include <vector>

int main()
{
    dawai::Engine engine;

    std::vector<float> block{0.08f, -0.11f, 0.27f, -0.31f, 0.15f, -0.04f};
    const auto meter = engine.analyze(block);

    std::cout << "AIFR3D Standalone 2.2.4 Beta\n";
    std::cout << "Catalog entries: " << engine.catalog().size() << "\n";
    std::cout << "RMS=" << meter.rms << " Peak=" << meter.peak
              << " Crest=" << meter.crestFactor << "\n";
    std::cout << "Harshness=" << meter.harshness << " Mud=" << meter.mud
              << " TransientMatch=" << meter.transientAlignment
              << " StereoMatch=" << meter.stereoAlignment << "\n";
    return 0;
}
