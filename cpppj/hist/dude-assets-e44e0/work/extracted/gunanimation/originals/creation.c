// ==== FUN_0040b380 @ 0040b380

void FUN_0040b380(void)

{
  return;
}


// ==== FUN_004429e0 @ 004429e0

undefined4 __cdecl FUN_004429e0(void)

{
  return 0;
}


// ==== FUN_004437f0 @ 004437f0

void __cdecl FUN_004437f0(void)

{
  return;
}


// ==== FUN_00466a20 @ 00466a20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00466a20(void)

{
  FUN_004e0620("false","\\Ns\\O\\Gunanim.cpp",0x48);
  return (float10)_DAT_00500fc4;
}


// ==== FUN_00466a40 @ 00466a40

undefined4 __cdecl FUN_00466a40(void)

{
  return DAT_00541198;
}


// ==== FUN_00466a50 @ 00466a50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00466a50(void)

{
  FUN_004e0620("false","\\Ns\\O\\Gunanim.cpp",0x4b);
  return (float10)_DAT_00500fc4;
}


// ==== FUN_00466a70 @ 00466a70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00466a70(void)

{
  FUN_004e0620("false","\\Ns\\O\\Gunanim.cpp",0x4c);
  return (float10)_DAT_00500fc4;
}


// ==== FUN_00466a90 @ 00466a90

undefined4 FUN_00466a90(void)

{
  FUN_004e0620("false","\\Ns\\O\\Gunanim.cpp",0x4e);
  return 0;
}


// ==== FUN_00466ab0 @ 00466ab0

int __fastcall FUN_00466ab0(int param_1)

{
  return param_1 + 0x2a;
}


// ==== FUN_00466e50 @ 00466e50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00466e50(void *this,undefined4 param_2)

{
  undefined8 *puVar1;
  ushort *puVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  (**(code **)(*(int *)this + 0x3c))(param_2);
  (**(code **)(*(int *)this + 0x40))(param_2);
  puVar1 = (undefined8 *)(**(code **)(*(int *)this + 0x44))();
  *puVar1 = _DAT_0055b4d0;
  puVar2 = (ushort *)(**(code **)(*(int *)this + 0x38))();
  uVar5 = 0x10;
  uVar4 = 0;
  uVar3 = (*(code *)**(undefined4 **)((uint)*puVar2 * 0x32 + DAT_005c8464))(0,0x10);
  FUN_0041bd20(this,param_2,uVar3,uVar4,uVar5);
  return;
}


// ==== FUN_00466eb0 @ 00466eb0

void __thiscall FUN_00466eb0(void *this,undefined4 param_2,undefined4 param_3)

{
  ushort *puVar1;
  int *piVar2;
  int iVar3;
  
  puVar1 = (ushort *)(**(code **)(*(int *)this + 0x38))();
  piVar2 = (int *)FUN_004ae2d0((uint)*puVar1 * 0x32 + DAT_005c8464);
  iVar3 = FUN_0041bff0(piVar2);
  if (iVar3 != DAT_005410f8) {
    FUN_004e0620("gunProcess->getProcessType() == gunProcessType","\\Ns\\O\\Gunanim.cpp",0x1d0);
  }
  (**(code **)(*piVar2 + 0x1c))(1,param_2,param_3);
  FUN_0041bf10(this,0x10);
  return;
}


// ==== FUN_00466f20 @ 00466f20

void __thiscall FUN_00466f20(void)

{
  int *piVar1;
  int unaff_retaddr;
  
  piVar1 = (int *)FUN_0041bfb0();
  (**(code **)(*piVar1 + 0x48))(0);
  piVar1[9] = unaff_retaddr;
  (**(code **)(*piVar1 + 0x90))
            (*(undefined4 *)((int)piVar1 + 0xe),*(undefined4 *)((int)piVar1 + 0x12),0x40);
  return;
}


// ==== FUN_00466f50 @ 00466f50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00466f50(void *this)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  void *pvVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  ushort *puVar8;
  int iVar9;
  float10 fVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  undefined4 uVar15;
  undefined4 local_14;
  undefined4 local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fa6cb;
  local_c = ExceptionList;
  iVar2 = *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
  iVar9 = (uint)*(ushort *)((int)this + 0x2c) * 0x32 + DAT_005c8464;
  if ((*(byte *)(iVar9 + 0xb) & 3) != 0) {
    return 0;
  }
  ExceptionList = &local_c;
  uVar3 = FUN_004ac1e0(iVar9);
  if ((uVar3 & 0x10) == 0) {
    ExceptionList = local_c;
    return 0;
  }
  iVar4 = FUN_0041bfb0(this);
  FUN_004571b0(&local_14,(iVar4 - DAT_005c8464) / 0x32,*(undefined2 *)((int)this + 0x2c));
  if (*(byte *)(iVar2 + 10) != DAT_005412a8) {
    pvVar5 = operator_new(0x80);
    local_4 = 0;
    if (pvVar5 != (void *)0x0) {
      uVar1 = *(undefined4 *)((int)this + 8);
      uVar12 = *(undefined4 *)((int)this + 0x10);
      uVar13 = 0;
      uVar11 = *(undefined4 *)((int)this + 0xc);
      uVar14 = DAT_00545370;
      uVar15 = DAT_00545374;
      uVar6 = (**(code **)(*(int *)this + 0x28))
                        (uVar11,uVar12,local_14,local_10,0,DAT_00545370,DAT_00545374);
      uVar7 = (**(code **)(*(int *)this + 0x24))(uVar6);
      fVar10 = (float10)(**(code **)(*(int *)this + 0x20))(uVar7);
      FUN_00481a50(pvVar5,uVar1,*(undefined2 *)((int)this + 0x2c),(float)fVar10,uVar7,uVar6,uVar11,
                   uVar12,local_14,local_10,uVar13,uVar14,uVar15);
    }
    local_4 = 0xffffffff;
    puVar8 = (ushort *)(**(code **)(*(int *)this + 0x38))();
    iVar2 = FUN_004ae2d0((uint)*puVar8 * 0x32 + DAT_005c8464);
    if (iVar2 == 0) {
      iVar2 = 0;
    }
    else {
      iVar2 = iVar2 + -0x28;
    }
    iVar2 = FUN_0041bff0(iVar2 + 0x28);
    if (iVar2 == DAT_005410f8) {
      ExceptionList = local_c;
      return 1;
    }
    FUN_004e0620("gunProcess->getProcessType() == gunProcessType","\\Ns\\O\\Gunanim.cpp",0x22b);
    ExceptionList = local_c;
    return 1;
  }
  FUN_00472700(*(undefined4 *)((int)this + 8),*(undefined2 *)((int)this + 0x2c));
  if (((*(byte *)(iVar9 + 0xb) & 3) != 0) ||
     ((*(byte *)(iVar9 + 10) == DAT_005412a0 &&
      (fVar10 = (float10)FUN_00455930(0,0x447a0000), fVar10 < (float10)_DAT_00540bbc)))) {
    FUN_004d5000(iVar2);
  }
  else {
    FUN_004d4f00(iVar2,*(undefined2 *)((int)this + 0x2c));
  }
  if ((*(byte *)(iVar9 + 0xb) & 3) != 0) {
    ExceptionList = local_c;
    return 0;
  }
  ExceptionList = local_c;
  return 1;
}


