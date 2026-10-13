// ==== FUN_00481fc0 @ 00481fc0

void __thiscall FUN_00481fc0(void *this,int param_2,int param_3)

{
  uint uVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  float *pfVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  undefined4 *unaff_FS_OFFSET;
  float local_18;
  float local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00482143;
  *unaff_FS_OFFSET = &local_10;
  iVar8 = DAT_005395dc + param_2 * 0x24;
  iVar4 = FUN_004812a0(this,0xffffffff);
  if (4 < iVar4) goto LAB_004821cb;
  fVar2 = *(float *)(iVar8 + 0x12);
  fVar3 = *(float *)(iVar8 + 0xe);
  pfVar5 = (float *)FUN_00481d10(this,&local_18,iVar4);
  local_18 = *pfVar5;
  local_14 = pfVar5[1];
  if ((fVar3 != local_18) || (fVar2 != local_14)) {
    if (3 < iVar4) goto LAB_004821cb;
    fVar2 = *(float *)(iVar8 + 0x12);
    fVar3 = *(float *)(iVar8 + 0xe);
    pfVar5 = (float *)FUN_00481d10(this,&local_18,iVar4 + 1);
    local_14 = pfVar5[1];
    if ((fVar3 != *pfVar5) || (fVar2 != local_14)) goto LAB_004821cb;
  }
  local_18 = *(float *)(iVar8 + 0xe);
  iVar6 = FUN_00481de0(this,local_18,*(undefined4 *)(iVar8 + 0x12));
  if (iVar6 == 0) {
    if (param_3 == 0) {
      FUN_0040cf10(param_2,iVar4);
      local_18 = *(float *)(iVar8 + 0xe);
      local_14 = *(float *)(iVar8 + 0x12);
      FUN_00438c00(local_18,local_14,(&PTR_s_forWind2_wav_0052e2f0)[iVar4],0);
      local_14 = (float)FUN_004f1650(0x28);
      local_8 = 0;
      if (local_14 != 0.0) {
        FUN_0048f1d0(local_14,0x307,((int)this - DAT_005395dc) / 0x24,
                     *(undefined4 *)(&DAT_0052e308 + iVar4 * 4));
      }
    }
    else if (iVar4 < 5) {
      iVar8 = FUN_004817e0(this);
      iVar4 = DAT_005395dc;
      uVar1 = iVar8 * 0x24 + DAT_005395dc;
      if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
        uVar7 = FUN_004812a0(this,0xffffffff);
        FUN_0040cf60((int)(uVar1 - iVar4) / 0x24,uVar7);
      }
      if ((DAT_00540a2c != 0) && (iVar4 = FUN_00481ee0(this), iVar4 == 0)) {
        FUN_00481e50(this);
      }
    }
  }
LAB_004821cb:
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004812a0 @ 004812a0

undefined4 __thiscall FUN_004812a0(void *this,uint param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  if (param_2 == 0xffffffff) {
    param_2 = (uint)*(byte *)((int)this + 0x22);
  }
  iVar1 = (**(code **)(*(int *)this + 0x80))();
  iVar1 = param_2 - *(int *)(s_MissingMission__s__0052e2a4 + iVar1 * 4 + 0x10);
  if (iVar1 < 0xb) {
    return 0;
  }
  if (iVar1 < 0x16) {
    return 1;
  }
  if (iVar1 < 0x21) {
    return 2;
  }
  if (iVar1 < 0x2c) {
    return 3;
  }
  uVar2 = 4;
  if (0x36 < iVar1) {
    uVar2 = 5;
  }
  return uVar2;
}


// ==== FUN_00481310 @ 00481310

undefined4 __thiscall FUN_00481310(void *this)

