package acl

import (
    "net"
    "os"
    "testing"
)

func TestLoadACL(t *testing.T) {
    // Create a temporary ACL file
    tmpFile, err := os.CreateTemp("", "acl_test_*.txt")
    if err != nil {
        t.Fatalf("Failed to create temp file: %v", err)
    }
    defer os.Remove(tmpFile.Name())

    rules := `
        # Sample ACL rules
        40.92.0.0/15 permit
        40.107.0.0/16 permit
        52.100.0.0/14 reject
        127.0.0.1/8 permit
        104.47.0.0/17 permit
    `

    if _, err := tmpFile.WriteString(rules); err != nil {
        t.Fatalf("Failed to write to temp file: %v", err)
    }
    tmpFile.Close()

    // Load ACL from the temporary file
    acl := LoadACL(tmpFile.Name())

    if len(acl) != 5 {
        t.Fatalf("Expected 5 ACL rules, got %d", len(acl))
    }

    testCases := []struct {
        IP     string
        Permit bool
    }{
        {"40.92.1.1", true},
        {"40.107.5.5", true},
        {"52.100.10.10", false},
        {"127.0.0.1", true},
        {"104.47.50.50", true},
        {"8.8.8.8", false}, // Should not match any rule
    }

    for _, testCase := range testCases {
        ip := net.ParseIP(testCase.IP)
        matched := false
        permit := false

        for _, rule := range acl {
            if rule.CIDR.Contains(ip) {
                matched = true
                permit = rule.Permit
                break
            }
        }

        if !matched && testCase.Permit {
            t.Errorf("IP %s: expected to match an allow rule but did not match any", testCase.IP)
        } else if matched && permit != testCase.Permit {
            t.Errorf("IP %s: expected permit=%t, got permit=%t", testCase.IP, testCase.Permit, permit)
        }
    }
}
