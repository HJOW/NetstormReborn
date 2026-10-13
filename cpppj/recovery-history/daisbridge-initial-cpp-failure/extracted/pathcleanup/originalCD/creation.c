// ==== FUN_0047f0a0 @ 0047f0a0

void __thiscall FUN_0047f0a0(void *this)

{
  bool bVar1;
  int iVar2;
  uint uVar3;
  int *piVar4;
  undefined4 *unaff_FS_OFFSET;
  undefined **local_60 [13];
  int local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0047f2c1;
  *unaff_FS_OFFSET = &local_10;
  piVar4 = (int *)(DAT_005395dc + *(int *)((int)this + 8) * 0x24);
  if (((DAT_00540a2c != 0) || ((4 < *(int *)((int)this + 4) && (*(int *)((int)this + 4) < 6000))))
     && ((*(byte *)((int)piVar4 + 0xb) & 2) == 0)) {
    (**(code **)(*piVar4 + 0x48))(0);
    (**(code **)(*piVar4 + 0x4c))(0);
  }
  if ((((*(int *)((int)this + 0x2f14) != 0) &&
       (uVar3 = DAT_005395dc + *(int *)((int)this + 0x2f14) * 0x24, DAT_005395dc + 0x24U <= uVar3))
      && (uVar3 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) &&
     (uVar3 = FUN_004abae0(), (uVar3 & 0x404200) != 0)) {
    iVar2 = FUN_0048f870(this);
    local_20 = *(undefined4 *)(iVar2 + 0xe);
    local_1c = *(undefined4 *)(iVar2 + 0x12);
    local_28 = 0;
    local_24 = 0;
    local_60[0] = &PTR_FUN_005003d0;
    FUN_00440a80(&local_20);
    local_14 = __ftol();
    iVar2 = __ftol();
    FUN_004eae20(local_60,iVar2 + -2,local_14 + -2,iVar2 + 2,local_14 + 2);
    while (local_2c != 0) {
      if (*(int *)((int)this + 0x2f14) == local_2c) {
        local_14 = FUN_004f1650(0x28);
        local_8 = 0;
        if (local_14 != 0) {
          FUN_0048f1d0(local_14,DAT_00520fe0,*(undefined4 *)((int)this + 0x2f14),0x3f800000);
        }
        local_8 = 0xffffffff;
        *(undefined4 *)((int)this + 0x2f14) = 0;
        break;
      }
      FUN_004eafe0(local_60);
    }
  }
  if (*(byte *)((int)piVar4 + 10) == DAT_0051cbbc) {
    iVar2 = FUN_0048f870(this);
    local_18 = *(undefined4 *)(iVar2 + 0xe);
    iVar2 = FUN_004eb7b0(local_18,*(undefined4 *)(iVar2 + 0x12),2,DAT_0051cbd8);
    if ((iVar2 != 0) && (iVar2 = DAT_005395dc + iVar2 * 0x24, (*(byte *)(iVar2 + 0x23) & 8) == 0)) {
      FUN_00481fc0(iVar2,*(undefined4 *)((int)this + 8),0);
    }
  }
  if (((DAT_00540a2c != 0) && (*(int *)((int)this + 0x2f14) != 0)) &&
     (piVar4 = (int *)(DAT_005395dc + *(int *)((int)this + 0x2f14) * 0x24),
     (*(byte *)((int)piVar4 + 0xb) & 1) == 0)) {
    if (((*(byte *)((int)piVar4 + 0x23) & 1) == 0) ||
       (uVar3 = FUN_004abae0(piVar4), (uVar3 & 0x404200) == 0)) {
      bVar1 = false;
    }
    else {
      bVar1 = true;
    }
    if (bVar1) {
      FUN_004d1bc0(*(undefined1 *)((int)piVar4 + 10),(char)piVar4[8]);
      (**(code **)(*piVar4 + 0x10))(0);
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0048f870 @ 0048f870

int __thiscall FUN_0048f870(void *this)

{
  int iVar1;
  
  iVar1 = DAT_005395dc + *(int *)((int)this + 8) * 0x24;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_00490040(s_parent_>getHead______0_0053133c,s__ns_o_BaseProcess_cpp_00531324,0x9f);
  }
  return iVar1;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
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


// ==== FUN_004e3d70 @ 004e3d70

int __cdecl FUN_004e3d70(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_4 = 0;
  local_c = 0;
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = 1;
  if (param_3 == 0) {
    uVar1 = __ftol();
    uVar2 = __ftol();
    uVar4 = uVar2;
    uVar3 = uVar1;
  }
  else {
    uVar3 = __ftol();
    uVar4 = __ftol(uVar3);
    uVar1 = __ftol(uVar4);
    uVar2 = __ftol(uVar1);
  }
  FUN_004eae20(local_40,uVar2,uVar1,uVar4,uVar3);
  if (local_c != 0) {
    do {
      iVar5 = local_c * 0x24 + DAT_005395dc;
      if (((*(byte *)(iVar5 + 0x23) & 1) == 0) && ((*(byte *)(iVar5 + 0xb) & 2) == 0)) {
        uVar4 = FUN_004abf50();
        iVar5 = FUN_004e3d40(uVar4);
        if (iVar5 != 0) {
          return local_c;
        }
      }
      FUN_004eafe0(local_40);
    } while (local_c != 0);
    return 0;
  }
  return 0;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004e3d40 @ 004e3d40

undefined4 __cdecl FUN_004e3d40(int param_1)

{
  int iVar1;
  
  if ((*(byte *)(param_1 + 0xed) & 2) == 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 != DAT_0051cb3c) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_004448e0 @ 004448e0

int __thiscall FUN_004448e0(void *this)

{
  return ((int)this - DAT_0051c960) / 0x1d4;
}


// ==== FUN_004eb7b0 @ 004eb7b0

int __cdecl FUN_004eb7b0(undefined4 param_1,undefined4 param_2,int param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = (uint)((*(uint *)(param_4 * 0x1d4 + 0xec + DAT_0051c960) & 6) == 0);
  local_48 = param_1;
  local_44 = param_2;
  local_4 = 0;
  FUN_00440a80(&local_48);
  iVar1 = __ftol();
  iVar2 = __ftol();
  FUN_004eae20(local_40,iVar2 - param_3,iVar1 - param_3,iVar2 + param_3,iVar1 + param_3);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      uVar3 = local_c * 0x24 + DAT_005395dc;
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar3) {
        return 0;
      }
      if (*(byte *)(uVar3 + 10) == param_4) {
        return (local_c * 0x24) / 0x24;
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  return 0;
}


// ==== FUN_004ac560 @ 004ac560

int __thiscall FUN_004ac560(void *this,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_005004b0;
  local_4 = 0;
  FUN_004eb8d0(local_10,((int)this - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(local_8 * 0x24 + 0x12 + DAT_005395dc) != param_3)) {
      bVar1 = false;
    }
    if ((param_2 != 0) && ((param_2 & *(ushort *)(local_8 * 0x24 + 0x14 + DAT_005395dc)) == 0)) {
      bVar1 = false;
    }
    if (bVar1) {
      if (param_4 == 0) {
        return (local_8 * 0x24) / 0x24;
      }
      param_4 = param_4 + -1;
    }
    FUN_004eb900(local_10);
  } while( true );
}


// ==== FUN_004eb8d0 @ 004eb8d0

void __thiscall FUN_004eb8d0(void *this,int param_2)

{
  *(uint *)((int)this + 4) = (uint)*(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  FUN_004eb900();
  return;
}


// ==== FUN_004eb900 @ 004eb900

void __thiscall FUN_004eb900(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar2 = DAT_005395dc;
  do {
    do {
      iVar1 = *(int *)((int)this + 4);
      *(int *)((int)this + 8) = iVar1;
      if (iVar1 == 0) {
        *(undefined4 *)((int)this + 4) = 0;
        *(undefined4 *)((int)this + 8) = 0;
        return;
      }
      iVar1 = iVar1 * 0x24 + iVar2;
      *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 + 4);
    } while ((iVar1 != 0) && (*(byte *)(iVar1 + 10) != DAT_0051c978));
    iVar1 = (*(code *)**(undefined4 **)this)(iVar1);
    iVar2 = DAT_005395dc;
  } while (iVar1 == 0);
  return;
}


// ==== FUN_0040f230 @ 0040f230

undefined4 FUN_0040f230(void)

{
  return 1;
}


