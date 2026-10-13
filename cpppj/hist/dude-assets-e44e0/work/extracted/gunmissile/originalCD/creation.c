// ==== FUN_0044a960 @ 0044a960

void __cdecl
FUN_0044a960(float *param_1,float param_2,float param_3,float param_4,float param_5,int param_6)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int local_8 [2];
  
  iVar2 = FUN_0043f620(&param_2,param_4,param_5,1);
  iVar6 = DAT_005395dc + param_6 * 0x24;
  if (*(byte *)(iVar6 + 10) == DAT_0051cb44) {
    iVar4 = FUN_004a1050(iVar2,*(undefined1 *)(iVar6 + 0x22));
    iVar3 = iVar2;
    if (iVar4 == 0) {
      iVar3 = 0;
      do {
        iVar4 = FUN_004a1050(iVar3,*(undefined1 *)(iVar6 + 0x22));
        if (iVar4 != 0) break;
        iVar3 = iVar3 + 2;
      } while (iVar3 < 7);
    }
  }
  else {
    iVar3 = FUN_00443110(iVar6);
    FUN_004aef30(iVar6,local_8);
  }
  if (iVar3 != iVar2) {
    param_4 = param_2;
    param_5 = param_3;
    uVar5 = FUN_004abf50(iVar6);
    FUN_004447a0(uVar5);
    iVar2 = __ftol();
    local_8[0] = (&DAT_0051c350)[iVar3] * iVar2;
    param_4 = (float)((&DAT_0051c330)[iVar3] * iVar2) + param_4;
    param_5 = (float)local_8[0] + param_5;
    FUN_0043f950(&param_4);
  }
  switch(iVar3) {
  case 0:
  case 4:
    *param_1 = param_2;
    param_1[1] = param_5;
    return;
  default:
    FUN_00490040(s_false_0051f148,s_Missile_cpp_0051f13c,0x4b);
    fVar1 = DAT_0056549c;
    *param_1 = DAT_00565498;
    param_1[1] = fVar1;
    return;
  case 2:
  case 6:
    *param_1 = param_4;
    param_1[1] = param_3;
    return;
  }
}


// ==== FUN_0044aae0 @ 0044aae0

void __cdecl FUN_0044aae0(undefined4 *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar4 = DAT_005395dc + param_2 * 0x24;
  iVar3 = FUN_004abf50(iVar4);
  FUN_004449d0(iVar3,&local_10,*(undefined4 *)(iVar4 + 0xe),*(undefined4 *)(iVar4 + 0x12),0);
  iVar1 = *(int *)((uint)*(byte *)(iVar4 + 10) * 0x1d4 + 0xdc + DAT_0051c960);
  iVar2 = *(int *)(iVar1 + 8 + (uint)*(byte *)(iVar4 + 0x22) * 8);
  iVar4 = iVar2 + -0x24 + iVar1;
  FUN_0043fa10(&local_8,(int)*(short *)(iVar4 + 0x20),(int)*(short *)(iVar2 + -2 + iVar1));
  FUN_0043fa10(&local_10,(int)*(short *)(iVar4 + 0x20),(int)*(short *)(iVar4 + 0x22));
  FUN_0043fa10(&local_8,-*(int *)(iVar3 + 0x1bc),-*(int *)(iVar3 + 0x1c0));
  FUN_0043fa10(&local_10,-*(int *)(iVar3 + 0x1bc),-*(int *)(iVar3 + 0x1c0));
  *param_1 = local_10;
  param_1[1] = local_c;
  param_1[2] = local_8;
  param_1[3] = local_4;
  return;
}


// ==== FUN_0044ac70 @ 0044ac70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall
FUN_0044ac70(void *this,int param_2,int param_3,undefined4 param_4,undefined4 param_5,int param_6,
            float param_7,float param_8,float param_9,float param_10,int param_11,float param_12,
            float param_13)

