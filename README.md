# 兎時計

ESP32でステッピングモーターを制御して動かす壁掛け時計。
兎と亀がモチーフ

## ソフトウェア説明

* ESP-IDF v5.2 (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

## Bluetooth Low Energyクライアント

リポジトリをPublicにした際に Github Pageを設定

## 回路図

![Schematic](docs/rabbit_clock_scheme.svg)

## ハードウェア設計図

[図面(FreeCAD)](docs/rabbit_clock_drawing.FCStd)

## 開発メモ


### ステッピングモーター駆動
esp-idfにPWM(ledc mcpwm)が存在するが、パルス数調整ができないためステッピングモーター制御には利用できない。
ウェブを彷徨うとGPIO + nopを組み合わせる方法があり、動作はするもののCPUリソースを使い果たすため2台同時制御ができないことから、この方法も却下。
General Purpose Timerの割り込みでGPIOをOn/Offする仕組みで実装しました。オシロスコープで確認したらちゃんと精度が出ている。

今回はAT2100 モータードライバーを利用しました。
設定を変更することでA4988やTMC2208でも動作しますが、これらはモーターからの音がうるさく時計用途には微妙。

ステッピングモーター制御については以下の記事を参考にした。日本語の記事は怪しい内容の記載があるなど危険…
https://howtomechatronics.com/tutorials/arduino/how-to-control-stepper-motor-with-a4988-driver-and-arduino/

### Bluetooth Low Energy対応
ボタンを増やしたくなかったので、Bluetooth Low Energyから設定・デモ動作を行えるよう対応しました。

Bluetooth Low Energyをはじめよう (Make:PROJECTS) を購入したが、Webにある記事で十分だった。
https://www.amazon.co.jp/dp/4873117135

Bluetooth Low Energyについては以下記事で学びました。
https://www.musen-connect.co.jp/blog/course/trial-production/ble-beginner-1/

クライアントはBlueJellyを利用
https://monomonotech.jp/kurage/webbluetooth/getting_started.html
https://github.com/electricbaka/bluejelly

### その他
BOOTボタンを押しながらENボタンを押す

idf.py flash -p /dev/cu.usbserial-110 -b 115200
idf.py monitor -p /dev/cu.usbserial-110 -b 115200

Motor
A+ (Blue) A-(Red)  B+(Grren) B-(Black)

Black Green Blue Red
B- B+  A+ A-
2B 2A 1A 1B
