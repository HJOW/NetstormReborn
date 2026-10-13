// ==== FUN_00483800 @ 00483800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00483800(void *this,int param_2,int param_3,float param_4)

{
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  int *piVar4;
  int iVar5;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar6;
  undefined4 uVar7;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00483bdf;
  *unaff_FS_OFFSET = &local_10;
  iVar2 = FUN_00483c50();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    puVar3 = (undefined4 *)FUN_00483120(uVar1,&local_18);
    local_20 = *puVar3;
    local_1c = puVar3[1];
  }
  if (param_2 == 0x15) {
    iVar2 = FUN_004ae4b0(this);
    if (iVar2 != 0xf) {
      uVar7 = 0x14;
      iVar2 = FUN_0048cc10(0,100000);
      if (iVar2 / 1000 < DAT_0052e348) {
        uVar7 = 0x16;
        FUN_004acce0(this,0xffffffff,0);
      }
      local_14 = FUN_004f1650(0x28);
      local_8 = 0;
      if (local_14 != 0) {
        FUN_0048f1d0(local_14,uVar7,((int)this - DAT_005395dc) / 0x24,0x3da3d70a);
      }
      local_8 = 0xffffffff;
      if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
        iVar2 = FUN_004ae4b0(uVar1);
        if (iVar2 != 10) {
          iVar2 = FUN_0048cc10(0,5,0);
          FUN_00438c00(local_20,local_1c,(&PTR_s_chainRattle23_wav_0052e350)[iVar2]);
          iVar2 = FUN_0048cc10(0,3000,0);
          FUN_00438c00(local_20,local_1c,(&PTR_s_priestStruggleFade01_wav_0052e368)[iVar2 / 1000]);
        }
      }
      fVar6 = (float10)FUN_0048cc50(0,0x40000000);
      fVar6 = fVar6 + (float10)DAT_0052e344;
      goto LAB_00483bf9;
    }
  }
  else if ((param_2 == 0x14) || (param_2 == 0x16)) {
    iVar2 = FUN_004ae4b0(this);
    if (iVar2 != 0xe) {
      fVar6 = (float10)_DAT_00504038;
      goto LAB_00483bf9;
    }
    iVar5 = (-(uint)(param_2 == 0x14) & 2) - 1;
    iVar2 = FUN_004acce0(this,iVar5,0);
    if (iVar2 != 0) {
      if (iVar5 == -1) {
        FUN_004acce0(this,1,0);
      }
      fVar6 = (float10)_DAT_00504038;
      goto LAB_00483bf9;
    }
  }
  else if (param_2 != 0x17) {
    if (param_2 != 0x18) {
      fVar6 = (float10)FUN_00461a40(this,param_2,param_3,param_4);
      goto LAB_00483bf9;
    }
    iVar2 = FUN_00483c50(this);
    piVar4 = (int *)(DAT_005395dc + iVar2 * 0x24);
    iVar2 = FUN_004ae4b0(piVar4);
    if (iVar2 == 10) {
      if (DAT_00540a2c != 0) {
        (**(code **)(*piVar4 + 0x10))(0x400000);
        fVar6 = (float10)_DAT_0050405c;
        goto LAB_00483bf9;
      }
    }
    else {
      iVar2 = FUN_004acce0(this,1,0);
      if (iVar2 == 0) {
        if (param_3 < 3) {
LAB_004839f8:
          if (((int *)(DAT_005395dc + 0x24U) <= piVar4) &&
             (piVar4 < (int *)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
            FUN_00483040(piVar4,param_3);
          }
        }
        else {
          iVar2 = FUN_0048cc10(0,100000);
          if (iVar2 / 1000 < 0x32) goto LAB_004839f8;
        }
        fVar6 = (float10)FUN_0048cc50(0,0x3f800000);
        if (fVar6 < (float10)param_4) {
          local_18 = *(undefined4 *)((int)this + 0xe);
          FUN_00438c00(local_18,*(undefined4 *)((int)this + 0x12),s_altarBurnCollapse_wav_0052e6f8,0
                      );
        }
      }
      else {
        iVar2 = FUN_00483c50(this);
        uVar1 = iVar2 * 0x24 + DAT_005395dc;
        if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc)))
        {
          if (DAT_00540a2c != 0) {
            iVar2 = FUN_004eb260((int)(uVar1 - DAT_005395dc) / 0x24,DAT_0051cbbc,0,0);
            if (iVar2 != 0) {
              (**(code **)(*(int *)(DAT_005395dc + iVar2 * 0x24) + 0x10))(8);
            }
          }
          FUN_004547d0(((int)this - DAT_005395dc) / 0x24);
          local_18 = *(undefined4 *)((int)this + 0xe);
          FUN_00438c00(local_18,*(undefined4 *)((int)this + 0x12),s_explosion_wav_0052e6e8,0);
          uVar7 = 0;
          iVar2 = FUN_004abf50(uVar1,0);
          FUN_004acbc0(uVar1,*(undefined4 *)(iVar2 + 0x158),uVar7);
          uVar7 = 0;
          iVar2 = FUN_004abf50(this,0);
          FUN_004acbc0(this,*(undefined4 *)(iVar2 + 0x160),uVar7);
          fVar6 = (float10)_DAT_00504058;
          goto LAB_00483bf9;
        }
      }
    }
  }
  fVar6 = (float10)param_4;
