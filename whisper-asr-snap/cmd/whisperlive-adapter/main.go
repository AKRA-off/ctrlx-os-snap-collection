package main

import (
	"github.com/spf13/cobra"
)

func NewRootCmd() *cobra.Command {
	cobra.EnableCommandSorting = false

	rootCmd := &cobra.Command{
		Use:           "whisperlive-adapter",
		Short:         "An OpenAI transcription adapter leveraging Whisper Live as a backend.",
		Long:          "Launch and manage an OpenAI transcription API server that leverages Whisper Live as a backend.",
		SilenceUsage:  true,
		SilenceErrors: true,
	}
	rootCmd.CompletionOptions.HiddenDefaultCmd = true

	rootCmd.AddCommand(NewServeCmd())
	return rootCmd
}

func main() {
	if err := NewRootCmd().Execute(); err != nil {
		panic(err)
	}
}