{
  byte bVar1;
  int iVar2;
  undefined4 uVar3;
  
  bVar1 = *(byte *)((int)this + 0x22);
  iVar2 = FUN_004ae4b0();
  if (iVar2 == 0xd) {
    return 1;
  }
  if (iVar2 == 0xc) {
    return 2;
  }
  if (iVar2 == 0xb) {
    return 3;
  }
  if (bVar1 < 0x40) {
    return 1;
  }
  uVar3 = 2;
  if (0x77 < bVar1) {
    uVar3 = 3;
  }
  return uVar3;
}


// ==== FUN_00481770 @ 00481770

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00481770(void *this,float *param_2)

{
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_004aeec0(this,&local_10,0);
  local_c = local_c - _DAT_00504034;
  local_8 = local_8 + _DAT_00504034;
  local_4 = local_4 + _DAT_00504034;
  *param_2 = local_10 - _DAT_00504034;
  param_2[1] = local_c;
  param_2[2] = local_8;
  param_2[3] = local_4;
  return;
}


// ==== FUN_004817e0 @ 004817e0

int __thiscall FUN_004817e0(void *this)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined **local_50 [13];
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  puVar1 = (undefined4 *)FUN_00481770(this,local_10);
  local_60 = *puVar1;
  local_5c = puVar1[1];
  local_58 = puVar1[2];
  local_54 = puVar1[3];
  local_18 = 0;
  local_50[0] = &PTR_FUN_005003d0;
  local_14 = 0;
  FUN_00440a50(&local_60);
  FUN_00440a80(&local_58);
  uVar2 = __ftol();
  uVar3 = __ftol(uVar2);
  uVar4 = __ftol(uVar3);
  uVar5 = __ftol(uVar4);
  FUN_004eae20(local_50,uVar5,uVar4,uVar3,uVar2);
  while( true ) {
    if (local_1c == 0) {
      return 0;
    }
    iVar6 = local_1c * 0x24 + DAT_005395dc;
    if (((*(byte *)(iVar6 + 10) == DAT_0051cbbc) &&
        (*(char *)(iVar6 + 0x20) == *(char *)((int)this + 0x20))) &&
       (iVar6 = FUN_0040c3b0(), iVar6 == 0)) break;
    FUN_004eafe0(local_50);
  }
  return local_1c;
}


// ==== FUN_00481c80 @ 00481c80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00481c80(void *this,undefined4 *param_2,int param_3)

{
  int iVar1;
  int iVar2;
  undefined4 local_8;
  undefined4 local_4;
  
  if (4 < param_3) {
    FUN_00490040(s_numBridgesMissing_<__4_0052e4f4,s_dais_cpp_0052e4e8,0x193);
  }
  local_4 = *(undefined4 *)((int)this + 0x12);
  local_8 = *(undefined4 *)((int)this + 0xe);
  iVar2 = *(int *)(&DAT_0052e2c8 + param_3 * 4) + _DAT_0052e2b8;
  iVar1 = FUN_004abf50(this);
  iVar1 = *(int *)(iVar1 + 0xdc);
  iVar2 = *(int *)(iVar1 + 8 + iVar2 * 8);
  FUN_0043fa10(&local_8,(int)*(short *)(iVar2 + iVar1 + -4),(int)*(short *)(iVar2 + -2 + iVar1));
  *param_2 = local_8;
  param_2[1] = local_4;
  return;
}


// ==== FUN_00481d10 @ 00481d10

undefined4 * __thiscall FUN_00481d10(void *this,undefined4 *param_2,int param_3)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_3 < 5) {
    FUN_00481c80(this,&local_8,param_3);
    iVar2 = *(int *)(DAT_0051cbdc * 0x1d4 + 0xdc + DAT_0051c960);
    iVar3 = *(int *)(iVar2 + 8 + param_3 * 8);
    FUN_0043fa10(&local_8,(int)*(short *)(iVar3 + iVar2 + -4),(int)*(short *)(iVar3 + -2 + iVar2));
    if (param_3 == 4) {
      iVar2 = __ftol();
      iVar3 = __ftol();
      bVar1 = *(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48);
      if (((bVar1 & 6) != 0) && ((bVar1 & 8) == 0)) {
        FUN_0043fa10(&local_8,DAT_0052e2e4,DAT_0052e2e8);
      }
    }
    *param_2 = local_8;
    param_2[1] = local_4;
    return param_2;
  }
  return param_2;
}