// ==== FUN_00467130 @ 00467130

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00467130(void *this)

{
  double dVar1;
  int iVar2;
  undefined8 *puVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  int *piVar7;
  float10 fVar8;
  undefined8 uVar9;
  undefined4 uVar10;
  undefined1 local_10 [8];
  undefined1 local_8 [8];
  
  if (*(short *)((int)this + 0x2c) == 0) {
    FUN_004e0620();
  }
  iVar2 = FUN_0041bfb0(this);
  if (*(double *)((int)this + 0x30) <= _DAT_0055b4d0) {
    puVar3 = (undefined8 *)
             FUN_004adea0((uint)*(ushort *)((int)this + 0x2c) * 0x32 + DAT_005c8464,local_10);
    uVar9 = *puVar3;
    uVar10 = 1;
    uVar4 = FUN_004adea0(iVar2,local_8);
    iVar5 = FUN_0041ce90(uVar4,uVar9,uVar10);
    uVar4 = *(undefined4 *)(iVar2 + 0x24);
    iVar5 = iVar5 / 2;
    uVar6 = (**(code **)(*(int *)this + 0x34))();
    if ((char)(uVar6 >> 8) < '\0') {
      FUN_00466eb0(this,3,4);
      return;
    }
    piVar7 = (int *)FUN_0041bfb0(this);
    (**(code **)(*piVar7 + 0x48))(0,uVar4,iVar5);
    piVar7[9] = uVar6 & 0x3fff;
    (**(code **)(*piVar7 + 0x90))
              (*(undefined4 *)((int)piVar7 + 0xe),*(undefined4 *)((int)piVar7 + 0x12),0x40);
    dVar1 = _DAT_0055b4d0;
    uVar4 = SUB84(_DAT_0055b4d0,0);
    *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
    *(undefined4 *)((int)this + 0x30) = uVar4;
    *(int *)((int)this + 0x34) = (int)((ulonglong)dVar1 >> 0x20);
    fVar8 = (float10)(**(code **)(*(int *)this + 0x2c))();
    *(double *)((int)this + 0x30) = (double)(fVar8 + (float10)_DAT_0055b4d0);
  }
  return;
}


// ==== FUN_00467240 @ 00467240

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00467240(void *this)

{
  ulonglong uVar1;
  double dVar2;
  int iVar3;
  uint uVar4;
  int *piVar5;
  float10 fVar6;
  undefined4 uVar7;
  
  iVar3 = FUN_0041bfb0();
  if (*(double *)((int)this + 0x30) <= _DAT_0055b4d0) {
    uVar4 = (**(code **)(*(int *)this + 0x34))
                      (*(undefined4 *)(iVar3 + 0x24),*(int *)((int)this + 0x38) / 2);
    if ((char)(uVar4 >> 8) < '\0') {
      if (*(int *)((int)this + 0x38) == 8) {
        FUN_00466eb0(this,9,10);
        return;
      }
      *(int *)((int)this + 0x38) = *(int *)((int)this + 0x38) + 2;
      dVar2 = _DAT_0055b4d0;
      uVar1 = (ulonglong)_DAT_0055b4d0 >> 0x20;
      *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
      *(int *)((int)this + 0x30) = SUB84(dVar2,0);
      *(int *)((int)this + 0x34) = (int)uVar1;
    }
    else {
      piVar5 = (int *)FUN_0041bfb0(this);
      (**(code **)(*piVar5 + 0x48))(0);
      piVar5[9] = uVar4 & 0x3fff;
      (**(code **)(*piVar5 + 0x90))
                (*(undefined4 *)((int)piVar5 + 0xe),*(undefined4 *)((int)piVar5 + 0x12),0x40);
      dVar2 = _DAT_0055b4d0;
      uVar7 = SUB84(_DAT_0055b4d0,0);
      *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
      *(undefined4 *)((int)this + 0x30) = uVar7;
      *(int *)((int)this + 0x34) = (int)((ulonglong)dVar2 >> 0x20);
    }
    fVar6 = (float10)(**(code **)(*(int *)this + 0x2c))();
    *(double *)((int)this + 0x30) = (double)(fVar6 + (float10)_DAT_0055b4d0);
  }
  return;
}


// ==== FUN_00467330 @ 00467330

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00467330(void *this)

