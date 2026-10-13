// ==== FUN_00461b40 @ 00461b40

void __thiscall FUN_00461b40(void *this,uint param_2)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_b8 [8];
  undefined **local_b0 [13];
  int local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined **local_70 [13];
  int local_3c;
  undefined4 local_38;
  int local_34;
  undefined1 local_30 [4];
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  int local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00461e8f;
  *unaff_FS_OFFSET = &local_10;
  if ((param_2 & 0x300000) != 0) {
    uVar1 = FUN_004abae0(this,1);
    FUN_00461970(((int)this - DAT_005395dc) / 0x24,uVar1);
  }
  if (((*(byte *)((int)this + 0x23) & 9) == 0) && (uVar2 = FUN_004abac0(this), (uVar2 & 0x400) != 0)
     ) {
    local_18 = *(undefined4 *)((int)this + 0xe);
    iVar3 = FUN_004eb5c0(local_18,*(undefined4 *)((int)this + 0x12),0x1000000);
    if (((iVar3 != 0) && (uVar2 = FUN_004abae0(this), (uVar2 & 0x10000000) == 0)) &&
       (((param_2 & 0x200000) != 0 || (*(byte *)((int)this + 10) != DAT_0051cb90)))) {
      FUN_0048f960(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),param_2,
                   *(undefined1 *)((int)this + 0x20));
    }
  }
  if (((DAT_00540a20 != 0) && (iVar3 = FUN_004aecf0(this), iVar3 != 0)) &&
     ((*(byte *)((int)this + 0x23) & 9) == 0)) {
    local_34 = FUN_004aaa50(param_2);
    if (((param_2 & 0x200000) == 0) &&
       (((iVar3 = FUN_004ac420(this), iVar3 == 0 || (iVar3 = FUN_004ac420(this), iVar3 == 1)) ||
        (uVar2 = FUN_004abae0(this), (uVar2 & 0x200) != 0)))) {
      iVar3 = FUN_004aed30(this);
      local_1c = iVar3 / 2 + 1;
      uVar2 = FUN_004abae0(this);
      if ((uVar2 & 0x200) != 0) {
        local_1c = 400;
      }
      if ((param_2 & 0x400000) == 0) {
        iVar3 = FUN_004abf50(this);
        local_14 = *(int *)(iVar3 + 0x1b4) / 2 + 1;
        puVar4 = (undefined4 *)FUN_004aef30(this,local_b8);
        local_28 = *puVar4;
        local_24 = puVar4[1];
        local_b0[0] = &PTR_FUN_005003d0;
        local_78 = 1;
        local_74 = 0;
        FUN_00440a80(&local_28);
        local_2c = __ftol();
        iVar3 = __ftol();
        FUN_004eae20(local_b0,iVar3 - local_14,local_2c - local_14,local_14 + iVar3,
                     local_2c + local_14);
        while (local_7c != 0) {
          iVar3 = FUN_004aecf0(local_7c * 0x24 + DAT_005395dc);
          if (((iVar3 != 0) &&
              (iVar3 = DAT_005395dc + local_7c * 0x24, (*(byte *)(iVar3 + 0xb) & 2) == 0)) &&
             ((*(byte *)(iVar3 + 0x23) & 1) == 0)) {
            local_14 = FUN_004f1650(0x28);
            local_8 = 0;
            iVar3 = 0;
            if (local_14 != 0) {
              iVar3 = FUN_0048efd0(local_14,DAT_00520fdc,(local_7c * 0x24) / 0x24,0);
            }
            local_8 = 0xffffffff;
            *(int *)(iVar3 + 0x20) = local_34 * 0x10000 + local_1c;
          }
          FUN_004eafe0(local_b0);
        }
      }
    }
    uVar2 = FUN_004abae0(this);
    if ((uVar2 & 0x130000) == 0) {
      puVar4 = (undefined4 *)FUN_004aef30(this,local_30);
      local_20 = *puVar4;
      local_1c = puVar4[1];
      local_70[0] = &PTR_FUN_005003d0;
      local_38 = 0;
      local_34 = 0;
      FUN_00440a80(&local_20);
      local_14 = __ftol();
      iVar3 = __ftol();
      FUN_004eae20(local_70,iVar3 + -2,local_14 + -2,iVar3 + 2,local_14 + 2);
      while (local_3c != 0) {
        uVar2 = FUN_004abae0(local_3c * 0x24 + DAT_005395dc);
        if ((uVar2 & 4) != 0) {
          iVar3 = FUN_00449670(local_3c);
          if ((iVar3 == 0) || (iVar3 = FUN_00449170(local_3c), iVar3 != 0)) {
            iVar3 = FUN_00449710(local_3c);
            if (iVar3 == 0) {
              FUN_00448c30(local_3c);
            }
          }
          else if (DAT_00540a2c != 0) {
            (**(code **)(*(int *)(DAT_005395dc + local_3c * 0x24) + 0x10))(0);
          }
        }
        FUN_004eafe0(local_70);
      }
    }
  }
  FUN_004adbe0(this,param_2);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00461970 @ 00461970

