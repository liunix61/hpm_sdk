# HPM6E00EVK开发板

## 概述

HPM6E00是一款运行于600MHz的双核MCU。它有一个2MB的连续片上ram。HPM6E00EVK为HPM6E00系列微控制器的特色外设提供了一系列接口，包括ADC输入SMA接口、SDM输入SMA接口，电机控制接口、ABZ接口、RS485/422接口、Ethernet接口、两个EtherCAT接口、一个PPI/FEMC接口和一个树莓派接口。HPM6E00EVK集成了一个板载调试器。

 ![hpm6e00evk](doc/hpm6e00evk.png "hpm6e00evk")

## 板上硬件资源

- HPM6E00 微控制器 (主频600Mhz, 1M片上SRAM)
- 板载存储
  - 16MB Quad SPI NOR Flash
- 以太网
  - 1000 Mbits PHY
- EtherCAT
  - 2 端口
- USB
  - USB type C (USB 2.0 OTG) connector x1
- 音频
  - Line in
  - Mic
  - Speaker
  - DAO
- 电机
  - RS422
  - RS485
- 模拟采样
  - NSI1306W25
- 其他
  - RGB LED
  - CAN
- 注意
  - **当需要使用FEMC(SDRAM)或PPI外设时，请在PPI/FEMC接口插入相应的扩展板**
    - HPM6E00EVK标配的扩展板如下，有1个16bits的SDRAM（FEMC访问）和1个并口ADC（PPI访问），供评估使用。
      ![hpm6e00evk_ext](doc/hpm6e00evk_ext.png "hpm6e00evk_ext")
    - FEMC/PPI接口具有很高的灵活性，若需要评估其他并口设备，例如FPGA、ASYNC SRAM等，可自行设计扩展板或联系我们。
  - **当需要使用SDM外设和板上AD采样芯片(NSI1306W25)时，请连接跳帽JP4、JP5、JP6，断开J3**
  - **板级SEI接口CLK引脚与SDM采样芯片CLK引脚相同，不能同时使用。请使用SDM采样芯片时，断开J3；使用SEI接口时，断开JP6。**

## 拨码开关 SW2

- Bit 1，2控制启动模式

| Bit[2:1] | 功能描述                |
| -------- | ----------------------- |
| OFF, OFF | Quad SPI NOR flash 启动 |
| OFF, ON  | eMMC启动                |
| ON, OFF  | 在系统编程              |

(lab_hpm6e00_evk_board)=

## 按键

(**lab_hpm6e00_evk_board_buttons**)=

| 名称         | 功能                                  |
| ------------ | ------------------------------------- |
| PBUTN (KEYA) | GPIO 按键                             |
| PBUTN2 (KEYB) | GPIO 按键2                             |
| WBUTN (WKUP) | WAKE UP 按键                          |
| RESETN (RESET) | Reset 按键                            |

## 插件

- SEI CLK 选择

| 功能      | 位置   | 说明 |
| --------- | ------ |------|
| SEI.CLK选择  | J3 | Master侧，选择CLKO；Slave侧，选择CLKI |

- 调试器接口选择

| 功能      | 位置   | 说明 |
| --------- | ------ |------|
| 调试器选择  | J17 | 全部连接：使用板载ft2232，全部断开：使用标准JTAG接口 |

- PPI/FEMC接口

| 功能      | 位置   | 说明 |
| --------- | ------ |------|
| PPI/FEMC接口 | CN1 | 接PPI或FEMC扩展板 |

## 引脚描述

- PUART串口引脚
PUART用于低功耗测试，例如唤醒等。

| 功能 | 引脚 |  位置 |
| ---- | ----- | ------|
| PUART.TX | PY0 | P5[8] |
| PUART.RX | PY1 | P5[10] |

- UART0串口引脚：

 UART0用于调试控制台串口。

| 功能     | 引脚 |   位置     |
| -------- | ---- |  --------- |
| UART0.TX | PA00 | DEBUGUART0 |
| UART0.RX | PA01 | DEBUGUART0 |

- UART1串口引脚：

 UART1用于一些使用UART的功能测试，例如MICROROS_UART，USB_CDC_ACM_UART, MODBUS_RTU, lin等。

| 功能     | 引脚 |   位置     | 位置     |
| -------- | ---- |  --------- | ------ |
| UART1.TX | PY07 | P5[5]  |
| UART1.RX | PY06 | P5[3] |
| UART1.break | PF27 | J4[6] | 产生uart break信号|