{
  undefined4 *puVar1;
  uint uVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined1 local_8 [8];
  
  if (*(short *)((int)this + 0x2c) == 0) {
    FUN_004e0620("getTargetSid() != 0","\\Ns\\O\\Gunanim.cpp",0x2e2);
  }
  iVar5 = (uint)*(ushort *)((int)this + 0x2c) * 0x32;
  iVar6 = iVar5 + DAT_005c8464;
  if ((*(byte *)(iVar5 + 0xb + DAT_005c8464) & 3) == 0) {
    *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar6 + 0xe);
    *(undefined4 *)((int)this + 0x10) = *(undefined4 *)(iVar6 + 0x12);
    puVar1 = (undefined4 *)FUN_004adea0(iVar6,local_8);
    *(undefined4 *)((int)this + 0x14) = *puVar1;
    *(undefined4 *)((int)this + 0x18) = puVar1[1];
  }
  iVar5 = FUN_0041bfb0(this);
  if (*(double *)((int)this + 0x30) <= _DAT_0055b4d0) {
    iVar6 = *(int *)(iVar5 + 0x24);
    uVar2 = (**(code **)(*(int *)this + 0x48))();
    piVar3 = (int *)FUN_0041bfb0(this);
    (**(code **)(*piVar3 + 0x48))(0);
    piVar3[9] = uVar2 & 0x3fff;
    (**(code **)(*piVar3 + 0x90))
              (*(undefined4 *)((int)piVar3 + 0xe),*(undefined4 *)((int)piVar3 + 0x12),0x40);
    if ((((uVar2 & 0x4000) == 0) || (iVar4 = FUN_00466f50(this), iVar4 != 0)) || (iVar6 != 0)) {
      if ((char)(uVar2 >> 8) < '\0') {
        FUN_00466eb0(this,6,7);
        return;
      }
      uVar8 = SUB84(_DAT_0055b4d0,0);
      uVar9 = (undefined4)((ulonglong)_DAT_0055b4d0 >> 0x20);
      if (((uVar2 & 0x4000) != 0) && (*(byte *)(iVar5 + 10) == DAT_00541264)) {
        *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
        *(undefined4 *)((int)this + 0x34) = uVar9;
        *(undefined4 *)((int)this + 0x30) = uVar8;
        fVar7 = (float10)(**(code **)(*(int *)this + 0x30))();
        *(double *)((int)this + 0x30) =
             (double)(fVar7 * (float10)_DAT_00501550 + (float10)_DAT_0055b4d0);
        return;
      }
      *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
      *(undefined4 *)((int)this + 0x34) = uVar9;
      *(undefined4 *)((int)this + 0x30) = uVar8;
      fVar7 = (float10)(**(code **)(*(int *)this + 0x30))();
      *(double *)((int)this + 0x30) = (double)(fVar7 + (float10)_DAT_0055b4d0);
    }
  }
  return;
}


// ==== FUN_004674c0 @ 004674c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004674c0(int *param_1)

{
  ulonglong uVar1;
  double dVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int unaff_EBP;
  float10 fVar7;
  
  if (*(double *)(param_1 + 0xc) <= _DAT_0055b4d0) {
    iVar3 = FUN_0041bfb0();
    iVar6 = (uint)*(ushort *)(param_1 + 0xb) * 0x32 + DAT_005c8464;
    if ((*(byte *)(iVar6 + 0xb) & 3) == 0) {
      param_1[3] = *(int *)(iVar6 + 0xe);
      param_1[4] = *(int *)(iVar6 + 0x12);
      piVar4 = (int *)FUN_004adea0();
      param_1[5] = *piVar4;
      param_1[6] = piVar4[1];
    }
    iVar6 = FUN_004ac550(iVar3);
    iVar6 = *(int *)(iVar6 + 0x110);
    iVar5 = FUN_004ac550(iVar3);
    iVar6 = *(int *)(iVar5 + 0x114) / iVar6;
    iVar3 = *(int *)(iVar3 + 0x24);
    if (iVar3 % iVar6 == 0) {
      piVar4 = (int *)FUN_0041bfb0(param_1);
      (**(code **)(*piVar4 + 0x48))(0);
      piVar4[9] = iVar3 + -1 + iVar6;
      (**(code **)(*piVar4 + 0x90))
                (*(undefined4 *)((int)piVar4 + 0xe),*(undefined4 *)((int)piVar4 + 0x12),0x40);
      iVar3 = FUN_00466f50(param_1);
      if ((iVar3 != 0) || (unaff_EBP != 0)) {
        FUN_00466eb0(param_1,6,7);
        return;
      }
    }
    else {
      piVar4 = (int *)FUN_0041bfb0(param_1);
      (**(code **)(*piVar4 + 0x48))(0);
      piVar4[9] = iVar3 + -1;
      (**(code **)(*piVar4 + 0x90))
                (*(undefined4 *)((int)piVar4 + 0xe),*(undefined4 *)((int)piVar4 + 0x12),0x40);
      dVar2 = _DAT_0055b4d0;
      uVar1 = (ulonglong)_DAT_0055b4d0 >> 0x20;
      *(byte *)(param_1 + 10) = *(byte *)(param_1 + 10) | 8;
      param_1[0xd] = (int)uVar1;
      param_1[0xc] = SUB84(dVar2,0);
      fVar7 = (float10)(**(code **)(*param_1 + 0x30))();
      *(double *)(param_1 + 0xc) = (double)(fVar7 + (float10)_DAT_0055b4d0);
    }
  }
  return;
}


// ==== FUN_00467610 @ 00467610

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __cdecl FUN_00467610(int param_1)

{
  float fVar1;
  
  param_1 = param_1 / 5;
  if (0x12 < param_1) {
    param_1 = 0x13;
  }
  if ((DAT_005e4794 != 0) && ((param_1 < 0 || (0x13 < param_1)))) {
    FUN_004e0620("0 <= index && index < 20","\\Ns\\O\\Gunanim.cpp",0x387);
  }
  fVar1 = (float)(&DAT_00540940)[param_1];
  if (_DAT_0050229c <= fVar1) {
    if (fVar1 < _DAT_0050af94) {
      return 1;
    }
    if (fVar1 < _DAT_0050af90) {
      return 2;
    }
    if (fVar1 < _DAT_0050af8c) {
      return 3;
    }
  }
  return 0;
}


// ==== FUN_004676c0 @ 004676c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl FUN_004676c0(float param_1)