{
  float *pfVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  undefined1 uVar6;
  int iVar7;
  float *pfVar8;
  undefined4 *puVar9;
  undefined4 uVar10;
  undefined4 extraout_ECX;
  float *pfVar11;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar12;
  float10 extraout_ST0;
  int local_28;
  int local_24;
  int *local_20;
  undefined4 local_1c;
  float local_18;
  int *local_14;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_0044b24c;
  *unaff_FS_OFFSET = &local_10;
  *(undefined ***)this = &PTR_FUN_00500d68;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  local_8 = 0;
  *(undefined4 *)((int)this + 0x44) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x40) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x4c) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x48) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x50) = 0;
  *(undefined4 *)((int)this + 0x54) = 0;
  *(undefined4 *)((int)this + 0x58) = 0;
  *(undefined4 *)((int)this + 0x5c) = 0;
  *(undefined ***)this = &PTR_FUN_00501ca0;
  *(int *)((int)this + 0x14) = param_2;
  *(int *)((int)this + 0x18) = param_3;
  *(undefined4 *)((int)this + 0x20) = param_4;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(int *)((int)this + 0xc) = param_11;
  *(undefined4 *)((int)this + 100) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x60) = 0xc61c4000;
  local_20 = (int *)(param_2 * 0x24 + DAT_005395dc);
  local_14 = (int *)(param_3 * 0x24 + DAT_005395dc);
  if (((((int)param_12 < 1) || (local_18 = (float)DAT_0052e9a8, local_18 <= param_12)) ||
      ((int)param_13 < 1)) || (local_18 <= param_13)) {
    puVar9 = (undefined4 *)(**(code **)(*local_20 + 0xb4))(&local_1c);
    uVar10 = puVar9[1];
    *(undefined4 *)((int)this + 0x28) = *puVar9;
    *(undefined4 *)((int)this + 0x2c) = uVar10;
  }
  else {
    if (param_11 == 0) {
      FUN_00490040(s_riccochet_0051f15c,s_Missile_cpp_0051f150,0x7e);
    }
    *(float *)((int)this + 0x28) = param_12;
    *(float *)((int)this + 0x2c) = param_13;
  }
  pfVar11 = (float *)((int)this + 0x28);
  iVar7 = FUN_004abf50(local_20);
  if ((*(int *)((int)this + 0xc) == 0) && (DAT_0051ca60 != *(byte *)((int)local_20 + 10))) {
    iVar3 = *(int *)((uint)*(byte *)((int)local_20 + 10) * 0x1d4 + 0xdc + DAT_0051c960);
    iVar4 = *(int *)(iVar3 + 8 + (uint)*(byte *)((int)local_20 + 0x22) * 8);
    FUN_0043fa10(pfVar11,(int)*(short *)(iVar4 + iVar3 + -4),(int)*(short *)(iVar4 + -2 + iVar3));
    FUN_0043fa10(pfVar11,-*(int *)(iVar7 + 0x1bc),-*(int *)(iVar7 + 0x1c0));
  }
  bVar2 = *(byte *)((int)local_14 + 10);
  if ((bVar2 < 10) || (DAT_0051cbf0 <= (int)(uint)bVar2)) {
    local_24 = 3;
    local_28 = 3;
  }
  else {
    FUN_004442b0((uint)bVar2 * 0x1d4 + DAT_0051c960,&local_28,&local_24,1);
  }
  if ((int)param_7 < 1) {
LAB_0044ae5f:
    FUN_00490040(s__targetPos_isValid___0051f174,s_Missile_cpp_0051f168,0x96);
  }
  else {
    local_18 = (float)DAT_0052e9a8;
    if (((local_18 <= param_7) || ((int)param_8 < 1)) || (local_18 <= param_8)) goto LAB_0044ae5f;
  }
  iVar7 = DAT_0052e9a8;
  pfVar1 = (float *)((int)this + 0x38);
  fVar5 = (float)local_24 * _DAT_00501c64;
  *pfVar1 = (float)local_28 * _DAT_00501c64 + param_7;
  *(float *)((int)this + 0x3c) = fVar5 + param_8;
  if (*(int *)((int)this + 0xc) == 0) {
    *(undefined4 *)((int)this + 0x1c) = 0;
    if (0 < (int)param_9) {
      local_18 = (float)iVar7;
      if (((param_9 < local_18) && (0 < (int)param_10)) && (param_10 < local_18)) {
        iVar7 = FUN_004ac420(local_20);
        if (iVar7 == 1) {
          pfVar8 = (float *)FUN_0044a960(&local_1c,*pfVar11,*(undefined4 *)((int)this + 0x2c),
                                         param_9,param_10,((int)local_20 - DAT_005395dc) / 0x24);
          fVar5 = pfVar8[1];
          *pfVar1 = *pfVar8;
          *(float *)((int)this + 0x3c) = fVar5;
        }
        *(undefined4 *)((int)this + 0x1c) = 1;
        *(float *)((int)this + 0x30) = param_9;
        *(float *)((int)this + 0x34) = param_10;
        goto LAB_0044afab;
      }
    }
    *(float *)((int)this + 0x30) = param_7;
    *(float *)((int)this + 0x34) = param_8;
    iVar7 = FUN_004ac420(local_20);
    if (iVar7 == 1) {
      pfVar8 = (float *)FUN_0044a960(&local_1c,*pfVar11,*(undefined4 *)((int)this + 0x2c),*pfVar1,
                                     *(undefined4 *)((int)this + 0x3c),
                                     ((int)local_20 - DAT_005395dc) / 0x24);
      fVar5 = pfVar8[1];
      *pfVar1 = *pfVar8;
      *(float *)((int)this + 0x3c) = fVar5;
    }
  }
  else {
    *(undefined4 *)((int)this + 0x1c) = 0;
    *(float *)((int)this + 0x30) = param_7;
    *(float *)((int)this + 0x34) = param_8;
  }
LAB_0044afab:
  FUN_0043f950(pfVar1);
  FUN_0043f950(pfVar11);
  if (((*(byte *)((int)local_20 + 0xb) & 1) != 0) && ((*(byte *)((int)local_14 + 0xb) & 1) != 0)) {
    FUN_00490040(s__firing_isFree_______target_isFr_0051f198,s_Missile_cpp_0051f18c,0xb8);
  }
  if ((int)*pfVar11 < 1) {
LAB_0044b012:
    FUN_00490040(s_firingPos_isValid___0051f1cc,s_Missile_cpp_0051f1c0,0xb9);
  }
  else {
    local_14 = (int *)(float)DAT_0052e9a8;
    if ((((float)local_14 <= *pfVar11) || (*(int *)((int)this + 0x2c) < 1)) ||
       ((float)local_14 <= *(float *)((int)this + 0x2c))) goto LAB_0044b012;
  }
  if (0 < (int)*pfVar1) {
    local_14 = (int *)(float)DAT_0052e9a8;
    if (((*pfVar1 < (float)local_14) && (0 < *(int *)((int)this + 0x3c))) &&
       (*(float *)((int)this + 0x3c) < (float)local_14)) goto LAB_0044b070;
  }
  FUN_00490040(s_targetCenter_isValid___0051f1ec,s_Missile_cpp_0051f1e0,0xba);
