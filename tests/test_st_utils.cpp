#include <catch2/catch_test_macros.hpp>

#include "st_utils.hpp"

TEST_CASE("String is converted to lowercase", "[tolower]") {
    auto upper_in = std::string("HELLO");
    auto lower_in = std::string("hello");
    auto mixed_in = std::string("HeLlO");

    auto expected = std::string("hello");

    SECTION("Uppercase input")
    {
        auto actual = stToLower(upper_in);
        REQUIRE(expected == actual);
    }
    SECTION("Lowercase input")
    {
        auto actual = stToLower(lower_in);
        REQUIRE(expected == actual);
    }
    SECTION("Mixed input")
    {
        auto actual = stToLower(mixed_in);
        REQUIRE(expected == actual);
    }


}

TEST_CASE("Case insensitive comparison", "[stInsensitiveCmp]") {
    auto upper_in_01 = std::string("HELLO");
    auto upper_in_02 = std::string("HELLO");

    auto lower_in = std::string("hello");
    auto mixed_in = std::string("HeLlO");
    auto different_in_01 = std::string("Something Else");
    auto different_in_02 = std::string("QwErt");
    auto expected = std::string("hello");

    SECTION("Equal Input")
    {
        REQUIRE(stInsensitiveCmp(upper_in_01, upper_in_01) == true);
        REQUIRE(stInsensitiveCmp(upper_in_01, upper_in_02) == true);
    }
    SECTION("Mixed cases")
    {
        REQUIRE(stInsensitiveCmp(upper_in_01, lower_in) == true);
        REQUIRE(stInsensitiveCmp(mixed_in, upper_in_01) == true);
    }
    SECTION("Different, same length")
    {
        REQUIRE(stInsensitiveCmp(upper_in_01, different_in_02) == false);
    }
    SECTION("Different, different length")
    {
        REQUIRE(stInsensitiveCmp(upper_in_01, different_in_01) == false);
    }
}
