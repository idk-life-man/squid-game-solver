#include "hand_eval.h"
#include <algorithm>
#include <random>
#include <array>
#include <numeric>

std::string hand_rank_name(HandRank rank) {
    switch (rank) {
        case HandRank::HIGH_CARD:       return "High Card";
        case HandRank::ONE_PAIR:        return "One Pair";
        case HandRank::TWO_PAIR:        return "Two Pair";
        case HandRank::THREE_OF_A_KIND: return "Three of a Kind";
        case HandRank::STRAIGHT:        return "Straight";
        case HandRank::FLUSH:           return "Flush";
        case HandRank::FULL_HOUSE:      return "Full House";
        case HandRank::FOUR_OF_A_KIND:  return "Four of a Kind";
        case HandRank::STRAIGHT_FLUSH:  return "Straight Flush";
        case HandRank::ROYAL_FLUSH:     return "Royal Flush";
        default:                        return "Unknown";
    }
}

// Evaluate exactly 5 cards
static HandResult evaluate_five(const Hand& h) {
    // Sort descending by rank
    Hand cards = h;
    std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.rank > b.rank;
    });

    // Check flush
    bool flush = true;
    for (int i = 1; i < 5; i++)
        if (cards[i].suit != cards[0].suit) { flush = false; break; }

    // Check straight
    bool straight = true;
    for (int i = 1; i < 5; i++)
        if (cards[i].rank != cards[i-1].rank - 1) { straight = false; break; }

    // Wheel straight: A-2-3-4-5
    bool wheel = (cards[0].rank == 14 && cards[1].rank == 5 &&
                  cards[2].rank == 4  && cards[3].rank == 3 && cards[4].rank == 2);
    if (wheel) straight = true;

    // Count rank frequencies
    std::array<int,15> freq = {};
    for (auto& c : cards) freq[c.rank]++;

    // Sort cards by freq desc, then rank desc (for score building)
    std::sort(cards.begin(), cards.end(), [&](const Card& a, const Card& b) {
        if (freq[a.rank] != freq[b.rank]) return freq[a.rank] > freq[b.rank];
        return a.rank > b.rank;
    });

    // Build score: category * 10^10 + kickers
    auto kicker_score = [&]() {
        int s = 0;
        for (int i = 0; i < 5; i++) s = s * 15 + cards[i].rank;
        return s;
    };

    HandResult result;

    if (straight && flush) {
        if (cards[0].rank == 14 && !wheel) {
            result.category = HandRank::ROYAL_FLUSH;
            result.score = (int)HandRank::ROYAL_FLUSH * 10000000 + kicker_score();
            result.description = "Royal Flush";
        } else {
            result.category = HandRank::STRAIGHT_FLUSH;
            result.score = (int)HandRank::STRAIGHT_FLUSH * 10000000 + (wheel ? 5 : cards[0].rank);
            result.description = "Straight Flush, " + std::string(1, rank_char(wheel ? 5 : cards[0].rank)) + " high";
        }
    } else if (freq[cards[0].rank] == 4) {
        result.category = HandRank::FOUR_OF_A_KIND;
        result.score = (int)HandRank::FOUR_OF_A_KIND * 10000000 + kicker_score();
        result.description = "Four of a Kind, " + std::string(1, rank_char(cards[0].rank)) + "s";
    } else if (freq[cards[0].rank] == 3 && freq[cards[3].rank] == 2) {
        result.category = HandRank::FULL_HOUSE;
        result.score = (int)HandRank::FULL_HOUSE * 10000000 + kicker_score();
        result.description = "Full House, " + std::string(1, rank_char(cards[0].rank)) + "s full of " + std::string(1, rank_char(cards[3].rank)) + "s";
    } else if (flush) {
        result.category = HandRank::FLUSH;
        result.score = (int)HandRank::FLUSH * 10000000 + kicker_score();
        result.description = "Flush, " + std::string(1, rank_char(cards[0].rank)) + " high";
    } else if (straight) {
        int high = wheel ? 5 : cards[0].rank;
        result.category = HandRank::STRAIGHT;
        result.score = (int)HandRank::STRAIGHT * 10000000 + high;
        result.description = "Straight, " + std::string(1, rank_char(high)) + " high";
    } else if (freq[cards[0].rank] == 3) {
        result.category = HandRank::THREE_OF_A_KIND;
        result.score = (int)HandRank::THREE_OF_A_KIND * 10000000 + kicker_score();
        result.description = "Three of a Kind, " + std::string(1, rank_char(cards[0].rank)) + "s";
    } else if (freq[cards[0].rank] == 2 && freq[cards[2].rank] == 2) {
        result.category = HandRank::TWO_PAIR;
        result.score = (int)HandRank::TWO_PAIR * 10000000 + kicker_score();
        result.description = "Two Pair, " + std::string(1, rank_char(cards[0].rank)) + "s and " + std::string(1, rank_char(cards[2].rank)) + "s";
    } else if (freq[cards[0].rank] == 2) {
        result.category = HandRank::ONE_PAIR;
        result.score = (int)HandRank::ONE_PAIR * 10000000 + kicker_score();
        result.description = "One Pair, " + std::string(1, rank_char(cards[0].rank)) + "s";
    } else {
        result.category = HandRank::HIGH_CARD;
        result.score = (int)HandRank::HIGH_CARD * 10000000 + kicker_score();
        result.description = "High Card, " + std::string(1, rank_char(cards[0].rank));
    }

    return result;
}

