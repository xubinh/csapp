# Labs

## 目录

[TOC]

## 1. Data Lab

### 目标

填充文件 `bits.c` 中所包含的全部 13 个 puzzle 的函数框架.

### 大纲

- 完成后的函数主体参考如下风格:

  ```c
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }
  
  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }
  ```

#### 整数 puzzle

- 仅允许使用函数形参和定义局部变量, 不允许定义全局变量.
- 仅允许使用白话代码 (straightline code) 完成每个函数, 不允许使用任何语句, 例如循环或条件语句等.
- 仅允许使用一元运算符 `!`, `˜`, 以及二元运算符 `&`, `ˆ`, `|`, `+`, `<<`, `>>`, **部分 puzzle 会进一步限制可用运算符的范围**.
- 仅允许使用 0 到 255 范围内的常数.
- 仅允许使用 `int` 类型.
- 不允许使用宏定义.
- 不允许定义新函数和调用任何函数.
- 不允许使用强制类型转换.
- 可以假设机器使用 32 位补码整数, 使用算数右移, 并且当位移量小于 0 或大于 31 时会导致无法预测的行为.

#### 浮点数 puzzle

- 仅允许使用条件或循环语句.
- 仅允许使用 `int` 和 `unsigned` 数据类型.
- 允许使用任意 `int` 和 `unsigned` 类型的常数.
- 允许使用任意算数, 逻辑和比较运算符.
- 不允许使用宏定义.
- 不允许定义新函数和调用任何函数.
- 不允许使用强制类型转换.

#### `btest` - 正确性检查工具

- `btest` 需要与 `bits.c` 一同编译, 用于检测所实现函数的正确性, 基本命令 (测试所有函数):

  ```bash
  ./btest
  ```

- 每次修改 `bits.c` 之后均需要重新编译 `btest`.

- 可以使用选项 `-f` 来单独指定一个函数进行测试, 此外还可以通过选项 `-1`, `-2` 和 `-3` 来指定传入的实参, 命令:

  ```bash
  ./btest -f bitXor -1 4 -2 5
  ```

#### `dlc` - 合法性检查工具

- 全称 data lab checker, 用于检测函数实现是否遵循上述规则, 基本命令:

  ```bash
  ./dlc bits.c
  ```

- 使用选项 `-e` 输出每个函数所使用的运算符的个数, 命令:

  ```bash
  ./dlc -e bits.c
  ```

- 使用命令 `./dlc -help` 查看文档.

- 程序 `dlc` 要求所有声明必须位于某个块中所有其他非声明语句之前.

- 赋值运算符 `=` 的使用次数不受限制.

#### `driver.pl` - 最终评分工具

- `driver.pl` 用于为全部实验代码进行最终评分.

#### 其他

- 帮手程序 `ishow` 和 `fshow` 分别用于展示相应的二进制模式. 两个程序均只输入一个参数, 该参数可以是表示十进制数, 十六进制数和浮点数的字符串. 使用 `make` 对两个程序进行编译.

- 如果要使用 `printf` 语句进行 debug, 直接使用即可, 无需包含 `stdio.h` 头文件 (可能 gcc 会报 warning, 忽略即可).

- 具体打分细则见 handout.

### 思路

#### #01 `bitXor` - 使用 `~` 和 `&` 实现 `^`

