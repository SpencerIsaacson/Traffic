typedef unsigned int uint32_t;
#define PERIPHERAL_BASE 0x3F000000
#define PWM_BASE       (PERIPHERAL_BASE + 0x20C000)
#define CLK_BASE       (PERIPHERAL_BASE + 0x101000)
#define GPIO_BASE      (PERIPHERAL_BASE + 0x200000)

#define PWM_CTL        (*(volatile uint32_t*)(PWM_BASE + 0x00))
#define PWM_RNG1       (*(volatile uint32_t*)(PWM_BASE + 0x10))
#define PWM_DAT1       (*(volatile uint32_t*)(PWM_BASE + 0x14))

#define CM_PWMCTL      (*(volatile uint32_t*)(CLK_BASE + 0xA0))
#define CM_PWMDIV      (*(volatile uint32_t*)(CLK_BASE + 0xA4))

#define GPFSEL1         (*(volatile uint32_t*)(GPIO_BASE + 0x04))  // GPIO 10–19
#define GPFSEL2         (*(volatile uint32_t*)(GPIO_BASE + 0x08))  // GPIO 20–29

void set_gpio_alt(int gpio, int alt) {
    volatile uint32_t* reg;
    int shift;

    if (gpio < 10) {
        reg = (volatile uint32_t*)(GPIO_BASE + 0x00);  // GPFSEL0
        shift = gpio * 3;
    } else if (gpio < 20) {
        reg = GPFSEL1;
        shift = (gpio - 10) * 3;
    } else if (gpio < 30) {
        reg = GPFSEL2;
        shift = (gpio - 20) * 3;
    } else {
        return; // unsupported GPIO
    }

    uint32_t val = *reg;
    val &= ~(7 << shift);        // clear existing 3 bits
    val |= (alt & 7) << shift;   // set new alt
    *reg = val;
}

void pwm_audio_init() {
    // Stop clock
    CM_PWMCTL = 0x5A000000 | (1 << 5); // Password + kill bit
    //delay(); // brief pause

    // Set clock divider (e.g., divide 500MHz to 10MHz)
    CM_PWMDIV = 0x5A000000 | (50 << 12); // divide by 50
    CM_PWMCTL = 0x5A000011; // enable, source = PLLD (500 MHz)

    // Setup GPIO 18 to ALT5 (PWM0)
    //set_gpio_alt(18, 5); 

    PWM_CTL = 0; // disable
    //delay();

    PWM_RNG1 = 1024; // range (acts like max sample)
    //delay();

    PWM_DAT1 = 512; // neutral value
    PWM_CTL = (1 << 0) | (1 << 7); // enable channel 1 + MSEN1 (mark-space)
}