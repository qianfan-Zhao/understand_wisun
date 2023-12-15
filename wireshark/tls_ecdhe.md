TLS ECDHE
=========

EC-DH-E: Elliptic Curve Diffie–Hellman key exchange, 使用椭圆曲线的秘钥交换算法.

EC 的基础知识可以参考 [EC](../asymmetric/ec/README.md). TLS 中使用 EC 算法做了两件
事, 一个是 ECDHE 进行秘钥交换, 另一个是 ECDSA 进行数字签名. 下面是从
[wireshark/20231128](./20231128/node_join.pcapng) 中截取的 Server hello 的数据,
下面对数据进行分析.

![Server Hello](./pic/tls_ecdhe/server_hello_ecdhe.png)

从 wsbrd 的工程中复制 br 和 node 的秘钥如下:

```console
$ cat br_key.pem
-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgN0Zj70BWw1o/ZQWK
fzslLkjUW4eUEMWZK5YII0aA0PahRANCAARqgQLAeOxmxKWWzt4gzWl3jfUtgBIz
BVBss6GL6FYP11DYR+WeOnqFRSLfP/rX3iPVIWtsY9CiovQq90NBJj+q
-----END PRIVATE KEY-----

$ cat node_key.pem
-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQguF1oBuIMzOtpsOMH
df97vr2GppQfXOKDJ4RogFMk7QChRANCAASo6qu6eHf6ACvzdrrC2kH5CFG8zij1
1N1V40KjRqZeHNtSwXKXDs6BuCq2gIpCC5hOxuJyl4sxa/hNXsnFfoiP
-----END PRIVATE KEY-----
```

使用 openssl 生成对应的公钥:

```console
$ openssl ec -in br_key.pem -pubout -out br_pub.pem
$ openssl ec -in node_key.pem -pubout -out node_pub.pem
```

# 秘钥交换

参考 RFC5246 文档, 里面定义了 master_secret 的生成公式. 其中 ClientHello.random
和 ServerHello.random 都是明文传递的, 因此保密的关键是 pre_master_secret.

![master_secret](./pic/tls_ecdhe/master_secret.png)

这个 pre_master_secret 根据使用环境可以有多种传递方式, 可以是 RSA 加密或者是 ECDH.
参考 [EC](../asymmetric/ec/README.md) 一节中 ECDH 的介绍, 因为我们这边无法拿到两边
生成的 a, b 数字, 无法进行计算. 这个也就是 ECDH 算法保证的前向安全性.

两边计算出的联合秘钥被当做 pre_master_secret 使用. 参考 RFC5246 的介绍, 当计算出
pre_master_secret 应该立即计算 master_secret, 并在内存中销毁 pre_master_secret
防止造成秘钥泄漏.

![RFC5246](./pic/tls_ecdhe/rfc5246_f1113.png)

# 服务器对 ECDHE 关键数据进行签名

现在我们也知道生成 master_secret 的关键是 ClientHello.random, ServerHello.random
以及 ECDH 的交换数据, 为了防止这些关键数据被篡改, TLS 部分会使用 ECDSA 算法对这些关键
数据进行签名.

在 [EC](../asymmetric/ec/README.md) 一节中我们也介绍过 ECDSA 的原理, 生成的签名数据
中是通过 ASN1 DER 格式编码的 R 和 S. 我们将 ServerHello ECDH 的签名数据提取出来:

```console
serverSig=3046022100840caf00f162b8685d705d366d157981b9aedb36eb57eda3b313ad76bd8699430221009c882e88536a5c39708a9164782aa1d4d7dda903bf7fcee0bbca1f9e1f2ba981
$ echo ${serverSig} \
        | xxd -r -p \
        | openssl asn1parse -inform DER
    0:d=0  hl=2 l=  70 cons: SEQUENCE
    2:d=1  hl=2 l=  33 prim: INTEGER           :840CAF00F162B8685D705D366D157981B9AEDB36EB57EDA3B313AD76BD869943
   37:d=1  hl=2 l=  33 prim: INTEGER           :9C882E88536A5C39708A9164782AA1D4D7DDA903BF7FCEE0BBCA1F9E1F2BA981
```

其中 ECDH 参数中的签名区域是图中圈出的部分:

![ServerParams](./pic/tls_ecdhe/server_hello_ecdhe_server_params.png)

将全部数据提取出来, 使用 openssl 计算下签名值:

```console
$ clientRandom=248117040b044a8c10129f706e10d02a0d8aa4bbe850caf24774dc27313c0284
$ serverRandom=6565a6f1f88b9a68a37c3260cf00ee498f1ea1c3cdd91f04d9aebae0d120fdd8
$ serverParam=0300174104c782cae22bdf76cd7d8264fc4648c7b00cf6269f8c7102e807e5a1b86bc9c21c3aaa14db586a015852101419da7c48658d953067d271d9812921e658163366b2

$ echo "${clientRandom} ${serverRandom} ${serverParam}" |
        xxd -r -p > ke.bin
$ echo "${serverSig}" | xxd -r -p > ke.sign
```

使用 openssl 做验证, 可以看到验证成功.

```console
$ openssl dgst -sha256 -verify br_pub.pem -signature ke.sign ke.bin
Verified OK
```

如果我们使用 `openssl dgst` 对 ke.bin 计算一个签名文件, 将与 wireshark 中的无法对应,
因为签名时会使用一个随机数 R, 每次签出的数据都会不同.

