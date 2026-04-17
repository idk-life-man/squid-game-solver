package models

// Incoming message actions
const (
    ActionEvalHand      = "eval_hand"
    ActionCompareHands  = "compare_hands"
    ActionCalcEquity    = "calc_equity"
    ActionAnalyzeShow   = "analyze_show"
    ActionAggression    = "analyze_aggression"
    ActionOrbitEV       = "orbit_ev"
    ActionNoMarkerProb  = "no_marker_prob"
)

// Generic inbound message envelope
type InboundMessage struct {
    Action string      `json:"action"`
    Data   interface{} `json:"data"`
}

// Generic outbound message envelope
type OutboundMessage struct {
    Action  string      `json:"action"`
    Success bool        `json:"success"`
    Data    interface{} `json:"data"`
    Error   string      `json:"error,omitempty"`
}

// --- Request payloads ---

type EvalHandRequest struct {
    Cards string `json:"cards"` // "Ah Kd Qc Jh Ts"
}

type CompareHandsRequest struct {
    Hand1 string `json:"hand1"`
    Hand2 string `json:"hand2"`
}

type EquityRequest struct {
    Hole1       string `json:"hole1"`       // "Ah Kd"
    Hole2       string `json:"hole2"`       // "Qc Jh"
    Board       string `json:"board"`       // "2c 7h Ks" or ""
    Simulations int    `json:"simulations"` // default 10000
}

type SquidGameStateJSON struct {
    NumPlayers     int    `json:"num_players"`
    NumMarkers     int    `json:"num_markers"`
    HasMarker      []bool `json:"has_marker"`
    Stacks         []int  `json:"stacks"`
    Penalty        int    `json:"penalty"`
    HandsRemaining int    `json:"hands_remaining"`
}

type ShowDecisionRequest struct {
    State     SquidGameStateJSON `json:"state"`
    PlayerIdx int                `json:"player_idx"`
    PotSize   float64            `json:"pot_size"`
}

type AggressionRequest struct {
    State     SquidGameStateJSON `json:"state"`
    PlayerIdx int                `json:"player_idx"`
}

type OrbitEVRequest struct {
    State SquidGameStateJSON `json:"state"`
}

type NoMarkerProbRequest struct {
    State       SquidGameStateJSON `json:"state"`
    Simulations int                `json:"simulations"`
}
