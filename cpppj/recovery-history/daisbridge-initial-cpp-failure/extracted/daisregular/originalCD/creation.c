// ==== FUN_004846b0 @ 004846b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_004846b0(void *this,int param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  float10 fVar4;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_2 != 0x304) {
    fVar4 = (float10)FUN_0048f430(param_2,param_3,param_4);
    return fVar4;
  }
  local_44 = *(undefined4 *)((int)this + 0x12);
  local_40[0] = &PTR_FUN_005003d0;
  local_48 = *(undefined4 *)((int)this + 0xe);
  local_8 = 0;
  local_4 = 0;
  FUN_00440a80(&local_48);
  iVar2 = __ftol();
  iVar3 = __ftol();
  FUN_004eae20(local_40,iVar3 + -1,iVar2 + -1,iVar3 + 1,iVar2 + 1);
  while (local_c != 0) {
    if ((*(byte *)(local_c * 0x24 + 10 + DAT_005395dc) == DAT_0051cbd8) &&
       (*(char *)((int)this + 0x20) == *(char *)(local_c * 0x24 + DAT_005395dc + 0x20)))
    goto LAB_0048476a;
    FUN_004eafe0(local_40);
  }
  local_c = 0;
LAB_0048476a:
  uVar1 = local_c * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    FUN_00482a20(uVar1,((int)this - DAT_005395dc) / 0x24);
  }
  (**(code **)(*(int *)this + 0x10))(0x48);
  return (float10)_DAT_0050405c;
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


// ==== FUN_00482a20 @ 00482a20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00482a20(void *this)

{
  uint uVar1;
  undefined *puVar2;
  int iVar3;
  undefined4 uVar4;
  double *pdVar5;
  int iVar6;
  undefined4 *unaff_FS_OFFSET;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined1 local_2c [8];
  undefined1 local_24 [4];
  float local_20;
  int local_1c;
  undefined8 local_18;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00482c23;
  *unaff_FS_OFFSET = &local_10;
  iVar3 = FUN_004812a0(this,0xffffffff);
  FUN_00482950(this);
  if (iVar3 < 4) {
    if (DAT_00540a2c != 0) {
      FUN_00481ab0(this,0,0);
    }
    FUN_00482370(this);
  }
  else {
    iVar6 = 0;
    local_18 = (double)CONCAT44(local_18._4_4_,*(undefined4 *)((int)this + 0xe));
    FUN_00438c00(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                 s_itIsDone2_wav_0052e5c0,0);
    local_1c = FUN_004824f0(this);
    puVar2 = (&PTR_s_priestSacrifice4_wav_0052e324)[-(uint)(local_1c == 0)];
    local_20 = *(float *)(&DAT_0052e32c + (uint)(local_1c == 0) * -4);
    iVar3 = FUN_004f1650(0x28);
    local_18 = (double)CONCAT44(iVar3,(undefined4)local_18);
    local_8 = 0;
    if (iVar3 != 0) {
      uVar8 = 0xc;
      uVar7 = 0;
      uVar4 = FUN_00437d20(puVar2,0,0xc);
      iVar6 = FUN_00453800(local_18._4_4_,((int)this - DAT_005395dc) / 0x24,uVar4,uVar7,uVar8);
    }
    local_8 = 0xffffffff;
    if (iVar6 != 0) {
      pdVar5 = (double *)FUN_004011b0(local_2c);
      local_18 = *pdVar5 + (double)local_20;
      *(double *)(iVar6 + 0x20) = local_18;
    }
    iVar3 = FUN_004f1650(0x28);
    local_18 = (double)CONCAT44(iVar3,(undefined4)local_18);
    iVar6 = 0;
    local_8 = 1;
    if (iVar3 != 0) {
      uVar8 = 0xc;
      uVar7 = 0;
      uVar4 = FUN_00437d20(s_thunderCrack_wav_0052e5d0,0,0xc);
      iVar6 = FUN_00453800(iVar3,((int)this - DAT_005395dc) / 0x24,uVar4,uVar7,uVar8);
    }
    local_8 = 0xffffffff;
    if (iVar6 != 0) {
      pdVar5 = (double *)FUN_004011b0(local_24);
      local_18 = *pdVar5 + _DAT_00504048;
      *(double *)(iVar6 + 0x20) = local_18;
    }
    if (local_1c == 0) {
      iVar3 = FUN_00482520();
      if ((iVar3 != 0) || (DAT_0052e170 != 0)) {
        iVar3 = FUN_004816a0(this);
        uVar1 = iVar3 * 0x24 + DAT_005395dc;
        if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc)))
        {
          FUN_00483cd0();
        }
      }
    }
    else {
      FUN_00482cf0(this);
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00482df0 @ 00482df0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00482df0(void *this)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  
  uVar3 = (uint)*(byte *)((int)this + 0x22);
  iVar2 = DAT_0051cbe8 * 0x1d4 + DAT_0051c960;
  iVar1 = (int)*(char *)(*(int *)(iVar2 + 0x124) + uVar3 * 4);
  iVar4 = *(int *)(iVar2 + 0x2c + iVar1 * 4);
  iVar1 = *(int *)(iVar2 + 0x6c + iVar1 * 4);
  if (iVar4 <= (int)uVar3) {
    if ((int)uVar3 < iVar4 + iVar1) {
      iVar4 = iVar4 + (int)((uVar3 - iVar4) + 1) % iVar1;
    }
    if (iVar4 < (int)uVar3) {
      *(byte *)((int)this + 0x23) = *(byte *)((int)this + 0x23) & 0xfd;
      FUN_00482620();
      return (float10)_DAT_00504038;
    }
  }
  FUN_004acbc0(this,iVar4,0);
  return (float10)_DAT_00504050;
}


