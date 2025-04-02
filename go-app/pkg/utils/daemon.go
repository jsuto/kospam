package utils

import (
    "fmt"
    "log"
    "os"
    "os/exec"
    "path/filepath"
    "syscall"
)

func Daemonize() {
    // Get the full path of the current executable
    executable, err := os.Executable()
    if err != nil {
        log.Fatalf("Failed to get executable path: %v", err)
    }
    execPath, err := filepath.Abs(executable)
    if err != nil {
        log.Fatalf("Failed to get absolute path: %v", err)
    }

    // Start the process in the background
    // But use a special environment variable to signal to the child that it's a daemon
    cmd := exec.Command(execPath)
    cmd.Env = append(os.Environ(), "RUNNING_AS_DAEMON=true")
    cmd.Stdout = nil
    cmd.Stderr = nil
    cmd.Stdin = nil
    cmd.SysProcAttr = &syscall.SysProcAttr{
        Setsid: true, // Create a new session
    }

    if err := cmd.Start(); err != nil {
        log.Fatalf("Failed to start daemon: %v", err)
    }

    // Exit the parent process
    fmt.Printf("Daemon started with PID %d\n", cmd.Process.Pid)
    os.Exit(0)
}
