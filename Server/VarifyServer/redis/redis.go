package redis

import (
	"VarifyServer/config"
	"context"
	"fmt"
	"log"
	"time"

	"github.com/go-redis/redis/v8"
)

// 创建 Redis 客户端实例
var rdb = redis.NewClient(&redis.Options{
	Addr:     fmt.Sprintf("%s:%d", config.GetRedisHost(), config.GetRedisPort()),
	Password: config.GetRedisPasswd(),
	DB:       0, // 默认数据库
})

// 获取 Redis 上的某个 key 对应的值
func GetRedis(ctx context.Context, key string) (string, error) {
	val, err := rdb.Get(ctx, key).Result()
	if err == redis.Nil {
		fmt.Println("This key does not exist:", key)
		return "", nil
	} else if err != nil {
		log.Println("Error getting value from Redis:", err)
		return "", err
	}
	fmt.Println("Result:", val, "Get key success!")
	return val, nil
}

// 查询 Redis 中是否存在某个 key
func QueryRedis(ctx context.Context, key string) (bool, error) {
	exists, err := rdb.Exists(ctx, key).Result()
	if err != nil {
		log.Println("Error checking key existence:", err)
		return false, err
	}
	if exists == 0 {
		fmt.Println("This key does not exist:", key)
		return false, nil
	}
	fmt.Println("Key exists:", key)
	return true, nil
}

// 设置 Redis 上的 key-value 并设置过期时间（秒）
func SetRedisExpire(ctx context.Context, key string, value interface{}, exptime int) error {
	err := rdb.Set(ctx, key, value, time.Duration(exptime)*time.Second).Err()
	if err != nil {
		log.Println("Error setting key-value in Redis:", err)
		return err
	}
	fmt.Println("Key-value set with expiry:", key, value, exptime)
	return nil
}

// 心跳机制：定时发送心跳消息
func StartHeartbeat(ctx context.Context) {
	go func() {
		for {
			err := rdb.Set(ctx, "heartbeat", time.Now().Unix(), 0).Err()
			if err != nil {
				log.Println("Error sending heartbeat:", err)
			}
			time.Sleep(60 * time.Second) // 每 60 秒发送一次心跳消息
		}
	}()
}
