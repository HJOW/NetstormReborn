// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_00419850 @ 00419850

int __thiscall FUN_00419850(void *this,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  
  if ((*(uint *)((int)this + 0xe8) & 0x440000) == 0) {
    if ((*(int *)((int)this + 0x114) <= param_3) || (param_3 < 0)) goto LAB_004198a8;
LAB_00419887:
    if (((*(int *)((int)this + 0xdc) != 0) && (*(int *)this != -0x22222223)) &&
       (*(int *)this != -0x32323233)) goto LAB_004198d0;
  }
  else {
    if ((param_3 < *(int *)((int)this + 0x114) * 2) && (-1 < param_3)) goto LAB_00419887;
LAB_004198a8:
    uVar1 = FUN_0049adc0(this,param_3,param_2);
    FUN_00436d40(uVar1);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_004198d0:
  return *(int *)(*(int *)((int)this + 0xdc) + 8 + param_3 * 8) + -0x24 + *(int *)((int)this + 0xdc)
  ;
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


// ==== FUN_0040e800 @ 0040e800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0040e800(void *this)

{
  if ((((_DAT_00500fc4 < *(float *)this) && (*(float *)this < (float)DAT_00531928)) &&
      (_DAT_00500fc4 < *(float *)((int)this + 4))) &&
     (*(float *)((int)this + 4) < (float)DAT_00531928)) {
    return 1;
  }
  return 0;
}


