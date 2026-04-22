# 🦑 Squid Poker Solver


Hosted at - https://squid-game-solver-production.up.railway.app/


A **C++ + Go WebSocket** solver for the **Squid Game poker side game** — the "Stand Up" / marker variant where you must win a pot AND show your hand to claim a marker, with the last player(s) without a marker paying a penalty to all holders.

## Architecture

```
Client (Browser)
      │ WebSocket JSON
      ▼
Go WebSocket Server  (gorilla/websocket)
      │ CGO
      ▼
C++ Solver Engine
  ├── Hand Evaluator    (5-7 card evaluation, Monte Carlo equity)
  ├── Marker Logic      (Show/muck EV, aggression adjustments)
  └── C Bridge API      (extern "C" for CGO)
```

## Features

* **Hand Evaluator** — evaluate any 5–7 card hand (High Card → Royal Flush)
* **Equity Calculator** — Monte Carlo win % for any two hands + board
* **Hand Comparison** — head-to-head showdown comparison
* **Show/Muck Advisor** — should you show to claim a marker, or keep your hand hidden?
* **Aggression Advisor** — how the marker situation should change your bet sizing
* **Orbit EV** — expected chip value for each player over the remaining orbit

## Project Structure

```
squid-poker-solver/
├── cpp/
│   ├── src/
│   │   ├── card.h / card.cpp          # Card representation \& deck utilities
│   │   ├── hand\_eval.h / hand\_eval.cpp # Hand evaluation \& equity calc
│   │   ├── marker.h / marker.cpp       # Squid Game marker mechanic solver
│   │   └── bridge.h / bridge.cpp       # C API (extern "C") for CGO
│   └── CMakeLists.txt
├── go/
│   ├── main.go                         # Entry point, HTTP + WebSocket server
│   ├── server/websocket.go             # Message routing
│   ├── bridge/cgo\_bridge.go            # CGO bindings to C++ .so
│   └── models/types.go                 # JSON message structs
└── client/
    └── index.html                      # Browser UI dashboard
```

## Build \& Run

### 1\. Build the C++ shared library

```bash
cd cpp
mkdir build \&\& cd build
cmake .. -DCMAKE\_BUILD\_TYPE=Release
make -j$(nproc)
# Produces: libsquid\_solver.so (Linux) or libsquid\_solver.dylib (macOS)
```

### 2\. Run the Go server

```bash
cd go
go mod tidy
go run main.go
# Server starts at http://localhost:8080
# WebSocket at ws://localhost:8080/ws
```

### 3\. Open the client

Open `client/index.html` in a browser, or navigate to `http://localhost:8080`.

## WebSocket Protocol

All messages follow this envelope:

**Request:**

```json
{ "action": "eval\_hand", "data": { "cards": "Ah Kd Qc Jh Ts" } }
```

**Response:**

```json
{ "action": "eval\_hand", "success": true, "data": { "category": "Royal Flush", ... } }
```

### Actions

|Action|Description|
|-|-|
|`eval\_hand`|Evaluate a 5–7 card hand|
|`compare\_hands`|Compare two hands at showdown|
|`calc\_equity`|Monte Carlo equity for two hole card combos|
|`analyze\_show`|Show vs muck EV for Squid Game marker|
|`analyze\_aggression`|Bet sizing adjustments based on marker situation|
|`orbit\_ev`|EV for each player over the remaining orbit|
|`no\_marker\_prob`|Probability each player ends without a marker|

## Card Format

* Ranks: `2 3 4 5 6 7 8 9 T J Q K A`
* Suits: `c` (clubs) `d` (diamonds) `h` (hearts) `s` (spades)
* Examples: `Ah` = Ace of hearts, `Kd` = King of diamonds, `Ts` = Ten of spades

## Next Steps

* \[ ] CFR solver for show/muck strategy across full orbits
* \[ ] Multi-player equity (3+ way pots)
* \[ ] Range input (e.g. "top 20% of hands")
* \[ ] Hand history import
* \[ ] Bet sizing optimizer

