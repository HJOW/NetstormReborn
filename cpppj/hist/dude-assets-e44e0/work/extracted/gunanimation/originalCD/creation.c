// ==== FUN_004139f0 @ 004139f0

void __cdecl FUN_004139f0(void)

{
  return;
}


// ==== FUN_00413a10 @ 00413a10

void FUN_00413a10(void)

{
  return;
}


// ==== FUN_00413a20 @ 00413a20

void __cdecl FUN_00413a20(void)

{
  return;
}


// ==== FUN_00413a30 @ 00413a30

void FUN_00413a30(void)

{
  return;
}


// ==== FUN_0041f9f0 @ 0041f9f0

void FUN_0041f9f0(void)

{
  return;
}


// ==== FUN_004a0c70 @ 004a0c70

int __fastcall FUN_004a0c70(int param_1)

{
  return param_1 + 0x2a;
}


// ==== FUN_004a0c80 @ 004a0c80

int __fastcall FUN_004a0c80(int param_1)

{
  return param_1 + 0x2a;
}


// ==== FUN_004a10c0 @ 004a10c0

void * __thiscall FUN_004a10c0(void *this)

{
  undefined4 uVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_10 [12];
  
  uVar1 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = local_10;
  *(undefined ***)this = &PTR_FUN_00500d68;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)this = &PTR_FUN_00505388;
  *unaff_FS_OFFSET = uVar1;
  return this;
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


// ==== FUN_004a1fc0 @ 004a1fc0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a1fc0(void *this)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  uint uVar4;
  float10 fVar5;
  undefined1 *puStack_2c;
  char *pcStack_28;
  char *pcStack_24;
  int iStack_20;
  undefined1 local_10 [8];
  undefined1 local_8 [8];
  
  if (*(short *)((int)this + 0x2c) == 0) {
    iStack_20 = 0x2a4;
    pcStack_24 = s__ns_o_GunAnim_cpp_00537c6c;
    pcStack_28 = s_getTargetSid______0_00537c80;
    puStack_2c = (undefined1 *)0x4a1fe3;
    FUN_00490040();
  }
  iStack_20 = 0x4a1fed;
  iVar1 = FUN_0048f870(this);
  if (*(double *)((int)this + 0x30) <= _DAT_005484a8) {
    pcStack_24 = local_8;
    iStack_20 = 1;
    pcStack_28 = (char *)0x4a2023;
    puVar2 = (undefined4 *)FUN_004aef30((uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc);
    pcStack_28 = (char *)*puVar2;
    pcStack_24 = (char *)puVar2[1];
    puStack_2c = local_10;
    uVar3 = FUN_004aef30(iVar1);
    puStack_2c = (undefined1 *)0x4a203d;
    iStack_20 = FUN_0043f620(uVar3);
    iStack_20 = iStack_20 / 2;
    pcStack_24 = (char *)(uint)*(byte *)(iVar1 + 0x22);
    pcStack_28 = (char *)0x4a2051;
    uVar4 = (**(code **)(*(int *)this + 0x34))();
    if ((uVar4 & 0x8000) != 0) {
      pcStack_28 = (char *)0x4;
      puStack_2c = (undefined1 *)0x3;
      FUN_004a11a0(this);
      return;
    }
    pcStack_28 = (char *)(uVar4 & 0x3fff);
    puStack_2c = (undefined1 *)0x4a2075;
    FUN_004a12b0(this);
    FUN_00436e50(&puStack_2c,0);
    FUN_004a3f00((int)this + 0x20);
    pcStack_28 = (char *)0x4a2096;
    fVar5 = (float10)(**(code **)(*(int *)this + 0x2c))();
    *(double *)((int)this + 0x30) = (double)(fVar5 + (float10)_DAT_005484a8);
  }
  return;
}


// ==== FUN_004a20c0 @ 004a20c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a20c0(void *this)