// ==== FUN_00482370 @ 00482370

void __thiscall FUN_00482370(void *this)

{
  undefined4 uVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0048240d;
  *unaff_FS_OFFSET = &local_10;
  local_14 = FUN_004f1650(0x28);
  local_8 = 0;
  if (local_14 != 0) {
    FUN_0048f1d0(local_14,0x305,((int)this - DAT_005395dc) / 0x24,0x3c23d70a);
  }
  local_8 = 0xffffffff;
  uVar1 = FUN_004812a0(this,0xffffffff);
  FUN_00481d10(this,&local_1c,uVar1);
  FUN_00438c00(local_1c,local_18,s_altarBurnCollapse_wav_0052e50c,0);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00482cf0 @ 00482cf0

void __thiscall FUN_00482cf0(void *this)

{
  uint uVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 uVar3;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_00482dcd;
  *unaff_FS_OFFSET = &local_10;
  *(byte *)((int)this + 0x23) = *(byte *)((int)this + 0x23) | 2;
  iVar2 = FUN_004816a0();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    uVar3 = 0;
    iVar2 = FUN_004abf50(uVar1,0);
    FUN_004acbc0(uVar1,*(undefined4 *)(iVar2 + 0x160),uVar3);
  }
  iVar2 = (**(code **)(*(int *)this + 0x80))();
  if (iVar2 == 1) {
    uVar3 = 0;
  }
  else {
    if (iVar2 != 2) goto LAB_00482d7b;
    uVar3 = 0x2d;
  }
  FUN_004acbc0(this,uVar3,0);
LAB_00482d7b:
  iVar2 = FUN_004f1650(0x28);
  local_8 = 0;
  if (iVar2 != 0) {
    FUN_0048f1d0(iVar2,0x306,((int)this - DAT_005395dc) / 0x24,0x3c23d70a);
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0040c840 @ 0040c840

void __thiscall FUN_0040c840(void *this)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0040c800();
  if (iVar1 != 0) {
    iVar1 = FUN_004ac890(this,0x25c);
    if (iVar1 != 0) {
      FUN_0048f7b0(iVar1,0);
    }
    uVar2 = 0;
    iVar1 = FUN_004abf50(this,0);
    FUN_004acbc0(this,*(undefined4 *)(iVar1 + 0x140),uVar2);
  }
  return;
}


// ==== FUN_0040c800 @ 0040c800

undefined4 __thiscall FUN_0040c800(void)

{
  int iVar1;
  
  iVar1 = FUN_004ae4b0();
  if ((9 < iVar1) && (iVar1 < 0xf)) {
    return 1;
  }
  return 0;
}


// ==== FUN_00481ab0 @ 00481ab0

void __thiscall FUN_00481ab0(void *this,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  int local_10;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if ((param_2 == 0) && (param_2 = FUN_004817e0(), param_2 == 0)) {
    return;
  }
  iVar2 = FUN_00481910(this,param_2);
  if ((iVar2 != 0) && (iVar2 = FUN_004812a0(this,0xffffffff), iVar2 < 5)) {
    if (param_2 == 0) {
      param_2 = FUN_004817e0(this);
    }
    uVar1 = param_2 * 0x24 + DAT_005395dc;
    if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
      if (param_3 == 0) {
        iVar2 = FUN_004812a0(this,0xffffffff);
        iVar2 = iVar2 + 1;
      }
      else {
        iVar2 = FUN_004812a0(this,0xffffffff);
      }
      iVar3 = FUN_004812a0(this,0xffffffff);
      if (iVar3 < 5) {
        FUN_00481d10(this,&local_10,iVar2);
        local_4 = *(undefined4 *)(uVar1 + 0x12);
        local_8 = *(undefined4 *)(uVar1 + 0xe);
        iVar3 = __ftol();
        iVar4 = __ftol();
        if (iVar3 == iVar4) {
          iVar3 = __ftol();
          iVar4 = __ftol();
          if (iVar3 == iVar4) {
            FUN_00481fc0(this,(int)(uVar1 - DAT_005395dc) / 0x24,0);
            return;
          }
        }
        if (DAT_00540a2c != 0) {
          iVar3 = FUN_004e7660(uVar1);
          if ((iVar3 + 1 < 4) &&
             (piVar5 = (int *)FUN_0047d150((int)(uVar1 - DAT_005395dc) / 0x24,local_10,local_c,1,0,
                                           ((int)this - DAT_005395dc) / 0x24,iVar2),
             piVar5 != (int *)0x0)) {
            piVar5[piVar5[4] * 2 + 3] = local_10;
            iVar2 = *piVar5;
            piVar5[piVar5[4] * 2 + 4] = local_c;
            (**(code **)(iVar2 + 0x30))(0,0);
            piVar5[0xbc6] = iVar3 + 1;
          }
        }
      }
    }
  }
  return;
}


