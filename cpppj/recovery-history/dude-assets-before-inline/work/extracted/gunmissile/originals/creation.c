// ==== FUN_00481750 @ 00481750

void __cdecl
FUN_00481750(float *param_1,float param_2,float param_3,float param_4,float param_5,int param_6)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  undefined1 local_20 [20];
  int local_c;
  
  iVar1 = FUN_0041ce90(&param_2,param_4,param_5,1);
  iVar4 = param_6 * 0x32 + DAT_005c8464;
  if (*(byte *)(iVar4 + 10) == DAT_00541258) {
    iVar2 = FUN_00466df0(iVar1,*(undefined4 *)(iVar4 + 0x24));
    iVar5 = iVar1;
    if (iVar2 != 0) goto LAB_0048183e;
    iVar5 = 0;
    do {
      iVar2 = FUN_00466df0(iVar5,*(undefined4 *)(iVar4 + 0x24));
      if (iVar2 != 0) break;
      iVar5 = iVar5 + 2;
    } while (iVar5 < 7);
  }
  else {
    FUN_0045ae60(local_20,param_6);
    iVar5 = local_c;
  }
  if (iVar5 != iVar1) {
    param_4 = param_2;
    param_5 = param_3;
    uVar3 = FUN_004ac550(iVar4);
    FUN_0049ace0(uVar3);
    iVar1 = FUN_004e49c0();
    param_6 = (&DAT_0052f85c)[iVar5] * iVar1;
    param_4 = (float)((&DAT_0052f83c)[iVar5] * iVar1) + param_4;
    param_5 = (float)param_6 + param_5;
    FUN_0041d0d0(&param_4);
  }
LAB_0048183e:
  switch(iVar5) {
  case 0:
  case 4:
    *param_1 = param_2;
    param_1[1] = param_5;
    return;
  default:
    FUN_004e0620("false",".\\Missile.cpp",0x4f);
    *param_1 = DAT_00545370;
    param_1[1] = DAT_00545374;
    return;
  case 2:
  case 6:
    *param_1 = param_4;
    param_1[1] = param_3;
    return;
  }
}


// ==== FUN_004818d0 @ 004818d0

void __cdecl FUN_004818d0(undefined4 *param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar3 = param_2 * 0x32 + DAT_005c8464;
  iVar1 = FUN_004ac550(iVar3);
  FUN_0049ae80(iVar1,&local_10,*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),0);
  uVar2 = FUN_0049a840(*(undefined1 *)(iVar3 + 10));
  iVar3 = FUN_00419850(uVar2,*(byte *)(iVar3 + 0x28) & 1,*(undefined4 *)(iVar3 + 0x24));
  FUN_0041d180(&local_8,(int)*(short *)(iVar3 + 0x20),(int)*(short *)(iVar3 + 0x22));
  FUN_0041d180(&local_10,(int)*(short *)(iVar3 + 0x20),(int)*(short *)(iVar3 + 0x22));
  FUN_0041d180(&local_8,-*(int *)(iVar1 + 0x1dc),-*(int *)(iVar1 + 0x1e0));
  FUN_0041d180(&local_10,-*(int *)(iVar1 + 0x1dc),-*(int *)(iVar1 + 0x1e0));
  *param_1 = local_10;
  param_1[1] = local_c;
  param_1[2] = local_8;
  param_1[3] = local_4;
  return;
}


// ==== FUN_004819b0 @ 004819b0

void __cdecl FUN_004819b0(undefined4 *param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar3 = param_2 * 0x32 + DAT_005c8464;
  iVar1 = FUN_004ac550(iVar3);
  local_8 = *(undefined4 *)(iVar3 + 0xe);
  local_4 = *(undefined4 *)(iVar3 + 0x12);
  uVar2 = FUN_0049a840(*(undefined1 *)(iVar3 + 10));
  iVar3 = FUN_00419850(uVar2,*(byte *)(iVar3 + 0x28) & 1,*(undefined4 *)(iVar3 + 0x24));
  FUN_0041d180(&local_8,(int)*(short *)(iVar3 + 0x20),(int)*(short *)(iVar3 + 0x22));
  FUN_0041d180(&local_8,-*(int *)(iVar1 + 0x1dc),-*(int *)(iVar1 + 0x1e0));
  *param_1 = local_8;
  param_1[1] = local_4;
  return;
}


// ==== FUN_00481a50 @ 00481a50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall
FUN_00481a50(void *this,undefined4 param_2,int param_3,int param_4,undefined4 param_5,
            undefined4 param_6,float param_7,float param_8,float param_9,float param_10,int param_11
            ,float param_12,float param_13)

