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


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0x1a) = param_2;
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __thiscall FUN_004abac0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004acad0 @ 004acad0

undefined4 __thiscall FUN_004acad0(void)

{
  uint uVar1;
  
  uVar1 = FUN_004abae0();
  if (((uVar1 & 2) != 0) && ((uVar1 & 0x1000000) == 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004acaa0 @ 004acaa0

uint __thiscall FUN_004acaa0(void *this)

{
  uint uVar1;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x1000006) == 0) {
    return DAT_00505eb0;
  }
  if ((uVar1 & 4) != 0) {
    return DAT_00505eb0;
  }
  return (uint)*(ushort *)((int)this + 8);
}


// ==== FUN_004011b0 @ 004011b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004011b0(double *param_1)

{
  uint local_c [2];
  uint local_4;
  
  FUN_004010c0(&local_4,local_c,0);
  *param_1 = (double)local_4 + (double)local_c[0] * _DAT_00500008;
  return;
}


// ==== FUN_004aaa20 @ 004aaa20

void __cdecl FUN_004aaa20(undefined4 param_1,undefined4 param_2)

{
  DAT_005395d4 = param_1;
  DAT_005395d8 = param_2;
  return;
}


// ==== FUN_004aaa40 @ 004aaa40

int __cdecl FUN_004aaa40(int param_1)

{
  if (param_1 != 0) {
    return param_1 << 0x10;
  }
  return 0;
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


