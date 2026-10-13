// ==== FUN_004e50d0 @ 004e50d0

undefined4 __fastcall FUN_004e50d0(int *param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x320000) != 0) {
    return 0;
  }
  iVar2 = __ftol();
  iVar3 = __ftol();
  if ((*(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48) & 6) == 0) {
    iVar2 = (**(code **)(*param_1 + 200))();
    if (iVar2 != 0) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_004e5150 @ 004e5150

undefined4 __fastcall FUN_004e5150(int param_1)

{
  int iVar1;
  
  if ((*(byte *)(param_1 + 0x23) & 0x20) == 0) {
    iVar1 = FUN_0040d7b0((param_1 - DAT_005395dc) / 0x24);
    if (iVar1 == 0) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_0040d7b0 @ 0040d7b0

undefined4 FUN_0040d7b0(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = DAT_005395dc + param_1 * 0x24;
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x200000) != 0) {
    iVar2 = FUN_004aed30();
    if ((int)*(short *)(iVar4 + 0x1a) < iVar2 / 2) {
      return 1;
    }
    iVar2 = __ftol();
    iVar3 = __ftol();
    if ((*(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48) & 6) == 0) {
      return 1;
    }
    if ((*(byte *)(iVar4 + 0x23) & 0x20) != 0) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004aed30 @ 004aed30

undefined4 __fastcall FUN_004aed30(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x400000) == 0) {
    uVar2 = FUN_00444720();
    return uVar2;
  }
  if ((*(byte *)(param_1 + 0x23) & 1) == 0) {
    return 0;
  }
  uVar2 = FUN_00444720();
  return uVar2;
}


// ==== FUN_00444720 @ 00444720

int __fastcall FUN_00444720(int *param_1)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
    }
  }
  return *param_1;
}


// ==== FUN_004448e0 @ 004448e0

int __fastcall FUN_004448e0(int param_1)

{
  return (param_1 - DAT_0051c960) / 0x1d4;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_0040d770 @ 0040d770

undefined4 FUN_0040d770(void)

{
  int iVar1;
  
  iVar1 = FUN_0040c5b0();
  if (iVar1 == 0) {
    FUN_0040c880();
  }
  return 1;
}


// ==== FUN_0040c5b0 @ 0040c5b0

bool FUN_0040c5b0(void)

{
  int iVar1;
  
  iVar1 = FUN_004ae4b0();
  return iVar1 == 9;
}