void __cdecl FUN_00461970(int param_1,uint param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  
  if (((param_1 != 0) && (iVar2 = DAT_005395dc + param_1 * 0x24, (*(byte *)(iVar2 + 0x23) & 1) == 0)
      ) && ((param_2 & 0x174200) != 0)) {
    uVar3 = 1;
    if ((param_2 & 0x4200) != 0) {
      uVar3 = 5;
    }
    if ((param_2 & 0x20000) != 0) {
      uVar3 = 2;
    }
    uVar1 = (uint)*(byte *)(iVar2 + 10);
    if (((DAT_0051cb58 != uVar1) && (DAT_0051cbb4 != uVar1)) &&
       ((DAT_0051cb84 != uVar1 && (DAT_0051cb6c != uVar1)))) {
      if (DAT_0051cb60 == uVar1) {
        uVar3 = 3;
      }
      if (0x14 < DAT_005b72c0) {
        uVar3 = uVar3 / 2;
      }
      iVar2 = 0;
      if (uVar3 != 0) {
        do {
          if (param_3 == 0) {
            FUN_004e8e00(param_1,iVar2);
          }
          else {
            FUN_004e8d20();
          }
          iVar2 = iVar2 + 1;
        } while (iVar2 < (int)uVar3);
      }
    }
  }
  return;
}


// ==== FUN_004e8e00 @ 004e8e00

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8e00(int param_1,int param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  undefined1 local_8 [8];
  
  if ((param_1 != 0) && (iVar3 = param_1 * 0x24 + DAT_005395dc, (*(byte *)(iVar3 + 0xb) & 1) == 0))
  {
    puVar1 = (undefined4 *)
             FUN_004e8a30(local_8,param_1,*(undefined1 *)(iVar3 + 0x20),0x5104,
                          (float)param_2 * _DAT_005072e8);
    uVar2 = FUN_004e8b30(*puVar1,puVar1[1]);
    FUN_004e8ba0(uVar2);
  }
  return;
}


// ==== FUN_004e8d20 @ 004e8d20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8d20(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  float10 fVar6;
  float10 fVar7;
  
  if ((param_1 != 0) && (iVar5 = DAT_005395dc + param_1 * 0x24, (*(byte *)(iVar5 + 0xb) & 1) == 0))
  {
    fVar1 = *(float *)(iVar5 + 0x12);
    fVar2 = *(float *)(iVar5 + 0xe);
    iVar3 = FUN_004abf50(iVar5);
    fVar6 = (float10)FUN_0048cc50(0x3dcccccd,(float)*(int *)(iVar3 + 0x1b4) - _DAT_005072c8);
    iVar3 = FUN_004abf50(iVar5);
    fVar7 = (float10)FUN_0048cc50(0x3dcccccd,(float)*(int *)(iVar3 + 0x1b8) - _DAT_005072c8);
    uVar4 = FUN_004e8b30((float)((float10)fVar2 - fVar6),(float)((float10)fVar1 - fVar7),
                         *(undefined1 *)(iVar5 + 0x20),0x5103,(float)param_2 * _DAT_005072c8);
    FUN_004e8ba0(uVar4);
  }
  return;
}


