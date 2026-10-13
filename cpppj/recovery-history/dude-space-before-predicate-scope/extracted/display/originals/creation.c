// ==== FUN_004ad500 @ 004ad500

void __fastcall FUN_004ad500(int param_1)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar1 = *(int *)(param_1 + 0x24);
  if (DAT_005e4794 == 0) goto LAB_004ad580;
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    if ((piVar2[0x45] <= iVar1) || (iVar1 < 0)) goto LAB_004ad54f;
LAB_004ad5ea:
    if (((piVar2[0x37] != 0) && (*piVar2 != -0x22222223)) && (*piVar2 != -0x32323233))
    goto LAB_004ad580;
  }
  else {
    if ((iVar1 < piVar2[0x45] * 2) && (-1 < iVar1)) goto LAB_004ad5ea;
LAB_004ad54f:
    if (DAT_005e4794 != 0) {
      uVar3 = FUN_0049adc0(iVar1,*(byte *)(param_1 + 0x28) & 1);
      FUN_00436d40(uVar3);
    }
  }
  FUN_004e0620("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp",
               0x846);
LAB_004ad580:
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    iVar4 = piVar2[0x45];
  }
  else {
    iVar4 = piVar2[0x45] * 2;
  }
  if ((iVar1 < iVar4) && (-1 < iVar1)) {
    if ((piVar2[0x37] != 0) && ((*piVar2 != -0x22222223 && (*piVar2 != -0x32323233)))) {
      FUN_004990d0(*(undefined1 *)(param_1 + 10),iVar1,*(undefined4 *)(param_1 + 0xe),
                   *(undefined4 *)(param_1 + 0x12),(*(byte *)(param_1 + 0x28) >> 2 & 1) << 4);
    }
  }
  else if (DAT_005e4794 != 0) {
    uVar3 = FUN_0049adc0(iVar1,*(byte *)(param_1 + 0x28) & 1);
    FUN_00436d40(uVar3);
    return;
  }
  return;
}


// ==== FUN_004ad670 @ 004ad670

