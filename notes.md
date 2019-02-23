# GBA cart info

https://files.darkfader.net//gba/files/cartridge.txt

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
