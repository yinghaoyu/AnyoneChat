package config

import (
	"encoding/json"
	"log"
	"os"
)

type Config struct {
	Email struct {
		User string `json:"user"`
		Pass string `json:"pass"`
	} `json:"email"`
	MySQL struct {
		Host string `json:"host"`
		Port int    `json:"port"`
	} `json:"mysql"`
	Redis struct {
		Host   string `json:"host"`
		Port   int    `json:"port"`
		Passwd string `json:"passwd"`
	} `json:"redis"`
}

var config Config

func init() {
	data, err := os.ReadFile("config.json")
	if err != nil {
		log.Fatal(err)
	}

	err = json.Unmarshal(data, &config)
	if err != nil {
		log.Fatal(err)
	}
}

func GetEmailUser() string {
	return config.Email.User
}

func GetEmailPass() string {
	return config.Email.Pass
}

func GetMySQLHost() string {
	return config.MySQL.Host
}

func GetMySQLPort() int {
	return config.MySQL.Port
}

func GetRedisHost() string {
	return config.Redis.Host
}

func GetRedisPort() int {
	return config.Redis.Port
}

func GetRedisPasswd() string {
	return config.Redis.Passwd
}

func GetCodePrefix() string {
	return "code_"
}
