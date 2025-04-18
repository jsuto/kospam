package cache

import (
    "context"
    "os"
    "time"

    "github.com/redis/go-redis/v9"
)

var (
    rdb *redis.Client
    ctx = context.Background()
)

// InitRedis initializes the Redis connection pool
func InitRedis() {
    rdb = redis.NewClient(&redis.Options{
        Addr:         os.Getenv("REDIS_ADDR"),
        Password:     "",
        DB:           0,
        PoolSize:     30,  // Higher than concurrent connections to handle spikes
        MinIdleConns: 10,  // Keep some connections ready for immediate use
        DialTimeout:  1 * time.Second,
        ReadTimeout:  1 * time.Second,
        WriteTimeout: 1 * time.Second,
        PoolTimeout:  2 * time.Second,
    })
}

// UpdateQueueCounter updates the specified queue counter
func UpdateQueueCounter(queueName string, delta int64) error {
    if rdb == nil {
        InitRedis() // Initialize if not already done
    }

    key := "queue:" + queueName
    _, err := rdb.IncrBy(ctx, key, delta).Result()
    return err
}

// GetQueueCounter retrieves the current value of the specified queue counter
func GetQueueCounter(queueName string) (int64, error) {
    if rdb == nil {
        InitRedis() // Initialize if not already done
    }

    key := "queue:" + queueName
    val, err := rdb.Get(ctx, key).Int64()
    if err == redis.Nil {
        // Key doesn't exist, return 0 without error
        return 0, nil
    }
    return val, err
}
