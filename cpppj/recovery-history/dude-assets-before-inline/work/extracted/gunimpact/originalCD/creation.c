// ==== FUN_00455b40 @ 00455b40

undefined4 __cdecl FUN_00455b40(undefined4 param_1,undefined4 param_2)

{
  FUN_0043f950(&param_1);
  FUN_004e8e70(param_1,param_2);
  FUN_0048cbd0(100);
  FUN_0047c8a0(param_1,param_2,3,0x3ca3d70a);
  return 0;
}


// ==== FUN_0047c8a0 @ 0047c8a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0047c8a0(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0047c981;
  *unaff_FS_OFFSET = &local_10;
  if ((((DAT_00564dbc & 0x7fffffff) == 0 && _DAT_00564db8 == 0) && (DAT_005178d4 == 0)) &&
     (iVar1 = FUN_004ed650(), iVar1 < 0xbb9)) {
    FUN_0043f950(&param_1);
    piVar2 = (int *)FUN_004ab390(DAT_0051cbc4,2);
    iVar3 = FUN_004abf50(piVar2);
    iVar1 = param_3;
    *(undefined1 *)((int)piVar2 + 0x22) = *(undefined1 *)(iVar3 + 0x130 + param_3 * 4);
    (**(code **)(*piVar2 + 0x90))(param_1,param_2,0);
    iVar3 = FUN_004f1650(0x28);
    local_8 = 0;
    if (iVar3 != 0) {
      FUN_0048efd0(iVar3,iVar1,((int)piVar2 - DAT_005395dc) / 0x24,param_4);
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0047c810 @ 0047c810

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0047c810(void *this,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  float10 fVar3;
  
  iVar1 = FUN_004abf50();
  iVar1 = *(int *)(iVar1 + 0x170 + param_2 * 4);
  iVar2 = FUN_004abf50(this);
  if ((-1 < param_3) && (param_3 < iVar1)) {
    FUN_004acbc0(this,*(int *)(iVar2 + 0x130 + param_2 * 4) + param_3,0);
    fVar3 = (float10)FUN_0048f430(param_2,param_3,param_4);
    return fVar3;
  }
  (**(code **)(*(int *)this + 0x10))(0);
  return (float10)_DAT_00503a44;
}


// ==== FUN_0047c880 @ 0047c880

undefined4 * __cdecl FUN_0047c880(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00503b30;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004e8e70 @ 004e8e70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e8e70(float param_1,float param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  float *pfVar4;
  float fVar5;
  int iVar6;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_80 [8];
  undefined **local_78 [13];
  int local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  float local_28;
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
  pcStack_c = FUN_004e91e4;
  local_18 = param_1 - _DAT_005072b8;
  *unaff_FS_OFFSET = &local_10;
  local_14 = param_2 - _DAT_005072b8;
  local_78[0] = &PTR_FUN_005003d0;
  local_40 = 0;
  local_3c = 0;
  FUN_00440a80(&local_18);
  iVar1 = __ftol();
  iVar2 = __ftol();
  FUN_004eae20(local_78,iVar2 + -4,iVar1 + -4,iVar2 + 4,iVar1 + 4);
  if (DAT_005395dc + 0x24U <= (uint)(local_44 * 0x24 + DAT_005395dc)) {
    do {
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_44 * 0x24 + DAT_005395dc))
      break;
      uVar3 = FUN_004abae0();
      if (((((uVar3 & 0x400) != 0) &&
           (iVar1 = local_44 * 0x24 + DAT_005395dc, (*(byte *)(iVar1 + 0xb) & 2) == 0)) &&
          (*(short *)(iVar1 + 0xc) != 0x5107)) && (*(short *)(iVar1 + 0xc) != 0x5105)) {
        iVar2 = FUN_004ac9c0(iVar1,0xfff0,0x5100);
        if (iVar2 == 0) {
          local_14 = (float)FUN_004f1650(0x28);
          local_8 = 0;
          iVar2 = 0;
          if (local_14 != 0.0) {
            iVar2 = FUN_0048efd0(local_14,0x5107,(iVar1 - DAT_005395dc) / 0x24,0x40a00000);
          }
          local_8 = 0xffffffff;
        }
        else {
          *(undefined4 *)(iVar2 + 0x20) = 0;
          *(undefined4 *)(iVar2 + 0x18) = 0x5107;
        }
        local_34 = DAT_005484ac;
        *(undefined4 *)(iVar2 + 0x10) = DAT_005484a8;
        *(undefined4 *)(iVar2 + 0x14) = local_34;
        FUN_004abb00(iVar1,0x5107);
        iVar2 = FUN_004eb310((iVar1 - DAT_005395dc) / 0x24,DAT_0051c998,0,0);
        if (iVar2 == 0) {
          FUN_004a66a0((iVar1 - DAT_005395dc) / 0x24);
        }
        local_2c = *(undefined4 *)(iVar1 + 0x12);
        local_30 = *(undefined4 *)(iVar1 + 0xe);
        iVar2 = FUN_00406ef0(local_30,local_2c,&DAT_00565ae0,*(undefined1 *)(iVar1 + 0x20));
        if ((iVar2 != 0) && ((*(byte *)(DAT_005395dc + iVar2 * 0x24 + 0xb) & 3) == 0)) {
          pfVar4 = (float *)FUN_004e8a30(local_80,iVar2);
          local_20 = *pfVar4;
          local_1c = pfVar4[1];
          iVar2 = FUN_004eb310((iVar1 - DAT_005395dc) / 0x24,DAT_0051c9c0,0,0);
          if (iVar2 != 0) {
            iVar2 = FUN_004af4e0(DAT_005395dc + iVar2 * 0x24);
            local_28 = local_20;
            local_24 = local_1c;
            FUN_00440a80(&local_28);
            if ((*(float *)(iVar2 + 0xc + *(int *)(iVar2 + 0x10) * 8) == local_28) &&
               (*(float *)(iVar2 + *(int *)(iVar2 + 0x10) * 8 + 0x10) == local_24))
            goto LAB_004e91aa;
          }
          iVar2 = FUN_004abf50(iVar1);
          fVar5 = (float)__ftol();
          *(float *)(iVar2 + 0xe0) = *(float *)(iVar2 + 0xe0) * _DAT_005072d4;
          iVar6 = FUN_0047d150((iVar1 - DAT_005395dc) / 0x24,local_20,local_1c,0,0,0,0);
          *(float *)(iVar2 + 0xe0) = (float)(int)fVar5;
          local_14 = fVar5;
          if ((iVar6 != 0) && (iVar2 = FUN_004ae4b0(iVar1), iVar2 == 8)) {
            iVar2 = FUN_004abf50(iVar1);
            FUN_004acbc0(iVar1,*(undefined4 *)(iVar2 + 0x130),0);
          }
        }
      }
LAB_004e91aa:
      FUN_004eafe0(local_78);
    } while (DAT_005395dc + 0x24U <= (uint)(local_44 * 0x24 + DAT_005395dc));
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0048cbd0 @ 0048cbd0

uint __cdecl FUN_0048cbd0(uint param_1)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return (DAT_005308ec >> 0x10 & 0xffffU) % param_1;
}


// ==== FUN_004ed650 @ 004ed650

int __cdecl FUN_004ed650(void)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = 0;
  piVar2 = &DAT_005b7bb0;
  do {
    if (*piVar2 != 0) {
      iVar1 = iVar1 + 1;
    }
    piVar2 = piVar2 + 1;
  } while (piVar2 < &DAT_005bba30);
  return iVar1;
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


