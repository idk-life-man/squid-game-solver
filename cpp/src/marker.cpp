#include "marker.h"
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>

// How many players still need a marker?
static int markers_needed(const SquidGameState& s) {
    int have = 0;
    for (bool b : s.has_marker) if (b) have++;
    return s.num_markers - have; // slots remaining
}

static int players_without_marker(const SquidGameState& s) {
    int count = 0;
    for (bool b : s.has_marker) if (!b) count++;
    return count;
}

// Probability of winning at least one pot in N hands remaining
// Approximated: each hand, player wins with prob 1/num_players
static double prob_win_at_least_one(int hands, int num_players) {
    double p_lose_all = std::pow(1.0 - 1.0 / num_players, hands);
    return 1.0 - p_lose_all;
}

ShowDecision analyze_show_decision(
    const SquidGameState& state,
    int player_idx,
    double pot_size)
{
    ShowDecision decision;
    bool already_has_marker = state.has_marker[player_idx];
    int slots_left = markers_needed(state);
    int without = players_without_marker(state);

    // If player already has a marker, showing gives nothing extra
    if (already_has_marker) {
        decision.ev_show = 0.0;
        decision.ev_muck = 0.0;
        decision.should_show = false;
        decision.reasoning = "You already have a marker — no benefit to showing.";
        return decision;
    }

    // If no marker slots left, can't get one anyway
    if (slots_left <= 0) {
        decision.ev_show = 0.0;
        decision.ev_muck = 0.0;
        decision.should_show = false;
        decision.reasoning = "All markers claimed — showing has no value.";
        return decision;
    }

    // EV of marker = penalty_amount * (num_players - num_markers)
    // i.e. each marker holder gets paid by each non-holder
    int non_holders = state.num_players - state.num_markers;
    double marker_ev = (double)state.penalty_amount * non_holders;

    // If you don't show, you need to win another hand (with show) before orbit ends
    double prob_get_later = prob_win_at_least_one(state.hands_remaining - 1, state.num_players);

    // Risk: if you muck now, you might not get another chance
    double ev_muck_future = marker_ev * prob_get_later;

    // Showing now = certain marker (you already won this pot)
    // But: shows reveal hand strength info to opponents
    // We apply a small info_leak penalty (0-20% of pot depending on hand strength)
    double info_leak_cost = pot_size * 0.05; // 5% of pot as default info leak cost

    decision.ev_show = marker_ev - info_leak_cost;
    decision.ev_muck = ev_muck_future;
    decision.should_show = decision.ev_show > decision.ev_muck;

    std::ostringstream oss;
    oss << "Marker EV: " << (int)marker_ev << " chips. ";
    if (decision.should_show) {
        oss << "Show! Securing marker now is worth more than waiting ("
            << (int)(prob_get_later * 100) << "% chance of getting one later).";
    } else {
        oss << "Muck. High chance (" << (int)(prob_get_later * 100)
            << "%) of getting a marker later without giving info.";
    }
    decision.reasoning = oss.str();

    return decision;
}

AggressionAdjustment analyze_aggression(
    const SquidGameState& state,
    int player_idx)
{
    AggressionAdjustment adj;
    bool has_marker = state.has_marker[player_idx];
    int slots_left = markers_needed(state);
    int without = players_without_marker(state);
    int hands_left = state.hands_remaining;

    // Baseline: no adjustment
    adj.bet_size_multiplier = 1.0;
    adj.call_threshold_adjustment = 0.0;

    if (has_marker) {
        // Already safe — play tighter, protect chips
        adj.bet_size_multiplier = 0.85;
        adj.call_threshold_adjustment = -0.05; // Need slightly better odds to call
        adj.reasoning = "You have a marker. Tighten up — protect your stack, avoid marginal spots.";
    } else if (slots_left == 0) {
        // No markers left — you're paying penalty, just minimize chip losses
        adj.bet_size_multiplier = 0.7;
        adj.call_threshold_adjustment = -0.10;
        adj.reasoning = "No markers left — you're paying the penalty. Minimize further chip losses.";
    } else if (slots_left == 1 && without >= 3) {
        // Last marker, high competition — very aggressive to secure it
        adj.bet_size_multiplier = 1.4;
        adj.call_threshold_adjustment = 0.08;
        adj.reasoning = "Last marker available with " + std::to_string(without) +
                        " players competing — increase aggression significantly.";
    } else if ((double)slots_left / without < 0.4) {
        // Fewer slots than players who need them — moderate aggression boost
        adj.bet_size_multiplier = 1.2;
        adj.call_threshold_adjustment = 0.05;
        adj.reasoning = "Marker competition is tight. Play slightly more aggressively to secure one.";
    } else if (hands_left <= 2 && !has_marker) {
        // Running out of time
        adj.bet_size_multiplier = 1.35;
        adj.call_threshold_adjustment = 0.10;
        adj.reasoning = "Orbit almost over and no marker yet — time pressure demands aggression.";
    } else {
        adj.reasoning = "Normal play — marker situation is not critical yet.";
    }

    return adj;
}

std::vector<double> calculate_orbit_ev(const SquidGameState& state) {
    int n = state.num_players;
    int non_holders = n - state.num_markers; // players who will pay penalty at end
    double total_penalty_pool = (double)state.penalty_amount * n; // total chips in reserve
    double penalty_per_holder = total_penalty_pool / state.num_markers; // each holder receives

    std::vector<double> ev(n);
    for (int i = 0; i < n; i++) {
        if (state.has_marker[i]) {
            // Already has marker: receives penalty_per_holder, already paid reserve
            ev[i] = penalty_per_holder - state.penalty_amount;
        } else {
            // Doesn't have marker: estimate probability of getting one
            double p = prob_win_at_least_one(state.hands_remaining, n);
            int slots = markers_needed(state);
            // Adjust probability by competition
            double competition_factor = std::min(1.0, (double)slots / players_without_marker(state));
            p *= competition_factor;

            // EV = p * (receive penalty_per_holder - penalty paid) + (1-p) * (-penalty paid + pay penalty_amount to each holder)
            double ev_win_marker  = penalty_per_holder - state.penalty_amount;
            double ev_lose_marker = -(double)state.penalty_amount * state.num_markers;
            ev[i] = p * ev_win_marker + (1.0 - p) * ev_lose_marker;
        }
    }
    return ev;
}

std::vector<double> no_marker_probability(const SquidGameState& state, int simulations) {
    std::mt19937 rng(std::random_device{}());
    int n = state.num_players;
    std::vector<int> no_marker_count(n, 0);

    for (int sim = 0; sim < simulations; sim++) {
        std::vector<bool> markers = state.has_marker;
        int slots = markers_needed(state);

        for (int hand = 0; hand < state.hands_remaining && slots > 0; hand++) {
            // Simulate a hand winner (uniform random)
            std::uniform_int_distribution<int> dist(0, n - 1);
            int winner = dist(rng);

            // Winner can claim marker if they don't have one and show
            // Simplified: assume they always show if they need a marker
            if (!markers[winner] && slots > 0) {
                markers[winner] = true;
                slots--;
            }
        }

        for (int i = 0; i < n; i++)
            if (!markers[i]) no_marker_count[i]++;
    }

    std::vector<double> probs(n);
    for (int i = 0; i < n; i++)
        probs[i] = (double)no_marker_count[i] / simulations;
    return probs;
}