- CAN 接口

| 功能      | 位置   |
| --------- | ------ |
| CAN_H  | J7[0] |
| CAN_L  | J7[2] |

- 音频接口

| 功能      | 位置   |
| --------- | ------ |
| 扬声器左声道  | J11 |
| 扬声器右声道  | J12 |
| 3.5毫米接口  | J10 |
| DAO接口  | J5 |

- ADC 接口

| 功能      | 位置   |
| --------- | ------ |
| ADC输入  | J4[2] |
| SDM ADC输入  | J13 |

- ACMP

| 功能      | 位置   |
| --------- | -------- |
| CMP0.INN4 | J4[18]   |

- 正交旋转编码器接口

| 功能      | 位置   |
| --------- | ------ |
| QEI.A / HALL.U  | J4[1] |
| QEI.B / HALL.V  | J4[3] |
| QEI.Z / HALL.W  | J4[5] |
| QEO.A  | J4[26] |
| QEO.B  | J4[24] |
| QEO.Z  | J4[22] |

- PWM 输出接口

| 功能      | 位置   |
| --------- | ------ |
| PWM.WL  | J4[12] |
| PWM.WH  | J4[11] |
| PWM.VL  | J4[10] |
| PWM.VH  | J4[9] |
| PWM.UL  | J4[8] |
| PWM.UH  | J4[7] |

- SEI 接口

| 功能      | 位置   | 说明 |
| --------- | ------ |------|
| SEI.CLK_IN_P  | J4[29] |主机模式下时钟差分输出P|
| SEI.CLK_IN_N  | J4[31] |主机模式下时钟差分输出N|
| SEI.CLK_OUT_P  | J4[27] |从机模式下时钟差分输入P|
| SEI.CLK_OUT_N  | J4[25] |从机模式下时钟差分输入N|
| SEI.DATA_P  | J4[23] |数据差分信号线P|
| SEI.DATA_N  | J4[21] |数据差分信号线N|

- SEI CLK选择

| 功能      | 位置   | 说明 |
| --------- | ------ |------|
| SEI.CLK选择  | J3 | Master侧，选择CLKO；Slave侧，选择CLKI |

- QEIV2 Sin/Cos引脚

| 功能       | 位置   |   备注  |
| ---------- | ------ | ------  |
| ADC2.INA11  | J4[15] | ADC_IW (Cos) |
| ADC0.INA14  | J4[13] | ADC_IU (Sin) |

- RDC引脚

| 功能      | evk板位置 | RDC板位置 |
| --------- | ------ | ------ |
| RDC.PWM   | J4[7]  | J2[7]  |
| RDC.ADC0  | J4[13] | J2[13]  |
| RDC.ADC1  | J4[14] | J2[14]  |
| GND       | J4[32] | J2[17]  |

- PLB引脚
| 功能 | 位置   |
| PLB.PULSE_OUT  | J4[5] |

- Tamper 接口

| 功能     | 引脚   | 位置   |   模式   |
|----------|--------|--------|---------|
| TAMP.04  | PZ04   | P5[18] | 主动模式 |
| TAMP.05  | PZ05   | P5[33] | 主动模式 |
| TAMP.03  | PZ03   | P5[7]  | 被动模式 |

- LOBS 触发信号

| 引脚   | 位置   |
|--------|--------|
| PF26   | P5[23] |

- GPTMR引脚

| 功能          | 位置   |  备注 |
| ------------- | ----- | ------ |
| GPTMR4.CAPT_0 | J4[3] |
| GPTMR4.COMP_0 | J4[1] | SPI模拟I2S的BLCK |
| GPTMR0.COMP_0 | J4[26] | SPI模拟I2S的LRCK |
| GPTMR5.COMP_2 | J4[5] | SPI模拟I2S的MCLK |

- SPI模拟I2S CS引脚

| 功能 | 位置   |  备注 |
| ---- | ----- | ------|
| PE6  | J4[24] | 控制SPI从机CS的引脚 |

- SPI引脚：

| 功能      | 引脚 | 位置    |
| --------- | ---- | ------- |
| SPI7.CSN  | PF27 | J4[6]  |
| SPI7.SCLK | PF26 | P5[23]  |
| SPI7.MISO | PF28 | P5[21]  |
| SPI7.MOSI | PF29 | P5[19]  |

- I2C引脚：

| 功能     | 位置   |
| -------- | ------ |
| I2C0.SCL | P5[28] |
| I2C0.SDA | P5[27] |
