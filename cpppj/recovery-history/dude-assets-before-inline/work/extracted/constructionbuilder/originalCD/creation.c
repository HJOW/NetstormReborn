// ==== FUN_004d1120 @ 004d1120

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d1120(int *param_1,undefined4 param_2,int param_3,int param_4,uint param_5)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int *piVar4;
  undefined4 *puVar5;
  int iVar6;
  uint uVar7;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar8;
  float10 fVar9;
  float10 extraout_ST1;
  undefined4 *puVar10;
  float *pfVar11;
  float fVar12;
  undefined4 uVar13;
  float fVar14;
  undefined4 uVar15;
  undefined4 local_48;
  undefined4 local_44;
  int local_40;
  int iStack_3c;
  undefined4 local_38;
  undefined1 local_34 [8];
  undefined4 local_2c;
  int *local_28;
  int local_24;
  float local_20;
  float local_1c;
  undefined4 local_18;
  int *local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004d147c;
  *unaff_FS_OFFSET = &local_10;
  iVar6 = DAT_005395dc + param_3 * 0x24;
  puVar5 = (undefined4 *)(iVar6 + 0xe);
  local_2c = *puVar5;
  iVar1 = FUN_00406f80(param_2,*(undefined1 *)(iVar6 + 10),local_2c,*(undefined4 *)(iVar6 + 0x12));
  if (iVar1 != 0) {
    local_24 = iVar1 * 0x24 + DAT_005395dc;
    local_14 = (int *)FUN_004ab390(DAT_0051cb5c,2);
    (**(code **)(*local_14 + 0x74))(param_2);
    uVar15 = 0;
    pfVar11 = &local_20;
    uVar13 = 0;
    FUN_004aeec0(local_34,0);
    FUN_004402d0(pfVar11,uVar13,uVar15);
    fVar8 = (float10)FUN_0048ee20(local_20);
    local_20 = (float)fVar8;
    uVar7 = 1;
    fVar8 = (float10)FUN_0048ee20(local_1c);
    local_1c = (float)fVar8;
    uVar2 = FUN_004abae0();
    if ((DAT_005395cc & uVar2) != 0) {
      uVar7 = DAT_005395cc | 1;
    }
    puVar10 = &local_48;
    fVar12 = local_20;
    fVar14 = local_1c;
    FUN_004aeec0(local_34,0);
    FUN_00440400(puVar10,fVar12,fVar14,uVar7);
    (**(code **)(*local_14 + 0x90))(local_48,local_44,0x10);
    uVar13 = 0;
    local_28 = (int *)FUN_0047d150(((int)local_14 - DAT_005395dc) / 0x24,local_20,local_1c,0,0,0,0);
    if (local_28 != (int *)0x0) {
      if ((param_5 & 0x7fffffff) != 0 || param_4 != 0) {
        iStack_3c = local_28[local_28[4] * 2 + 0x4b5];
        local_40 = local_28[local_28[4] * 2 + 0x4b4];
        iVar3 = FUN_004abf50();
        fVar8 = (float10)*(float *)(iVar3 + 0xe0);
        iVar1 = *local_28;
        if (DAT_00546778 == 0) {
          fVar9 = ((float10)(double)CONCAT44(param_5,param_4) - (float10)_DAT_005484a8) /
                  ((float10)(double)CONCAT44(iStack_3c,local_40) - (float10)_DAT_005484a8);
        }
        else {
          fVar9 = (float10)FUN_004f0857(iVar3,uVar13,0);
          fVar8 = extraout_ST1;
        }
        (**(code **)(iVar1 + 0x30))((float)(fVar9 * fVar8));
      }
      FUN_0048a430();
      local_18 = *puVar5;
      iVar1 = 0;
      FUN_0048aa40(local_18,*(undefined4 *)(iVar6 + 0x12),*(undefined1 *)(iVar6 + 10),
                   *(undefined1 *)(local_24 + 0x20));
      local_38 = 0;
      local_18 = *puVar5;
      FUN_0048ab40(local_18,*(undefined4 *)(iVar6 + 0x12));
      while( true ) {
        iVar6 = FUN_0048aaf0(&local_38);
        if (iVar6 == 0) break;
        piVar4 = (int *)FUN_004ab390(DAT_0051cae8,2);
        FUN_004abb00(iVar1);
        (**(code **)(*piVar4 + 0x74))(*(undefined1 *)(local_24 + 0x20));
        local_14 = (int *)FUN_004ac440();
        iVar6 = FUN_004abf50();
        uVar13 = 0;
        *(undefined1 *)((int)piVar4 + 0x22) = *(undefined1 *)(iVar6 + 0x130 + (int)local_14 * 4);
        puVar5 = (undefined4 *)FUN_004aef30(&local_40);
        (**(code **)(*piVar4 + 0x90))(*puVar5,puVar5[1],uVar13);
        local_14 = (int *)FUN_004f1650(0x28);
        local_8 = 0;
        if (local_14 != (int *)0x0) {
          FUN_0048efd0(0,((int)piVar4 - DAT_005395dc) / 0x24,DAT_0052da30);
        }
        iVar1 = iVar1 + 1;
        local_8 = 0xffffffff;
        FUN_0047d150(((int)piVar4 - DAT_005395dc) / 0x24,local_20,local_1c,0,0,0,0);
      }
      iVar1 = local_28[local_28[4] * 2 + 0x4b5];
      *param_1 = local_28[local_28[4] * 2 + 0x4b4];
      param_1[1] = iVar1;
      goto LAB_004d1188;
    }
  }
  *param_1 = 0;
  param_1[1] = 0x40140000;
