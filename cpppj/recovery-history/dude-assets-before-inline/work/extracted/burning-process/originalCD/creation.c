// ==== FUN_004a5a50 @ 004a5a50

undefined4 * __thiscall FUN_004a5a50(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 *extraout_ECX;
  undefined1 **ppuVar3;
  undefined1 **ppuVar4;
  undefined4 *puVar5;
  undefined4 *unaff_FS_OFFSET;
  bool bVar6;
  undefined1 local_5064 [20488];
  undefined4 uStack_5c;
  undefined4 auStack_58 [5];
  undefined1 *local_44 [10];
  undefined4 local_10;
  undefined1 *puStack_c;
  int local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004a5bcd;
  *unaff_FS_OFFSET = &local_10;
  FUN_004f1bd0();
  local_44[9] = (undefined1 *)DAT_0051c998;
  local_44[8] = (undefined1 *)param_2;
  local_44[7] = (undefined1 *)0x4a5a8b;
  FUN_0045cc40();
  local_8 = 0;
  *extraout_ECX = &PTR_FUN_00505870;
  extraout_ECX[0x1b] = param_2;
  extraout_ECX[0x15] = 0;
  extraout_ECX[0x17] = 0;
  extraout_ECX[0x18] = 0;
  extraout_ECX[0x19] = 0;
  extraout_ECX[0x1a] = 0;
  extraout_ECX[0x16] = 0;
  local_44[9] = (undefined1 *)0x96;
  extraout_ECX[0x1f] = 0;
  extraout_ECX[0x1e] = 0;
  extraout_ECX[0x20] = 0;
  extraout_ECX[0x14] = 0;
  local_44[8] = (undefined1 *)0x4a5ac5;
  FUN_0045cad0(extraout_ECX + 9);
  bVar6 = DAT_00538d34 == 0;
  extraout_ECX[0x1c] = param_3;
  if (bVar6) {
    local_44[9] = (undefined1 *)0x8;
    local_44[8] = (undefined1 *)0x4a5ade;
    iVar1 = FUN_004f1650();
    local_8._0_1_ = 1;
    iVar2 = 0;
    if (iVar1 != 0) {
      local_44[9] = (undefined1 *)0x4a5afa;
      iVar2 = FUN_0045cfa0(iVar1);
    }
    local_8 = (uint)local_8._1_3_ << 8;
    local_44[9] = (undefined1 *)0x4a5b08;
    DAT_00538d34 = iVar2;
    FUN_0048ef00();
    local_44[0] = (undefined1 *)0x0;
    local_44[2] = (undefined1 *)0xbebebebf;
    local_44[3] = (undefined1 *)0x3feebebe;
    local_44[4] = (undefined1 *)0x5d5d5d5d;
    local_44[5] = (undefined1 *)0x3fdd5d5d;
    local_44[6] = (undefined1 *)0x0;
    local_44[1] = (undefined1 *)0x3ff00000;
    local_44[7] = (undefined1 *)0x3ff00000;
    local_44[8] = (undefined1 *)0xdcdcdcdd;
    local_44[9] = (undefined1 *)0x3fdcdcdc;
    ppuVar3 = local_44;
    ppuVar4 = local_44 + 1;
    for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
      *ppuVar4 = *ppuVar3;
      ppuVar3 = ppuVar3 + 1;
      ppuVar4 = ppuVar4 + 1;
    }
    ppuVar3 = local_44 + 6;
    puVar5 = auStack_58;
    for (iVar1 = 6; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar5 = *ppuVar3;
      ppuVar3 = ppuVar3 + 1;
      puVar5 = puVar5 + 1;
    }
    uStack_5c = 0x4a5b8e;
    FUN_00459460(local_5064);
    local_44[9] = (undefined1 *)0x0;
    local_44[7] = local_5064;
    local_44[8] = (undefined1 *)0x0;
    local_44[6] = (undefined1 *)0x4a5ba4;
    FUN_0045d020(DAT_00538d34);
  }
  *unaff_FS_OFFSET = local_10;
  return extraout_ECX;
}


// ==== FUN_0045cc40 @ 0045cc40

