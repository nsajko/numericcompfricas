// Copyright © 2020 Neven Sajko. All rights reserved.

// Compares the libc libm sin and cos to a certain implementation that
// returns the sine, cosine and 1-cosine.
//
// Interpreting the output:
//
// This program outputs relevant info to stdout. The first section
// contains lines, each of which represents one point (input value)
// where a function's result differ significantly between the libc and
// my code, relative to the distance from the value that should be
// accurate (see below for how we get that). The line contains eight
// fields:
//
// * a string: "better" or "worse"
// * the input value
// * the name of the math function whose evaluation is being considered
// * a string that says whether the old and new results differ in the
//   sign or exponent or in how many bits of the mantissa (significand)
//   they differ.
// * the ULP distance between the old and new result
// * the old result for the function value
// * the new result
// * the accurate result (computed with high precision, with a CAS)
//
// Following the first section there is a line that tells you how many
// points are there in each range of consecutive IEEE 754 numbers that
// are being operated on.
//
// Last comes the section with an entry for each mathematical function
// that is being considered here, starting with the function's name.
// After that there is an entry with statistics for each range. Only
// the points whose function value differs between the old and new
// implementations are being added to the stats. Every range entry has
// four lines:
//
// * the endpoints of the interval
// * data for improvements: count, maximum (iscore), maximum relative to
//   the difference from the accurate value (fscore), quadratic mean
// * data for deteriorations: same as for improvements
// * arithmetic mean of differences between the old and new function
//   values, for all points where there is a difference between the old
//   and new.
//
// When a function has no range entries, it is because in all the
// considered points the old and new evaluations were equal.
//
// The purpose of the first section is basically to show the salient
// differences in the results between the implementations, and the
// second section tries to judge which is better with some statistics.
//
// Note the difference between the first section's ULP distance and the
// second section's iscore: the ULP distance is between the old and new
// values, while the iscore is the difference between ULP distances
// between old and accurate values and new and accurate values. Thus the
// second tells one how much better or worse the new result is compared
// to the old one. (The fscore is the same, but in relative terms.)
//
// The FriCAS computer algebra system is used for (hopefully) accurate
// computation of values of the mathematical functions. (Ensure
// 'fricas' is in PATH.)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cfricas.h>

typedef long int64;
typedef unsigned long uint64;

#define nil 0

typedef double mfloat_t;
typedef int mint_t;

/* The following code (sincos1cos and float and double versions of
 * sncs1cs) is Copyright © 1985, 1995, 2000 Stephen L. Moshier and
 * Copyright © 2020 Neven Sajko. The intention is to get accurate
 * 1-cosine, while also getting the sine and cosine as a bonus. The
 * implementation is derived from the Cephes Math Library's sin.c and
 * sinf.c. To be more specific, I took Stephen Moshier's sin, cos, sinf
 * and cosf (without changing the polynomials) and adapted them to give
 * all three required function values (in double and float versions),
 * without unnecessary accuracy losses.
 *
 * sncs1cs is not correct for values of x of huge magnitude. That can
 * be fixed by more elaborate range reduction.
 */

typedef struct {
	/* Sine, cosine, 1-cosine */
	mfloat_t sin, cos, omc;
} sincos1cos;

