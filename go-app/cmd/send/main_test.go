package main

import (
    "os"
    "testing"
)

func TestParseEmailFile(t *testing.T) {
    tempFile, err := os.CreateTemp("", "test_email.txt")
    if err != nil {
        t.Fatal(err)
    }
    defer os.Remove(tempFile.Name())

    emailContent := "Kospam-Envelope-From: sender@bbb.fu\r\n" +
        "Kospam-Envelope-Recipient: aaa@aaa.fu,bbb@aaa.fu\r\n" +
        "Kospam-Xforward: client-192-168-19-02.cable.aaa.fu,192.168.19.2\r\n" +
        "Return-Path: <sport@trops.eu>\r\n" +
        "Delivered-To: bela@aaa.fu\r\n" +
        "Received: from antispam.localhost (localhost [127.0.0.1])\r\n"

    if _, err := tempFile.WriteString(emailContent); err != nil {
        t.Fatal(err)
    }
    tempFile.Close()

    sender, rcpt, content, err := parseEmailFile(tempFile.Name())
    if err != nil {
        t.Fatalf("Unexpected error: %v", err)
    }

    expectedSender := "sender@bbb.fu"
    expectedRcpt := []string{"aaa@aaa.fu", "bbb@aaa.fu"}
    expectedContent := "Return-Path: <sport@trops.eu>\r\n" +
        "Delivered-To: bela@aaa.fu\r\n" +
        "Received: from antispam.localhost (localhost [127.0.0.1])\r\n"

    if sender != expectedSender {
        t.Errorf("Expected sender %q, got %q", expectedSender, sender)
    }

    if len(rcpt) != len(expectedRcpt) {
        t.Fatalf("Expected %d recipients, got %d", len(expectedRcpt), len(rcpt))
    }
    for i, r := range rcpt {
        if r != expectedRcpt[i] {
            t.Errorf("Expected recipient %q, got %q", expectedRcpt[i], r)
        }
    }

    if content != expectedContent {
        t.Errorf("Expected content:\n%q\nGot:\n%q", expectedContent, content)
    }
}

func TestParseEmailFileEmptyXforward(t *testing.T) {
    tempFile, err := os.CreateTemp("", "test_email.txt")
    if err != nil {
        t.Fatal(err)
    }
    defer os.Remove(tempFile.Name())

    emailContent := "Kospam-Envelope-From: sender@bbb.fu\r\n" +
        "Kospam-Envelope-Recipient: aaa@aaa.fu,bbb@aaa.fu\r\n" +
        "Kospam-Xforward: \r\n" +
        "Return-Path: <sport@trops.eu>\r\n" +
        "Delivered-To: bela@aaa.fu\r\n" +
        "Received: from antispam.localhost (localhost [127.0.0.1])\r\n"

    if _, err := tempFile.WriteString(emailContent); err != nil {
        t.Fatal(err)
    }
    tempFile.Close()

    sender, rcpt, content, err := parseEmailFile(tempFile.Name())
    if err != nil {
        t.Fatalf("Unexpected error: %v", err)
    }

    expectedSender := "sender@bbb.fu"
    expectedRcpt := []string{"aaa@aaa.fu", "bbb@aaa.fu"}
    expectedContent := "Return-Path: <sport@trops.eu>\r\n" +
        "Delivered-To: bela@aaa.fu\r\n" +
        "Received: from antispam.localhost (localhost [127.0.0.1])\r\n"

    if sender != expectedSender {
        t.Errorf("Expected sender %q, got %q", expectedSender, sender)
    }

    if len(rcpt) != len(expectedRcpt) {
        t.Fatalf("Expected %d recipients, got %d", len(expectedRcpt), len(rcpt))
    }
    for i, r := range rcpt {
        if r != expectedRcpt[i] {
            t.Errorf("Expected recipient %q, got %q", expectedRcpt[i], r)
        }
    }

    if content != expectedContent {
        t.Errorf("Expected content:\n%q\nGot:\n%q", expectedContent, content)
    }
}

func TestParseEmailFileMissingSender(t *testing.T) {
    tempFile, err := os.CreateTemp("", "test_email_missing_sender.txt")
    if err != nil {
        t.Fatal(err)
    }
    defer os.Remove(tempFile.Name())

    emailContent := "Kospam-Envelope-Recipient: aaa@aaa.fu,bbb@aaa.fu\r\n" +
        "Kospam-Xforward: client-192-168-19-02.cable.aaa.fu,192.168.19.2\r\n" +
        "Return-Path: <sport@trops.eu>\r\n" +
        "Delivered-To: bela@aaa.fu\r\n" +
        "Received: from antispam.localhost (localhost [127.0.0.1])\r\n"

    if _, err := tempFile.WriteString(emailContent); err != nil {
        t.Fatal(err)
    }
    tempFile.Close()

    _, _, _, err = parseEmailFile(tempFile.Name())
    if err == nil {
        t.Fatal("Expected an error due to missing sender, but got nil")
    }
}
