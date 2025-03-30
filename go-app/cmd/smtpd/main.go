package main

import (
    "crypto/tls"
    "flag"
    "fmt"
    "io"
    "log"
    "net"
    "os"
    "path/filepath"
    "strconv"
    "strings"
    "time"
    randmath "math/rand"

    "github.com/jsuto/go-smtp"

    "kospam/smtpd/pkg/acl"
    "kospam/smtpd/pkg/config"
    "kospam/smtpd/pkg/utils"
    "kospam/smtpd/pkg/version"

)

type Backend struct{
    acl []acl.CIDRRule;
    envelopeDir string;
    queueDir string;
    numWorkers int;
}

var (
    configfile = flag.String("config", "kospam.conf", "config file to use")
    showVersion = flag.Bool("version", false, "show version number, then exit")
)

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

    return &Session{
        envelopeDir: b.envelopeDir,
        queueDir: b.queueDir,
        numWorkers: b.numWorkers,
        },
        nil
}

type Xforward struct {
    Name string
    Addr string
    Proto string
    Helo string
}

type Session struct{
    numWorkers int
    queueID string
    queueDir string
    envelopeDir string
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
    s.queueID = opts.QueueId
    s.mailFrom = from
    log.Printf("MAIL FROM: %s", from)

    //log.Printf("forward: %s %s %s %s", s.xforward.Name, s.xforward.Addr, s.xforward.Proto, s.xforward.Helo)
    return nil
}

func (s *Session) Rcpt(to string, opts *smtp.RcptOptions) error {
    s.rcptTo = append(s.rcptTo, to)
    log.Printf("RCPT TO: %s", to)
    return nil
}

func (s *Session) Data(r io.Reader) error {
    subdir := strconv.Itoa(randmath.Intn(1000) % s.numWorkers)

    filePath := filepath.Join(s.queueDir, subdir, s.queueID)

    // Save the email to file

    file, err := os.OpenFile(filePath, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0600)
    if err != nil {
        return fmt.Errorf("failed to create file: %w", err)
    }
    defer file.Close()

    header := fmt.Sprintf("Kospam-Envelope-From: %s\r\nKospam-Envelope-Recipient: %s\r\n", s.mailFrom, strings.Join(s.rcptTo, ","))
    if _, err := file.WriteString(header); err != nil {
        return fmt.Errorf("failed to write header: %w", err)
    }

    _, err = io.Copy(file, r)
    if err != nil {
        return fmt.Errorf("failed to write email to file: %w", err)
    }

    log.Printf("Email saved to %s", filePath)

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

func createDirs(config *config.SmtpdConfig) {
    utils.CreateSubDirs(config.QueueDir, config.NumWorkers)
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

    createDirs(config)

    backend := &Backend{
        acl: acl.LoadACL(config.Acl),
        envelopeDir: config.EnvelopeDir,
        queueDir: config.QueueDir,
        numWorkers: config.NumWorkers,
    }
    server := smtp.NewServer(backend)

    server.Addr = config.ListenAddr
    server.Domain = config.Hostname
    server.MaxMessageBytes = config.MaxMessageBytes
    server.AllowInsecureAuth = true
    server.ReadTimeout = 30 * time.Second
    server.WriteTimeout = 30 * time.Second
    server.MaxRecipients = config.MaxRecipients
    server.MaxLineLength = 6000 // The RFC says the max line length is 1000

    tlsCert, err := tls.LoadX509KeyPair(config.PemFile, config.PemFile)
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
