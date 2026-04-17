package main

import (
	"fmt"
	"log"
	"net/http"
	"squid-poker-solver/server"
)

func main() {
	port := "8080"

	// WebSocket endpoint
	http.HandleFunc("/ws", server.Handler)

	// Health check
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		fmt.Fprint(w, `{"status":"ok","service":"squid-poker-solver"}`)
	})

	// Serve static client files
	http.Handle("/", http.FileServer(http.Dir("../client")))

	log.Printf("🦑 Squid Poker Solver running on http://localhost:%s", port)
	log.Printf("   WebSocket: ws://localhost:%s/ws", port)
	log.Printf("   Health:    http://localhost:%s/health", port)

	if err := http.ListenAndServe(":"+port, nil); err != nil {
		log.Fatal("Server error:", err)
	}
}