HandResult evaluate_hand(const Hand& cards) {
    if (cards.size() == 5) return evaluate_five(cards);

    HandResult best;
    best.score = -1;
    int n = cards.size();

    // Try all C(n,5) combinations
    std::vector<int> idx(n);
    std::iota(idx.begin(), idx.end(), 0);

    std::vector<bool> selector(n, false);
    std::fill(selector.begin(), selector.begin() + 5, true);
    std::sort(selector.rbegin(), selector.rend());

    do {
        Hand five;
        for (int i = 0; i < n; i++)
            if (selector[i]) five.push_back(cards[i]);
        auto result = evaluate_five(five);
        if (result.score > best.score) best = result;
    } while (std::prev_permutation(selector.begin(), selector.end()));

    return best;
}

HandResult evaluate_hand(const Hand& hole, const Hand& board) {
    Hand combined = hole;
    combined.insert(combined.end(), board.begin(), board.end());
    return evaluate_hand(combined);
}

int compare_hands(const Hand& hand1, const Hand& hand2) {
    auto r1 = evaluate_hand(hand1);
    auto r2 = evaluate_hand(hand2);
    if (r1 > r2) return 1;
    if (r1 < r2) return -1;
    return 0;
}

EquityResult calculate_equity(
    const Hand& hole1, const Hand& hole2,
    const Hand& board, int simulations)
{
    std::mt19937 rng(std::random_device{}());

    // Build available deck
    Hand known = hole1;
    known.insert(known.end(), hole2.begin(), hole2.end());
    known.insert(known.end(), board.begin(), board.end());
    auto deck = remove_cards(make_deck(), known);

    int cards_needed = 5 - (int)board.size();
    int wins = 0, ties = 0, losses = 0;

    for (int i = 0; i < simulations; i++) {
        // Shuffle deck and take first cards_needed
        std::shuffle(deck.begin(), deck.end(), rng);
        Hand run_board = board;
        for (int j = 0; j < cards_needed; j++)
            run_board.push_back(deck[j]);

        auto r1 = evaluate_hand(hole1, run_board);
        auto r2 = evaluate_hand(hole2, run_board);

        if (r1 > r2) wins++;
        else if (r1 == r2) ties++;
        else losses++;
    }

    return {
        (double)wins  / simulations,
        (double)ties  / simulations,
        (double)losses / simulations
    };
}
