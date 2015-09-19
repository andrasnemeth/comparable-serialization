#include <serialization/Serial.hpp>

#include <iostream>

template<typename T>
void convertAndPrint(const T& value) {
    serialization::Serial<> serial;
    serial << value;

    T reconstructedValue;
    serial >> reconstructedValue;

    std::cout << value << " == " << reconstructedValue << std::endl;
}

template<typename T>
void compareAndPrint(const T& value1, const T& value2) {
    serialization::Serial<> s1, s2;
    s1 << value1;
    s2 << value2;

    std::cout << value1 << " == " << value2 << " = " << (s1 == s2) << "\n"
              << value1 << " > " << value2 << " = " << (s1 > s2) << std::endl;
}

int main() {
    int i = 345;
    convertAndPrint(i);
    convertAndPrint(-3434);
    compareAndPrint(454543, -31343);

    double d = 3.45;
    convertAndPrint(d);

    compareAndPrint(3.45, -1.23);
    compareAndPrint(1.0, 0.000000000000000000000000000000000000000000000000004);
}
