package acl

import (
    "bufio"
    "log"
    "net"
    "os"
    "strings"
)

const AclDenyText = "Service currently unavailable"

type CIDRRule struct {
    CIDR   *net.IPNet
    Permit bool
}

func LoadACL(filename string) []CIDRRule {
    var acl []CIDRRule

    file, err := os.Open(filename)
    if err != nil {
        log.Printf("Failed to open ACL file %s, we are wide open\n", filename)
        return acl
        //log.Fatalf("Failed to open ACL file: %v", err)
    }
    defer file.Close()

    //var acl []CIDRRule
    scanner := bufio.NewScanner(file)
    for scanner.Scan() {
        line := strings.TrimSpace(scanner.Text())
        if line == "" || strings.HasPrefix(line, "#") {
            continue // Skip empty lines and comments
        }

        fields := strings.Fields(line)
        if len(fields) != 2 {
            log.Fatalf("Invalid ACL rule format: %s", line)
        }

        _, cidr, err := net.ParseCIDR(fields[0])
        if err != nil {
            log.Fatalf("Invalid CIDR: %s", fields[0])
        }
        permit := fields[1] == "permit"
        acl = append(acl, CIDRRule{CIDR: cidr, Permit: permit})
    }

    if err := scanner.Err(); err != nil {
        log.Fatalf("Error reading ACL file: %v", err)
    }

    return acl
}
