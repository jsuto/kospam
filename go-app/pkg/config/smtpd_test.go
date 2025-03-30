package config // adjust the package name as needed

import (
    "os"
    "path/filepath"
    "testing"

    "github.com/stretchr/testify/assert"
)

func TestLoadS3Config(t *testing.T) {
    tests := []struct {
        name     string
        content  string
        want     *SmtpdConfig
        wantErr  bool
    }{
        {
            name: "valid full config",
            content: `hostname = antispam.local
listen_addr = 127.0.0.1:10025
max_line_len=1000
max_message_bytes = 1000000
max_recipients = 10
pemfile = kospam.pem
smtp_acl = test.acl
number_of_worker_processes=3
queue_dir=/var/kospam/tmp
envelope_dir=/var/kospam/envelope
username = kospam`,
            want: &SmtpdConfig{
                Acl:             "test.acl",
                EnvelopeDir:     "/var/kospam/envelope",
                Hostname:        "antispam.local",
                ListenAddr:      "127.0.0.1:10025",
                MaxLineLength:   1000,
                MaxMessageBytes: 1000000,
                MaxRecipients:   10,
                NumWorkers:      3,
                PemFile:         "kospam.pem",
                QueueDir:        "/var/kospam/tmp",
                Username:        "kospam",
            },
            wantErr: false,
        },
        {
            name: "minimal config with defaults",
            content: `hostname = antispam.local
listen_addr = 127.0.0.1:10025
max_message_bytes = 2000000`,
            want: &SmtpdConfig{
                Acl:             "/etc/kospam/smtp.acl",
                EnvelopeDir:     "/var/kospam/envelope",
                Hostname:        "antispam.local",
                ListenAddr:      "127.0.0.1:10025",
                MaxLineLength:   2000,
                MaxMessageBytes: 2000000,
                MaxRecipients:   128,
                NumWorkers:      3,
                PemFile:         "/etc/kospam/kospam.pem",
                QueueDir:        "/var/kospam/tmp",
                Username:        "kospam",
            },
            wantErr: false,
        },
        {
            name: "invalid max recipient value",
            content: `hostname = antispam.local
max_recipients = invalid`,
            want: &SmtpdConfig{
                Acl:             "/etc/kospam/smtp.acl",
                EnvelopeDir:     "/var/kospam/envelope",
                Hostname:        "antispam.local",
                ListenAddr:      "127.0.0.1:10025",
                MaxLineLength:   2000,
                MaxMessageBytes: 52428800,
                MaxRecipients:   128,
                NumWorkers:      3,
                PemFile:         "/etc/kospam/kospam.pem",
                QueueDir:        "/var/kospam/tmp",
                Username:        "kospam",
            },
            wantErr: false,
        },
        {
            name: "negative max recipient value",
            content: `hostname = antispam.local
max_recipients = -5`,
            want: &SmtpdConfig{
                Acl:             "/etc/kospam/smtp.acl",
                EnvelopeDir:     "/var/kospam/envelope",
                Hostname:        "antispam.local",
                ListenAddr:      "127.0.0.1:10025",
                MaxLineLength:   2000,
                MaxMessageBytes: 52428800,
                MaxRecipients:   128,
                NumWorkers:      3,
                PemFile:         "/etc/kospam/kospam.pem",
                QueueDir:        "/var/kospam/tmp",
                Username:        "kospam",
            },
            wantErr: false,
        },
        {
            name: "malformed line",
            content: `hostname = antispam.local
invalid_line_without_equals
max_message_bytes = 2000000`,
            want: &SmtpdConfig{
                Acl:             "/etc/kospam/smtp.acl",
                EnvelopeDir:     "/var/kospam/envelope",
                Hostname:        "antispam.local",
                ListenAddr:      "127.0.0.1:10025",
                MaxLineLength:   2000,
                MaxMessageBytes: 2000000,
                MaxRecipients:   128,
                NumWorkers:      3,
                PemFile:         "/etc/kospam/kospam.pem",
                QueueDir:        "/var/kospam/tmp",
                Username:        "kospam",
            },
            wantErr: false,
        },
    }

    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            // Create temporary config file
            tmpDir := t.TempDir()
            tmpFile := filepath.Join(tmpDir, "config.txt")
            err := os.WriteFile(tmpFile, []byte(tt.content), 0644)
            if err != nil {
                t.Fatalf("Failed to create test config file: %v", err)
            }

            // Test the config loader
            got, err := LoadSmtpdConfig(tmpFile)
            if tt.wantErr {
                assert.Error(t, err)
                return
            }

            assert.NoError(t, err)
            assert.Equal(t, tt.want, got)
        })
    }
}

func TestLoadS3ConfigFileError(t *testing.T) {
    // Test with non-existent file
    _, err := LoadSmtpdConfig("nonexistent_file.txt")
    assert.Error(t, err)
    assert.Contains(t, err.Error(), "error opening config file")
}