// ==== FUN_004e8a30 @ 004e8a30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8a30(float *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  float local_c;
  
  if (param_2 == 0) {
    FUN_00490040(&DAT_00541818,s_Dude_cpp_0054180c,0x29);
  }
  iVar4 = DAT_005395dc + param_2 * 0x24;
  local_c = *(float *)(iVar4 + 0x12);
  FUN_004abf50(iVar4);
  iVar2 = __ftol();
  fVar1 = (float)iVar2 + _DAT_005072b4;
  iVar2 = FUN_004abf50(iVar4);
  local_c = local_c - (float)*(int *)(iVar2 + 0x1b8);
  uVar3 = FUN_004abae0(iVar4);
  if ((DAT_005395cc & uVar3) == 0) {
    local_c = local_c - _DAT_005072b4;
  }
  else {
    uVar3 = FUN_004abae0(iVar4);
    if ((uVar3 & 0x10040000) != 0) {
      local_c = local_c + _DAT_005072b8;
    }
  }
  *param_1 = fVar1;
  param_1[1] = local_c;
  return;
}


// ==== FUN_004e8b30 @ 004e8b30

int __cdecl FUN_004e8b30(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  undefined4 unaff_retaddr;
  
  if ((DAT_005b72c0 < DAT_005b72bc) && (DAT_005417c8 == 0)) {
    piVar1 = (int *)FUN_004ab390(DAT_0051ca5c,2);
    *(undefined1 *)((int)piVar1 + 0x22) = 0;
    (**(code **)(*piVar1 + 0x74))(param_3);
    (**(code **)(*piVar1 + 0x90))(unaff_retaddr,param_1,0);
    return ((int)piVar1 - DAT_005395dc) / 0x24;
  }
  return 0;
}


// ==== FUN_004e8ba0 @ 004e8ba0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8ba0(int param_1,int param_2,float param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_004e8c8a;
  *unaff_FS_OFFSET = &local_10;
  if ((param_1 != 0) && (iVar3 = DAT_005395dc + param_1 * 0x24, (*(byte *)(iVar3 + 0xb) & 3) == 0))
  {
    if ((*(short *)(iVar3 + 0xc) == 0x5105) && (param_2 != 0x5105)) {
      param_2 = 0x5105;
    }
    iVar1 = FUN_004ac9c0(iVar3,0xfff0,0x5100);
    if (iVar1 == 0) {
      iVar2 = FUN_004f1650(0x28);
      iVar1 = 0;
      local_8 = 0;
      if (iVar2 != 0) {
        iVar1 = FUN_0048efd0(iVar2,param_2,(iVar3 - DAT_005395dc) / 0x24,0x40a00000);
      }
      local_8 = 0xffffffff;
    }
    else {
      *(undefined4 *)(iVar1 + 0x20) = 0;
      *(int *)(iVar1 + 0x18) = param_2;
    }
    *(double *)(iVar1 + 0x10) = _DAT_005484a8 + (double)param_3;
    FUN_004abb00(iVar3,param_2);
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __thiscall FUN_004abac0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004ac420 @ 004ac420

undefined4 __thiscall FUN_004ac420(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x9c + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004aecf0 @ 004aecf0

byte __thiscall FUN_004aecf0(void *this)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = FUN_004abae0();
  if ((uVar2 & 0x400000) == 0) {
    bVar1 = FUN_004abac0(this);
    return bVar1 & 0x10;
  }
  return *(byte *)((int)this + 0x23) & 1;
}


// ==== FUN_004aed30 @ 004aed30

undefined4 __thiscall FUN_004aed30(void *this)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x400000) == 0) {
    uVar2 = FUN_00444720((uint)*(byte *)((int)this + 10) * 0x1d4 + DAT_0051c960);
    return uVar2;
  }
  if ((*(byte *)((int)this + 0x23) & 1) == 0) {
    return 0;
  }
  uVar2 = FUN_00444720(DAT_0051cbd4 * 0x1d4 + DAT_0051c960);
  return uVar2;
}


// ==== FUN_00444720 @ 00444720

int __thiscall FUN_00444720(void *this)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*(int *)this + (*(int *)this >> 0x1f & 3U)) >> 2;
    }
  }
  return *(int *)this;
}


// ==== FUN_004aef30 @ 004aef30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004aef30(void *this,float *param_2)

