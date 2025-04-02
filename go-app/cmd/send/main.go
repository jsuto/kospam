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
    "os/exec"
    "path/filepath"
    "strings"
    "sync"
    "syscall"
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
    QueueId  string
    Sender   string
    Rcpt     []string
    FilePath string
    Content  string
}

var (
    configfile = flag.String("config", "/etc/kospam/kospam.conf", "config file to use")
    showVersion = flag.Bool("version", false, "show version number, then exit")
    daemon = flag.Bool("daemon", false, "run in daemon mode")
)

// TODO: Write a test!

func parseEmailFile(filename string) (string, []string, string, error) {
    data, err := os.ReadFile(filename)
    if err != nil {
        return "", nil, "", err
    }

    content := string(data)
    lines := strings.SplitN(content, "\r\n", 3)
    if len(lines) < 3 {
        return "", nil, "", fmt.Errorf("invalid file format")
    }

    sender := strings.TrimSpace(strings.TrimPrefix(lines[0], "Kospam-Envelope-From: "))
    rcpt := strings.Split(strings.TrimSpace(strings.TrimPrefix(lines[1], "Kospam-Envelope-Recipient: ")), ",")
    remainingContent := lines[2]

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

    // TODO: use the address in the Kospam-* headers

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

func daemonize() {
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


func init() {
    isDaemon := *daemon || os.Getenv("RUNNING_AS_DAEMON") == "true"

    // If running as daemon, redirect logs to syslog
    if isDaemon {
        // Connect to syslog
        syslogWriter, err := syslog.New(syslog.LOG_MAIL, syslogId)
        if err != nil {
            log.Fatalf("Failed to connect to syslog: %v", err)
        }

        // Redirect standard log package to syslog
        log.SetOutput(syslogWriter)

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
        daemonize()
    }

    processQueue(config)
}
