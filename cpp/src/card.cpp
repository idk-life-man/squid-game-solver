#include "card.h"
#include <stdexcept>
#include <algorithm>

char rank_char(int rank) {
    const char* ranks = "23456789TJQKA";
    return ranks[rank - 2];
}

char suit_char(int suit) {
    const char* suits = "cdhs";
    return suits[suit];
}

int parse_rank(char c) {
    switch (c) {
        case '2': return 2;  case '3': return 3;  case '4': return 4;
        case '5': return 5;  case '6': return 6;  case '7': return 7;
        case '8': return 8;  case '9': return 9;  case 'T': return 10;
        case 'J': return 11; case 'Q': return 12; case 'K': return 13;
        case 'A': return 14;
        default: throw std::invalid_argument("Invalid rank");
    }
}

int parse_suit(char c) {
    switch (c) {
        case 'c': case 'C': return 0;
        case 'd': case 'D': return 1;
        case 'h': case 'H': return 2;
        case 's': case 'S': return 3;
        default: throw std::invalid_argument("Invalid suit");
    }
}

Card Card::from_string(const std::string& s) {
    if (s.size() != 2) throw std::invalid_argument("Card string must be 2 chars");
    return Card(parse_rank(s[0]), parse_suit(s[1]));
}

std::string Card::to_string() const {
    return std::string(1, rank_char(rank)) + suit_char(suit);
}

std::vector<Card> make_deck() {
    std::vector<Card> deck;
    deck.reserve(52);
    for (int r = 2; r <= 14; r++)
        for (int s = 0; s < 4; s++)
            deck.emplace_back(r, s);
    return deck;
}

std::vector<Card> remove_cards(const std::vector<Card>& deck, const Hand& remove) {
    std::vector<Card> result;
    result.reserve(deck.size());
    for (const auto& c : deck) {
        bool found = false;
        for (const auto& r : remove)
            if (c == r) { found = true; break; }
        if (!found) result.push_back(c);
    }
    return result;
}
