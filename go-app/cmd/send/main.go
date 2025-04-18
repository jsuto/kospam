package main

import (
    "bufio"
    "crypto/tls"
    "flag"
    "fmt"
    "log"
    "net"
    "os"
    "path/filepath"
    "regexp"
    "strings"
    "sync"
    "time"

    "kospam/smtpd/pkg/cache"
    "kospam/smtpd/pkg/config"
    "kospam/smtpd/pkg/utils"
    "kospam/smtpd/pkg/version"
)

const (
    cacheCounterKey = "kospam"
    queueDir = "/var/kospam/send"
    errorDir = "/var/kospam/send-error"
    maxConcurrency = 10
    scanInterval = 5 * time.Second
    syslogId = "kospam/send"
)

var (
    configfile = flag.String("config", "/etc/kospam/kospam.conf", "config file to use")
    showVersion = flag.Bool("version", false, "show version number, then exit")
    daemon = flag.Bool("daemon", false, "run in daemon mode")

    dotStart = regexp.MustCompile(`(?m)^(\.)`)
)

type Email struct {
    QueueId  string
    Sender   string
    Rcpt     []string
    FilePath string
    Content  string
}

func parseEmailFile(filename string) (string, []string, string, error) {
    data, err := os.ReadFile(filename)
    if err != nil {
        return "", nil, "", err
    }

    content := string(data)
    lines := strings.SplitN(content, "\r\n", 4)
    if len(lines) < 4 {
        return "", nil, "", fmt.Errorf("invalid file format")
    }

    sender := ""

    if strings.HasPrefix(lines[0], "Kospam-Envelope-From: ") {
        sender = strings.TrimSpace(strings.TrimPrefix(lines[0], "Kospam-Envelope-From: "))
    }

    rcpt := []string{}

    if strings.HasPrefix(lines[1], "Kospam-Envelope-Recipient: ") {
        rcpt = strings.Split(strings.TrimSpace(strings.TrimPrefix(lines[1], "Kospam-Envelope-Recipient: ")), ",")
    }

    // Don't send the 3rd line, the XFORWARD info

    // Apply the dot stuffing
    remainingContent := dotStart.ReplaceAllString(lines[3], "..")

    if sender == "" {
        return "", nil, "", fmt.Errorf("Missing Kospam-Envelope-From: header line")
    }

    if rcpt == nil {
        return "", nil, "", fmt.Errorf("Missing Kospam-Envelope-Recipient: header line")
    }

    return sender, rcpt, remainingContent, nil
}

func sendEmail(config *config.SmtpdConfig, email Email) bool {
    dialer := net.Dialer{Timeout: 15 * time.Second}
    conn, err := dialer.Dial("tcp", config.SmtpAddr)
    if err != nil {
        fmt.Println("Failed to connect:", err)
        return false
    }
    defer conn.Close()

    reader := bufio.NewReader(conn)
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ := reader.ReadString('\n')
    //fmt.Print(resp)

    fmt.Fprintf(conn, "EHLO %s\r\n", config.Hostname)
    var supportsStartTLS bool
    for {
        conn.SetReadDeadline(time.Now().Add(15 * time.Second))
        resp, _ := reader.ReadString('\n')
        //fmt.Print(resp)
        if strings.HasPrefix(resp, "250-STARTTLS") {
            supportsStartTLS = true
        }
        if !strings.HasPrefix(resp, "250-") {
            break
        }
    }

    if supportsStartTLS {
        fmt.Fprintf(conn, "STARTTLS\r\n")
        conn.SetReadDeadline(time.Now().Add(15 * time.Second))
        resp, _ = reader.ReadString('\n')
        //fmt.Print(resp)
        if !strings.HasPrefix(resp, "220") {
            fmt.Println("STARTTLS not supported")
            return false
        }

        tlsConn := tls.Client(conn, &tls.Config{InsecureSkipVerify: true})
        if err := tlsConn.Handshake(); err != nil {
            fmt.Println("TLS handshake failed:", err)
            return false
        }
        conn = tlsConn
        reader = bufio.NewReader(conn)

        fmt.Fprintf(conn, "EHLO %s\r\n", config.Hostname)
        for {
            conn.SetReadDeadline(time.Now().Add(15 * time.Second))
            resp, _ := reader.ReadString('\n')
            //fmt.Print(resp)
            if !strings.HasPrefix(resp, "250-") {
                break
            }
        }
    }

    fmt.Fprintf(conn, "MAIL FROM: <%s>\r\n", email.Sender)
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    // TODO: pipeline support?

    for _, rcpt := range email.Rcpt {
        fmt.Fprintf(conn, "RCPT TO: <%s>\r\n", rcpt)
        conn.SetReadDeadline(time.Now().Add(15 * time.Second))
        resp, _ = reader.ReadString('\n')
        //fmt.Print(resp)
    }

    fmt.Fprintf(conn, "DATA\r\n")
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    if !strings.HasSuffix(email.Content, "\r\n") {
        fmt.Fprintf(conn, "%s\r\n", email.Content)
    } else {
        fmt.Fprint(conn, email.Content)
    }
    fmt.Fprint(conn, ".\r\n")

    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    fmt.Printf("%s: relay=%s, status=%s", email.QueueId, config.SmtpAddr, resp)

    if !strings.HasPrefix(resp, "250") {
        return false
    }

    fmt.Fprintf(conn, "QUIT\r\n")
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    return true
}

func processQueue(config *config.SmtpdConfig) {
    for {
        files, err := os.ReadDir(queueDir)
        if err != nil {
            fmt.Println("Failed to read queue directory:", err)
            continue
        }

        var wg sync.WaitGroup
        sem := make(chan struct{}, maxConcurrency)

        for _, file := range files {
            if !file.IsDir() {
                path := filepath.Join(queueDir, file.Name())

                sender, rcpt, content, err := parseEmailFile(path)
                if err != nil {
                    fmt.Println("Error:", err)
                    destPath := filepath.Join(errorDir, filepath.Base(path))
                    os.Rename(path, destPath)
                    continue
                }

                email := Email{FilePath: path, QueueId: file.Name(), Sender: sender, Rcpt: rcpt, Content: content}

                wg.Add(1)
                sem <- struct{}{}
                go func(email Email) {
                    defer wg.Done()
                    if sendEmail(config, email) {
                        os.Remove(email.FilePath)
                        if err := cache.UpdateQueueCounter(cacheCounterKey, -1); err != nil {
                            log.Printf("Error updating incoming queue counter: %v", err)
                        }

                    } else {
                        destPath := filepath.Join(errorDir, filepath.Base(email.FilePath))
                        os.Rename(email.FilePath, destPath)
                    }
                    <-sem
                }(email)
            }
        }

        wg.Wait()
        time.Sleep(scanInterval)
    }
}

func init() {
    isDaemon := *daemon || os.Getenv("RUNNING_AS_DAEMON") == "true"

    // If running as daemon, redirect logs to syslog
    if isDaemon {
        utils.RedirectSyslog(syslogId)
    }
}

func main() {
    flag.Parse()

    if *showVersion {
       fmt.Println("version:", version.VERSION)
       return
    }

    // Load configuration
    config, err := config.LoadSmtpdConfig(*configfile)
    if err != nil {
        log.Fatalf("Error loading config: %v", err)
    }

    //log.Printf("%+v\n", config)

    if os.Geteuid() == 0 && config.Username != "" {
        if err := utils.SwitchUser(config.Username); err != nil {
            log.Fatal(err)
        }
        log.Printf("Successfully switched to user %s", config.Username)
    }

    if *daemon {
        utils.Daemonize()
    }

    processQueue(config)
}