{
  float fVar1;
  undefined4 uVar2;
  int *piVar3;
  int *piVar4;
  undefined4 uVar5;
  int local_10 [3];
  int local_4;
  
  piVar4 = &local_4;
  uVar5 = 0;
  piVar3 = local_10;
  uVar2 = FUN_004abf50(this,piVar3,piVar4,0);
  FUN_004442b0(uVar2,piVar3,piVar4,uVar5);
  fVar1 = *(float *)((int)this + 0x12);
  *param_2 = (float)local_10[0] * _DAT_005058cc + *(float *)((int)this + 0xe);
  param_2[1] = (float)local_4 * _DAT_005058cc + fVar1;
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1c4) + *(int *)((int)this + 0x1b8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1b8);
  return;
}


// ==== FUN_004aaa50 @ 004aaa50

uint __cdecl FUN_004aaa50(uint param_1)

{
  uint uVar1;
  
  uVar1 = (param_1 & 0xf0000) >> 0x10;
  return (uVar1 == 0) - 1 & uVar1;
}


// ==== FUN_00440a50 @ 00440a50

void __thiscall FUN_00440a50(void *this)

{
  int iVar1;
  
  iVar1 = __ftol();
  *(float *)this = (float)iVar1;
  iVar1 = __ftol();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== FUN_00440a80 @ 00440a80

void __thiscall FUN_00440a80(void *this)

{
  int iVar1;
  
  iVar1 = __ftol();
  *(float *)this = (float)iVar1;
  iVar1 = __ftol();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_004eae20 @ 004eae20

void __thiscall FUN_004eae20(void *this,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int iVar6;
  int iVar7;
  
  iVar2 = DAT_0052e9a8;
  if (param_4 == -1) {
    param_4 = DAT_0052e9a8;
  }
  if (param_5 == -1) {
    param_5 = DAT_0052e9a8;
  }
  if (param_2 < 1) {
    param_2 = 0;
  }
  *(int *)((int)this + 4) = param_2;
  if (param_3 < 1) {
    param_3 = 0;
  }
  *(int *)((int)this + 8) = param_3;
  iVar1 = iVar2 + -1;
  if (iVar1 <= param_4) {
    param_4 = iVar1;
  }
  *(int *)((int)this + 0xc) = param_4;
  if (iVar1 <= param_5) {
    param_5 = iVar1;
  }
  *(int *)((int)this + 0x10) = param_5;
  *(undefined4 *)((int)this + 0x24) = 1;
  if (*(int *)((int)this + 0x38) == 0) {
    *(undefined4 *)((int)this + 0x24) = 0;
  }
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = iVar2 / (int)(&DAT_0052d598)[iVar1] + -1;
  iVar3 = param_2 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_2 / (int)(&DAT_0052d598)[iVar1]) {
    iVar3 = iVar2;
  }
  *(int *)((int)this + 0x14) = iVar3;
  iVar6 = param_3 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_3 / (int)(&DAT_0052d598)[iVar1]) {
    iVar6 = iVar2;
  }
  *(int *)((int)this + 0x18) = iVar6;
  iVar7 = param_4 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_4 / (int)(&DAT_0052d598)[iVar1]) {
    iVar7 = iVar2;
  }
  *(int *)((int)this + 0x1c) = iVar7;
  iVar4 = param_5 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_5 / (int)(&DAT_0052d598)[iVar1]) {
    iVar4 = iVar2;
  }
  *(int *)((int)this + 0x20) = iVar4;
  *(int *)((int)this + 0x1c) = iVar7 + 1;
  *(int *)((int)this + 0x20) = iVar4 + 1;
  if (iVar7 + 1 < 0) {
    *(undefined4 *)((int)this + 0x1c) = 0;
  }
  if (iVar4 + 1 < 0) {
    *(undefined4 *)((int)this + 0x20) = 0;
  }
  if (iVar2 < *(int *)((int)this + 0x1c)) {
    *(int *)((int)this + 0x1c) = iVar2;
  }
  if (iVar2 < *(int *)((int)this + 0x20)) {
    *(int *)((int)this + 0x20) = iVar2;
  }
  *(int *)((int)this + 0x28) = iVar3;
  *(int *)((int)this + 0x2c) = iVar6;
  *(undefined4 *)((int)this + 0x34) = 0;
  puVar5 = (ushort *)FUN_00479d90(&DAT_005670c0,iVar3,iVar6,*(undefined4 *)((int)this + 0x24));
  *(uint *)((int)this + 0x30) = (uint)*puVar5;
  (**(code **)(*(int *)this + 4))();
  return;
}