// ==== FUN_00481910 @ 00481910

undefined4 __thiscall FUN_00481910(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  iVar1 = FUN_004812a0(this,0xffffffff);
  if ((iVar1 == 4) && (iVar1 = FUN_00481ee0(this), iVar1 != 0)) {
    return 0;
  }
  iVar1 = FUN_004816a0(this);
  uVar2 = iVar1 * 0x24 + DAT_005395dc;
  if ((((DAT_005395dc + 0x24U <= uVar2) && (uVar2 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) &&
      (iVar1 = FUN_004ae4b0(uVar2), iVar1 != 0xc)) && (iVar1 = FUN_004ae4b0(uVar2), iVar1 != 0xb)) {
    local_4 = 0;
    local_10[0] = &PTR_FUN_005004b0;
    FUN_004eb8d0(local_10,((int)this - DAT_005395dc) / 0x24);
    iVar1 = DAT_005395dc + param_2 * 0x24;
    uVar2 = FUN_004abae0(iVar1);
    if ((((uVar2 & 0x200000) != 0) && (*(char *)(iVar1 + 0x20) == *(char *)((int)this + 0x20))) &&
       (DAT_005395dc + 0x24U <= (uint)(local_8 * 0x24 + DAT_005395dc))) {
      while (uVar2 = local_8 * 0x24 + DAT_005395dc,
            uVar2 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc)) {
        if (((*(ushort *)(uVar2 + 0x14) & 0x20) != 0) && (*(ushort *)(uVar2 + 0x12) == DAT_0051cbbc)
           ) {
          return 1;
        }
        FUN_004eb900(local_10);
        if ((uint)(local_8 * 0x24 + DAT_005395dc) < DAT_005395dc + 0x24U) {
          return 0;
        }
      }
    }
    return 0;
  }
  return 0;
}


// ==== FUN_00482430 @ 00482430

int __cdecl FUN_00482430(undefined4 param_1,uint param_2,uint param_3)

{
  bool bVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_005004b0;
  local_4 = 1;
  FUN_004eb8d0(local_10,param_1);
  while( true ) {
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
    if (bVar1) break;
    FUN_004eb900(local_10);
  }
  return (local_8 * 0x24) / 0x24;
}


// ==== FUN_00482520 @ 00482520

undefined2 __fastcall FUN_00482520(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00482430((param_1 - DAT_005395dc) / 0x24,4,0);
  if (iVar1 == 0) {
    return 0;
  }
  return *(undefined2 *)(iVar1 * 0x24 + 0x12 + DAT_005395dc);
}


// ==== FUN_00482620 @ 00482620

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00482620(void *this)

