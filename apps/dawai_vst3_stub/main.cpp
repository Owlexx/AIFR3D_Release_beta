#include "dawai/engine.hpp"

#include <iostream>

int main()
{
    dawai::Engine engine;
    std::cout << "DawAI VST3 Baseline Stub\n";
    std::cout << "Catalog entries: " << engine.catalog().size() << "\n";
    std::cout << "DSP math pack contract: docs/dsp_math_pack_v1.md\n";
    std::cout << "JUCE VST3 runtime integration is the next layer on this baseline.\n";
    return 0;
}
