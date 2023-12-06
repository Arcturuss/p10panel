#include <Arduino.h>
#include <SPI.h>

const int OE = 13;
const int A = 13;
const int B = 8;
const int R = 12;
const int CLK = 2;
const int SCLK = 4;

byte buf[512];
char sprintbuf[128];

volatile uint8_t * port_a;
volatile uint8_t * port_clk;
uint8_t mask_A;
uint8_t mask_B;
uint8_t mask_R;
uint8_t mask_OE;
uint8_t mask_CLK;
uint8_t mask_SCLK;

#define FRAMETIMES

// #define dw digitalWrite
#define dw2(port, m, b)              \
    {                                \
        if (b == 0) *port &= m;      \
        else *port |= m;             \
    }
#define unsugarC(c)                  \
    {                                \
        dw2(port_a, A, (c & 1));              \
        dw2(port_a, B, ((c & 1) ^ (c >> 1))); \
    }

int coordToP(int x, int y)
{
    return (((31 - x) >> 3) << 7) | ((y >> 2) << 5) | ((7 - x & 7) << 2) | (y & 3);
}

void setup()
{
    Serial.begin(115200);

    port_a = portOutputRegister(digitalPinToPort(A)); // pins 8-13 = PORT_B
    port_clk = portOutputRegister(digitalPinToPort(CLK)); // pins 0-7 = PORT_D

    mask_OE = digitalPinToBitMask(OE);
    mask_A = digitalPinToBitMask(A);
    mask_B = digitalPinToBitMask(B);
    mask_R = digitalPinToBitMask(R);
    mask_CLK = digitalPinToBitMask(CLK);
    mask_SCLK = digitalPinToBitMask(SCLK);

    // *port_a |= mask_A; // equivalent to digitalWrite(A, HIGH)
    // *port_a &= ~mask_A; // equivalent to digitalWrite(A, LOW)
    
    pinMode(OE, OUTPUT);
    pinMode(A, OUTPUT);
    pinMode(B, OUTPUT);
    pinMode(R, OUTPUT);
    pinMode(CLK, OUTPUT);
    pinMode(SCLK, OUTPUT);
    dw2(port_a, mask_OE, 0);
    for (int x = 0; x < 32; x++)
    {
        for (int y = 0; y < 16; y++)
        {
            double len = sqrt((x - 5) * (x - 5) + (y - 5) * (y - 5));
            if (len <= 10)
            {
                int br = (10.0 - len) * 4;
                if (br > 4)
                    br = 4;
                buf[coordToP(x, y)] = br;
            }
        }
    }
}
#ifdef FRAMETIMES
int clock = 0;
long maxdiff = 0;
long mindiff = 999999;
#endif
void loop()
{
#ifdef FRAMETIMES
    clock += 1;
    clock %= 1000;
    long time = micros();
#endif

    {
        for (int c = 0; c < 4; c++)
        {
            unsugarC(c);
            {
                for (int p = 0; p < 128; p++)
                {
                    int px = ((p << 2) | c);
                    dw2(port_a, mask_R, !(buf[px]));
                    dw2(port_clk, mask_CLK, 0);
                    dw2(port_clk, mask_CLK, 1);
                }
                dw2(port_clk, mask_SCLK, 0);
                dw2(port_clk, mask_SCLK, 1);
            }
        }
    }
#ifdef FRAMETIMES
    long diff = micros() - time;
    if (diff < mindiff) mindiff = diff;
    if (diff > maxdiff) maxdiff = diff;
    if (clock == 0)
    {
        sprintf(sprintbuf, "Frame in %ld (%ld - %ld)", diff, mindiff, maxdiff);
        Serial.println(sprintbuf);
        maxdiff = 0;
        mindiff = 999999;
    }
#endif
}