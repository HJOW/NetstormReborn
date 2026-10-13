// ==== FUN_004a18f0 @ 004a18f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl FUN_004a18f0(int param_1,int param_2,undefined4 param_3)

{
  double *pdVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *unaff_FS_OFFSET;
  char *pcVar7;
  char *pcVar8;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004a1f73;
  *unaff_FS_OFFSET = &local_10;
  if (param_2 == 0) {
    FUN_00490040(s_gunFormSid____0_00537bc4,s__ns_o_GunAnim_cpp_00537bb0,0x249);
  }
  puVar6 = (undefined4 *)(DAT_005395dc + param_2 * 0x24);
  uVar2 = FUN_004af4e0(puVar6);
  iVar3 = FUN_0048f8b0(uVar2);
  if (iVar3 != DAT_0051c9e8) {
    FUN_00490040(s_gunProcess_>getProcessType______g_00537be8,s__ns_o_GunAnim_cpp_00537bd4,0x24c);
  }
  iVar3 = (**(code **)*puVar6)();
  iVar3 = DAT_005395dc + iVar3 * 0x24;
  puVar6 = (undefined4 *)0x0;
  uVar5 = (uint)*(byte *)(iVar3 + 10);
  iVar4 = FUN_004ac420(iVar3);
  iVar3 = FUN_004ac440(iVar3);
  if (param_1 == 2) {
    if (iVar4 == 1) {
      puVar6 = (undefined4 *)FUN_004f1650(0x38);
      local_8 = 0;
      if (puVar6 == (undefined4 *)0x0) {
        puVar6 = (undefined4 *)0x0;
      }
      else {
        FUN_004a3610(puVar6,param_2,param_3);
        uVar2 = DAT_0051c9f0;
        local_8 = CONCAT31(local_8._1_3_,1);
        *puVar6 = &PTR_FUN_00505610;
        puVar6[8] = &PTR_FUN_00505608;
        FUN_004a1130(puVar6,param_2,param_3,uVar2);
        puVar6[8] = &PTR_FUN_00505518;
        *puVar6 = &PTR_FUN_00505520;
      }
    }
    else if ((iVar4 == 0) && (iVar3 == 0)) {
      puVar6 = (undefined4 *)FUN_004f1650(0x38);
      local_8 = 3;
      if (puVar6 == (undefined4 *)0x0) {
        puVar6 = (undefined4 *)0x0;
      }
      else {
        FUN_004a10c0(puVar6,param_2,param_3,DAT_0051c9f0);
        puVar6[8] = &PTR_FUN_00505568;
        puVar6[0xc] = 0;
        puVar6[0xd] = 0;
        puVar6[0xc] = 0;
        *puVar6 = &PTR_FUN_005055c0;
        *(undefined1 *)(puVar6 + 10) = 0xff;
        puVar6[8] = &PTR_FUN_005055b8;
        local_8 = CONCAT31((int3)((uint)local_8 >> 8),5);
        puVar6[0xd] = 0;
        *puVar6 = &PTR_FUN_00505570;
        uVar2 = DAT_0051c9f0;
        puVar6[8] = &PTR_FUN_0050556c;
        FUN_004a1130(puVar6,param_2,param_3,uVar2);
      }
    }
    goto LAB_004a1fa1;
  }
  if (param_1 == 5) {
    if (iVar4 == 0) {
      if (DAT_0051ca60 == uVar5) {
        iVar3 = FUN_004f1650(0x58);
        local_8 = 6;
        puVar6 = (undefined4 *)0x0;
        if (iVar3 != 0) {
          puVar6 = (undefined4 *)FUN_004a3960(iVar3,param_2,param_3);
        }
      }
      else if (DAT_0051cb74 == uVar5) {
        puVar6 = (undefined4 *)FUN_004f1650(0x40);
        local_8 = 7;
        if (puVar6 == (undefined4 *)0x0) {
          puVar6 = (undefined4 *)0x0;
        }
        else {
          FUN_004a10c0(puVar6,param_2,param_3,DAT_0051c9ec);
          local_8._0_1_ = 8;
          FUN_004a35f0(puVar6 + 0xc);
          puVar6[8] = &PTR_FUN_00505424;
          *(undefined1 *)(puVar6 + 10) = 0xff;
          puVar6[8] = &PTR_FUN_00505428;
          puVar6[0xe] = 0;
          uVar2 = DAT_0051c9ec;
          local_8 = CONCAT31(local_8._1_3_,9);
          *puVar6 = &PTR_FUN_00505430;
          *puVar6 = &PTR_FUN_005056b0;
          puVar6[8] = &PTR_FUN_005056a8;
          FUN_004a1130(puVar6,param_2,param_3,uVar2);
        }
      }
      else if (DAT_0051cb94 == uVar5) {
        puVar6 = (undefined4 *)FUN_004f1650(0x40);
        local_8 = 10;
        if (puVar6 == (undefined4 *)0x0) {
          puVar6 = (undefined4 *)0x0;
        }
        else {
          FUN_004a36b0(puVar6,param_2,param_3);
          uVar2 = DAT_0051c9ec;
          local_8 = CONCAT31(local_8._1_3_,0xb);
          *puVar6 = &PTR_FUN_005053d8;
          puVar6[8] = &PTR_FUN_005053d0;
          FUN_004a1130(puVar6,param_2,param_3,uVar2);
          puVar6[8] = &PTR_FUN_005056f8;
          *puVar6 = &PTR_FUN_00505700;
        }
      }
      goto LAB_004a1fa1;
    }
    if (iVar4 == 1) {
      if (iVar3 == 2) {
        puVar6 = (undefined4 *)FUN_004f1650(0x40);
        local_8 = 0xd;
        if (puVar6 == (undefined4 *)0x0) {
          puVar6 = (undefined4 *)0x0;
        }
        else {
          FUN_004a36b0(puVar6,param_2,param_3);
          uVar2 = DAT_0051c9ec;
          local_8 = CONCAT31(local_8._1_3_,0xe);
          *puVar6 = &PTR_FUN_005053d8;
          puVar6[8] = &PTR_FUN_005053d0;
          FUN_004a1130(puVar6,param_2,param_3,uVar2);
          puVar6[8] = &PTR_FUN_0050574c;
          *puVar6 = &PTR_FUN_00505750;
        }
      }
      else if (iVar3 == 1) {
        puVar6 = (undefined4 *)FUN_004f1650(0x50);
        local_8 = 0x10;
        if (puVar6 == (undefined4 *)0x0) {
          puVar6 = (undefined4 *)0x0;
        }
        else {
          FUN_004a3750(puVar6,param_2,param_3);
          pdVar1 = (double *)(puVar6 + 0x12);
          *puVar6 = &PTR_FUN_005057a0;
          puVar6[8] = &PTR_FUN_0050579c;
          puVar6[0x10] = 1;
          *(undefined4 *)pdVar1 = 0;
          puVar6[0x13] = 0;
          *(undefined4 *)pdVar1 = 0;
          puVar6[0x13] = 0;
          *pdVar1 = _DAT_005484a8 + _DAT_00505320;
        }
      }
      else {
        puVar6 = (undefined4 *)FUN_004f1650(0x40);
        local_8 = 0x12;
        if (puVar6 == (undefined4 *)0x0) {
          puVar6 = (undefined4 *)0x0;
        }
        else {
          FUN_004a36b0(puVar6,param_2,param_3);
          uVar2 = DAT_0051c9ec;
          local_8 = CONCAT31(local_8._1_3_,0x13);
          *puVar6 = &PTR_FUN_005053d8;
          puVar6[8] = &PTR_FUN_005053d0;
          FUN_004a1130(puVar6,param_2,param_3,uVar2);
          puVar6[8] = &PTR_FUN_00505478;
          *puVar6 = &PTR_FUN_00505480;
        }
      }
      goto LAB_004a1fa1;
    }
    uVar2 = 0x272;
    pcVar8 = s__ns_o_GunAnim_cpp_00537c18;
    pcVar7 = s_false_00537c2c;
  }
  else if (param_1 == 8) {
    if (DAT_0051cb44 == uVar5) {
      puVar6 = (undefined4 *)FUN_004f1650(0x40);
      local_8 = 0x15;
      if (puVar6 == (undefined4 *)0x0) {
        puVar6 = (undefined4 *)0x0;
      }
      else {
        FUN_004a10c0(puVar6,param_2,0,DAT_0051c9f0);
        local_8._0_1_ = 0x16;
        FUN_004a35f0(puVar6 + 0xc);
        puVar6[8] = &PTR_FUN_00505568;
        *(undefined1 *)(puVar6 + 10) = 0xff;
        puVar6[8] = &PTR_FUN_005055b8;
        uVar2 = DAT_0051c9f0;
        local_8 = CONCAT31(local_8._1_3_,0x17);
        *puVar6 = &PTR_FUN_005055c0;
        *puVar6 = &PTR_FUN_00505660;
        puVar6[8] = &PTR_FUN_00505658;
        puVar6[0xe] = 0;
        FUN_004a1130(puVar6,param_2,0,uVar2);
      }
      goto LAB_004a1fa1;
    }
    uVar2 = 0x27a;
    pcVar8 = s__ns_o_GunAnim_cpp_00537c34;
    pcVar7 = s_false_00537c48;
  }
  else {
    uVar2 = 0x27f;
    pcVar8 = s__ns_o_GunAnim_cpp_00537c50;
    pcVar7 = s_false_00537c64;
  }
  FUN_00490040(pcVar7,pcVar8,uVar2);
LAB_004a1fa1:
  uVar2 = puVar6[1];
  *unaff_FS_OFFSET = local_10;
  return uVar2;
}


