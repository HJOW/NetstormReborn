// ==== FUN_004d06a0 @ 004d06a0

void FUN_004d06a0(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004d06fa;
  *unaff_FS_OFFSET = &local_10;
  iVar1 = FUN_004f1650(0x38);
  local_8 = 0;
  if (iVar1 != 0) {
    FUN_004d0720(param_1,param_2);
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004d0720 @ 004d0720

undefined4 * __thiscall FUN_004d0720(undefined4 *param_1,undefined4 param_2,float param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar4;
  undefined4 local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_004d087c;
  *unaff_FS_OFFSET = &local_10;
  param_1[6] = 0;
  param_1[7] = 0;
  *param_1 = &PTR_LAB_00506b84;
  param_1[4] = 0;
  param_1[0xc] = 0;
  param_1[5] = 0;
  param_1[0xb] = 0;
  local_8 = 0;
  *param_1 = &PTR_LAB_00506ba8;
  *(undefined1 *)(param_1 + 2) = 0xff;
  param_1[10] = &PTR_FUN_00500d68;
  param_1[10] = &PTR_LAB_00506b88;
  if (ABS(param_3) == 0.0) {
    FUN_004eab60();
  }
  else {
    FUN_004eac30();
  }
  uVar1 = DAT_005484ac;
  uVar2 = DAT_005484a8;
  *(byte *)(param_1 + 2) = *(byte *)(param_1 + 2) | 2;
  param_1[4] = uVar2;
  param_1[5] = uVar1;
  if (ABS(param_3) == 0.0) {
    fVar4 = (float10)FUN_004d08a0();
    param_3 = (float)fVar4;
  }
  fVar4 = (float10)FUN_004d2be0();
  FUN_00413c10((double)(fVar4 + (float10)param_3));
  FUN_004d2bf0();
  FUN_0048f590(DAT_0051c9d8,param_2);
  if ((int)param_3 < 0x3f800001) {
    param_1[0xd] = 0;
  }
  else {
    uVar2 = FUN_00437d20();
    param_1[0xd] = uVar2;
    iVar3 = FUN_004f1650();
    local_8._0_1_ = 1;
    if (iVar3 != 0) {
      FUN_00453800(param_2,param_1[0xd]);
    }
    local_8 = (uint)local_8._1_3_ << 8;
  }
  FUN_0041eac0();
  *unaff_FS_OFFSET = local_10;
  return param_1;
}


// ==== FUN_004d08a0 @ 004d08a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_004d08a0(void)

{
  float10 fVar1;
  
  fVar1 = (float10)FUN_004d2be0();
  FUN_00413c10((double)(fVar1 + (float10)_DAT_00506940));
  FUN_004d2bf0();
  return (float10)_DAT_00506948;
}


// ==== FUN_004d0ba0 @ 004d0ba0

void FUN_004d0ba0(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  int *piVar5;
  int iVar6;
  
  FUN_0048f870();
  iVar1 = FUN_004aed30();
  uVar2 = FUN_0048f870();
  if (DAT_00546778 != 0) {
    FUN_004f0857(uVar2);
  }
  iVar3 = __ftol();
  if (DAT_00537eb4 == 0) {
    if (DAT_00537eb0 == 0) goto LAB_004d0c19;
    FUN_0048f870();
    uVar4 = FUN_004abae0();
    if ((uVar4 & 0x200) == 0) goto LAB_004d0c19;
  }
  iVar3 = iVar1;
LAB_004d0c19:
  iVar6 = iVar1;
  if (iVar3 <= iVar1) {
    iVar6 = iVar3;
  }
  if (iVar6 < 1) {
    iVar6 = 0;
  }
  iVar3 = iVar6;
  FUN_0048f870(iVar6);
  FUN_004aed20(iVar3);
  piVar5 = (int *)FUN_0048f870();
  (**(code **)(*piVar5 + 0x88))();
  iVar3 = FUN_0048f870();
  if ((*(byte *)(iVar3 + 0x23) & 1) == 0) {
    FUN_0048f7b0(0);
  }
  if ((iVar1 == iVar6) && (DAT_00540a2c != 0)) {
    FUN_0048f7b0(0);
  }
  return;
}


// ==== FUN_004d09e0 @ 004d09e0

void __fastcall FUN_004d09e0(int param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  
  FUN_0048f870();
  iVar2 = FUN_004aed30();
  iVar3 = FUN_0048f870();
  sVar1 = *(short *)(iVar3 + 0x1a);
  FUN_004eb310(*(undefined4 *)(param_1 + 0x30),DAT_0051ca1c,0,1);
  FUN_0041eac0();
  iVar3 = FUN_0048f870();
  if (*(byte *)(iVar3 + 0x20) == DAT_0050f6c8) {
    iVar3 = FUN_0048f870();
    iVar3 = (iVar3 - DAT_005395dc) / 0x24;
    if ((iVar3 < 5) || (5999 < iVar3)) {
      iVar3 = FUN_0048f870();
      FUN_00438c00(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),
                   s_buildDone_wav_0053fd04,0);
      FUN_0048f870();
      iVar3 = FUN_004abf50();
      if (*(int *)(iVar3 + 0x90) != 0) {
        FUN_004383f0(*(int *)(iVar3 + 0x90),0,0,0,0,0);
      }
    }
  }
  if (DAT_00540a2c != 0) {
    iVar3 = FUN_0048f870();
    iVar3 = (iVar3 - DAT_005395dc) / 0x24;
    if (((iVar3 < 5) || (5999 < iVar3)) && (iVar2 == sVar1)) {
      iVar2 = FUN_0048f870();
      if ((*(byte *)(iVar2 + 0x23) & 1) != 0) {
        piVar4 = (int *)FUN_0048f870();
        (**(code **)(*piVar4 + 0x48))(0);
        iVar2 = FUN_0048f870();
        if ((*(byte *)(iVar2 + 0xb) & 4) == 0) {
          FUN_00490040(s_isVoid___00514a24,s__Ns_o_squid_h_00531138,0x268);
        }
        *(byte *)(iVar2 + 0x23) = *(byte *)(iVar2 + 0x23) & 0xfe;
        piVar4 = (int *)FUN_0048f870();
        (**(code **)(*piVar4 + 0x4c))(9);
        FUN_0048f870();
        uVar5 = FUN_004abae0();
        if ((DAT_0053fc28 & uVar5) != 0) {
          iVar2 = FUN_0048f870();
          iVar3 = FUN_0048f870();
          FUN_004d2450(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),
                       *(undefined1 *)(iVar2 + 10));
        }
      }
    }
  }
  return;
}