void * __thiscall FUN_0045cc40(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0045ccbd;
  *unaff_FS_OFFSET = &local_10;
  FUN_0045c6c0(this,param_2,param_3);
  local_8 = 0;
  FUN_0045caa0((int)this + 0x24);
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x3c) = 0;
  *(undefined ***)this = &PTR_FUN_00500590;
  *(undefined4 *)((int)this + 0x48) = 0;
  *unaff_FS_OFFSET = local_10;
  return this;
}


// ==== FUN_0045c6c0 @ 0045c6c0

void * __thiscall FUN_0045c6c0(void *this,int param_2,undefined4 param_3)

{
  bool bVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  int *piVar10;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_0045c825;
  *unaff_FS_OFFSET = &local_10;
  *(undefined ***)this = &PTR_FUN_00500d68;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  local_8 = 0;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x14) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  uVar8 = DAT_005209c4;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined ***)this = &PTR_FUN_00502468;
  bVar1 = false;
  piVar6 = DAT_005209c0;
  do {
    iVar4 = DAT_005395dc;
    iVar9 = 0;
    piVar5 = piVar6;
    if (0 < (int)uVar8) {
      do {
        if (*piVar5 == 0) {
          bVar1 = true;
          break;
        }
        iVar9 = iVar9 + 1;
        piVar5 = piVar5 + 1;
      } while (iVar9 < (int)uVar8);
    }
    DAT_005209c0 = piVar6;
    if (bVar1) {
      piVar6[iVar9] = (int)this;
      *(int *)((int)this + 0x1c) = iVar9;
      *(int *)((int)this + 0x20) = (int)*(char *)(*(int *)((int)this + 8) * 0x24 + 0x21 + iVar4);
      if (param_2 != 0) {
        FUN_0048f590(this,param_3,param_2,0,0x10);
        *(int *)((int)this + 0x20) = (int)*(char *)(param_2 * 0x24 + 0x21 + DAT_005395dc);
      }
      *unaff_FS_OFFSET = local_10;
      return this;
    }
    DAT_005209c4 = uVar8 + 0x28;
    if ((int)DAT_005209c4 < 0x29) {
      DAT_005209c4 = 0x28;
    }
    piVar6 = (int *)FUN_004f1650(DAT_005209c4 << 2);
    FUN_0048ef00();
    uVar3 = DAT_005209c4;
    piVar5 = piVar6;
    for (uVar7 = DAT_005209c4 & 0x3fffffff; piVar2 = DAT_005209c0, uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar5 = 0;
      piVar5 = piVar5 + 1;
    }
    piVar5 = DAT_005209c0;
    piVar10 = piVar6;
    for (uVar8 = uVar8 & 0x3fffffff; uVar8 != 0; uVar8 = uVar8 - 1) {
      *piVar10 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar10 = piVar10 + 1;
    }
    uVar8 = uVar3;
    if (piVar2 != (int *)0x0) {
      FUN_004f1a50(piVar2);
      uVar8 = DAT_005209c4;
    }
  } while( true );
}


// ==== FUN_0045caa0 @ 0045caa0

void * __thiscall FUN_0045caa0(void *this)

{
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  return this;
}


// ==== FUN_0045cad0 @ 0045cad0

void __thiscall FUN_0045cad0(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0045cb64;
  *unaff_FS_OFFSET = &local_10;
  if (*(int *)((int)this + 8) != 0) {
    FUN_004f1a50(*(int *)((int)this + 8));
  }
  if (param_2 < 1) {
    param_2 = 0;
  }
  *(int *)this = param_2;
  iVar2 = FUN_004f1650(param_2 * 0x18);
  local_8 = 0;
  iVar1 = iVar2;
  if (iVar2 == 0) {
    iVar2 = 0;
  }
  else {
    while (param_2 = param_2 + -1, -1 < param_2) {
      FUN_0045d410(iVar1);
      iVar1 = iVar1 + 0x18;
    }
  }
  local_8 = 0xffffffff;
  *(int *)((int)this + 8) = iVar2;
  FUN_0045cb80(this);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0045cb80 @ 0045cb80

void __thiscall FUN_0045cb80(void *this)

{
  uint uVar1;
  undefined4 *puVar2;
  
  if (this == (void *)0x0) {
    FUN_00490040(s_this____0_00520a34,s_Particle_cpp_00520a24,0xc9);
  }
  if (*(int *)((int)this + 8) == 0) {
    FUN_00490040(s_particles_00520a50,s_Particle_cpp_00520a40,0xca);
  }
  puVar2 = *(undefined4 **)((int)this + 8);
  for (uVar1 = (uint)(*(int *)this * 0x18) >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined4 *)((int)this + 4) = 0;
  return;
}


// ==== FUN_0045cfa0 @ 0045cfa0

void * __thiscall FUN_0045cfa0(void *this)

{
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 4) = 0;
  return this;
}