{
  float *pfVar1;
  bool bVar2;
  double dVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined4 uVar6;
  int iVar7;
  uint uVar8;
  float *pfVar9;
  int *piVar10;
  int *piVar11;
  void *unaff_EDI;
  float10 fVar12;
  int *unaff_retaddr;
  undefined4 *puVar13;
  undefined4 *puVar14;
  undefined4 uVar15;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fae08;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  dVar3 = _DAT_00500460;
  *(undefined ***)this = &PTR_FUN_0050d630;
  *(undefined4 *)((int)this + 0x5c) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x58) = 0xc61c4000;
  *(undefined4 *)((int)this + 100) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x60) = 0xc61c4000;
  *(double *)((int)this + 0x68) = dVar3;
  local_4 = 0;
  *(double *)((int)this + 0x70) = _DAT_00500460;
  *(undefined4 *)((int)this + 0x14) = param_2;
  bVar2 = param_12 <= _DAT_00500fc4;
  *(int *)((int)this + 0x18) = param_3;
  *(int *)((int)this + 0x30) = param_4;
  *(int *)((int)this + 0xc) = param_11;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined4 *)((int)this + 0x28) = 0;
  *(undefined4 *)((int)this + 0x7c) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x78) = 0xc61c4000;
  param_11 = *(int *)((int)this + 0x18) * 0x32 + DAT_005c8464;
  piVar11 = (int *)(*(int *)((int)this + 0x14) * 0x32 + DAT_005c8464);
  if ((((bVar2) || ((float)DAT_00531928 <= param_12)) || (param_13 <= _DAT_00500fc4)) ||
     ((float)DAT_00531928 <= param_13)) {
    puVar4 = (undefined4 *)(**(code **)(*piVar11 + 0xb4))(&param_12);
    *(undefined4 *)((int)this + 0x38) = *puVar4;
    *(undefined4 *)((int)this + 0x3c) = puVar4[1];
  }
  else {
    if (*(int *)((int)this + 0xc) == 0) {
      FUN_004e0620("riccochet",".\\Missile.cpp",0x12a);
    }
    *(float *)((int)this + 0x38) = param_12;
    *(float *)((int)this + 0x3c) = param_13;
  }
  puVar4 = (undefined4 *)((int)this + 0x38);
  iVar5 = FUN_004ac550(piVar11);
  if ((*(int *)((int)this + 0xc) == 0) && (*(byte *)((int)piVar11 + 10) != DAT_00541174)) {
    uVar6 = FUN_0049a840((uint)*(byte *)((int)piVar11 + 10));
    iVar7 = FUN_00419850(uVar6,*(byte *)(piVar11 + 10) & 1,piVar11[9]);
    FUN_0041d180(puVar4,(int)*(short *)(iVar7 + 0x20),(int)*(short *)(iVar7 + 0x22));
    FUN_0041d180(puVar4,-*(int *)(iVar5 + 0x1dc),-*(int *)(iVar5 + 0x1e0));
  }
  uVar8 = (uint)*(byte *)(param_11 + 10);
  if ((uVar8 < 10) || (DAT_00541348 <= (int)uVar8)) {
    param_4 = 3;
    param_3 = 3;
  }
  else {
    uVar15 = 1;
    puVar14 = &param_4;
    puVar13 = &param_3;
    uVar6 = FUN_0049a840(uVar8,puVar13,puVar14,1);
    FUN_0049a900(uVar6,puVar13,puVar14,uVar15);
  }
  if ((((param_7 <= _DAT_00500fc4) || ((float)DAT_00531928 <= param_7)) ||
      (param_8 <= _DAT_00500fc4)) || ((float)DAT_00531928 <= param_8)) {
    FUN_004e0620("_targetPos.isValid()",".\\Missile.cpp",0x142);
  }
  pfVar1 = (float *)((int)this + 0x48);
  *pfVar1 = param_7 - (float)param_3 * _DAT_00500ee4;
  param_12 = param_9;
  *(float *)((int)this + 0x4c) = param_8 - (float)param_4 * _DAT_00500ee4;
  *(float *)((int)this + 0x50) = param_9;
  *(float *)((int)this + 0x54) = param_10;
  param_13 = param_10;
  *(undefined4 *)((int)this + 0x1c) = 0;
  if (*(int *)((int)this + 0xc) == 0) {
    iVar5 = FUN_0040e800(&param_9);
    if (iVar5 == 0) {
      *(float *)((int)this + 0x44) = param_8;
      *(float *)((int)this + 0x40) = param_7;
      iVar5 = FUN_004ac950(piVar11);
      if (iVar5 == 1) {
        pfVar9 = (float *)FUN_00481750(&param_7,*puVar4,*(undefined4 *)((int)this + 0x3c),*pfVar1,
                                       *(undefined4 *)((int)this + 0x4c),
                                       ((int)piVar11 - DAT_005c8464) / 0x32);
        *pfVar1 = *pfVar9;
        *(float *)((int)this + 0x4c) = pfVar9[1];
      }
    }
    else {
      iVar5 = FUN_004ac950(piVar11);
      if (iVar5 == 1) {
        iVar5 = FUN_004e49c0();
        *(undefined4 *)((int)this + 0x24) = 0;
        *(undefined4 *)((int)this + 0x20) = 0;
        fVar12 = (float10)FUN_0041cfc0(&param_9,*puVar4,*(undefined4 *)((int)this + 0x3c));
        if ((iVar5 != 0) && (fVar12 < (float10)_DAT_00501444 != (fVar12 == (float10)_DAT_00501444)))
        {
          *(undefined4 *)((int)this + 0x28) = 1;
        }
        pfVar9 = (float *)FUN_00481750(&param_12,*puVar4,*(undefined4 *)((int)this + 0x3c),param_9,
                                       param_10,((int)piVar11 - DAT_005c8464) / 0x32);
        param_7 = *pfVar9;
        param_8 = pfVar9[1];
        *pfVar1 = param_7;
        *(float *)((int)this + 0x4c) = param_8;
        if (*(int *)((int)this + 0x28) != 0) {
          param_7 = *(float *)((int)this + 0x50);
          param_8 = *(float *)((int)this + 0x54);
          *pfVar1 = param_7;
          *(float *)((int)this + 0x4c) = param_8;
        }
      }
      else {
        param_7 = param_9;
        param_8 = param_10;
        *pfVar1 = param_9;
        *(float *)((int)this + 0x4c) = param_10;
      }
      *(float *)((int)this + 0x40) = param_9;
      *(float *)((int)this + 0x44) = param_10;
      *(undefined4 *)((int)this + 0x1c) = 1;
    }
  }
  else {
    *(float *)((int)this + 0x40) = param_7;
    *(float *)((int)this + 0x44) = param_8;
  }
  FUN_0041d0d0(pfVar1);
  FUN_0041d0d0(puVar4);
  if (((*(byte *)((int)piVar11 + 0xb) & 1) != 0) && ((*(byte *)(param_11 + 0xb) & 1) != 0)) {
    FUN_004e0620("!firing.isFree() || !target.isFree()",".\\Missile.cpp",0x1a2);
  }
  iVar5 = FUN_0040e800(puVar4);
  if (iVar5 == 0) {
    FUN_004e0620("firingPos.isValid()",".\\Missile.cpp",0x1a3);
  }
  iVar5 = FUN_0040e800(pfVar1);
  if (iVar5 == 0) {
    FUN_004e0620("targetCenter.isValid()",".\\Missile.cpp",0x1a4);
  }
  *(undefined4 *)((int)this + 0x34) = 0;
  fVar12 = (float10)FUN_0041cfc0(pfVar1,*puVar4,*(undefined4 *)((int)this + 0x3c));
  if ((fVar12 < (float10)_DAT_00500460) || ((float)DAT_00531928 <= (float)fVar12)) {
    FUN_004e0620("dist >= 0.0 && dist < boardDimInTiles",".\\Missile.cpp",0x1ba);
  }
  uVar6 = DAT_0055b4d0;
  *(undefined4 *)((int)this + 0x68) = DAT_0055b4d0;
  uVar15 = DAT_0055b4d4;
  *(undefined4 *)((int)this + 0x6c) = DAT_0055b4d4;
  *(undefined4 *)((int)this + 0x70) = uVar6;
  *(undefined4 *)((int)this + 0x74) = uVar15;
  *(double *)((int)this + 0x68) =
       (double)((float)fVar12 / *(float *)((int)this + 0x30) + (float)*(double *)((int)this + 0x68))
  ;
  piVar10 = (int *)FUN_004af530(param_5,2);
  param_11 = FUN_0049a840(*(undefined1 *)((int)piVar10 + 10));
  if (0 < *(int *)(param_11 + 0x110)) {
    if (*(byte *)((int)piVar10 + 10) == DAT_005411a4) {
      piVar10[9] = 1;
    }
    else {
      uVar6 = FUN_004ac550();
      iVar5 = FUN_0041d6e0(*puVar4,*(undefined4 *)((int)this + 0x3c),*pfVar1,
                           *(undefined4 *)((int)this + 0x4c),uVar6);
      piVar10[9] = iVar5;
    }
  }
  (**(code **)(*piVar10 + 0x74))(*(undefined1 *)((int)piVar11 + 0x22));
  iVar5 = FUN_0049a840(*(undefined1 *)((int)piVar11 + 10));
  iVar5 = *(int *)(iVar5 + 0x70);
  if ((iVar5 != 0) || (iVar5 = *(int *)((int)param_10 + 0x70), iVar5 != 0)) {
    FUN_004a9c30(*puVar4,*(undefined4 *)((int)this + 0x3c),iVar5,0,0);
  }
  (**(code **)(*unaff_retaddr + 0x90))(*puVar4,*(undefined4 *)((int)this + 0x3c),0);
  if (piVar10 != (int *)0x0) {
    iVar5 = FUN_0049a840(piVar10);
    (**(code **)(iVar5 + 0x1d0))(((int)unaff_retaddr - DAT_005c8464) / 0x32,0);
  }
  FUN_0041bd20(this,DAT_005410e0,((int)unaff_retaddr - DAT_005c8464) / 0x32,0,0x10);
  ExceptionList = unaff_EDI;
  return this;
}


