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


// ==== FUN_0041bfb0 @ 0041bfb0

int __thiscall FUN_0041bfb0(void *this)

{
  int iVar1;
  
  iVar1 = *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_004e0620("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4);
  }
  return iVar1;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_0041ce20 @ 0041ce20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0041ce20(float *param_1,float param_2,float param_3)

{
  float10 fVar1;
  
  if ((*param_1 == param_2) && (param_1[1] == param_3)) {
    return (float10)_DAT_00500fc4;
  }
  fVar1 = (float10)fpatan(-((float10)param_3 - (float10)param_1[1]),
                          (float10)param_2 - (float10)*param_1);
  fVar1 = (float10)FUN_0041cdb0((float)((float10)_DAT_005022e0 - fVar1 * (float10)_DAT_0052f950));
  return fVar1;
}


// ==== FUN_0041d7e0 @ 0041d7e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_0041d7e0(float param_1,float param_2,float param_3,float param_4)

{
  float10 fVar1;
  
  if ((param_1 == param_3) && (param_2 == param_4)) {
    return (float10)_DAT_00500fc4;
  }
  fVar1 = (float10)fpatan(-((float10)param_4 - (float10)param_2),(float10)param_3 - (float10)param_1
                         );
  return fVar1 * (float10)_DAT_0052f950;
}


// ==== FUN_0041cdb0 @ 0041cdb0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_0041cdb0(void)

{
  float10 fVar1;
  
  fVar1 = (float10)FUN_004e5e8a();
  if (fVar1 < (float10)_DAT_00500460) {
    fVar1 = fVar1 + (float10)_DAT_005022d0;
  }
  if ((float10)_DAT_005022d0 <= fVar1) {
    if ((DAT_005e4794 != 0) && ((float10)_DAT_005022cc <= fVar1)) {
      FUN_004e0620("angle < 361.0f","\\Ns\\O\\Boardcoord.cpp",0xb3);
      return (float10)_DAT_005022a0;
    }
    fVar1 = (float10)_DAT_005022a0;
  }
  return fVar1;
}


// ==== FUN_0043fa50 @ 0043fa50

void __cdecl FUN_0043fa50(void)

{
  FUN_004e49c0();
  return;
}


// ==== FUN_004e5e8a @ 004e5e8a

void __cdecl FUN_004e5e8a(void)

{
  __cintrindisp2();
  return;
}


