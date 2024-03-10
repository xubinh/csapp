# <div align="center">Notes for CS:APP Labs</div>

<div align="center">

![GitHub Repo stars](https://img.shields.io/github/stars/xubinh/csapp?style=flat-square&color=007bff) ![GitHub forks](https://img.shields.io/github/forks/xubinh/csapp?style=flat-square&color=28a745)

</div>

<p align="center">关于 CS:APP 的 Labs 的一些笔记与总结</p>

<br>

<br>

<div align="center"><a href="http://csapp.cs.cmu.edu/"><img width=200 src="./csapp.png" alt="CS:APP"></img></a></div>

## <a id="toc"></a>目录

<details open="open"><summary><a href="#1">1. Data Lab</a></summary>

- <a href="#1.1">实验目的</a>
- <a href="#1.2">实验框架</a>
  - <a href="#1.2.1">整数 puzzle</a>
  - <a href="#1.2.2">浮点数 puzzle</a>
  - <a href="#1.2.3">`btest` - 正确性检查工具</a>
  - <a href="#1.2.4">`dlc` - 合法性检查工具</a>
  - <a href="#1.2.5">`driver.pl` - 最终评分工具</a>
  - <a href="#1.2.6">其他</a>
- <a href="#1.3">实验思路与总结</a>
  - <a href="#1.3.1">#01 `bitXor` - 使用 `~` 和 `&` 实现 `^`</a>
  - <a href="#1.3.2">#02 `tmin` - 返回 </a>$TMin_w$
  - <a href="#1.3.3">#03 `isTmax` - 判断是否为 </a>$TMax_w$
  - <a href="#1.3.4">#04 `allOddBits` - 判断一个二进制模式的所有奇数位是否均为 1</a>
  - <a href="#1.3.5">#05 `negate` - 返回一个补码整数的相反数</a>
  - <a href="#1.3.6">#06 `isAsciiDigit`</a>
  - <a href="#1.3.7">#07 `conditional`</a>
  - <a href="#1.3.8">#08 `isLessOrEqual`</a>
  - <a href="#1.3.9">#09 `logicalNeg`</a>
  - <a href="#1.3.10">#10 `howManyBits`</a>
  - <a href="#1.3.11">#11 `floatScale2`</a>
  - <a href="#1.3.12">#12 `floatFloat2Int`</a>
  - <a href="#1.3.13">#13 `floatPower2`</a>
- <a href="#1.4">相关资料</a>
  - <a href="#1.4.1">命题逻辑</a>

</details>
<details><summary><a href="#2">2. Bomb Lab</a></summary>

- <a href="#2.1">实验目的</a>
- <a href="#2.2">实验框架</a>
  - <a href="#2.2.1">`bomb` - 待调试文件主体</a>
  - <a href="#2.2.2">`bomb.c` - 描述主体构成</a>
- <a href="#2.3">实验思路与总结</a>
  - <a href="#2.3.1">`sig_handler`</a>
  - <a href="#2.3.2">`initialize_bomb`</a>
  - <a href="#2.3.3">`string_length`</a>
  - <a href="#2.3.4">`strings_not_equal`</a>
  - <a href="#2.3.5">`explode_bomb`</a>
  - <a href="#2.3.6">`phase_defused`</a>
  - <a href="#2.3.7">`read_six_numbers`</a>
  - <a href="#2.3.8">`func4`</a>
  - <a href="#2.3.9">`fun7`</a>
  - <a href="#2.3.10">`phase_1`</a>
  - <a href="#2.3.11">`phase_2`</a>
  - <a href="#2.3.12">`phase_3`</a>
  - <a href="#2.3.13">`phase_4`</a>
  - <a href="#2.3.14">`phase_5`</a>
  - <a href="#2.3.15">`phase_6`</a>
  - <a href="#2.3.16">`secret_phase`</a>
- <a href="#2.4">相关资料</a>
  - <a href="#2.4.1">GDB</a>
  - <a href="#2.4.2">Linux 命令</a>

</details>
<details><summary><a href="#3">3. Attack Lab</a></summary>

- <a href="#3.1">实验目的</a>
- <a href="#3.2">实验框架</a>
  - <a href="#3.2.1">`ctarget` - 易受 "代码注入" 攻击的可执行文件</a>
  - <a href="#3.2.2">`rtarget` - 易受 "返回导向编程" 攻击的可执行文件</a>
  - <a href="#3.2.3">`hex2raw` - 帮手程序</a>
  - <a href="#3.2.4">`farm.c` - "小工具" 仓库</a>
  - <a href="#3.2.5">其他</a>
- <a href="#3.3">实验思路与总结</a>
  - <a href="#3.3.1">`ctarget` - level#1</a>
  - <a href="#3.3.2">`ctarget` - level#2</a>
  - <a href="#3.3.3">`ctarget` - level#3</a>
  - <a href="#3.3.4">`rtarget` - level#2</a>
  - <a href="#3.3.5">`rtarget` - level#3</a>
- <a href="#3.4">相关资料</a>

</details>
<details><summary><a href="#4">GDB 入门</a></summary>
</details>
<details><summary><a href="#5">make 入门</a></summary>
</details>
<details><summary><a href="#6">待办</a></summary>
</details>

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

## <a id="1"></a>1. Data Lab

### <a id="1.1"></a>实验目的

补全文件 `bits.c` 中所包含的全部 13 个 puzzle 的函数主体.

### <a id="1.2"></a>实验框架

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

#### <a id="1.2.1"></a>整数 puzzle

- 仅允许使用函数形参和定义局部变量, 不允许定义全局变量.
- 仅允许使用白话代码 (straightline code) 完成每个函数, 不允许使用任何语句, 例如循环或条件语句等.
- 仅允许使用一元运算符 `!`, `˜`, 以及二元运算符 `&`, `ˆ`, `|`, `+`, `<<`, `>>`, **部分 puzzle 会进一步限制可用运算符的范围**.
- 仅允许使用 0 到 255 范围内的常数.
- 仅允许使用 `int` 类型.
- 不允许使用宏定义.
- 不允许定义新函数和调用任何函数.
- 不允许使用强制类型转换.
- 可以假设机器使用 32 位补码整数, 使用算数右移, 并且当位移量小于 0 或大于 31 时会导致无法预测的行为.

#### <a id="1.2.2"></a>浮点数 puzzle

- 仅允许使用条件或循环语句.
- 仅允许使用 `int` 和 `unsigned` 数据类型.
- 允许使用任意 `int` 和 `unsigned` 类型的常数.
- 允许使用任意算数, 逻辑和比较运算符.
- 不允许使用宏定义.
- 不允许定义新函数和调用任何函数.
- 不允许使用强制类型转换.

#### <a id="1.2.3"></a>`btest` - 正确性检查工具

- `btest` 需要与 `bits.c` 一同编译, 用于检测所实现函数的正确性, 基本命令 (测试所有函数):

  ```bash
  ./btest
  ```

- 每次修改 `bits.c` 之后均需要重新编译 `btest`.

- 可以使用选项 `-f` 来单独指定一个函数进行测试, 此外还可以通过选项 `-1`, `-2` 和 `-3` 来指定传入的实参, 命令:

  ```bash
  ./btest -f bitXor -1 4 -2 5
  ```

#### <a id="1.2.4"></a>`dlc` - 合法性检查工具

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

#### <a id="1.2.5"></a>`driver.pl` - 最终评分工具

- `driver.pl` 用于为全部实验代码进行最终评分.

#### <a id="1.2.6"></a>其他

- 帮手程序 `ishow` 和 `fshow` 分别用于展示相应的二进制模式. 两个程序均只输入一个参数, 该参数可以是表示十进制数, 十六进制数和浮点数的字符串. 使用 `make` 对两个程序进行编译.

- 如果要使用 `printf` 语句进行 debug, 直接使用即可, 无需包含 `stdio.h` 头文件 (可能 gcc 会报 warning, 忽略即可).

- 具体打分细则见 handout.

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="1.3"></a>实验思路与总结

#### <a id="1.3.1"></a>#01 `bitXor` - 使用 `~` 和 `&` 实现 `^`

设两个比特 $p$ 和 $q$ 以及对应的命题 $P$ 和 $Q$, 在命题逻辑 (propositional logic) 下, 由德摩根定律 (De Morgan's laws), 异或运算可表示为

$$
\begin{equation}
P \oplus Q = (P \wedge \neg Q) \vee (\neg P \wedge Q) = \neg (\neg (P \wedge \neg Q) \wedge \neg (\neg P \wedge Q)), \tag{1} \label{1.3.1.bitXor-1}
\end{equation}
$$

将 $\eqref{1.3.1.bitXor-1}$ 式翻译至布尔代数下即为

```text
p ^ q = ~((~(p & (~q))) & (~((~p) & q))).
```

#### <a id="1.3.2"></a>#02 `tmin` - 返回 $TMin_w$

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

#### <a id="1.3.3"></a>#03 `isTmax` - 判断是否为 $TMax_w$

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
x + 1 \equiv 2^{w} - 1 - x \ \ (\text{mod}\ \  2^{w}),
\end{equation}
$$

解得 $x = 2^{w - 1} - 1$ 或 $x = 2^{w} - 1$. 因此 $x$ 为 $TMax_w$ 的位模式当且仅当其满足如下约束:

$$
\begin{numcases}{}
x + 1 \equiv 2^{w} - 1 - x \ \ (\text{mod}\ \  2^{w}), \tag{2} \label{1.3.3.isTmax-1}\\
x \neq 2^w - 1. \tag{3} \label{1.3.3.isTmax-2}
\end{numcases}
$$

约束 $\eqref{1.3.3.isTmax-1}$ 左侧即为 `x + 1`, 右侧可表示为 `~x`. 由于整数 puzzle 同样限制了比较运算符的使用, 为了判断两个二进制位模式是否相同可以使用异或 `^` 与逻辑非 `!` 间接实现. 实际上两个位模式的异或结果为全 0 当且仅当这两个位模式相同, 而逻辑非能够方便的将异或结果转化为布尔值, 因此判断两个位模式 `x` 和 `y` 是否相同的表达式为 `!(x ^ y)`. 于是约束 $\eqref{1.3.3.isTmax-1}$ 可表示为:

```c
int x_plus_one = x + 1;
int x_complement = ~x;
int constraint_1 = !(x_plus_one ^ x_complement);
```

约束 $\eqref{1.3.3.isTmax-2}$ 成立当且仅当 $x$ 取反不为全 0. 为了将取反结果转化为布尔值, 需要使用逻辑非 `!` 进行转化:

```c
int constraint_2 = !!(~x);
```

使用连续两个 `!` 是因为单次转化得到的布尔值为原值的相反结果.

结合约束 $\eqref{1.3.3.isTmax-1}, \eqref{1.3.3.isTmax-2}$ 得到最终表达式:

```c
int is_tmax = constraint_1 & constraint_2;
```

#### <a id="1.3.4"></a>#04 `allOddBits` - 判断一个二进制模式的所有奇数位是否均为 1

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

#### <a id="1.3.5"></a>#05 `negate` - 返回一个补码整数的相反数

最终实现为取反加一, 原理待补充.

#### <a id="1.3.6"></a>#06 `isAsciiDigit`

通过将两个模式相减并检查结果是否为负数来实现比较运算.

#### <a id="1.3.7"></a>#07 `conditional`

首先判断是否为零, 若为零则选择全 0 模式, 否则取反得到全 1 模式; 再使用按位与 `&` 进行条件选择.

#### <a id="1.3.8"></a>#08 `isLessOrEqual`

先检查两个数是否同号, 若同号则通过相减是否得到负数进行判断, 若异号直接检查哪一个数为正哪一个数为负即可.

#### <a id="1.3.9"></a>#09 `logicalNeg`

逻辑非等价于检查取反后的结果是否为全 0.

#### <a id="1.3.10"></a>#10 `howManyBits`

举例:

- `0x0001101` 需要 5 位, 具体为 `0x00 01101`, 即从左往右找最长连续相同的子串, 然后退一格断开. 思路是先将模式与其自身的右移一位结果进行按位异或, 得到每两位之间是否相同的结果, 最终答案的大小等于按位异或得到的模式中从左往右最长全等子串的长度关于 32 的补集. 注意由于至少需要 1 位进行表示, 所以需要对异或的结果至少将其与 1 进行按位或. 最后使用二分法找到该子串的补集的长度.

#### <a id="1.3.11"></a>#11 `floatScale2`

分情况讨论, 如果是特殊值, 直接返回; 如果是非规格化数, 直接对尾数部分进行左移; 否则就是规格化数, 对阶码加一即可. 注意如果加一之后变为特殊值还需要对尾数进行清空.

#### <a id="1.3.12"></a>#12 `floatFloat2Int`

分情况讨论, 如果阶码大于整型范围则直接返回 (由于返回值恰好为 -2147483648, 阶码真实值等于 31 的情况也可归纳至其中); 如果小于 0 则返回 0; 否则对尾数直接进行截断即可.

#### <a id="1.3.13"></a>#13 `floatPower2`

分情况讨论, 如果指数大于阶码范围则直接返回, 如果小于阶码范围则返回 0, 然后分别处理位于规格化范围和非规格化范围两种情况即可.

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="1.4"></a>相关资料

#### <a id="1.4.1"></a>命题逻辑

- [维基百科: 命题逻辑](https://en.wikipedia.org/wiki/Propositional_calculus)

## <a id="2"></a>2. Bomb Lab

### <a id="2.1"></a>实验目的

对可执行文件 `bomb` 进行调试, 深入其汇编代码, 依次推断出 6 个密码并通过相应 6 个阶段的考验. 除了这 6 个考验以外还存在第七个隐藏考验, 因此实际上共有 7 个阶段的考验以及对应的密码, 具体见下方小节.

### <a id="2.2"></a>实验框架

#### <a id="2.2.1"></a>`bomb` - 待调试文件主体

`bomb` 为可执行文件, 需要对其进行调试并破解得到 6 个密码 (加上隐藏考验总共为 7 个密码).

#### <a id="2.2.2"></a>`bomb.c` - 描述主体构成

`bomb.c` 中使用 C 程序描述了炸弹 bomb 的主体构成, 其中 `support.h` 与 `phases.h` 猜测是实现炸弹本体的代码的头文件.

观察 `bomb.c` 可得到 bomb 的构成如下:

- 首先检查命令行中是否给出文件路径, 并打开标准输入/给定文件.
- 初始化炸弹: `initialize_bomb();`
- 连续执行 6 个阶段, 每个阶段均为读入一行输入, 将输入用于解密, 若解密成功则可顺序进入下一阶段, 若 6 个阶段均解密成功则函数退出 (炸弹解除).
- 此外还可选择挑战第七个隐藏考验.

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="2.3"></a>实验思路与总结

目录结构:

- 可执行文件 `bomb` 的反汇编内容见文件[objdump_bomb.txt](2-bomb-lab/objdump_bomb.txt).
- 下列所有函数的机器指令的具体解释见目录[disas-output](2-bomb-lab/disas-output), 某些特定函数所需要的额外信息 (例如跳转表或全局字符串等) 将会在对应的 `disas_xxx.txt` 文件开头列出.
- 测试通过的 7 个密码见文件[password.txt](2-bomb-lab/password.txt).

#### <a id="2.3.1"></a>`sig_handler`

对炸弹拆除进度进行安全重置 `:-)`.

#### <a id="2.3.2"></a>`initialize_bomb`

函数 `initialize_bomb` 使用 C 库函数 `signal` 为信号 `SIGINT` 安装处理程序 `sig_handler`.

#### <a id="2.3.3"></a>`string_length`

函数 `string_length` 返回所传入字符串的长度.

#### <a id="2.3.4"></a>`strings_not_equal`

函数 `strings_not_equal` 检查传入的 2 个字符串是否相同.

#### <a id="2.3.5"></a>`explode_bomb`

函数 `explode_bomb` 输出 "炸弹拆除失败" 的提示信息并终止整个程序.

#### <a id="2.3.6"></a>`phase_defused`

函数 `phase_defused` 首先检查是否已经通过前 6 个阶段, 若仍未通过前 6 个阶段则直接返回; 若已经通过前 6 个阶段则尝试从第四个阶段的密码中读取开启隐藏阶段的口令, 若口令读取成功并且与一个已知字符串相同则进入隐藏阶段; 否则输出 "成功拆除炸弹" 的提示信息并返回.

#### <a id="2.3.7"></a>`read_six_numbers`

函数 `read_six_numbers` 从传入的字符串中读取 6 个整数.

#### <a id="2.3.8"></a>`func4`

函数 `func4` 在一个给定范围内对目标值进行二分查找, 并在递归过程中按一定规则产生最终的返回值.

- 注: 在二分查找时还 `func4` 还实现了补码下整数除以 2 的幂的算法.

#### <a id="2.3.9"></a>`fun7`

函数 `fun7` 在一棵简单的二叉搜索树中查找给定整数, 并在递归过程中按一定规则产生最终的返回值.

#### <a id="2.3.10"></a>`phase_1`

函数 `phase_1` 通过调用函数 `strings_not_equal` 来比较密码是否与一个已知字符串相同.

#### <a id="2.3.11"></a>`phase_2`

函数 `phase_2` 首先调用函数 `read_six_numbers` 从密码中读入 6 个整数, 然后检查这 6 个整数是否满足特定规则.

#### <a id="2.3.12"></a>`phase_3`

函数 `phase_3` 首先调用函数 `sscanf` 读入 2 个整数, 然后实现了一个简单的 C 语言 `switch` 语句来检查这 2 个数字是否满足特定规则.

#### <a id="2.3.13"></a>`phase_4`

函数 `phase_4` 首先调用函数 `sscanf` 读入 2 个整数, 然后将第一个整数传入函数 `func4` 并得到 1 个整数结果, 最后检查所返回的整数结果以及第二个整数是否满足特定规则.

#### <a id="2.3.14"></a>`phase_5`

函数 `phase_5` 将输入的密码字符串看作是一串整数数组, 通过将数组中的整数作为下标索引某个字母表来将整数数组映射为长度相等的新字符串, 最后比较新字符串是否与一个已知字符串相同.

#### <a id="2.3.15"></a>`phase_6`

函数 `phase_6` 首先调用函数 `read_six_numbers` 从密码中读取 6 个整数, 并使用这 6 个整数从某个全局链表中依次挑选出 6 个结点并连接形成一个新的链表, 最后检查新的链表中的值是否满足特定规则.

#### <a id="2.3.16"></a>`secret_phase`

函数 `secret_phase` 首先调用 `read_line` 函数读入第七个密码, 然后使用 C 库函数 `strtol` 将其转换为整数, 将该整数传入函数 `fun7` 得到结果, 最后检查该结果是否满足特定规则.

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="2.4"></a>相关资料

#### <a id="2.4.1"></a>GDB

- [Beej's Quick Guide to GDB](https://beej.us/guide/bggdb/)
- [GDB: The GNU Project Debugger](https://www.sourceware.org/gdb/)
- [gdbnotes-x86-64.pdf](http://csapp.cs.cmu.edu/3e/docs/gdbnotes-x86-64.pdf)
- stackoverflow: [How to highlight and color gdb output during interactive debugging?](https://stackoverflow.com/questions/209534/how-to-highlight-and-color-gdb-output-during-interactive-debugging)
  - GitHub: [gdb-dashboard](https://github.com/cyrus-and/gdb-dashboard)

#### <a id="2.4.2"></a>Linux 命令

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

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

## <a id="3"></a>3. Attack Lab

### <a id="3.1"></a>实验目的

对两个可执行文件 `ctarget` 和 `rtarget` 成功实施共计 5 次攻击, 攻击类型包括 "代码注入" 攻击和 "返回导向编程" 攻击两种.

### <a id="3.2"></a>实验框架

#### <a id="3.2.1"></a>`ctarget` - 易受 "代码注入" 攻击的可执行文件

#### <a id="3.2.2"></a>`rtarget` - 易受 "返回导向编程" 攻击的可执行文件

#### <a id="3.2.3"></a>`hex2raw` - 帮手程序

生成攻击字符串的帮手程序

#### <a id="3.2.4"></a>`farm.c` - "小工具" 仓库

用于帮助实施 "返回导向编程" 攻击的 "小工具" 函数的源代码文件

#### <a id="3.2.5"></a>其他

- 本实验可参考教材的 3.10.3 到 3.10.4 小节.
- 不允许使用攻击来绕过 validation 程序. 任何攻击字符串所导向的地址必须为下列地址中的其中一个:
  - 函数 `touch1`, `touch2`, 和 `touch3` 的起始地址.
  - 注入代码的起始地址.
  - 小工具仓库中的任意函数的起始地址.
- You may only construct gadgets from file `rtarget` with addresses ranging between those for functions `start_farm` and `end_farm`.

> [!CAUTION]
>
> 在运行 `ctarget` 时可能会报如下错误:
>
> ```text
> Program received signal SIGSEGV, Segmentation fault.
> 0x00007ffff7dfe0d0 in __vfprintf_internal (s=0x7ffff7fa4780 <_IO_2_1_stdout_>, format=0x4032b4 "Type string:", ap=ap@entry=0x5561dbd8, mode_flags=mode_flags@entry=2) at ./stdio-common/vfprintf-internal.c:1244
> ```
>
> 对应位置的汇编代码为:
>
> ```text
>  0x00007ffff7dfe0c0  __vfprintf_internal+144 movdqu (%rax),%xmm1
>  0x00007ffff7dfe0c4  __vfprintf_internal+148 movups %xmm1,0x118(%rsp)
>  0x00007ffff7dfe0cc  __vfprintf_internal+156 mov    0x10(%rax),%rax
>  0x00007ffff7dfe0d0  __vfprintf_internal+160 movaps %xmm1,0x10(%rsp)
>  0x00007ffff7dfe0d5  __vfprintf_internal+165 mov    %rax,0x128(%rsp)
> ```
>
> 调试过程具体见[debug.md](3-attack-lab/debug.md).
>
> 查阅网络资料后得知报错的直接原因是汇编代码
>
> ```text
> 0x00007ffff7dfe0d0  __vfprintf_internal+160 movaps %xmm1,0x10(%rsp)
> ```
>
> 中的指令 `movaps` 要求目的地址 `0x10(%rsp)` 必须与 16 字节对齐, 否则会导致段错误. 而报错的根本原因可能是 `ctarget` 程序编译时的环境和当前执行的环境不同, 导致调用 `printf` 函数时无法保证 `movaps` 指令的目的地址总是与 16 字节对齐.
>
> 由于上述问题, 在运行 `ctarget` 与 `rtarget` 的过程中要尽可能避免程序调用 `printf` 函数, 例如在运行前应使用选项 `-i <input-file>` 指定输入文件路径, 因为若使用标准输入会导致程序调用 `printf` 函数输出提示信息 "Type string:" 从而造成段错误; 此外在执行完攻击代码结束并返回时也会调用 `printf` 函数输出攻击成功的提示信息, 尽管此时无法避免调用 `printf` 函数, 但可以通过在攻击代码中增加对齐 `$rsp` 的功能的方法成功调用 `printf` 函数.

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="3.3"></a>实验思路与总结

#### <a id="3.3.1"></a>`ctarget` - level#1

目标: 利用栈溢出漏洞使函数 `test` 在返回时跳转至函数 `touch1` 处.

- 通过反汇编 `test` 函数可知 `test` 函数的工作是调用 `getbuf` 函数, 若 `getbuf` 成功返回则打印字符串 "No exploit.  Getbuf returned 0x%x\n". 因此攻击目标调整为使函数 `getbuf` 返回至 `touch1` 处.
- 通过反汇编 `getbuf` 函数可知字符串的缓冲区的大小固定为 `0x28` 字节, 紧随缓冲区之后的 8 个字节内存存放的即为函数 `getbuf` 的返回地址.
- 通过反汇编代码内容可知函数 `touch1` 的入口地址为 `0x00000000004017c0`.

因此最终的攻击策略是将攻击字符串的前 40 字节设置为任意内容用于填充缓冲区的 40 字节, 然后将接下来的 8 个字节设置为函数 `touch1` 的入口地址. 具体见文件[c1.txt](3-attack-lab/c1.txt).

#### <a id="3.3.2"></a>`ctarget` - level#2

目标: 利用栈溢出漏洞使函数 `test` 在返回时跳转**并调用**函数 `touch2`.

> [!NOTE]
> 在使用攻击代码进行函数调用时不需要使用 `jmp` 或 `call` 指令, 因为这些指令可能要求使用偏移地址而非绝对地址, 而偏移地址较为难以计算. 若要实现函数跳转, 直接使用 `ret` 指令配合绝对地址即可, 由于函数 `touch2` 并不包含返回语句 (其使用 C 库函数 `exit()` 直接终止程序的运行), 因此不需要考虑是否需要使用 `call` 指令压入返回地址等问题.

- 由于需要调用函数 `touch2` 并传入参数, 直接使用地址跳转至特定函数的方法此时便不再适用, 因为并不存在现有的函数实现 "跳转至函数 `touch2` 并传入给定参数" 的功能, 因此需要在攻击字符串中直接注入攻击代码.
- 若要使函数 `getbuf` 执行攻击字符串中包含的攻击代码, 需要使其原地返回至攻击代码的入口地址 (该地址仍位于字符串中, 因此称之为 "原地返回"). 通过反汇编代码内容可知进入 `getbuf` 时 (同时在分配缓冲区局部变量前) 寄存器 `$rsp` 的值为 `0x000000005561dca0`, 因此攻击字符串中的返回地址应设置为 `$rsp + 8` 即 `0x000000005561dca8`.
- 若要调用函数 `touch2`, 汇编代码首先需要将 cookie 作为参数传入 `touch2`, 然后通过移动 `$rsp` 至存储有函数 `touch2` 的入口地址的内存上并执行 `ret` 指令的方法调用函数 `touch2`. 函数 `touch2` 的入口地址为 `0x00000000004017ec`.
- 此外还需要对齐指针的 8 字节以及对齐前述的 `movaps` 指令的 16 字节, 因此需要额外注意移动 `$rsp` 的距离的设置.

综上可知攻击策略为 40 字节 padding + 8 字节攻击代码入口地址 + 攻击代码 (传入参数, 移动 `$rsp`, 执行 `ret`) + 16 字节对齐 padding + 函数 `touch2` 的入口地址. 具体见文件[c2.txt](3-attack-lab/c2.txt).

#### <a id="3.3.3"></a>`ctarget` - level#3

#### <a id="3.3.4"></a>`rtarget` - level#2

#### <a id="3.3.5"></a>`rtarget` - level#3

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

### <a id="3.4"></a>相关资料

## <a id="4"></a>GDB 入门

**进入 gdb 环境**:

- `gdb` : 直接启动环境
- `gdb <executable>` : 指定可执行文件并启动环境

> [!NOTE]
> 常用的是 `gdb <executable>` 命令, 以直接载入可执行文件.

**设置断点**:

- `break <function-name>` : 将断点设置在函数入口处
- `break *<memory-address>` : 将断点设置在指定内存位置

> [!NOTE]
> `<function-name>` 可以是函数名称, 例如 `my_func`; 而 `<memory-address>` 可以是某个具体的内存地址, 例如 `0x4328afe`, 使用内存地址设置断点时务必前缀一个星号 `*`.

**禁用和启用断点**:

- `disable n` : 暂时禁用断点
- `enable n` : 重新启用断点

> [!NOTE]
> `n` 指 gdb 为该断点分配的序号. gdb 在设置断点时会为每一个断点分配一个正整数序号, 从 1 开始从小到大分配. 可以直接使用序号对断点进行操作.

**删除断点**:

- `delete n` : 删除指定断点
- `delete` : 删除当前设置的所有断点

**运行**:

- `run <arg1> <arg2> <arg3>` : 传入参数并运行可执行文件
- `stepi` : 单步进入 (以指令为单位)
- `stepi n` : 连续执行 `n` 次单步进入 (以指令为单位)
- `nexti` : 单步跳过 (以指令为单位)
- `nexti n` : 连续执行 `n` 次单步跳过 (以指令为单位)
- `step` : 单步进入 (以 C 语句为单位)
- `continue` : 继续执行, 直至遇见下一个断点
- `finish` : 继续执行, 直至当前函数返回
- `kill` : 终止当前运行的可执行文件
- `exit` : 退出 gdb

**反汇编函数**:

- `disas` : 反汇编当前函数
- `disas <function-name>` : 反汇编指定函数

**打印数据**:

- `print (char *) <memory-address>` : 打印字符串
- `print *(int *) <memory-address>` : 打印整数

> [!NOTE]
> `<memory-address>` 可以是一般的内存地址, 例如 `0x423fabd`, 也可以是某个表达式, 例如 `($rip + 0x423fabd)`.

**打印二进制内容**:

- `x/[n][s][f] <memory-address>` : 打印指定内存地址附近的二进制内容
- `x/[n][s][f] <function-name>` : 打印函数入口附近的二进制内容
- `x/[n]i <function-name>` : 打印函数入口附近的若干行机器指令
- `x/s <memory-address>` : 打印指定地址处的字符串
- `x/a <memory-address>` : 打印指定地址处所存储的地址 (以当前位置到前一个最近的全局符号的偏移的形式打印)

> [!NOTE]
> 命令 `x` 以字节块的形式组织并打印二进制内容, 上述命令中的 `[n]`, `[s]` 和 `[f]` 分别代表要显示的字节块的 "数量 (number)", "大小 (size)" 以及 "格式 (format)". 其中:
>
> - 数量 `[n]` 为正整数;
> - 大小 `[s]` 可以是:
>   - `b` : 字节
>   - `h` : 双字节
>   - `w` : 四字节 (32 bits)
>   - `g` : 八字节 (64 bits)
> - 格式 `[f]` 可以是:
>   - `d` : 十进制
>   - `x` : 十六进制
>   - `o` : 八进制
>
> 常见例子:
>
> - `x/2gx $rsp` : 以十六进制格式打印从 `$rsp` 中存储的地址处开始的 2 个八字节块

**打印环境信息**:

- `info breakpoints` : 打印所有断点
- `info registers` : 打印所有寄存器内容

<div align="right"><b><a href="#toc">返回顶部↑</a></b></div>

## <a id="5"></a>make 入门

## <a id="6"></a>待办

- [x] 添加 gdb 入门小节
- [ ] 添加 make 入门小节
- [ ] Data Lab 的第二个函数的原理应该使用 $U2T_w$ 推导, 而不是 $B2T_w$
- [ ] 对 Data Lab 所有函数的思路进行补充
- [ ] 对 Bomb Lab 的所有反汇编代码的原理体现在 README.md 中, 必要时可附加图像
- [ ] 使用文件树对每个 Lab 下的目录结构进行展示
- [ ] 在 "实验目的" 小节中添加 "知识点" 环节
- [ ] 对 Attack Lab 添加对使用 gdb + objdump + hex2raw 将汇编代码转换为攻击字符串的说明
