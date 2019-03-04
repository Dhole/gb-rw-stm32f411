- b 0x08002a5c
- b 0x0807c85c <- hit on save
- b 0x080b0c0c <- hit on save

Trace of flashing cart RAM:

```
..0x0807c85e ->
 0x08f80054..0x08f80056 ->
  0x08f80070..0x08f80104 ->
   0x08f80180..0x08f801a8 ->
   0x0203fc10[0x08f801b8]..0x0203fc64[0x08f8020c] ->
   0x08f80260..0x08f80288 ->
   0x0203fc10[0x08f80294]..0x0203fc50[0x08f802d4] ->
   0x0203fcfc[0x08f80380]..0x0203fd00[0x08f80384] ->
   0x08f80388..0x08f803a4..0x08F80738..0x08f80760 -> ; 3 different cases, code only goes through '2'
   0x0203fc10[0x08f80764]..0x0203fc38[0x08f8078c] ->
   0x0203fc98[0x08f807ec]..0x0203fcb0[0x08f80804] ->
   0x0203fc3c[0x08f80790]..[]..0x0203fc94[0x08f807e8] ->
  0x08f80108..0x08f80164 ->
 0x08f80058..0x08f80062 ->
0x0807c864..
```

```
0807C85C:  4900     	ldr r1, [$0807C860] ; [$0807C860] = 08F80055
0807C85E:  4708     	bx r1
08F80054:  4903     	ldr r1, [$08F80064] ; [$08F80064] = 08F80070
08F80056:  4708     	bx r1
08F80070:  E92D00FC	stmdb sp!, {r2-r7}
```

CPU instruction mode: `bx`
- if bit 0 is set -> thumb
- if bit 0 is clear -> ARM


```C
0x08f80054:
  goto 0x8f80070;
0x08f80058:
  // Instructions that were removed from the original cart to add the jump to
  // 0x08f80054
  (u16) *(0x200008a) = r5; // ldr r1, [0x08f80068] ; [0x8f80068:4]=0x2000080 ; original: ldr r1, [pc, 0xc]
                           // strh r5, [r1, 0xa]
  (u8) *(0x2000007) = 2;   // movs r0, 2
                           // strb r0, [r1, 7]
  goto 0x0807c864; // bx 0x0807c865 ; return back to the game
```

```C
0x8f80070:
```

```C
0x08f801b8:
  r3 = 0x8000000;

  *0x8000000 = 0xf0f0;
  *0x8000154 = 0x9898;
  if (*0x8000040 == 0x5152) { //goto 0x08f80210;
    *0x8000000 = 0xf0f0;
    r0 = 0;
    goto 0x08f80230;
  }

  *0x8000000 = 0xf0f0;
  *0x80000aa = 0x9898;
  if (*0x8000020 == 0x5152) { // goto 0x08f8021c;
    *0x8000000 = 0xf0f0;
    r0 = 4;
    goto 0x08f80230;
  }

  *0x8000000 = 0xf0;
  goto 0x08f80260; // run 0x08f80294 from EWRAM
```

```C
0x08f80230:
  if (r0 == 4) {
    goto 0x08f803cc; // run 0x08f803f8 from EWRAM
  } else { // r0 == 0
    goto 0x08f80578; // run 0x08f805a4 from EWRAM
  }
```

EWRAM
```C
0x08f801b8:
  [...]
```

EWRAM
```C
0x08f803f8:
  [...]
  goto 0x08f80108;
```

EWRAM
```C
0x08f805a4:
  [...]
  goto 0x08f80108;
```

```C
0x08f80108:
  // r0 = 0x4000208
  // r1 = 0x203fc00
  *0x4000208 = *0x203fc00;
  *0x4000200 = *0x203fc04:
  r0 = 0x4000000;
  *0x4000082 = *0x203fc02
  *0x4000080 = *0x203fc02
  *0x40000ba = *0x203fc02
  *0x40000d2 = *0x203fc02
  // pop {r2, r3, r4, r5, r6, r7}
  // r1 = 0x08f80059
  goto 0x08f80059;
```

EWRAM
```C
0x08f80764:
  [...]
```

EWRAM
```C
0x08f808dc:
  [...]
```

EWRAM
```C
0x08f80a90:
  [...]
```

EWRAM
```C
0x08f80294: // Detect flash chip model.  Output result (1 | 2 | 3) in r0.
  r3 = 0x8000000;

  *0x8000000 = 0xff;
  *0x8000000 = 0x50;
  *0x8000000 = 0x90;

  if (*0x8000000 != 0x8a) {
    *0x8000000 = 0xf0;
    return 2; // r0 = 2; goto 0x08f80380;
  }

  *0x8000000 = 0xff;
  *0x8000000 = 0x90;
  switch (*0x8000002) {
  case 0x8815:
  case 0x8810:
  case 0x880e:
    *0x8000002 = 0xff;
    return 3 // r0 = 3; goto 0x08f80380;
  case 0x887d:
  case 0x88b0:
    *0x8000002 = 0xff;
    return 1; // r0 = 1; goto 0x08f80380;
  case 0x227d:
  default:
    *0x8000002 = 0xf0;
    return 2; // r0 = 2; goto 0x08f80380;
    break;
  }
0x08f80380:
  goto 0x08f80388;
```

```C
0x08f80388:
  [...]
```
