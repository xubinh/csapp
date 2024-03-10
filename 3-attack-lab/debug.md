`run -q`

错误信息:

```text
Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7dfe0d0 in __vfprintf_internal (s=0x7ffff7fa4780 <_IO_2_1_stdout_>, format=0x4032b4 "Type string:", ap=ap@entry=0x5561dbd8, mode_flags=mode_flags@entry=2) at ./stdio-common/vfprintf-internal.c:1244
1244    ./stdio-common/vfprintf-internal.c: No such file or directory.
```

栈:

```text
[0] from 0x00007ffff7dfe0d0 in __vfprintf_internal+160 at ./stdio-common/vfprintf-internal.c:1244
[1] from 0x00007ffff7ebdc4b in ___printf_chk+171 at ./debug/printf_chk.c:33
[2] from 0x0000000000401f10 in printf+20 at /usr/include/x86_64-linux-gnu/bits/stdio2.h:105
[3] from 0x0000000000401f10 in launch+92 at support.c:293
[4] from 0x0000000000401ffa in stable_launch+153 at support.c:340
```

附近的机器代码:

```text
 0x00007ffff7dfe0c0  __vfprintf_internal+144 movdqu (%rax),%xmm1
 0x00007ffff7dfe0c4  __vfprintf_internal+148 movups %xmm1,0x118(%rsp)
 0x00007ffff7dfe0cc  __vfprintf_internal+156 mov    0x10(%rax),%rax
 0x00007ffff7dfe0d0  __vfprintf_internal+160 movaps %xmm1,0x10(%rsp)
 0x00007ffff7dfe0d5  __vfprintf_internal+165 mov    %rax,0x128(%rsp)
```

断点:

1. `launch` | `rsp 0x0000000055685ff0`

   ```
    0x0000000000401efc  launch+72 mov    $0x4032b4,%esi  // "Type string:"
    0x0000000000401f01  launch+77 mov    $0x1,%edi
    0x0000000000401f06  launch+82 mov    $0x0,%eax
    0x0000000000401f0b  launch+87 call   0x400df0 <__printf_chk@plt>
   ```

1. `*0x401f0b  // 0x400df0 <__printf_chk@plt>` | `rsp 0x000000005561dcb8`

1. `*0x400df0  // 全局函数指针, 分发至 0x0000000000400df6 或 0x00007ffff7ebdba0` | `rsp 0x000000005561dcb0`

   ```
   !0x0000000000400df0  ? jmp    *0x2032ca(%rip)        # 0x6040c0 <__printf_chk@got.plt>
    0x0000000000400df6  ? push   $0x18
    0x0000000000400dfb  ? jmp    0x400c60
    0x0000000000400e00  ? jmp    *0x2032c2(%rip)        # 0x6040c8 <fopen@got.plt>
    0x0000000000400e06  ? push   $0x19
    0x0000000000400e0b  ? jmp    0x400c60
    0x0000000000400e10  ? jmp    *0x2032ba(%rip)        # 0x6040d0 <getopt@got.plt>
    0x0000000000400e16  ? push   $0x1a
    0x0000000000400e1b  ? jmp    0x400c60
    0x0000000000400e20  ? jmp    *0x2032b2(%rip)        # 0x6040d8 <strtoul@got.plt>
   ```

1. `*0x7ffff7ebdc46` | `rsp 0x000000005561dbd8`

   ```
   !0x00007ffff7ebdc46  ___printf_chk+166 call   0x7ffff7dfe030 <__vfprintf_internal>
    0x00007ffff7ebdc4b  ___printf_chk+171 mov    0x18(%rsp),%rdx
    0x00007ffff7ebdc50  ___printf_chk+176 sub    %fs:0x28,%rdx
    0x00007ffff7ebdc59  ___printf_chk+185 jne    0x7ffff7ebdc63 <___printf_chk+195>
    0x00007ffff7ebdc5b  ___printf_chk+187 add    $0xd8,%rsp
   ```

1. `*0x00007ffff7dfe030` | `rsp 0x000000005561dbd0`

1. `*0x00007ffff7dfe0cc` | `rsp 0x000000005561d668`

   ```
   !0x00007ffff7dfe0cc  __vfprintf_internal+156 mov    0x10(%rax),%rax
    0x00007ffff7dfe0d0  __vfprintf_internal+160 movaps %xmm1,0x10(%rsp)
    0x00007ffff7dfe0d5  __vfprintf_internal+165 mov    %rax,0x128(%rsp)
    0x00007ffff7dfe0dd  __vfprintf_internal+173 call   0x7ffff7db14d0 <*ABS*+0xab010@plt>
    0x00007ffff7dfe0e2  __vfprintf_internal+178 mov    %rbp,0xf8(%rsp)
   ```

   

函数 `__printf_chk@plt (FILE *stream, char *fmt)` 入口处的机器代码:

```text
 0x0000000000400df0  ? jmp    *0x2032ca(%rip)        # 0x6040c0 <__printf_chk@got.plt>
 0x0000000000400df6  ? push   $0x18
 0x0000000000400dfb  ? jmp    0x400c60
 0x0000000000400e00  ? jmp    *0x2032c2(%rip)        # 0x6040c8 <fopen@got.plt>
 0x0000000000400e06  ? push   $0x19
 0x0000000000400e0b  ? jmp    0x400c60
 0x0000000000400e10  ? jmp    *0x2032ba(%rip)        # 0x6040d0 <getopt@got.plt>
 0x0000000000400e16  ? push   $0x1a
 0x0000000000400e1b  ? jmp    0x400c60
 0x0000000000400e20  ? jmp    *0x2032b2(%rip)        # 0x6040d8 <strtoul@got.plt>
```

跳转点 (正常情况):

```text
>>> x/1xg ($rip+6+0x2032ca)
0x6040c0 <__printf_chk@got.plt>:        0x0000000000400df6
```

跳转点 (非正常情况):

```text
>>> x/1xg ($rip+6+0x2032ca)
0x6040c0 <__printf_chk@got.plt>:        0x00007ffff7ebdba0
```



---

`0x4032b4`:

```text
>>> print (char *) 0x4032b4
$10 = 0x4032b4 "Type string:"
```

源码 1:

```c
printf (__fmt=0x4032b4 "Type string:") at /usr/include/x86_64-linux-gnu/bits/stdio2.h:105
105       return __fprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt,
```

源码 2:

```c
 100
 101  # ifdef __va_arg_pack
 102  __fortify_function int
 103  fprintf (FILE *__restrict __stream, const char *__restrict __fmt, ...)
 104  {
 105    return __fprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt,
 106              __va_arg_pack ());
 107  }
 108
 109  __fortify_function int
```

跳转点 1:

```
 0x0000000000401f0b  printf+15 call   0x400df0 <__printf_chk@plt>
```

跳转点 2:

```
 0x0000000000400df0  ? jmp    *0x2032ca(%rip)        # 0x6040c0 <__printf_chk@got.plt>
```

跳转点 3: