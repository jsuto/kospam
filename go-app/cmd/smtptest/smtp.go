package main

import (
    "crypto/tls"
    "fmt"
    "io/ioutil"
    "log"
    "net"
    "net/smtp"
    "os"
    "path/filepath"
    "sync"
)

const (
    maxWorkers = 10
)

var from = "sender@example.com"
var to = []string{"recipient@example.com"}

func main() {
    if len(os.Args) < 2 {
        log.Fatalf("Usage: %s <server:port> <directory>", os.Args[0])
    }
    smtpServer := os.Args[1]
    dir := os.Args[2]

    files := make(chan string, maxWorkers)
    var wg sync.WaitGroup

    count := 0

    for i := 0; i < maxWorkers; i++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            for path := range files {
                log.Printf("Processing: %s", path)
                if err := sendEmail(smtpServer, path); err != nil {
                    log.Printf("Failed to send %s: %v", path, err)
                } else {
                    count++
                }
            }
        }()
    }

    err := filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
        if err != nil {
            return err
        }
        if !info.IsDir() {
            files <- path
        }
        return nil
    })
    close(files)

    if err != nil {
        log.Fatalf("Error walking the path %s: %v", dir, err)
    }

    wg.Wait()

    log.Printf("count: %d", count)
}

func sendEmail(smtpServer string, filePath string) error {
    data, err := ioutil.ReadFile(filePath)
    if err != nil {
        return fmt.Errorf("could not read file: %w", err)
    }

    msg := data

    conn, err := net.Dial("tcp", smtpServer)
    if err != nil {
        return fmt.Errorf("could not connect to SMTP server: %w", err)
    }
    defer conn.Close()

    host, _, err := net.SplitHostPort(smtpServer)

    client, err := smtp.NewClient(conn, host)
    if err != nil {
        return fmt.Errorf("could not create SMTP client: %w", err)
    }

    tlsConfig := &tls.Config{
        InsecureSkipVerify: true,
        ServerName:         host,
    }
    if err := client.StartTLS(tlsConfig); err != nil {
        log.Printf("Skipping TLS: %v", err)
    }

    if err := client.Mail(from); err != nil {
        return fmt.Errorf("could not set sender: %w", err)
    }
    for _, recipient := range to {
        if err := client.Rcpt(recipient); err != nil {
            return fmt.Errorf("could not set recipient: %w", err)
        }
    }

    w, err := client.Data()
    if err != nil {
        return fmt.Errorf("could not send data: %w", err)
    }
    _, err = w.Write(msg)
    if err != nil {
        return fmt.Errorf("could not write message: %w", err)
    }
    w.Close()

    client.Quit()
    return nil
}