LAB_0044b070:
  *(undefined4 *)((int)this + 0x24) = 0;
  fVar12 = (float10)FUN_0043f750(pfVar1,*pfVar11,*(undefined4 *)((int)this + 0x2c));
  local_14 = (int *)(float)fVar12;
  if ((fVar12 < (float10)_DAT_00501c58) || ((float)DAT_0052e9a8 <= (float)local_14)) {
    FUN_00490040(s_dist_>__0_0____dist_<_boardDimIn_0051f210,s_Missile_cpp_0051f204,0xca);
  }
  local_1c = DAT_005484a8;
  puVar9 = (undefined4 *)((int)this + 0x50);
  local_18 = (float)DAT_005484ac;
  if (DAT_00546778 == 0) {
    fVar12 = (float10)(float)local_14 / (float10)*(float *)((int)this + 0x20);
    uVar10 = DAT_005484a8;
  }
  else {
    puVar9 = (undefined4 *)__adj_fdiv_m32(*(undefined4 *)((int)this + 0x20));
    uVar10 = extraout_ECX;
    fVar12 = extraout_ST0;
  }
  *puVar9 = uVar10;
  puVar9[1] = local_18;
  *(undefined4 *)((int)this + 0x58) = uVar10;
  *(float *)((int)this + 0x5c) = local_18;
  *(double *)((int)this + 0x50) = (double)(fVar12 + (float10)*(double *)((int)this + 0x50));
  local_14 = (int *)FUN_004ab390(param_5,2);
  local_18 = (float)(DAT_0051c960 + (uint)*(byte *)((int)local_14 + 10) * 0x1d4);
  if (0 < *(int *)((int)local_18 + 0x110)) {
    if (DAT_0051ca90 == *(byte *)((int)local_14 + 10)) {
      *(undefined1 *)((int)local_14 + 0x22) = 1;
    }
    else {
      uVar10 = FUN_004abf50();
      uVar6 = FUN_004409a0(*pfVar11,*(undefined4 *)((int)this + 0x2c),*pfVar1,
                           *(undefined4 *)((int)this + 0x3c),uVar10);
      *(undefined1 *)((int)local_14 + 0x22) = uVar6;
    }
  }
  (**(code **)(*local_14 + 0x74))((char)local_20[8]);
  iVar7 = *(int *)((uint)*(byte *)((int)local_20 + 10) * 0x1d4 + 0x70 + DAT_0051c960);
  if ((iVar7 != 0) || (iVar7 = *(int *)((int)local_18 + 0x70), iVar7 != 0)) {
    FUN_00438ae0(*pfVar11,*(undefined4 *)((int)this + 0x2c),iVar7,0,0);
  }
  (**(code **)(*local_14 + 0x90))(*pfVar11,*(undefined4 *)((int)this + 0x2c),0);
  if (param_6 != 0) {
    (**(code **)(param_6 * 0x1d4 + 0x1b0 + DAT_0051c960))(((int)local_14 - DAT_005395dc) / 0x24,0);
  }
  FUN_0048f590(this,DAT_0051c9d0,((int)local_14 - DAT_005395dc) / 0x24,0,0x10);
  *unaff_FS_OFFSET = local_10;
  return this;
}


// ==== FUN_0044bc50 @ 0044bc50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float * __thiscall FUN_0044bc50(void *this,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  float10 fVar6;
  float10 fVar7;
  float local_18;
  float local_14;
  float fStack_8;
  float fStack_4;
  
  iVar4 = FUN_0048f870();
  fVar1 = *(float *)(iVar4 + 0x12);
  fVar2 = *(float *)(iVar4 + 0xe);
  if ((*(float *)((int)this + 0x28) == fVar2) && (*(float *)((int)this + 0x2c) == fVar1)) {
    return param_2;
  }
  fVar6 = (float10)FUN_0043f530(fVar2,fVar1);
  iVar5 = FUN_0048cc10(0,100);
  fVar3 = _DAT_00501c68;
  if (0x31 < iVar5) {
    fVar3 = _DAT_00501c78;
  }
  fVar7 = (float10)FUN_0048cc50(0x41f00000,0x43160000);
  fVar6 = fVar7 * (float10)fVar3 + (float10)(float)fVar6;
  local_18 = (float)fVar6;
  if (fVar6 < (float10)_DAT_00501c60) {
    iVar5 = __ftol();
    local_18 = (float)(iVar5 + 1) * _DAT_00501c88 + local_18;
  }
  if ((int)local_18 < 0x43b40001) {
    local_14 = local_18;
  }
  else {
    fVar6 = (float10)FUN_004f28fa();
    local_14 = (float)fVar6;
  }
  iVar4 = FUN_004abf50(iVar4);
  fVar6 = (float10)fcos((float10)local_14);
  fStack_8 = (float)(fVar6 * (float10)*(float *)(iVar4 + 0x44) + (float10)fVar2);
  fVar6 = (float10)fsin((float10)local_14);
  fStack_4 = (float)(fVar6 * (float10)*(float *)(iVar4 + 0x44) + (float10)fVar1);
  FUN_0043f950(&fStack_8);
  *param_2 = fStack_8;
  param_2[1] = fStack_4;
  return param_2;
}


// ==== FUN_0044bda0 @ 0044bda0

