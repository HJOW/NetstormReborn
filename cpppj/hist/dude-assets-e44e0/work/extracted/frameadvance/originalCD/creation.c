// ==== FUN_004acce0 @ 004acce0

undefined4 __thiscall FUN_004acce0(int param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  
  iVar2 = FUN_004abf50();
  iVar3 = FUN_004ae4b0();
  iVar1 = *(int *)(iVar2 + 0x170 + iVar3 * 4);
  iVar2 = *(int *)(iVar2 + 0x130 + iVar3 * 4);
  uVar4 = 0;
  param_2 = ((uint)*(byte *)(param_1 + 0x22) - iVar2) + param_2;
  if (param_2 < iVar1) {
    if (param_2 < 0) {
      uVar4 = 1;
      param_2 = param_2 + iVar1;
    }
  }
  else {
    uVar4 = 1;
    param_2 = param_2 - iVar1;
  }
  FUN_004acbc0(iVar2 + param_2,param_3);
  return uVar4;
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004ae4b0 @ 004ae4b0

int __fastcall FUN_004ae4b0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)(param_1 + 0x22) * 4) + -0x41;
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __fastcall FUN_004ae3f0(int param_1)

{
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)(param_1 + 0x22) * 4);
}