{
  double *pdVar1;
  int iVar2;
  uint uVar3;
  float10 fVar4;
  undefined4 uStack_20;
  uint uStack_1c;
  uint uStack_18;
  int iStack_14;
  
  pdVar1 = (double *)((int)this + 0x30);
  iStack_14 = 0x4a20cf;
  iVar2 = FUN_0048f870();
  if (*pdVar1 <= _DAT_005484a8) {
    iStack_14 = *(int *)((int)this + 0x38) / 2;
    uStack_18 = (uint)*(byte *)(iVar2 + 0x22);
    uStack_1c = 0x4a20fb;
    uVar3 = (**(code **)(*(int *)this + 0x34))();
    if ((uVar3 & 0x8000) != 0) {
      if (*(int *)((int)this + 0x38) == 8) {
        uStack_1c = 10;
        uStack_20 = 9;
        FUN_004a11a0(this);
        return;
      }
      *(int *)((int)this + 0x38) = *(int *)((int)this + 0x38) + 2;
      FUN_00436e50(&uStack_20,0);
      FUN_004a3f00((int)this + 0x20);
      uStack_1c = 0x4a2140;
      fVar4 = (float10)(**(code **)(*(int *)this + 0x2c))();
      *pdVar1 = (double)(fVar4 + (float10)_DAT_005484a8);
      return;
    }
    uStack_1c = uVar3 & 0x3fff;
    uStack_20 = 0x4a216a;
    FUN_004a12b0(this);
    FUN_00436e50(&uStack_20,0);
    FUN_004a3f00((int)this + 0x20);
    uStack_1c = 0x4a218b;
    fVar4 = (float10)(**(code **)(*(int *)this + 0x2c))();
    *pdVar1 = (double)(fVar4 + (float10)_DAT_005484a8);
  }
  return;
}


// ==== FUN_004a21b0 @ 004a21b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a21b0(void *this)

{
  double *pdVar1;
  undefined4 uVar2;
  ulonglong uVar3;
  undefined4 *puVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined4 unaff_EBX;
  float10 fVar8;
  char *pcStack_28;
  char *pcStack_24;
  undefined1 *puStack_20;
  ulonglong uStack_10;
  undefined1 auStack_c [4];
  int iStack_8;
  undefined4 local_4;
  
  uStack_10 = CONCAT44(auStack_c,unaff_EBX);
  if (*(short *)((int)this + 0x2c) == 0) {
    puStack_20 = (undefined1 *)0x2db;
    pcStack_24 = s__ns_o_GunAnim_cpp_00537c94;
    pcStack_28 = s_getTargetSid______0_00537ca8;
    FUN_00490040();
  }
  local_4 = 0;
  iVar7 = (uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc;
  if ((*(byte *)(iVar7 + 0xb) & 3) == 0) {
    uVar2 = *(undefined4 *)(iVar7 + 0x12);
    puStack_20 = auStack_c;
    *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar7 + 0xe);
    *(undefined4 *)((int)this + 0x10) = uVar2;
    pcStack_24 = (char *)0x4a2213;
    puVar4 = (undefined4 *)FUN_004aef30();
    uVar2 = puVar4[1];
    *(undefined4 *)((int)this + 0x14) = *puVar4;
    *(undefined4 *)((int)this + 0x18) = uVar2;
  }
  else {
    local_4 = 1;
  }
  pdVar1 = (double *)((int)this + 0x30);
  puStack_20 = (undefined1 *)0x4a2234;
  iVar7 = FUN_0048f870(this);
  if (*pdVar1 <= _DAT_005484a8) {
    puStack_20 = (undefined1 *)(uint)*(byte *)(iVar7 + 0x22);
    pcStack_24 = (char *)0x4a2256;
    uVar5 = (**(code **)(*(int *)this + 0x48))();
    uStack_10 = CONCAT44(auStack_c,uVar5);
    pcStack_24 = (char *)(uVar5 & 0x3fff);
    pcStack_28 = (char *)0x4a2267;
    FUN_004a12b0(this);
    uVar3 = uStack_10 & 0x4000;
    if ((uStack_10 & 0x4000) != 0) {
      pcStack_24 = (char *)0x4a227a;
      iVar6 = FUN_004a12f0(this);
      if ((iVar6 == 0) && (iStack_8 == 0)) {
        return;
      }
    }
    if ((uStack_10 & 0x8000) != 0) {
      pcStack_24 = (char *)0x7;
      pcStack_28 = (char *)0x6;
      FUN_004a11a0(this);
      return;
    }
    if ((uVar3 != 0) && (*(byte *)(iVar7 + 10) == DAT_0051cb50)) {
      uStack_10 = CONCAT44(auStack_c,&pcStack_28);
      FUN_00436e50(&pcStack_28,0);
      FUN_004a3f20((int)this + 0x20);
      pcStack_24 = (char *)0x4a22d5;
      fVar8 = (float10)(**(code **)(*(int *)this + 0x30))();
      *pdVar1 = (double)(fVar8 * (float10)_DAT_00505338 + (float10)_DAT_005484a8);
      return;
    }
    uStack_10 = CONCAT44(auStack_c,&pcStack_28);
    FUN_00436e50(&pcStack_28,0);
    FUN_004a3f20((int)this + 0x20);
    pcStack_24 = (char *)0x4a231b;
    fVar8 = (float10)(**(code **)(*(int *)this + 0x30))();
    *pdVar1 = (double)(fVar8 + (float10)_DAT_005484a8);
  }
  return;
}


