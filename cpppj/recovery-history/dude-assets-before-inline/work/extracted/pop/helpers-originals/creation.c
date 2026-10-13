// ==== FUN_004990d0 @ 004990d0

void FUN_004990d0(undefined4 param_1,int param_2,undefined4 param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  int *piVar2;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined1 local_10 [16];
  
  if (DAT_0059a8b0 == 0) {
    FUN_00498ff0(&local_20,param_1,param_2,param_3,param_4);
    FUN_00445290(DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
    if ((param_5 & 0x10) != 0) {
      if (local_18 - local_20 < 0x13) {
        local_20 = local_20 + -9;
        local_18 = local_18 + 9;
      }
      local_1c = local_1c + -0xf;
      local_20 = local_20 + -3;
      local_18 = local_18 + 3;
    }
    if ((local_18 != local_20 && -1 < local_18 - local_20) &&
       (local_14 != local_1c && -1 < local_14 - local_1c)) {
      FUN_00497580(&local_20,param_5);
    }
    iVar1 = FUN_0049a840(param_1);
    if ((*(uint *)(iVar1 + 0xe8) & 0x40000) != 0) {
      piVar2 = (int *)FUN_00498ff0(local_10,param_1,*(int *)(iVar1 + 0x114) + param_2,param_3,
                                   param_4);
      local_20 = *piVar2;
      local_1c = piVar2[1];
      local_18 = piVar2[2];
      local_14 = piVar2[3];
      FUN_00445290(DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
      if ((local_18 != local_20 && -1 < local_18 - local_20) &&
         (local_14 != local_1c && -1 < local_14 - local_1c)) {
        FUN_00497580(&local_20,param_5);
      }
    }
  }
  return;
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


