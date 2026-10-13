// ==== FUN_004cdf30 @ 004cdf30

undefined4 __thiscall FUN_004cdf30(void *this,int param_2,int param_3,int param_4)

{
  if ((DAT_00583e28 - param_4 < DAT_00583e20 + param_4) ||
     (DAT_00583e2c - param_4 < DAT_00583e24 + param_4)) {
    return 0;
  }
  if ((*(int *)this <= param_2) && (*(int *)((int)this + 4) <= param_3)) {
    if ((((DAT_00583e20 + param_4 < param_2) && (*(int *)this < DAT_00583e28 - param_4)) &&
        (DAT_00583e24 + param_4 < param_3)) && (*(int *)((int)this + 4) < DAT_00583e2c - param_4)) {
      return 1;
    }
    return 0;
  }
  return 0;
}


// ==== FUN_00456d10 @ 00456d10

void __cdecl FUN_00456d10(int param_1,int param_2,int param_3,int param_4,undefined4 param_5)

{
  int iVar1;
  
  iVar1 = DAT_00583e28;
  if (param_1 <= DAT_00583e28) {
    iVar1 = param_1;
  }
  param_1 = iVar1;
  if (iVar1 <= DAT_00583e20) {
    param_1 = DAT_00583e20;
  }
  iVar1 = DAT_00583e2c;
  if (param_2 <= DAT_00583e2c) {
    iVar1 = param_2;
  }
  param_2 = iVar1;
  if (iVar1 <= DAT_00583e24) {
    param_2 = DAT_00583e24;
  }
  iVar1 = DAT_00583e28;
  if (param_3 <= DAT_00583e28) {
    iVar1 = param_3;
  }
  param_3 = iVar1;
  if (iVar1 <= DAT_00583e20) {
    param_3 = DAT_00583e20;
  }
  iVar1 = DAT_00583e2c;
  if (param_4 <= DAT_00583e2c) {
    iVar1 = param_4;
  }
  param_4 = iVar1;
  if (iVar1 <= DAT_00583e24) {
    param_4 = DAT_00583e24;
  }
  if ((param_3 != param_1 && -1 < param_3 - param_1) &&
     (param_4 != param_2 && -1 < param_4 - param_2)) {
    FUN_00457320(DAT_005203a8,&param_1,param_5);
  }
  return;
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_004d64e0 @ 004d64e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004d64e0(int param_1,undefined4 param_2,int param_3,int param_4)

{
  if (param_1 != 0) {
    FUN_00465772(&DAT_0055bb18,param_1,param_2,param_3 - DAT_0055bb1c,param_4 - _DAT_0055bb20);
  }
  return;
}


// ==== FUN_004d6510 @ 004d6510

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004d6510(int param_1,undefined4 param_2,int param_3,int param_4)

{
  if (param_1 != 0) {
    FUN_00465c7f(&DAT_0055bb18,param_1,param_2,param_3 - DAT_0055bb1c,param_4 - _DAT_0055bb20);
  }
  return;
}


// ==== FUN_004d6630 @ 004d6630

void __cdecl FUN_004d6630(undefined4 param_1)

{
  FUN_00465c5e(param_1);
  return;
}


