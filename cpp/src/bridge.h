#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// --- Hand Evaluation ---

// Evaluate a hand given space-separated cards e.g. "Ah Kd Qc Jh Ts"
// Returns JSON string: {"category":"Royal Flush","score":90000000,"description":"Royal Flush"}
const char* eval_hand(const char* cards_str);

// Compare two hands (space-separated each)
// Returns JSON: {"result":1,"hand1":"...","hand2":"..."} (1=hand1 wins, -1=hand2, 0=tie)
const char* compare_two_hands(const char* hand1_str, const char* hand2_str);

// Calculate equity for hole1 vs hole2 given board
// board_str can be empty string for preflop
// Returns JSON: {"win":0.65,"tie":0.02,"loss":0.33}
const char* calc_equity(const char* hole1_str, const char* hole2_str,
                         const char* board_str, int simulations);

// --- Squid Game Marker Logic ---

// Analyze show/muck decision
// state_json: {"num_players":6,"num_markers":5,"has_marker":[true,false,...],"stacks":[...],"penalty":500,"hands_remaining":4}
// player_idx: 0-based index of the deciding player
// pot_size: current pot
// Returns JSON: {"ev_show":450,"ev_muck":280,"should_show":true,"reasoning":"..."}
const char* analyze_show(const char* state_json, int player_idx, double pot_size);

// Analyze aggression adjustment for a player
// Returns JSON: {"bet_size_multiplier":1.2,"call_threshold_adjustment":0.05,"reasoning":"..."}
const char* analyze_aggression_c(const char* state_json, int player_idx);

// Calculate orbit EV for all players
// Returns JSON array: [120.5, -80.2, 45.0, ...]
const char* calc_orbit_ev(const char* state_json);

// Calculate no-marker probability for all players
// Returns JSON array: [0.12, 0.45, 0.08, ...]
const char* calc_no_marker_prob(const char* state_json, int simulations);

// Free a string returned by any of the above functions
void free_result(char* ptr);

#ifdef __cplusplus
}
#endif