LAB_004d1188:
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004aeec0 @ 004aeec0

undefined4 __thiscall FUN_004aeec0(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  uVar2 = *(undefined4 *)(param_1 + 0xe);
  uVar3 = *(undefined4 *)(param_1 + 0x12);
  uVar1 = param_2;
  FUN_004abf50(param_2,uVar2,uVar3,param_3);
  FUN_004449d0(uVar1,uVar2,uVar3,param_3);
  return param_2;
}


// ==== FUN_004449d0 @ 004449d0

float * FUN_004449d0(float *param_1,float param_2,float param_3,uint param_4)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  int local_8;
  int local_4;
  
  FUN_004442b0(&local_8,&local_4,param_4 & 1);
  fVar2 = (float)(1 - local_8) + param_2;
  pfVar1 = param_1 + 2;
  *param_1 = param_2;
  *pfVar1 = param_2;
  param_1[1] = param_3;
  param_1[3] = param_3;
  param_3 = (float)(1 - local_4) + param_3;
  FUN_0043f950();
  if (((((int)*param_1 < 1) || ((float)DAT_0052e9a8 <= *param_1)) || ((int)param_1[1] < 1)) ||
     ((float)DAT_0052e9a8 <= param_1[1])) {
    *param_1 = fVar2;
    param_1[1] = param_3;
  }
  else {
    fVar3 = *param_1;
    if (fVar2 <= *param_1) {
      fVar3 = fVar2;
    }
    *param_1 = fVar3;
    fVar3 = param_1[1];
    if (param_3 <= param_1[1]) {
      fVar3 = param_3;
    }
    param_1[1] = fVar3;
  }
  FUN_0043f950();
  if (((0 < (int)*pfVar1) && (*pfVar1 < (float)DAT_0052e9a8)) &&
     ((0 < (int)param_1[3] && (param_1[3] < (float)DAT_0052e9a8)))) {
    fVar3 = *pfVar1;
    if (*pfVar1 <= fVar2) {
      fVar3 = fVar2;
    }
    *pfVar1 = fVar3;
    fVar2 = param_1[3];
    if (param_1[3] <= param_3) {
      fVar2 = param_3;
    }
    param_1[3] = fVar2;
    return param_1;
  }
  *pfVar1 = fVar2;
  param_1[3] = param_3;
  return param_1;
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_0043f950 @ 0043f950

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_0043f950(float *param_1)