void __thiscall FUN_0044bda0(void *this)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar4;
  undefined **local_70 [13];
  int local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  int local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0044bf8e;
  *unaff_FS_OFFSET = &local_10;
  iVar1 = FUN_0048cc10(0,99);
  if (0x13 < iVar1) {
    local_28 = FUN_0048f870(this);
    FUN_0044bc50(this,&local_24);
    if ((((0 < (int)local_24) && (local_24 < (float)DAT_0052e9a8)) && (0 < (int)local_20)) &&
       (local_20 < (float)DAT_0052e9a8)) {
      iVar1 = 0;
      local_1c = local_24;
      local_18 = local_20;
      local_38 = 0;
      local_34 = 0;
      local_14 = 10000.0;
      local_70[0] = &PTR_FUN_005003d0;
      FUN_00440a80(&local_1c);
      iVar2 = __ftol();
      iVar3 = __ftol();
      FUN_004eae20(local_70,iVar3 + -1,iVar2 + -1,iVar3 + 1,iVar2 + 1);
      while (local_3c != 0) {
        iVar2 = local_3c * 0x24 + DAT_005395dc;
        local_18 = (float)FUN_004abae0(iVar2);
        iVar3 = FUN_004aecf0(iVar2);
        if ((((iVar3 != 0) && ((iVar3 = (iVar2 - DAT_005395dc) / 0x24, iVar3 < 5 || (5999 < iVar3)))
             ) && ((*(byte *)(iVar2 + 0x23) & 1) == 0)) &&
           ((((uint)local_18 & 0x120000) == 0 && (*(int *)((int)this + 0x18) != iVar3)))) {
          local_30 = *(undefined4 *)(iVar2 + 0xe);
          local_2c = *(undefined4 *)(iVar2 + 0x12);
          fVar4 = (float10)FUN_0043f750(&local_30,local_24,local_20);
          local_18 = (float)fVar4;
          if (fVar4 < (float10)local_14) {
            iVar1 = local_3c;
            local_14 = local_18;
          }
        }
        FUN_004eafe0(local_70);
      }
      local_14 = (float)FUN_004f1650(0x68);
      local_8 = 0;
      if (local_14 != 0.0) {
        local_1c = *(float *)(local_28 + 0xe);
        FUN_0044ac70(local_14,*(undefined4 *)((int)this + 8),iVar1,*(undefined4 *)((int)this + 0x20)
                     ,*(undefined1 *)(local_28 + 10),0,local_24,local_20,DAT_00565498,DAT_0056549c,1
                     ,local_1c,*(undefined4 *)(local_28 + 0x12));
      }
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0044b260 @ 0044b260

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0044b260(void *this)

{
  int *piVar1;
  int *piVar2;
  byte bVar3;
  undefined1 uVar4;
  float fVar5;
  bool bVar6;
  uint uVar7;
  float *pfVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined4 uVar11;
  undefined4 unaff_EBX;
  float unaff_EBP;
  float *pfVar12;
  float unaff_EDI;
  bool bVar13;
  float10 fVar14;
  undefined4 uVar15;
  float fVar16;
  float fVar17;
  float local_130;
  float local_12c;
  float fStack_124;
  float fStack_120;
  int iStack_11c;
  float local_118;
  float local_114;
  float fStack_110;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  
  piVar1 = (int *)(*(int *)((int)this + 8) * 0x24 + DAT_005395dc);
  piVar2 = (int *)(*(int *)((int)this + 0x18) * 0x24 + DAT_005395dc);
  local_10c = (float)(*(int *)((int)this + 0x14) * 0x24 + DAT_005395dc);
  local_104 = (float)((float10)*(double *)((int)this + 0x50) - (float10)_DAT_005484a8);
  local_118 = (float)((float10)*(double *)((int)this + 0x50) -
                     (float10)*(double *)((int)this + 0x58));
  local_108 = 0.0;
  local_114 = *(float *)((int)this + 0x3c) - *(float *)((int)this + 0x2c);
  iVar9 = _DAT_0051f134;
  if ((*(byte *)((int)piVar1 + 10) == DAT_0051cb54) ||
     (iVar9 = _DAT_0051f130, *(int *)((int)this + 0xc) == 0)) {
    local_108 = (float)iVar9;
  }
  if (DAT_00546778 == 0) {
    fVar14 = (float10)(*(float *)((int)this + 0x38) - *(float *)((int)this + 0x28)) /
             (float10)local_118;
  }
  else {
    fVar14 = (float10)__adj_fdiv_m32(local_118);
  }
  fVar16 = (float)((float10)*(float *)((int)this + 0x38) - fVar14 * (float10)local_104);
  if (DAT_00546778 == 0) {
    fVar14 = (float10)local_114 / (float10)local_118;
  }
  else {
    fVar14 = (float10)__adj_fdiv_m32(local_118);
  }
  fVar17 = (float)((float10)*(float *)((int)this + 0x3c) - fVar14 * (float10)local_104);
  if (((((int)fVar16 < 1) || (local_100 = (float)DAT_0052e9a8, local_100 <= fVar16)) ||
      ((int)fVar17 < 1)) || (local_100 <= fVar17)) {
    (**(code **)(*piVar1 + 0x10))(0);
    return;
  }
  (**(code **)(*piVar1 + 0x48))(0);
  (**(code **)(*piVar1 + 0x90))(unaff_EBX,fVar16,0);
  if (((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)piVar2 + 0xb) & 2) == 0)) &&
     ((*(byte *)((int)piVar2 + 0xb) & 1) == 0)) {
    if (*(int *)((int)this + 0x1c) == 0) {
      bVar3 = *(byte *)((int)piVar2 + 10);
      if ((bVar3 < 10) || (DAT_0051cbf0 <= (int)(uint)bVar3)) {
        uVar15 = *(undefined4 *)((int)this + 0x34);
        uVar11 = *(undefined4 *)((int)this + 0x30);
        uVar7 = DAT_0051cb44;
      }
      else {
        uVar11 = *(undefined4 *)((int)this + 0x30);
        uVar15 = *(undefined4 *)((int)this + 0x34);
        uVar7 = (uint)bVar3;
      }
      pfVar12 = (float *)((int)this + 0x40);
      pfVar8 = (float *)FUN_00444b50(uVar7 * 0x1d4 + DAT_0051c960,&fStack_110,uVar11,uVar15);
      *pfVar12 = *pfVar8;
      *(float *)((int)this + 0x44) = pfVar8[1];
      *(float *)((int)this + 0x48) = pfVar8[2];
      *(float *)((int)this + 0x4c) = pfVar8[3];
      *pfVar12 = *pfVar12 - *(float *)((int)this + 0x24);
      *(float *)((int)this + 0x44) = *(float *)((int)this + 0x44) - *(float *)((int)this + 0x24);
      *(float *)((int)this + 0x48) = *(float *)((int)this + 0x24) + *(float *)((int)this + 0x48);
      *(float *)((int)this + 0x4c) = *(float *)((int)this + 0x24) + *(float *)((int)this + 0x4c);
    }
    else {
      pfVar8 = (float *)((int)this + 0x40);
      *(undefined4 *)((int)this + 0x44) = 0xc61c4000;
      fVar16 = *(float *)((int)this + 0x38);
      fVar17 = *(float *)((int)this + 0x3c);
      *pfVar8 = -10000.0;
      *(undefined4 *)((int)this + 0x4c) = 0xc61c4000;
      *(undefined4 *)((int)this + 0x48) = 0xc61c4000;
      fStack_110 = fVar16;
      local_10c = fVar17;
      FUN_0043f950(&stack0xfffffec8);
      if (((((int)*pfVar8 < 1) || (fStack_124 = (float)DAT_0052e9a8, fStack_124 <= *pfVar8)) ||
          (*(int *)((int)this + 0x44) < 1)) || (fStack_124 <= *(float *)((int)this + 0x44))) {
        *pfVar8 = fVar16;
        *(float *)((int)this + 0x44) = fVar17;
      }
      else {
        fVar5 = *pfVar8;
        if (fVar16 <= *pfVar8) {
          fVar5 = fVar16;
        }
        *pfVar8 = fVar5;
        fVar16 = *(float *)((int)this + 0x44);
        if (fVar17 <= *(float *)((int)this + 0x44)) {
          fVar16 = fVar17;
        }
        *(float *)((int)this + 0x44) = fVar16;
      }
      FUN_0043f950(&fStack_110);
      if (((*(int *)((int)this + 0x48) < 1) || ((float)DAT_0052e9a8 <= *(float *)((int)this + 0x48))
          ) || ((*(int *)((int)this + 0x4c) < 1 ||
                ((float)DAT_0052e9a8 <= *(float *)((int)this + 0x4c))))) {
        *(float *)((int)this + 0x48) = fStack_110;
        *(float *)((int)this + 0x4c) = local_10c;
      }
      else {
        fVar16 = *(float *)((int)this + 0x48);
        if (*(float *)((int)this + 0x48) <= fStack_110) {
          fVar16 = fStack_110;
        }
        *(float *)((int)this + 0x48) = fVar16;
        fVar16 = *(float *)((int)this + 0x4c);
        if (*(float *)((int)this + 0x4c) <= local_10c) {
          fVar16 = local_10c;
        }
        *(float *)((int)this + 0x4c) = fVar16;
      }
      *pfVar8 = *pfVar8 - _DAT_00501c68;
      *(float *)((int)this + 0x44) = *(float *)((int)this + 0x44) - _DAT_00501c68;
    }
  }
  bVar6 = false;
  if (((0 < *(int *)((int)this + 0x60)) &&
      (fStack_110 = (float)DAT_0052e9a8, *(float *)((int)this + 0x60) < fStack_110)) &&
     ((0 < *(int *)((int)this + 100) && (*(float *)((int)this + 100) < fStack_110)))) {
    fStack_120 = *(float *)((int)this + 100);
    fStack_124 = *(float *)((int)this + 0x60);
    fStack_110 = fStack_124;
    local_10c = fStack_120;
    FUN_0043f950(&fStack_124);
    fVar16 = fStack_124;
    fVar17 = fStack_120;
    if ((((0 < (int)unaff_EBP) && (unaff_EBP < (float)DAT_0052e9a8)) && (0 < (int)unaff_EDI)) &&
       (unaff_EDI < (float)DAT_0052e9a8)) {
      fVar16 = unaff_EBP;
      if (fStack_124 <= unaff_EBP) {
        fVar16 = fStack_124;
      }
      fVar17 = unaff_EDI;
      if (fStack_120 <= unaff_EDI) {
        fVar17 = fStack_120;
      }
    }
    FUN_0043f950(&fStack_110);
    if ((((int)unaff_EBP < 1) || (fStack_124 = (float)DAT_0052e9a8, fStack_124 <= unaff_EBP)) ||
       (((int)unaff_EDI < 1 || (fStack_124 <= unaff_EDI)))) {
      local_130 = fStack_110;
      local_12c = local_10c;
    }
    else {
      local_130 = unaff_EBP;
      if (unaff_EBP <= fStack_110) {
        local_130 = fStack_110;
      }
      local_12c = local_10c;
      if (local_10c <= unaff_EDI) {
        local_12c = unaff_EDI;
      }
    }
    if ((DAT_0052e830 != 0) &&
       (((*(byte *)(iStack_11c + 0x23) & 4) != 0 || ((*(byte *)((int)piVar2 + 0x23) & 4) != 0)))) {
      FUN_00440920(&stack0xfffffec8,DAT_0055bad0);
      FUN_00440920((int)this + 0x40,DAT_0055bad0);
    }
    if (*(int *)((int)this + 0x10) == 0) {
      local_10c = *(float *)((int)this + 0x44);
      fStack_110 = *(float *)((int)this + 0x40);
      local_108 = *(float *)((int)this + 0x48);
      local_104 = *(float *)((int)this + 0x4c);
      if ((((((fVar16 <= fStack_110) || (fVar16 <= local_108)) &&
            ((fStack_110 <= local_130 || (local_108 <= local_130)))) &&
           ((fVar17 <= local_10c || (fVar17 <= local_104)))) &&
          ((local_10c <= local_12c || (local_104 <= local_12c)))) &&
         (bVar6 = true, (*(byte *)((int)piVar2 + 0x23) & 0x10) == 0)) goto LAB_0044b819;
    }
    bVar6 = false;
  }
