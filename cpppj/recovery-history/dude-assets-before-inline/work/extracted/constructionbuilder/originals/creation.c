// ==== FUN_00443000 @ 00443000

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00443000(undefined8 *param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  uint uVar7;
  undefined4 unaff_EBP;
  undefined4 unaff_EDI;
  int iVar8;
  float10 fVar9;
  undefined4 unaff_retaddr;
  int **ppiVar10;
  int *piVar11;
  undefined4 uVar12;
  void *pvVar13;
  undefined4 uVar14;
  int *piStack_140;
  void *pvStack_13c;
  int iStack_138;
  undefined1 auStack_130 [8];
  undefined1 auStack_128 [8];
  undefined4 uStack_120;
  undefined4 uStack_11c;
  void *pvStack_1c;
  undefined4 uStack_14;
  int *local_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_004f9e8e;
  local_c = ExceptionList;
  param_3 = param_3 * 0x32;
  iVar8 = param_3 + DAT_005c8464;
  ExceptionList = &local_c;
  iVar1 = FUN_0048fdb0(param_2,*(undefined1 *)(iVar8 + 10),
                       *(undefined4 *)(param_3 + 0xe + DAT_005c8464),
                       *(undefined4 *)(param_3 + 0x12 + DAT_005c8464));
  if (iVar1 == 0) {
    *param_1 = _DAT_005071e0;
    ExceptionList = local_c;
    return;
  }
  iVar1 = iVar1 * 0x32 + DAT_005c8464;
  piVar2 = (int *)FUN_004af530(DAT_00541270,2);
  (**(code **)(*piVar2 + 0x74))(param_2);
  uVar14 = 0;
  uVar12 = 0;
  ppiVar10 = &piStack_140;
  FUN_004ade70(auStack_130,0);
  FUN_0041d400(ppiVar10,uVar12,uVar14);
  fVar9 = (float10)FUN_0043fa60(piStack_140);
  piStack_140 = (int *)(float)fVar9;
  fVar9 = (float10)FUN_0043fa60(pvStack_13c);
  pvStack_13c = (void *)(float)fVar9;
  uVar7 = 1;
  uVar3 = FUN_004ac200();
  if ((DAT_005424b8 & uVar3) != 0) {
    uVar7 = DAT_005424b8 | 1;
  }
  puVar6 = &uStack_120;
  piVar11 = piStack_140;
  pvVar13 = pvStack_13c;
  FUN_004ade70(auStack_130,0);
  FUN_0041db70(puVar6,piVar11,pvVar13,uVar7);
  (**(code **)(*piVar2 + 0x90))(uStack_120,uStack_11c,0x10);
  piVar2 = (int *)FUN_00489080(((int)piVar2 - DAT_005c8464) / 0x32,unaff_EBP,unaff_EDI,0,0,0,0,0);
  if (piVar2 == (int *)0x0) {
    *(undefined8 *)local_c = _DAT_005071e0;
  }
  else {
    piStack_140 = piVar2;
    if ((double)CONCAT44(param_1,unaff_retaddr) != _DAT_00500460) {
      pvStack_13c = (void *)piVar2[piVar2[4] * 2 + 0x2714];
      iStack_138 = piVar2[piVar2[4] * 2 + 0x2715];
      iVar4 = FUN_004ac550();
      (**(code **)(*piVar2 + 0x30))
                ((float)((((float10)(double)CONCAT44(param_1,unaff_retaddr) - (float10)_DAT_0055b4d0
                          ) * (float10)*(float *)(iVar4 + 0xe0)) /
                        ((float10)(double)CONCAT44(iStack_138,pvStack_13c) - (float10)_DAT_0055b4d0)
                        ),0);
    }
    FUN_00473010();
    FUN_004734d0(*(undefined4 *)(iVar8 + 0xe),*(undefined4 *)(iVar8 + 0x12),
                 *(undefined1 *)(iVar8 + 10),*(undefined1 *)(iVar1 + 0x22));
    iVar4 = 0;
    FUN_00474970(*(undefined4 *)(iVar8 + 0xe),*(undefined4 *)(iVar8 + 0x12));
    iVar8 = FUN_00473570(&stack0xfffffebc);
    while (iVar8 != 0) {
      piVar2 = (int *)FUN_004af530(DAT_005411fc,2);
      FUN_004ac220(iVar4);
      (**(code **)(*piVar2 + 0x74))(*(undefined1 *)(iVar1 + 0x22));
      iVar8 = FUN_004ac970();
      iVar5 = FUN_004ac550();
      piVar2[9] = *(int *)(iVar5 + 0x130 + iVar8 * 4);
      puVar6 = (undefined4 *)FUN_004adea0(auStack_128);
      (**(code **)(*piVar2 + 0x90))(*puVar6,puVar6[1],0);
      pvStack_13c = operator_new(0x28);
      uStack_14 = 0;
      if (pvStack_13c != (void *)0x0) {
        FUN_00496e80(0,((int)piVar2 - DAT_005c8464) / 0x32,DAT_00532750);
      }
      uStack_14 = 0xffffffff;
      FUN_00489080(((int)piVar2 - DAT_005c8464) / 0x32,unaff_EBP,unaff_EDI,0,0,0,0,0);
      iVar4 = iVar4 + 1;
      iVar8 = FUN_00473570(&stack0xfffffebc);
      piVar2 = piStack_140;
    }
    iVar1 = piVar2[4];
    *local_c = piVar2[iVar1 * 2 + 0x2714];
    local_c[1] = piVar2[iVar1 * 2 + 0x2715];
  }
  ExceptionList = pvStack_1c;
  return;
}