// ==== FUN_004d08e0 @ 004d08e0

void __fastcall FUN_004d08e0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_004d09e0();
  iVar1 = *(int *)(param_1 + 8) * 0x24;
  if (((*(byte *)(iVar1 + 0xb + DAT_005395dc) & 3) == 0) &&
     (*(byte *)(iVar1 + DAT_005395dc + 10) != DAT_0051cbd8)) {
    FUN_0048f870();
    uVar2 = FUN_004aed30();
    FUN_004aed20(uVar2);
  }
  return;
}


// ==== FUN_004d2be0 @ 004d2be0

float10 __fastcall FUN_004d2be0(double *param_1)

{
  return (float10)*param_1;
}


// ==== FUN_00413c10 @ 00413c10

undefined4 * __thiscall FUN_00413c10(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  param_1[1] = param_3;
  *param_1 = param_2;
  return param_1;
}


// ==== FUN_004d2bf0 @ 004d2bf0

void __thiscall FUN_004d2bf0(int param_1,undefined4 param_2,undefined4 param_3)

{
  *(byte *)(param_1 + 8) = *(byte *)(param_1 + 8) | 4;
  *(undefined4 *)(param_1 + 0x18) = param_2;
  *(undefined4 *)(param_1 + 0x1c) = param_3;
  return;
}


// ==== FUN_0048f870 @ 0048f870

int __fastcall FUN_0048f870(int param_1)

{
  int iVar1;
  
  iVar1 = DAT_005395dc + *(int *)(param_1 + 8) * 0x24;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_00490040(s_parent_>getHead______0_0053133c,s__ns_o_BaseProcess_cpp_00531324,0x9f);
  }
  return iVar1;
}


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
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