// ==== FUN_00482080 @ 00482080

float * __thiscall FUN_00482080(void *this,float *param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  float10 fVar6;
  float local_1c;
  float local_8;
  float local_4;
  
  iVar3 = FUN_0041bfb0();
  fVar1 = *(float *)(iVar3 + 0x12);
  fVar2 = *(float *)(iVar3 + 0xe);
  if ((fVar2 == *(float *)((int)this + 0x38)) && (fVar1 == *(float *)((int)this + 0x3c))) {
    return param_2;
  }
  fVar5 = (float10)FUN_0041ce20(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12));
  iVar4 = FUN_004558f0(0,100);
  local_1c = 1.0;
  if (0x31 < iVar4) {
    local_1c = -1.0;
  }
  fVar6 = (float10)FUN_00455930(0x41f00000,0x43160000);
  fVar5 = (float10)FUN_00468e70((float)(fVar6 * (float10)local_1c + (float10)(float)fVar5));
  iVar3 = FUN_004ac550(iVar3);
  fVar6 = (float10)fcos((float10)(float)fVar5);
  local_8 = (float)(fVar6 * (float10)*(float *)(iVar3 + 0x44) + (float10)fVar2);
  fVar5 = (float10)fsin((float10)(float)fVar5);
  local_4 = (float)(fVar5 * (float10)*(float *)(iVar3 + 0x44) + (float10)fVar1);
  FUN_0041d0d0(&local_8);
  *param_2 = local_8;
  param_2[1] = local_4;
  return param_2;
}


// ==== FUN_00482190 @ 00482190

