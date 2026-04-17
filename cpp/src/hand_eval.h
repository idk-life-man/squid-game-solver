#pragma once
#include "card.h"
#include <string>

// Hand rank categories (higher = better)
enum class HandRank : int {
    HIGH_CARD       = 0,
    ONE_PAIR        = 1,
    TWO_PAIR        = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT        = 4,
    FLUSH           = 5,
    FULL_HOUSE      = 6,
    FOUR_OF_A_KIND  = 7,
    STRAIGHT_FLUSH  = 8,
    ROYAL_FLUSH     = 9
};

struct HandResult {
    HandRank category;
    int score;          // Full comparable score (higher = better)
    std::string description;

    bool operator>(const HandResult& o) const { return score > o.score; }
    bool operator<(const HandResult& o) const { return score < o.score; }
    bool operator==(const HandResult& o) const { return score == o.score; }
};

// Evaluate best 5-card hand from 5, 6, or 7 cards
HandResult evaluate_hand(const Hand& cards);

// Evaluate best hand from hole cards + board
HandResult evaluate_hand(const Hand& hole, const Hand& board);

// Compare two hands: +1 = hand1 wins, -1 = hand2 wins, 0 = tie
int compare_hands(const Hand& hand1, const Hand& hand2);

// Human-readable hand category name
std::string hand_rank_name(HandRank rank);

// Monte Carlo equity: returns win probability for hand1 vs hand2 given board
// Runs `simulations` random runouts
struct EquityResult {
    double win;   // hand1 win %
    double tie;   // tie %
    double loss;  // hand2 win %
};

EquityResult calculate_equity(
    const Hand& hole1,
    const Hand& hole2,
    const Hand& board,
    int simulations = 10000
);
