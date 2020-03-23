package fricas

import (
	"bufio"
	"fmt"
	"io"
	"math"
	"os"
	"os/exec"
	"strconv"
)

const (
	// Change this to your directory containing the necessary
	// FriCAS package(s).
	FricasLibDir = "/home/nsajko/src/github.com/nsajko/numericcompfricas/fricas"

	// How many lines of Fricas's output to discard when it is
	// started. (Start-up messages, etc.)
	FloatFricasLinesToSkipAtStartup = 17

	// Bits of precision for floating point representation.
	FloatFricasBits = "32768"
)

type FloatFricas struct {
	in io.WriteCloser
	out io.ReadCloser
	inBuf *bufio.Writer
	outBuf *bufio.Reader
}

// TODO: provide access to Fricas's interval computation facilities
// (package Interval)?

func FloatEval(f *FloatFricas, fricasCmd string, x float64) float64 {
	// Lexes the floating point value from lines like these that
	// Fricas outputs (note the space after the minus sign):
	//
	//    (13)  0.3300000000000000000000000E1
	//    (1)  - 0.3300000000000000000000000E1

	go func() {
		if _, err := fmt.Fprintf(f.inBuf, fricasCmd, x); err != nil {
			fmt.Fprintf(os.Stderr, "goFricas: fmt.Fprintf(f.inBuf, fricasCmd, x): %v\n", err)
		}
		if err := f.inBuf.Flush(); err != nil {
			fmt.Fprintf(os.Stderr, "goFricas: f.inBuf.Flush: %v\n", err)
		}
	}()

	// TODO: Maybe return a custom NaN for each error?

	// Skip "[^)]*)  ".
	if _, err := f.outBuf.ReadSlice(')'); err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: f.outBuf.ReadSlice(')'): %v\n", err)
		return math.NaN()
	}
	if _, err := f.outBuf.ReadByte(); err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: f.outBuf.ReadByte 0: %v\n", err)
		return math.NaN()
	}
	if _, err := f.outBuf.ReadByte(); err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: f.outBuf.ReadByte 1: %v\n", err)
		return math.NaN()
	}

	num, err := f.outBuf.ReadSlice('\n')
	if err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: f.outBuf.ReadSlice('\\n'): %v\n", err)
		return math.NaN()
	}

	if num[0] == '-' {
		// Remove the extra space character after the -.
		num = num[1:]
		num[0] = '-'
	}
	n, err := strconv.ParseFloat(string(num[:len(num)-1]), 64)
	if err != nil && err.(*strconv.NumError).Err != strconv.ErrRange {
		fmt.Fprintf(os.Stderr, "goFricas: strconv.ParseFloat(string(num[:len(num)-1]), 64): %v\n", err)
		return math.NaN()
	}
	return n
}

func NewFloatFricas(f *FloatFricas) {
	cmd := exec.Command("fricas", "-nosman", "-eval", ")set output algebra off", "-eval", ")lib )dir " + FricasLibDir,
		"-eval", ")set history off", "-eval", ")set messages prompt none", "-eval", ")set messages type off",
		"-eval", "bits(" + FloatFricasBits + ")$Float", "-eval", "outputGeneral(25)$Float", "-eval", "outputSpacing(0)$Float",
		"-eval", ")set output algebra on")
	var err error
	f.in, err = cmd.StdinPipe()
	if err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: cmd.StdinPipe: %v\n", err)
	}
	f.out, err = cmd.StdoutPipe()
	if err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: cmd.StdoutPipe: %v\n", err)
	}
	if err = cmd.Start(); err != nil {
		fmt.Fprintf(os.Stderr, "goFricas: cmd.Start: %v\n", err)
	}
	f.inBuf, f.outBuf = bufio.NewWriter(f.in), bufio.NewReader(f.out)
	for i := 0; i != FloatFricasLinesToSkipAtStartup; i++ {
		if _, err = f.outBuf.ReadSlice('\n'); err != nil {
			fmt.Fprintf(os.Stderr, "goFricas: f.outBuf.ReadSlice('\\n'): %v\n", err)
		}
	}
}

func (f *FloatFricas) Close() error {
	if err := f.in.Close(); err != nil {
		return err
	}
	return f.out.Close()
}
