package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"strings"

	"VarifyServer/config"
	"VarifyServer/email"
	"VarifyServer/redis"

	pb "VarifyServer/message"

	"github.com/google/uuid"
	"google.golang.org/grpc"
)

type server struct {
	pb.VarifyServiceServer
}

// 生成4位验证码
func randomCode() string {
	code := uuid.New().String()
	code = strings.ReplaceAll(code, "-", "")
	if len(code) > 4 {
		code = code[:4]
	}
	return code
}

func (s *server) GetVarifyCode(ctx context.Context, req *pb.GetVarifyReq) (*pb.GetVarifyRsp, error) {
	emailAddr := req.Email
	codeKey := config.GetCodePrefix() + emailAddr

	// 查询 Redis 是否已有验证码
	code, err := redis.GetRedis(ctx, codeKey)
	if err != nil {
		return &pb.GetVarifyRsp{
			Email: emailAddr,
			Error: int32(config.Errors.RedisErr),
		}, nil
	}
	uniqueId := code
	if code == "" {
		uniqueId = randomCode()
		err := redis.SetRedisExpire(ctx, codeKey, uniqueId, 180)
		if err != nil {
			return &pb.GetVarifyRsp{
				Email: emailAddr,
				Error: int32(config.Errors.RedisErr),
			}, nil
		}
	}

	textStr := "您的验证码为" + uniqueId + "请三分钟内完成注册"
	mailOptions := map[string]string{
		"to":      emailAddr,
		"subject": "验证码",
		"body":    textStr,
	}

	_, err = email.SendMail(mailOptions)
	if err != nil {
		log.Println("send mail error:", err)
		return &pb.GetVarifyRsp{
			Email: emailAddr,
			Error: int32(config.Errors.Exception),
		}, nil
	}

	return &pb.GetVarifyRsp{
		Email: emailAddr,
		Error: int32(config.Errors.Success),
		Code:  uniqueId,
	}, nil
}

func main() {
	ctx := context.Background()
	redis.StartHeartbeat(ctx)

	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	grpcServer := grpc.NewServer()
	pb.RegisterVarifyServiceServer(grpcServer, &server{})
	fmt.Println("varify server started on :50051")
	if err := grpcServer.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
