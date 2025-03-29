package utils

import (
    "path/filepath"
    "log"
    "os"
    "strconv"
)

func CreateSubDirs(path string, n int) {
    for i := 0; i < n; i++ {
       dir := filepath.Join(path, strconv.Itoa(i))
       if _, err := os.Stat(dir); os.IsNotExist(err) {
           if err := os.Mkdir(dir, 0700); err != nil {
               log.Fatalf("Failed to create error directory: %v", err)
           }
           log.Println("Created dir", dir)
       }
    }
}
