package config

var Errors = struct {
	Success   int
	RedisErr  int
	Exception int
}{
	Success:   0,
	RedisErr:  1,
	Exception: 2,
}