static
sincos1cos
sncs1cs(mfloat_t x) {
	const mfloat_t fourOverPi = 1.27323954473516268615;

	mfloat_t y, z, zz;
	mint_t j, sign = 1, csign = 1;
	sincos1cos r;

	/* Handle +-0. */
	if (x == (mfloat_t)0) {
		r.sin = x;
		r.cos = 1;
		r.omc = 0;
		return r;
	}
	if (isnan(x)) {
		r.sin = r.cos = r.omc = x;
		return r;
	}
	if (isinf(x)) {
		r.sin = r.cos = r.omc = x - x;
		return r;
	}
	if (x < 0) {
		sign = -1;
		x = -x;
	}
	j = (mint_t)(x * fourOverPi);
	y = (mfloat_t)j;
	/* map zeros to origin */
	if ((j & 1)) {
		j += 1;
		y += 1;
	}
	j = j & 7; /* octant modulo one turn */
	/* reflect in x axis */
	if (j > 3) {
		sign = -sign;
		csign = -csign;
		j -= 4;
	}
	if (j > 1) {
		csign = -csign;
	}

	const double sc[] = {
		1.58962301576546568060E-10,
		-2.50507477628578072866E-8,
		2.75573136213857245213E-6,
		-1.98412698295895385996E-4,
		8.33333333332211858878E-3,
		-1.66666666666666307295E-1,
	};

	const double cc[] = {
		-1.13585365213876817300E-11,
		2.08757008419747316778E-9,
		-2.75573141792967388112E-7,
		2.48015872888517045348E-5,
		-1.38888888888730564116E-3,
		4.16666666666665929218E-2,
	};

	const double DP1 = 7.85398125648498535156E-1;
	const double DP2 = 3.77489470793079817668E-8;
	const double DP3 = 2.69515142907905952645E-15;

	/* Extended precision modular arithmetic */
	z = ((x - y * DP1) - y * DP2) - y * DP3;
	zz = z * z;
	r.sin = z + zz*z*(((((sc[0]*zz + sc[1])*zz + sc[2])*zz + sc[3])*zz + sc[4])*zz + sc[5]);
	r.omc = (mfloat_t)0.5*zz - zz*zz*(((((cc[0]*zz + cc[1])*zz + cc[2])*zz + cc[3])*zz + cc[4])*zz + cc[5]);

	if (j == 1 || j == 2) {
		if (csign < 0) {
			r.sin = -r.sin;
		}
		r.cos = r.sin;
		r.sin = 1 - r.omc;
		r.omc = 1 - r.cos;
	} else {
		if (csign < 0) {
			r.cos = r.omc - 1;
			r.omc = 1 - r.cos;
		} else {
			r.cos = 1 - r.omc;
		}
	}
	if (sign < 0) {
		r.sin = -r.sin;
	}
	return r;
}

enum {
	sinIndex,
	cosIndex,
	omcIndex,
	FuncLimit,

	PointsInOneRange = 32,
};

#define FLTFMT "%27.20e"

#define TMPLT "(" FLTFMT ")$CNF\n"

static const char *const funcNames[] = {"sin", "cos", "omc"};
static const char *const fricasFuncNames[] = {"cnf_sin" TMPLT, "cnf_cos" TMPLT, "cnf_1cs" TMPLT};

static const mfloat_t posInf = 1.0/0.0;

typedef struct {
	mfloat_t old, new, accurate;
} funcVal;

typedef struct {
	funcVal a[PointsInOneRange][FuncLimit];
	mfloat_t limits[2];
} Range;

typedef struct {
	// Interface to FriCAS
	FloatFricas fr;

	// Slice of ranges of points.
	Range *funcData;
	int i;
} dat;

static
int
interesting(long d) {
	return d != 0;
}

// ULP distance. Distance between 0.0 and -0.0 is taken to be 0.
//
// If x or y are NaN, the distance is taken to be the greatest positive
// value of the return type.
static
int64
ud(mfloat_t x, mfloat_t y) {
	// Handle NaNs.
	if (x != x || y != y) {
		return (int64)(~0UL >> 1);
	}

	union {mfloat_t X; uint64 a;} u1;
	union {mfloat_t Y; uint64 b;} u2;
	u1.X = x;
	u2.Y = y;
	uint64 a = u1.a, b = u2.b;

	if ((a & (1UL << 63)) == (b & (1UL << 63))) {
		// a and b are of the same sign.
		a = a - b;
		if ((a & (1UL << 63))) {
			return -a;
		}
		return a;
	}
	return (int64)(a - (1UL << 63) + b);
}

static
const char *
quiteInteresting(funcVal v) {
	int64 ac = ud(v.old, v.accurate), bc = ud(v.new, v.accurate);
	const mfloat_t L = 1e-7; // Needs to be positive and close to zero.
	if ((mfloat_t)(ac - bc)/(mfloat_t)(bc) > L) {
		return "better";
	}
	if ((mfloat_t)(bc - ac)/(mfloat_t)(ac) > L) {
		return "worse ";
	}
	return nil;
}

