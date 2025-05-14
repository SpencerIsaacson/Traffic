#define M_PI 3.14159265f
#define M_PI_2 (M_PI / 2)

float exp(float x) {
    float sum = 1.0f;
    float term = 1.0f;
    for (int i = 1; i <= 10; ++i) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

float log(float x) {
    if (x <= 0.0f) return 0.0f; // undefined, but avoid crash

    float y = 0.0f;
    for (int i = 0; i < 10; ++i) {
        y = y - 1 + x / exp(y);
    }
    return y;
}

float round(float f) {
    return f >= 0.0f ? (float)(int)(f + 0.5f) : (float)(int)(f - 0.5f);
}

float fabs(float f) {
    return f < 0 ? -f : f;
}

float sqrt(float f) {
    if (f <= 0) return 0;
    float guess = f;

    for (int i = 0; i < 5; ++i)
        guess = 0.5f * (guess + f / guess);

    return guess;
}

float sin(float x) {
    // Wrap x to [-π, π]
    while (x > M_PI) x -= 2 * M_PI;
    while (x < -M_PI) x += 2 * M_PI;

    float x2 = x * x;
    return x * (1 - x2 / 6 + x2 * x2 / 120);  // up to x^5 term
}

float cos(float x) {
    return sin(x + M_PI / 2);
}

float fmin(float a, float b) {
    return a < b ? a : b;
}

float fmax(float a, float b) {
    return a > b ? a : b;
}

float atan_approx(float z) {
    // Rational approximation of arctangent on [0,1]
    // atan(z) ≈ z / (1 + 0.28*z^2)
    return z / (1.0f + 0.28f * z * z);
}

float atan2(float y, float x) {
    if (x > 0.0f) {
        return atan_approx(y / x);
    } else if (x < 0.0f) {
        if (y >= 0.0f)
            return atan_approx(y / x) + M_PI;
        else
            return atan_approx(y / x) - M_PI;
    } else { // x == 0
        if (y > 0.0f)
            return M_PI_2;
        else if (y < 0.0f)
            return -M_PI_2;
        else
            return 0.0f; // undefined, return 0
    }

    return 0;
}