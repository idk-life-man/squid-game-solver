#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Ranks: 2-14 (2=2, 14=Ace)
// Suits: 0=Clubs, 1=Diamonds, 2=Hearts, 3=Spades
struct Card {
    int rank; // 2-14
    int suit; // 0-3

    Card() : rank(0), suit(0) {}
    Card(int r, int s) : rank(r), suit(s) {}

    // Parse from string like "Ah", "Kd", "2c", "Ts"
    static Card from_string(const std::string& s);
    std::string to_string() const;

    // Compact encoding: 0-51
    int encode() const { return (rank - 2) * 4 + suit; }
    static Card decode(int code) { return Card(code / 4 + 2, code % 4); }

    bool operator==(const Card& o) const { return rank == o.rank && suit == o.suit; }
    bool operator<(const Card& o) const { return encode() < o.encode(); }
};

using Hand = std::vector<Card>;

// Deck utilities
std::vector<Card> make_deck();
std::vector<Card> remove_cards(const std::vector<Card>& deck, const Hand& remove);

// Suit/rank character helpers
char rank_char(int rank);
char suit_char(int suit);
int parse_rank(char c);
int parse_suit(char c);
