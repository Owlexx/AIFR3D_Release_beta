#pragma once

#include "../Common/MixMetrics.h"
#include "AifredReport.h"

class DiagnosticEngine
{
  public:
    static AifredReport interpret(const MixMetrics& metrics);
};
