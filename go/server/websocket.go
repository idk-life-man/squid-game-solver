package server

import (
	"encoding/json"
	"log"
	"net/http"
	"squid-poker-solver/bridge"
	"squid-poker-solver/models"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true }, // Allow all origins for dev
}

// Handler upgrades HTTP to WebSocket and handles messages
func Handler(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("WebSocket upgrade error: %v", err)
		return
	}
	defer conn.Close()
	log.Printf("Client connected: %s", r.RemoteAddr)

	for {
		_, msgBytes, err := conn.ReadMessage()
		if err != nil {
			log.Printf("Client disconnected: %v", err)
			break
		}

		response := handleMessage(msgBytes)
		if err := conn.WriteJSON(response); err != nil {
			log.Printf("Write error: %v", err)
			break
		}
	}
}

func handleMessage(data []byte) models.OutboundMessage {
	// First pass: get the action
	var envelope struct {
		Action string          `json:"action"`
		Data   json.RawMessage `json:"data"`
	}
	if err := json.Unmarshal(data, &envelope); err != nil {
		return errResponse("", "Invalid JSON: "+err.Error())
	}

	switch envelope.Action {

	case models.ActionEvalHand:
		var req models.EvalHandRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		result, err := bridge.EvalHand(req.Cards)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, result)

	case models.ActionCompareHands:
		var req models.CompareHandsRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		result, err := bridge.CompareHands(req.Hand1, req.Hand2)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, result)

	case models.ActionCalcEquity:
		var req models.EquityRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		sims := req.Simulations
		if sims == 0 {
			sims = 10000
		}
		result, err := bridge.CalcEquity(req.Hole1, req.Hole2, req.Board, sims)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, result)

	case models.ActionAnalyzeShow:
		var req models.ShowDecisionRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		stateJSON, _ := json.Marshal(req.State)
		result, err := bridge.AnalyzeShow(string(stateJSON), req.PlayerIdx, req.PotSize)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, result)

	case models.ActionAggression:
		var req models.AggressionRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		stateJSON, _ := json.Marshal(req.State)
		result, err := bridge.AnalyzeAggression(string(stateJSON), req.PlayerIdx)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, result)

	case models.ActionOrbitEV:
		var req models.OrbitEVRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		stateJSON, _ := json.Marshal(req.State)
		raw, err := bridge.CalcOrbitEV(string(stateJSON))
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, json.RawMessage(raw))

	case models.ActionNoMarkerProb:
		var req models.NoMarkerProbRequest
		if err := json.Unmarshal(envelope.Data, &req); err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		stateJSON, _ := json.Marshal(req.State)
		sims := req.Simulations
		if sims == 0 {
			sims = 50000
		}
		raw, err := bridge.CalcNoMarkerProb(string(stateJSON), sims)
		if err != nil {
			return errResponse(envelope.Action, err.Error())
		}
		return okResponse(envelope.Action, json.RawMessage(raw))

	default:
		return errResponse(envelope.Action, "Unknown action: "+envelope.Action)
	}
}

func okResponse(action string, data interface{}) models.OutboundMessage {
	return models.OutboundMessage{Action: action, Success: true, Data: data}
}

func errResponse(action, msg string) models.OutboundMessage {
	return models.OutboundMessage{Action: action, Success: false, Error: msg}
}
