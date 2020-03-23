// Copyright 2020 Neven Sajko <nsajko@gmail.com>. All rights reserved.

#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cfricas.h"

#define nil 0

// TODO: provide access to Fricas's interval computation facilities
// (package Interval)?

ieee754FloatingPointNumber
FricasFloatEval(FloatFricas f, const char *fricasCmd, ieee754FloatingPointNumber x) {
	// Lexes the floating point value from lines like these that
	// Fricas outputs (note the space after the minus sign):
	//
	//    (13)  0.3300000000000000000000000E1
	//    (1)  - 0.3300000000000000000000000E1

	// TODO: Maybe return a custom NaN for each error?
	const ieee754FloatingPointNumber nan = (ieee754FloatingPointNumber)0 / (ieee754FloatingPointNumber)0;

	long n = fprintf(f.in, fricasCmd, x);
	if (n <= 0) {
		return nan;
	}
	n = fflush(f.in);
	if (n != 0) {
		return nan;
	}

	// Skip "[^)]*)  ".
	for (;;) {
		int c = fgetc(f.out);
		if (c < 0) {
			return nan;
		}
		if (c == ')') {
			break;
		}
	}
	for (n = 0; n < 2; n++) {
		int c = fgetc(f.out);
		if (c < 0) {
			return nan;
		}
	}

	char num[60];
	char *ss, *s = fgets(num, sizeof(num), f.out);
	if (s == nil) {
		return nan;
	}

	s = num;
	if (num[0] == '-') {
		// Remove the extra space character after the -.
		s = &num[1];
		s[0] = '-';
	}
	x = strtod(s, &ss);
	if (ss == s) {
		return nan;
	}
	return x;
}

FloatFricas
FricasFloatNew(void) {
	FloatFricas r = {nil};
	int s;
	int in[2], out[2];
	s = pipe(in);
	if (s != 0) {
		return r;
	}
	s = pipe(out);
	if (s != 0) {
		return r;
	}
	posix_spawn_file_actions_t fa;
	s = posix_spawn_file_actions_init(&fa);
	if (s != 0) {
		return r;
	}
	s = posix_spawn_file_actions_addclose(&fa, in[1]);
	if (s != 0) {
		return r;
	}
	s = posix_spawn_file_actions_addclose(&fa, out[0]);
	if (s != 0) {
		return r;
	}
	s = posix_spawn_file_actions_adddup2(&fa, in[0], 0);
	if (s != 0) {
		return r;
	}
	s = posix_spawn_file_actions_adddup2(&fa, out[1], 1);
	if (s != 0) {
		return r;
	}
	static char *const a[] = {"fricas", "-nosman", "-eval", ")set output algebra off", "-eval", ")lib )dir " FricasLibDir,
		"-eval", ")set history off", "-eval", ")set messages prompt none", "-eval", ")set messages type off",
		"-eval", "bits(" FloatFricasBits ")$Float", "-eval", "outputGeneral(21)$Float", "-eval", "outputSpacing(0)$Float",
		"-eval", ")set output algebra on", nil};
	extern char **environ;
	s = posix_spawnp(nil, "fricas", &fa, nil, a, environ);
	if (s != 0) {
		fprintf(stderr, "cfricas: failed to posix_spawnp FriCAS\n");
		return r;
	}
	posix_spawn_file_actions_destroy(&fa);
	close(in[0]);
	close(out[1]);
	FloatFricas rr;
	rr.in = fdopen(in[1], "w");
	if (rr.in == nil) {
		return r;
	}
	rr.out = fdopen(out[0], "r");
	if (rr.out == nil) {
		return r;
	}
	// Discard the redundant lines of Fricas output.
	int i;
	for (i = 0; i != FloatFricasLinesToSkipAtStartup; i++) {
		for (;;) {
			int c = fgetc(rr.out);
			if (c < 0) {
				fprintf(stderr, "cfricas: EOF or I/O error\n");
				return r;
			}
			if (c == '\n') {
				break;
			}
		}
	}
	return rr;
}

int
FricasClose(FloatFricas f) {
	int s = fclose(f.in);
	if (s != 0) {
		return s;
	}
	return fclose(f.out);
}
