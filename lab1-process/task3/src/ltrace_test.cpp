#include <cmath>
#include <format>
#include <iostream>

int main() {
    std::cout
        << std::format("Nobody knows calculation better than me")
        << std::endl;

    double x = 1.0;
    auto sinx = std::sin(x);
    auto cosx = std::cos(x);
    auto tanx = std::tan(x);
    std::cout
        << std::format(
               "sin({}) = {}, cos({}) = {}, tan({}) = {}.",
               x, sinx,
               x, cosx,
               x, tanx)
        << std::endl;

    return 0;
}
