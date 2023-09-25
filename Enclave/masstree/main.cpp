#include <iostream>

#include "include/masstree.h"

void worker() {
    return;
}

int main() {
    Masstree masstree;
    GarbageCollector gc;

    Key k0{{0}, 1};
    masstree.put(k0, new Value{0}, gc);


    for (size_t i = 1; i < 100; i++) {
        Key *test = new Key({i}, 1);
        masstree.put(*test, new Value{i}, gc);
        Value *value = masstree.get(*test);
        if (value->getBody() == i) std::cout << "unchi!" << std::endl;
    }

    return 0;
}