// ==== FUN_004eafe0 @ 004eafe0

void __thiscall FUN_004eafe0(void *this)

{
  float *pfVar1;
  byte bVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  ushort *puVar9;
  int iVar10;
  uint uVar11;
  int local_8;
  int local_4;
  
  iVar5 = DAT_005395dc;
LAB_004eafef:
  do {
    uVar11 = *(uint *)((int)this + 0x30);
    while (uVar11 == 0) {
      iVar5 = *(int *)((int)this + 0x28) + 1;
      *(int *)((int)this + 0x28) = iVar5;
      if (*(int *)((int)this + 0x1c) < iVar5) {
        *(undefined4 *)((int)this + 0x28) = *(undefined4 *)((int)this + 0x14);
        iVar5 = *(int *)((int)this + 0x2c) + 1;
        *(int *)((int)this + 0x2c) = iVar5;
        if (*(int *)((int)this + 0x20) < iVar5) {
          iVar5 = *(int *)((int)this + 0x24) + 1;
          *(int *)((int)this + 0x24) = iVar5;
          if (3 < iVar5) {
            *(undefined4 *)((int)this + 0x34) = 0;
            return;
          }
          iVar10 = DAT_0052e9a8 / (int)(&DAT_0052d598)[iVar5] + -1;
          iVar6 = *(int *)((int)this + 4) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar6) {
            iVar6 = iVar10;
          }
          *(int *)((int)this + 0x14) = iVar6;
          iVar7 = *(int *)((int)this + 8) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar7) {
            iVar7 = iVar10;
          }
          *(int *)((int)this + 0x18) = iVar7;
          iVar8 = *(int *)((int)this + 0xc) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar8) {
            iVar8 = iVar10;
          }
          *(int *)((int)this + 0x1c) = iVar8;
          iVar5 = *(int *)((int)this + 0x10) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar5) {
            iVar5 = iVar10;
          }
          *(int *)((int)this + 0x20) = iVar5;
          *(int *)((int)this + 0x1c) = iVar8 + 1;
          *(int *)((int)this + 0x20) = iVar5 + 1;
          if (iVar8 + 1 < 0) {
            *(undefined4 *)((int)this + 0x1c) = 0;
          }
          if (iVar5 + 1 < 0) {
            *(undefined4 *)((int)this + 0x20) = 0;
          }
          if (iVar10 < *(int *)((int)this + 0x1c)) {
            *(int *)((int)this + 0x1c) = iVar10;
          }
          if (iVar10 < *(int *)((int)this + 0x20)) {
            *(int *)((int)this + 0x20) = iVar10;
          }
          *(int *)((int)this + 0x2c) = iVar7;
          *(undefined4 *)((int)this + 0x34) = 0;
          *(int *)((int)this + 0x28) = iVar6;
        }
      }
      puVar9 = (ushort *)
               FUN_00479d90(&DAT_005670c0,*(undefined4 *)((int)this + 0x28),
                            *(undefined4 *)((int)this + 0x2c),*(undefined4 *)((int)this + 0x24));
      iVar5 = DAT_005395dc;
      uVar11 = (uint)*puVar9;
      *(uint *)((int)this + 0x30) = uVar11;
    }
    do {
      iVar10 = *(int *)((int)this + 0x30);
      *(int *)((int)this + 0x34) = iVar10;
      if (iVar10 == 0) goto LAB_004eafef;
      iVar10 = iVar10 * 0x24 + iVar5;
      bVar2 = *(byte *)(iVar10 + 0x23);
      *(uint *)((int)this + 0x30) = (uint)*(ushort *)(iVar10 + 4);
    } while ((bVar2 & 8) != 0);
    iVar10 = *(int *)((int)this + 0x34) * 0x24;
    pfVar1 = (float *)(iVar10 + 0xe + iVar5);
    fVar3 = *pfVar1;
    fVar4 = pfVar1[1];
    if (((((int)fVar3 < 1) || ((float)DAT_0052e9a8 <= fVar3)) || ((int)fVar4 < 1)) ||
       ((float)DAT_0052e9a8 <= fVar4)) {
      FUN_00490040(s_pos_isValid___00541890,s__ns_o_SquidFinder_cpp_00541878,0x89);
    }
    FUN_004442b0(DAT_0051c960 + (uint)*(byte *)(iVar10 + iVar5 + 10) * 0x1d4,&local_8,&local_4,
                 *(undefined4 *)((int)this + 0x3c));
    iVar10 = __ftol();
    iVar6 = __ftol();
    iVar8 = (iVar10 - local_8) + 1;
    iVar7 = (iVar6 - local_4) + 1;
    iVar5 = DAT_005395dc;
    if ((((((*(int *)((int)this + 4) <= iVar10) || (*(int *)((int)this + 4) <= iVar8)) &&
          ((iVar10 <= *(int *)((int)this + 0xc) || (iVar8 <= *(int *)((int)this + 0xc))))) &&
         ((*(int *)((int)this + 8) <= iVar6 || (*(int *)((int)this + 8) <= iVar7)))) &&
        ((iVar6 <= *(int *)((int)this + 0x10) || (iVar7 <= *(int *)((int)this + 0x10))))) &&
       (iVar10 = (*(code *)**(undefined4 **)this)(*(int *)((int)this + 0x34) * 0x24 + DAT_005395dc),
       iVar5 = DAT_005395dc, iVar10 != 0)) {
      return;
    }
  } while( true );
}


