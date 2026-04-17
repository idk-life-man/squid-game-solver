#pragma once
#include "hand_eval.h"
#include <vector>
#include <string>

// State of a single Squid Game orbit
struct SquidGameState {
    int num_players;
    int num_markers;          // Total markers available (usually num_players - 1)
    std::vector<bool> has_marker;  // Which players already have a marker
    std::vector<int> stacks;       // Chip stacks
    int penalty_amount;            // Chips held in reserve per player
    int hands_remaining;           // Hands left in the orbit
};

// Decision point: player won a pot — should they show or muck?
struct ShowDecision {
    double ev_show;    // EV of showing (gaining marker)
    double ev_muck;    // EV of mucking (keeping hand private)
    bool should_show;  // Recommended action
    std::string reasoning;
};

// Analyze whether to show cards after winning a pot
ShowDecision analyze_show_decision(
    const SquidGameState& state,
    int player_idx,
    double pot_size
);

// Aggression adjustment: how much should marker pressure change bet sizing?
// Returns multiplier on normal bet size (1.0 = no change)
struct AggressionAdjustment {
    double bet_size_multiplier;
    double call_threshold_adjustment; // Adjust pot odds threshold
    std::string reasoning;
};

AggressionAdjustment analyze_aggression(
    const SquidGameState& state,
    int player_idx
);

// Full orbit EV for each player given current state
std::vector<double> calculate_orbit_ev(const SquidGameState& state);

// Probability that a player ends up without a marker (pays penalty)
std::vector<double> no_marker_probability(
    const SquidGameState& state,
    int simulations = 50000
);
