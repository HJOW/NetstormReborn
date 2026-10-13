// ==== FUN_004adc60 @ 004adc60

uint __thiscall FUN_004adc60(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) != 0) {
    return *(byte *)((int)this + 0x28) & 1;
  }
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(uint *)(iVar1 + 0xe8) & 0x10;
}


// ==== FUN_004adca0 @ 004adca0

void __thiscall FUN_004adca0(void *this,undefined4 param_2)

{
  *(undefined4 *)((int)this + 0x1a) = param_2;
  return;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __thiscall FUN_004ac1e0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_004acdf0 @ 004acdf0

undefined4 __thiscall FUN_004acdf0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if (((*(uint *)(iVar1 + 0xec) & 2) != 0) && ((*(uint *)(iVar1 + 0xec) & 0x1000000) == 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004acda0 @ 004acda0

uint __thiscall FUN_004acda0(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x1000006) == 0) {
    FUN_004e0620("genus & (gISLAND|gBRIDGE|gISLAND3x3)","\\Ns\\O\\Squid.cpp",0x4cf);
    uVar2 = DAT_0050c038;
  }
  else {
    uVar2 = DAT_0050c038;
    if ((*(uint *)(iVar1 + 0xec) & 4) == 0) {
      uVar2 = (uint)*(ushort *)((int)this + 8);
    }
  }
  return uVar2;
}


// ==== FUN_004180a0 @ 004180a0

undefined4 __thiscall FUN_004180a0(void *this)

{
  double *pdVar1;
  undefined1 local_8 [8];
  
  pdVar1 = (double *)FUN_00460d70(local_8);
  if (*(double *)this <= *pdVar1) {
    return 1;
  }
  return 0;
}


// ==== FUN_00418070 @ 00418070

void __thiscall FUN_00418070(void *this,float param_2)

{
  double *pdVar1;
  undefined1 local_8 [8];
  
  pdVar1 = (double *)FUN_00460d70(local_8);
  *(double *)this = (double)(param_2 + (float)*pdVar1);
  return;
}


// ==== FUN_00460d70 @ 00460d70

/* WARNING: Removing unreachable block (ram,0x00460da9) */
/* WARNING: Removing unreachable block (ram,0x00460dc1) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00460d70(double *param_1)

{
  DWORD DVar1;
  uint uVar2;
  
  DVar1 = timeGetTime();
  uVar2 = (DVar1 - _DAT_0055b4a0) + DAT_0055b4a4 * 1000;
  *param_1 = (double)(uVar2 / 1000) + (double)(uVar2 % 1000) * _DAT_0050a668;
  return;
}


// ==== FUN_0040eaf0 @ 0040eaf0

undefined2 __cdecl FUN_0040eaf0(void)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((((-1 < iVar1) && (-1 < iVar2)) && (iVar1 < 0x100)) && (iVar2 < 0x100)) {
    return *(undefined2 *)(DAT_00542514 + (iVar2 * 0x100 + iVar1) * 2);
  }
  return 0;
}


