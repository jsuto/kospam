package main

import (
    "bufio"
    "crypto/tls"
    "fmt"
    "net"
    "os"
    "path/filepath"
    "strings"
    "sync"
    "time"
)

const queueDir = "/var/kospam/send"
const errorDir = "/var/kospam/send-error"
const maxConcurrency = 10
const scanInterval = 5 * time.Second
const myHostname = "kospam.localhost"

type Email struct {
    FilePath string
    Content  string
}

func sendEmail(email Email) bool {
    dialer := net.Dialer{Timeout: 15 * time.Second}
    conn, err := dialer.Dial("tcp", "127.0.0.1:10025")
    if err != nil {
        fmt.Println("Failed to connect:", err)
        return false
    }
    defer conn.Close()

    reader := bufio.NewReader(conn)
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ := reader.ReadString('\n')
    //fmt.Print(resp)

    fmt.Fprintf(conn, "EHLO %s\r\n", myHostname)
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

        fmt.Fprintf(conn, "EHLO %s\r\n", myHostname)
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
    fmt.Print(resp) // TODO: Log this!

    if !strings.HasPrefix(resp, "250") {
        return false
    }

    fmt.Fprintln(conn, "QUIT")
    conn.SetReadDeadline(time.Now().Add(15 * time.Second))
    resp, _ = reader.ReadString('\n')
    //fmt.Print(resp)

    return true
}

func processQueue() {
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
                    if sendEmail(email) {
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
    os.MkdirAll(errorDir, 0755)
    processQueue()
}