{
  int iVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  
  iVar4 = 0;
  pfVar5 = (float *)&DAT_00540944;
  iVar3 = 2;
  fVar2 = _DAT_0050afb4;
  do {
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[-1] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + -2;
      fVar2 = pfVar5[-1] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(*pfVar5 - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + -1;
      fVar2 = *pfVar5 - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[1] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3;
      fVar2 = pfVar5[1] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[2] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 1;
      fVar2 = pfVar5[2] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[3] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 2;
      fVar2 = pfVar5[3] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[4] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 3;
      fVar2 = pfVar5[4] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[5] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 4;
      fVar2 = pfVar5[5] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[6] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 5;
      fVar2 = pfVar5[6] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[7] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 6;
      fVar2 = pfVar5[7] - param_1;
    }
    if (_DAT_00500fc4 <= fVar2) break;
    if (ABS(pfVar5[8] - param_1) < ABS(fVar2)) {
      iVar4 = iVar3 + 7;
      fVar2 = pfVar5[8] - param_1;
    }
    iVar1 = iVar3 + 8;
    pfVar5 = pfVar5 + 10;
    iVar3 = iVar3 + 10;
  } while (iVar1 < 0x14);
  if ((iVar4 == 0x13) && (ABS(_DAT_005022d0 - param_1) < ABS(fVar2))) {
    return 0;
  }
  return iVar4;
}


// ==== FUN_00467940 @ 00467940

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00467940(void *this)

{
  ulonglong uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  undefined4 uVar9;
  int unaff_EBX;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;
  float10 fVar10;
  undefined4 uVar11;
  undefined8 uVar12;
  int local_110 [2];
  int local_108 [66];
  
  uVar12 = CONCAT44(unaff_ESI,unaff_EDI);
  iVar2 = FUN_0041bfb0();
  iVar3 = FUN_004ac550(iVar2,uVar12);
  iVar3 = *(int *)(iVar3 + 0x110);
  iVar4 = FUN_004ac550(iVar2);
  local_110[0] = *(int *)(iVar4 + 0x114) / iVar3;
  iVar4 = *(int *)(iVar2 + 0x24) / local_110[0];
  if (*(ushort *)((int)this + 0x2c) == 0) {
    local_108[0] = FUN_004ad390(iVar2);
    fVar10 = (float10)local_108[0] * (float10)_DAT_0050afd8;
  }
  else {
    iVar6 = (uint)*(ushort *)((int)this + 0x2c) * 0x32;
    iVar7 = iVar6 + DAT_005c8464;
    if ((*(byte *)(iVar6 + 0xb + DAT_005c8464) & 3) == 0) {
      *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar7 + 0xe);
      *(undefined4 *)((int)this + 0x10) = *(undefined4 *)(iVar7 + 0x12);
      puVar5 = (undefined4 *)FUN_004adea0(iVar7,local_108);
      *(undefined4 *)((int)this + 0x14) = *puVar5;
      *(undefined4 *)((int)this + 0x18) = puVar5[1];
    }
    uVar9 = *(undefined4 *)((int)this + 0x18);
    uVar11 = *(undefined4 *)((int)this + 0x14);
    FUN_004adea0(iVar2,local_108);
    fVar10 = (float10)FUN_0041ce20(uVar11,uVar9);
  }
  iVar6 = FUN_004676c0((float)fVar10);
  if (*(double *)((int)this + 0x30) <= _DAT_0055b4d0) {
    if (iVar4 == iVar6) {
      FUN_00466eb0(this,3,4);
      return;
    }
    if (iVar4 < iVar6) {
      iVar7 = (iVar4 - iVar6) + iVar3;
    }
    else {
      iVar7 = iVar4 - iVar6;
    }
    if (iVar6 < iVar4) {
      iVar6 = (iVar6 - iVar4) + iVar3;
    }
    else {
      iVar6 = iVar6 - iVar4;
    }
    iVar6 = (uint)(iVar6 < iVar7) * 2 + -1;
    if ((iVar4 == 0) && (iVar6 == -1)) {
      iVar3 = iVar3 + -1;
    }
    else {
      iVar3 = (iVar6 + iVar4) % iVar3;
    }
    piVar8 = (int *)FUN_0041bfb0(this);
    (**(code **)(*piVar8 + 0x48))(0);
    piVar8[9] = iVar3 * unaff_EBX;
    (**(code **)(*piVar8 + 0x90))
              (*(undefined4 *)((int)piVar8 + 0xe),*(undefined4 *)((int)piVar8 + 0x12),0x40);
    uVar9 = SUB84(_DAT_0055b4d0,0);
    uVar1 = (ulonglong)_DAT_0055b4d0 >> 0x20;
    *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 8;
    *(undefined4 *)((int)this + 0x30) = uVar9;
    *(int *)((int)this + 0x34) = (int)uVar1;
    fVar10 = (float10)(**(code **)(*(int *)this + 0x2c))();
    *(double *)((int)this + 0x30) = (double)(fVar10 + (float10)_DAT_0055b4d0);
    iVar3 = FUN_004558f0(1000,10000);
    uVar9 = FUN_004a89c0(local_110,"sunDiscThrowerRatchet0%d.wav",iVar3 / 1000);
    FUN_004a9d70(*(undefined4 *)(iVar2 + 0xe),*(undefined4 *)(iVar2 + 0x12),uVar9,0);
  }
  return;
}


// ==== FUN_00467b20 @ 00467b20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00467b20(void)

{
  int iVar1;
  float10 fVar2;
  
  iVar1 = FUN_0041bfb0();
  iVar1 = *(int *)(iVar1 + 0x24);
  if (0xf < iVar1) {
    iVar1 = iVar1 + -0x10;
  }
  fVar2 = (float10)iVar1 * (float10)_DAT_0050aff8 + (float10)_DAT_005022d8;
  if (((float10)_DAT_00500460 < fVar2 == ((float10)_DAT_00500460 == fVar2)) ||
     (fVar2 < (float10)_DAT_005022d8 == (fVar2 == (float10)_DAT_005022d8))) {
    FUN_004e0620("0.0 <= a && a <= 360.0","\\Ns\\O\\Gunanim.cpp",0x409);
    fVar2 = (float10)(double)fVar2;
  }
  return fVar2;
}