void __thiscall FUN_00482190(void *this)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  void *pvVar4;
  int iVar5;
  float10 fVar6;
  float local_68;
  int local_64;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined1 local_4c [52];
  int local_18;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fae2b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  iVar1 = FUN_004558f0(0,99);
  if (0x13 < iVar1) {
    iVar1 = FUN_0041bfb0(this);
    FUN_00482080(this,&local_5c);
    iVar2 = FUN_0040e800(&local_5c);
    if (iVar2 != 0) {
      local_64 = 0;
      local_68 = 10000.0;
      FUN_00422230(local_4c,local_5c,local_58,1,1,0);
      while (local_18 != 0) {
        iVar5 = local_18 * 0x32 + DAT_005c8464;
        uVar3 = FUN_004ac200(iVar5);
        iVar2 = FUN_004adc60(iVar5);
        if (((iVar2 != 0) &&
            (((iVar2 = (iVar5 - DAT_005c8464) / 0x32, iVar2 < 5 || (14999 < iVar2)) &&
             ((*(byte *)(iVar5 + 0x28) & 1) == 0)))) &&
           (((uVar3 & 0x120000) == 0 && (iVar2 != *(int *)((int)this + 0x18))))) {
          local_54 = *(undefined4 *)(iVar5 + 0xe);
          local_50 = *(undefined4 *)(iVar5 + 0x12);
          fVar6 = (float10)FUN_0041cfc0(&local_54,local_5c,local_58);
          if (fVar6 < (float10)local_68) {
            local_68 = (float)fVar6;
            local_64 = local_18;
          }
        }
        FUN_004b1810(local_4c);
      }
      pvVar4 = operator_new(0x80);
      local_4 = 0;
      if (pvVar4 != (void *)0x0) {
        FUN_00481a50(pvVar4,*(undefined4 *)((int)this + 8),local_64,
                     *(undefined4 *)((int)this + 0x30),*(undefined1 *)(iVar1 + 10),0,local_5c,
                     local_58,DAT_00545370,DAT_00545374,1,*(undefined4 *)(iVar1 + 0xe),
                     *(undefined4 *)(iVar1 + 0x12));
      }
    }
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_00482340 @ 00482340

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00482340(void *this)

{
  float fVar1;
  bool bVar2;
  int iVar3;
  undefined4 uVar4;
  float *pfVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  uint uVar9;
  undefined4 uVar10;
  int *unaff_EBX;
  float fVar11;
  float unaff_ESI;
  float unaff_EDI;
  int *piVar12;
  bool bVar13;
  byte bVar14;
  float10 fVar15;
  float10 fVar16;
  float *pfVar17;
  undefined4 uVar18;
  int *local_138;
  float local_134;
  float local_130;
  int *local_12c;
  float local_128 [2];
  float local_120;
  float fStack_11c;
  float local_118;
  undefined1 auStack_110 [272];
  
  if ((DAT_005e4794 != 0) &&
     (iVar3 = FUN_0041be80(this,*(undefined4 *)((int)this + 0x14)), iVar3 == 0)) {
    FUN_004e0620("isRobust(firingSid)",".\\Missile.cpp",0x1ee);
  }
  fVar1 = DAT_0055b4d4;
  uVar10 = DAT_0055b4d0;
  piVar12 = (int *)(*(int *)((int)this + 8) * 0x32 + DAT_005c8464);
  fVar11 = (float)(*(int *)((int)this + 0x18) * 0x32 + DAT_005c8464);
  iVar3 = *(int *)((int)this + 0x14) * 0x32 + DAT_005c8464;
  fStack_11c = DAT_0055b4d4;
  if (*(int *)((int)this + 0x28) != 0) {
    *(undefined4 *)((int)this + 0x28) = 0;
    *(undefined4 *)((int)this + 0x2c) = 1;
  }
  fVar15 = (float10)*(double *)((int)this + 0x68) - (float10)(double)CONCAT44(fVar1,uVar10);
  local_118 = 0.0;
  local_130 = *(float *)((int)this + 0x48) - *(float *)((int)this + 0x38);
  local_120 = *(float *)((int)this + 0x4c) - *(float *)((int)this + 0x3c);
  iVar6 = _DAT_00540bb8;
  if ((*(byte *)((int)piVar12 + 10) == DAT_00541268) ||
     (iVar6 = _DAT_00540bb4, *(int *)((int)this + 0xc) == 0)) {
    local_118 = (float)iVar6;
  }
  fVar16 = (float10)_DAT_005012d0 /
           ((float10)*(double *)((int)this + 0x68) - (float10)*(double *)((int)this + 0x70));
  local_138 = (int *)(float)((float10)*(float *)((int)this + 0x48) -
                            (float10)local_130 * fVar16 * fVar15);
  local_134 = (float)((float10)*(float *)((int)this + 0x4c) - fVar16 * (float10)local_120 * fVar15);
  local_12c = piVar12;
  local_128[0] = fVar11;
  if (((((float)local_138 <= _DAT_00500fc4) ||
       (local_130 = (float)DAT_00531928, local_130 <= (float)local_138)) ||
      (local_134 <= _DAT_00500fc4)) || (local_130 <= local_134)) {
    if (*(int *)((int)this + 0x2c) != 0) {
      iVar3 = FUN_0049a840(DAT_00541278,0,0);
      FUN_004a9c30(local_138,local_134,*(undefined4 *)(iVar3 + 0x74));
    }
    (**(code **)(*piVar12 + 0x10))(0);
    return;
  }
  (**(code **)(*piVar12 + 0x48))(0);
  (**(code **)(*piVar12 + 0x90))(unaff_EBX,local_138,0);
  if ((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)fVar11 + 0xb) & 3) == 0)) {
    if (*(int *)((int)this + 0x1c) == 0) {
      bVar14 = *(byte *)((int)fVar11 + 10);
      iVar6 = FUN_00436d00((uint)bVar14);
      uVar10 = *(undefined4 *)((int)this + 0x44);
      uVar18 = *(undefined4 *)((int)this + 0x40);
      pfVar17 = &local_120;
      uVar9 = DAT_00541258;
      if (iVar6 != 0) {
        uVar9 = (uint)bVar14;
      }
      uVar4 = FUN_0049a840(uVar9,pfVar17,uVar18,uVar10);
      pfVar5 = (float *)FUN_0049af10(uVar4,pfVar17,uVar18,uVar10);
      pfVar17 = (float *)((int)this + 0x58);
      *pfVar17 = *pfVar5;
      *(float *)((int)this + 0x5c) = pfVar5[1];
      *(float *)((int)this + 0x60) = pfVar5[2];
      *(float *)((int)this + 100) = pfVar5[3];
      *pfVar17 = *pfVar17 - *(float *)((int)this + 0x34);
      *(float *)((int)this + 0x5c) = *(float *)((int)this + 0x5c) - *(float *)((int)this + 0x34);
      *(float *)((int)this + 0x60) = *(float *)((int)this + 0x60) + *(float *)((int)this + 0x34);
      *(float *)((int)this + 100) = *(float *)((int)this + 100) + *(float *)((int)this + 0x34);
    }
    else {
      pfVar17 = (float *)((int)this + 0x58);
      *(undefined4 *)((int)this + 0x5c) = 0xc61c4000;
      *pfVar17 = -10000.0;
      *(undefined4 *)((int)this + 100) = 0xc61c4000;
      *(undefined4 *)((int)this + 0x60) = 0xc61c4000;
      FUN_00431f90(pfVar17,*(undefined4 *)((int)this + 0x48),*(undefined4 *)((int)this + 0x4c));
      *pfVar17 = *pfVar17 - _DAT_005012d0;
      *(float *)((int)this + 0x5c) = *(float *)((int)this + 0x5c) - _DAT_005012d0;
    }
  }
  bVar2 = false;
  iVar6 = FUN_0040e800((int)this + 0x78);
  piVar12 = local_138;
  if (iVar6 != 0) {
    uVar10 = *(undefined4 *)((int)this + 0x7c);
    uVar18 = *(undefined4 *)((int)this + 0x78);
    local_120 = unaff_EDI;
    fStack_11c = unaff_ESI;
    local_118 = unaff_EDI;
    FUN_0040e880(&local_120,uVar18,uVar10);
    FUN_0040e8f0(&local_120,uVar18,uVar10);
    piVar12 = local_138;
    if ((DAT_0054daa4 != 0) &&
       (((*(byte *)(iVar3 + 0x28) & 4) != 0 || ((*(byte *)(local_138 + 10) & 4) != 0)))) {
      FUN_0041d670(&local_120,DAT_005bec2c);
      FUN_0041d670((int)this + 0x58,DAT_005bec2c);
    }
    if (((*(int *)((int)this + 0x10) == 0) &&
        (iVar6 = FUN_00468cc0(&local_120,*(undefined4 *)((int)this + 0x58),
                              *(undefined4 *)((int)this + 0x5c),*(undefined4 *)((int)this + 0x60),
                              *(undefined4 *)((int)this + 100)), iVar6 != 0)) &&
       ((*(byte *)(piVar12 + 10) & 0x10) == 0)) {
      bVar2 = true;
    }
    else {
      bVar2 = false;
    }
  }
  if (*(int *)((int)this + 0xc) == 0) {
    if (*(int *)((int)this + 0x1c) == 0) {
      puVar7 = (undefined4 *)FUN_004adea0(iVar3,&local_120);
      fVar15 = (float10)FUN_0041cfc0(&stack0xfffffeb8,*puVar7,puVar7[1]);
      local_130 = (float)fVar15;
      iVar3 = FUN_004ac550(iVar3);
      fVar1 = local_128[0] + *(float *)(iVar3 + 0x44);
      bVar14 = fVar1 < local_130 |
               (byte)((ushort)((ushort)(NAN(fVar1) || NAN(local_130)) << 10) >> 8);
    }
    else {
      iVar6 = FUN_004ac950();
      if (iVar6 == 1) {
        puVar7 = (undefined4 *)FUN_004adea0(iVar3,&local_138);
        puVar8 = (undefined4 *)FUN_004adea0(iVar3,local_128);
        fVar15 = (float10)FUN_0041cfc0(&stack0xfffffeb8,*puVar7,puVar7[1]);
        local_130 = (float)fVar15;
        fVar15 = (float10)FUN_0041cfc0((int)this + 0x48,*puVar8,puVar8[1]);
        bVar14 = fVar15 < (float10)local_130 |
                 (byte)((ushort)((ushort)(NAN(fVar15) || NAN((float10)local_130)) << 10) >> 8);
      }
      else {
        puVar7 = (undefined4 *)FUN_004adea0(iVar3,&local_138);
        puVar8 = (undefined4 *)FUN_004adea0(iVar3,&local_120);
        fVar15 = (float10)FUN_0041cfc0(&stack0xfffffeb8,*puVar7,puVar7[1]);
        local_130 = (float)fVar15;
        fVar15 = (float10)FUN_0041cfc0((int)this + 0x48,*puVar8,puVar8[1]);
        bVar14 = fVar15 + (float10)local_128[0] < (float10)local_130 |
                 (byte)((ushort)((ushort)(NAN(fVar15 + (float10)local_128[0]) ||
                                         NAN((float10)local_130)) << 10) >> 8);
      }
    }
  }
  else {
    fVar15 = (float10)FUN_0041cfc0(&stack0xfffffeb8,*(undefined4 *)((int)this + 0x38),
                                   *(undefined4 *)((int)this + 0x3c));
    local_130 = (float)fVar15;
    iVar3 = FUN_004ac550(unaff_EBX);
    fVar1 = local_128[0] + *(float *)(iVar3 + 0x44);
    bVar14 = fVar1 < local_130 | (byte)((ushort)((ushort)(NAN(fVar1) || NAN(local_130)) << 10) >> 8)
    ;
  }
  bVar13 = (POPCOUNT(bVar14) & 1U) == 0;
  if (bVar2) {
    if ((*(byte *)((int)piVar12 + 10) != DAT_005412a0) ||
       (fVar15 = (float10)FUN_00455930(0,0x447a0000), (float10)_DAT_00540bbc <= fVar15)) {
      unaff_EDI = (unaff_EDI + *(float *)((int)this + 0x78)) * _DAT_00500ee4;
      unaff_ESI = (unaff_ESI + *(float *)((int)this + 0x7c)) * _DAT_00500ee4;
      if ((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)piVar12 + 0xb) & 1) == 0)) {
        if ((*(int *)((int)this + 0x1c) == 0) && (*(byte *)((int)unaff_EBX + 10) != DAT_00541260)) {
          FUN_00470ac0(unaff_EDI,unaff_ESI);
        }
        else {
          FUN_0045a750(unaff_EDI,unaff_ESI,2,0x3ca3d70a);
        }
        iVar3 = FUN_0049a840(*(undefined1 *)((int)unaff_EBX + 10));
        if (*(int *)((int)this + 0x1c) == 0) {
          iVar3 = *(int *)(iVar3 + 0x74);
          if (iVar3 != 0) {
            if (*(byte *)((int)unaff_EBX + 10) == DAT_00541260) {
              iVar3 = FUN_004558f0(1000,5000);
              uVar10 = FUN_004a89c0(auStack_110,"iceCannonImpact%d.wav",iVar3 / 1000);
              FUN_004a9d70(unaff_EDI,unaff_ESI,uVar10,0);
            }
            else if (bVar13) {
              uVar10 = 0;
              uVar18 = 0;
              goto LAB_004828f8;
            }
          }
        }
        else {
          uVar18 = 0;
          uVar10 = 0;
          iVar3 = FUN_0049a840(DAT_00541278,0,0);
          iVar3 = *(int *)(iVar3 + 0x74);
LAB_004828f8:
          FUN_004a9c30(unaff_EDI,unaff_ESI,iVar3,uVar10,uVar18);
        }
      }
      if ((*(byte *)((int)unaff_EBX + 10) == DAT_00541260) && (*(int *)((int)this + 0x1c) == 0)) {
        FUN_00482190(this);
      }
      if ((*(byte *)((int)unaff_EBX + 10) == DAT_00541268) &&
         (*(byte *)((int)piVar12 + 10) == DAT_00541190)) {
        FUN_00452fe0(DAT_005429c8,DAT_00541268,DAT_00541190);
      }
    }
    else {
      bVar2 = false;
      *(undefined4 *)((int)this + 0x10) = 1;
    }
  }
  if ((DAT_005e4794 != 0) &&
     (iVar3 = FUN_0041be80(this,*(undefined4 *)((int)this + 0x14)), iVar3 == 0)) {
    FUN_004e0620("isRobust(firingSid)",".\\Missile.cpp",0x2a0);
  }
  *(float *)((int)this + 0x7c) = unaff_ESI;
  *(float *)((int)this + 0x78) = unaff_EDI;
  if ((!bVar2) && (bVar13)) {
    return;
  }
  iVar3 = *(int *)((int)this + 0x14) * 0x32;
  iVar6 = iVar3 + DAT_005c8464;
  if ((*(byte *)(iVar3 + 0xb + DAT_005c8464) & 1) == 0) {
    iVar3 = FUN_004ac950(iVar6);
    if ((iVar3 == 0) || (iVar3 = FUN_004ac950(iVar6), iVar3 == 1)) {
      if ((bVar2) && (*(int *)((int)this + 0x1c) == 0)) {
        FUN_004d4f00(iVar6,*(undefined4 *)((int)this + 0x18));
      }
      else {
        FUN_004d5000(iVar6);
      }
    }
    else {
      iVar3 = FUN_0049a840(*(undefined1 *)(iVar6 + 10));
      local_138 = (int *)(*(float *)(iVar3 + 0x48) * _DAT_0050d650);
      iVar3 = FUN_0049a840(*(undefined1 *)(iVar6 + 10));
      if ((*(int *)(iVar3 + 0x1b0) != 0) && (uVar9 = FUN_004ac200(piVar12), (uVar9 & 0x120000) != 0)
         ) {
        iVar3 = FUN_0049a840(*(undefined1 *)(iVar6 + 10));
        local_138 = *(int **)(iVar3 + 0x1b4);
      }
      if ((*(int *)((int)this + 0x18) != 0) && ((*(byte *)((int)piVar12 + 0xb) & 2) != 0)) {
        bVar14 = *(byte *)((int)piVar12 + 0xb) & 1;
LAB_00482ace:
        if (bVar14 == 0) {
          iVar3 = *piVar12;
          uVar10 = FUN_004e49c0(*(undefined1 *)((int)unaff_EBX + 0x22),0);
          (**(code **)(iVar3 + 0x98))(uVar10);
        }
      }
    }
LAB_00482aeb:
    if (bVar2) goto LAB_00482b18;
  }
  else if (bVar2) {
    if (*(int *)((int)this + 0x1c) == 0) {
      iVar3 = FUN_0049a840(DAT_0054125c);
      local_138 = (int *)(*(float *)(iVar3 + 0x48) * _DAT_0050d650);
      iVar3 = FUN_0049a840(*(undefined1 *)(iVar6 + 10));
      if ((*(int *)(iVar3 + 0x1b0) != 0) && (uVar9 = FUN_004ac200(piVar12), (uVar9 & 0x120000) != 0)
         ) {
        iVar3 = FUN_0049a840(*(undefined1 *)(iVar6 + 10));
        local_138 = *(int **)(iVar3 + 0x1b4);
      }
      if (*(int *)((int)this + 0x18) != 0) {
        bVar14 = *(byte *)((int)piVar12 + 0xb) & 3;
        goto LAB_00482ace;
      }
    }
    goto LAB_00482aeb;
  }
  if (*(byte *)((int)unaff_EBX + 10) == DAT_00541260) {
    FUN_0045a750(unaff_EDI,unaff_ESI,2,0x3ca3d70a);
  }