static
void
about(mfloat_t old, mfloat_t new, char *r) {
	union {mfloat_t oldF; uint64 oldI;} u1;
	union {mfloat_t newF; uint64 newI;} u2;
	u1.oldF = old;
	u2.newF = new;
	if (((u1.oldI ^ u2.newI) & 0xfff0000000000000)) {
		sprintf(r, "Exponents or signs differ !");
		return;
	}

	int64 d = (int64)(u1.oldI - u2.newI);
	if (d < 0) {
		d = -d;
	}
	// Floor of the binary logarithm AKA position of the MSB.
	int n;
	for (n = 1;; n++) {
		d >>= 1;
		if (d == 0) {
			break;
		}
	}

	sprintf(r, "Mantissas differ in %2d bits", n);
}

typedef struct {
	int64 iscor;
	mfloat_t fscor;
} ifscor;

static
ifscor
scoresOf(funcVal v) {
	int64 ac = ud(v.old, v.accurate), bc = ud(v.new, v.accurate);
	ifscor r = {ac - bc, 0};
	r.fscor = (mfloat_t)r.iscor / (mfloat_t)bc;
	return r;
}

static
int
isNull(funcVal v) {
	return v.old == v.new;
}

// Record all interesting differences between old and new values of
// mathematical functions.
static
void
checkSinCosOmcInPoint(dat *data, int pointInRange, mfloat_t x) {
	funcVal a[FuncLimit] = {{sin(x), 0, 0}, {cos(x), 0, 0}, {0, 0, 0}};
	a[omcIndex].old = 1 - a[cosIndex].old;
	sincos1cos sc1c = sncs1cs(x);
	a[sinIndex].new = sc1c.sin;
	a[cosIndex].new = sc1c.cos;
	a[omcIndex].new = sc1c.omc;

	int i;
	funcVal *funcData = data->funcData[data->i].a[pointInRange];
	for (i = 0; i < FuncLimit; i++) {
		const char *const f = "%6s " FLTFMT " %3s: %30s %22ld " FLTFMT " " FLTFMT " " FLTFMT "\n";
		int64 diff = ud(a[i].old, a[i].new);
		if (interesting(diff)) {
			funcData[i] = a[i];
			funcData[i].accurate = FricasFloatEval(data->fr, fricasFuncNames[i], x);
			const char *s = quiteInteresting(funcData[i]);
			if (s != nil) {
				char buf[30];
				about(funcData[i].old, funcData[i].new, buf);
				printf(f, s, x, funcNames[i], buf, diff, funcData[i].old, funcData[i].new, funcData[i].accurate);
			}
		}
	}
}

// Check mathematical functions in PointsInOneRange points after and
// including x.
static
void
testRange(dat *data, mfloat_t x) {
	data->funcData[data->i].limits[0] = x;
	int i;
	for (i = 0; i < PointsInOneRange; i++) {
		checkSinCosOmcInPoint(data, i, x);
		x = nextafter(x, posInf);
	}
	data->funcData[data->i].limits[1] = x;
}

typedef struct {
	int count;
	int64 max;      // max integer (absolute) score
	mfloat_t maxScor; // max floating point (relative) score
	mfloat_t mean2;   // quadratic mean of the relative scores
} microReport;

// For making the final report, represents a range of consecutive IEEE 754 numbers.
typedef struct {
	// unordered set containing the max and min input values
	mfloat_t limits[2];

	microReport improvements, worsenings;

	// arithmetic mean of the relative scores
	mfloat_t mean1;
} rangeReport;

static
void
updateMicroReport(microReport *r, int64 iS, mfloat_t fS) {
	r->count++;
	r->mean2 += fS * fS;
	if (0 <= iS && r->max < iS || iS <= 0 && iS <= r->max) {
		r->max = iS;
	}
	if (copysign(r->maxScor, (mfloat_t)1) < copysign(fS, (mfloat_t)1)) {
		r->maxScor = fS;
	}
}