// ==== FUN_00467b90 @ 00467b90

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __thiscall FUN_00467b90(void *this)

{
  uint uVar1;
  float10 fVar2;
  
  if ((*(float *)((int)this + 0x48) != DAT_00540994) &&
     (*(float *)((int)this + 0x48) != -DAT_00540994)) {
    FUN_004e0620("angularAccel == initAngularAccel || angularAccel == -initAngularAccel",
                 "\\Ns\\O\\Gunanim.cpp",0x40f);
  }
  fVar2 = (float10)FUN_004e5e8a();
  fVar2 = (float10)_DAT_005022d8 - fVar2;
  if ((fVar2 <= (float10)_DAT_00500460) ||
     (fVar2 < (float10)_DAT_005022d8 == (fVar2 == (float10)_DAT_005022d8))) {
    FUN_004e0620("0.0 < a && a <= 360.0","\\Ns\\O\\Gunanim.cpp",0x412);
    fVar2 = (float10)(double)fVar2;
  }
  if ((float10)_DAT_005022d8 <= fVar2) {
    fVar2 = (float10)_DAT_0050b008;
  }
  uVar1 = FUN_0043fa50((float)(fVar2 * (float10)_DAT_0050b000));
  if (*(int *)((int)this + 0x4c) != 0) {
    uVar1 = uVar1 + 2 & 0x8000000f;
    if ((int)uVar1 < 0) {
      uVar1 = (uVar1 - 1 | 0xfffffff0) + 1;
    }
    return uVar1 + 0x10;
  }
  uVar1 = uVar1 & 0x8000000f;
  if ((int)uVar1 < 0) {
    uVar1 = (uVar1 - 1 | 0xfffffff0) + 1;
  }
  return uVar1;
}


// ==== FUN_00467c80 @ 00467c80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00467c80(void *this)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  bool bVar4;
  float10 fVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined8 uVar8;
  
  if (*(short *)((int)this + 0x2c) == 0) {
    FUN_004e0620("getTargetSid() != 0","\\Ns\\O\\Gunanim.cpp",0x42b);
  }
  if ((_DAT_0055b4e0 <= _DAT_00500460) || (_DAT_00506590 <= _DAT_0055b4e0)) goto LAB_00467ebb;
  iVar1 = FUN_0041bfb0(this);
  iVar3 = (uint)*(ushort *)((int)this + 0x2c) * 0x32 + DAT_005c8464;
  bVar4 = (*(byte *)(iVar3 + 0xb) & 3) == 0;
  if (bVar4) {
    *(undefined4 *)((int)this + 0xc) = *(undefined4 *)(iVar3 + 0xe);
    *(undefined4 *)((int)this + 0x10) = *(undefined4 *)(iVar3 + 0x12);
    puVar2 = (undefined4 *)FUN_004adea0();
    *(undefined4 *)((int)this + 0x14) = *puVar2;
    *(undefined4 *)((int)this + 0x18) = puVar2[1];
  }
  *(float *)((int)this + 0x40) =
       (float)_DAT_0055b4e0 * *(float *)((int)this + 0x44) + *(float *)((int)this + 0x40);
  *(float *)((int)this + 0x44) =
       (float)_DAT_0055b4e0 * *(float *)((int)this + 0x48) + *(float *)((int)this + 0x44);
  fVar5 = (float10)FUN_0041d7e0(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12),
                                *(undefined4 *)((int)this + 0x14),*(undefined4 *)((int)this + 0x18))
  ;
  fVar5 = ((float10)_DAT_00540998 * (float10)_DAT_005022d0 + fVar5) - (float10)_DAT_005409b0;
  if (*(int *)((int)this + 0x50) == 0) {
    if ((*(int *)((int)this + 0x4c) == 0) &&
       ((float)fVar5 - _DAT_005409a8 < *(float *)((int)this + 0x40) !=
        ((float)fVar5 - _DAT_005409a8 == *(float *)((int)this + 0x40)))) {
      *(undefined4 *)((int)this + 0x4c) = 1;
      *(float *)((int)this + 0x48) = -*(float *)((int)this + 0x48);
      FUN_004a9d70(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12),"sasprung.wav",0);
    }
    if (*(int *)((int)this + 0x50) != 0) goto LAB_00467e2c;
    if ((float)fVar5 <= *(float *)((int)this + 0x40)) {
      *(undefined4 *)((int)this + 0x4c) = 1;
      *(undefined4 *)((int)this + 0x50) = 1;
      *(ushort *)(iVar1 + 0xc) = *(ushort *)(iVar1 + 0xc) | 1;
      FUN_004214a0();
      iVar3 = FUN_00466f50(this);
      if ((iVar3 == 0) && (bVar4)) {
        return;
      }
      FUN_004a9d70(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12),"safired.wav",0);
    }
    if (*(int *)((int)this + 0x50) != 0) goto LAB_00467e2c;
  }
  else {
LAB_00467e2c:
    if ((*(int *)((int)this + 0x4c) != 0) &&
       (_DAT_005409ac + (float)fVar5 < *(float *)((int)this + 0x40) !=
        (_DAT_005409ac + (float)fVar5 == *(float *)((int)this + 0x40)))) {
      *(undefined4 *)((int)this + 0x4c) = 0;
    }
  }
  if ((_DAT_00500fc4 < *(float *)((int)this + 0x40)) &&
     (iVar3 = FUN_00467b90(this,(double)*(float *)((int)this + 0x40)),
     iVar3 != *(int *)(iVar1 + 0x24))) {
    FUN_00466f20(this);
    if (_DAT_00500fc4 <= *(float *)((int)this + 0x44)) {
      if (*(int *)(iVar1 + 0x24) != 2) goto LAB_00467ebb;
      uVar7 = *(undefined4 *)(iVar1 + 0x12);
      uVar6 = *(undefined4 *)(iVar1 + 0xe);
      uVar8 = 0x50b078;
    }
    else {
      uVar7 = *(undefined4 *)(iVar1 + 0x12);
      uVar6 = *(undefined4 *)(iVar1 + 0xe);
      uVar8 = 0x50b094;
    }
    FUN_004a9d70(uVar6,uVar7,uVar8);
  }
