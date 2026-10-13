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


// ==== FUN_004abb00 @ 004abb00

void __thiscall FUN_004abb00(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == DAT_0051ca8c) {
    FUN_00448c10(((int)this - DAT_005395dc) / 0x24);
  }
  return;
}


// ==== FUN_004ac9c0 @ 004ac9c0

int __thiscall FUN_004ac9c0(void *this,uint param_2,uint param_3)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_4 = 0;
  local_10[0] = &PTR_LAB_00500d40;
  FUN_004eb960(local_10,((int)this - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    uVar1 = (uint)*(byte *)(local_8 * 0x24 + 10 + DAT_005395dc);
    if ((DAT_0051ca18 == uVar1) || (DAT_0051ca54 == uVar1)) {
      uVar2 = FUN_004eb9f0(local_10);
      iVar3 = FUN_004af4e0(uVar2);
      if ((*(uint *)(iVar3 + 0x18) & param_2) == param_3) {
        return iVar3;
      }
    }
    FUN_004eb990(local_10);
  } while( true );
}


// ==== FUN_004eb310 @ 004eb310

uint __cdecl FUN_004eb310(int param_1,uint param_2,int param_3,int param_4)

{
  int *piVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  
  uVar2 = *(ushort *)(param_1 * 0x24 + 6 + DAT_005395dc);
  iVar5 = DAT_005395dc;
  do {
    uVar3 = (uint)uVar2;
    if (uVar3 == 0) {
      return 0;
    }
    piVar1 = (int *)(uVar3 * 0x24 + iVar5);
    if ((*(byte *)(uVar3 * 0x24 + 10 + iVar5) == param_2) &&
       ((*(byte *)((int)piVar1 + 0xb) & 2) == 0)) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return uVar3;
        }
        if ((DAT_00540a28 != 0) ||
           ((iVar4 = ((int)piVar1 - iVar5) / 0x24, 4 < iVar4 && (iVar4 < 6000)))) {
          (**(code **)(*piVar1 + 0x10))(0);
          iVar5 = DAT_005395dc;
        }
      }
      param_3 = param_3 + -1;
    }
    uVar2 = *(ushort *)(piVar1 + 1);
  } while( true );
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


// ==== FUN_00406ef0 @ 00406ef0

undefined4 __cdecl FUN_00406ef0(undefined4 param_1,undefined4 param_2,int *param_3,uint param_4)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  float10 fVar7;
  float local_10;
  
  piVar2 = param_3;
  local_10 = 9999.0;
  iVar4 = 0;
  uVar5 = 0;
  if (0 < param_3[2]) {
    iVar6 = 0;
    iVar3 = DAT_005395dc;
    do {
      iVar1 = *(int *)(*piVar2 + iVar6) * 0x24 + iVar3;
      if (*(byte *)(iVar1 + 0x20) == param_4) {
        fVar7 = (float10)FUN_0043f750(&param_1,*(undefined4 *)(iVar1 + 0xe),
                                      *(undefined4 *)(iVar1 + 0x12));
        iVar3 = DAT_005395dc;
        if (fVar7 < (float10)local_10) {
          uVar5 = *(undefined4 *)(*piVar2 + iVar6);
          local_10 = (float)fVar7;
        }
      }
      iVar6 = iVar6 + 4;
      iVar4 = iVar4 + 1;
    } while (iVar4 < piVar2[2]);
  }
  return uVar5;
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