// ==== FUN_0045d020 @ 0045d020

void __thiscall FUN_0045d020(void *this,int param_2,int param_3,int param_4)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  
  if (*(int *)((int)this + 4) != 0) {
    FUN_004f1a50(*(int *)((int)this + 4));
  }
  iVar4 = 0;
  if (param_3 != 0) {
    iVar4 = *(int *)(param_3 + 0x5010);
  }
  iVar2 = 0;
  if (param_4 != 0) {
    iVar2 = *(int *)(param_4 + 0x5010);
  }
  iVar2 = iVar2 + *(int *)(param_2 + 0x5010) + iVar4;
  *(int *)this = iVar2;
  puVar3 = (undefined4 *)FUN_004f1650(iVar2 * 4);
  iVar4 = 0;
  *(undefined4 **)((int)this + 4) = puVar3;
  if (param_2 != 0) {
    iVar2 = *(int *)(param_2 + 0x5010);
    if (0 < iVar2) {
      puVar6 = (undefined4 *)(param_2 + 0x38);
      iVar5 = iVar2;
      do {
        uVar1 = *puVar6;
        puVar6 = puVar6 + 0x14;
        iVar5 = iVar5 + -1;
        *puVar3 = uVar1;
        puVar3 = puVar3 + 1;
        iVar4 = iVar2;
      } while (iVar5 != 0);
    }
    if (param_2 == 0) goto LAB_0045d141;
    if (param_3 != 0) {
      iVar2 = *(int *)(param_3 + 0x5010) + *(int *)(param_2 + 0x5010);
      if (iVar4 < iVar2) {
        puVar3 = (undefined4 *)(param_3 + 0x38);
        iVar5 = iVar4 * 4;
        iVar2 = iVar2 - iVar4;
        iVar4 = iVar4 + iVar2;
        puVar6 = (undefined4 *)(iVar5 + *(int *)((int)this + 4));
        do {
          uVar1 = *puVar3;
          puVar3 = puVar3 + 0x14;
          iVar2 = iVar2 + -1;
          *puVar6 = uVar1;
          puVar6 = puVar6 + 1;
        } while (iVar2 != 0);
      }
    }
  }
  if ((((param_2 != 0) && (param_3 != 0)) && (param_4 != 0)) &&
     (iVar2 = *(int *)(param_3 + 0x5010) + *(int *)(param_4 + 0x5010) + *(int *)(param_2 + 0x5010),
     iVar4 < iVar2)) {
    puVar6 = (undefined4 *)(param_4 + 0x38);
    iVar5 = iVar4 * 4;
    iVar2 = iVar2 - iVar4;
    iVar4 = iVar4 + iVar2;
    puVar3 = (undefined4 *)(iVar5 + *(int *)((int)this + 4));
    do {
      uVar1 = *puVar6;
      puVar6 = puVar6 + 0x14;
      iVar2 = iVar2 + -1;
      *puVar3 = uVar1;
      puVar3 = puVar3 + 1;
    } while (iVar2 != 0);
  }
LAB_0045d141:
  if (*(int *)this != iVar4) {
    FUN_00490040(s_totalColors____i_00520a6c,s_Particle_cpp_00520a5c,0x176);
  }
  return;
}


// ==== FUN_00459460 @ 00459460

void * __thiscall
FUN_00459460(void *this,double param_2,double param_3,double param_4,double param_5,double param_6,
            double param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10)