// ==== FUN_004a3960 @ 004a3960

void * __thiscall FUN_004a3960(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar3;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004a3a62;
  *unaff_FS_OFFSET = &local_10;
  FUN_004a10c0(this,param_2,param_3,DAT_0051c9ec);
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_00505424;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined ***)this = &PTR_FUN_00505430;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_00505428;
  uVar1 = DAT_0051c9ec;
  local_8 = 1;
  *(undefined ***)this = &PTR_FUN_005054d0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_005054cc;
  FUN_004a1130(this,param_2,param_3,uVar1);
  fVar3 = (float10)FUN_004a28a0(this);
  *(float *)((int)this + 0x40) = (float)fVar3;
  uVar1 = DAT_00537904;
  *(undefined4 *)((int)this + 0x44) = DAT_00537900;
  *(undefined4 *)((int)this + 0x48) = uVar1;
  *(undefined4 *)((int)this + 0x4c) = 0;
  *(undefined4 *)((int)this + 0x50) = 0;
  iVar2 = FUN_0048f870(this);
  *(ushort *)(iVar2 + 0xc) = *(ushort *)(iVar2 + 0xc) & 0xfffe;
  iVar2 = FUN_0048f870(this);
  FUN_00448c10((iVar2 - DAT_005395dc) / 0x24);
  *unaff_FS_OFFSET = local_10;
  return this;
}


