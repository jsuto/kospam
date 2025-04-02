package utils

import (
    "fmt"
    "log"
    "log/syslog"
    "os"
)


func RedirectSyslog(syslogId string) {
        // Connect to syslog
        syslogWriter, err := syslog.New(syslog.LOG_MAIL, syslogId)
        if err != nil {
            log.Fatalf("Failed to connect to syslog: %v", err)
        }

        // Redirect standard log package to syslog
        log.SetOutput(syslogWriter)

        // Remove the timestamp flag (and other flags) when redirecting to syslog
        // This will prevent the duplicate timestamps
        log.SetFlags(0) // Setting flags to 0 removes all prefixes including timestamps

        // Redirect fmt output by replacing stdout and stderr
        // Note: This won't capture direct writes to file descriptors 1 and 2
        r, w, err := os.Pipe()
        if err != nil {
            syslogWriter.Err(fmt.Sprintf("Failed to create pipe: %v", err))
            return
        }

        os.Stdout = w
        os.Stderr = w

        // Start a goroutine to forward pipe output to syslog
        go func() {
            buffer := make([]byte, 1024)
            for {
                n, err := r.Read(buffer)
                if err != nil {
                    syslogWriter.Err(fmt.Sprintf("Error reading from pipe: %v", err))
                    return
                }
                if n > 0 {
                    syslogWriter.Notice(string(buffer[:n]))
                }
            }
        }()

        // Log that we've started as a daemon
        log.Println("Started in daemon mode, redirected outputs to syslog")
}
