# GBA cart info

https://files.darkfader.net//gba/files/cartridge.txt

# Information on dumping that is not exactly right

http://douevenknow.us/post/68126856498/arduino-based-gba-rom-dumper-part-1

> - Select dump start address and lay it across pins AD0-AD23. In our case we want to start at 0 so leaving all those pins LOW will work for our needs
> - Pulse CS pin to latch in our address
> - Pulse RD pin and retrieve 16 bit result
> - Repeat step 3 until end of ROM

https://github.com/shinyquagsire23/GBA-GB-ROMDumper/blob/master/GBADump.ino
```C++
void latchAddress(unsigned long address)
{
  DDRC = 0xFF;
  DDRL = 0xFF;
  DDRA = 0xFF;
  PORTC = address & 0xFF;
  PORTL = (address & 0xFF00) >> 8;
  PORTA = (address & 0xFF0000) >> 16;
  digitalWrite(CS, HIGH);  
  delayMicroseconds(10);  
  digitalWrite(CS, LOW);   
  PORTC = 0x0;
  PORTL = 0x0;
  PORTA = 0x0;
  DDRC = 0x0;
  DDRL = 0x0;
  DDRA = 0x0;
}

void strobeRS(int del)
{
  digitalWrite(RD, LOW); 
  delayMicroseconds(del);
  digitalWrite(RD, HIGH);  
}

void dumpROM()
{
  unsigned long currentAddress = 0;
  int bufferCnt = 0;
  while(currentAddress < 0x800000)
  {
    latchAddress(currentAddress / 2);
    strobeRS();

    buffer1[bufferCnt] = PINC;
    buffer1[bufferCnt+1] = PINL;

    currentAddress += 2;
    bufferCnt+=2;

    // [...]
  }
  // [...]
}
```

https://web.archive.org/web/20170914163430/http://reinerziegler.de/GBA/gba.htm

> How does the GBA ROM cart interface work?
>
> GBA ROMs are special chips that contain a standard ROM, address latches, and
> address counters all on one chip. Cart accesses can be either sequential or
> non-sequential. The first access to a random cart ROM location must be
> non-sequential. This type of access is done by putting the lower 16 bits of
> the ROM address on cart lines AD0-AD15 and setting /CS low to latch address
> lines A0-A15. Then /RD is strobed low to read 16 bits of data from that ROM
> location. (Data is valid on the rising edge of /RD.) The following sequential
> ROM location(s) can be read by again strobing /RD low. Sequential ROM access
> does not require doing another /CS high-to-low transitions because there are
> count up registers in the cart ROM chip that keep track of the next ROM
> location to read. Address increment occurs on the low-to-high edge of all
> /RD. In theory, you can read an entire GBA ROM with just one non-sequential
> read (address 0) and all of the other reads as sequential so address counters
> must be used on most address lines to exactly emulate a GBA ROM. However, you
> only need to use address latch / counters on A0-A15 in order to satisfy the
> GBA since A16-A23 are always accurate. For more details, take a look [link]
> here :-) 

# Timing diagram

https://wavedrom.com/editor.html

```
{signal: [
  {name: 'CS2',    wave: '1................'},
  {name: 'WR',     wave: '1................'},
  {name: 'CS',     wave: '1.0............1.'},
  {name: 'RD',     wave: '1...0.1.0.1.0.1..'},
  {name: 'LADDR',  wave: 'x..3.....3...3..x', data: ['0x000000', '0x000001', '0x000002']},
  {name: 'ADDR',   wave: 'x4.x.............', data: ['0x000000']},
  {name: 'DATA',   wave: 'x....5.xx5.xx5.x.', data: ['data(00)', 'data(01)', 'data(02)']},
]}
```

# Diffoscope output

`diffoscope online/Zelda\ -\ the\ Minish\ Cap\ #\ GBA.GBA chinese_zelda_minish_cap.gba`

```diff
--- online/Zelda - the Minish Cap # GBA.GBA
+++ chinese_zelda_minish_cap.gba
@@ -1,19 +1,19 @@
-00000000: 2e00 00ea 24ff ae51 699a a221 3d84 820a  ....$..Qi..!=...
+00000000: feff 3dea 24ff ae51 699a a221 3d84 820a  ..=.$..Qi..!=...
 00000010: 84e4 09ad 1124 8b98 c081 7f21 a352 be19  .....$.....!.R..
 00000020: 9309 ce20 1046 4a4a f827 31ec 58c7 e833  ... .FJJ.'1.X..3
 00000030: 82e3 cebf 85f4 df94 ce4b 09c1 9456 8ac0  .........K...V..
 00000040: 1372 a7fc 9f84 4d73 a3ca 9a61 5897 a327  .r....Ms...aX..'
 00000050: fc03 9876 231d c761 0304 ae56 bf38 8400  ...v#..a...V.8..
 00000060: 40a7 0efd ff52 fe03 6f95 30f1 97fb c085  @....R..o.0.....
 00000070: 60d6 8025 a963 be03 014e 38e2 f9a2 34ff  `..%.c...N8...4.
 00000080: bb3e 0344 7800 90cb 8811 3a94 65c0 7c63  .>.Dx.....:.e.|c
 00000090: 87f0 3caf d625 e48b 380a ac72 21d4 f807  ..<..%..8..r!...
 000000a0: 4742 415a 454c 4441 204d 4300 425a 4d50  GBAZELDA MC.BZMP