LAB_00467ebb:
  if ((((*(int *)((int)this + 0x50) != 0) &&
       (*(float *)((int)this + 0x44) < _DAT_005409a0 !=
        (*(float *)((int)this + 0x44) == _DAT_005409a0))) &&
      (fVar5 = (float10)FUN_0041cdb0(), ABS(fVar5 - (float10)_DAT_0054099c) < (float10)_DAT_005409a4
      )) || (*(float *)((int)this + 0x40) < _DAT_00500fc4)) {
    FUN_00466eb0(this,6,7);
  }
  return;
}


// ==== FUN_00467f20 @ 00467f20

void __thiscall FUN_00467f20(int param_1,undefined2 param_2)

{
  *(byte *)(param_1 + 0x28) = *(byte *)(param_1 + 0x28) | 2;
  *(undefined2 *)(param_1 + 0x2a) = param_2;
  return;
}


// ==== FUN_00467f30 @ 00467f30

void __thiscall FUN_00467f30(int param_1,undefined2 param_2)

{
  *(byte *)(param_1 + 0x28) = *(byte *)(param_1 + 0x28) | 4;
  *(undefined2 *)(param_1 + 0x2c) = param_2;
  return;
}


// ==== FUN_00467f40 @ 00467f40

void __thiscall FUN_00467f40(void *this,undefined4 param_2,int param_3,int param_4)

