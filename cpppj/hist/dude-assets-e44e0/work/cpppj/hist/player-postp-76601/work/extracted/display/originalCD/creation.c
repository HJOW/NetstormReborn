// ==== FUN_004ae380 @ 004ae380

void __fastcall FUN_004ae380(int param_1)

{
  undefined1 uVar1;
  
  uVar1 = *(undefined1 *)(param_1 + 0x22);
  FUN_004abf50();
  FUN_00456a00(*(undefined1 *)(param_1 + 10),uVar1,*(undefined4 *)(param_1 + 0xe),
               *(undefined4 *)(param_1 + 0x12),((*(byte *)(param_1 + 0x23) & 4) == 0) - 1U & 0x10);
  return;
}


// ==== FUN_004ae3e0 @ 004ae3e0

void __fastcall FUN_004ae3e0(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ae3e2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_00456a00 @ 00456a00

void FUN_00456a00(int param_1,int param_2,undefined4 param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  int *piVar2;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined1 local_10 [16];
  
  if (DAT_0052039c == 0) {
    FUN_00456930(&local_20,param_1,param_2,param_3,param_4);
    iVar1 = DAT_00583e28;
    if (local_20 <= DAT_00583e28) {
      iVar1 = local_20;
    }
    local_20 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_20 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_1c <= DAT_00583e2c) {
      iVar1 = local_1c;
    }
    local_1c = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_1c = DAT_00583e24;
    }
    iVar1 = DAT_00583e28;
    if (local_18 <= DAT_00583e28) {
      iVar1 = local_18;
    }
    local_18 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_18 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_14 <= DAT_00583e2c) {
      iVar1 = local_14;
    }
    local_14 = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_14 = DAT_00583e24;
    }
    if ((param_5 & 0x10) != 0) {
      if (local_18 - local_20 < 0x13) {
        local_20 = local_20 + -9;
        local_18 = local_18 + 9;
      }
      local_1c = local_1c + -9;
      local_20 = local_20 + -3;
      local_18 = local_18 + 3;
    }
    if ((local_18 != local_20 && -1 < local_18 - local_20) &&
       (local_14 != local_1c && -1 < local_14 - local_1c)) {
      FUN_00457320(&local_20,param_5);
    }
    iVar1 = DAT_0051c960 + param_1 * 0x1d4;
    if ((*(byte *)(iVar1 + 0xea) & 4) != 0) {
      piVar2 = (int *)FUN_00456930(local_10,param_1,*(int *)(iVar1 + 0x114) + param_2,param_3,
                                   param_4);
      local_20 = DAT_00583e28;
      if (*piVar2 <= DAT_00583e28) {
        local_20 = *piVar2;
      }
      if (local_20 <= DAT_00583e20) {
        local_20 = DAT_00583e20;
      }
      local_1c = DAT_00583e2c;
      if (piVar2[1] <= DAT_00583e2c) {
        local_1c = piVar2[1];
      }
      if (local_1c <= DAT_00583e24) {
        local_1c = DAT_00583e24;
      }
      local_18 = DAT_00583e28;
      if (piVar2[2] <= DAT_00583e28) {
        local_18 = piVar2[2];
      }
      if (local_18 <= DAT_00583e20) {
        local_18 = DAT_00583e20;
      }
      local_14 = DAT_00583e2c;
      if (piVar2[3] <= DAT_00583e2c) {
        local_14 = piVar2[3];
      }
      if (local_14 <= DAT_00583e24) {
        local_14 = DAT_00583e24;
      }
      if ((local_18 != local_20 && -1 < local_18 - local_20) &&
         (local_14 != local_1c && -1 < local_14 - local_1c)) {
        FUN_00457320(&local_20,param_5);
      }
    }
  }
  return;
}


// ==== FUN_00456930 @ 00456930