// ==== FUN_00479d90 @ 00479d90

int __thiscall FUN_00479d90(void *this,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_00490040(s_level_>_0____level_<_HASH_LEVEL__0052d5f8,s__ns_o_SquidHash_cpp_0052d5e4,0x5c);
  }
  return ((DAT_0052e9a8 * param_3) / (int)(&DAT_0052d598)[param_4] + param_2) * 2 +
         *(int *)((int)this + param_4 * 4 + 0xc);
}


// ==== FUN_0040f180 @ 0040f180

undefined4 FUN_0040f180(void)

{
  return 1;
}


// ==== FUN_004eb5c0 @ 004eb5c0

int __cdecl FUN_004eb5c0(undefined4 param_1,undefined4 param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = (uint)((param_3 & 6) == 0);
  local_48 = param_1;
  local_44 = param_2;
  local_4 = 0;
  FUN_00440a50(&local_48);
  uVar1 = __ftol();
  uVar2 = __ftol();
  FUN_004eae20(local_40,uVar2,uVar1,uVar2,uVar1);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_c * 0x24 + DAT_005395dc)) {
        return 0;
      }
      uVar3 = FUN_004abae0();
      if ((param_3 & uVar3) != 0) {
        return (local_c * 0x24) / 0x24;
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  return 0;
}


// ==== FUN_00449670 @ 00449670

byte __cdecl FUN_00449670(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 != 0) {
    iVar3 = DAT_005395dc + param_1 * 0x24;
    uVar1 = FUN_004abae0(iVar3);
    if ((uVar1 & 4) != 0) {
      iVar2 = FUN_004abf50(iVar3);
      return *(byte *)(*(int *)(iVar2 + 0x124) + 3 + (uint)*(byte *)(iVar3 + 0x22) * 4) & 0x20;
    }
  }
  return 0;
}


// ==== FUN_00449170 @ 00449170

bool __cdecl FUN_00449170(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_004ac890(param_1 * 0x24 + DAT_005395dc,0x2692);
  return iVar1 != 0;
}


// ==== FUN_00449710 @ 00449710

byte __cdecl FUN_00449710(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 != 0) {
    iVar3 = DAT_005395dc + param_1 * 0x24;
    uVar1 = FUN_004abae0(iVar3);
    if ((uVar1 & 4) != 0) {
      iVar2 = FUN_004abf50(iVar3);
      return *(byte *)(*(int *)(iVar2 + 0x124) + 3 + (uint)*(byte *)(iVar3 + 0x22) * 4) & 0x40;
    }
  }
  return 0;
}


// ==== FUN_00448c30 @ 00448c30

undefined4 __cdecl FUN_00448c30(int param_1)