LAB_00482b18:
  if (*(int *)((int)this + 0x2c) != 0) {
    iVar3 = FUN_0049a840(DAT_00541278,0,0);
    FUN_004a9c30(unaff_EDI,unaff_ESI,*(undefined4 *)(iVar3 + 0x74));
  }
  (**(code **)(*unaff_EBX + 0x10))(0);
  return;
}


// ==== FUN_0040e800 @ 0040e800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0040e800(void *this)

{
  if ((((_DAT_00500fc4 < *(float *)this) && (*(float *)this < (float)DAT_00531928)) &&
      (_DAT_00500fc4 < *(float *)((int)this + 4))) &&
     (*(float *)((int)this + 4) < (float)DAT_00531928)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0040e880 @ 0040e880

void __thiscall FUN_0040e880(void *this,float param_2,float param_3)

{
  int iVar1;
  
  FUN_0041d0d0(&param_2);
  iVar1 = FUN_0040e800(this);
  if (iVar1 == 0) {
    *(float *)this = param_2;
    *(float *)((int)this + 4) = param_3;
    return;
  }
  if (*(float *)this < param_2) {
    param_2 = *(float *)this;
  }
  *(float *)this = param_2;
  if (param_3 <= *(float *)((int)this + 4)) {
    *(float *)((int)this + 4) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 4) = *(undefined4 *)((int)this + 4);
  return;
}


// ==== FUN_0040e8f0 @ 0040e8f0

void __thiscall FUN_0040e8f0(void *this,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  
  FUN_0041d0d0(&param_2);
  pfVar1 = (float *)((int)this + 8);
  iVar2 = FUN_0040e800(pfVar1);
  if (iVar2 == 0) {
    *pfVar1 = param_2;
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  if (param_2 < *pfVar1) {
    param_2 = *pfVar1;
  }
  *pfVar1 = param_2;
  if (*(float *)((int)this + 0xc) <= param_3) {
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)this + 0xc);
  return;
}


// ==== FUN_0041ce90 @ 0041ce90

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0041ce90(void *this,float param_2,float param_3,int param_4)

{
  param_2 = param_2 - *(float *)this;
  param_3 = param_3 - *(float *)((int)this + 4);
  if (param_4 == 0) {
    if (param_2 <= _DAT_00500fc4) {
      if (param_2 < _DAT_00500fc4) {
        if (param_3 < _DAT_00500fc4) {
          return 7;
        }
        if (param_3 <= _DAT_00500fc4) {
          return 6;
        }
        return 5;
      }
      if (param_3 < _DAT_00500fc4) {
        return 0;
      }
      if (_DAT_00500fc4 < param_3) {
        return 4;
      }
      FUN_004e0620("false","\\Ns\\O\\Boardcoord.cpp",0xfd);
      return 0xffffffff;
    }
    if (param_3 < _DAT_00500fc4) {
      return 1;
    }
    if (_DAT_00500fc4 < param_3) {
      return 3;
    }
  }
  else {
    if (ABS(param_2) <= ABS(param_3)) {
      if (_DAT_00500fc4 < param_3) {
        return 4;
      }
      return 0;
    }
    if (param_2 <= _DAT_00500fc4) {
      return 6;
    }
  }
  return 2;
}


// ==== FUN_0041cfc0 @ 0041cfc0

float10 __thiscall FUN_0041cfc0(void *this,float param_2,float param_3)

{
  float10 fVar1;
  
  fVar1 = (float10)*(float *)((int)this + 4) - (float10)param_3;
  return SQRT(fVar1 * fVar1 +
              ((float10)*(float *)this - (float10)param_2) *
              ((float10)*(float *)this - (float10)param_2));
}


// ==== FUN_0041d0d0 @ 0041d0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d0d0(void *this)

{
  float fVar1;
  float fVar2;
  
  fVar1 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)this) {
    fVar1 = *(float *)this;
  }
  *(float *)this = fVar1;
  fVar2 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)((int)this + 4)) {
    fVar2 = *(float *)((int)this + 4);
  }
  *(float *)((int)this + 4) = fVar2;
  if (_DAT_005022e8 < fVar1) {
    fVar1 = _DAT_005022e8;
  }
  *(float *)this = fVar1;
  if (fVar2 <= _DAT_005022e8) {
    *(float *)((int)this + 4) = fVar2;
    return;
  }
  *(undefined4 *)((int)this + 4) = 0x437f0000;
  return;
}


