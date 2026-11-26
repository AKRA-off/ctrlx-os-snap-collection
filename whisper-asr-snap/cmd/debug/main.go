package main

import (
	"github.com/spf13/cobra"
)

func NewRootCmd() *cobra.Command {
	cobra.EnableCommandSorting = false

	rootCmd := &cobra.Command{
		Use:           "debug",
		Short:         "Debug client for testing",
		Long:          "Connect to a running backend or to a running Myna Adapter server to transcribe audio.",
		SilenceUsage:  true,
		SilenceErrors: true,
	}
	rootCmd.CompletionOptions.HiddenDefaultCmd = true

	rootCmd.AddCommand(NewUseBackendCmd())
	rootCmd.AddCommand(NewUseAdapterCmd())
	return rootCmd
}

func main() {
	if err := NewRootCmd().Execute(); err != nil {
		panic(err)
	}
}