// ==== FUN_004a26c0 @ 004a26c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a26c0(void *this)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  char *pcStack_124;
  double *local_120;
  double local_100 [32];
  
  local_120 = (double *)0x4a26d1;
  iVar2 = FUN_0048f870();
  local_120 = (double *)0x4a26dc;
  iVar3 = FUN_004abf50(iVar2);
  iVar3 = *(int *)(iVar3 + 0x110);
  local_120 = (double *)0x4a26eb;
  iVar4 = FUN_004abf50(iVar2);
  iVar4 = *(int *)(iVar4 + 0x114) / iVar3;
  iVar5 = (int)((longlong)(ulonglong)*(byte *)(iVar2 + 0x22) / (longlong)iVar4);
  if (*(ushort *)((int)this + 0x2c) == 0) {
    local_120 = (double *)0x4a2716;
    iVar6 = FUN_004adbc0();
    local_100[0] = (double)CONCAT44(local_100[0]._4_4_,iVar6);
    fVar10 = (float10)iVar6 * (float10)_DAT_00505354;
  }
  else {
    iVar6 = (uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc;
    if ((*(byte *)(iVar6 + 0xb) & 3) == 0) {
      uVar1 = *(undefined4 *)(iVar6 + 0x12);
      *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar6 + 0xe);
      local_120 = local_100;
      *(undefined4 *)((int)this + 0x10) = uVar1;
      pcStack_124 = (char *)0x4a2758;
      puVar7 = (undefined4 *)FUN_004aef30();
      uVar1 = puVar7[1];
      *(undefined4 *)((int)this + 0x14) = *puVar7;
      *(undefined4 *)((int)this + 0x18) = uVar1;
    }
    local_120 = *(double **)((int)this + 0x18);
    pcStack_124 = *(char **)((int)this + 0x14);
    FUN_004aef30(iVar2,local_100);
    fVar10 = (float10)FUN_0043f530();
  }
  local_120 = (double *)(float)fVar10;
  pcStack_124 = (char *)0x4a2792;
  iVar6 = FUN_004a2640();
  if (*(double *)((int)this + 0x30) <= _DAT_005484a8) {
    if (iVar5 == iVar6) {
      local_120 = (double *)0x4;
      pcStack_124 = (char *)0x3;
      FUN_004a11a0(this);
      return;
    }
    if (iVar5 < iVar6) {
      iVar8 = (iVar5 - iVar6) + iVar3;
    }
    else {
      iVar8 = iVar5 - iVar6;
    }
    if (iVar6 < iVar5) {
      iVar6 = (iVar6 - iVar5) + iVar3;
    }
    else {
      iVar6 = iVar6 - iVar5;
    }
    iVar9 = 1;
    if (iVar8 <= iVar6) {
      iVar9 = -1;
    }
    if ((iVar5 == 0) && (iVar9 == -1)) {
      iVar3 = iVar3 + -1;
    }
    else {
      iVar3 = (iVar9 + iVar5) % iVar3;
    }
    local_120 = (double *)(iVar4 * iVar3);
    pcStack_124 = (char *)0x4a280d;
    FUN_004a12b0(this);
    local_100[0] = (double)CONCAT44(local_100[0]._4_4_,&pcStack_124);
    FUN_00436e50(&pcStack_124,0);
    FUN_004a3f00((int)this + 0x20);
    local_120 = (double *)0x4a282e;
    fVar10 = (float10)(**(code **)(*(int *)this + 0x2c))();
    local_100[0] = (double)(fVar10 + (float10)_DAT_005484a8);
    local_120 = (double *)0x2710;
    pcStack_124 = (char *)0x3e8;
    *(double *)((int)this + 0x30) = local_100[0];
    iVar3 = FUN_0048cc10();
    local_120 = (double *)(iVar3 / 1000);
    pcStack_124 = s_sunDiscThrowerRatchet0_d_wav_00537ce4;
    pcStack_124 = (char *)FUN_00420560(local_100);
    local_120 = (double *)0x0;
    FUN_00438c00(*(undefined4 *)(iVar2 + 0xe),*(undefined4 *)(iVar2 + 0x12));
  }
  return;
}