{
  undefined4 uVar1;
  
  if ((param_3 == 0) && (param_4 == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = *(undefined4 *)((int)this + 8);
  }
  FUN_00466ac0((int)this + 0x20,param_2,*(undefined4 *)((int)this + 4),uVar1,param_4);
  return;
}


// ==== FUN_00467f70 @ 00467f70

int __fastcall FUN_00467f70(int param_1)

{
  return param_1 + 0x30;
}


// ==== FUN_00467f80 @ 00467f80

void __thiscall FUN_00467f80(void *this,undefined4 param_2,int param_3,int param_4)

{
  undefined4 uVar1;
  
  if ((param_3 == 0) && (param_4 == 0)) {
    uVar1 = 0;
  }
  else {
    uVar1 = *(undefined4 *)((int)this + 8);
  }
  FUN_00466c20((int)this + 0x20,param_2,*(undefined4 *)((int)this + 4),uVar1,param_4);
  return;
}


// ==== FUN_00467fb0 @ 00467fb0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00467fb0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c4;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  *(undefined4 *)((int)this + 0x38) = 0;
  local_4 = 0;
  *(undefined ***)this = &PTR_FUN_0050b0d0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c8;
  FUN_00466e50(this,param_2,param_3,DAT_005410fc);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_00468040 @ 00468040

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468040(void)

{
  return (float10)_DAT_00504190;
}


// ==== FUN_00468050 @ 00468050

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00468050(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c4;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  *(undefined4 *)((int)this + 0x38) = 0;
  local_4 = 0;
  *(undefined ***)this = &PTR_FUN_0050b120;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b11c;
  FUN_00466e50(this,param_2,param_3,DAT_005410fc);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_004680d0 @ 004680d0

undefined4 __cdecl FUN_004680d0(void)

{
  return DAT_0054119c;
}


// ==== FUN_004680e0 @ 004680e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_004680e0(void)

{
  return (float10)_DAT_0050b168;
}


// ==== FUN_004680f0 @ 004680f0

void * __thiscall FUN_004680f0(void *this,undefined4 param_2,undefined4 param_3)

{
  FUN_00467fb0(this,param_2,param_3);
  *(undefined ***)this = &PTR_FUN_0050b170;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b16c;
  return this;
}


// ==== FUN_00468120 @ 00468120

undefined4 __cdecl FUN_00468120(void)

{
  return DAT_005410bc;
}


// ==== FUN_00468130 @ 00468130

uint FUN_00468130(uint param_1)

{
  return param_1 | 0xc000;
}


// ==== FUN_00468140 @ 00468140

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00468140(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  int iVar2;
  float10 fVar3;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c4;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined ***)this = &PTR_FUN_0050b1c0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b1bc;
  local_4 = 0;
  FUN_00466e50(this,param_2,param_3,DAT_005410fc);
  fVar3 = (float10)FUN_00467b20(this);
  *(float *)((int)this + 0x40) = (float)fVar3;
  *(undefined4 *)((int)this + 0x44) = DAT_00540990;
  *(undefined4 *)((int)this + 0x48) = DAT_00540994;
  *(undefined4 *)((int)this + 0x4c) = 0;
  *(undefined4 *)((int)this + 0x50) = 0;
  iVar2 = FUN_0041bfb0(this);
  *(byte *)(iVar2 + 0xc) = *(byte *)(iVar2 + 0xc) & 0xfe;
  iVar2 = FUN_0041bfb0(this);
  FUN_004214a0((iVar2 - DAT_005c8464) / 0x32);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_00468220 @ 00468220

undefined4 __cdecl FUN_00468220(void)

{
  return DAT_005411a4;
}


// ==== FUN_00468230 @ 00468230

void * __thiscall FUN_00468230(void *this,byte param_2)

{
  thunk_FUN_0041bd00();
  if ((param_2 & 1) != 0) {
    _free(this);
  }
  return this;
}


// ==== FUN_00468250 @ 00468250

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00468250(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c0;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  local_4 = 0;
  *(undefined ***)this = &PTR_FUN_0050b210;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b208;
  FUN_00466e50(this,param_2,param_3,DAT_00541100);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_004682f0 @ 004682f0

void * __thiscall FUN_004682f0(void *this,undefined4 param_2,undefined4 param_3)

{
  FUN_00468250(this,param_2,param_3);
  *(undefined ***)this = &PTR_FUN_0050b260;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b258;
  return this;
}


// ==== FUN_00468320 @ 00468320

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468320(void)

{
  return (float10)_DAT_00508d98;
}


// ==== FUN_00468330 @ 00468330

uint __thiscall FUN_00468330(void *this,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined1 local_100 [256];
  
  uVar1 = *(uint *)(&DAT_00540550 + (param_3 * 0x1c + param_2) * 4);
  if ((uVar1 & 0x8000) == 0) {
    iVar2 = FUN_004558f0(1000,10000);
    uVar3 = FUN_004a89c0(local_100,"sunCannonRaiseLower%d.wav",iVar2 / 1000);
    iVar2 = FUN_0041bfb0(this);
    FUN_004a9d70(*(undefined4 *)(iVar2 + 0xe),*(undefined4 *)(iVar2 + 0x12),uVar3,0);
  }
  return uVar1;
}


// ==== FUN_004683c0 @ 004683c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_004683c0(void *this,undefined4 param_2)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c0;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  *(undefined ***)this = &PTR_FUN_0050b2c8;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b2c4;
  *(undefined4 *)((int)this + 0x38) = 0;
  local_4 = 0;
  FUN_00466e50(this,param_2,0,DAT_00541100);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_00468440 @ 00468440

undefined4 FUN_00468440(int param_1,int param_2)

{
  return *(undefined4 *)(&DAT_00540550 + (param_2 * 0x1c + param_1) * 4);
}


// ==== FUN_00468460 @ 00468460

void * __thiscall FUN_00468460(void *this,undefined4 param_2,undefined4 param_3)

{
  FUN_00467fb0(this,param_2,param_3);
  *(undefined ***)this = &PTR_FUN_0050b318;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b310;
  return this;
}


// ==== FUN_00468490 @ 00468490

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468490(void)

{
  return (float10)_DAT_0050b364;
}


// ==== FUN_004684a0 @ 004684a0

undefined4 __cdecl FUN_004684a0(void)

{
  return DAT_0054109c;
}


// ==== FUN_004684b0 @ 004684b0

uint FUN_004684b0(int param_1)

{
  if (param_1 < 0x18) {
    return param_1 + 0x18U | 0x4000;
  }
  return param_1 - 0x18U | 0x8000;
}


// ==== FUN_004684d0 @ 004684d0

void * __thiscall FUN_004684d0(void *this,undefined4 param_2,undefined4 param_3)

{
  FUN_00467fb0(this,param_2,param_3);
  *(undefined ***)this = &PTR_FUN_0050b370;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b368;
  return this;
}


// ==== FUN_00468500 @ 00468500

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468500(void)

{
  return (float10)_DAT_00500ee4;
}


// ==== FUN_00468510 @ 00468510

undefined4 __cdecl FUN_00468510(void)

{
  return DAT_00541268;
}


// ==== FUN_00468520 @ 00468520

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __thiscall FUN_00468520(void *this,int param_2)

{
  uint uVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  
  uVar3 = FUN_0041bfb0();
  iVar4 = FUN_004ad710(uVar3);
  uVar3 = FUN_0041bfb0(this);
  iVar5 = FUN_004ac550(uVar3);
  uVar2 = *(uint *)(iVar5 + 0x130 + iVar4 * 4);
  uVar1 = *(int *)(iVar5 + 0x170 + iVar4 * 4) + -1 + uVar2;
  uVar6 = param_2 + 1;
  if ((int)uVar1 < (int)uVar6) {
    return uVar2 | 0x8000;
  }
  if (uVar6 == uVar1) {
    return uVar6 | 0x4000;
  }
  if (uVar6 == uVar1 - DAT_00540474) {
    iVar4 = FUN_0041bfb0(this);
    FUN_004a9d70(*(undefined4 *)(iVar4 + 0xe),*(undefined4 *)(iVar4 + 0x12),"thunderCannonGrowl.wav"
                 ,0);
  }
  if (uVar6 == uVar1 - _DAT_00540478) {
    iVar4 = FUN_0041bfb0(this);
    FUN_004a9d70(*(undefined4 *)(iVar4 + 0xe),*(undefined4 *)(iVar4 + 0x12),"thunderCannonRoar.wav",
                 0);
  }
  return uVar6;
}


// ==== FUN_004685e0 @ 004685e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_004685e0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  
  FUN_00467fb0(this,param_2,param_3);
  uVar1 = _DAT_00500460;
  *(undefined ***)this = &PTR_FUN_0050b3f0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b3ec;
  *(undefined8 *)((int)this + 0x48) = uVar1;
  *(undefined4 *)((int)this + 0x40) = 1;
  *(double *)((int)this + 0x48) = _DAT_0055b4d0 + _DAT_00506588;
  return this;
}


// ==== FUN_00468630 @ 00468630

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468630(void)

{
  return (float10)_DAT_00507828;
}


// ==== FUN_00468640 @ 00468640

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468640(void)

{
  return (float10)_DAT_00501a54;
}


// ==== FUN_00468650 @ 00468650

undefined4 __cdecl FUN_00468650(void)

{
  return DAT_00541260;
}


// ==== FUN_00468660 @ 00468660

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __thiscall FUN_00468660(int param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 auStack_58 [7];
  undefined4 uStack_3c;
  uint uStack_38;
  undefined1 auStack_20 [32];
  
  if (*(double *)(param_1 + 0x48) <= _DAT_0055b4d0) {
    if (*(int *)(param_1 + 0x40) != 0) {
      *(undefined4 *)(param_1 + 0x40) = 0;
      uStack_38 = 0x46868c;
      iVar3 = FUN_0041bfb0();
      uStack_38 = (iVar3 - DAT_005c8464) / 0x32;
      uStack_3c = 0x4686ab;
      FUN_004a8770();
      *(double *)(param_1 + 0x48) = _DAT_0055b4d0 + _DAT_0050b440;
      return param_2;
    }
    *(undefined4 *)(param_1 + 0x40) = 1;
    *(double *)(param_1 + 0x48) = _DAT_0055b4d0 + _DAT_00506588;
  }
  uStack_38 = 0x4686e8;
  uVar4 = FUN_0041bfb0(param_1);
  uStack_38 = 0x4686ef;
  iVar3 = FUN_004ad710(uVar4);
  uStack_38 = 0x4686f8;
  uVar4 = FUN_0041bfb0(param_1);
  uStack_38 = 0x4686ff;
  iVar5 = FUN_004ac550(uVar4);
  uVar2 = *(uint *)(iVar5 + 0x130 + iVar3 * 4);
  uVar1 = *(int *)(iVar5 + 0x170 + iVar3 * 4) + -1 + uVar2;
  param_2 = param_2 + 1;
  if ((int)uVar1 < (int)param_2) {
    return uVar2;
  }
  uStack_38 = *(undefined4 *)(param_1 + 8);
  uStack_3c = 0x468737;
  puVar6 = (undefined4 *)FUN_0045ae60(auStack_20);
  uStack_38 = (uint)*(ushort *)(param_1 + 0x2c);
  puVar7 = auStack_58;
  for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  iVar3 = FUN_0046af80();
  if (iVar3 == 0) {
    return param_2 | 0x8000;
  }
  if ((param_2 == uVar1) && (*(int *)(param_1 + 0x40) != 0)) {
    return param_2 | 0x4000;
  }
  return param_2;
}


// ==== FUN_004687a0 @ 004687a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_004687a0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  uVar1 = _DAT_00500460;
  pcStack_8 = FUN_004fa6e8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b0c0;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined1 *)((int)this + 0x28) = 0xff;
  local_4 = 0;
  *(undefined ***)this = &PTR_FUN_0050b450;
  *(undefined ***)((int)this + 0x20) = &PTR_FUN_0050b448;
  FUN_00466e50(this,param_2,param_3,DAT_00541100);
  ExceptionList = local_c;
  return this;
}