{
  int *piVar1;
  ushort *puVar2;
  ushort uVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  
  iVar4 = DAT_005395dc;
  puVar2 = (ushort *)(param_1 * 0x24 + 0xc + DAT_005395dc);
  piVar1 = (int *)(param_1 * 0x24 + DAT_005395dc);
  uVar3 = *puVar2;
  *puVar2 = ((short)DAT_0051f05c * 8 - 8U ^ uVar3) & 0x78 ^ uVar3;
  FUN_00448c10(((int)piVar1 - iVar4) / 0x24);
  iVar4 = FUN_004abf50(piVar1);
  iVar5 = (int)*(char *)(*(int *)(iVar4 + 0x124) + 2 + (uint)*(byte *)((int)piVar1 + 0x22) * 4);
  if (0x13 < iVar5) {
    iVar5 = 1;
  }
  if (iVar5 < 10) {
    uVar9 = 0x20;
    uVar6 = CONCAT31((int3)((uint)iVar5 >> 8),(char)iVar5 + '\n');
    uVar8 = 0x50;
    uVar7 = FUN_004ae3f0(piVar1,0x50,uVar6,0x20);
    iVar4 = FUN_00444410(iVar4,uVar7,uVar8,uVar6,uVar9);
    if (iVar4 != -1) {
      *(char *)((int)piVar1 + 0x22) = (char)iVar4;
      (**(code **)(*piVar1 + 0x88))();
      return 1;
    }
  }
  return 0;
}


// ==== FUN_00448c10 @ 00448c10

int __cdecl FUN_00448c10(int param_1)