{
  double dVar1;
  double dVar2;
  double dVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined4 local_50 [6];
  double local_38;
  double local_30;
  double local_28;
  
  FUN_00458e60();
  *(undefined ***)this = &PTR_LAB_005022e0;
  *(undefined4 *)((int)this + 0x5010) = param_8;
  *(undefined4 *)((int)this + 0x501c) = param_10;
  *(undefined4 *)((int)this + 0x5018) = param_9;
  *(undefined4 *)((int)this + 0x500c) = 0xffffffff;
  iVar5 = 0;
  puVar6 = (undefined4 *)((int)this + 8);
  dVar2 = 100000.0;
  dVar3 = 100000.0;
  *(undefined4 *)((int)this + 0x5008) = 0xffffffff;
  do {
    puVar7 = puVar6;
    puVar8 = local_50;
    for (iVar4 = 0x14; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar8 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar8 = puVar8 + 1;
    }
    dVar1 = (local_38 - param_2) * (local_38 - param_2) +
            (local_28 - param_4) * (local_28 - param_4) +
            (local_30 - param_3) * (local_30 - param_3);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x5008) = iVar5;
      dVar2 = dVar1;
    }
    dVar1 = (local_38 - param_5) * (local_38 - param_5) +
            (local_28 - param_7) * (local_28 - param_7) +
            (local_30 - param_6) * (local_30 - param_6);
    if (dVar1 < dVar3) {
      *(int *)((int)this + 0x500c) = iVar5;
      dVar3 = dVar1;
    }
    puVar6 = puVar6 + 0x14;
    iVar5 = iVar5 + 1;
  } while (iVar5 < 0x100);
  if ((((*(int *)((int)this + 0x5008) < 0) || (0xff < *(int *)((int)this + 0x5008))) ||
      (*(int *)((int)this + 0x500c) < 0)) || (0xff < *(int *)((int)this + 0x500c))) {
    FUN_00490040(s_0_<__p_&&_p_<_256_&&_0_<__q_&&_q_00520648,s_ColorSort_cpp_00520638,0x11d);
  }
  FUN_00459630(this);
  return this;
}


// ==== FUN_0048ef00 @ 0048ef00

void __cdecl FUN_0048ef00(void)

{
  return;
}


// ==== FUN_004a66a0 @ 004a66a0

undefined4 __cdecl FUN_004a66a0(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_004a6701;
  *unaff_FS_OFFSET = &local_10;
  iVar1 = FUN_004f1650(0x90);
  local_8 = 0;
  uVar2 = 0;
  if (iVar1 != 0) {
    uVar2 = FUN_004a5a50(iVar1,param_1,0);
  }
  *unaff_FS_OFFSET = local_10;
  return uVar2;
}


// ==== FUN_004a6720 @ 004a6720

undefined4 __cdecl FUN_004a6720(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_004a6781;
  *unaff_FS_OFFSET = &local_10;
  iVar1 = FUN_004f1650(0x90);
  local_8 = 0;
  uVar2 = 0;
  if (iVar1 != 0) {
    uVar2 = FUN_004a5a50(iVar1,param_1,1);
  }
  *unaff_FS_OFFSET = local_10;
  return uVar2;
}


// ==== FUN_004f1bd0 @ 004f1bd0

/* WARNING: Unable to track spacebase fully for stack */

void __cdecl FUN_004f1bd0(void)

{
  uint in_EAX;
  undefined1 *puVar1;
  undefined4 unaff_retaddr;
  
  puVar1 = &stack0x00000004;
  for (; 0xfff < in_EAX; in_EAX = in_EAX - 0x1000) {
    puVar1 = puVar1 + -0x1000;
  }
  *(undefined4 *)(puVar1 + (-4 - in_EAX)) = unaff_retaddr;
  return;
}