{
  int *piVar1;
  byte bVar2;
  ushort uVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  char *pcVar8;
  undefined4 uVar9;
  undefined4 local_100 [64];
  
  FUN_004a6aa0(DAT_00512bf8,0,0);
  local_100[0] = *(undefined4 *)((int)this + 0xe);
  iVar5 = FUN_004eb4b0(local_100[0],*(undefined4 *)((int)this + 0x12),DAT_0051cbd4);
  if (iVar5 == 0) {
    FUN_00490040(s_altarSid_0052e548,s_dais_cpp_0052e53c,0x2a9);
  }
  uVar9 = 0;
  iVar6 = DAT_005395dc + iVar5 * 0x24;
  iVar5 = FUN_004abf50(iVar6,0);
  FUN_004acbc0(iVar6,*(undefined4 *)(iVar5 + 0x16c),uVar9);
  uVar3 = *(ushort *)(iVar6 + 0xc);
  if (DAT_0052e170 == 0) {
    iVar5 = FUN_004824f0(this);
    if (iVar5 == 0) goto LAB_004827d1;
    uVar9 = 0;
    iVar5 = (**(code **)(*(int *)this + 0x80))(0);
    FUN_004acbc0(this,*(undefined4 *)(&DAT_0052e2b8 + iVar5 * 4),uVar9);
    bVar2 = *(byte *)((int)this + 0x20);
    uVar9 = (**(code **)(*(int *)this + 0x80))();
    *(undefined4 *)((uint)bVar2 * 0xac + 0x98 + DAT_0050f834) = uVar9;
    iVar5 = FUN_00482430(((int)this - DAT_005395dc) / 0x24,0,*(undefined1 *)((int)this + 10));
    piVar1 = (int *)(iVar5 * 0x24 + DAT_005395dc);
    if (((int *)(DAT_005395dc + 0x24U) <= piVar1) &&
       (piVar1 < (int *)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
      (**(code **)(*piVar1 + 0x10))(0x400048);
    }
    iVar5 = FUN_00482430(((int)this - DAT_005395dc) / 0x24,0,DAT_0051cbbc);
    piVar1 = (int *)(iVar5 * 0x24 + DAT_005395dc);
    if (((int *)(DAT_005395dc + 0x24U) <= piVar1) &&
       (piVar1 < (int *)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
      (**(code **)(*piVar1 + 0x10))(0x400048);
      FUN_004abb00(iVar6,0);
    }
    if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
      FUN_004c3dc0(s_AltarUpgrade_0052e554);
    }
  }
  else {
LAB_004827d1:
    uVar9 = FUN_00482520();
    if (DAT_0052e170 == 0) {
      FUN_00482560(uVar9,*(undefined1 *)((int)this + 0x20));
    }
    *(undefined4 *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0x98 + DAT_0050f834) = uVar9;
  }
  iVar5 = 1;
  do {
    FUN_0040f0e0(iVar5);
    iVar5 = iVar5 + 1;
  } while (iVar5 < 9);
  if (DAT_0052e170 == 0) {
    if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
      FUN_004825f0();
    }
    if (DAT_0050f6c8 == uVar3) {
      iVar6 = FUN_00483e70();
      iVar5 = (uint)*(byte *)((int)this + 0x20) * 0xac + DAT_0050f834;
      pcVar8 = s_MultiplayerPriestDead__s__d__0052e564;
    }
    else {
      if (DAT_0050f6c8 == *(byte *)((int)this + 0x20)) goto LAB_004828ca;
      iVar6 = (uint)*(byte *)((int)this + 0x20) * 0xac + DAT_0050f834 + 0xc;
      iVar5 = (uint)uVar3 * 0xac + DAT_0050f834;
      pcVar8 = s_MultiplayerOtherPriestDead__s__s_0052e584;
    }
    uVar9 = FUN_00420560(local_100,pcVar8,iVar5 + 0xc,iVar6);
    FUN_004c3dc0(uVar9);
  }
LAB_004828ca:
  if ((DAT_0052e170 != 0) && (*(byte *)((int)this + 0x20) == DAT_0050f6c8)) {
    fVar7 = (float10)FUN_00414610();
    bVar4 = false;
    iVar5 = 1;
    FUN_00414650((float)(fVar7 + (float10)_DAT_00504044));
    do {
      iVar6 = FUN_0040da20(iVar5);
      if ((iVar6 == 0) && ((&DAT_0050f6e0)[DAT_0050f6c8 * 9 + iVar5] == 0)) {
        bVar4 = true;
      }
      iVar5 = iVar5 + 1;
    } while (iVar5 < 9);
    if (bVar4) {
      FUN_004c3dc0(s_SacrificeForStormpower_0052e5a8);
    }
  }
  return;
}