{
  float fVar1;
  
  fVar1 = *param_1;
  if (*param_1 <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  *param_1 = fVar1;
  fVar1 = param_1[1];
  if (param_1[1] <= _DAT_00501764) {
    fVar1 = _DAT_00501764;
  }
  param_1[1] = fVar1;
  fVar1 = *param_1;
  if (_DAT_00501768 <= *param_1) {
    fVar1 = _DAT_00501768;
  }
  *param_1 = fVar1;
  fVar1 = param_1[1];
  if (_DAT_00501768 <= param_1[1]) {
    fVar1 = _DAT_00501768;
  }
  param_1[1] = fVar1;
  return;
}


// ==== FUN_00413c10 @ 00413c10

undefined4 * __thiscall FUN_00413c10(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  param_1[1] = param_3;
  *param_1 = param_2;
  return param_1;
}


// ==== FUN_004402d0 @ 004402d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004402d0(float *param_1,float *param_2,int param_3,byte param_4)

{
  float10 fVar1;
  float10 fVar2;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  local_14 = *param_1;
  local_10 = param_1[1];
  local_c = param_1[2];
  local_8 = param_1[3];
  if ((param_4 & 4) != 0) {
    local_14 = local_14 - _DAT_00501764;
    local_10 = local_10 - _DAT_00501764;
    local_c = local_c + _DAT_00501764;
    local_8 = local_8 + _DAT_00501764;
  }
  if (param_3 == 0) {
    fVar1 = (float10)FUN_00440210();
    fVar2 = (float10)local_14;
    local_14 = local_10;
    local_4 = (float)(fVar1 * (float10)_DAT_0050177c + fVar2);
  }
  else if (param_3 == 2) {
    local_4 = local_c;
    fVar2 = (float10)FUN_00440220();
    local_14 = (float)(fVar2 * (float10)_DAT_0050177c + (float10)local_10);
  }
  else if (param_3 == 4) {
    fVar1 = (float10)FUN_00440210();
    fVar2 = (float10)local_14;
    local_14 = local_8;
    local_4 = (float)(fVar1 * (float10)_DAT_0050177c + fVar2);
  }
  else if (param_3 == 6) {
    local_4 = local_14;
    fVar2 = (float10)FUN_00440220();
    local_14 = (float)(fVar2 * (float10)_DAT_0050177c + (float10)local_10);
  }
  else {
    FUN_00490040(s_false_0051c4bc,s__ns_o_BoardCoord_cpp_0051c4a4,0x1f6);
  }
  *param_2 = local_4;
  param_2[1] = local_14;
  return;
}


// ==== FUN_0048ee20 @ 0048ee20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_0048ee20(float param_1)

{
  double dVar1;
  
  dVar1 = _floor((double)(param_1 + _DAT_00504654));
  return (float10)dVar1;
}


// ==== _floor @ 004f3540

/* Library Function - Single Match
    _floor
   
   Library: Visual Studio 1998 Release */

double __cdecl _floor(double _X)

{
  double dVar1;
  uint uVar2;
  int iVar3;
  float10 fVar4;
  
  uVar2 = FUN_004f9d30();
  if ((_X._6_2_ & 0x7ff0) != 0x7ff0) {
    fVar4 = (float10)__frnd();
    dVar1 = (double)fVar4;
    if (fVar4 == (float10)_X) {
      FUN_004f9d30();
      return dVar1;
    }
    if ((uVar2 & 0x20) != 0) {
      FUN_004f9d30();
      return dVar1;
    }
    fVar4 = (float10)__except1(0x10,0xb,_X,dVar1,uVar2);
    return (double)fVar4;
  }
  iVar3 = __sptype();
  if (0 < iVar3) {
    if (iVar3 < 3) {
      FUN_004f9d30();
      return _X;
    }
    if (iVar3 == 3) {
      fVar4 = (float10)__handle_qnan1(0xb,_X,uVar2);
      return (double)fVar4;
    }
  }
  fVar4 = (float10)__except1(8,0xb,_X,_X + 1.0,uVar2);
  return (double)fVar4;
}


// ==== FUN_004f9d30 @ 004f9d30

int FUN_004f9d30(void)

{
  short in_FPUControlWord;
  
  return (int)in_FPUControlWord;
}


// ==== __frnd @ 004fa180

/* Library Function - Single Match
    __frnd
   
   Library: Visual Studio 1998 Release */

float10 __frnd(double param_1)

{
  return (float10)ROUND(param_1);
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


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_0048a430 @ 0048a430

undefined1 * __fastcall FUN_0048a430(undefined1 *param_1)

{
  *param_1 = 0;
  return param_1;
}


// ==== FUN_0048aaf0 @ 0048aaf0

undefined4 FUN_0048aaf0(int *param_1)

{
  int *piVar1;
  int iVar2;
  
  iVar2 = 0;
  *param_1 = 0;
  piVar1 = DAT_00567658;
  if (0 < DAT_00567660) {
    do {
      if (*piVar1 != 0) {
        *param_1 = DAT_00567658[iVar2];
        DAT_00567658[iVar2] = 0;
        return 1;
      }
      iVar2 = iVar2 + 1;
      piVar1 = piVar1 + 1;
    } while (iVar2 < DAT_00567660);
  }
  return 0;
}


// ==== FUN_004abb00 @ 004abb00

void __thiscall FUN_004abb00(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xc) = param_2;
  if (*(byte *)(param_1 + 10) == DAT_0051ca8c) {
    FUN_00448c10((param_1 - DAT_005395dc) / 0x24);
  }
  return;
}


// ==== FUN_004ac440 @ 004ac440

undefined4 __fastcall FUN_004ac440(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x98 + DAT_0051c960);
}


// ==== FUN_004aef30 @ 004aef30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004aef30(int param_1,float *param_2)

{
  float fVar1;
  int *piVar2;
  int *piVar3;
  undefined4 uVar4;
  int local_10 [3];
  int local_4;
  
  piVar3 = &local_4;
  uVar4 = 0;
  piVar2 = local_10;
  FUN_004abf50(piVar2,piVar3,0);
  FUN_004442b0(piVar2,piVar3,uVar4);
  fVar1 = *(float *)(param_1 + 0x12);
  *param_2 = (float)local_10[0] * _DAT_005058cc + *(float *)(param_1 + 0xe);
  param_2[1] = (float)local_4 * _DAT_005058cc + fVar1;
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1c4) + *(int *)(param_1 + 0x1b8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1b8);
  return;
}


// ==== FUN_00440210 @ 00440210

float10 __fastcall FUN_00440210(float *param_1)

{
  return (float10)param_1[2] - (float10)*param_1;
}


