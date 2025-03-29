package version

import "testing"

func TestVersion(t *testing.T) {
    if VERSION != "APP_VERSION" {
        t.Errorf("VERSION = %q, want %q", VERSION, "APP_VERSION")
    }
}