// ==== FUN_004a2640 @ 004a2640

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl FUN_004a2640(float param_1)

{
  int iVar1;
  int iVar2;
  float local_8;
  
  iVar1 = 0;
  iVar2 = 0;
  local_8 = -1000.0;
  do {
    if (_DAT_005052f8 <= local_8) break;
    if (ABS((float)(&DAT_005378b0)[iVar2] - param_1) < ABS(local_8)) {
      iVar1 = iVar2;
      local_8 = (float)(&DAT_005378b0)[iVar2] - param_1;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x14);
  if ((iVar1 == 0x13) && (ABS(_DAT_00505350 - param_1) < ABS(local_8))) {
    iVar1 = 0;
  }
  return iVar1;
}


// ==== FUN_004a28a0 @ 004a28a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_004a28a0(void)

{
  double dVar1;
  int iVar2;
  uint uVar3;
  
  iVar2 = FUN_0048f870();
  uVar3 = (uint)*(byte *)(iVar2 + 0x22);
  if (0xf < *(byte *)(iVar2 + 0x22)) {
    uVar3 = uVar3 - 0x10;
  }
  dVar1 = (double)(int)uVar3 * _DAT_00505358 + _DAT_00505360;
  if ((dVar1 < _DAT_005052f0) || (_DAT_00505360 < dVar1)) {
    FUN_00490040(s_0_0_<__a_&&_a_<__360_0_00537d18,s__ns_o_GunAnim_cpp_00537d04,0x402);
  }
  return (float10)dVar1;
}


// ==== FUN_004a2910 @ 004a2910

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_004a2910(void *this)

{
  uint uVar1;
  uint uVar2;
  float10 fVar3;
  double dStack00000004;
  
  if ((*(float *)((int)this + 0x48) != DAT_00537904) &&
     (-DAT_00537904 != *(float *)((int)this + 0x48))) {
    FUN_00490040(s_angularAccel____initAngularAccel_00537d44,s__ns_o_GunAnim_cpp_00537d30,0x408);
  }
  fVar3 = (float10)FUN_004f28fa();
  dStack00000004 = (double)((float10)_DAT_00505360 - fVar3);
  if (((float10)_DAT_00505360 - fVar3 <= (float10)_DAT_005052f0) || (_DAT_00505360 < dStack00000004)
     ) {
    FUN_00490040(s_0_0_<_a_&&_a_<__360_0_00537da0,s__ns_o_GunAnim_cpp_00537d8c,0x40b);
  }
  if (_DAT_00505360 <= dStack00000004) {
    dStack00000004 = 359.0;
  }
  uVar1 = FUN_0048ee10((float)((float10)dStack00000004 * (float10)_DAT_00505378));
  if (*(int *)((int)this + 0x4c) != 0) {
    uVar2 = (int)(uVar1 + 2) >> 0x1f;
    return (((uVar1 + 2 ^ uVar2) - uVar2 & 0xf ^ uVar2) - uVar2) + 0x10;
  }
  uVar2 = (int)uVar1 >> 0x1f;
  return ((uVar1 ^ uVar2) - uVar2 & 0xf ^ uVar2) - uVar2;
}


// ==== FUN_004a2a10 @ 004a2a10

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a2a10(void *this)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  undefined4 uVar5;
  int iVar6;
  bool bVar7;
  float10 fVar8;
  undefined8 uVar9;
  undefined4 local_10;
  
  if (*(short *)((int)this + 0x2c) == 0) {
    FUN_00490040(s_getTargetSid______0_00537dcc,s__ns_o_GunAnim_cpp_00537db8,0x424);
  }
  if ((_DAT_00548430 <= 0.0) || (_DAT_00505380 <= _DAT_00548430)) goto LAB_004a2c58;
  iVar2 = FUN_0048f870(this);
  iVar6 = (uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc;
  bVar7 = (*(byte *)(iVar6 + 0xb) & 3) == 0;
  if (bVar7) {
    uVar5 = *(undefined4 *)(iVar6 + 0x12);
    *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar6 + 0xe);
    *(undefined4 *)((int)this + 0x10) = uVar5;
    puVar3 = (undefined4 *)FUN_004aef30();
    uVar5 = puVar3[1];
    *(undefined4 *)((int)this + 0x14) = *puVar3;
    *(undefined4 *)((int)this + 0x18) = uVar5;
  }
  puVar3 = (undefined4 *)(iVar2 + 0xe);
  uVar5 = *puVar3;
  uVar1 = *(undefined4 *)(iVar2 + 0x12);
  *(float *)((int)this + 0x40) =
       *(float *)((int)this + 0x44) * (float)_DAT_00548430 + *(float *)((int)this + 0x40);
  *(float *)((int)this + 0x44) =
       *(float *)((int)this + 0x48) * (float)_DAT_00548430 + *(float *)((int)this + 0x44);
  fVar8 = (float10)FUN_00440ac0(uVar5,uVar1,*(undefined4 *)((int)this + 0x14),
                                *(undefined4 *)((int)this + 0x18));
  fVar8 = (float10)_DAT_00537908 * (float10)_DAT_00505350 + (fVar8 - (float10)_DAT_00537920);
  if (*(int *)((int)this + 0x50) == 0) {
    if ((*(int *)((int)this + 0x4c) == 0) &&
       ((float)fVar8 - _DAT_00537918 <= *(float *)((int)this + 0x40))) {
      uVar5 = *puVar3;
      uVar1 = *(undefined4 *)(iVar2 + 0x12);
      *(float *)((int)this + 0x48) = -*(float *)((int)this + 0x48);
      *(undefined4 *)((int)this + 0x4c) = 1;
      FUN_00438c00(uVar5,uVar1,s_sasprung_wav_00537de0,0);
    }
    if (*(int *)((int)this + 0x50) == 0) {
      if ((float)fVar8 <= *(float *)((int)this + 0x40)) {
        *(undefined4 *)((int)this + 0x4c) = 1;
        *(undefined4 *)((int)this + 0x50) = 1;
        *(byte *)(iVar2 + 0xc) = *(byte *)(iVar2 + 0xc) | 1;
        FUN_00448c10();
        iVar6 = FUN_004a12f0(this);
        if ((iVar6 == 0) && (bVar7)) {
          return;
        }
        FUN_00438c00(*puVar3,*(undefined4 *)(iVar2 + 0x12),s_safired_wav_00537df0,0);
      }
      goto LAB_004a2bc7;
    }
LAB_004a2bcd:
    if ((*(int *)((int)this + 0x4c) != 0) &&
       (_DAT_0053791c + (float)fVar8 <= *(float *)((int)this + 0x40))) {
      *(undefined4 *)((int)this + 0x4c) = 0;
    }
  }
  else {
LAB_004a2bc7:
    if (*(int *)((int)this + 0x50) != 0) goto LAB_004a2bcd;
  }
  if ((0 < *(int *)((int)this + 0x40)) &&
     (uVar4 = FUN_004a2910(this,(double)*(float *)((int)this + 0x40)),
     *(byte *)(iVar2 + 0x22) != uVar4)) {
    FUN_004a12b0(this);
    if (*(uint *)((int)this + 0x44) < 0x80000001) {
      if (*(char *)(iVar2 + 0x22) != '\x02') goto LAB_004a2c58;
      local_10 = *puVar3;
      uVar5 = *(undefined4 *)(iVar2 + 0x12);
      uVar9 = 0x537e0c;
    }
    else {
      local_10 = *puVar3;
      uVar5 = *(undefined4 *)(iVar2 + 0x12);
      uVar9 = 0x537dfc;
    }
    FUN_00438c00(local_10,uVar5,uVar9);
  }
LAB_004a2c58:
  if ((((*(int *)((int)this + 0x50) != 0) && (*(float *)((int)this + 0x44) <= _DAT_00537910)) &&
      (fVar8 = (float10)FUN_0043f4e0(), ABS(fVar8 - (float10)_DAT_0053790c) < (float10)_DAT_00537914
      )) || (0x80000000 < *(uint *)((int)this + 0x40))) {
    FUN_004a11a0(this,6,7);
  }
  return;
}


