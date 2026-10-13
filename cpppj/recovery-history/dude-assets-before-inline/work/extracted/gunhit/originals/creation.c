// ==== FUN_004d4d40 @ 004d4d40

float10 __thiscall FUN_004d4d40(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  float10 fVar5;
  
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  iVar1 = FUN_004ac550();
  if (*(int *)(iVar1 + 0x1b0) != 0) {
    uVar2 = FUN_004ac200(iVar4);
    if ((uVar2 & 0x120000) != 0) {
      iVar1 = FUN_004ac550(this);
      return (float10)*(float *)(iVar1 + 0x1b4);
    }
  }
  uVar3 = FUN_004ac550(this);
  fVar5 = (float10)FUN_0049acf0(uVar3);
  return fVar5;
}


// ==== FUN_004d4da0 @ 004d4da0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004d4da0(void *this,double *param_2)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004ac950();
  uVar2 = FUN_004ac200(this);
  if ((((uVar2 & 0x100000) == 0) && (iVar1 != 1)) && (iVar1 != 0)) {
    FUN_004e0620("(getGenus() & gFLYER) || group == grCANNON || group == grARCHER",
                 "\\Ns\\O\\Unit.cpp",0x46);
  }
  uVar2 = (uint)(*(ushort *)((int)this + 0xc) >> 1);
  *param_2 = (double)((int)uVar2 >> 3) + (double)(uVar2 & 7) * _DAT_0050df38;
  return;
}


// ==== FUN_004d4e10 @ 004d4e10

void __thiscall FUN_004d4e10(void *this)

{
  short sVar1;
  ushort uVar2;
  int iVar3;
  uint uVar4;
  
  iVar3 = FUN_004ac950();
  uVar4 = FUN_004ac200(this);
  if ((((uVar4 & 0x100000) == 0) && (iVar3 != 1)) && (iVar3 != 0)) {
    FUN_004e0620("(getGenus() & gFLYER) || group == grCANNON || group == grARCHER",
                 "\\Ns\\O\\Unit.cpp",0x53);
  }
  sVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  *(ushort *)((int)this + 0xc) = (uVar2 | sVar1 * 8) << 1 | *(byte *)((int)this + 0xc) & 1;
  FUN_004214a0(((int)this - DAT_005c8464) / 0x32);
  return;
}


// ==== FUN_004d4eb0 @ 004d4eb0

void __thiscall FUN_004d4eb0(void *this,int param_2,undefined4 param_3)

{
  int *piVar1;
  
  piVar1 = (int *)(param_2 * 0x32 + DAT_005c8464);
  if ((param_2 != 0) && ((*(byte *)((int)piVar1 + 0xb) & 3) == 0)) {
    (**(code **)(*piVar1 + 0x98))
              (param_3,*(undefined1 *)((int)this + 0x22),((int)this - DAT_005c8464) / 0x32);
  }
  return;
}


// ==== FUN_004d4f00 @ 004d4f00

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004d4f00(void *this,undefined4 param_2)

{
  ushort uVar1;
  int iVar2;
  double *pdVar3;
  undefined4 uVar4;
  float10 fVar5;
  undefined1 auStack_c [12];
  
  fVar5 = (float10)(**(code **)(*(int *)this + 0xbc))(param_2);
  iVar2 = FUN_004e49c0();
  uVar1 = FUN_004e49c0();
  pdVar3 = (double *)FUN_004d4da0(this,auStack_c);
  if (_DAT_005012d0 <=
      (((float)((int)(((uint)uVar1 | iVar2 * 8) & 0x7fff) >> 3) +
       (float)(uVar1 & 7) * (float)_DAT_0050df38) - (float)*pdVar3) * (float)fVar5) {
    iVar2 = *(int *)this;
    uVar4 = FUN_004e49c0();
    (**(code **)(iVar2 + 0xc0))(param_2,uVar4);
  }
  FUN_004d4e10(this,DAT_0055b4d0,DAT_0055b4d4);
  return;
}


// ==== FUN_004d5000 @ 004d5000

void __thiscall FUN_004d5000(void *this)

{
  FUN_004d4e10(this,DAT_0055b4d0,DAT_0055b4d4);
  return;
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


