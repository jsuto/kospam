package utils

import (
    "fmt"
    "os/user"
    "strconv"
    "syscall"
)

func SwitchUser(username string) error {
    if username == "" {
        return nil
    }

    u, err := user.Lookup(username)
    if err != nil {
        return fmt.Errorf("failed to lookup user %s: %v", username, err)
    }

    // Convert uid/gid to integers
    uid, err := strconv.Atoi(u.Uid)
    if err != nil {
        return fmt.Errorf("invalid uid %s: %v", u.Uid, err)
    }

    gid, err := strconv.Atoi(u.Gid)
    if err != nil {
        return fmt.Errorf("invalid gid %s: %v", u.Gid, err)
    }

    // Set supplementary group IDs
    groups, err := u.GroupIds()
    if err != nil {
        return fmt.Errorf("failed to get supplementary groups: %v", err)
    }

    gids := make([]int, 0, len(groups))
    for _, sgid := range groups {
        gid, err := strconv.Atoi(sgid)
        if err != nil {
            return fmt.Errorf("invalid supplementary gid %s: %v", sgid, err)
        }
        gids = append(gids, gid)
    }

    // Set supplementary groups first
    if err := syscall.Setgroups(gids); err != nil {
        return fmt.Errorf("failed to set supplementary groups: %v", err)
    }

    // Set GID
    if err := syscall.Setgid(gid); err != nil {
        return fmt.Errorf("failed to set GID: %v", err)
    }

    // Set UID
    if err := syscall.Setuid(uid); err != nil {
        return fmt.Errorf("failed to set UID: %v", err)
    }

    return nil
}