// ==== FUN_004a1130 @ 004a1130

void __thiscall FUN_004a1130(void *this,undefined4 param_2)

{
  undefined4 *puVar1;
  ushort *puVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  (**(code **)(*(int *)this + 0x3c))(param_2);
  (**(code **)(*(int *)this + 0x40))(param_2);
  puVar1 = (undefined4 *)(**(code **)(*(int *)this + 0x44))();
  uVar3 = DAT_005484ac;
  *puVar1 = DAT_005484a8;
  puVar1[1] = uVar3;
  puVar2 = (ushort *)(**(code **)(*(int *)this + 0x38))();
  uVar5 = 0x10;
  uVar4 = 0;
  uVar3 = (*(code *)**(undefined4 **)((uint)*puVar2 * 0x24 + DAT_005395dc))(0,0x10);
  FUN_0048f590(this,param_2,uVar3,uVar4,uVar5);
  return;
}


// ==== FUN_004a11a0 @ 004a11a0

void __thiscall FUN_004a11a0(void *this,undefined4 param_2,undefined4 param_3)

{
  ushort *puVar1;
  int *piVar2;
  int iVar3;
  
  puVar1 = (ushort *)(**(code **)(*(int *)this + 0x38))();
  piVar2 = (int *)FUN_004af4e0((uint)*puVar1 * 0x24 + DAT_005395dc);
  iVar3 = FUN_0048f8b0(piVar2);
  if (iVar3 != DAT_0051c9e8) {
    FUN_00490040(s_gunProcess_>getProcessType______g_00537a88,s__ns_o_GunAnim_cpp_00537a74,0x1c9);
  }
  (**(code **)(*piVar2 + 0x1c))(1,param_2,param_3);
  FUN_0048f7b0(this,0x10);
  return;
}