{
  return DAT_005395dc + param_1 * 0x24;
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __thiscall FUN_004ae3f0(void *this)

{
  if (*(byte *)((int)this + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)((int)this + 0x22) * 4);
}


// ==== FUN_00444410 @ 00444410

int __thiscall FUN_00444410(void *this,char param_2,char param_3,char param_4,uint param_5)

{
  int iVar1;
  char *pcVar2;
  
  iVar1 = 0;
  pcVar2 = *(char **)((int)this + 0x124);
  if (0 < *(int *)((int)this + 0x114)) {
    do {
      if ((((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) &&
         ((param_5 & (int)pcVar2[3]) != 0)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)((int)this + 0x114));
  }
  return -1;
}


// ==== FUN_0048f960 @ 0048f960

void __cdecl FUN_0048f960(float param_1,float param_2,uint param_3,uint param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  undefined4 local_4c;
  uint local_48;
  uint local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (DAT_0052e92c == 0) {
    if ((DAT_00540a28 != 0) && ((param_3 & 0x50) == 0)) {
      FUN_0040a3d0(&local_54);
      local_50 = (float)__ftol();
      local_4c = __ftol();
      local_48 = param_3;
      local_44 = param_4;
      FUN_004c0420(&local_54);
    }
    iVar6 = DAT_0051cabc * 0x1d4 + DAT_0051c960;
    local_54 = (float)(1 - *(int *)(iVar6 + 0x1b4)) + param_1;
    local_5c = param_1;
    local_58 = param_2;
    local_50 = (float)(1 - *(int *)(iVar6 + 0x1b8)) + param_2;
    local_40[0] = &PTR_FUN_005003d0;
    iVar6 = 0;
    local_8 = 0;
    local_4 = 0;
    FUN_00440a50(&local_54);
    FUN_00440a80(&local_5c);
    uVar1 = __ftol();
    uVar2 = __ftol(uVar1);
    uVar3 = __ftol(uVar2);
    iVar8 = 0;
    uVar4 = __ftol(uVar3);
    FUN_004eae20(local_40,uVar4,uVar3,uVar2,uVar1);
    local_5c = 0.0;
    if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
      do {
        uVar5 = local_c * 0x24 + DAT_005395dc;
        if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar5) break;
        if (*(byte *)(uVar5 + 0x20) == param_4) {
          uVar5 = (uint)*(byte *)(uVar5 + 10);
          if ((uVar5 == DAT_0051cbb8) || (uVar5 == DAT_0051cac0)) {
            if ((param_3 & 0x1000) == 0) {
              FUN_00454810(local_c);
              iVar7 = local_c * 0x24 + DAT_005395dc;
              FUN_00438c00(*(undefined4 *)(iVar7 + 0xe),*(undefined4 *)(iVar7 + 0x12),
                           s_islandFall_wav_005313ac,0);
            }
            (**(code **)(*(int *)(DAT_005395dc + local_c * 0x24) + 0x10))(param_3 | 0x48);
            if (uVar5 == DAT_0051cbb8) {
              iVar8 = iVar8 + 1;
            }
            else {
              iVar6 = iVar6 + 1;
            }
          }
          if (uVar5 == DAT_0051cac4) {
            (**(code **)(*(int *)(DAT_005395dc + local_c * 0x24) + 0x10))(param_3 | 0x48);
          }
          if (uVar5 == DAT_0051cabc) {
            (**(code **)(*(int *)(DAT_005395dc + local_c * 0x24) + 0x10))(param_3);
            local_5c = (float)((int)local_5c + 1);
          }
          uVar5 = FUN_004abae0(DAT_005395dc + local_c * 0x24);
          if ((uVar5 & 0x10000) != 0) {
            (**(code **)(*(int *)(local_c * 0x24 + DAT_005395dc) + 200))();
          }
        }
        FUN_004eafe0(local_40);
      } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
    }
    if (((iVar8 != 0) || (local_5c != 0.0)) || (iVar6 != 0)) {
      if (iVar8 != 9) {
        FUN_00490040(s_noIslandTypeCount____9_005313d4,s__ns_o_IslandDropper_cpp_005313bc,0x7b);
      }
      if (local_5c != 1.4013e-45) {
        FUN_00490040(s_islandTypeCount____1_00531404,s__ns_o_IslandDropper_cpp_005313ec,0x7c);
      }
      if (iVar6 != 1) {
        FUN_00490040(s_islandStalagTypeCount____1_00531434,s__ns_o_IslandDropper_cpp_0053141c,0x7d);
      }
    }
  }
  return;
}


// ==== FUN_004adbe0 @ 004adbe0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004adbe0(void *this,uint param_2)

{
  uint uVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined **local_50 [13];
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  if ((((DAT_005207f8 != 0) && ((*(byte *)((int)this + 0x23) & 8) == 0)) && ((param_2 & 0x800) == 0)
      ) && (uVar1 = FUN_004abae0(), ((DAT_005395cc | 8) & uVar1) != 0)) {
    puVar2 = (undefined4 *)FUN_004aeec0(this,local_10,0);
    local_60 = *puVar2;
    local_5c = puVar2[1];
    local_58 = puVar2[2];
    local_54 = puVar2[3];
    local_50[0] = &PTR_FUN_005003d0;
    local_18 = 0;
    local_14 = 0;
    FUN_00440a50(&local_60);
    FUN_00440a80(&local_58);
    uVar3 = __ftol();
    uVar4 = __ftol(uVar3);
    uVar5 = __ftol(uVar4);
    uVar6 = __ftol(uVar5);
    FUN_004eae20(local_50,uVar6,uVar5,uVar4,uVar3);
    while (local_1c != 0) {
      uVar1 = FUN_004abac0(local_1c * 0x24 + DAT_005395dc);
      if ((uVar1 & 0x800) != 0) {
        FUN_0045b7d0(local_1c);
      }
      FUN_004eafe0(local_50);
    }
  }
  if ((*(byte *)((int)this + 0x23) & 9) == 0) {
    FUN_004147a0(*(undefined1 *)((int)this + 10));
    _DAT_00512114 = __ftol();
  }
  FUN_004aba90(param_2);
  return;
}


// ==== FUN_0048efd0 @ 0048efd0

void * __thiscall FUN_0048efd0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_0048f073;
  *unaff_FS_OFFSET = &local_10;
  *(undefined ***)this = &PTR_FUN_00500d68;
  puVar1 = (undefined4 *)((int)this + 0x10);
  local_8 = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *puVar1 = 0;
  uVar2 = DAT_0051ca18;
  *(undefined4 *)((int)this + 0x14) = 0;
  *puVar1 = 0;
  *(undefined ***)this = &PTR_FUN_00504670;
  *(undefined4 *)((int)this + 0x14) = 0;
  FUN_0048f590(this,uVar2,param_3,0,0x50);
  *(undefined4 *)((int)this + 0x1c) = param_4;
  uVar2 = DAT_005484ac;
  *puVar1 = DAT_005484a8;
  *(undefined4 *)((int)this + 0x14) = uVar2;
  *(undefined4 *)((int)this + 0x18) = param_2;
  *(undefined4 *)((int)this + 0x20) = 0;
  *unaff_FS_OFFSET = local_10;
  return this;
}