// ==== FUN_0047d150 @ 0047d150

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * __cdecl FUN_0047d150(int param_1,float param_2,float param_3)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  undefined4 uVar5;
  float *pfVar6;
  float unaff_EBX;
  uint uVar7;
  int *piVar8;
  int unaff_retaddr;
  float local_1c;
  undefined4 uStack_18;
  
  if (param_2 <= _DAT_00503ca8) {
    param_2 = _DAT_00503ca8;
  }
  if (param_3 <= _DAT_00503ca8) {
    param_3 = _DAT_00503ca8;
  }
  if (_DAT_00503cac <= param_2) {
    param_2 = _DAT_00503cac;
  }
  if (_DAT_00503cac <= param_3) {
    param_3 = _DAT_00503cac;
  }
  piVar8 = (int *)(DAT_005395dc + param_1 * 0x24);
  uVar7 = (uint)*(byte *)((int)piVar8 + 10);
  FUN_004abae0(piVar8);
  iVar2 = FUN_0047d020(uVar7);
  bVar1 = false;
  if (((DAT_0051ca5c == uVar7) || (DAT_0051cae8 == uVar7)) || (DAT_0051cb5c == uVar7)) {
    bVar1 = true;
  }
  else if ((DAT_0052da98 != 0) && (uVar7 == DAT_0051cbbc)) {
    FUN_00440a80(&param_2);
    param_2 = param_2 - _DAT_00503ca4;
    param_3 = param_3 - _DAT_00503ca4;
  }
  local_1c = 0.0;
  if (DAT_00540a2c != 0) {
    iVar3 = FUN_004eb310(param_1,iVar2,0,0);
    if (iVar3 != 0) {
      iVar3 = FUN_004af4e0(iVar3 * 0x24 + DAT_005395dc);
      local_1c = *(float *)(iVar3 + 0x2f10);
      FUN_0048f7b0(iVar3,0);
    }
  }
  if (iVar2 == 0) {
    FUN_00490040(s_processType_0052dae4,s__Ns_o_pathprocess_cpp_0052dacc,0xb3);
  }
  iVar3 = 0;
  piVar4 = (int *)(**(code **)(iVar2 * 0x1d4 + 0x1b0 + DAT_0051c960))(0,0);
  iVar2 = *(int *)((int)piVar8 + 0x12);
  (**(code **)(*piVar4 + 0x20))(param_1,param_2,param_3,*(float *)((int)piVar8 + 0xe));
  piVar4[0xbc2] = 0;
  piVar4[0xbc3] = (int)unaff_EBX;
  piVar4[0xbc4] = iVar2;
  piVar4[0xbc5] = unaff_retaddr;
  piVar4[0xbc7] = param_1;
  piVar4[0xbc8] = (int)param_2;
  if (piVar4[4] < 1) {
    if (piVar4 != (int *)0x0) {
      (**(code **)*piVar4)(1);
    }
    return (int *)0x0;
  }
  if (bVar1) {
    unaff_EBX = *(float *)((int)piVar8 + 0xe);
    local_1c = *(float *)((int)piVar8 + 0x12);
    FUN_00440a80(&stack0xffffffe0);
    if (0 < piVar4[4]) {
      pfVar6 = (float *)(piVar4 + 5);
      do {
        if ((*pfVar6 != unaff_EBX) || (pfVar6[1] != local_1c)) break;
        *pfVar6 = unaff_EBX;
        iVar3 = iVar3 + 1;
        pfVar6[1] = local_1c;
        pfVar6 = pfVar6 + 2;
      } while (iVar3 < piVar4[4]);
    }
  }
  uVar5 = FUN_004abf50(piVar8);
  (**(code **)(*piVar4 + 0x30))(0,uVar5);
  (**(code **)(*piVar4 + 0x2c))(uStack_18,0,unaff_EBX);
  (**(code **)(*piVar8 + 0x44))(unaff_EBX,local_1c,piVar4);
  if (piVar4 != (int *)0x0) {
    (**(code **)(*piVar8 + 0x40))();
  }
  return piVar4;
}


// ==== FUN_004ae4b0 @ 004ae4b0

int __thiscall FUN_004ae4b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)((int)this + 0x22) * 4) + -0x41;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004acbc0 @ 004acbc0

void __thiscall FUN_004acbc0(void *this,uint param_2,uint param_3)

{
  char cVar1;
  int iVar2;
  
  if ((param_3 != 0) && (param_3 != 0x2000)) {
    FUN_00490040(s_popFlags____0____popFlags____pfA_00539f00,s__ns_o_Squid_cpp_00539ef0,0x4f9);
  }
  cVar1 = *(char *)((int)this + 0x1f);
  iVar2 = FUN_004acb00(this);
  if (iVar2 != cVar1) {
    if (*(byte *)((int)this + 0x22) != param_2) {
      (**(code **)(*(int *)this + 0x48))(param_3);
      *(undefined1 *)((int)this + 0x22) = (undefined1)param_2;
      (**(code **)(*(int *)this + 0x4c))(param_3 | 0x50);
    }
    return;
  }
  if ((param_3 & 0x2000) == 0) {
    (**(code **)(*(int *)this + 0x88))();
  }
  else {
    (**(code **)(*(int *)this + 0x8c))();
  }
  *(undefined1 *)((int)this + 0x22) = (undefined1)param_2;
  if ((param_3 & 0x2000) != 0) {
    (**(code **)(*(int *)this + 0x8c))();
    return;
  }
  (**(code **)(*(int *)this + 0x88))();
  return;
}


// ==== FUN_004aed90 @ 004aed90

undefined4 FUN_004aed90(void)

{
  return 0;
}


