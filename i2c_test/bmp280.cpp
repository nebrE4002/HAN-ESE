#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstdint>

#define I2C_DEVICE "/dev/i2c-1"
#define BMP280_ADDR 0x76   // Verander naar 0x77 indien nodig

int file;

// Kalibratie struct
struct BMP280_CalibData {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
};

BMP280_CalibData calib;
int32_t t_fine;

// Lees 8-bit register
uint8_t read8(uint8_t reg) {
    write(file, &reg, 1);
    uint8_t data;
    read(file, &data, 1);
    return data;
}

// Lees 16-bit little endian
uint16_t read16_LE(uint8_t reg) {
    uint8_t buf[2];
    write(file, &reg, 1);
    read(file, buf, 2);
    return (buf[1] << 8) | buf[0];
}

// Lees kalibratiegegevens
void readCalibration() {
    calib.dig_T1 = read16_LE(0x88);
    calib.dig_T2 = (int16_t)read16_LE(0x8A);
    calib.dig_T3 = (int16_t)read16_LE(0x8C);

    calib.dig_P1 = read16_LE(0x8E);
    calib.dig_P2 = (int16_t)read16_LE(0x90);
    calib.dig_P3 = (int16_t)read16_LE(0x92);
    calib.dig_P4 = (int16_t)read16_LE(0x94);
    calib.dig_P5 = (int16_t)read16_LE(0x96);
    calib.dig_P6 = (int16_t)read16_LE(0x98);
    calib.dig_P7 = (int16_t)read16_LE(0x9A);
    calib.dig_P8 = (int16_t)read16_LE(0x9C);
    calib.dig_P9 = (int16_t)read16_LE(0x9E);
}

// Compensatie temperatuur
double compensateTemperature(int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
              ((int32_t)calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    double T = (t_fine * 5 + 128) >> 8;
    return T / 100.0;
}

// Compensatie druk
double compensatePressure(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1*(int64_t)calib.dig_P5)<<17);
    var2 = var2 + (((int64_t)calib.dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3)>>8) +
           ((var1 * (int64_t)calib.dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calib.dig_P1)>>33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p<<31) - var2)*3125)/var1;
    var1 = (((int64_t)calib.dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7)<<4);
    return (double)p / 25600.0; // hPa
}

int main() {
    file = open(I2C_DEVICE, O_RDWR);
    if (file < 0) {
        std::cerr << "Fout bij openen I2C\n";
        return 1;
    }

    ioctl(file, I2C_SLAVE, BMP280_ADDR);

    // Sensor in normal mode
    uint8_t config[2];
    config[0] = 0xF4;
    config[1] = 0x27;  // temp en pressure aan
    write(file, config, 2);

    readCalibration();

    while (true) {
        uint8_t reg = 0xF7;
        uint8_t data[6];

        write(file, &reg, 1);
        read(file, data, 6);

        int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
        int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

        double temperature = compensateTemperature(adc_T);
        double pressure = compensatePressure(adc_P);

        std::cout << "Temp: " << temperature << " °C  ";
        std::cout << "Pressure: " << pressure << " hPa\n";

        sleep(1);
    }

    close(file);
    return 0;
}