// ==== FUN_00468820 @ 00468820

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00468820(void)

{
  return (float10)_DAT_005022ec;
}


// ==== FUN_004689c0 @ 004689c0

undefined4 __cdecl FUN_004689c0(int param_1,int param_2,undefined4 param_3)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  void *pvVar4;
  uint uVar5;
  undefined4 *puVar6;
  int iVar7;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004fa763;
  pvStack_c = ExceptionList;
  iVar7 = 0;
  ExceptionList = &pvStack_c;
  if (param_2 == 0) {
    ExceptionList = &pvStack_c;
    FUN_004e0620("gunFormSid != 0","\\Ns\\O\\Gunanim.cpp",0x250);
  }
  puVar6 = (undefined4 *)(param_2 * 0x32 + DAT_005c8464);
  uVar1 = FUN_004ae2d0(puVar6);
  iVar2 = FUN_0041bff0(uVar1);
  if (iVar2 != DAT_005410f8) {
    FUN_004e0620("gunProcess->getProcessType() == gunProcessType","\\Ns\\O\\Gunanim.cpp",0x253);
  }
  iVar2 = (**(code **)*puVar6)();
  iVar2 = iVar2 * 0x32 + DAT_005c8464;
  uVar5 = (uint)*(byte *)(iVar2 + 10);
  iVar3 = FUN_004ac950(iVar2);
  iVar2 = FUN_004ac970(iVar2);
  if (param_1 == 2) {
    if (iVar3 == 1) {
      pvVar4 = operator_new(0x38);
      uStack_4 = 0;
      if (pvVar4 != (void *)0x0) {
        iVar7 = FUN_004682f0(pvVar4,param_2,param_3);
        goto LAB_00468ca5;
      }
    }
    else {
      if ((iVar3 != 0) || (iVar2 != 0)) goto LAB_00468ca5;
      pvVar4 = operator_new(0x38);
      uStack_4 = 1;
      if (pvVar4 != (void *)0x0) {
        iVar7 = FUN_004687a0(pvVar4,param_2,param_3);
        goto LAB_00468ca5;
      }
    }
  }
  else {
    if (param_1 != 5) {
      if (param_1 == 8) {
        if (uVar5 == DAT_00541258) {
          pvVar4 = operator_new(0x40);
          uStack_4 = 8;
          if (pvVar4 != (void *)0x0) {
            iVar7 = FUN_004683c0(pvVar4,param_2);
            goto LAB_00468ca5;
          }
          goto LAB_00468ca1;
        }
        uVar1 = 0x281;
      }
      else {
        uVar1 = 0x286;
      }
LAB_00468a73:
      FUN_004e0620("false","\\Ns\\O\\Gunanim.cpp",uVar1);
      goto LAB_00468ca5;
    }
    if (iVar3 == 0) {
      if (uVar5 == DAT_00541174) {
        pvVar4 = operator_new(0x58);
        uStack_4 = 2;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_00468140(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
      else if (uVar5 == DAT_00541288) {
        pvVar4 = operator_new(0x40);
        uStack_4 = 3;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_00468050(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
      else {
        if (uVar5 != DAT_005412a8) goto LAB_00468ca5;
        pvVar4 = operator_new(0x40);
        uStack_4 = 4;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_004680f0(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
    }
    else {
      if (iVar3 != 1) {
        uVar1 = 0x279;
        goto LAB_00468a73;
      }
      if (iVar2 == 2) {
        pvVar4 = operator_new(0x40);
        uStack_4 = 5;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_004684d0(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
      else if (iVar2 == 1) {
        pvVar4 = operator_new(0x50);
        uStack_4 = 6;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_004685e0(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
      else {
        pvVar4 = operator_new(0x40);
        uStack_4 = 7;
        if (pvVar4 != (void *)0x0) {
          iVar7 = FUN_00468460(pvVar4,param_2,param_3);
          goto LAB_00468ca5;
        }
      }
    }
  }
LAB_00468ca1:
  iVar7 = 0;
LAB_00468ca5:
  ExceptionList = pvStack_c;
  return *(undefined4 *)(iVar7 + 4);
}


// ==== FUN_00480260 @ 00480260

void FUN_00480260(void)

{
  return;
}


