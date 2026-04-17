package bridge

/*
#cgo CFLAGS: -I../cpp/src
#cgo LDFLAGS: -L../cpp/build -lsquid_solver -lstdc++ -lm

#include "bridge.h"
#include <stdlib.h>
*/
import "C"
import (
	"encoding/json"
	"unsafe"
)

// callCPP is a helper that calls a C function returning a char* JSON result,
// unmarshals it, and frees the C memory.
func callCPP(cResult *C.char) (map[string]interface{}, error) {
	defer C.free_result(cResult)
	goStr := C.GoString(cResult)
	var result map[string]interface{}
	if err := json.Unmarshal([]byte(goStr), &result); err != nil {
		return nil, err
	}
	return result, nil
}

func callCPPRaw(cResult *C.char) (string, error) {
	defer C.free_result(cResult)
	return C.GoString(cResult), nil
}

// EvalHand evaluates a poker hand given space-separated card strings
func EvalHand(cards string) (map[string]interface{}, error) {
	cCards := C.CString(cards)
	defer C.free(unsafe.Pointer(cCards))
	return callCPP(C.eval_hand(cCards))
}

// CompareHands compares two poker hands
func CompareHands(hand1, hand2 string) (map[string]interface{}, error) {
	cH1 := C.CString(hand1)
	cH2 := C.CString(hand2)
	defer C.free(unsafe.Pointer(cH1))
	defer C.free(unsafe.Pointer(cH2))
	return callCPP(C.compare_two_hands(cH1, cH2))
}

// CalcEquity runs Monte Carlo equity simulation
func CalcEquity(hole1, hole2, board string, simulations int) (map[string]interface{}, error) {
	cH1 := C.CString(hole1)
	cH2 := C.CString(hole2)
	cBoard := C.CString(board)
	defer C.free(unsafe.Pointer(cH1))
	defer C.free(unsafe.Pointer(cH2))
	defer C.free(unsafe.Pointer(cBoard))
	return callCPP(C.calc_equity(cH1, cH2, cBoard, C.int(simulations)))
}

// AnalyzeShow analyzes the show/muck decision for Squid Game marker
func AnalyzeShow(stateJSON string, playerIdx int, potSize float64) (map[string]interface{}, error) {
	cState := C.CString(stateJSON)
	defer C.free(unsafe.Pointer(cState))
	return callCPP(C.analyze_show(cState, C.int(playerIdx), C.double(potSize)))
}

// AnalyzeAggression returns bet sizing adjustments based on marker situation
func AnalyzeAggression(stateJSON string, playerIdx int) (map[string]interface{}, error) {
	cState := C.CString(stateJSON)
	defer C.free(unsafe.Pointer(cState))
	return callCPP(C.analyze_aggression_c(cState, C.int(playerIdx)))
}

// CalcOrbitEV returns EV array for all players in the orbit
func CalcOrbitEV(stateJSON string) (string, error) {
	cState := C.CString(stateJSON)
	defer C.free(unsafe.Pointer(cState))
	return callCPPRaw(C.calc_orbit_ev(cState))
}

// CalcNoMarkerProb returns probability of each player ending without a marker
func CalcNoMarkerProb(stateJSON string, simulations int) (string, error) {
	cState := C.CString(stateJSON)
	defer C.free(unsafe.Pointer(cState))
	return callCPPRaw(C.calc_no_marker_prob(cState, C.int(simulations)))
}