// ==== FUN_004a60e0 @ 004a60e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a60e0(void *this)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  undefined4 uVar7;
  int iVar8;
  float *pfVar9;
  float *pfVar10;
  int iVar11;
  float10 fVar12;
  undefined4 uVar13;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_8;
  float local_4;
  
  if (*(int *)((int)this + 0x24) != 0) {
    if (*(int *)((int)this + 0x54) == 0) {
      FUN_0045e280((int)this + 0x30);
      FUN_004a5c70(this);
      *(int *)((int)this + 0x54) = *(int *)((int)this + 0x54) + 1;
      return;
    }
    iVar8 = FUN_004ab670(*(undefined4 *)((int)this + 0x6c),1);
    if (iVar8 != 0) {
      iVar11 = DAT_005395dc + *(int *)((int)this + 0x6c) * 0x24;
      iVar8 = FUN_004aecf0(iVar11);
      if (iVar8 != 0) {
        fVar5 = *(float *)(iVar11 + 0xe) * DAT_00565c3c;
        fVar6 = *(float *)(iVar11 + 0x12) * DAT_00565c40;
        *(int *)((int)this + 0x20) = *(char *)(iVar11 + 0x21) + -1;
        if (*(int *)((int)this + 0x54) == 1) {
          *(undefined4 *)((int)this + 0x80) = 1;
          *(float *)((int)this + 0xc) = fVar5;
          *(undefined4 *)((int)this + 0x54) = 2;
          *(float *)((int)this + 0x10) = fVar6;
          *(float *)((int)this + 0x14) = fVar5;
          *(float *)((int)this + 0x18) = fVar6;
          return;
        }
        iVar8 = __ftol();
        local_8 = (float)iVar8;
        iVar8 = __ftol();
        local_4 = (float)iVar8;
        iVar8 = __ftol();
        iVar11 = __ftol();
        iVar8 = FUN_0045c630(&local_8,(float)iVar8,(float)iVar11,(DAT_005209bc == 0) - 1U & 100);
        uVar7 = DAT_005484ac;
        uVar13 = DAT_005484a8;
        if (iVar8 == 0) {
          if (*(int *)((int)this + 0x80) == 0) {
            FUN_004386f0(*(undefined4 *)((int)this + 0x50));
          }
          *(undefined4 *)((int)this + 0x80) = 1;
          return;
        }
        if (*(int *)((int)this + 0x80) != 0) {
          *(undefined4 *)((int)this + 0x80) = 0;
          *(undefined4 *)((int)this + 0x30) = uVar13;
          *(undefined4 *)((int)this + 0x34) = uVar7;
        }
        iVar8 = 0;
        FUN_0045e2b0((int)this + 0x30);
        FUN_004a5d30(this);
        pfVar10 = *(float **)((int)this + 0x2c);
        local_2c = fVar5;
        local_28 = fVar6;
        local_24 = fVar5;
        local_20 = fVar6;
        while (iVar8 < *(int *)((int)this + 0x24)) {
          fVar4 = local_20;
          if ((int)pfVar10[4] < 1) {
LAB_004a6470:
            local_20 = fVar4;
            iVar8 = iVar8 + 1;
            pfVar10 = pfVar10 + 6;
          }
          else {
            if ((int)pfVar10[5] - (int)this != 0x74) {
              pfVar10[4] = pfVar10[4] - *(float *)((int)this + 0x40);
              pfVar10[2] = pfVar10[2] + _DAT_0051fbf8;
              *pfVar10 = pfVar10[2] * *(float *)((int)this + 0x40) + *pfVar10;
              pfVar10[1] = pfVar10[3] * *(float *)((int)this + 0x40) + pfVar10[1];
              fVar4 = *pfVar10;
              if (local_2c <= *pfVar10) {
                fVar4 = local_2c;
              }
              fVar2 = pfVar10[1];
              if (local_28 <= pfVar10[1]) {
                fVar2 = local_28;
              }
              fVar3 = *pfVar10 + _DAT_00505860;
              if (*pfVar10 + _DAT_00505860 <= local_24) {
                fVar3 = local_24;
              }
              local_24 = fVar3;
              local_2c = fVar4;
              local_28 = fVar2;
              fVar4 = pfVar10[1] + _DAT_00505860;
              if (pfVar10[1] + _DAT_00505860 <= local_20) {
                fVar4 = local_20;
              }
              goto LAB_004a6470;
            }
            fVar12 = (float10)FUN_0048cc50(0,0x3fb33333);
            if ((float10)*(float *)((int)this + 0x40) <= fVar12) goto LAB_004a6470;
            fVar2 = *pfVar10;
            iVar11 = *(int *)((int)this + 100);
            uVar13 = 0;
            fVar3 = pfVar10[1];
            iVar1 = *(int *)((int)this + 0x68);
            fVar12 = (float10)FUN_0048cc50(0x40800000,0x41100000,0);
            pfVar9 = (float *)FUN_004a60a0(this,(fVar2 - (float)iVar11) + fVar5,
                                           (fVar3 - (float)iVar1) + fVar6,0,0xc0000000,(float)fVar12
                                           ,uVar13);
            if (pfVar9 == (float *)0x0) goto LAB_004a6470;
            fVar4 = *pfVar9;
            if (local_2c <= *pfVar9) {
              fVar4 = local_2c;
            }
            fVar2 = pfVar9[1];
            if (local_28 <= pfVar9[1]) {
              fVar2 = local_28;
            }
            fVar3 = *pfVar9 + _DAT_00505860;
            if (*pfVar9 + _DAT_00505860 <= local_24) {
              fVar3 = local_24;
            }
            local_24 = fVar3;
            fVar3 = pfVar9[1] + _DAT_00505860;
            if (pfVar9[1] + _DAT_00505860 <= local_20) {
              fVar3 = local_20;
            }
            local_20 = fVar3;
            iVar8 = iVar8 + 1;
            pfVar10 = pfVar10 + 6;
            local_2c = fVar4;
            local_28 = fVar2;
          }
        }
        fVar5 = (local_24 + _DAT_00505864) - _DAT_00505868;
        if (local_2c <= fVar5) {
          fVar5 = local_2c;
        }
        FUN_0045ce80((int)this + 0xc,fVar5,local_28 - _DAT_00505864,local_24 + _DAT_00505864,
                     local_20 + _DAT_00505860);
      }
    }
  }
  return;
}


