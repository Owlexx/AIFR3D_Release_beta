#include "dawai/aifr3d_core/reporting/mix_reporter.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Mix report generation is stable", "[reporting]")
{
    dawai::aifr3d_core::reporting::MixReporter reporter;
    dawai::aifr3d_core::reporting::MixReport report;
    report.timestampUtc = "2026-02-27T10-00-00Z";
    report.sessionName = "sessionA";
    report.referenceUsed = "ref1";
    report.rating = 8.2;
    report.fixList.push_back(
        {1, "Low End", "Changed", "Outside", "Trim boom", {"Safe", "Bold"}, 0.7});

    REQUIRE(reporter.writeJsonAndMarkdown("session_reports", report));

    const auto loaded = reporter.loadLastN("session_reports", 1);
    REQUIRE(loaded.size() == 1);
    REQUIRE(loaded.front().rating == Catch::Approx(8.2));

    const auto delta = dawai::aifr3d_core::reporting::compareReports(report, loaded.front());
    REQUIRE(delta.ratingDelta == Catch::Approx(0.0));
}
