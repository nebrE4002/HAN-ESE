#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstdint>
#include <cstdlib>   // voor atoi()

#define I2C_DEVICE "/dev/i2c-1"
#define ADS1115_ADDR 0x48

const float LSB_SIZE = 4.096f / 32768.0f;

// Config MSB waarden voor single-ended kanalen
uint8_t channel_config[4] = {
    0xC3,  // A0
    0xD3,  // A1
    0xE3,  // A2
    0xF3   // A3
};

float readChannel(int file, uint8_t config_msb)
{
    uint8_t config[3];
    config[0] = 0x01;        // Config register
    config[1] = config_msb;
    config[2] = 0x83;        // 128 SPS, single-shot

    write(file, config, 3);

    usleep(8000);  // wacht op conversie (~8ms bij 128 SPS)

    uint8_t reg[1] = {0x00};  // Conversion register
    write(file, reg, 1);

    uint8_t data[2];
    read(file, data, 2);

    int16_t raw = (data[0] << 8) | data[1];

    return raw * LSB_SIZE;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout << "Gebruik: ./ads1115 <delay in ms>\n";
        return 1;
    }

    int delay_ms = std::atoi(argv[1]);

    if (delay_ms <= 0)
    {
        std::cout << "Delay moet groter zijn dan 0 ms\n";
        return 1;
    }

    int file;

    if ((file = open(I2C_DEVICE, O_RDWR)) < 0) {
        std::cerr << "Kan I2C bus niet openen\n";
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ADS1115_ADDR) < 0) {
        std::cerr << "Kan ADS1115 niet vinden\n";
        return 1;
    }

    std::cout << "Start met meten (delay = " << delay_ms << " ms)\n";

    while (true)
    {
        for (int ch = 0; ch < 1; ch++)
        {
            float voltage = readChannel(file, channel_config[ch]);
            std::cout << "A" << ch << ": " << voltage << " V   ";
        }

        std::cout << std::endl;

        usleep(delay_ms * 1000);  
    }

    close(file);
    return 0;
}