// ==== FUN_0041d180 @ 0041d180

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d180(void *this,int param_2,int param_3)

{
  *(float *)this = (float)param_2 / _DAT_0059a92c + *(float *)this;
  *(float *)((int)this + 4) = (float)param_3 / _DAT_0059a930 + *(float *)((int)this + 4);
  return;
}


// ==== FUN_0041d6e0 @ 0041d6e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl FUN_0041d6e0(float param_1,float param_2,float param_3,float param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  
  iVar1 = *(int *)(param_5 + 0x110);
  iVar2 = *(int *)(param_5 + 0x114);
  fVar5 = (float10)fpatan(-((float10)param_4 - (float10)param_2),(float10)param_3 - (float10)param_1
                         );
  fVar5 = fVar5 - (float10)_DAT_00502308;
  if (fVar5 < (float10)_DAT_00500460) {
    fVar5 = fVar5 + (float10)_DAT_00502300;
  }
  iVar3 = FUN_0043fa50((float)(((float10)_DAT_00502300 - fVar5) * (float10)iVar1 *
                              (float10)_DAT_005022f8));
  iVar4 = FUN_004e49c0();
  return (iVar3 % iVar4) * (iVar2 / iVar1);
}


// ==== FUN_00466df0 @ 00466df0

uint __cdecl FUN_00466df0(int param_1,int param_2)

