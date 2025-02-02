# QEOV2_WAVE_OUT

## 概述

QEO_WAVE_OUT示例工程展示QEO根据得到的位置信息，生成弦波, 可以通过PWM输出占空比可变的PWM。

## 注意

QEO支持输出3路弦波信号。

## 运行现象

- 当工程正确运行后，串口终端会输出如下信息：
```console
QEO DAC wave example
QEO generate wave with sofeware inject postion
qeo wave0 output:
65535
65526
65496
65447
65378
65290
65182
65054
64907
64740
64554
64349
64126
63883
63621
63342
63043
62726
...
qeo wave1 output:
16384
15696
15015
14344
13684
13036
12400
11777
11165
10567
9982
9410
8854
8311
7783
7270
...
qeo wave2 output:
16382
17081
17792
18512
19240
19976
20721
21472
22231
22995
23766
24542
25323
26108
26897
27690
28486
...
```
串口输出软件注入位置模式下，3相弦波的数值，使用工具(excel等)，将数据拟合得到波形。
![](doc/qeo_dac_1.png)