LAB_00483bf9:
  *unaff_FS_OFFSET = local_10;
  return fVar6;
}


// ==== FUN_00483040 @ 00483040

void __thiscall FUN_00483040(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004ae4b0();
  iVar2 = FUN_004abf50(this);
  if (param_2 < 3) {
    if (param_2 == 0) {
      iVar1 = (**(code **)(*(int *)this + 0x80))();
      iVar1 = 0xe - iVar1;
    }
    FUN_004acbc0(this,*(int *)(iVar2 + 0x130 + iVar1 * 4) + param_2,0);
    return;
  }
  FUN_0048cc10(3000,*(int *)(iVar2 + 0x170 + iVar1 * 4) * 1000);
  return;
}


// ==== FUN_00482ec0 @ 00482ec0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00482ec0(void *this)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int *piVar4;
  float *pfVar5;
  int *piVar6;
  undefined1 auStack_14 [8];
  undefined1 auStack_c [12];
  
  iVar3 = FUN_004816a0();
  uVar1 = iVar3 * 0x24 + DAT_005395dc;
  if ((uVar1 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)) {
    FUN_00490040(s_altar_isValid___0052e5f0,s_dais_cpp_0052e5e4,0x3ac);
  }
  if (DAT_00540a2c == 0) {
    FUN_00490040(&DAT_0052e60c,s_dais_cpp_0052e600,0x3ae);
  }
  iVar3 = FUN_004eb260(((int)this - DAT_005395dc) / 0x24,DAT_0051cbbc,0,0);
  if (iVar3 != 0) {
    piVar4 = (int *)FUN_004ab390(DAT_0051cbbc,0);
    piVar6 = (int *)(DAT_005395dc + iVar3 * 0x24);
    FUN_004abb00(piVar4,(short)piVar6[3]);
    (**(code **)(*piVar4 + 0x74))(0);
    iVar3 = FUN_004aef30(this,auStack_c);
    uVar2 = *(undefined4 *)(iVar3 + 4);
    pfVar5 = (float *)FUN_004aef30(this,auStack_14);
    (**(code **)(*piVar4 + 0x90))(*pfVar5 + _DAT_00504024,uVar2,0);
    FUN_004e5c10(piVar4,0x40a00000,0);
    (**(code **)(*piVar6 + 0x10))(0);
  }
  FUN_00438b50(s_priestFree_wav_0052e614,0,0,0,1,0);
  if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
    FUN_00438b50(s_enemyPriestEsacping_wav_0052e624,0,0,0,1,0);
  }
  FUN_00482950(this);
  return;
}


// ==== FUN_004821e0 @ 004821e0

