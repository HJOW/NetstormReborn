// ==== FUN_0040c110 @ 0040c110

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0040c110(int param_1,uint param_2)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int extraout_ECX;
  int iVar5;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar6;
  float10 extraout_ST0;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined4 uStack_8;
  
  uStack_10 = *unaff_FS_OFFSET;
  uStack_8 = 0xffffffff;
  puStack_c = &LAB_0040c281;
  *unaff_FS_OFFSET = &uStack_10;
  if (((*(byte *)(param_1 + 0x23) & 1) == 0) && ((*(byte *)(param_1 + 0x23) & 8) == 0)) {
    if ((param_2 & 1) != 0) {
      if (DAT_00549190 < DAT_0054918c) {
        iVar5 = 0;
        if (0 < DAT_00549190) {
          piVar4 = DAT_00549188;
          do {
            if (*piVar4 == (param_1 - DAT_005395dc) / 0x24) goto LAB_0040c1b0;
            piVar4 = piVar4 + 1;
            iVar5 = iVar5 + 1;
          } while (iVar5 < DAT_00549190);
        }
        DAT_00549188[DAT_00549190] = (param_1 - DAT_005395dc) / 0x24;
        DAT_00549190 = DAT_00549190 + 1;
      }
LAB_0040c1b0:
      iVar5 = FUN_004ac890(0x25a);
      if (iVar5 == 0) {
        iVar5 = FUN_004f1650(0x28);
        uStack_8 = 0;
        if (iVar5 != 0) {
          iVar5 = FUN_00444720();
          fVar1 = (float)(iVar5 / 6) * _DAT_00500388;
          iVar2 = FUN_004aed30();
          iVar5 = 0x24;
          iVar3 = param_1 - DAT_005395dc;
          if (DAT_00546778 == 0) {
            fVar6 = (float10)fVar1 / (float10)iVar2;
          }
          else {
            iVar3 = __adj_fdivr_m32(fVar1);
            iVar5 = extraout_ECX;
            fVar6 = extraout_ST0;
          }
          FUN_0048efd0(0x25a,iVar3 / iVar5,(float)fVar6);
        }
        uStack_8 = 0xffffffff;
      }
    }
    if ((*(ushort *)(param_1 + 0xc) & 0x7f) == (ushort)*(byte *)(param_1 + 0x20)) {
      FUN_0040d9c0((ushort)*(byte *)(param_1 + 0x20),0);
    }
  }
  FUN_004e6360(param_2);
  if ((*(byte *)(param_1 + 0x23) & 9) == 0) {
    iVar5 = FUN_004e5150();
    if (iVar5 == 0) {
      FUN_0040c0d0();
    }
    else {
      if ((param_2 & 0x800) == 0) {
        iVar5 = __ftol();
        iVar2 = __ftol();
        if (((*(byte *)(iVar5 * 0x100 + iVar2 + DAT_0052fe48) & 6) == 0) &&
           (iVar5 = FUN_0040c5b0(), iVar5 == 0)) {
          FUN_0040c880();
        }
      }
      FUN_0040bfb0();
    }
  }
  *unaff_FS_OFFSET = uStack_10;
  return;
}


// ==== FUN_0040d9c0 @ 0040d9c0

void FUN_0040d9c0(int param_1,undefined4 param_2)

{
  if ((0 < param_1) && (param_1 < 9)) {
    *(undefined4 *)(&DAT_005119a0 + param_1 * 4) = param_2;
  }
  return;
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


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