LAB_0044b819:
  if (*(int *)((int)this + 0xc) == 0) {
    if (*(int *)((int)this + 0x1c) == 0) {
      puVar10 = (undefined4 *)FUN_004aef30(iStack_11c,&fStack_110);
      fVar14 = (float10)FUN_0043f750(&stack0xfffffec0,*puVar10,puVar10[1]);
      iVar9 = FUN_004abf50(iStack_11c);
      bVar13 = true;
      if ((float)fVar14 <= *(float *)(iVar9 + 0x44) + local_118) goto LAB_0044b910;
    }
    else {
      puVar10 = (undefined4 *)FUN_004aef30(iStack_11c,&stack0xfffffec8);
      fVar14 = (float10)FUN_0043f750((int)this + 0x38,*puVar10,puVar10[1]);
      fStack_124 = (float)(fVar14 + (float10)local_118);
      puVar10 = (undefined4 *)FUN_004aef30(iStack_11c,&fStack_110);
      fVar14 = (float10)FUN_0043f750(&stack0xfffffec0,*puVar10,puVar10[1]);
      bVar13 = (float10)fStack_124 < fVar14;
    }
  }
  else {
    fVar14 = (float10)FUN_0043f750(&stack0xfffffec0,*(undefined4 *)((int)this + 0x28),
                                   *(undefined4 *)((int)this + 0x2c));
    fStack_110 = (float)fVar14;
    iVar9 = FUN_004abf50(piVar1);
    bVar13 = true;
    if (fStack_110 <= *(float *)(iVar9 + 0x44) + local_118) {
LAB_0044b910:
      bVar13 = false;
    }
  }
  if (bVar6) {
    if ((*(byte *)((int)piVar2 + 10) == DAT_0051cb8c) &&
       (fVar14 = (float10)FUN_0048cc50(0,0x447a0000), fVar14 < (float10)_DAT_0051f138)) {
      *(undefined4 *)((int)this + 0x10) = 1;
      bVar6 = false;
    }
    if (bVar6) {
      unaff_EBP = (*(float *)((int)this + 0x60) + unaff_EBP) * _DAT_00501c4c;
      unaff_EDI = (*(float *)((int)this + 100) + unaff_EDI) * _DAT_00501c4c;
      if ((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)piVar2 + 0xb) & 1) == 0)) {
        if ((*(int *)((int)this + 0x1c) == 0) && (*(byte *)((int)piVar1 + 10) != DAT_0051cb4c)) {
          FUN_00455b40(unaff_EBP,unaff_EDI);
        }
        else {
          FUN_0047c8a0(unaff_EBP,unaff_EDI,2,0x3ca3d70a);
        }
        if (*(int *)((int)this + 0x1c) == 0) {
          iVar9 = *(int *)(DAT_0051c960 + (uint)*(byte *)((int)piVar1 + 10) * 0x1d4 + 0x74);
          if (iVar9 != 0) {
            if (DAT_0051cb4c != *(byte *)((int)piVar1 + 10)) goto LAB_0044ba81;
            iVar9 = FUN_0048cc10(1000,5000);
            uVar11 = FUN_00420560(&fStack_110,s_iceCannonImpact_d_wav_0051f238,iVar9 / 1000);
            FUN_00438c00(unaff_EBP,unaff_EDI,uVar11,0);
          }
        }
        else {
          iVar9 = *(int *)(DAT_0051cb64 * 0x1d4 + 0x74 + DAT_0051c960);
LAB_0044ba81:
          FUN_00438ae0(unaff_EBP,unaff_EDI,iVar9,0,0);
        }
      }
      if ((*(byte *)((int)piVar1 + 10) == DAT_0051cb4c) && (*(int *)((int)this + 0x1c) == 0)) {
        FUN_0044bda0(this);
      }
      if ((*(byte *)((int)piVar1 + 10) == DAT_0051cb54) &&
         (*(byte *)((int)piVar2 + 10) == DAT_0051ca7c)) {
        FUN_004a6aa0(DAT_00512b50,DAT_0051cb54,DAT_0051ca7c);
      }
    }
  }
  *(float *)((int)this + 0x60) = unaff_EBP;
  *(float *)((int)this + 100) = unaff_EDI;
  if ((!bVar6) && (!bVar13)) {
    return;
  }
  fStack_110 = (float)(*(int *)((int)this + 0x14) * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)fStack_110 + 0xb) & 1) == 0) {
    iVar9 = FUN_004ac420();
    if ((iVar9 == 0) || (iVar9 = FUN_004ac420(fStack_110), iVar9 == 1)) {
      if ((bVar6) && (*(int *)((int)this + 0x1c) == 0)) {
        FUN_004432f0(fStack_110,*(undefined4 *)((int)this + 0x18));
      }
      else {
        FUN_004433e0(fStack_110);
      }
    }
    else {
      fStack_110 = *(float *)((uint)*(byte *)((int)fStack_110 + 10) * 0x1d4 + 0x48 + DAT_0051c960) *
                   _DAT_00501c74;
      if (((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)piVar2 + 0xb) & 2) != 0)) &&
         ((*(byte *)((int)piVar2 + 0xb) & 1) == 0)) {
        uVar4 = (undefined1)piVar1[8];
LAB_0044bbe4:
        uVar11 = __ftol(uVar4,0);
        (**(code **)(*piVar2 + 0x98))(uVar11);
      }
    }