// ==== FUN_004a12b0 @ 004a12b0

void __thiscall FUN_004a12b0(void)

{
  int *piVar1;
  undefined1 unaff_retaddr;
  
  piVar1 = (int *)FUN_0048f870();
  (**(code **)(*piVar1 + 0x48))(0);
  *(undefined1 *)((int)piVar1 + 0x22) = unaff_retaddr;
  (**(code **)(*piVar1 + 0x90))
            (*(undefined4 *)((int)piVar1 + 0xe),*(undefined4 *)((int)piVar1 + 0x12),0x40);
  return;
}


// ==== FUN_004a12f0 @ 004a12f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_004a12f0(void *this)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  ushort *puVar7;
  int iVar8;
  undefined4 *unaff_FS_OFFSET;
  float10 fVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004a14b3;
  *unaff_FS_OFFSET = &local_10;
  iVar4 = (uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc;
  iVar8 = *(int *)((int)this + 8) * 0x24 + DAT_005395dc;
  if ((*(byte *)(iVar4 + 0xb) & 3) == 0) {
    uVar1 = FUN_004abac0(iVar4);
    if ((uVar1 & 0x10) != 0) {
      iVar2 = FUN_0048f870(this);
      FUN_004793d0(&local_1c,(iVar2 - DAT_005395dc) / 0x24,*(undefined2 *)((int)this + 0x2c));
      if (*(byte *)(iVar8 + 10) != DAT_0051cb94) {
        iVar4 = FUN_004f1650(0x68);
        local_8 = 0;
        local_14 = iVar4;
        if (iVar4 != 0) {
          iVar8 = *(int *)this;
          uVar11 = 0;
          uVar3 = *(undefined4 *)((int)this + 0x10);
          uVar10 = *(undefined4 *)((int)this + 0xc);
          uVar12 = DAT_00565498;
          uVar13 = DAT_0056549c;
          uVar5 = (**(code **)(iVar8 + 0x28))
                            (uVar10,uVar3,local_1c,local_18,0,DAT_00565498,DAT_0056549c);
          uVar6 = (**(code **)(iVar8 + 0x24))(uVar5);
          fVar9 = (float10)(**(code **)(iVar8 + 0x20))(uVar6);
          FUN_0044ac70(iVar4,*(undefined4 *)((int)this + 8),*(undefined2 *)((int)this + 0x2c),
                       (float)fVar9,uVar6,uVar5,uVar10,uVar3,local_1c,local_18,uVar11,uVar12,uVar13)
          ;
        }
        local_8 = 0xffffffff;
        puVar7 = (ushort *)(**(code **)(*(int *)this + 0x38))();
        iVar8 = FUN_004af4e0((uint)*puVar7 * 0x24 + DAT_005395dc);
        iVar4 = iVar8 + -0x28;
        if (iVar8 == 0) {
          iVar4 = 0;
        }
        iVar4 = FUN_0048f8b0(iVar4 + 0x28);
        if (iVar4 != DAT_0051c9e8) {
          FUN_00490040(s_gunProcess_>getProcessType______g_00537b48,s__ns_o_GunAnim_cpp_00537b34,
                       0x224);
        }
        uVar3 = 1;
        goto LAB_004a14cc;
      }
      FUN_00413890(*(undefined4 *)((int)this + 8),*(undefined2 *)((int)this + 0x2c));
      if ((*(byte *)(iVar4 + 0xb) & 3) == 0) {
        if (*(byte *)(iVar4 + 10) == DAT_0051cb8c) {
          fVar9 = (float10)FUN_0048cc50(0,0x447a0000);
          if (fVar9 < (float10)_DAT_0051f138) goto LAB_004a13d6;
        }
        FUN_004432f0(iVar8,*(undefined2 *)((int)this + 0x2c));
      }
      else {
LAB_004a13d6:
        FUN_004433e0(iVar8);
      }
      if ((*(byte *)(iVar4 + 0xb) & 3) == 0) {
        uVar3 = 1;
        goto LAB_004a14cc;
      }
    }
  }
  uVar3 = 0;
LAB_004a14cc:
  *unaff_FS_OFFSET = local_10;
  return uVar3;
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


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_0043f530 @ 0043f530

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0043f530(float *param_1,float param_2,float param_3)

{
  float10 fVar1;
  
  if ((*param_1 == param_2) && (param_1[1] == param_3)) {
    return (float10)_DAT_00501748;
  }
  if (DAT_00546778 == 0) {
    fVar1 = (float10)fpatan(-((float10)param_3 - (float10)param_1[1]),
                            (float10)param_2 - (float10)*param_1);
  }
  else {
    fVar1 = (float10)FUN_004f14c5();
  }
  FUN_0043f4c0((double)fVar1);
  fVar1 = (float10)FUN_0043f4e0();
  return fVar1;
}


// ==== FUN_00440ac0 @ 00440ac0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_00440ac0(float param_1,float param_2,float param_3,float param_4)

{
  float10 fVar1;
  
  if ((param_3 == param_1) && (param_4 == param_2)) {
    return (float10)_DAT_00501748;
  }
  if (DAT_00546778 == 0) {
    fVar1 = (float10)fpatan(-((float10)param_4 - (float10)param_2),
                            (float10)param_3 - (float10)param_1);
  }
  else {
    fVar1 = (float10)FUN_004f14c5();
  }
  fVar1 = (float10)FUN_0043f4c0((double)fVar1);
  return fVar1;
}


// ==== FUN_0043f4e0 @ 0043f4e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_0043f4e0(void)

{
  float10 fVar1;
  float fStack00000004;
  
  fVar1 = (float10)FUN_004f28fa();
  fStack00000004 = (float)fVar1;
  if (fVar1 < (float10)_DAT_00501738) {
    fStack00000004 = fStack00000004 + _DAT_00501740;
  }
  if (0x43b3ffff < (int)fStack00000004) {
    fStack00000004 = 359.9999;
  }
  return (float10)fStack00000004;
}


// ==== FUN_0048ee10 @ 0048ee10

void __cdecl FUN_0048ee10(void)

{
  __ftol();
  return;
}


// ==== FUN_004f28fa @ 004f28fa

void __cdecl FUN_004f28fa(void)

{
  __cintrindisp2();
  return;
}


