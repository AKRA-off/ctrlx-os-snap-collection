package main

import (
	"context"
	"errors"
	"fmt"
	"os"
	"strings"
	"time"

	"myna-adapter/backends/whisperlive"

	"github.com/spf13/cobra"
)

type useBackendCommand struct {
	host       string
	port       int
	audioPath  string
	model      string
	lang       string
	timeoutSec int
}

func NewUseBackendCmd() *cobra.Command {
	var cmd useBackendCommand

	cobraCmd := &cobra.Command{
		Use:               "use-backend",
		Short:             "Use Whisper Live backend to transcribe a local audio file",
		Long:              "Connect to a Whisper Live websocket backend and transcribe a local audio file through ffmpeg streaming. Requires ffmpeg.",
		Args:              cobra.NoArgs,
		ValidArgsFunction: cobra.NoFileCompletions,
		RunE:              cmd.run,
	}

	cobraCmd.Flags().StringVar(&cmd.host, "host", "127.0.0.1", "backend host")
	cobraCmd.Flags().IntVar(&cmd.port, "port", 9090, "backend port")
	cobraCmd.Flags().StringVar(&cmd.audioPath, "audio", "", "path to local audio file to transcribe")
	cobraCmd.Flags().StringVar(&cmd.model, "model", "small", "backend model name")
	cobraCmd.Flags().StringVar(&cmd.lang, "lang", "en", "audio language code")
	cobraCmd.Flags().IntVar(&cmd.timeoutSec, "timeout-sec", 180, "command timeout in seconds")
	_ = cobraCmd.MarkFlagRequired("audio")

	return cobraCmd
}

func (cmd *useBackendCommand) run(cobraCmd *cobra.Command, _ []string) error {
	if _, err := os.Stat(cmd.audioPath); err != nil {
		return fmt.Errorf("audio file not accessible: %w", err)
	}
	if strings.TrimSpace(cmd.host) == "" || cmd.port <= 0 {
		return errors.New("host and port are required")
	}

	timeout := time.Duration(cmd.timeoutSec) * time.Second
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	cfg := whisperlive.Config{
		Host:             cmd.host,
		Port:             cmd.port,
		Lang:             cmd.lang,
		Model:            cmd.model,
		LogTranscription: true,
	}

	fmt.Fprintf(cobraCmd.OutOrStdout(), "connecting to whisper live at %s:%d\n", cmd.host, cmd.port)
	client, err := whisperlive.Dial(ctx, cfg)
	if err != nil {
		return fmt.Errorf("connecting to whisper live: %w", err)
	}
	defer client.Close()

	if err := client.WaitReady(ctx); err != nil {
		return fmt.Errorf("waiting for server ready: %w", err)
	}

	fmt.Fprintf(cobraCmd.OutOrStdout(), "streaming audio file: %s\n", cmd.audioPath)
	if err := client.TranscribeFile(ctx, cmd.audioPath); err != nil {
		return fmt.Errorf("processing audio stream: %w", err)
	}

	finalText := client.FinalTranscript()
	if finalText == "" {
		fmt.Fprintln(cobraCmd.OutOrStdout(), "transcription completed but no text was returned")
	} else {
		fmt.Fprintf(cobraCmd.OutOrStdout(), "final transcript:\n%s\n", finalText)
	}

	return nil
}
