// ==== FUN_0048cc10 @ 0048cc10

int __cdecl FUN_0048cc10(int param_1,int param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return param_1 + (DAT_005308ec >> 0x10 & 0xffffU) % (uint)(param_2 - param_1);
}


// ==== FUN_004ab670 @ 004ab670

undefined4 __cdecl FUN_004ab670(int param_1,int param_2)

{
  if (param_2 == 0) {
    if (param_1 < 1) {
      return 0;
    }
    if (0xfffe < param_1) {
      return 0;
    }
  }
  else {
    if (param_1 < 1) {
      return 0;
    }
    if (0xfffe < param_1) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_004aecf0 @ 004aecf0

byte __thiscall FUN_004aecf0(void *this)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = FUN_004abae0();
  if ((uVar2 & 0x400000) == 0) {
    bVar1 = FUN_004abac0(this);
    return bVar1 & 0x10;
  }
  return *(byte *)((int)this + 0x23) & 1;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_00444720 @ 00444720

int __thiscall FUN_00444720(void *this)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*(int *)this + (*(int *)this >> 0x1f & 3U)) >> 2;
    }
  }
  return *(int *)this;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