void FUN_00456930(int *param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int local_10;
  int local_c;
  undefined1 local_8 [8];
  
  FUN_00455c90(local_8,param_4,param_5);
  iVar2 = *(int *)(param_2 * 0x1d4 + 0xdc + DAT_0051c960);
  if (iVar2 == 0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[2] = 0;
    param_1[3] = 0;
    return;
  }
  iVar3 = *(int *)(iVar2 + 8 + param_3 * 8);
  iVar4 = iVar3 + -0x24 + iVar2;
  iVar1 = *(short *)(iVar4 + 0x18) * DAT_00565c30;
  iVar3 = local_10 - (*(short *)(iVar3 + -8 + iVar2) * DAT_00565c30 >> 0x10);
  iVar2 = *(short *)(iVar4 + 0x1a) * DAT_00565c30;
  iVar4 = local_c - (*(short *)(iVar4 + 0x1e) * DAT_00565c30 >> 0x10);
  *param_1 = iVar3;
  param_1[1] = iVar4;
  param_1[2] = (iVar1 >> 0x10) + iVar3 + 1;
  param_1[3] = iVar4 + (iVar2 >> 0x10) + 1;
  return;
}


// ==== FUN_00455c90 @ 00455c90

void __thiscall FUN_00455c90(int *param_1,int *param_2)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = iVar1 - DAT_00565c34;
  iVar1 = __ftol();
  iVar1 = iVar1 - DAT_00565c38;
  param_1[1] = iVar1;
  *param_2 = *param_1;
  param_2[1] = iVar1;
  return;
}


// ==== FUN_00457320 @ 00457320

void __thiscall FUN_00457320(int param_1,int *param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  int iVar12;
  int iVar13;
  uint *local_20;
  int local_14;
  
  iVar1 = *param_2;
  if ((((iVar1 < DAT_00516b10) && (iVar2 = param_2[1], iVar2 < DAT_00516b14)) &&
      (iVar3 = param_2[2], 0 < iVar3)) && (iVar12 = param_2[3], 0 < iVar12)) {
    if (99 < *(int *)(param_1 + 100)) {
      DAT_0052039c = 1;
      return;
    }
    iVar8 = (iVar12 - iVar2) * (iVar3 - iVar1);
    local_14 = 0;
    if (0 < *(int *)(param_1 + 100)) {
      piVar11 = (int *)(param_1 + 0x68);
      local_20 = (uint *)(param_1 + 0x6a8);
      do {
        if (DAT_005203bc != 0) break;
        uVar4 = *local_20;
        if ((uVar4 & 1) == 0) {
          iVar5 = (piVar11[3] - piVar11[1]) * (piVar11[2] - *piVar11);
          iVar13 = *piVar11;
          if (iVar1 <= *piVar11) {
            iVar13 = iVar1;
          }
          iVar10 = piVar11[1];
          if (iVar2 <= piVar11[1]) {
            iVar10 = iVar2;
          }
          iVar6 = piVar11[2];
          if (piVar11[2] <= iVar3) {
            iVar6 = iVar3;
          }
          iVar7 = piVar11[3];
          if (piVar11[3] <= iVar12) {
            iVar7 = iVar12;
          }
          iVar9 = (iVar7 - iVar10) * (iVar6 - iVar13);
          if (((param_3 & 4) == 0) || (iVar8 != iVar9)) {
            if (((uVar4 & 4) != 0) && (iVar9 - iVar5 == 0)) {
              return;
            }
            if (iVar9 - ((int)(iVar9 + (iVar9 >> 0x1f & 7U)) >> 3) < iVar8 + iVar5) {
              *piVar11 = iVar13;
              piVar11[1] = iVar10;
              piVar11[2] = iVar6;
              piVar11[3] = iVar7;
              return;
            }
          }
          else {
            *local_20 = uVar4 | 1;
          }
        }
        piVar11 = piVar11 + 4;
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
      } while (local_14 < *(int *)(param_1 + 100));
    }
    iVar12 = *(int *)(param_1 + 100) * 0x10 + param_1;
    iVar1 = param_2[1];
    *(int *)(iVar12 + 0x68) = *param_2;
    iVar2 = param_2[2];
    iVar3 = param_2[3];
    *(int *)(iVar12 + 0x6c) = iVar1;
    *(int *)(iVar12 + 0x70) = iVar2;
    *(int *)(iVar12 + 0x74) = iVar3;
    *(uint *)(param_1 + 0x6a8 + *(int *)(param_1 + 100) * 4) = param_3 & 0xfffffffe;
    *(int *)(param_1 + 100) = *(int *)(param_1 + 100) + 1;
  }
  return;
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