设两个比特 $p$ 和 $q$ 以及对应的命题 $P$ 和 $Q$, 在命题逻辑 (propositional logic) 下, 由德摩根定律 (De Morgan's laws), 异或运算可表示为

$$
\newcommand\and{\wedge}
\newcommand\or{\vee}
\newcommand\not{\neg}

\begin{equation}
P \oplus Q = (P \and \not Q) \or (\not P \and Q) = \not (\not (P \and \not Q) \and \not (\not P \and Q)),
\end{equation}
$$

翻译至布尔代数下即为

```text
p ^ q = ~((~(p & (~q))) & (~((~p) & q))).
```

#### #02 `tmin` - 返回 $TMin_w$

根据 $B2T_w$ 的定义,
$$
\begin{equation}
B2T_w (\boldsymbol{x}) \triangleq - x_{w - 1}2^{w-1} + \sum_{i=0}^{w-2} x_i 2^i,
\end{equation}
$$
最小值 $TMin_w = -2^{w - 1}$, 其二进制模式为 `0x100...00`, 即设置唯一的负权位即最高有效位为 1, 其他所有正权位为 0.

上述二进制模式在C语言下可利用移位运算得到:

```c
int tmin = 1 << 31; // 0x100...00
```

#### #03 `isTmax` - 判断是否为 $TMax_w$

在没有任何运算符与常数使用限制的情况下, 判断一个整型值 $x$ 是否为 $TMax_w$ 只需将其二进制模式与 $TMax_w$ 的进行比较即可:

```c
x == 0x7fffffff
```

由于本题限制了整型常量的范围, 因此无法直接与 `0x7fffffff` 进行比较. 为了解决这一问题可以通过验证 $x$ 的二进制模式是否具有与 $TMax_w$ 的相同的性质来间接进行判断, 因为表达式能够产生的二进制模式的范围并无限制 (例如全 1 模式 `0x111...11` 无法直接使用, 但可通过对 0 取反, 即 `!0`, 来间接得到). 注意由于本题同时限制了移位运算符的使用, 因此即使通过表达式也同样无法间接获取 `0x7fffffff`.

$TMax_w$ 具有如下两个特殊性质 (二者均在模 $2^w$ 意义下成立):

1. $TMax_w$ 的位模式加 1 会上溢至 $TMin_w$ 的位模式;
1. $TMax_w$ 的位模式取反等于 $TMin_w$ 的位模式.

根据上述两个性质构造等量关系 (令 $TMax_w$ 的位模式为 $x$):
$$
\begin{equation}
x + 1 \equiv 2^{w} - 1 - x \ \ (\operatorname{mod} 2^{w}),
\end{equation}
$$
解得 $x = 2^{w - 1} - 1$ 或 $x = 2^{w} - 1$. 因此 $x$ 为 $TMax_w$ 的位模式当且仅当
$$
\begin{numcases}{}
x + 1 \equiv 2^{w} - 1 - x \ \ (\operatorname{mod} 2^{w}), \label{istmax_1}\\
x \neq 2^w - 1. \label{istmax_2}
\end{numcases}
$$
$\eqref{istmax_1}$ 式左侧即为 `x + 1`, 右侧可表示为 `~x`. 由于整数 puzzle 同样限制了比较运算符的使用, 为了判断两个二进制位模式是否相同可以使用异或 `^` 与逻辑非 `!` 间接实现. 实际上两个位模式的异或结果为全 0 当且仅当这两个位模式相同, 而逻辑非能够方便的将异或结果转化为布尔值, 因此判断两个位模式 `x` 和 `y` 是否相同的表达式为 `!(x ^ y)`. 于是 $\eqref{istmax_1}$ 式可表示为:

```c
int x_plus_one = x + 1;
int x_complement = ~x;
int constraint_1 = !(x_plus_one ^ x_complement);
```

$\eqref{istmax_2}$ 式成立当且仅当 $x$ 取反不为全 0. 为了将取反结果转化为布尔值, 需要使用逻辑非 `!` 进行转化:

```c
int constraint_2 = !!(~x);
```

使用连续两个 `!` 是因为单次转化得到的布尔值为原值的相反结果.

结合 $\eqref{istmax_1}$ 和 $\eqref{istmax_2}$ 得到最终表达式:

```c
int is_tmax = constraint_1 & constraint_2;
```

#### #04 `allOddBits` - 判断一个二进制模式的所有奇数位是否均为 1

所有特定位均为 1 当且仅当所有特定位的按位与的结果为 1. 由于最大允许运算符数量为 12, 直接顺次对每个位进行按位与的暴力解法不可行. 一种办法是通过折半移位同时对多个位进行按位与:

```c
int x_32 = x;
int x_16 = x_32 & (x_32 >> 16);
int x_8 = x_16 & (x_16 >> 8);
int x_4 = x_8 & (x_8 >> 4);
int x_2 = x_4 & (x_4 >> 2);
```

`x_2` 的次低有效位即为所有奇数位的按位与的结果:

```c
int are_all_odd_bits = (x_2 >> 1) & 1;
```

#### #05 `negate` - 返回一个补码整数的相反数

原理就是取反加一, 但自己还是想要通过定义推一遍.

#### #06 `isAsciiDigit` [TODO]

比较运算可以通过将两个模式相减, 看看是否会得到负数.

#### #07 `conditional` [TODO]

首先判断是不是零, 然后如果是零的话就是全 0 模式, 不是的话就取反得到全 1 模式, 再使用按位与 `&` 进行条件选择.

#### #08 `isLessOrEqual` [TODO]

先检查两个数是否同号, 如果同号则通过相减是否得到负数进行判断, 如果异号的话直接检查哪一个正哪一个负就好.

#### #09 `logicalNeg` [TODO]

逻辑非等价于检查取反后的结果是否为全 0.

#### #10 `howManyBits` [TODO]

举个例子进行描述. `0x0001101` 需要 5 位, 具体是在 `0x00 01101` 这里断开的, 也就是要从左往右找最长连续相同的子串, 然后退一格断开. 自己的思路是先自己跟自己的右移一位进行按位异或, 得到每两位之间是否相同的结果, 最终答案的大小就等于按位异或得到的模式中从左往右最长全等子串的长度关于 32 的补集. 注意由于至少需要 1 位进行表示, 所以需要对异或的结果至少将其与 1 进行按位或. 最后使用二分法找到该子串的补集的长度.

#### #11 `floatScale2` [TODO]

分情况讨论, 如果是特殊值, 直接返回; 如果是非规格化数, 直接对尾数部分进行左移; 否则就是规格化数, 对阶码加一即可. 注意如果加一之后变为特殊值还需要对尾数进行清空.

#### #12 `floatFloat2Int` [TODO]

分情况讨论, 如果阶码大于整型范围则直接返回 (由于返回值刚好是 -2147483648, 所以阶码真实值等于 31 的情况也给包含进去了); 如果小于 0 则返回 0; 否则对尾数直接进行截断即可.

#### #13 `floatPower2` [TODO]

分情况讨论, 如果指数大于阶码范围则直接返回, 如果小于阶码范围则返回 0, 然后分别处理位于规格化范围和非规格化范围两种情况即可.

### 附录

- [维基百科: 命题逻辑](https://en.wikipedia.org/wiki/Propositional_calculus)

## 2. Bomb Lab

### 目标

对程序 `bomb` 进行调试, 解密六个字符串以通过六个相应阶段的考验.

### 大纲

#### `bomb.c` - 框架描述

`bomb.c` 中使用 C 程序描述了炸弹 bomb 的主体框架, 其中 `support.h` 与 `phases.h` 猜测是实现炸弹本体的代码的头文件.

观察 `bomb.c` 可得到 bomb 的大致框架:

- 首先检查命令行中是否给出文件路径, 并打开标准输入/给定文件.
- 初始化炸弹: `initialize_bomb();`
- 连续执行六个阶段, 每个阶段均为读入一行输入, 将输入用于解密, 若解密成功则可顺序进入下一阶段, 若六个阶段均解密成功则函数退出 (炸弹解除).

#### `bomb` - 主体

使用 GDB 对 `bomb` 进行 debug, 以破解六个 phase 的密码.

### 思路

下列所有函数的机器指令的具体解释见目录[disas-output](2-bomb-lab/disas-output). 某些特定函数所需要的额外信息 (例如跳转表或全局字符串等) 将会在对应的文件开头列出.

#### `explode_bomb` [TODO]

#### `strings_not_equal` [TODO]

#### `read_six_numbers` [TODO]

#### `phase_1`

通过调用函数 `int strings_not_equal(char *s1, char *s2)` 来比较密码是否与某个全局字符串相同. 函数成功返回当且仅当密码与该全局字符串相同.

#### `phase_2`

函数 `phase_2` 首先调用函数 `read_six_numbers` 从密码中读入 6 个整数, 然后检查这 6 个整数是否满足某些特定规则. 函数成功返回当且仅当这 6 个整数满足规则.

#### `phase_3`

函数 `phase_3` 实现了一个简单的C语言 `switch` 语句.

### 附录

#### GDB

- [Beej's Quick Guide to GDB](https://beej.us/guide/bggdb/)
- [GDB: The GNU Project Debugger](https://www.sourceware.org/gdb/)
- [gdbnotes-x86-64.pdf](http://csapp.cs.cmu.edu/3e/docs/gdbnotes-x86-64.pdf)
- stackoverflow: [How to highlight and color gdb output during interactive debugging?](https://stackoverflow.com/questions/209534/how-to-highlight-and-color-gdb-output-during-interactive-debugging)
  - GitHub: [gdb-dashboard](https://github.com/cyrus-and/gdb-dashboard)


#### Linux 命令

- `objdump -t <binary-executable>`:

  - 打印符号表 (symbol table).

- `objdump -d <binary-executable>`:

  - 反汇编. 对于像系统调用 (例如 `sscanf`) 这样的函数而言, 反汇编得到的函数名有可能不是很直观, 此时在 gdb 中进行反汇编是更好的选择.

- `strings <binary-executable>`:

  - 打印可执行文件中的所有可打印字符串.

- `man ascii`:

  - 打印关于 ascii 编码的相关文档.

- `info gas`:

  - 打印关于 gas 的文档. 如果报错 `info: No menu item 'gas' in node '(dir)Top'`, 需要检查 `binutils` 包是否安装:

    ```bash
    # Check if binutils is installed
    which as
    
    # If binutils is not installed, install it
    sudo apt-get update
    sudo apt-get install binutils
    ```

    如果已经安装, 再检查是否安装文档 (因为有些 Linux 发行版可能会将 doc 和 software 分开打包):

    ```bash
    # Install documentation for binutils, which includes gas
    sudo apt-get install binutils-doc
    ```