// ==== FUN_00481de0 @ 00481de0

void __thiscall FUN_00481de0(void *this,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  
  iVar2 = FUN_004812a0(this,0xffffffff);
  iVar1 = *(int *)(DAT_0051cbdc * 0x1d4 + 0xdc + DAT_0051c960);
  iVar1 = *(int *)(iVar1 + 8 + iVar2 * 8) + -0x24 + iVar1;
  FUN_0043fa10(&param_2,-(int)*(short *)(iVar1 + 0x20),-(int)*(short *)(iVar1 + 0x22));
  FUN_004eb4b0(param_2,param_3,DAT_0051cbdc);
  return;
}


// ==== FUN_00481e50 @ 00481e50

void __thiscall FUN_00481e50(void *this)

{
  undefined1 uVar1;
  int *piVar2;
  undefined4 uVar3;
  undefined4 unaff_ESI;
  undefined4 uStack_8;
  
  piVar2 = (int *)FUN_004ab390(DAT_0051cbdc,0);
  FUN_004aed20(piVar2,0);
  uVar1 = FUN_004812a0(this,0xffffffff);
  *(undefined1 *)((int)piVar2 + 0x22) = uVar1;
  if ((*(byte *)((int)piVar2 + 0xb) & 4) == 0) {
    FUN_00490040(s_isVoid___00514a24,s____o_squid_h_00514a30,0x268);
  }
  uVar1 = *(undefined1 *)((int)this + 0x20);
  *(byte *)((int)piVar2 + 0x23) = *(byte *)((int)piVar2 + 0x23) | 1;
  (**(code **)(*piVar2 + 0x74))(uVar1);
  uVar3 = FUN_004812a0(this,0xffffffff);
  FUN_00481c80(this,&stack0xfffffff4,uVar3);
  (**(code **)(*piVar2 + 0x90))(unaff_ESI,uStack_8,0);
  return;
}


// ==== FUN_00481ee0 @ 00481ee0

undefined4 __thiscall FUN_00481ee0(void *this)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined **local_50 [13];
  int local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  puVar1 = (undefined4 *)FUN_004aeec0(this,local_10,0);
  local_60 = *puVar1;
  local_5c = puVar1[1];
  local_58 = puVar1[2];
  local_54 = puVar1[3];
  local_18 = 0;
  local_50[0] = &PTR_FUN_005003d0;
  local_14 = 0;
  FUN_00440a50(&local_60);
  FUN_00440a80(&local_58);
  uVar2 = __ftol();
  uVar3 = __ftol(uVar2);
  uVar4 = __ftol(uVar3);
  uVar5 = __ftol(uVar4);
  FUN_004eae20(local_50,uVar5,uVar4,uVar3,uVar2);
  while( true ) {
    if (local_1c == 0) {
      return 0;
    }
    iVar6 = DAT_005395dc + local_1c * 0x24;
    if ((*(byte *)(iVar6 + 10) == DAT_0051cbdc) && ((*(byte *)(iVar6 + 0x23) & 1) != 0)) break;
    FUN_004eafe0(local_50);
  }
  return 1;
}