int
main(void) {
	const mfloat_t fourPi = 12.5663706143591729539, step = 0.03125;
	const int size = 2*(int)((fourPi + 0.5)/step + 0.5) + 1;
	dat data = {FricasFloatNew(), calloc(size, sizeof(Range)), 0};
	if (data.fr.in == nil || data.fr.out == nil) {
		fprintf(stderr, "sinCosOmcTester: failed to use fricas\n");
		return 1;
	}
	for (; data.i < size; data.i++) {
		testRange(&data, -fourPi + step*data.i);
	}
	if (FricasClose(data.fr)) {
		fprintf(stderr, "sinCosOmcTester: failed to close fricas pipes\n");
	}

	typedef struct {
		rangeReport *p;
		int i;
	} slice;

	// Array with an element for each mathematical function, each containing
	// a slice with a rangeReport for each range.
	slice dataByFunction[FuncLimit];
	int fn, ran;
	for (fn = 0; fn < FuncLimit; fn++) {
		for (dataByFunction[fn].i = 0, ran = 0; ran < data.i; ran++) {
			int exists = 0 != 0, point;
			for (point = 0; point < PointsInOneRange; point++) {
				// Skip points without a change.
				if (isNull(data.funcData[ran].a[point][fn])) {
					continue;
				}

				ifscor s = scoresOf(data.funcData[ran].a[point][fn]);
				// Skip points without a relevant change.
				if (s.iscor == 0) {
					continue;
				}

				if (!exists) {
					dataByFunction[fn].i++;
					if (dataByFunction[fn].i == 1) {
						dataByFunction[fn].p = calloc(1, sizeof(rangeReport));
					} else {
						dataByFunction[fn].p = realloc(dataByFunction[fn].p,
								sizeof(rangeReport) * dataByFunction[fn].i);
						memset(&dataByFunction[fn].p[dataByFunction[fn].i-1], 0, sizeof(rangeReport));
					}
				}
				exists = 0 == 0;
				if (0 < s.iscor) {
					updateMicroReport(&dataByFunction[fn].p[dataByFunction[fn].i-1].improvements, s.iscor, s.fscor);
				} else  {
					updateMicroReport(&dataByFunction[fn].p[dataByFunction[fn].i-1].worsenings, s.iscor, s.fscor);
				}
				dataByFunction[fn].p[dataByFunction[fn].i-1].mean1 += s.fscor;
			}
			if(exists) {
				dataByFunction[fn].p[dataByFunction[fn].i-1].limits[0] = data.funcData[ran].limits[0];
				dataByFunction[fn].p[dataByFunction[fn].i-1].limits[1] = data.funcData[ran].limits[1];
				dataByFunction[fn].p[dataByFunction[fn].i-1].improvements.mean2 =
					sqrt(dataByFunction[fn].p[dataByFunction[fn].i-1].improvements.mean2 /
						(mfloat_t)dataByFunction[fn].p[dataByFunction[fn].i-1].improvements.count);
				dataByFunction[fn].p[dataByFunction[fn].i-1].worsenings.mean2 =
					sqrt(dataByFunction[fn].p[dataByFunction[fn].i-1].worsenings.mean2 /
						(mfloat_t)dataByFunction[fn].p[dataByFunction[fn].i-1].worsenings.count);
				dataByFunction[fn].p[dataByFunction[fn].i-1].mean1 /= (mfloat_t)(
					dataByFunction[fn].p[dataByFunction[fn].i-1].improvements.count +
					dataByFunction[fn].p[dataByFunction[fn].i-1].worsenings.count);
			}
		}
	}

	// Print reports for each range of each function where interesting
	// differences were recorded, from dataByFunction.
	printf("\n\nPointsInOneRange: %5d\n\n\n", PointsInOneRange);
	for (fn = 0; fn < FuncLimit; fn++) {
		printf("%3s:\n", funcNames[fn]);
		for (ran = 0; ran < dataByFunction[fn].i; ran++) {
#define fMR "%7d %22ld " FLTFMT " " FLTFMT "\n"
			printf(FLTFMT " " FLTFMT "\n" fMR fMR FLTFMT "\n\n",
				dataByFunction[fn].p[ran].limits[0], dataByFunction[fn].p[ran].limits[1],
				dataByFunction[fn].p[ran].improvements.count, dataByFunction[fn].p[ran].improvements.max, dataByFunction[fn].p[ran].improvements.maxScor, dataByFunction[fn].p[ran].improvements.mean2,
				dataByFunction[fn].p[ran].worsenings.count, dataByFunction[fn].p[ran].worsenings.max, dataByFunction[fn].p[ran].worsenings.maxScor, dataByFunction[fn].p[ran].worsenings.mean2,
				dataByFunction[fn].p[ran].mean1);
		}
		printf("\n\n");
	}
}
