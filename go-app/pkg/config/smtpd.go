package config

import (
    "bufio"
    "fmt"
    "os"
    "strconv"
    "strings"
)


type SmtpdConfig struct {
    Acl             string
    EnvelopeDir     string
    Hostname        string
    ListenAddr      string
    MaxMessageBytes int64
    MaxRecipients   int
    NumWorkers      int
    PemFile         string
    QueueDir        string
    Username        string
}

func LoadSmtpdConfig(filename string) (*SmtpdConfig, error) {
    file, err := os.Open(filename)
    if err != nil {
        return nil, fmt.Errorf("error opening config file: %v", err)
    }
    defer file.Close()

    // default values if not specified

    config := &SmtpdConfig{
        Acl:             "/etc/kospam/smtp.acl",
        EnvelopeDir:     "/var/kospam/envelope",
        Hostname:        "kospam.local",
        ListenAddr:      "127.0.0.1:10025",
        MaxMessageBytes: 50 * 1024 * 1024,
        MaxRecipients:   128,
        NumWorkers:      3,
        PemFile:         "/etc/kospam/kospam.pem",
        QueueDir:        "/var/kospam/tmp",
        Username:        "kospam",
    }

    scanner := bufio.NewScanner(file)

    for scanner.Scan() {
        line := scanner.Text()
        parts := strings.Split(line, "=")
        if len(parts) != 2 {
            continue
        }
        key := strings.TrimSpace(parts[0])
        value := strings.TrimSpace(parts[1])

        switch key {
        case "envelope_dir":
            config.EnvelopeDir = value
        case "smtp_acl":
            config.Acl = value
        case "hostname":
            config.Hostname = value
        case "listen_addr":
            config.ListenAddr = value
        case "max_message_bytes":
            if n, err := strconv.ParseInt(value, 10, 64); err == nil && n > 0 && n < 100000000 {
                config.MaxMessageBytes = n
            }
        case "max_recipients":
            if n, err := strconv.Atoi(value); err == nil && n > 0 && n < 512 {
                config.MaxRecipients = n
            }
        case "number_of_worker_processes":
            if n, err := strconv.Atoi(value); err == nil && n > 0 && n < 48 {
                config.NumWorkers = n
            }
        case "pemfile":
            config.PemFile = value
        case "queue_dir":
            config.QueueDir = value
        case "username":
            config.Username = value
        }
    }

    if err := scanner.Err(); err != nil {
        return nil, fmt.Errorf("error reading config file: %v", err)
    }

    return config, nil
}
