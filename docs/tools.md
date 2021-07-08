## 内置工具

内置工具为 [FPNN 框架](https://github.com/highras/fpnn) 内置工具的子集。为 FPNN 技术生态通用工具。

内置工具位置：[/tools/](../tools/)

### cmd

FPNN 通用命令行客户端。可以发送任何 fpnn 命令，调用任何 fpnn 接口。

	Usage: ./cmd ip port method body(json) [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-pem ecc-pem-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-der ecc-der-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-ecc-curve ecc-curve-name -ecc-raw-key ecc-raw-public-key-file] [-json] [-oneway] [-t timeout]
	Usage: ./cmd ip port method body(json) [-udp] [-json] [-oneway] [-discardable] [-t timeout]

支持 UDP 和 TCP 链接。

### fss

FPNN Secure Shell，FPNN 加密交互式命令行终端。  
支持 UDP 和 TCP 链接。对于 TCP链接，支持加密链接和非加密链接。

	Usage: ./fss ip port
	Usage: ./fss ip port -udp
	Usage: ./fss ip port -pem pem-file [encrypt-mode-opt] [encrypt-strength-opt]
	Usage: ./fss ip port -der der-file [encrypt-mode-opt] [encrypt-strength-opt]
	Usage: ./fss ip port ecc-curve raw-public-key-file [encrypt-mode-opt] [encrypt-strength-opt]

+ ecc-curve：  
	secp192r1、secp224r1、secp256r1、secp256k1 四者之一。
+ encrypt-mode-opt："stream" 或 "package"
+ encrypt-strength-opt："128bits" 或 "256bits"