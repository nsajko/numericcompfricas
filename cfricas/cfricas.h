// Change this to your directory containing the necessary FriCAS
// package(s).
#define FricasLibDir "/home/nsajko/src/github.com/nsajko/numericcompfricas/fricas"

enum {
	// How many lines of Fricas's output to discard when it is
	// started. (Start-up messages, etc.)
	FloatFricasLinesToSkipAtStartup = 17,
};

// Bits of precision for floating point representation.
#define FloatFricasBits "32768"

typedef double ieee754FloatingPointNumber;

typedef struct {
	FILE *in, *out;
} FloatFricas;

ieee754FloatingPointNumber FricasFloatEval(FloatFricas, const char *, ieee754FloatingPointNumber);
FloatFricas FricasFloatNew(void);
int FricasClose(FloatFricas);