LAB_0044bbf5:
    if (bVar6) goto LAB_0044bc1f;
  }
  else if (bVar6) {
    if (((*(int *)((int)this + 0x1c) == 0) &&
        (fStack_110 = *(float *)(DAT_0051cb48 * 0x1d4 + 0x48 + DAT_0051c960) * _DAT_00501c74,
        *(int *)((int)this + 0x18) != 0)) && ((*(byte *)((int)piVar2 + 0xb) & 3) == 0)) {
      uVar4 = (undefined1)piVar1[8];
      goto LAB_0044bbe4;
    }
    goto LAB_0044bbf5;
  }
  if (*(byte *)((int)piVar1 + 10) == DAT_0051cb4c) {
    FUN_0047c8a0(unaff_EBP,unaff_EDI,2,0x3ca3d70a);
  }
LAB_0044bc1f:
  (**(code **)(*piVar1 + 0x10))(0);
  return;
}


// ==== FUN_0043f620 @ 0043f620

undefined4 __thiscall FUN_0043f620(void *this,float param_2,float param_3,int param_4)

{
  param_2 = param_2 - *(float *)this;
  param_3 = param_3 - *(float *)((int)this + 4);
  if (param_4 != 0) {
    if (ABS(param_2) <= ABS(param_3)) {
      if (0 < (int)param_3) {
        return 4;
      }
      return 0;
    }
    if (0 < (int)param_2) {
      return 2;
    }
    return 6;
  }
  if (0 < (int)param_2) {
    if (0x80000000 < (uint)param_3) {
      return 1;
    }
    if (0 < (int)param_3) {
      return 3;
    }
    return 2;
  }
  if ((uint)param_2 < 0x80000001) {
    if (0x80000000 < (uint)param_3) {
      return 0;
    }
    if (0 < (int)param_3) {
      return 4;
    }
    FUN_00490040(s_false_0051c480,s__ns_o_BoardCoord_cpp_0051c468,0xfa);
    return 0xffffffff;
  }
  if (0x80000000 < (uint)param_3) {
    return 7;
  }
  if (0 < (int)param_3) {
    return 5;
  }
  return 6;
}


