#include <serialization/Serial.hpp>

#include <iostream>

int main() {
    serialization::Serial serial;

    int i = 4, j;

    serial << i;
    serial >> j;

    std::cout << i << " == " << j << std::endl;
}