void __fastcall FUN_004ad670(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ad672. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


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


// ==== FUN_00498ff0 @ 00498ff0

void FUN_00498ff0(int *param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar1 = FUN_004e49c0();
  iVar1 = iVar1 - DAT_0059a91c;
  iVar2 = FUN_004e49c0();
  iVar2 = iVar2 - DAT_0059a920;
  iVar3 = FUN_0049a840(param_2);
  if (*(int *)(iVar3 + 0xdc) == 0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[3] = 0;
    param_1[2] = 0;
    return;
  }
  iVar3 = FUN_00419850(0,param_3);
  iVar1 = iVar1 - (*(short *)(iVar3 + 0x1c) * DAT_0059a918 >> 0x10);
  iVar4 = *(short *)(iVar3 + 0x18) * DAT_0059a918;
  iVar2 = iVar2 - (*(short *)(iVar3 + 0x1e) * DAT_0059a918 >> 0x10);
  iVar3 = *(short *)(iVar3 + 0x1a) * DAT_0059a918;
  param_1[1] = iVar2;
  *param_1 = iVar1;
  param_1[3] = (iVar3 >> 0x10) + 1 + iVar2;
  param_1[2] = (iVar4 >> 0x10) + 1 + iVar1;
  return;
}


// ==== FUN_00419850 @ 00419850

int __thiscall FUN_00419850(int *param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  
  if ((param_1[0x3a] & 0x440000U) == 0) {
    if ((param_1[0x45] <= param_3) || (param_3 < 0)) goto LAB_004198a8;
LAB_00419887:
    if (((param_1[0x37] != 0) && (*param_1 != -0x22222223)) && (*param_1 != -0x32323233))
    goto LAB_004198d0;
  }
  else {
    if ((param_3 < param_1[0x45] * 2) && (-1 < param_3)) goto LAB_00419887;
LAB_004198a8:
    uVar1 = FUN_0049adc0(param_3,param_2);
    FUN_00436d40(uVar1);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_004198d0:
  return *(int *)(param_1[0x37] + 8 + param_3 * 8) + -0x24 + param_1[0x37];
}


// ==== FUN_00445290 @ 00445290

void __thiscall FUN_00445290(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *param_1;
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  iVar2 = param_2;
  if ((param_2 <= iVar3) && (iVar2 = iVar1, param_4 <= iVar1)) {
    iVar2 = param_4;
  }
  *param_1 = iVar2;
  iVar1 = param_1[1];
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  iVar2 = param_3;
  if ((param_3 <= iVar3) && (iVar2 = iVar1, param_5 <= iVar1)) {
    iVar2 = param_5;
  }
  param_1[1] = iVar2;
  iVar1 = param_1[2];
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  if ((param_2 <= iVar3) && (param_2 = iVar1, param_4 <= iVar1)) {
    param_2 = param_4;
  }
  param_1[2] = param_2;
  iVar1 = param_1[3];
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  if (param_3 <= iVar3) {
    if (param_5 <= iVar1) {
      param_1[3] = param_5;
      return;
    }
    param_1[3] = iVar1;
    return;
  }
  param_1[3] = param_3;
  return;
}


// ==== FUN_00497580 @ 00497580

void __thiscall FUN_00497580(int param_1,int *param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  uint *local_20;
  int local_14;
  
  if ((((*param_2 < DAT_005c78f0) && (param_2[1] < DAT_005c78f4)) && (0 < param_2[2])) &&
     (0 < param_2[3])) {
    if (99 < *(int *)(param_1 + 100)) {
      DAT_0059a8b0 = 1;
      return;
    }
    iVar6 = (param_2[3] - param_2[1]) * (param_2[2] - *param_2);
    local_14 = 0;
    if (0 < *(int *)(param_1 + 100)) {
      local_20 = (uint *)(param_1 + 0x6a8);
      piVar9 = (int *)(param_1 + 0x68);
      do {
        if (DAT_0059a8d0 != 0) break;
        uVar1 = *local_20;
        if ((uVar1 & 1) == 0) {
          iVar8 = (piVar9[2] - *piVar9) * (piVar9[3] - piVar9[1]);
          iVar2 = *piVar9;
          iVar3 = piVar9[1];
          iVar4 = piVar9[2];
          iVar5 = piVar9[3];
          FUN_004834c0(*param_2,param_2[1],param_2[2],param_2[3]);
          iVar7 = (iVar5 - iVar3) * (iVar4 - iVar2);
          if (((param_3 & 4) == 0) || (iVar7 - iVar6 != 0)) {
            if (((uVar1 & 4) != 0) && (iVar7 - iVar8 == 0)) {
              return;
            }
            if (iVar7 - ((int)(iVar7 + (iVar7 >> 0x1f & 7U)) >> 3) < iVar8 + iVar6) {
              FUN_004970a0(iVar2,iVar3,iVar4,iVar5);
              return;
            }
          }
          else {
            *local_20 = uVar1 | 1;
          }
        }
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
        piVar9 = piVar9 + 4;
      } while (local_14 < *(int *)(param_1 + 100));
    }
    piVar9 = (int *)(*(int *)(param_1 + 100) * 0x10 + 0x68 + param_1);
    *piVar9 = *param_2;
    piVar9[1] = param_2[1];
    piVar9[2] = param_2[2];
    piVar9[3] = param_2[3];
    *(uint *)(param_1 + 0x6a8 + *(int *)(param_1 + 100) * 4) = param_3 & 0xfffffffe;
    *(int *)(param_1 + 100) = *(int *)(param_1 + 100) + 1;
  }
  return;
}


// ==== FUN_004970a0 @ 004970a0

void __thiscall
FUN_004970a0(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5)

{
  *param_1 = param_2;
  param_1[1] = param_3;
  param_1[2] = param_4;
  param_1[3] = param_5;
  return;
}


// ==== FUN_004834c0 @ 004834c0

void __thiscall FUN_004834c0(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  
  iVar1 = *param_1;
  if (param_2 <= *param_1) {
    iVar1 = param_2;
  }
  *param_1 = iVar1;
  iVar1 = param_1[1];
  if (param_3 <= param_1[1]) {
    iVar1 = param_3;
  }
  param_1[1] = iVar1;
  iVar1 = param_1[2];
  if (param_1[2] <= param_4) {
    iVar1 = param_4;
  }
  param_1[2] = iVar1;
  if (param_5 < param_1[3]) {
    param_1[3] = param_1[3];
    return;
  }
  param_1[3] = param_5;
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


