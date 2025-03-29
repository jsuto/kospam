package main

import (
    "crypto/rand"
    "crypto/tls"
    "encoding/base32"
    "fmt"
    "io"
    "log"
    "net"
    "os"
    "path/filepath"
    "strings"
    "time"

    "github.com/jsuto/go-smtp"

    "kospam/smtpd/pkg/acl"

)

type Backend struct{
    acl []acl.CIDRRule
}

func (b *Backend) NewSession(c *smtp.Conn) (smtp.Session, error) {
    clientAddr := c.Conn().RemoteAddr().(*net.TCPAddr).IP
    log.Printf("connection from %s", clientAddr)

    allowed := false
    for _, rule := range b.acl {
        if rule.CIDR.Contains(clientAddr) {
            if !rule.Permit {
                log.Printf("denied connection from %s, acl: %s reject", clientAddr, rule.CIDR)
                c.SendResponse(550, acl.AclDenyText)
                c.Close()
                return nil, nil
            }
            allowed = true
        }
    }

    if len(b.acl) > 0 && !allowed {
        log.Printf("denied connection from %s by implicit default deny", clientAddr)
        c.SendResponse(550, acl.AclDenyText)
        c.Close()
    }

    return &Session{}, nil
}

type Xforward struct {
    Name string
    Addr string
    Proto string
    Helo string
}

type Session struct{
    queueID string
    mailFrom string
    rcptTo []string
    xforward Xforward
}

func (s *Session) Xforward(opts *smtp.XforwardOptions) error {
    if opts.Name != "" {
        s.xforward.Name = opts.Name
    }

    if opts.Addr != "" {
        s.xforward.Addr = opts.Addr
    }

    if opts.Proto != "" {
        s.xforward.Proto = opts.Proto
    }

    if opts.Helo != "" {
        s.xforward.Helo = opts.Helo
    }

    /*log.Printf("XFORWARD NAME: %s", opts.Name)
    log.Printf("XFORWARD ADDR: %s", opts.Addr)
    log.Printf("XFORWARD PROTO: %s", opts.Proto)
    log.Printf("XFORWARD HELO: %s", opts.Helo)*/

    return nil
}

func (s *Session) Mail(from string, opts *smtp.MailOptions) error {
    s.queueID = generateQueueID()
    s.mailFrom = from
    log.Printf("MAIL FROM: %s, Queue ID: %s", from, s.queueID)
    log.Printf("%s %s %s %s", s.xforward.Name, s.xforward.Addr, s.xforward.Proto, s.xforward.Helo)
    return nil
}

func (s *Session) Rcpt(to string, opts *smtp.RcptOptions) error {
    s.rcptTo = append(s.rcptTo, to)
    log.Printf("RCPT TO: %s", to)
    return nil
}

func (s *Session) Data(r io.Reader) error {
    log.Println("Receiving data...")
    filePath := filepath.Join("/var/clapf/tmp", s.queueID)
    envPath := filepath.Join("/var/clapf/envelope", s.queueID)

    file, err := os.Create(filePath)
    if err != nil {
        return fmt.Errorf("failed to create file: %w", err)
    }
    defer file.Close()

    _, err = io.Copy(file, r)
    if err != nil {
        return fmt.Errorf("failed to write email to file: %w", err)
    }

    envFile, err := os.Create(envPath)
    if err != nil {
        return fmt.Errorf("failed to create envelope file: %w", err)
    }
    defer envFile.Close()

    envData := fmt.Sprintf("MAIL FROM: %s\nRCPT TO: %s\n", s.mailFrom, strings.Join(s.rcptTo, ", "))
    _, err = envFile.WriteString(envData)
    if err != nil {
        return fmt.Errorf("failed to write envelope data: %w", err)
    }

    log.Printf("Email saved to %s", filePath)
    log.Printf("Envelope saved to %s", envPath)
    return nil
}

func (s *Session) XForward(args map[string]string) error {
    log.Printf("XFORWARD received: %+v", args)
    return nil
}

func (s *Session) Reset() {}

func (s *Session) Logout() error {
    return nil
}

func generateQueueID() string {
    b := make([]byte, 15) // 20 characters when base32 encoded
    _, err := rand.Read(b)
    if err != nil {
        log.Fatal("failed to generate queue ID")
    }
    return strings.ToUpper(base32.StdEncoding.WithPadding(base32.NoPadding).EncodeToString(b))
}

func main() {
    backend := &Backend{acl: acl.LoadACL("smtp.acl")}
    server := smtp.NewServer(backend)

    server.Addr = ":2525"
    server.Domain = "example.com"
    server.MaxMessageBytes = 10 * 1024 * 1024
    server.AllowInsecureAuth = true
    server.ReadTimeout = 30 * time.Second
    server.WriteTimeout = 30 * time.Second
    server.MaxRecipients = 128

    tlsCert, err := tls.LoadX509KeyPair("server.pem", "server.pem")
    if err != nil {
        log.Fatalf("failed to load TLS certificate: %v", err)
    }
    tlsConfig := &tls.Config{
        MinVersion: tls.VersionTLS12,
        Certificates: []tls.Certificate{tlsCert},
    }
    server.TLSConfig = tlsConfig

    log.Println("Starting SMTP server on", server.Addr)
    err = server.ListenAndServe()
    if err != nil {
        log.Fatal(err)
    }
}
