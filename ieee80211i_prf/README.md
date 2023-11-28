PRF-n: Pseudo-random function producing n bits of output
========================================================

PRF 是在 802.11i 规范中定义的一种伪随机数产生算法. 可以支持对任意数量的输入数据产生
128, 192, 256, 384, 512bits 的伪随机数. 一个简单的实现可以参考 [prf.c](./prf.c)

测试代码编译:
(编译之后会自动运行 802.11i 中的几个测试示例)

```console
$ sh ./prf.c
1: test pass
2: test pass
3: test pass
```

PRF 有三个输入参数, key, prefix 和 data. 给定输入参数和输出比特个数, 可以生成固定的
随机数.

```console
$ ./prf.out --key 0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b \
            --prefix "prefix" \
            --data "Hi There" \
            --bit 512
0xbcd4c650b30b9684951829e0d75f9d54
0xb862175ed9f00606e17d8da35402ffee
0x75df78c3d31e0f889f012120c0862beb
0x67753e7439ae242edb8373698356cf5a
```
