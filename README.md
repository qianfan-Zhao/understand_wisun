understand wisun
================

WISUN自身的标准不是开放的, 需要在联盟付费注册账户, 才能拿到. 其PHY部分是基于 IEEE
802.15.4规范, 还有一部分是基于 IEEE 802.11 规范. IEEE 的规范标准是开放的, 可以在
IEEE 官网上免费注册一个账号之后下载.

下面贴上两个标准的链接:

[802.15.4-2020 - IEEE Standard for Low-Rate Wireless Networks](https://ieeexplore.ieee.org/document/9144691)

[Part 11: Wireless LAN Medium Access Control (MAC) and Physical Layer (PHY) Specifications](https://ieeexplore.ieee.org/document/9363693)

# 目录

+ [node加入br的wireshark包](./wireshark/20231128/README.md)
+ [HMAC是啥](./hmac/README.md)
+ [PRF-n: n比特伪随机数算法](./ieee80211i_prf/README.md)
+ [秘钥](./key/README.md)
+ [AES算法](./aes/AES.md)
+ [AES38a](./aes/AES38a.md)
+ [AES38c (CCM)](./aes/AES38c.md)
+ [IEEE 802.15.4 AES-128-CCM*](./aes/IEEE802154_AES_CCM.md)
+ [RFC3394: 基于AES的秘钥包装算法](./aes/RFC3394.md)
+ [EAPOL-KEY中的MIC](./wireshark/eapol_key_mic.md)
+ [EAPOL-KEY中的Data](./wireshark/eapol_key_data.md)
+ [4步握手交换PTK秘钥](./wireshark/4way_handshake.md)
+ [2步握手: Group key handshake](./wireshark/2way_handshake.md)
+ [WISUN中的数据加密](./wireshark/data_encrypt.md)


非对称加密:

+ [RSA](./asymmetric/rsa/README.md)
+ [EC](./asymmetric/ec/README.md)

TLS握手

+ [TLS流程](./wireshark/tls.md)
+ [TLS中的ECDHE](./wireshark/tls_ecdhe.md)
+ [TLS客户端证书自证](./wireshark/tls_client_cert_verify.md)