// ==== FUN_00482c40 @ 00482c40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00482c40(void *this,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  float10 fVar5;
  undefined4 uVar6;
  
  if (param_2 == 0x305) {
    iVar1 = *(byte *)((int)this + 0x22) + 1;
    iVar2 = FUN_004812a0(this,(uint)*(byte *)((int)this + 0x22));
    iVar3 = FUN_004812a0(this,iVar1);
    if (iVar2 == iVar3) {
      FUN_004acbc0(this,iVar1,0);
      return (float10)_DAT_0052e330;
    }
    FUN_004acbc0(this,iVar1,0);
    return (float10)_DAT_00504038;
  }
  if (param_2 == 0x306) {
    fVar5 = (float10)FUN_00482df0(this);
    return fVar5;
  }
  if ((param_2 == 0x307) && (param_3 != 0)) {
    uVar6 = 1;
    uVar4 = FUN_004817e0(this,1);
    FUN_00481fc0(this,uVar4,uVar6);
    return (float10)_DAT_00504038;
  }
  fVar5 = (float10)FUN_00461a40(this,param_2,param_3,param_4);
  return fVar5;
}


// ==== FUN_00484520 @ 00484520

void __thiscall FUN_00484520(void *this,uint param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined **local_58 [13];
  int local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00484620;
  *unaff_FS_OFFSET = &local_10;
  if ((param_2 & 1) != 0) {
    local_14 = FUN_004f1650(0x28);
    local_8 = 0;
    if (local_14 != 0) {
      FUN_0048efd0(local_14,0x304,((int)this - DAT_005395dc) / 0x24,0x3c23d70a);
    }
  }
  local_8 = 0xffffffff;
  if ((param_2 & 2) != 0) {
    local_14 = *(int *)((int)this + 0x12);
    local_58[0] = &PTR_FUN_005003d0;
    local_18 = *(undefined4 *)((int)this + 0xe);
    local_20 = 0;
    local_1c = 0;
    FUN_00440a80(&local_18);
    iVar2 = __ftol();
    iVar3 = __ftol();
    FUN_004eae20(local_58,iVar3 + -1,iVar2 + -1,iVar3 + 1,iVar2 + 1);
    iVar2 = DAT_005395dc;
    while (DAT_005395dc = iVar2, local_24 != 0) {
      if ((*(byte *)(local_24 * 0x24 + 10 + iVar2) == DAT_0051cbd8) &&
         (*(char *)((int)this + 0x20) == *(char *)(local_24 * 0x24 + iVar2 + 0x20)))
      goto LAB_0048463e;
      FUN_004eafe0(local_58);
      iVar2 = DAT_005395dc;
    }
    local_24 = 0;
LAB_0048463e:
    uVar1 = local_24 * 0x24 + iVar2;
    if ((iVar2 + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + iVar2))) {
      local_14 = FUN_00482990();
      FUN_004d06a0(((int)this - iVar2) / 0x24,(float)local_14);
    }
  }
  FUN_004ae180(this,param_2);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00482990 @ 00482990

int __thiscall FUN_00482990(void *this)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = FUN_004816a0();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if (((uVar1 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)) ||
     (*(ushort *)(uVar1 + 0xc) == 0)) {
    return 10;
  }
  if (DAT_0052e170 != 0) {
    if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
      return 10;
    }
    if (DAT_0052e170 != 0) {
      return 10;
    }
  }
  return (-(uint)((*(uint *)((uint)*(ushort *)(uVar1 + 0xc) * 0xac + DAT_0050f834 + 0x50) & 1) == 0)
         & 0xffffffee) + 0x19;
}


// ==== FUN_004aeec0 @ 004aeec0

undefined4 __thiscall FUN_004aeec0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  
  uVar3 = *(undefined4 *)((int)this + 0xe);
  uVar4 = *(undefined4 *)((int)this + 0x12);
  uVar2 = param_2;
  uVar1 = FUN_004abf50(this,param_2,uVar3,uVar4,param_3);
  FUN_004449d0(uVar1,uVar2,uVar3,uVar4,param_3);
  return param_2;
}


// ==== FUN_004449d0 @ 004449d0

float * __thiscall FUN_004449d0(void *this,float *param_2,float param_3,float param_4,uint param_5)

