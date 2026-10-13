// ==== FUN_004ac220 @ 004ac220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ac220(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == _DAT_005411a0) {
    FUN_004214a0(((int)this - DAT_005c8464) / 0x32);
  }
  return;
}


// ==== FUN_004505a0 @ 004505a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004505a0(float *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  float local_4;
  
  if (param_2 == 0) {
    FUN_004e0620(&DAT_00501390,".\\Dude.cpp",0x29);
  }
  local_4 = *(float *)(param_2 * 0x32 + 0x12 + DAT_005c8464);
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  FUN_004ac550(iVar4);
  iVar2 = FUN_004e49c0();
  fVar1 = (float)iVar2 + _DAT_00500ee4;
  iVar2 = FUN_004ac550(iVar4);
  local_4 = local_4 - (float)*(int *)(iVar2 + 0x1d8);
  uVar3 = FUN_004ac200(iVar4);
  if ((DAT_005424b8 & uVar3) == 0) {
    local_4 = local_4 - _DAT_00500ee4;
  }
  else {
    uVar3 = FUN_004ac200(iVar4);
    if ((uVar3 & 0x10040000) != 0) {
      local_4 = local_4 + _DAT_005012d0;
    }
  }
  *param_1 = fVar1;
  param_1[1] = local_4;
  return;
}


// ==== FUN_00450760 @ 00450760

int __thiscall FUN_00450760(void *this,float param_2,float param_3,int param_4)

{
  int iVar1;
  int iVar2;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  iVar1 = FUN_004b1a90(((int)this - DAT_005c8464) / 0x32,DAT_005410d0,0,0);
  if (iVar1 != 0) {
    iVar1 = FUN_004ae2d0(iVar1 * 0x32 + DAT_005c8464);
    local_10 = param_2;
    local_c = param_3;
    FUN_0041d7a0(&local_10);
    local_8 = local_10;
    local_4 = local_c;
    if ((*(float *)(iVar1 + 0xc + *(int *)(iVar1 + 0x10) * 8) == local_10) &&
       (*(float *)(iVar1 + *(int *)(iVar1 + 0x10) * 8 + 0x10) == local_c)) {
      return 1;
    }
  }
  iVar1 = FUN_004ac550(this);
  local_10 = (float)FUN_004e49c0();
  if (param_4 != 0) {
    *(float *)(iVar1 + 0xe0) = *(float *)(iVar1 + 0xe0) + *(float *)(iVar1 + 0xe0);
  }
  iVar2 = FUN_00489080(((int)this - DAT_005c8464) / 0x32,param_2,param_3,0,0,0,0,0);
  *(float *)(iVar1 + 0xe0) = (float)(int)local_10;
  if (iVar2 != 0) {
    iVar1 = FUN_004ad710(this);
    if (iVar1 == 8) {
      iVar1 = FUN_004ac550(this);
      FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x130),0);
    }
  }
  return iVar2;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_004ad710 @ 004ad710

int __thiscall FUN_004ad710(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4) + -0x41;
}


// ==== FUN_0041cfc0 @ 0041cfc0

float10 __thiscall FUN_0041cfc0(void *this,float param_2,float param_3)

{
  float10 fVar1;
  
  fVar1 = (float10)*(float *)((int)this + 4) - (float10)param_3;
  return SQRT(fVar1 * fVar1 +
              ((float10)*(float *)this - (float10)param_2) *
              ((float10)*(float *)this - (float10)param_2));
}


// ==== FUN_00434380 @ 00434380

void FUN_00434380(int param_1,int param_2,int *param_3,int *param_4,int param_5)

{
  int iVar1;
  
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_1 / iVar1) {
    param_1 = DAT_00531928 / iVar1 + -1;
  }
  else {
    param_1 = param_1 / (int)(&DAT_005424f8)[param_5];
  }
  *param_3 = param_1;
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_2 / iVar1) {
    *param_4 = DAT_00531928 / iVar1 + -1;
    return;
  }
  *param_4 = param_2 / (int)(&DAT_005424f8)[param_5];
  return;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1d8);
  return;
}


// ==== FUN_004b2b00 @ 004b2b00

int __thiscall FUN_004b2b00(void *this,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_004e0620("level >=0 && level < HASH_LEVEL_COUNT","\\Ns\\O\\Squidhash.cpp",0x5c);
  }
  return *(int *)((int)this + param_4 * 4 + 0xc) +
         ((DAT_00531928 * param_3) / (int)(&DAT_005424f8)[param_4] + param_2) * 2;
}


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong __cdecl FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
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