// ==== FUN_004a5d30 @ 004a5d30

void __thiscall FUN_004a5d30(void *this)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int local_1c;
  int local_8;
  
  iVar2 = FUN_004ab670(*(undefined4 *)((int)this + 0x6c),1);
  if (iVar2 != 0) {
    iVar13 = DAT_005395dc + *(int *)((int)this + 0x6c) * 0x24;
    iVar2 = FUN_004aecf0(iVar13);
    if (iVar2 != 0) {
      sVar1 = *(short *)(iVar13 + 0x1a);
      uVar3 = FUN_004abf50(iVar13);
      iVar2 = FUN_00444720(uVar3);
      iVar2 = (sVar1 * 5) / (((int)(iVar2 * 2 + (iVar2 * 2 >> 0x1f & 3U)) >> 2) - iVar2) + 6;
      iVar4 = (*(int *)((int)this + 0x60) + *(int *)((int)this + 0x5c)) / 5;
      if (iVar4 <= iVar2) {
        iVar2 = iVar4;
      }
      if (iVar2 < 2) {
        iVar2 = 1;
      }
      uVar5 = FUN_004abae0(iVar13);
      if ((uVar5 & 0x400) != 0) {
        iVar2 = 1;
      }
      iVar13 = *(int *)((int)this + 0x7c);
      if (iVar2 != iVar13) {
        iVar4 = *(int *)((int)this + 0x60);
        if (iVar4 < 3) {
          *(int *)((int)this + 0x7c) = iVar2;
          return;
        }
        if (iVar13 < iVar2) {
          local_1c = 300;
          iVar6 = *(int *)((int)this + 0x5c) + -7;
          if (iVar6 < 9) {
            iVar6 = 8;
          }
          iVar7 = *(int *)((int)this + 0x5c) + -1;
          if (iVar7 <= iVar6) {
            iVar6 = iVar7;
          }
          iVar8 = (iVar4 + -3 + iVar6) / 5;
          iVar7 = local_1c;
          if (iVar2 <= iVar8) {
            iVar8 = iVar2;
          }
          while (local_1c = iVar7, iVar13 < iVar8) {
            iVar2 = 0;
            local_1c = iVar7 + -1;
            if (iVar7 == 0) break;
            iVar13 = FUN_0048cc10(0,iVar6);
            iVar7 = FUN_0048cc10(2,iVar4 + -1);
            iVar9 = (iVar7 + iVar13) / 5;
            if ((*(char *)((iVar7 + 1) * *(int *)((int)this + 0x5c) + *(int *)((int)this + 0x58) +
                          iVar13) != -1) && (*(char *)(*(int *)((int)this + 0x78) + iVar9) == '\0'))
            {
              iVar10 = FUN_0048cc10(6,10);
              local_8 = 0;
              if (0 < iVar10) {
                do {
                  iVar11 = iVar2 / 2;
                  if (((iVar11 + iVar13 < *(int *)((int)this + 0x5c) + -1) &&
                      (iVar12 = iVar7 - iVar11, -1 < iVar12)) &&
                     (*(char *)(iVar12 * *(int *)((int)this + 0x5c) + *(int *)((int)this + 0x58) +
                                iVar13 + iVar11) != -1)) {
                    FUN_004a60a0(this,(float)(iVar11 + iVar13),(float)iVar12,0,0,0x461c3c00,
                                 (int)this + 0x74);
                    local_8 = local_8 + 1;
                    *(undefined1 *)(*(int *)((int)this + 0x78) + iVar9) = 1;
                  }
                  iVar2 = iVar2 + 1;
                } while (iVar2 < iVar10);
              }
              if (3 < local_8) {
                *(int *)((int)this + 0x7c) = *(int *)((int)this + 0x7c) + 1;
              }
            }
            iVar7 = local_1c;
            iVar13 = *(int *)((int)this + 0x7c);
          }
          if (local_1c < 1) {
            iVar2 = 7;
            iVar13 = 0;
            do {
              if (*(int *)((int)this + 0x60) + -1 <= iVar13) {
                return;
              }
              for (iVar4 = 0; (iVar2 != 0 && (iVar4 < *(int *)((int)this + 0x5c)));
                  iVar4 = iVar4 + 1) {
                if (*(char *)(*(int *)((int)this + 0x58) + *(int *)((int)this + 0x5c) * iVar13 +
                             iVar4) != -1) {
                  iVar2 = iVar2 + -1;
                  FUN_004a60a0(this,(float)iVar4,(float)iVar13,0,0,0x461c3c00,(int)this + 0x74);
                }
              }
              iVar13 = iVar13 + 1;
            } while (iVar2 != 0);
          }
        }
        else {
          iVar2 = *(int *)((int)this + 0x2c);
          iVar4 = 0;
          iVar13 = *(int *)((int)this + 0x28);
          if (0 < iVar13) {
            do {
              if (*(int *)(iVar2 + 0x14) - (int)this == 0x74) break;
              iVar2 = iVar2 + 0x18;
              iVar4 = iVar4 + 1;
            } while (iVar4 < iVar13);
          }
          if ((iVar4 < iVar13) && (*(int *)(iVar2 + 0x14) - (int)this == 0x74)) {
            iVar2 = __ftol();
            iVar13 = __ftol();
            iVar2 = (iVar13 + iVar2) / 5;
            if (*(char *)(*(int *)((int)this + 0x78) + iVar2) != '\x01') {
              FUN_00490040(s_rowTaken_killRow_____1_00538d44,s_Burning_cpp_00538d38,0xf7);
            }
            iVar13 = *(int *)((int)this + 0x2c);
            iVar4 = *(int *)((int)this + 0x28);
            *(undefined1 *)(*(int *)((int)this + 0x78) + iVar2) = 0;
            if (0 < iVar4) {
              do {
                if (*(int *)(iVar13 + 0x14) - (int)this == 0x74) {
                  iVar6 = __ftol();
                  iVar7 = __ftol();
                  if ((iVar7 + iVar6) / 5 == iVar2) {
                    *(undefined4 *)(iVar13 + 0x14) = 0;
                    *(undefined4 *)(iVar13 + 0x10) = 0;
                  }
                }
                iVar13 = iVar13 + 0x18;
                iVar4 = iVar4 + -1;
              } while (iVar4 != 0);
              return;
            }
          }
        }
      }
    }
  }
  return;
}