{
  float *pfVar1;
  float fVar2;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  int local_8;
  int local_4;
  
  FUN_004442b0(this,&local_8,&local_4,param_5 & 1);
  local_1c = (float)(1 - local_8) + param_3;
  pfVar1 = param_2 + 2;
  *param_2 = param_3;
  *pfVar1 = param_3;
  param_2[1] = param_4;
  param_2[3] = param_4;
  local_18 = (float)(1 - local_4) + param_4;
  local_14 = local_1c;
  local_10 = local_18;
  FUN_0043f950(&local_14);
  if (((((int)*param_2 < 1) || (local_c = (float)DAT_0052e9a8, local_c <= *param_2)) ||
      ((int)param_2[1] < 1)) || (local_c <= param_2[1])) {
    *param_2 = local_14;
    param_2[1] = local_10;
  }
  else {
    fVar2 = *param_2;
    if (local_14 <= *param_2) {
      fVar2 = local_14;
    }
    *param_2 = fVar2;
    fVar2 = param_2[1];
    if (local_10 <= param_2[1]) {
      fVar2 = local_10;
    }
    param_2[1] = fVar2;
  }
  FUN_0043f950(&local_1c);
  if (((0 < (int)*pfVar1) && (*pfVar1 < (float)DAT_0052e9a8)) &&
     ((0 < (int)param_2[3] && (param_2[3] < (float)DAT_0052e9a8)))) {
    fVar2 = *pfVar1;
    if (*pfVar1 <= local_1c) {
      fVar2 = local_1c;
    }
    *pfVar1 = fVar2;
    fVar2 = param_2[3];
    if (param_2[3] <= local_18) {
      fVar2 = local_18;
    }
    param_2[3] = fVar2;
    return param_2;
  }
  *pfVar1 = local_1c;
  param_2[3] = local_18;
  return param_2;
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


// ==== FUN_004ae4b0 @ 004ae4b0

int __thiscall FUN_004ae4b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)((int)this + 0x22) * 4) + -0x41;
}


// ==== FUN_0040c3b0 @ 0040c3b0

bool __thiscall FUN_0040c3b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004aed30();
  return (int)*(short *)((int)this + 0x1a) < iVar1 / 2;
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


// ==== FUN_004448e0 @ 004448e0

int __thiscall FUN_004448e0(void *this)

{
  return ((int)this - DAT_0051c960) / 0x1d4;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
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


// ==== FUN_004eb4b0 @ 004eb4b0

int __cdecl FUN_004eb4b0(undefined4 param_1,undefined4 param_2,uint param_3)

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
  local_8 = (uint)((*(uint *)(param_3 * 0x1d4 + 0xec + DAT_0051c960) & 6) == 0);
  local_48 = param_1;
  local_44 = param_2;
  local_4 = 0;
  FUN_00440a50(&local_48);
  uVar1 = __ftol();
  uVar2 = __ftol();
  FUN_004eae20(local_40,uVar2,uVar1,uVar2,uVar1);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      uVar3 = local_c * 0x24 + DAT_005395dc;
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar3) {
        return 0;
      }
      if (*(byte *)(uVar3 + 10) == param_3) {
        return (local_c * 0x24) / 0x24;
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  return 0;
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


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0x1a) = param_2;
  return;
}


// ==== FUN_0040cf10 @ 0040cf10

void __cdecl FUN_0040cf10(int param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    uVar3 = 0;
    iVar2 = FUN_004abf50(uVar1,0);
    FUN_004acbc0(uVar1,*(undefined4 *)(iVar2 + 0x158 + param_2 * 4),uVar3);
  }
  return;
}


// ==== FUN_0040cf60 @ 0040cf60

void __cdecl FUN_0040cf60(int param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0040d00a;
  *unaff_FS_OFFSET = &local_10;
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    FUN_0040cf10(param_1,param_2);
    iVar2 = FUN_004f1650(0x28);
    local_8 = 0;
    if (iVar2 != 0) {
      FUN_0048f1d0(iVar2,0x25c,(int)(uVar1 - DAT_005395dc) / 0x24,0x3c23d70a);
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