// ==== FUN_004ade70 @ 004ade70

undefined4 __thiscall FUN_004ade70(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = *(undefined4 *)(param_1 + 0xe);
  uVar2 = *(undefined4 *)(param_1 + 0x12);
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  FUN_0049ae80(param_2,uVar1,uVar2,param_3);
  return param_2;
}


// ==== FUN_0049ae80 @ 0049ae80

float * __thiscall FUN_0049ae80(int param_1,float *param_2,float param_3,float param_4,byte param_5)

{
  float fVar1;
  int iVar2;
  
  if ((param_5 & 1) == 0) {
    iVar2 = *(int *)(param_1 + 0x1d8);
  }
  else {
    iVar2 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
  }
  fVar1 = param_3 - (float)(*(int *)(param_1 + 0x1d4) + -1);
  param_2[1] = param_4;
  param_2[3] = param_4;
  *param_2 = param_3;
  param_2[2] = param_3;
  param_4 = param_4 - (float)(iVar2 + -1);
  FUN_0040e880(fVar1,param_4);
  FUN_0040e8f0(fVar1,param_4);
  return param_2;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004ac550 @ 004ac550

void __fastcall FUN_004ac550(int param_1)

{
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return;
}


// ==== FUN_0040e880 @ 0040e880

void __thiscall FUN_0040e880(float *param_1,float param_2,float param_3)

{
  int iVar1;
  
  FUN_0041d0d0();
  iVar1 = FUN_0040e800();
  if (iVar1 == 0) {
    *param_1 = param_2;
    param_1[1] = param_3;
    return;
  }
  if (*param_1 < param_2) {
    param_2 = *param_1;
  }
  *param_1 = param_2;
  if (param_3 <= param_1[1]) {
    param_1[1] = param_3;
    return;
  }
  param_1[1] = param_1[1];
  return;
}


// ==== FUN_0040e8f0 @ 0040e8f0

void __thiscall FUN_0040e8f0(int param_1,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  
  FUN_0041d0d0();
  pfVar1 = (float *)(param_1 + 8);
  iVar2 = FUN_0040e800();
  if (iVar2 == 0) {
    *pfVar1 = param_2;
    *(float *)(param_1 + 0xc) = param_3;
    return;
  }
  if (param_2 < *pfVar1) {
    param_2 = *pfVar1;
  }
  *pfVar1 = param_2;
  if (*(float *)(param_1 + 0xc) <= param_3) {
    *(float *)(param_1 + 0xc) = param_3;
    return;
  }
  *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(param_1 + 0xc);
  return;
}


// ==== FUN_0040e800 @ 0040e800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __fastcall FUN_0040e800(float *param_1)

{
  if ((((_DAT_00500fc4 < *param_1) && (*param_1 < (float)DAT_00531928)) &&
      (_DAT_00500fc4 < param_1[1])) && (param_1[1] < (float)DAT_00531928)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0040eaf0 @ 0040eaf0

undefined2 FUN_0040eaf0(void)

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


// ==== FUN_0041d0d0 @ 0041d0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_0041d0d0(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = _DAT_005012d0;
  if (_DAT_005012d0 <= *param_1) {
    fVar1 = *param_1;
  }
  *param_1 = fVar1;
  fVar2 = _DAT_005012d0;
  if (_DAT_005012d0 <= param_1[1]) {
    fVar2 = param_1[1];
  }
  param_1[1] = fVar2;
  if (_DAT_005022e8 < fVar1) {
    fVar1 = _DAT_005022e8;
  }
  *param_1 = fVar1;
  if (fVar2 <= _DAT_005022e8) {
    param_1[1] = fVar2;
    return;
  }
  param_1[1] = 255.0;
  return;
}


// ==== FUN_0041d400 @ 0041d400

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d400(float *param_1,float *param_2,int param_3,float param_4)

{
  float fVar1;
  float local_10;
  float local_c;
  float local_4;
  
  local_10 = *param_1;
  local_c = param_1[1];
  fVar1 = param_1[2];
  local_4 = param_1[3];
  if (((uint)param_4 & 4) != 0) {
    local_10 = local_10 - _DAT_005012d0;
    local_c = local_c - _DAT_005012d0;
    fVar1 = fVar1 + _DAT_005012d0;
    local_4 = local_4 + _DAT_005012d0;
  }
  if (param_3 == 0) {
    *param_2 = (fVar1 - local_10) * _DAT_00500ee4 + local_10;
    param_2[1] = local_c;
    return;
  }
  if (param_3 != 2) {
    if (param_3 == 4) {
      *param_2 = (fVar1 - local_10) * _DAT_00500ee4 + local_10;
      param_2[1] = local_4;
      return;
    }
    if (param_3 != 6) {
      FUN_004e0620("false","\\Ns\\O\\Boardcoord.cpp",0x1f9);
      *param_2 = param_4;
      param_2[1] = param_4;
      return;
    }
    param_4 = local_10;
    fVar1 = param_4;
  }
  param_4 = fVar1;
  fVar1 = (local_4 - local_c) * _DAT_00500ee4;
  *param_2 = param_4;
  param_2[1] = fVar1 + local_c;
  return;
}


// ==== FUN_0043fa60 @ 0043fa60

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0043fa60(float param_1)

{
  FUN_004e62a0((double)(param_1 + _DAT_00500ee4));
  return;
}


// ==== FUN_004e62a0 @ 004e62a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_004e62a0(double param_1,undefined2 param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  ushort in_FPUControlWord;
  float10 fVar4;
  double dVar5;
  longlong lVar6;
  undefined4 uVar7;
  undefined8 uVar8;
  
  if ((DAT_005e5340 != 0) && ((MXCSR & 0x1f80) == 0x1f80 && (in_FPUControlWord & 0x7f) == 0x7f)) {
    uVar2 = (uint)((ulonglong)param_1 >> 0x20);
    uVar1 = uVar2 >> 0x14;
    lVar6 = CONCAT44(_UNK_0051a6f4,_DAT_0051a6f0 - (uVar2 >> 0x14 & _DAT_0051a720));
    if ((uVar1 & 0x800) == 0) {
      if (uVar1 < 0x3ff) {
        return (float10)0;
      }
      if (uVar1 < 0x433) {
        return (float10)(double)(((ulonglong)param_1 >> lVar6) << lVar6);
      }
    }
    else {
      dVar5 = (double)(((ulonglong)param_1 >> lVar6) << lVar6);
      if (uVar1 < 0xbff) {
        return (float10)(double)((-(ulonglong)(param_1 < _DAT_0051a710) | (ulonglong)_DAT_0051a710)
                                & _DAT_0051a700);
      }
      if (uVar1 < 0xc33) {
        return (float10)(dVar5 - (double)(-(ulonglong)(param_1 < dVar5) & _DAT_0051a6e0));
      }
    }
    if (NAN(param_1)) {
      ___libm_error_support(&param_1,&param_1,&param_1,0x3ed);
    }
    return (float10)(double)CONCAT26(param_1._6_2_,param_1._0_6_);
  }
  uVar2 = __ctrlfp(DAT_00544384,0xffff);
  if ((param_1._6_2_ & 0x7ff0) == 0x7ff0) {
    iVar3 = __sptype();
    if (0 < iVar3) {
      if (iVar3 < 3) {
        __ctrlfp(uVar2,0xffff);
        return (float10)(double)CONCAT26(param_1._6_2_,param_1._0_6_);
      }
      if (iVar3 == 3) {
        fVar4 = (float10)__handle_qnan1();
        return fVar4;
      }
    }
    dVar5 = (double)CONCAT26(param_1._6_2_,param_1._0_6_) + _DAT_00501580;
    uVar8 = CONCAT26(param_1._6_2_,param_1._0_6_);
    uVar7 = 8;
  }
  else {
    fVar4 = (float10)__frnd((int)param_1._0_6_,(int)(CONCAT26(param_1._6_2_,param_1._0_6_) >> 0x20))
    ;
    dVar5 = (double)fVar4;
    if ((dVar5 == (double)CONCAT26(param_1._6_2_,param_1._0_6_)) || ((uVar2 & 0x20) != 0)) {
      __ctrlfp(uVar2,0xffff);
      return (float10)dVar5;
    }
    uVar8 = CONCAT26(param_1._6_2_,param_1._0_6_);
    uVar7 = 0x10;
  }
  fVar4 = (float10)__except1(uVar7,0xb,uVar8,dVar5,uVar2);
  return fVar4;
}


// ==== __ctrlfp @ 004ec4c1

/* Library Function - Single Match
    __ctrlfp
   
   Library: Visual Studio 2003 Release */

int __ctrlfp(void)

{
  short in_FPUControlWord;
  
  return (int)in_FPUControlWord;
}


// ==== __frnd @ 004f5bac

/* Library Function - Single Match
    __frnd
   
   Library: Visual Studio 2003 Release */

float10 __frnd(double param_1)

{
  return (float10)ROUND(param_1);
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


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_00473010 @ 00473010

void __fastcall FUN_00473010(undefined1 *param_1)

{
  *param_1 = 0;
  return;
}


// ==== FUN_00473570 @ 00473570

undefined4 FUN_00473570(undefined4 *param_1)

{
  int iVar1;
  
  *param_1 = 0;
  iVar1 = 0;
  if (0 < DAT_00592f54) {
    do {
      if (*(int *)(DAT_00592f4c + iVar1 * 4) != 0) {
        *param_1 = *(undefined4 *)(DAT_00592f4c + iVar1 * 4);
        *(undefined4 *)(DAT_00592f4c + iVar1 * 4) = 0;
        return 1;
      }
      iVar1 = iVar1 + 1;
    } while (iVar1 < DAT_00592f54);
  }
  return 0;
}


// ==== FUN_004ac220 @ 004ac220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ac220(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xc) = param_2;
  if (*(byte *)(param_1 + 10) == _DAT_005411a0) {
    FUN_004214a0((param_1 - DAT_005c8464) / 0x32);
  }
  return;
}


// ==== FUN_004ac970 @ 004ac970

undefined4 __fastcall FUN_004ac970(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0x98);
}


// ==== FUN_004adea0 @ 004adea0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004adea0(int param_1,float *param_2)

{
  float fVar1;
  int *piVar2;
  int *piVar3;
  undefined4 uVar4;
  int local_10;
  int local_c [3];
  
  uVar4 = 0;
  piVar3 = local_c;
  piVar2 = &local_10;
  FUN_0049a840(*(undefined1 *)(param_1 + 10),piVar2,piVar3,0);
  FUN_0049a900(piVar2,piVar3,uVar4);
  fVar1 = *(float *)(param_1 + 0x12);
  *param_2 = *(float *)(param_1 + 0xe) - (float)local_10 * _DAT_00500ee4;
  param_2[1] = fVar1 - (float)local_c[0] * _DAT_00500ee4;
  return;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1d8);
  return;
}