// ==== FUN_004a5c70 @ 004a5c70

void __thiscall FUN_004a5c70(void *this)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  
  if (*(int *)((int)this + 0x58) != 0) {
    FUN_004f1a50(*(int *)((int)this + 0x58));
  }
  if (*(int *)((int)this + 0x78) != 0) {
    FUN_004f1a50(*(int *)((int)this + 0x78));
  }
  FUN_0045cb80((int)this + 0x24);
  iVar3 = FUN_004ab670(*(undefined4 *)((int)this + 0x6c),1);
  if ((iVar3 != 0) && (*(int *)(*(int *)((int)this + 0x6c) * 0x24 + DAT_005395dc) != 0)) {
    iVar3 = FUN_004aecf0();
    if (iVar3 != 0) {
      piVar1 = (int *)((int)this + 0x60);
      piVar2 = (int *)((int)this + 0x5c);
      uVar4 = FUN_0045b100(*(undefined4 *)((int)this + 0x6c),piVar2,piVar1,(int)this + 100,
                           (int)this + 0x68);
      *(undefined4 *)((int)this + 0x58) = uVar4;
      uVar4 = FUN_004f1650((*piVar2 + *piVar1) / 5);
      *(undefined4 *)((int)this + 0x78) = uVar4;
      uVar5 = (*piVar2 + *piVar1) / 5;
      puVar7 = *(undefined4 **)((int)this + 0x78);
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar7 = 0;
        puVar7 = puVar7 + 1;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)puVar7 = 0;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      *(undefined4 *)((int)this + 0x7c) = 0;
    }
  }
  return;
}


