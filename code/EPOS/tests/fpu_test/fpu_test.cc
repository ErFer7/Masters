#include <utility/ostream.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;

OStream cout;

Thread * calc[4];

Mutex print_lock;

const unsigned int DATA_SIZE = 100;

float addition_data[DATA_SIZE];
float subtraction_data[DATA_SIZE];
float multiplication_data[DATA_SIZE];
float division_data[DATA_SIZE];

int adder(int a, int b, int c);
int subtractor(int a, int b, int c);
int multiplier(int a, int b, int c);
int divider(int a, int b, int c);

int main()
{
    cout << "FPU test" << endl;

    print_lock.lock();

    calc[0] = new Thread(&adder, 1, 1, 0);
    calc[1] = new Thread(&subtractor, 1, 1, 0);
    calc[2] = new Thread(&multiplier, 2, 2, 0);
    calc[3] = new Thread(&divider, 100000, 10, 0);

    print_lock.unlock();

    for (int i = 0; i < 4; i++) {
        calc[i]->join();
    }

    for (int i = 0; i < 4; i++) {
        delete calc[i];
    }

    cout << "Done!" << endl;

    return 0;
}

int adder(int a, int b, int c) {
    cout << "Adder created!" << endl;

    addition_data[0] = static_cast<float>(a);
    addition_data[1] = static_cast<float>(b);

    print_lock.lock();
    cout << "Adder [0] = " << addition_data[0] << endl;
    cout << "Adder [1] = " << addition_data[1] << endl;
    print_lock.unlock();

    for (unsigned int i = 2; i < DATA_SIZE; i++) {
        addition_data[i] = addition_data[i - 2] + addition_data[i - 1];

        print_lock.lock();
        cout << "Adder [" << i << "]: " << addition_data[i - 2] << " + " << addition_data[i - 1] << " = " << addition_data[i] << endl;
        print_lock.unlock();
    }

    return c;
}

int subtractor(int a, int b, int c) {
    cout << "Subtractor created!" << endl;

    subtraction_data[0] = static_cast<float>(a);
    subtraction_data[1] = static_cast<float>(b);

    print_lock.lock();
    cout << "Subtractor [0] = " << subtraction_data[0] << endl;
    cout << "Subtractor [1] = " << subtraction_data[1] << endl;
    print_lock.unlock();

    for (unsigned int i = 2; i < DATA_SIZE; i++) {
        subtraction_data[i] = subtraction_data[i - 2] - subtraction_data[i - 1];

        print_lock.lock();
        cout << "Subtractor [" << i << "]: " << subtraction_data[i - 2] << " - " << subtraction_data[i - 1] << " = " << subtraction_data[i] << endl;
        print_lock.unlock();
    }

    return c;
}

int multiplier(int a, int b, int c) {
    cout << "Multiplier created!" << endl;

    multiplication_data[0] = static_cast<float>(a);
    multiplication_data[1] = static_cast<float>(b);

    print_lock.lock();
    cout << "Multiplier [0] = " << multiplication_data[0] << endl;
    cout << "Multiplier [1] = " << multiplication_data[1] << endl;
    print_lock.unlock();

    for (unsigned int i = 2; i < DATA_SIZE; i++) {
        multiplication_data[i] = multiplication_data[i - 2] * multiplication_data[i - 1];

        print_lock.lock();
        cout << "Multiplier [" << i << "]: " << multiplication_data[i - 2] << " * " << multiplication_data[i - 1] << " = " << multiplication_data[i] << endl;
        print_lock.unlock();
    }

    return c;
}

int divider(int a, int b, int c) {
    cout << "Divider created!" << endl;
    division_data[0] = static_cast<float>(a);
    division_data[1] = static_cast<float>(b);

    print_lock.lock();
    cout << "Division [0] = " << division_data[0] << endl;
    cout << "Division [1] = " << division_data[1] << endl;
    print_lock.unlock();

    for (unsigned int i = 2; i < DATA_SIZE; i++) {
        division_data[i] = division_data[i - 2] / division_data[i - 1];

        print_lock.lock();
        cout << "Division [" << i << "]: " << division_data[i - 2] << "/" << division_data[i - 1] << " = " << division_data[i] << endl;
        print_lock.unlock();
    }

    return c;
}
