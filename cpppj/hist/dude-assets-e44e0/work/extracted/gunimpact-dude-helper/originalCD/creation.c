// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_0043f750 @ 0043f750

float10 __thiscall FUN_0043f750(void *this,float param_2,float param_3)

{
  float10 fVar1;
  
  fVar1 = (float10)*(float *)((int)this + 4) - (float10)param_3;
  return SQRT(((float10)*(float *)this - (float10)param_2) *
              ((float10)*(float *)this - (float10)param_2) + fVar1 * fVar1);
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


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
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