{
  bool bVar1;
  
  if (param_2 < 0x18) {
    return *(uint *)(&DAT_00540550 + ((param_1 / 2) * 0x1c + param_2) * 4) & 0x8000;
  }
  if (param_2 == 0x18) {
    bVar1 = param_1 == 4;
  }
  else if (param_2 == 0x19) {
    bVar1 = param_1 == 0;
  }
  else {
    if (param_2 != 0x1a) {
      if (param_2 != 0x1b) {
        return 0;
      }
      if (param_1 != 2) {
        return 0;
      }
      return 1;
    }
    bVar1 = param_1 == 6;
  }
  if (!bVar1) {
    return 0;
  }
  return 1;
}


// ==== FUN_00468cc0 @ 00468cc0

undefined4 __thiscall
FUN_00468cc0(void *this,float param_2,float param_3,float param_4,float param_5)

{
  if (((((*(float *)this <= param_2) || (*(float *)this <= param_4)) &&
       ((param_2 <= *(float *)((int)this + 8) || (param_4 <= *(float *)((int)this + 8))))) &&
      ((*(float *)((int)this + 4) <= param_3 || (*(float *)((int)this + 4) <= param_5)))) &&
     ((param_3 <= *(float *)((int)this + 0xc) ||
      (param_5 < *(float *)((int)this + 0xc) != (param_5 == *(float *)((int)this + 0xc)))))) {
    return 1;
  }
  return 0;
}


