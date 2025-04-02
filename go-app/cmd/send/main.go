package main

import (
    "bufio"
    "crypto/tls"
    "flag"
    "fmt"
    "log"
    "log/syslog"
    "net"
    "os"
    "path/filepath"
    "strings"
    "sync"
    "time"

    "kospam/smtpd/pkg/config"
    "kospam/smtpd/pkg/utils"
    "kospam/smtpd/pkg/version"

)

const queueDir = "/var/kospam/send"
const errorDir = "/var/kospam/send-error"
const maxConcurrency = 10
const scanInterval = 5 * time.Second
const syslogId = "kospam/send"

type Email struct {
    FilePath string
    Content  string
}

var (
    configfile = flag.String("config", "/etc/kospam/kospam.conf", "config file to use")
    showVersion = flag.Bool("version", false, "show version number, then exit")
    daemon = flag.Bool("daemon", false, "run in daemon mode")
)

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
        fmt.Fprintln(conn, "STARTTLS")
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

    fmt.Fprintln(conn, "MAIL FROM:<sender@example.com>")
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    fmt.Fprintln(conn, "RCPT TO:<recipient@example.com>")
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    fmt.Fprintln(conn, "DATA")
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
    fmt.Printf("status=%s", resp)

    if !strings.HasPrefix(resp, "250") {
        return false
    }

    fmt.Fprintln(conn, "QUIT")
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
                content, err := os.ReadFile(path)
                if err != nil {
                    fmt.Println("Failed to read file:", path, err)
                    continue
                }

                email := Email{FilePath: path, Content: string(content)}

                wg.Add(1)
                sem <- struct{}{}
                go func(email Email) {
                    defer wg.Done()
                    if sendEmail(config, email) {
                        os.Remove(email.FilePath)
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
        // If running as daemon, fork to background
        if os.Getppid() != 1 {
            args := append([]string{os.Args[0]}, os.Args[1:]...)
            os.StartProcess(os.Args[0], args, &os.ProcAttr{
                Files: []*os.File{os.Stdin, os.Stdout, os.Stderr},
            })
            return
        }

        logwriter, err := syslog.New(syslog.LOG_MAIL, syslogId)
        if err != nil {
            log.Fatal("Failed to connect to syslog: ", err)
        }
        // Direct log output to syslog
        log.SetOutput(logwriter)
        log.SetFlags(log.Lshortfile)
    }

    //os.MkdirAll(errorDir, 0755)
    processQueue(config)
}
