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
    MaxLineLength   int
    MaxMessageBytes int64
    MaxRecipients   int
    NumWorkers      int
    PemFile         string
    QueueDir        string
    SmtpAddr        string
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
        Hostname:        "kospam.local",
        ListenAddr:      "127.0.0.1:10025",
        MaxLineLength:   2000,
        MaxMessageBytes: 50 * 1024 * 1024,
        MaxRecipients:   128,
        NumWorkers:      3,
        PemFile:         "/etc/kospam/kospam.pem",
        QueueDir:        "/var/kospam/tmp",
        SmtpAddr:        "127.0.0.1:10026",
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
        case "smtp_acl":
            config.Acl = value
        case "hostname":
            config.Hostname = value
        case "listen_addr":
            config.ListenAddr = value
        case "max_line_len":
            if n, err := strconv.Atoi(value); err == nil && n > 200 && n < 99999 {
                config.MaxLineLength = n
            }
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
        case "smtp_addr":
            config.SmtpAddr = value
        case "username":
            config.Username = value
        }
    }

    if err := scanner.Err(); err != nil {
        return nil, fmt.Errorf("error reading config file: %v", err)
    }

    return config, nil
}
