# CmdService
以系统服务模式运行bat,cmd,exe类的命令程序，压缩包里面的Release版本支持2000以上，Win8未测试，配置(CmdService.ini)例如：

```
[Account]
Domain = .
User = administrator
Password = xxxxxx

[Command]
plink = plink.exe -N -ssh -2 -P 22 -C -R 6080:localhost:6080 -R 6306:localhost:3306 -R 6022:localhost:22 -R 3690:localhost:3690 -nc 218.244.139.39:22 -pw xxxxxx root@xxx.xxx.xxx.xxx
status = status.bat

[Environment]
plink = D:\putty
#status的起始位置默认为CmdService.exe所在目录
```