// ==== FUN_004a60a0 @ 004a60a0

void __thiscall
FUN_004a60a0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_0045cc00((int)this + 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = param_2;
    puVar1[1] = param_3;
    puVar1[2] = param_4;
    puVar1[3] = param_5;
    puVar1[4] = param_6;
    puVar1[5] = param_7;
  }
  return;
}


// ==== FUN_004a64f0 @ 004a64f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a64f0(void *this)

{
  int iVar1;
  undefined1 uVar2;
  int iVar3;
  int iVar4;
  undefined1 *puVar5;
  int iVar6;
  int local_c;
  
  if ((*(int *)((int)this + 0x24) != 0) &&
     (*(int *)((int)this + 0x28) <= *(int *)((int)this + 0x24))) {
    iVar6 = *(int *)((int)this + 0x2c);
    local_c = 0;
    if (0 < *(int *)((int)this + 0x28)) {
      do {
        if ((0 < *(int *)(iVar6 + 0x10)) && (*(int *)(iVar6 + 0x14) - (int)this != 0x74)) {
          __ftol();
          __ftol();
          uVar2 = FUN_0045d340(DAT_00538d34,*(undefined4 *)(iVar6 + 0x10),0x41100000);
          iVar3 = __ftol();
          iVar4 = __ftol();
          iVar1 = DAT_00516b0c;
          if ((((DAT_00552880 <= iVar3) &&
               (((iVar3 < DAT_00552888 && (DAT_00552884 <= iVar4)) && (iVar4 < DAT_0055288c)))) &&
              (((DAT_00552880 <= iVar3 + 1 && (iVar3 + 1 < DAT_00552888)) &&
               (DAT_00552884 <= iVar4 + -2)))) && (iVar4 + -2 < DAT_0055288c)) {
            puVar5 = (undefined1 *)(iVar4 * DAT_00516b0c + DAT_00516b18 + iVar3);
            *puVar5 = uVar2;
            puVar5[1] = uVar2;
            puVar5 = puVar5 + (1 - iVar1);
            *puVar5 = uVar2;
            puVar5[-1] = uVar2;
            puVar5[-1 - iVar1] = uVar2;
          }
        }
        iVar6 = iVar6 + 0x18;
        local_c = local_c + 1;
      } while (local_c < *(int *)((int)this + 0x28));
    }
  }
  return;
}


// ==== FUN_0045cc00 @ 0045cc00

int __thiscall FUN_0045cc00(void *this)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  
  iVar2 = 0;
  iVar1 = *(int *)((int)this + 4);
  if (iVar1 < *(int *)this) {
    *(int *)((int)this + 4) = iVar1 + 1;
    return *(int *)((int)this + 8) + iVar1 * 0x18;
  }
  iVar4 = 0;
  if (0 < iVar1) {
    piVar3 = (int *)(*(int *)((int)this + 8) + 0x10);
    while (0 < *piVar3) {
      piVar3 = piVar3 + 6;
      iVar4 = iVar4 + 1;
      if (iVar1 <= iVar4) {
        return iVar2;
      }
    }
    iVar2 = *(int *)((int)this + 8) + iVar4 * 0x18;
  }
  return iVar2;
}