// ==== FUN_0043f750 @ 0043f750

float10 __thiscall FUN_0043f750(void *this,float param_2,float param_3)

{
  float10 fVar1;
  
  fVar1 = (float10)*(float *)((int)this + 4) - (float10)param_3;
  return SQRT(((float10)*(float *)this - (float10)param_2) *
              ((float10)*(float *)this - (float10)param_2) + fVar1 * fVar1);
}


// ==== FUN_0043f950 @ 0043f950

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0043f950(void *this)

{
  float fVar1;
  
  fVar1 = *(float *)this;
  if (*(float *)this <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *(float *)this = fVar1;
  fVar1 = *(float *)((int)this + 4);
  if (*(float *)((int)this + 4) <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *(float *)((int)this + 4) = fVar1;
  fVar1 = *(float *)this;
  if (_DAT_00501768 <= *(float *)this) {
    fVar1 = _DAT_00501768;
  }
  *(float *)this = fVar1;
  fVar1 = *(float *)((int)this + 4);
  if (_DAT_00501768 <= *(float *)((int)this + 4)) {
    fVar1 = _DAT_00501768;
  }
  *(float *)((int)this + 4) = fVar1;
  return;
}


// ==== FUN_0043fa10 @ 0043fa10

void __thiscall FUN_0043fa10(void *this,int param_2,int param_3)

{
  float *extraout_ECX;
  float *extraout_ECX_00;
  float10 fVar1;
  
  if (DAT_00546778 == 0) {
    fVar1 = (float10)param_2 / (float10)DAT_00565c44;
  }
  else {
    fVar1 = (float10)__adj_fdiv_m32(DAT_00565c44);
    this = extraout_ECX;
  }
  *(float *)this = (float)(fVar1 + (float10)*(float *)this);
  if (DAT_00546778 == 0) {
    fVar1 = (float10)param_3 / (float10)DAT_00565c48;
  }
  else {
    fVar1 = (float10)__adj_fdiv_m32(DAT_00565c48);
    this = extraout_ECX_00;
  }
  *(float *)((int)this + 4) = (float)(fVar1 + (float10)*(float *)((int)this + 4));
  return;
}


// ==== FUN_004409a0 @ 004409a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl FUN_004409a0(float param_1,float param_2,float param_3,float param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  undefined8 local_8;
  
  iVar1 = *(int *)(param_5 + 0x110);
  iVar2 = *(int *)(param_5 + 0x114);
  if (DAT_00546778 == 0) {
    fVar5 = (float10)fpatan(-((float10)param_4 - (float10)param_2),
                            (float10)param_3 - (float10)param_1);
  }
  else {
    fVar5 = (float10)FUN_004f14c5();
  }
  local_8 = (double)(fVar5 - (float10)_DAT_00501788);
  if (fVar5 - (float10)_DAT_00501788 < (float10)_DAT_00501738) {
    local_8 = local_8 + _DAT_00501790;
  }
  iVar3 = FUN_0048ee10((float)(((float10)_DAT_00501790 - (float10)local_8) * (float10)iVar1 *
                              (float10)_DAT_00501798));
  iVar4 = __ftol();
  return (iVar3 % iVar4) * (iVar2 / iVar1);
}


// ==== FUN_004a1050 @ 004a1050

uint __cdecl FUN_004a1050(int param_1,int param_2)

{
  if (param_2 < 0x18) {
    return *(uint *)(&DAT_005374c0 + (param_2 + (param_1 / 2) * 0x1c) * 4) & 0x8000;
  }
  if (((((param_2 != 0x18) || (param_1 != 4)) && ((param_2 != 0x19 || (param_1 != 0)))) &&
      ((param_2 != 0x1a || (param_1 != 6)))) && ((param_2 != 0x1b || (param_1 != 2)))) {
    return 0;
  }
  return 1;
}


// ==== FUN_004793d0 @ 004793d0

void __cdecl FUN_004793d0(float *param_1,undefined4 param_2,int param_3)

{
  uint uVar1;
  float fVar2;
  float *pfVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  int iVar10;
  float10 fVar11;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  undefined4 local_6c;
  undefined4 local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  undefined4 local_54;
  undefined4 local_50;
  float local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_60 = -10000.0;
  iVar10 = DAT_005395dc + param_3 * 0x24;
  local_64 = -10000.0;
  FUN_0044aae0(&local_74,param_2);
  FUN_0044abd0(&local_48,param_2);
  pfVar3 = (float *)FUN_004aeec0(iVar10,&local_5c,0);
  local_84 = *pfVar3;
  local_80 = pfVar3[1];
  local_7c = local_84;
  local_78 = local_80;
  FUN_0043f950(&local_7c);
  iVar4 = FUN_00404990(&local_74);
  if (iVar4 == 0) {
    local_74 = local_7c;
    local_70 = local_78;
  }
  else {
    if (local_7c <= local_74) {
      local_74 = local_7c;
    }
    if (local_78 <= local_70) {
      local_70 = local_78;
    }
  }
  FUN_004049d0(&local_74,local_84,local_80);
  iVar4 = FUN_004aeec0(iVar10,&local_5c,0);
  local_84 = *(float *)(iVar4 + 8);
  local_80 = *(float *)(iVar4 + 0xc);
  local_7c = local_84;
  local_78 = local_80;
  FUN_0043f950(&local_7c);
  iVar4 = FUN_00404990(&local_74);
  if (iVar4 == 0) {
    local_74 = local_7c;
    local_70 = local_78;
    fVar2 = local_70;
  }
  else {
    fVar2 = local_7c;
    if (local_74 <= local_7c) {
      fVar2 = local_74;
    }
    local_74 = fVar2;
    fVar2 = local_78;
    if (local_70 <= local_78) {
      fVar2 = local_70;
    }
  }
  local_70 = fVar2;
  FUN_004049d0(&local_74,local_84,local_80);
  pfVar3 = (float *)FUN_00479c00(&local_5c,local_74,local_70);
  local_84 = *pfVar3;
  local_80 = pfVar3[1];
  FUN_0043f950(&local_84);
  iVar4 = FUN_00404990(&local_74);
  if (iVar4 == 0) {
    local_74 = local_84;
    local_70 = local_80;
  }
  else {
    if (local_84 <= local_74) {
      local_74 = local_84;
    }
    if (local_80 <= local_70) {
      local_70 = local_80;
    }
  }
  puVar5 = (undefined4 *)FUN_00479cc0(&local_5c,local_6c,local_68);
  FUN_004049d0(&local_74,*puVar5,puVar5[1]);
  local_80 = -10000.0;
  local_4c = 10000.0;
  local_84 = -10000.0;
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = 1;
  local_5c = local_74;
  local_58 = local_70;
  local_54 = local_6c;
  local_4 = 0;
  local_50 = local_68;
  FUN_00440a50();
  FUN_00440a80(&local_54);
  uVar6 = __ftol();
  uVar7 = __ftol(uVar6);
  uVar8 = __ftol(uVar7);
  uVar9 = __ftol(uVar8);
  FUN_004eae20(local_40,uVar9,uVar8,uVar7,uVar6);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      uVar1 = local_c * 0x24 + DAT_005395dc;
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1) break;
      if (*(byte *)(uVar1 + 10) == DAT_0051caac) {
        pfVar3 = (float *)FUN_00479860(uVar1,&local_5c,param_2,param_3);
        local_64 = *pfVar3;
        local_60 = pfVar3[1];
        if ((((0 < (int)local_64) && (local_7c = (float)DAT_0052e9a8, local_64 < local_7c)) &&
            (0 < (int)local_60)) && (local_60 < local_7c)) {
          fVar11 = (float10)FUN_0043f750(&local_64,local_48,local_44);
          local_7c = (float)fVar11;
          if (fVar11 < (float10)local_4c) {
            local_84 = local_64;
            local_80 = local_60;
            local_4c = local_7c;
          }
        }
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  *param_1 = local_84;
  param_1[1] = local_80;
  return;
}


