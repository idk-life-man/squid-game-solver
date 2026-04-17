#include "bridge.h"
#include "hand_eval.h"
#include "marker.h"
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>

// ---- Minimal JSON helpers ----

static std::vector<std::string> split_tokens(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

static Hand parse_cards(const std::string& s) {
    Hand hand;
    auto tokens = split_tokens(s);
    for (auto& t : tokens) {
        if (!t.empty()) hand.push_back(Card::from_string(t));
    }
    return hand;
}

static char* make_result(const std::string& s) {
    char* buf = (char*)malloc(s.size() + 1);
    memcpy(buf, s.c_str(), s.size() + 1);
    return buf;
}

// Very minimal JSON parser for our specific state format
// Extracts int field from JSON: {"field": 42, ...}
static int json_int(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ')) pos++;
    return std::stoi(json.substr(pos));
}

static double json_double(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return 0.0;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ')) pos++;
    return std::stod(json.substr(pos));
}

// Parse bool array: "has_marker":[true,false,true,...]
static std::vector<bool> json_bool_array(const std::string& json, const std::string& key) {
    std::vector<bool> result;
    std::string search = "\"" + key + "\":[";
    auto pos = json.find(search);
    if (pos == std::string::npos) return result;
    pos += search.size();
    auto end = json.find(']', pos);
    std::string arr = json.substr(pos, end - pos);
    std::istringstream iss(arr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        // trim
        while (!token.empty() && (token[0] == ' ' || token[0] == '\n')) token = token.substr(1);
        result.push_back(token.find("true") != std::string::npos);
    }
    return result;
}

// Parse int array
static std::vector<int> json_int_array(const std::string& json, const std::string& key) {
    std::vector<int> result;
    std::string search = "\"" + key + "\":[";
    auto pos = json.find(search);
    if (pos == std::string::npos) return result;
    pos += search.size();
    auto end = json.find(']', pos);
    std::string arr = json.substr(pos, end - pos);
    std::istringstream iss(arr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try { result.push_back(std::stoi(token)); } catch (...) {}
    }
    return result;
}

static SquidGameState parse_state(const std::string& json) {
    SquidGameState s;
    s.num_players      = json_int(json, "num_players");
    s.num_markers      = json_int(json, "num_markers");
    s.has_marker       = json_bool_array(json, "has_marker");
    s.stacks           = json_int_array(json, "stacks");
    s.penalty_amount   = json_int(json, "penalty");
    s.hands_remaining  = json_int(json, "hands_remaining");

    // Ensure has_marker and stacks are the right size
    while ((int)s.has_marker.size() < s.num_players) s.has_marker.push_back(false);
    while ((int)s.stacks.size() < s.num_players) s.stacks.push_back(1000);
    return s;
}

// ---- C API implementations ----

extern "C" {

const char* eval_hand(const char* cards_str) {
    try {
        Hand h = parse_cards(std::string(cards_str));
        auto r = evaluate_hand(h);
        std::ostringstream oss;
        oss << "{\"category\":\"" << hand_rank_name(r.category)
            << "\",\"score\":" << r.score
            << ",\"description\":\"" << r.description << "\"}";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        std::string err = std::string("{\"error\":\"") + e.what() + "\"}";
        return make_result(err);
    }
}

const char* compare_two_hands(const char* hand1_str, const char* hand2_str) {
    try {
        Hand h1 = parse_cards(std::string(hand1_str));
        Hand h2 = parse_cards(std::string(hand2_str));
        auto r1 = evaluate_hand(h1);
        auto r2 = evaluate_hand(h2);
        int result = (r1 > r2) ? 1 : (r1 < r2) ? -1 : 0;
        std::ostringstream oss;
        oss << "{\"result\":" << result
            << ",\"hand1\":\"" << r1.description
            << "\",\"hand2\":\"" << r2.description << "\"}";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        std::string err = std::string("{\"error\":\"") + e.what() + "\"}";
        return make_result(err);
    }
}

const char* calc_equity(const char* hole1_str, const char* hole2_str,
                          const char* board_str, int simulations) {
    try {
        Hand h1 = parse_cards(std::string(hole1_str));
        Hand h2 = parse_cards(std::string(hole2_str));
        Hand board;
        std::string bs(board_str);
        if (!bs.empty()) board = parse_cards(bs);

        auto eq = calculate_equity(h1, h2, board, simulations > 0 ? simulations : 10000);
        std::ostringstream oss;
        oss << std::fixed;
        oss.precision(4);
        oss << "{\"win\":" << eq.win << ",\"tie\":" << eq.tie << ",\"loss\":" << eq.loss << "}";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        std::string err = std::string("{\"error\":\"") + e.what() + "\"}";
        return make_result(err);
    }
}

const char* analyze_show(const char* state_json, int player_idx, double pot_size) {
    try {
        auto state = parse_state(std::string(state_json));
        auto decision = analyze_show_decision(state, player_idx, pot_size);
        std::ostringstream oss;
        oss << std::fixed; oss.precision(2);
        oss << "{\"ev_show\":" << decision.ev_show
            << ",\"ev_muck\":" << decision.ev_muck
            << ",\"should_show\":" << (decision.should_show ? "true" : "false")
            << ",\"reasoning\":\"" << decision.reasoning << "\"}";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        return make_result(std::string("{\"error\":\"") + e.what() + "\"}");
    }
}

const char* analyze_aggression_c(const char* state_json, int player_idx) {
    try {
        auto state = parse_state(std::string(state_json));
        auto adj = analyze_aggression(state, player_idx);
        std::ostringstream oss;
        oss << std::fixed; oss.precision(4);
        oss << "{\"bet_size_multiplier\":" << adj.bet_size_multiplier
            << ",\"call_threshold_adjustment\":" << adj.call_threshold_adjustment
            << ",\"reasoning\":\"" << adj.reasoning << "\"}";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        return make_result(std::string("{\"error\":\"") + e.what() + "\"}");
    }
}

const char* calc_orbit_ev(const char* state_json) {
    try {
        auto state = parse_state(std::string(state_json));
        auto evs = calculate_orbit_ev(state);
        std::ostringstream oss;
        oss << std::fixed; oss.precision(2);
        oss << "[";
        for (int i = 0; i < (int)evs.size(); i++) {
            if (i > 0) oss << ",";
            oss << evs[i];
        }
        oss << "]";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        return make_result(std::string("{\"error\":\"") + e.what() + "\"}");
    }
}

const char* calc_no_marker_prob(const char* state_json, int simulations) {
    try {
        auto state = parse_state(std::string(state_json));
        auto probs = no_marker_probability(state, simulations > 0 ? simulations : 50000);
        std::ostringstream oss;
        oss << std::fixed; oss.precision(4);
        oss << "[";
        for (int i = 0; i < (int)probs.size(); i++) {
            if (i > 0) oss << ",";
            oss << probs[i];
        }
        oss << "]";
        return make_result(oss.str());
    } catch (const std::exception& e) {
        return make_result(std::string("{\"error\":\"") + e.what() + "\"}");
    }
}

void free_result(char* ptr) {
    free(ptr);
}

} // extern "C"