// ==== FUN_004a24b0 @ 004a24b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004a24b0(void *this)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  bool bVar7;
  float10 fVar8;
  undefined4 uStack_24;
  undefined4 **ppuStack_20;
  undefined4 *local_8 [2];
  
  if (*(double *)((int)this + 0x30) <= _DAT_005484a8) {
    ppuStack_20 = (undefined4 **)0x4a24d4;
    iVar2 = FUN_0048f870();
    iVar5 = (uint)*(ushort *)((int)this + 0x2c) * 0x24 + DAT_005395dc;
    bVar7 = (*(byte *)(iVar5 + 0xb) & 3) != 0;
    if (!bVar7) {
      uVar1 = *(undefined4 *)(iVar5 + 0x12);
      *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar5 + 0xe);
      ppuStack_20 = local_8;
      *(undefined4 *)((int)this + 0x10) = uVar1;
      uStack_24 = 0x4a250f;
      puVar3 = (undefined4 *)FUN_004aef30();
      uVar1 = puVar3[1];
      *(undefined4 *)((int)this + 0x14) = *puVar3;
      *(undefined4 *)((int)this + 0x18) = uVar1;
    }
    ppuStack_20 = (undefined4 **)0x4a252c;
    iVar5 = FUN_004abf50(iVar2);
    iVar5 = *(int *)(iVar5 + 0x110);
    ppuStack_20 = (undefined4 **)0x4a253b;
    iVar4 = FUN_004abf50(iVar2);
    iVar5 = *(int *)(iVar4 + 0x114) / iVar5;
    uVar6 = (uint)*(byte *)(iVar2 + 0x22);
    if ((int)((longlong)(ulonglong)uVar6 % (longlong)iVar5) == 0) {
      ppuStack_20 = (undefined4 **)(iVar5 + uVar6 + -1);
      uStack_24 = 0x4a2565;
      FUN_004a12b0(this);
      ppuStack_20 = (undefined4 **)0x4a256c;
      iVar2 = FUN_004a12f0(this);
      if ((iVar2 != 0) || (bVar7)) {
        ppuStack_20 = (undefined4 **)0x7;
        uStack_24 = 6;
        FUN_004a11a0(this);
        return;
      }
    }
    else {
      ppuStack_20 = (undefined4 **)(uVar6 - 1);
      uStack_24 = 0x4a2590;
      FUN_004a12b0(this);
      local_8[0] = &uStack_24;
      FUN_00436e50(&uStack_24,0);
      FUN_004a3f20((int)this + 0x20);
      ppuStack_20 = (undefined4 **)0x4a25b1;
      fVar8 = (float10)(**(code **)(*(int *)this + 0x30))();
      *(double *)((int)this + 0x30) = (double)(fVar8 + (float10)_DAT_005484a8);
    }
  }
  return;
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