-000000b0: 3031 9600 0000 0000 0000 0000 00cd 0000  01..............
+000000b0: 3031 9600 0000 0000 0000 0000 00cd 5645  01............VE
 000000c0: 1200 a0e3 00f0 29e1 30d0 9fe5 1f00 a0e3  ......).0.......
 000000d0: 00f0 29e1 28d0 9fe5 2810 9fe5 2800 9fe5  ..).(...(...(...
 000000e0: 0000 81e5 2410 9fe5 0fe0 a0e1 11ff 2fe1  ....$........./.
 000000f0: 1c10 9fe5 0fe0 a0e1 11ff 2fe1 efff ffea  ........../.....
 00000100: a07f 0003 007f 0003 fc7f 0003 905d 0003  .............]..
 00000110: 1901 0008 e959 0508 0aa0 0ec8 043a 1160  .....Y.......:.`
 00000120: 9a42 fbdc 7047 07a0 0ec8 1868 0433 8842  .B..pG.....h.3.B
@@ -671,15 +671,15 @@
 000029e0: 70bd 0020 70bd f0b5 1a4c 1b4f e688 fd8d  p.. p....L.O....
 000029f0: ad1b ad1a 401b 5200 9042 09d2 2689 7d8e  ....@.R..B..&.}.
 00002a00: ad1b ed1a 491b 5b00 9942 01d2 0120 f0bd  ....I.[..B... ..
 00002a10: 0020 f0bd 0001 0000 0428 0008 00ee 0000  . .......(......
 00002a20: 8888 0c08 0889 0c08 000e 0000 0e02 0000  ................
 00002a30: e004 0000 00e0 0000 0e20 0000 e040 0000  ......... ...@..
 00002a40: 04e0 0000 020e 0000 40e0 0000 200e 0000  ........@... ...
-00002a50: f00b 0003 f00b 0003 6011 0003 0000 0000  ........`.......
+00002a50: f00b 0003 f00b 0003 6011 0003 3075 0008  ........`...0u..
 00002a60: 0000 0000 0000 0000 0000 0000 3075 0008  ............0u..
 00002a70: e046 1108 00a5 1308 0000 0000 3075 0008  .F..........0u..
 00002a80: d44a 1108 00a5 1308 0000 0000 3075 0008  .J..........0u..
 00002a90: 6c4e 1108 00a5 1308 0000 0000 c07b 0008  lN...........{..
 00002aa0: 4052 1108 00a5 1308 0000 0000 c878 0008  @R...........x..
 00002ab0: e455 1108 00a5 1308 0000 0000 9c79 0008  .U...........y..
 00002ac0: c859 1108 00a5 1308 0000 0000 507a 0008  .Y..........Pz..
@@ -31871,16 +31871,16 @@
 0007c7e0: 8000 0002 00b5 0748 4189 0839 4181 0821  .......HA..9A..!
 0007c7f0: 405e 0028 04dc 0448 0821 4181 0121 c171  @^.(...H.!A..!.q
 0007c800: 0020 00bd e01e 0202 8000 0002 30b5 041c  . ..........0...
 0007c810: 0649 4889 0028 2bd1 d9f7 68fa 012c 11d0  .IH..(+...h..,..
 0007c820: 012c 05d3 022c 14d0 18e0 0000 8000 0002  .,...,..........
 0007c830: 8020 8004 0079 0249 00f0 78f8 0de0 0000  . ...y.I..x.....
 0007c840: 402a 0002 8020 8004 0079 00f0 8ff8 0125  @*... ...y.....%
-0007c850: 04e0 8020 8004 00f0 6df8 051c 0349 4d81  ... ....m....IM.
-0007c860: 0220 c871 d9f7 90fa 04e0 0000 8000 0002  . .q............
+0007c850: 04e0 8020 8004 00f0 6df8 051c 0049 0847  ... ....m....I.G
+0007c860: 5500 f808 d9f7 90fa 04e0 0000 8000 0002  U...............
 0007c870: 0138 4881 0020 30bd 30b5 0024 0b4a 5089  .8H.. 0.0..$.JP.
 0007c880: 0830 5081 d388 0625 515f 0004 0014 8142  .0P....%Q_.....B
 0007c890: 0adc 5381 d3f7 94fb 0548 0021 c171 4089  ..S......H.!.q@.
 0007c8a0: 013c 0128 00d1 0124 201c 30bd e01e 0202  .<.(...$ .0.....
 0007c8b0: 8000 0002 70b5 4020 34f0 e2f8 0420 00f0  ....p.@ 4.... ..
 0007c8c0: 93f9 041c 0025 e088 174e 2288 311c 00f0  .....%...N".1...
 0007c8d0: cff9 0028 00d1 0125 2089 2288 311c 00f0  ...(...% .".1...
@@ -45230,30 +45230,30 @@
 000b0ad0: 2388 1e1c 0023 2380 144d 2c88 144b 1c40  #....##..M,..K.@
 000b0ae0: 144b 1b68 db88 1c43 2c80 134b 1860 1348  .K.h...C,..K.`.H
 000b0af0: 0160 1349 8020 0006 0243 0a60 0231 8022  .`.I. ...C.`.1."
 000b0b00: 1202 101c 0988 0840 0028 07d0 0d4a 8020  .......@.(...J. 
 000b0b10: 0002 011c 1088 0840 0028 fbd1 0248 0680  .......@.(...H..
 000b0b20: 70bc 01bc 0047 0000 0802 0004 0402 0004  p....G..........
 000b0b30: fff8 0000 506a 0302 d400 0004 d800 0004  ....Pj..........
-000b0b40: dc00 0004 de00 0004 70b5 a2b0 0d1c 0004  ........p.......
-000b0b50: 030c 0348 0068 8088 8342 05d3 0148 44e0  ...H.h...B...HD.
-000b0b60: 506a 0302 ff80 0000 2248 061c 0068 017a  Pj......"H...h.z
+000b0b40: dc00 0004 de00 0004 70b5 0004 0a1c 400b  ........p.....@.
+000b0b50: e021 0905 4118 0731 0023 0878 1070 0133  .!..A..1.#.x.p.3
+000b0b60: 0132 0139 072b f8d9 0020 70bc 02bc 0847  .2.9.+... p....G
 000b0b70: 4800 6c46 0219 0232 0024 8c42 09d2 1380  H.lF...2.$.B....
 000b0b80: 023a 5b08 601c 0006 040e 3068 007a 8442  .:[.`.....0h.z.B
 000b0b90: f5d3 0120 1080 023a 1080 d024 2405 1548  ... ...:...$$..H
 000b0ba0: 0068 027a 0332 6846 211c fff7 8dff 201c  .h.z.2hF!..... .
 000b0bb0: 6946 4422 fff7 88ff 02aa 0635 0024 0126  iFD".......5.$.&
 000b0bc0: 0021 0023 4904 1088 3040 090c 0143 0232  .!.#I...0@...C.2
 000b0bd0: 581c 0006 030e 0f2b f4d9 2980 023d 601c  X......+..)..=`.
 000b0be0: 0006 040e 032c ebd9 0020 22b0 70bc 02bc  .....,... ".p...
 000b0bf0: 0847 0000 506a 0302 00b5 0004 000c 0122  .G..Pj........."
-000b0c00: 00f0 04f8 0004 000c 02bc 0847 f0b5 acb0  ...........G....
-000b0c10: 0d1c 0004 010c 1206 170e 0348 0068 8088  ...........H.h..
-000b0c20: 8142 05d3 0148 9de0 506a 0302 ff80 0000  .B...H..Pj......
-000b0c30: 0f48 0068 007a 4000 6a46 8318 8433 0020  .H.h.z@.jF...3. 
+000b0c00: 00f0 04f8 0004 000c 02bc 0847 70b5 0004  ...........Gp...
+000b0c10: 0a1c 400b e021 0905 4118 0731 0023 1078  ..@..!..A..1.#.x
+000b0c20: 0870 0133 0132 0139 072b f8d9 0020 70bc  .p.3.2.9.+... p.
+000b0c30: 02bc 0847 007a 4000 6a46 8318 8433 0020  ...G.z@.jF...3. 
 000b0c40: 1880 023b 0024 2a88 0235 0020 1a80 023b  ...;.$*..5. ...;
 000b0c50: 5208 0130 0006 000e 0f28 f7d9 601c 0006  R..0.....(..`...
 000b0c60: 040e 032c efd9 0024 0148 021c 0068 08e0  ...,...$.H...h..
 000b0c70: 506a 0302 1980 023b 4908 601c 0006 040e  Pj.....;I.`.....
 000b0c80: 1068 007a 8442 f5d3 0020 1880 023b 0126  .h.z.B... ...;.&
 000b0c90: 1e80 d024 2405 1748 0068 027a 4332 6846  ...$$..H.h.zC2hF
 000b0ca0: 211c fff7 11ff 0025 29aa 1580 6946 a631  !......%)...iF.1
@@ -1015802,205 +1015802,205 @@
 00f7ff90: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7ffa0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7ffb0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7ffc0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7ffd0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7ffe0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f7fff0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00f80000: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00f80010: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00f80020: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-*
-00f80bc0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00f80bd0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00f80be0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
+00f80000: 44d0 9fe5 fc00 a0e3 0008 a0e1 0213 a0e3  D...............
+00f80010: 0100 80e1 0e14 a0e3 0820 a0e3 0226 a0e1  ......... ...&..
+00f80020: 0030 d0e5 0040 d0e5 0400 53e1 fbff ff1a  .0...@....S.....
+00f80030: 0030 c1e5 0110 81e2 0100 80e2 0120 52e2  .0........... R.
+00f80040: f6ff ff1a 0400 9fe5 10ff 2fe1 007f 0003  ........../.....
+00f80050: c000 0008 0349 0847 0349 4d81 0220 c871  .....I.G.IM.. .q
+00f80060: 0248 0047 7000 f808 8000 0002 65c8 0708  .H.Gp.......e...
+00f80070: fc00 2de9 ec00 9fe5 ec10 9fe5 b020 d0e1  ..-.......... ..
+00f80080: b020 c1e1 e400 9fe5 e410 9fe5 b020 d0e1  . ........... ..
+00f80090: b220 c1e0 0103 a0e3 b228 d0e1 b220 c1e0  . .......(... ..
+00f800a0: b028 d0e1 b220 c1e0 ba2b d0e1 b220 c1e0  .(... ...+... ..
+00f800b0: b62c d0e1 b220 c1e0 b22d d0e1 b220 c1e0  .,... ...-... ..
+00f800c0: be2d d0e1 b220 c1e0 0000 a0e3 9410 9fe5  .-... ..........
+00f800d0: b000 c1e1 9410 9fe5 b000 c1e1 0113 a0e3  ................
+00f800e0: b008 c1e1 ba0b c1e1 b60c c1e1 b20d c1e1  ................
+00f800f0: be0d c1e1 0300 a0e3 b208 c1e1 7400 9fe5  ............t...
+00f80100: b010 d0e1 1d00 00ea 5800 9fe5 5810 9fe5  ........X...X...
+00f80110: b020 d1e1 b020 c0e1 5000 9fe5 5010 9fe5  . ... ..P...P...
+00f80120: b220 d1e0 b020 c0e1 0103 a0e3 b220 d1e0  . ... ....... ..
+00f80130: b228 c0e1 b220 d1e0 b028 c0e1 b220 d1e0  .(... ...(... ..
+00f80140: ba2b c0e1 b220 d1e0 b62c c0e1 b220 d1e0  .+... ...,... ..
+00f80150: b22d c0e1 b220 d1e0 be2d c0e1 fc00 bde8  .-... ...-......
+00f80160: 1410 9fe5 11ff 2fe1 0802 0004 00fc 0302  ....../.........
+00f80170: 0002 0004 02fc 0302 8400 0004 5900 f808  ............Y...
+00f80180: 2400 9fe5 2410 9fe5 2420 9fe5 0030 91e5  $...$...$ ...0..
+00f80190: 0030 80e5 0410 81e2 0400 80e2 0200 51e1  .0............Q.
+00f801a0: f9ff ff1a 0000 9fe5 10ff 2fe1 10fc 0302  ........../.....
+00f801b0: b801 f808 5c02 f808 0233 a0e3 8440 9fe5  ....\....3...@..
+00f801c0: 8450 9fe5 8460 9fe5 b040 c3e1 552f 83e2  .P...`...@..U/..
+00f801d0: b050 c2e1 4020 83e2 b000 d2e1 0600 50e1  .P..@ ........P.
+00f801e0: 0a00 000a b040 c3e1 aa20 83e2 b050 c2e1  .....@... ...P..
+00f801f0: 2020 83e2 b000 d2e1 0600 50e1 0600 000a    ........P.....
+00f80200: f010 a0e3 b010 c3e1 4410 9fe5 11ff 2fe1  ........D...../.
+00f80210: b040 c3e1 0000 a0e3 0200 00ea b040 c3e1  .@...........@..
+00f80220: 0400 a0e3 ffff ffea 2810 9fe5 11ff 2fe1  ........(...../.
+00f80230: 0000 50e3 0100 000a 0400 50e3 0000 000a  ..P.......P.....
+00f80240: cc00 00ea 6000 00ea f0f0 0000 9898 0000  ....`...........
+00f80250: 5251 0000 6002 f808 3002 f808 0000 a0e1  RQ..`...0.......
+00f80260: bc00 1fe5 2010 9fe5 2020 9fe5 0030 91e5  .... ...  ...0..
+00f80270: 0030 80e5 0410 81e2 0400 80e2 0200 51e1  .0............Q.
+00f80280: f9ff ff1a e000 1fe5 10ff 2fe1 9402 f808  ........../.....
+00f80290: c803 f808 0233 a0e3 ff10 a0e3 b010 c3e1  .....3..........
+00f802a0: 0000 a0e1 5010 a0e3 b010 c3e1 0000 a0e1  ....P...........
+00f802b0: 9010 a0e3 b010 c3e1 0000 a0e1 b010 d3e1  ................
+00f802c0: 8a00 51e3 0300 000a f010 a0e3 b010 c3e1  ..Q.............
+00f802d0: 0200 a0e3 2900 00ea ff10 a0e3 b010 c3e1  ....)...........
+00f802e0: 0000 a0e1 9010 a0e3 b010 c3e1 0000 a0e1  ................
+00f802f0: 0230 83e2 b010 d3e1 ac20 9fe5 0200 51e1  .0....... ....Q.
+00f80300: 1600 000a a420 9fe5 0200 51e1 1300 000a  ..... ....Q.....
+00f80310: 9c20 9fe5 0200 51e1 1000 000a 9420 9fe5  . ....Q...... ..
+00f80320: 0200 51e1 1100 000a 8c20 9fe5 0200 51e1  ..Q...... ....Q.
+00f80330: 0e00 000a 8420 9fe5 0200 51e1 0300 000a  ..... ....Q.....
+00f80340: f010 a0e3 b010 c3e1 0200 a0e3 0b00 00ea  ................
+00f80350: f010 a0e3 b010 c3e1 0200 a0e3 0700 00ea  ................
+00f80360: ff10 a0e3 b010 c3e1 0300 a0e3 0300 00ea  ................
+00f80370: ff10 a0e3 b010 c3e1 0100 a0e3 ffff ffea  ................
+00f80380: 3c10 9fe5 11ff 2fe1 0100 50e3 0300 000a  <...../...P.....
+00f80390: 0200 50e3 0200 000a 0300 50e3 0100 000a  ..P.......P.....
+00f803a0: 4201 00ea e300 00ea ad01 00ea 1588 0000  B...............
+00f803b0: 1088 0000 0e88 0000 7d88 0000 b088 0000  ........}.......
+00f803c0: 7d22 0000 8803 f808 0000 a0e1 2802 1fe5  }"..........(...
+00f803d0: 6411 9fe5 6421 9fe5 0030 91e5 0030 80e5  d...d!...0...0..
+00f803e0: 0410 81e2 0400 80e2 0200 51e1 f9ff ff1a  ..........Q.....
+00f803f0: 4c02 1fe5 10ff 2fe1 fc20 a0e3 0228 a0e1  L...../.. ...(..
+00f80400: 0203 a0e3 0020 82e1 3401 9fe5 3411 9fe5  ..... ..4...4...
+00f80410: 3441 9fe5 3451 9fe5 3431 9fe5 0630 53e2  4A..4Q..41...0S.
+00f80420: b030 c2e1 b040 c0e1 b050 c1e1 2431 9fe5  .0...@...P..$1..
+00f80430: b030 c0e1 b040 c0e1 b050 c1e1 1831 9fe5  .0...@...P...1..
+00f80440: b030 c2e1 0d00 00eb 1031 9fe5 0430 53e2  .0.......1...0S.
+00f80450: b030 c2e1 b040 c0e1 b050 c1e1 0031 9fe5  .0...@...P...1..
+00f80460: b030 c0e1 1e00 00eb f830 9fe5 0530 53e2  .0.......0...0S.
+00f80470: b030 c2e1 f000 9fe5 0e00 50e2 10ff 2fe1  .0........P.../.
+00f80480: 0040 2de9 0136 a0e3 0130 53e2 0700 000a  .@-..6...0S.....
+00f80490: b060 d2e1 0209 16e3 0400 001a 020a 16e3  .`..............
+00f804a0: f8ff ff0a b060 d2e1 0209 16e3 f5ff ff0a  .....`..........
+00f804b0: 0136 a0e3 0130 53e2 0700 000a b060 d2e1  .6...0S......`..
+00f804c0: 8000 16e3 0400 001a 2000 16e3 f8ff ff0a  ........ .......
+00f804d0: b060 d2e1 8000 16e3 f5ff ff0a 0040 bde8  .`...........@..
+00f804e0: 0ef0 a0e1 0440 2de9 0e14 a0e3 0830 a0e3  .....@-......0..
+00f804f0: 0336 a0e1 0160 d1e4 0170 d1e4 0764 86e1  .6...`...p...d..
+00f80500: 6850 9fe5 b050 c2e1 0000 a0e1 b060 c2e1  hP...P.......`..
+00f80510: 010c a0e3 b070 d2e1 0700 56e1 0100 000a  .....p....V.....
+00f80520: 0100 50e2 faff ff1a 0220 82e2 0230 53e2  ..P...... ...0S.
+00f80530: efff ff1a 0440 bde8 0ef0 a0e1 f803 f808  .....@..........
+00f80540: 7405 f808 aa0a 0008 5405 0008 a9aa 0000  t.......T.......
+00f80550: 5655 0000 f6f0 0000 8080 0000 3030 0000  VU..........00..
+00f80560: f4f0 0000 2020 0000 f5f0 0000 1601 f808  ....  ..........
+00f80570: a0a0 0000 0000 a0e1 d403 1fe5 8011 9fe5  ................
+00f80580: 8021 9fe5 0030 91e5 0030 80e5 0410 81e2  .!...0...0......
+00f80590: 0400 80e2 0200 51e1 f9ff ff1a f803 1fe5  ......Q.........
+00f805a0: 10ff 2fe1 fc20 a0e3 0228 a0e1 0203 a0e3  ../.. ...(......
+00f805b0: 0020 82e1 5001 9fe5 0e00 50e2 4c11 9fe5  . ..P.....P.L...
+00f805c0: 0e10 51e2 4841 9fe5 0e40 54e2 4451 9fe5  ..Q.HA...@T.DQ..
+00f805d0: 0e50 55e2 4031 9fe5 0e30 53e2 b030 c2e1  .PU.@1...0S..0..
+00f805e0: b040 c0e1 b050 c1e1 3031 9fe5 0e30 53e2  .@...P..01...0S.
+00f805f0: b030 c0e1 b040 c0e1 b050 c1e1 2031 9fe5  .0...@...P.. 1..
+00f80600: 0e30 53e2 b030 c2e1 0d00 00eb 0831 9fe5  .0S..0.......1..
+00f80610: 0e30 53e2 b030 c2e1 b040 c0e1 b050 c1e1  .0S..0...@...P..
+00f80620: 0031 9fe5 0e30 53e2 b030 c0e1 1d00 00eb  .1...0S..0......
+00f80630: e430 9fe5 0e30 53e2 b030 c2e1 e800 9fe5  .0...0S..0......
+00f80640: 10ff 2fe1 0040 2de9 0136 a0e3 0130 53e2  ../..@-..6...0S.
+00f80650: 0700 000a b060 d2e1 0209 16e3 0400 001a  .....`..........
+00f80660: 020a 16e3 f8ff ff0a b060 d2e1 0209 16e3  .........`......
+00f80670: f5ff ff0a 0136 a0e3 0130 53e2 0700 000a  .....6...0S.....
+00f80680: b060 d2e1 8000 16e3 0400 001a 2000 16e3  .`.......... ...
+00f80690: f8ff ff0a b060 d2e1 8000 16e3 f5ff ff0a  .....`..........
+00f806a0: 0040 bde8 0ef0 a0e1 0440 2de9 0e14 a0e3  .@.......@-.....
+00f806b0: 0830 a0e3 0336 a0e1 0160 d1e4 0170 d1e4  .0...6...`...p..
+00f806c0: 0764 86e1 6450 9fe5 0e50 55e2 b050 c2e1  .d..dP...PU..P..
+00f806d0: 0000 a0e1 b060 c2e1 010c a0e3 b070 d2e1  .....`.......p..
+00f806e0: 0700 56e1 0100 000a 0100 50e2 faff ff1a  ..V.......P.....
+00f806f0: 0220 82e2 0230 53e2 eeff ff1a 0440 bde8  . ...0S......@..
+00f80700: 0ef0 a0e1 a405 f808 3407 f808 6215 0008  ........4...b...
+00f80710: b80a 0008 b7aa 0000 6455 0000 fef0 0000  ........dU......
+00f80720: 8e80 0000 3e30 0000 2e20 0000 0801 f808  ....>0... ......
+00f80730: aea0 0000 0000 a0e1 9405 1fe5 5411 9fe5  ............T...
+00f80740: 5421 9fe5 0030 91e5 0030 80e5 0410 81e2  T!...0...0......
+00f80750: 0400 80e2 0200 51e1 f9ff ff1a b805 1fe5  ......Q.........
+00f80760: 10ff 2fe1 fc20 a0e3 0228 a0e1 0203 a0e3  ../.. ...(......
+00f80770: 0020 82e1 2401 9fe5 0a00 50e2 2011 9fe5  . ..$.....P. ...
+00f80780: 0a10 51e2 a940 a0e3 5650 a0e3 1600 00eb  ..Q..@..VP......
+00f80790: b040 c0e1 b050 c1e1 8030 a0e3 b030 c0e1  .@...P...0...0..
+00f807a0: b040 c0e1 b050 c1e1 3030 a0e3 b030 c2e1  .@...P..00...0..
+00f807b0: 2a00 00eb 0c00 00eb b040 c0e1 b050 c1e1  *........@...P..
+00f807c0: 2030 a0e3 b030 c0e1 0e00 00eb 9030 a0e3   0...0.......0..
+00f807d0: b030 c2e1 0030 a0e3 b030 c2e1 0200 00eb  .0...0...0......
+00f807e0: c000 9fe5 0300 50e2 10ff 2fe1 0040 2de9  ......P.../..@-.
+00f807f0: b040 c0e1 b050 c1e1 f030 a0e3 b030 c2e1  .@...P...0...0..
+00f80800: 0040 bde8 0ef0 a0e1 3f40 2de9 0e14 a0e3  .@......?@-.....
+00f80810: 0830 a0e3 0336 a0e1 0160 d1e4 0170 d1e4  .0...6...`...p..
+00f80820: 0764 86e1 a050 a0e3 b050 c2e1 0000 a0e1  .d...P...P......
+00f80830: b060 c2e1 010c a0e3 b070 d2e1 0700 56e1  .`.......p....V.
+00f80840: 0100 000a 0100 50e2 faff ff1a 0220 82e2  ......P...... ..
+00f80850: 0230 53e2 efff ff1a 3f40 bde8 0ef0 a0e1  .0S.....?@......
+00f80860: 0440 2de9 0136 a0e3 0130 53e2 0700 000a  .@-..6...0S.....
+00f80870: b060 d2e1 8000 16e3 0400 001a 2000 16e3  .`.......... ...
+00f80880: f8ff ff0a b060 d2e1 8000 16e3 f5ff ff0a  .....`..........
+00f80890: 0440 bde8 0ef0 a0e1 6407 f808 ac08 f808  .@......d.......
+00f808a0: b40a 0008 5e05 0008 0b01 f808 0000 a0e1  ....^...........
+00f808b0: 0c07 1fe5 9411 9fe5 9421 9fe5 0030 91e5  .........!...0..
+00f808c0: 0030 80e5 0410 81e2 0400 80e2 0200 51e1  .0............Q.
+00f808d0: f9ff ff1a 3007 1fe5 10ff 2fe1 fc20 a0e3  ....0...../.. ..
+00f808e0: 0228 a0e1 0203 a0e3 0020 82e1 3200 00eb  .(....... ..2...
+00f808f0: 0e54 a0e3 0830 a0e3 0336 a0e1 0200 00eb  .T...0...6......
+00f80900: 5001 9fe5 0100 50e2 10ff 2fe1 0040 2de9  P.....P.../..@-.
+00f80910: ff00 a0e3 b000 c2e1 0000 a0e1 7000 a0e3  ............p...
+00f80920: b000 c2e1 0000 a0e1 0000 d2e5 ff00 00e2  ................
+00f80930: 8000 50e3 fbff ff1a ff00 a0e3 b000 c2e1  ..P.............
+00f80940: 0000 a0e1 ea00 a0e3 b000 c2e1 0000 a0e1  ................
+00f80950: 0401 9fe5 b000 c2e1 0000 a0e1 021c a0e3  ................
+00f80960: 0000 d5e5 0150 85e2 0070 d5e5 0150 85e2  .....P...p...P..
+00f80970: 0704 80e1 b000 c2e1 0220 82e2 0110 51e2  ......... ....Q.
+00f80980: f6ff ff1a d000 a0e3 b000 c2e1 0000 a0e1  ................
+00f80990: 0000 d2e5 ff00 00e2 8000 50e3 fbff ff1a  ..........P.....
+00f809a0: 014b a0e3 0430 53e0 d8ff ff1a ff00 a0e3  .K...0S.........
+00f809b0: b000 c2e1 0040 bde8 0ef0 a0e1 0440 2de9  .....@.......@-.
+00f809c0: ff00 a0e3 b000 c2e1 0000 a0e1 6000 a0e3  ............`...
+00f809d0: b000 c2e1 0000 a0e1 d000 a0e3 b000 c2e1  ................
+00f809e0: 0000 a0e1 9000 a0e3 b000 c2e1 0000 a0e1  ................
+00f809f0: 0220 82e2 0000 d2e5 0300 00e2 fcff ff1a  . ..............
+00f80a00: 0220 52e2 0000 a0e1 ff00 a0e3 b000 c2e1  . R.............
+00f80a10: 0000 a0e1 2000 a0e3 b000 c2e1 0000 a0e1  .... ...........
+00f80a20: d000 a0e3 b000 c2e1 0000 a0e1 0000 d2e5  ................
+00f80a30: ff00 00e2 8000 50e3 fbff ff1a 0000 a0e1  ......P.........
+00f80a40: ff00 a0e3 b000 c2e1 0440 bde8 0ef0 a0e1  .........@......
+00f80a50: dc08 f808 600a f808 0901 f808 ff01 0000  ....`...........
+00f80a60: 0000 a0e1 c008 1fe5 6811 9fe5 6821 9fe5  ........h...h!..
+00f80a70: 0030 91e5 0030 80e5 0410 81e2 0400 80e2  .0...0..........
+00f80a80: 0200 51e1 f9ff ff1a e408 1fe5 10ff 2fe1  ..Q.........../.
+00f80a90: fc20 a0e3 0228 a0e1 0203 a0e3 0020 82e1  . ...(....... ..
+00f80aa0: 2700 00eb 0e54 a0e3 0830 a0e3 0336 a0e1  '....T...0...6..
+00f80ab0: 0200 00eb 2401 9fe5 0600 50e2 10ff 2fe1  ....$.....P.../.
+00f80ac0: 0040 2de9 ff00 a0e3 b000 c2e1 0000 a0e1  .@-.............
+00f80ad0: 7000 a0e3 b000 c2e1 0000 a0e1 0000 d2e5  p...............
+00f80ae0: ff00 00e2 8000 50e3 fbff ff1a ff00 a0e3  ......P.........
+00f80af0: b000 c2e1 0000 a0e1 4000 a0e3 b000 c2e1  ........@.......
+00f80b00: 0060 d5e5 0150 85e2 0070 d5e5 0150 85e2  .`...P...p...P..
+00f80b10: 0764 86e1 b060 c2e1 0000 d2e5 ff00 00e2  .d...`..........
+00f80b20: 8000 50e3 fbff ff1a 0220 82e2 0230 53e2  ..P...... ...0S.
+00f80b30: e3ff ff1a ff00 a0e3 b000 c2e1 0040 bde8  .............@..
+00f80b40: 0ef0 a0e1 0440 2de9 ff00 a0e3 b000 c2e1  .....@-.........
+00f80b50: 0000 a0e1 6000 a0e3 b000 c2e1 0000 a0e1  ....`...........
+00f80b60: d000 a0e3 b000 c2e1 0000 a0e1 9000 a0e3  ................
+00f80b70: b000 c2e1 0000 a0e1 0220 82e2 0000 d2e5  ......... ......
+00f80b80: 0300 00e2 fcff ff1a 0220 52e2 0000 a0e1  ......... R.....
+00f80b90: ff00 a0e3 b000 c2e1 0000 a0e1 2000 a0e3  ............ ...
+00f80ba0: b000 c2e1 0000 a0e1 d000 a0e3 b000 c2e1  ................
+00f80bb0: 0000 a0e1 0000 d2e5 ff00 00e2 8000 50e3  ..............P.
+00f80bc0: fbff ff1a 0000 a0e1 ff00 a0e3 b000 c2e1  ................
+00f80bd0: 0440 bde8 0ef0 a0e1 900a f808 e40b f808  .@..............
+00f80be0: 0e01 f808 0000 a0e1 0000 a0e1 ffff ffff  ................
 00f80bf0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c00: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c10: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c20: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c30: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c40: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00f80c50: ffff ffff ffff ffff ffff ffff ffff ffff  ................
@@ -1032186,102 +1032186,102 @@
 00fbff90: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbffa0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbffb0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbffc0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbffd0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbffe0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fbfff0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc0000: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc0010: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc0020: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-*
-00fc0550: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc0560: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc0570: ffff ffff ffff ffff ffff ffff ffff ffff  ................
+00fc0000: 4144 4c45 5a42 4741 4e49 4d20 4548 543a  ADLEZBGANIM EHT:
+00fc0010: 3a50 4143 2048 5349 0033 2041 444c 455a  :PAC HSI.3 ADLEZ
+00fc0020: 4d43 5a33 196f e691 ffff ffff ffff ffff  MCZ3.o..........
+00fc0030: 4d43 5a33 2931 d6cf ffff ffff ffff ffff  MCZ3)1..........
+00fc0040: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc0050: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc0060: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc0070: 0201 0100 4d43 5a33 0000 0000 4b4e 494c  ....MCZ3....KNIL
+00fc0080: 0000 0000 0101 0000 0000 0000 0000 0000  ................
+00fc0090: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00a0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00b0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00c0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00d0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00e0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc00f0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0100: 0000 0000 6b6e 694c 0000 0000 0000 0000  ....kniL........
+00fc0110: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0120: 0000 0000 0000 0000 0000 0000 1818 0000  ................
+00fc0130: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0140: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0150: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0550: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+*
+00fc0560: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc0570: 0000 0000 0000 0000 0000 0000 0000 0000  ................
 00fc0580: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0590: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc05a0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc05b0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc05c0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc05d0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc05e0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
@@ -1032442,102 +1032442,102 @@
 00fc0f90: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0fa0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0fb0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0fc0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0fd0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0fe0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc0ff0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc1000: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc1010: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc1020: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-*
-00fc1550: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc1560: ffff ffff ffff ffff ffff ffff ffff ffff  ................
-00fc1570: ffff ffff ffff ffff ffff ffff ffff ffff  ................
+00fc1000: 4144 4c45 5a42 4741 4e49 4d20 4548 543a  ADLEZBGANIM EHT:
+00fc1010: 3a50 4143 2048 5349 0033 2041 444c 455a  :PAC HSI.3 ADLEZ
+00fc1020: 4d43 5a33 196f e691 ffff ffff ffff ffff  MCZ3.o..........
+00fc1030: 4d43 5a33 2931 d6cf ffff ffff ffff ffff  MCZ3)1..........
+00fc1040: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc1050: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc1060: 5449 4e49 ffff ffff ffff ffff ffff ffff  TINI............
+00fc1070: 0201 0100 4d43 5a33 0000 0000 4b4e 494c  ....MCZ3....KNIL
+00fc1080: 0000 0000 0101 0000 0000 0000 0000 0000  ................
+00fc1090: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10a0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10b0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10c0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10d0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10e0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc10f0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1100: 0000 0000 6b6e 694c 0000 0000 0000 0000  ....kniL........
+00fc1110: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1120: 0000 0000 0000 0000 0000 0000 1818 0000  ................
+00fc1130: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1140: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1150: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+*
+00fc1550: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1560: 0000 0000 0000 0000 0000 0000 0000 0000  ................
+00fc1570: 0000 0000 0000 0000 0000 0000 0000 0000  ................
 00fc1580: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc1590: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc15a0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc15b0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc15c0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc15d0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
 00fc15e0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
```

# Analyzing the differences

```
-00000000: 2e00 00ea 24ff ae51 699a a221 3d84 820a  ....$..Qi..!=...
+00000000: feff 3dea 24ff ae51 699a a221 3d84 820a  ..=.$..Qi..!=...
```

Original:
```
 $ rasm2 -a arm -b 32 -d '2e0000ea'
b 0xc0
```

Note: 0xc0 is in the BIOS area, but what's there?

Chinese:
```
 $ rasm2 -a arm -b 32 -d 'feff3dea'
b 0xf80000
```

NOTE: jump to a routine that copies 0x8000 bytes from 0x8fc0000 to 0xe000000.
It writes the Cart RAM from Cart ROM.  The routine then jumps back to 0xc0 and
follows as the original game.

```
-000000b0: 3031 9600 0000 0000 0000 0000 00cd 0000  01..............
+000000b0: 3031 9600 0000 0000 0000 0000 00cd 5645  01............VE
```

Original:
```
 $ rasm2 -a arm -b 32 -d '00cd0000'
andeq ip, r0, r0, lsl 26
```

Chinese:
```
 $ rasm2 -a arm -b 32 -d '00cd5645'
ldrbmi ip, [r6, -0xd00]
```

According to https://problemkaputt.de/gbatek.htm#gbacartridgeheader 0xbe is a
reserved area and should be zero filled.  The chinese rom has 0x0000.  This is
part of the ROM header and is not code.

```
-00002a50: f00b 0003 f00b 0003 6011 0003 0000 0000  ........`.......
+00002a50: f00b 0003 f00b 0003 6011 0003 3075 0008  ........`...0u..
```

Original:
```
 $ rasm2 -a arm -b 32 -d '00000000'
andeq r0, r0, r0
 $ rasm2 -a arm -b 16 -d '0000 0000'
movs r0, r0
movs r0, r0
```

Chinese:
```
 $ rasm2 -a arm -b 32 -d '30750008'
stmdaeq r0, {r4, r5, r8, sl, ip, sp, lr}
 $ rasm2 -a arm -b 16 -d '3075 0008'
strb r0, [r6, 0x14]
lsrs r0, r0, 0x20
```

```
-0007c850: 04e0 8020 8004 00f0 6df8 051c 0349 4d81  ... ....m....IM.
-0007c860: 0220 c871 d9f7 90fa 04e0 0000 8000 0002  . .q............
+0007c850: 04e0 8020 8004 00f0 6df8 051c 0049 0847  ... ....m....I.G
+0007c860: 5500 f808 d9f7 90fa 04e0 0000 8000 0002  U...............
```

Original:
```
 $ rasm2 -a arm -b 16 -d '0349 4d81 0220 c871'
ldr r1, [pc, 0xc]
strh r5, [r1, 0xa]
movs r0, 2
strb r0, [r1, 7]
```

Chinese:
```
 $ rasm2 -a arm -b 16 -d '0049 0847 5500 f808'
ldr r1, [pc, 0] ; r1 = *(pc) = *(0x0807C860) = 0x08F80055
bx r1
; .dword 0x08f80055  
```

Notice that the Chinese cart jumps to a code that goes to 0x8f80070 and then
does the same operations as the original cart before going back [0x08f80058 -
0x08f8005e]:

```
 0x08f80054  ~   0349           ldr r1, aav.0x08f80070    ;[1] ; [0x8f80064:4]=0x8f80070 aav.0x08f80070
 0x08f80056      0847           bx r1                     ; aav.0x08f80070                             
 0x08f80058  ~   0349           ldr r1, [0x08f80068]      ;[2] ; [0x8f80068:4]=0x2000080               
 0x08f8005a      4d81           strh r5, [r1, 0xa]                                                     
 0x08f8005c      0220           movs r0, 2                                                             
 0x08f8005e      c871           strb r0, [r1, 7]                                                       
 0x08f80060      0248           ldr r0, aav.0x0807c865    ;[3] ; [0x8f8006c:4]=0x807c865 aav.0x0807c865
 0x08f80062      0047           bx r0                     ; aav.0x0807c865                             
```

```
-000b0b40: dc00 0004 de00 0004 70b5 a2b0 0d1c 0004  ........p.......
-000b0b50: 030c 0348 0068 8088 8342 05d3 0148 44e0  ...H.h...B...HD.
-000b0b60: 506a 0302 ff80 0000 2248 061c 0068 017a  Pj......"H...h.z
+000b0b40: dc00 0004 de00 0004 70b5 0004 0a1c 400b  ........p.....@.
+000b0b50: e021 0905 4118 0731 0023 0878 1070 0133  .!..A..1.#.x.p.3
+000b0b60: 0132 0139 072b f8d9 0020 70bc 02bc 0847  .2.9.+... p....G
```

Original:
```
 $ rasm2 -a arm -b 16 -d 'a2b0 0d1c 0004 030c 0348 0068 8088 8342 05d3 0148 44e0 506a 0302 ff80 0000 2248 061c 0068 017
a'
sub sp, 0x88
adds r5, r1, 0
lsls r0, r0, 0x10
lsrs r3, r0, 0x10
ldr r0, [pc, 0xc]
ldr r0, [r0]
ldrh r0, [r0, 4]
cmp r3, r0
blo 0x1e
ldr r0, [pc, 4]
b 0xa0
ldr r0, [r2, 0x24]
lsls r3, r0, 8
strh r7, [r7, 6]
movs r0, r0
ldr r0, [pc, 0x88]
adds r6, r0, 0
ldr r0, [r0]
ldrb r1, [r0, 8]
```

Chinese:
```
 $ rasm2 -a arm -b 16 -d '0004 0a1c 400b e021 0905 4118 0731 0023 0878 1070 0133 0132 0139 072b f8d9 0020 70bc 02bc 084
7'
lsls r0, r0, 0x10
adds r2, r1, 0
lsrs r0, r0, 0xd
movs r1, 0xe0
lsls r1, r1, 0x14
adds r1, r0, r1
adds r1, 7
movs r3, 0
ldrb r0, [r1]
strb r0, [r2]
adds r3, 1
adds r2, 1
subs r1, 1
cmp r3, 7
bls 0x10
movs r0, 0
pop {r4, r5, r6}
pop {r1}
bx r1
```

```
-000b0c00: 00f0 04f8 0004 000c 02bc 0847 f0b5 acb0  ...........G....
-000b0c10: 0d1c 0004 010c 1206 170e 0348 0068 8088  ...........H.h..
-000b0c20: 8142 05d3 0148 9de0 506a 0302 ff80 0000  .B...H..Pj......
-000b0c30: 0f48 0068 007a 4000 6a46 8318 8433 0020  .H.h.z@.jF...3. 
+000b0c00: 00f0 04f8 0004 000c 02bc 0847 70b5 0004  ...........Gp...
+000b0c10: 0a1c 400b e021 0905 4118 0731 0023 1078  ..@..!..A..1.#.x
+000b0c20: 0870 0133 0132 0139 072b f8d9 0020 70bc  .p.3.2.9.+... p.
+000b0c30: 02bc 0847 007a 4000 6a46 8318 8433 0020  ...G.z@.jF...3. 
```

Original:
```
 $ rasm2 -a arm -b 16 -d 'f0b5 acb0 0d1c 0004 010c 1206 170e 0348 0068 8088 8142 05d3 0148 9de0 506a 0302 ff80 0000 0f48 0068'
push {r4, r5, r6, r7, lr}
sub sp, 0xb0
adds r5, r1, 0
lsls r0, r0, 0x10
lsrs r1, r0, 0x10
lsls r2, r2, 0x18
lsrs r7, r2, 0x18
ldr r0, [pc, 0xc]
ldr r0, [r0]
ldrh r0, [r0, 4]
cmp r1, r0
blo 0x24
ldr r0, [pc, 4]
b 0x158
ldr r0, [r2, 0x24]
lsls r3, r0, 8
strh r7, [r7, 6]
movs r0, r0
ldr r0, [pc, 0x3c]
ldr r0, [r0]
```

Chinese:
```
 $ rasm2 -a arm -b 16 -d '70b5 0004 0a1c 400b e021 0905 4118 0731 0023 1078 0870 0133 0132 0139 072b f8d9 0020 70bc 02bc 0847'
     push {r4, r5, r6, lr}
     lsls r0, r0, 0x10 ; r0 <<= 0x10
     adds r2, r1, 0 ; r2 = r1
     lsrs r0, r0, 0xd ; r0 <<= 0xd
     movs r1, 0xe0 ; r1 = 0xe0
     lsls r1, r1, 0x14 ; r1 = r1 << 0x14 = 0xe000000
     adds r1, r0, r1 ; r1 = r1 + r0
     adds r1, 7 ; r1 += 7
     movs r3, 0 ; r3 = 0
0x12 ldrb r0, [r2] ; r0 = *r2
     strb r0, [r1] ; *r1 = r0
     adds r3, 1 ; r3++
     adds r2, 1 ; r2++
     subs r1, 1 ; r1--
     cmp r3, 7 ; if r3 <= 7 b 
     bls 0x12
     movs r0, 0 ; r0 = 0
     pop {r4, r5, r6}
     pop {r1}
     bx r1
```

# Notes

Rom gets mapped to address `0x8000000`.

Chinese ROM highlights:
- `0x8000000`: `b 0x8f80000`
- `0x8f80000`: `memcpy(0xe000000, 0x8fc0000, 0x8000)`
- `0x8f80048`: `bx 0x80000c0`
- `0x08f80180`: `memcpy(0x203fc10, 0x08f801b8, 0x08f8025c - 0x08f801b8); bx 0x203fc10`
- `0x08F80260`: `memcpy(0x203fc10, 0x08f80294, 0x08f803c8 - 0x08f80294); bx 0x203fc10`
- `0x08f803cc`: `memcpy(0x203fc10, 0x08f803f8, 0x08f80574 - 0x08f803f8); bx 0x203fc10`
- `0x08f80578`: `memcpy(0x203fc10, 0x08f805a4, 0x08f80734 - 0x08f805a4); bx 0x203fc10`
- `0x08f80738`: `memcpy(0x203fc10, 0x08f80764, 0x08f808ac - 0x08f80764); bx 0x203fc10`
- `0x08f808b0`: `memcpy(0x203fc10, 0x08f808dc, 0x08f80a60 - 0x08f808dc); bx 0x203fc10`
- `0x08f80a64`: `memcpy(0x203fc10, 0x08f80a90, 0x08f80be4 - 0x08f80a90); bx 0x203fc10`

0x8f80a64

Original ROM highlights:
- `0x8000000`: `b 0x80000c0`

Debugging with watchpoint/w at 0x8000000 gives a breakpoint at 0x0203FC24, some
disassembly from mgba of surrounding code:

```
> b 0x80000c0
> c
> watch/w 0x8000000
```

```
0203FC10:  E3A03302	mov r3, $8000000
0203FC14:  E59F4084	ldr r4, [$0203FCA0]
0203FC18:  E59F5084	ldr r5, [$0203FCA4]
0203FC1C:  E59F6084	ldr r6, [$0203FCA8]
0203FC20:  E1C340B0	strh r4, [r3, #0]
0203FC24:  E2832F55	add r2, r3, #340
```

Radare search for hex `0233a0e3` (that's `E3A03302` in reverse endianness)

```
0x08f801b8 hit1_0 0233a0e3
0x08f80294 hit1_1 0233a0e3
```

Let's see what we have there:

First force 32 bits because radare has autodetected that area to be thumb
`ahb 32 @ 0x08f801b8`

then:
`pd 10 arm 32 @ 0x08f801b8`

```
;-- aav.0x08f801b8:
;-- hit1_0:
; UNKNOWN XREFS from aav.0x08f8010b (+0x81, +0xa5)
0x08f801b8      0233a0e3       mov r3, entry0              ; move value between registers
; DATA XREF from aav.0x08f8010b (+0x89)
0x08f801bc      84409fe5       ldr r4, [0x08f80248]        ; [0x8f80248:4]=0xf0f0 ; load from memory to register
; CODE XREF from aav.0x08f80808 (+0x12)
0x08f801c0      84509fe5       ldr r5, [0x08f8024c]        ; [0x8f8024c:4]=0x9898 ; load from memory to register
; CODE XREF from aav.0x08f80808 (+0x16)
0x08f801c4      84609fe5       ldr r6, [0x08f80250]        ; [0x8f80250:4]=0x5152 ; load from memory to register
0x08f801c8      b040c3e1       strh r4, [r3]               ; store byte value in register into memory
```

So, at some point 0x08f801b8 is copied from ROM to EWRAM.  Let's watchpoint it to see when.

```
    0x08f80180      24009fe5       ldr r0, [0x08f801ac]      ;[3] ; [0x8f801ac:4]=0x203fc10 ; load from memory to register  
    0x08f80184      24109fe5       ldr r1, aav.0x08f801b8    ;[4] ; [0x8f801b0:4]=0x8f801b8 hit1_0 ; load from memory to register
    0x08f80188      24209fe5       ldr r2, aav.0x08f8025c    ;[5] ; [0x8f801b4:4]=0x8f8025c aav.0x08f8025c ; load from memor
.-> 0x08f8018c      003091e5       ldr r3, [r1]              ; 0x8f801b8 ; hit1_0 ; load from memory to register            
:   0x08f80190      003080e5       str r3, [r0]              ; store register into memory                                   
:   0x08f80194      041081e2       add r1, r1, 4             ; add two values                                               
:   0x08f80198      040080e2       add r0, r0, 4             ; add two values                                               
:   0x08f8019c      020051e1       cmp r1, r2                ; compare                                                      
|   ; CODE XREF from aav.0x08f80734 (+0x2a)                                                                                 
`=< 0x08f801a0      f9ffff1a       bne 0x8f8018c             ; branch if Z clear (not equal)                                
    ; XREFS: DATA 0x08f80284  DATA 0x08f803cc  DATA 0x08f803f0  DATA 0x08f80578  DATA 0x08f8059c  DATA 0x08f8075c           
    ; XREFS: DATA 0x08f808b0  DATA 0x08f808d4  DATA 0x08f80a64  DATA 0x08f80a88                                             
    0x08f801a4      00009fe5       ldr r0, [pc]              ;[3] ; [0x8f801ac:4]=0x203fc10 ; load from memory to register  
    0x08f801a8      10ff2fe1       bx r0                     ; branches and exchanges cpu mode to 16 bits (thumb mode)                                            
```

```
    ;-- aav.0x08f80260:                                                                                                  
    ; UNKNOWN XREF from aav.0x08f80230 (+0x24)                                                                           
    0x08f80260      bc001fe5       ldr r0, [0x08f801ac]      ;[1] ; [0x8f801ac:4]=0x203fc10 ; load from memory to register
    0x08f80264      20109fe5       ldr r1, aav.0x08f80294    ;[2] ; [0x8f8028c:4]=0x8f80294 hit1_1 ; load from memory to register
    0x08f80268      20209fe5       ldr r2, aav.0x08f803c8    ;[3] ; [0x8f80290:4]=0x8f803c8 aav.0x08f803c8 ; load from memory to register
    ; CODE XREF from aav.0x08f80734 (+0x12)                                                                              
.-> 0x08f8026c      003091e5       ldr r3, [r1]              ; load from memory to register                              
:   0x08f80270      003080e5       str r3, [r0]              ; store register into memory                                
:   0x08f80274      041081e2       add r1, r1, 4             ; add two values                                            
:   0x08f80278      040080e2       add r0, r0, 4             ; add two values                                            
:   0x08f8027c      020051e1       cmp r1, r2                ; compare                                                   
|   ; CODE XREF from aav.0x08f80734 (+0xa)                                                                               
`=< 0x08f80280      f9ffff1a       bne 0x8f8026c             ; branch if Z clear (not equal)                             
    ; CODE XREF from fcn.08f801e2 (0x8f80742)                                                                            
    0x08f80284      e0001fe5       ldr r0, [0x08f801ac]      ;[1] ; [0x8f801ac:4]=0x203fc10 ; load from memory to register
    0x08f80288      10ff2fe1       bx r0                     ; branches and exchanges cpu mode to 16 bits (thumb mode)   
```

```
/ (fcn) aav.0x08f80070 196                                                                                       
|   aav.0x08f80070 (int arg1);                                                                                                  
|           ; arg int arg1 @ r0                                                                                            
|           ; UNKNOWN XREF from aav.0x08f8005c (+0x8)                                                                            
|           0x08f80070      fc002de9       push {r2, r3, r4, r5, r6, r7}                                                          
|           0x08f80074      ec009fe5       ldr r0, [0x08f80168]      ;[1] ; [0x8f80168:4]=0x4000208 ; load from memory to register
|           0x08f80078      ec109fe5       ldr r1, [0x08f8016c]      ;[2] ; [0x8f8016c:4]=0x203fc00 ; load from memory to register
|           0x08f8007c      b020d0e1       ldrh r2, [r0]             ; arg1                                                       
|           0x08f80080      b020c1e1       strh r2, [r1]             ; store byte value in register into memory                   
|           0x08f80084      e4009fe5       ldr r0, [0x08f80170]      ;[3] ; [0x8f80170:4]=0x4000200 ; load from memory to register
|           0x08f80088      e4109fe5       ldr r1, [0x08f80174]      ;[4] ; [0x8f80174:4]=0x203fc02 ; load from memory to register
|           0x08f8008c      b020d0e1       ldrh r2, [r0]             ; arg1                                                       
|           0x08f80090      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f80094      0103a0e3       mov r0, 0x4000000         ; move value between registers                               
|           0x08f80098      b228d0e1       ldrh r2, [r0, 0x82]       ; arg1                                                       
|           0x08f8009c      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f800a0      b028d0e1       ldrh r2, [r0, 0x80]       ; arg1                                                       
|           0x08f800a4      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f800a8      ba2bd0e1       ldrh r2, [r0, 0xba]       ; arg1                                                       
|           ; CODE XREF from aav.0x08f80574 (+0x12)                                                                               
|           0x08f800ac      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f800b0      b62cd0e1       ldrh r2, [r0, 0xc6]       ; arg1                                                       
|           0x08f800b4      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f800b8      b22dd0e1       ldrh r2, [r0, 0xd2]       ; arg1                                                       
|           0x08f800bc      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           ; CODE XREF from aav.0x08f80574 (+0xa)                                                                                
|           0x08f800c0      be2dd0e1       ldrh r2, [r0, 0xde]       ; arg1                                                       
|           ; CODE XREF from aav.0x08f80574 (+0xe)                                                                                
|           0x08f800c4      b220c1e0       strh r2, [r1], 2          ; store byte value in register into memory                   
|           0x08f800c8      0000a0e3       mov r0, 0                 ; move value between registers                               
|           0x08f800cc      94109fe5       ldr r1, [0x08f80168]      ;[1] ; [0x8f80168:4]=0x4000208 ; load from memory to register
|           0x08f800d0      b000c1e1       strh r0, [r1]             ; store byte value in register into memory                   
|           0x08f800d4      94109fe5       ldr r1, [0x08f80170]      ;[3] ; [0x8f80170:4]=0x4000200 ; load from memory to register
|           0x08f800d8      b000c1e1       strh r0, [r1]             ; store byte value in register into memory                   
|           0x08f800dc      0113a0e3       mov r1, 0x4000000         ; move value between registers                               
|           0x08f800e0      b008c1e1       strh r0, [r1, 0x80]       ; store byte value in register into memory                   
|           0x08f800e4      ba0bc1e1       strh r0, [r1, 0xba]       ; store byte value in register into memory                   
|           0x08f800e8      b60cc1e1       strh r0, [r1, 0xc6]       ; store byte value in register into memory                   
|           0x08f800ec      b20dc1e1       strh r0, [r1, 0xd2]       ; store byte value in register into memory                   
|           0x08f800f0      be0dc1e1       strh r0, [r1, 0xde]       ; store byte value in register into memory                   
|           0x08f800f4      0300a0e3       mov r0, 3                 ; move value between registers                               
|           ; CODE XREF from aav.0x08f805a4 (+0x12)                                                                               
|           0x08f800f8      b208c1e1       strh r0, [r1, 0x82]       ; store byte value in register into memory                   
|           0x08f800fc      74009fe5       ldr r0, [0x08f80178]      ;[5] ; [0x8f80178:4]=0x4000084 ; load from memory to register
|           ;-- aav.0x08f80100:                                                                                                   
|           ; UNKNOWN XREF from fcn.08c34918 (+0xd4)                                                                              
|           ; CODE XREF from aav.0x08f805a4 (+0x1a)                                                                               
|           0x08f80100      b010d0e1       ldrh r1, [r0]             ; arg1                                                       
\       ,=< 0x08f80104      1d0000ea       b loc.08f80180            ; branches the program counter to dst (pc aka r15)           
```

# GBA I/O Map 

0x4000208  2    R/W  IME         Interrupt Master Enable Register
0x4000200  2    R/W  IE          Interrupt Enable Register
0x4000082  2    R/W  SOUNDCNT_H  Control Mixing/DMA Control
0x4000080  2    R/W  SOUNDCNT_L  Control Stereo/Volume/Enable   (NR50, NR51)
0x40000BA  2    R/W  DMA0CNT_H   DMA 0 Control
0x40000C6  2    R/W  DMA1CNT_H   DMA 1 Control
0x40000D2  2    R/W  DMA2CNT_H   DMA 2 Control
0x40000DE  2    R/W  DMA3CNT_H   DMA 3 Control
