package email

import (
	"VarifyServer/config"

	"gopkg.in/gomail.v2"
)

var smtpHost = "smtp.126.com"
var smtpPort = 465

func SendMail(mailOptions map[string]string) (string, error) {
	m := gomail.NewMessage()
	m.SetHeader("From", config.GetEmailUser())
	m.SetHeader("To", mailOptions["to"])
	m.SetHeader("Subject", mailOptions["subject"])
	m.SetBody("text/plain", mailOptions["body"])

	d := gomail.NewDialer(smtpHost, smtpPort, config.GetEmailUser(), config.GetEmailPass())
	d.SSL = true // 465端口必须SSL

	err := d.DialAndSend(m)
	if err != nil {
		return "", err
	}
	return "邮件已成功发送", nil
}