// ==== FUN_004571b0 @ 004571b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004571b0(float *param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  float fVar3;
  float fVar4;
  undefined4 *puVar5;
  float *pfVar6;
  uint uVar7;
  float fVar8;
  int iVar9;
  float fVar10;
  float10 fVar11;
  float local_80;
  float local_78;
  float local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined1 local_68 [16];
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined1 local_48 [52];
  int local_14;
  
  iVar9 = param_3 * 0x32 + DAT_005c8464;
  local_74 = -10000.0;
  local_78 = -10000.0;
  FUN_004818d0(&local_58,param_2);
  FUN_004819b0(&local_70,param_2);
  puVar5 = (undefined4 *)FUN_004ade70(iVar9,local_68,0);
  uVar1 = puVar5[1];
  uVar2 = *puVar5;
  FUN_0040e880(&local_58,uVar2,uVar1);
  FUN_0040e8f0(&local_58,uVar2,uVar1);
  iVar9 = FUN_004ade70(iVar9,local_68,0);
  uVar1 = *(undefined4 *)(iVar9 + 0xc);
  uVar2 = *(undefined4 *)(iVar9 + 8);
  FUN_0040e880(&local_58,uVar2,uVar1);
  FUN_0040e8f0(&local_58,uVar2,uVar1);
  FUN_004559f0(&local_58);
  local_80 = 10000.0;
  FUN_0041def0(local_48,local_58,local_54,local_50,local_4c,1);
  fVar8 = -10000.0;
  fVar10 = -10000.0;
  do {
    while( true ) {
      uVar7 = local_14 * 0x32 + DAT_005c8464;
      if ((uVar7 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar7)) {
        *param_1 = fVar10;
        param_1[1] = fVar8;
        return;
      }
      if (*(byte *)(uVar7 + 10) == DAT_005411c0) break;
LAB_00457390:
      FUN_004b1810(local_48);
    }
    pfVar6 = (float *)FUN_004568b0(uVar7,local_68,param_2,param_3);
    local_78 = *pfVar6;
    local_74 = pfVar6[1];
    if ((((local_78 <= _DAT_00500fc4) || ((float)DAT_00531928 <= local_78)) ||
        (local_74 <= _DAT_00500fc4)) ||
       (((float)DAT_00531928 <= local_74 ||
        (fVar11 = (float10)FUN_0041cfc0(&local_78,local_70,local_6c), fVar4 = local_74,
        fVar3 = local_78, (float10)local_80 <= fVar11)))) goto LAB_00457390;
    local_80 = (float)fVar11;
    FUN_004b1810(local_48);
    fVar8 = fVar4;
    fVar10 = fVar3;
  } while( true );
}


