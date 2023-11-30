RFC3394: Advanced Encryption Standard (AES) Key Wrap Algorithm
==============================================================

[RFC3394](https://www.rfc-editor.org/rfc/rfc3394)是一种基于AES的秘钥包装算法.

AES-Key Wrap 下面简写成 AES-KW.

当通信的双方需要传递一串秘钥时, 通过明文传输是不够安全的, 因此需要发明一种算法, 对这串
需要传递的秘钥使用另外一个秘钥(KEK)进行加密, 接收方收到之后使用同样的KEK进行解密.
RFC3394 就定义了这一种算法.

openssl 中已经实现了这一个算法, 通过这个库编写了一个测试用例: [aes.c](./aes.c)

假设发送发和接收方使用的KEK秘钥

        0x0349144194681655ec5ab1d8f8451109

假设发送方需要传递的秘钥是

        0xdd16000fac010000461d435d6fa20994287b108632fcf6ffdd08000fac0700278c6cdd050c5a9e0201dd000000000000

# 发送方

通过 KEK 对发送的秘钥进行包装:

```console
$ ./aes.out --wrap --key 0x0349144194681655ec5ab1d8f8451109 \
        0xdd16000fac010000461d435d6fa20994287b108632fcf6ffdd08000fac0700278c6cdd050c5a9e0201dd000000000000
108194534fb1f64dd062e19da756402a401ddce6c8ff8845c538fc5dc0c1089c83566b1b9cb410b580b9f789f5e16b8d21c3b1f7bd7415b6
```

之后发送方通过网络将包装之后的数据发送给接收方.

# 接收方

使用同样的 KEK 秘钥对数据进行解包:

```consosle
$ ./aes.out --unwrap --key 0x0349144194681655ec5ab1d8f8451109 \
        0x108194534fb1f64dd062e19da756402a401ddce6c8ff8845c538fc5dc0c1089c83566b1b9cb410b580b9f789f5e16b8d21c3b1f7bd7415b6
dd16000fac010000461d435d6fa20994287b108632fcf6ffdd08000fac0700278c6cdd050c5a9e0201dd000000000000
```

可以看到, 接收方已经正确的接收到了发送方的秘钥了.