void __thiscall FUN_004821e0(void *this,uint param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  int *piVar8;
  undefined **local_60 [2];
  int local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (((*(byte *)((int)this + 0x23) & 1) == 0) && ((*(byte *)((int)this + 0x23) & 8) == 0)) {
    if ((param_2 & 0x400000) != 0) {
      local_60[0] = &PTR_LAB_00500d40;
      local_54 = 1;
      uVar1 = FUN_004816a0();
      FUN_004eb960(local_60,uVar1);
      while (local_58 != 0) {
        uVar1 = FUN_004eb9f0(local_60);
        iVar2 = FUN_004af4e0(uVar1);
        iVar3 = FUN_0048f8b0(iVar2);
        if (((iVar3 == DAT_0051ca18) || (iVar3 = FUN_0048f8b0(iVar2), iVar3 == DAT_0051ca54)) &&
           (*(int *)(iVar2 + 0x18) == 0x18)) {
          FUN_00482620(this);
        }
        FUN_004eb990(local_60);
      }
    }
    FUN_00482950(this);
    puVar4 = (undefined4 *)FUN_004aeec0(this,local_60,0);
    local_50 = *puVar4;
    local_4c = puVar4[1];
    local_48 = puVar4[2];
    local_44 = puVar4[3];
    local_8 = 0;
    local_40[0] = &PTR_FUN_005003d0;
    local_4 = 0;
    FUN_00440a50(&local_50);
    FUN_00440a80(&local_48);
    uVar1 = __ftol();
    uVar5 = __ftol(uVar1);
    uVar6 = __ftol(uVar5);
    uVar7 = __ftol(uVar6);
    FUN_004eae20(local_40,uVar7,uVar6,uVar5,uVar1);
    while (local_c != 0) {
      piVar8 = (int *)(local_c * 0x24 + DAT_005395dc);
      if (*(byte *)((int)piVar8 + 10) == DAT_0051cbd4) {
        (**(code **)(*piVar8 + 0x10))(0x48);
      }
      FUN_004eafe0(local_40);
    }
    iVar2 = 1;
    do {
      FUN_0040f0e0(iVar2);
      iVar2 = iVar2 + 1;
    } while (iVar2 < 9);
  }
  FUN_004615e0(this,param_2);
  return;
}


// ==== FUN_00483670 @ 00483670

undefined4 FUN_00483670(void)

{
  int *piVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = FUN_00483c50();
  piVar1 = (int *)(iVar2 * 0x24 + DAT_005395dc);
  if (((int *)(DAT_005395dc + 0x24U) <= piVar1) &&
     (piVar1 < (int *)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
                    /* WARNING: Could not recover jumptable at 0x0048369e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    uVar3 = (**(code **)(*piVar1 + 0x80))();
    return uVar3;
  }
  return 0;
}


// ==== FUN_004eb260 @ 004eb260

int __cdecl FUN_004eb260(undefined4 param_1,uint param_2,int param_3,int param_4)

{
  int iVar1;
  int *piVar2;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_005004b0;
  local_4 = 0;
  FUN_004eb8d0(local_10,param_1);
  do {
    if (local_8 == 0) {
      return 0;
    }
    piVar2 = (int *)(DAT_005395dc + local_8 * 0x24);
    if (*(ushort *)((int)piVar2 + 0x12) == param_2) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return local_8;
        }
        if ((DAT_00540a28 != 0) || ((iVar1 = (local_8 * 0x24) / 0x24, 4 < iVar1 && (iVar1 < 6000))))
        {
          (**(code **)(*piVar2 + 0x10))(0);
        }
      }
      param_3 = param_3 + -1;
    }
    FUN_004eb900(local_10);
  } while( true );
}


// ==== FUN_00483120 @ 00483120

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00483120(void *this,float *param_2)

{
  float fVar1;
  
  fVar1 = *(float *)((int)this + 0x12);
  *param_2 = *(float *)((int)this + 0xe) - _DAT_00504058;
  param_2[1] = fVar1 - _DAT_00504058;
  return;
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