// ==== FUN_004e7610 @ 004e7610

undefined4 __thiscall FUN_004e7610(void *this)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_0047d020(*(undefined1 *)((int)this + 10),0,0);
  iVar2 = FUN_004eb310(((int)this - DAT_005395dc) / 0x24,uVar1);
  if (iVar2 != 0) {
    uVar1 = FUN_004af4e0(iVar2 * 0x24 + DAT_005395dc);
    return uVar1;
  }
  return 0;
}


// ==== FUN_004e7660 @ 004e7660

undefined4 __thiscall FUN_004e7660(void)

{
  int iVar1;
  
  iVar1 = FUN_004e7610();
  if (iVar1 == 0) {
    return 0;
  }
  return *(undefined4 *)(iVar1 + 0x2f18);
}


// ==== FUN_0048f430 @ 0048f430

float10 FUN_0048f430(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_00461a40 @ 00461a40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00461a40(void *this,int param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  float10 fVar3;
  
  if (param_2 == DAT_00520fdc) {
    FUN_00455b40(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
    iVar1 = (**(code **)(*(int *)this + 0x98))(param_3 & 0xffff,(int)param_3 >> 0x10,0);
    if (iVar1 == 0) {
      uVar2 = FUN_004abae0(this);
      if ((uVar2 & 0x200000) == 0) {
        return (float10)_DAT_00502b74;
      }
    }
    return (float10)_DAT_00502b78;
  }
  if (param_2 != DAT_00520fe0) {
    fVar3 = (float10)FUN_0048f430(param_2,param_3,param_4);
    return fVar3;
  }
  if ((*(byte *)((int)this + 0x23) & 1) == 0) {
    return (float10)_DAT_00502b78;
  }
  iVar1 = FUN_004eb310(((int)this - DAT_005395dc) / 0x24,DAT_0051c9d8,0,0);
  if (iVar1 == 0) {
    FUN_004d06a0(((int)this - DAT_005395dc) / 0x24,0);
  }
  return (float10)_DAT_00502b78;
}


// ==== FUN_00482560 @ 00482560

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00482560(int param_1,int param_2)

{
  bool bVar1;
  bool bVar2;
  float10 fVar3;
  
  bVar1 = DAT_0050f6c8 == param_2;
  bVar2 = DAT_0051cbac == param_1;
  *(undefined1 *)(param_2 * 0xac + 0x69 + DAT_0050f834) = 0;
  if (bVar2) {
    if (bVar1) {
      fVar3 = (float10)FUN_00414610();
      FUN_00414650((float)(fVar3 + (float10)_DAT_00504044));
      FUN_004c3dc0(s_SacrificeForStormpower_0052e524);
      return;
    }
  }
  else if (DAT_0051cab8 == param_1) {
    if (bVar1) {
      FUN_0045ef80(0);
      return;
    }
  }
  else if (bVar1) {
    FUN_0040e210(param_1);
    DAT_0052e2e0 = param_1;
  }
  return;
}


// ==== FUN_00483cd0 @ 00483cd0

void __thiscall FUN_00483cd0(void *this)

{
  int iVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined4 uVar2;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_00483d5a;
  *unaff_FS_OFFSET = &local_10;
  FUN_00483c50();
  uVar2 = 0;
  iVar1 = FUN_004abf50(this,0);
  FUN_004acbc0(this,*(undefined4 *)(iVar1 + 0x15c),uVar2);
  iVar1 = FUN_004f1650(0x28);
  local_8 = 0;
  if (iVar1 != 0) {
    FUN_0048f1d0(iVar1,0x18,((int)this - DAT_005395dc) / 0x24,0x3df5c28f);
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004824f0 @ 004824f0

void __thiscall FUN_004824f0(void *this)

{
  FUN_00482430(((int)this - DAT_005395dc) / 0x24,4,*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_00483c50 @ 00483c50

void __thiscall FUN_00483c50(void *this)

{
  FUN_004eb4b0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),DAT_0051cbd8);
  return;
}


// ==== FUN_00482950 @ 00482950

void __thiscall FUN_00482950(void)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = FUN_004817e0();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    FUN_0040c840();
    return;
  }
  return;
}


