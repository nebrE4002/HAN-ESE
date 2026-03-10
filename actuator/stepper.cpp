#include <gpiod.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>

#define CHIP "/dev/gpiochip0"

unsigned int pins[4] = {5,6,13,19};

int sequence[8][4] = {
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cout << "Usage: ./stepper <speed>\n";
        std::cout << "Example: ./stepper 2000\n";
        std::cout << "Reverse: ./stepper -2000\n";
        return 1;
    }

    int speed = atoi(argv[1]);
    bool reverse = false;

    if(speed < 0)
    {
        reverse = true;
        speed = abs(speed);
    }

    gpiod_chip *chip = gpiod_chip_open(CHIP);
    if(!chip)
    {
        std::cerr << "Failed to open gpiochip\n";
        return 1;
    }

    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, pins, 4, settings);

    gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "stepper");

    gpiod_line_request *request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    if(!request)
    {
        std::cerr << "Failed to request GPIO lines\n";
        return 1;
    }

    while(true)
    {
        if(!reverse)
        {
            for(int step=0; step<8; step++)
            {
                for(int i=0;i<4;i++)
                {
                    gpiod_line_value value =
                        sequence[step][i] ?
                        GPIOD_LINE_VALUE_ACTIVE :
                        GPIOD_LINE_VALUE_INACTIVE;

                    gpiod_line_request_set_value(request, pins[i], value);
                }

                usleep(speed);
            }
        }
        else
        {
            for(int step=7; step>=0; step--)
            {
                for(int i=0;i<4;i++)
                {
                    gpiod_line_value value =
                        sequence[step][i] ?
                        GPIOD_LINE_VALUE_ACTIVE :
                        GPIOD_LINE_VALUE_INACTIVE;

                    gpiod_line_request_set_value(request, pins[i], value);
                }

                usleep(speed);
            }
        }
    }

    gpiod_chip_close(chip);
}
