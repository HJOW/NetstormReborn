// ==== FUN_00470ac0 @ 00470ac0

undefined4 __cdecl FUN_00470ac0(undefined4 param_1,undefined4 param_2)

{
  FUN_0041d0d0(&param_1);
  FUN_00451470(param_1,param_2);
  FUN_004558c0(100);
  FUN_0045a750(param_1,param_2,3,0x3ca3d70a);
  return 0;
}


// ==== FUN_0045a750 @ 0045a750

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_0045a750(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  void *pvVar4;
  void *local_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004fa4db;
  local_c = ExceptionList;
  if (((_DAT_005c85f0 == _DAT_00500460) && (DAT_005c89b8 == 0)) &&
     (ExceptionList = &local_c, iVar1 = FUN_00471ab0(), iVar1 < 0x7531)) {
    FUN_0041d0d0(&param_1);
    piVar2 = (int *)FUN_004af530(DAT_005412d8,2);
    iVar3 = FUN_004ac550(piVar2);
    iVar1 = param_3;
    piVar2[9] = *(int *)(iVar3 + 0x130 + param_3 * 4);
    (**(code **)(*piVar2 + 0x90))(param_1,param_2,0);
    pvVar4 = operator_new(0x28);
    uStack_4 = 0;
    if (pvVar4 != (void *)0x0) {
      FUN_00496e80(pvVar4,iVar1,((int)piVar2 - DAT_005c8464) / 0x32,param_4);
    }
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_0045a6e0 @ 0045a6e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0045a6e0(void *this,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  float10 fVar3;
  
  iVar1 = FUN_004ac550();
  iVar1 = *(int *)(iVar1 + 0x170 + param_2 * 4);
  iVar2 = FUN_004ac550(this);
  if ((-1 < param_3) && (param_3 < iVar1)) {
    FUN_004acee0(this,*(int *)(iVar2 + 0x130 + param_2 * 4) + param_3,0);
    fVar3 = (float10)FUN_00496e70(param_2,param_3,param_4);
    return fVar3;
  }
  (**(code **)(*(int *)this + 0x10))(0);
  return (float10)_DAT_00502350;
}


// ==== FUN_0045ae00 @ 0045ae00

undefined4 * __cdecl FUN_0045ae00(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00509758;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_00451470 @ 00451470

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00451470(float param_1,float param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  float local_48;
  float local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_48 = param_1 - _DAT_005012d0;
  local_40[0] = &PTR_FUN_00502318;
  local_44 = param_2 - _DAT_005012d0;
  local_8 = 0;
  local_4 = 0;
  FUN_0041d7a0(&local_48);
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  FUN_004b16d0(local_40,iVar2 + -4,iVar1 + -4,iVar2 + 4,iVar1 + 4);
  while( true ) {
    uVar3 = local_c * 0x32 + DAT_005c8464;
    if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3))
    break;
    uVar3 = FUN_004ac200();
    if ((uVar3 & 0x400) != 0) {
      FUN_00450db0(local_c * 0x32 + DAT_005c8464);
    }
    FUN_004b1810(local_40);
  }
  return;
}


// ==== FUN_004558c0 @ 004558c0

uint __cdecl FUN_004558c0(uint param_1)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % param_1;
}


// ==== FUN_00471ab0 @ 00471ab0

int __cdecl FUN_00471ab0(void)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = 0;
  piVar2 = &DAT_0056b80c;
  do {
    if (piVar2[-1] != 0) {
      iVar1 = iVar1 + 1;
    }
    if (*piVar2 != 0) {
      iVar1 = iVar1 + 1;
    }
    if (piVar2[1] != 0) {
      iVar1 = iVar1 + 1;
    }
    if (piVar2[2] != 0) {
      iVar1 = iVar1 + 1;
    }
    if (piVar2[3] != 0) {
      iVar1 = iVar1 + 1;
    }
    piVar2 = piVar2 + 5;
  } while ((int)piVar2 < 0x59290c);
  return iVar1;
}


// ==== FUN_00496e80 @ 00496e80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00496e80(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb2d8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  uVar1 = _DAT_00500460;
  *(undefined ***)this = &PTR_FUN_0050f420;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  local_4 = 0;
  FUN_0041bd20(this,DAT_00541128,param_3,0,0x50);
  *(undefined4 *)((int)this + 0x1c) = param_4;
  uVar1 = _DAT_0055b4d0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  *(undefined4 *)((int)this + 0x18) = param_2;
  ExceptionList = local_c;
  return this;
}


// ==== FUN_0041d0d0 @ 0041d0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d0d0(void *this)

{
  float fVar1;
  float fVar2;
  
  fVar1 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)this) {
    fVar1 = *(float *)this;
  }
  *(float *)this = fVar1;
  fVar2 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)((int)this + 4)) {
    fVar2 = *(float *)((int)this + 4);
  }
  *(float *)((int)this + 4) = fVar2;
  if (_DAT_005022e8 < fVar1) {
    fVar1 = _DAT_005022e8;
  }
  *(float *)this = fVar1;
  if (fVar2 <= _DAT_005022e8) {
    *(float *)((int)this + 4) = fVar2;
    return;
  }
  *(undefined4 *)((int)this + 4) = 0x437f0000;
  return;
}


