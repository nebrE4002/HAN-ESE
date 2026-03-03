#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <unistd.h>

using namespace std;

// BOARD 16 ? BCM23
const int GPIO = 23;  

// Servo PWM instellingen
const int PERIOD_US = 20000;  // 20ms = 50Hz
const int SERVO_0 = 1000;     // 0° pulse width in µs
const int SERVO_90 = 2000;    // 90° pulse width in µs
const int SERVO_MINUS_90 = 1500; // -90° pulse width in µs

// Schrijf een waarde naar sysfs file
void writeFile(const string& path, const string& value) {
    ofstream file(path);
    if (!file.is_open()) {
        cerr << "Kan bestand niet openen: " << path << endl;
        exit(1);
    }
    file << value;
    file.close();
}

// GPIO exporteren
void exportGPIO(int gpio) {
    string path = "/sys/class/gpio/gpio" + to_string(gpio);
    ifstream check(path);
    if (!check.is_open()) {  // alleen exporteren als nog niet bestaat
        writeFile("/sys/class/gpio/export", to_string(gpio));
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

// GPIO richting instellen
void setDirection(int gpio, const string& dir) {
    writeFile("/sys/class/gpio/gpio" + to_string(gpio) + "/direction", dir);
}

// GPIO waarde zetten
void writeGPIO(int gpio, int value) {
    writeFile("/sys/class/gpio/gpio" + to_string(gpio) + "/value", to_string(value));
}

// Software PWM pulse
void servoPulse(int gpio, int pulse_us) {
    writeGPIO(gpio, 1);
    this_thread::sleep_for(chrono::microseconds(pulse_us));
    writeGPIO(gpio, 0);
    this_thread::sleep_for(chrono::microseconds(PERIOD_US - pulse_us));
}

int main() {
    // check of we root zijn
    if (geteuid() != 0) {
        cerr << "Run dit programma met sudo!" << endl;
        return 1;
    }

    cout << "Servo PWM starten op GPIO" << GPIO << endl;

    exportGPIO(GPIO);
    setDirection(GPIO, "out");

    while (true) {
        servoPulse(GPIO, SERVO_0);
        this_thread::sleep_for(chrono::milliseconds(500));

        servoPulse(GPIO, SERVO_MINUS_90);
        this_thread::sleep_for(chrono::milliseconds(500));

        servoPulse(GPIO, SERVO_90);
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    return 0;
}
