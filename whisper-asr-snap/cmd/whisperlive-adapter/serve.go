package main

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"slices"
	"strings"
	"syscall"
	"time"

	"myna-adapter/backends"
	"myna-adapter/backends/whisperlive"
	"myna-adapter/openai/server"

	"github.com/spf13/cobra"
)

type serveCommand struct {
	host       string
	port       int
	unixSocket string

	backendHost      string
	backendPort      int
	defaultModel     string
	defaultLang      string
	allowedModels    []string
	allowedLanguages []string
}

func NewServeCmd() *cobra.Command {
	var cmd serveCommand

	cobraCmd := &cobra.Command{
		Use:               "serve",
		Short:             "Start the websocket server",
		Long:              "Start an OpenAI transcription API websocket server that adapts each connection to a Whisper Live transcription backend.",
		Args:              cobra.NoArgs,
		ValidArgsFunction: cobra.NoFileCompletions,
		RunE:              cmd.run,
	}

	cobraCmd.Flags().StringVar(&cmd.host, "host", "127.0.0.1", "host interface to bind")
	cobraCmd.Flags().IntVar(&cmd.port, "port", 0, "port to listen on; TCP listener is only enabled when this is set to a value > 0")
	cobraCmd.Flags().StringVar(&cmd.unixSocket, "unix-socket", "", "path to a Unix domain socket to bind to. Can coexist with TCP if --port is set")

	cobraCmd.Flags().StringVar(&cmd.backendHost, "backend-host", "127.0.0.1", "The host the backend is running on")
	cobraCmd.Flags().IntVar(&cmd.backendPort, "backend-port", 9090, "The port the backend is running on")

	cobraCmd.Flags().StringVar(&cmd.defaultModel, "model", "small", "Default model name")
	cobraCmd.Flags().StringVar(&cmd.defaultLang, "language", "en", "Default language code")
	cobraCmd.Flags().StringSliceVar(&cmd.allowedModels, "allowed-models", []string{"small"}, "Allowed model names")
	cobraCmd.Flags().StringSliceVar(&cmd.allowedLanguages, "allowed-languages", []string{"en"}, "Allowed language codes")

	return cobraCmd
}

func (cmd *serveCommand) validate() error {
	if cmd.port <= 0 && cmd.unixSocket == "" {
		return fmt.Errorf("at least one of --port or --unix-socket must be specified")
	}

	// Validate that default model is in allowed models
	if !slices.Contains(cmd.allowedModels, cmd.defaultModel) {
		return fmt.Errorf("default-model %q is not in allowed-models: %v", cmd.defaultModel, cmd.allowedModels)
	}

	// Validate that default language is in allowed languages
	if !slices.Contains(cmd.allowedLanguages, cmd.defaultLang) {
		return fmt.Errorf("default-lang %q is not in allowed-languages: %v", cmd.defaultLang, cmd.allowedLanguages)
	}

	return nil
}

func (cmd *serveCommand) run(cobraCmd *cobra.Command, _ []string) error {
	if err := cmd.validate(); err != nil {
		return err
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	srv := server.NewWebSocketServer(cmd.host, cmd.port, cmd.unixSocket)
	srv.SetAllowedModels(cmd.allowedModels)
	srv.SetBackend(
		backends.SessionConfig{
			Model: cmd.defaultModel,
			Lang:  cmd.defaultLang,
		},
		func(ctx context.Context, cbs backends.BackendCallbacks) (backends.Backend, error) {
			return whisperlive.Dial(ctx, whisperlive.Config{
				Host:             cmd.backendHost,
				Port:             cmd.backendPort,
				Model:            cmd.defaultModel,
				Lang:             cmd.defaultLang,
				Callbacks:        cbs,
				AllowedModels:    cmd.allowedModels,
				AllowedLanguages: cmd.allowedLanguages,
			})
		},
	)
	fmt.Fprintf(cobraCmd.OutOrStdout(), "starting websocket server on %s\n", strings.Join(srv.Addresses(), ", "))

	startErr := make(chan error, 1)
	go func() { startErr <- srv.Start() }()

	select {
	case err := <-startErr:
		return err
	case <-ctx.Done():
		stop() // restore default SIGINT behaviour so a second Ctrl+C kills immediately
		fmt.Fprintf(cobraCmd.OutOrStdout(), "\nshutting down...\n")
		shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		_ = srv.Stop(shutdownCtx)
		return <-startErr
	}
}
