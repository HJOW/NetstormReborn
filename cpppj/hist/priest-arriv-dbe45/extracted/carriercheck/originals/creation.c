// ==== FUN_00426fc0 @ 00426fc0

undefined4 __fastcall FUN_00426fc0(int *param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_004ac200();
  if ((uVar1 & 0x320000) == 0) {
    iVar2 = FUN_004e49c0();
    iVar3 = FUN_004e49c0();
    if ((*(byte *)(iVar3 + iVar2 * 0x100 + DAT_005c7c44) & 6) == 0) {
      iVar2 = (**(code **)(*param_1 + 200))();
      if (iVar2 != 0) {
        return 1;
      }
    }
  }
  return 0;
}


// ==== FUN_00427030 @ 00427030

undefined4 __fastcall FUN_00427030(int param_1)

{
  int iVar1;
  
  if ((*(byte *)(param_1 + 0x28) & 0x20) == 0) {
    iVar1 = FUN_00492090((param_1 - DAT_005c8464) / 0x32);
    if (iVar1 == 0) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_00492090 @ 00492090

undefined4 FUN_00492090(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = param_1 * 0x32 + DAT_005c8464;
  uVar1 = FUN_004ac200();
  if ((uVar1 & 0x200000) == 0) {
    return 0;
  }
  iVar3 = *(int *)(iVar4 + 0x1a);
  iVar2 = FUN_004adcb0();
  if (iVar2 / 2 <= iVar3) {
    iVar3 = FUN_004e49c0();
    iVar2 = FUN_004e49c0();
    if (((*(byte *)(iVar2 + iVar3 * 0x100 + DAT_005c7c44) & 6) != 0) &&
       ((*(byte *)(iVar4 + 0x28) & 0x20) == 0)) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004adcb0 @ 004adcb0

undefined4 __fastcall FUN_004adcb0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) == 0) {
    FUN_0049a840(*(undefined1 *)(param_1 + 10));
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  if ((*(byte *)(param_1 + 0x28) & 1) != 0) {
    FUN_0049a840(DAT_005412e8);
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  return 0;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049fce0 @ 0049fce0

int __fastcall FUN_0049fce0(int *param_1)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)(param_1 + 0x3a) & 0x10) == 0)) &&
     (((int)param_1 - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)param_1 - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
  }
  return *param_1;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


// ==== FUN_00494e80 @ 00494e80

undefined4 FUN_00494e80(void)

{
  int iVar1;
  
  iVar1 = FUN_004ad710();
  if (iVar1 != 9) {
    FUN_004941f0();
  }
  return 1;
}


