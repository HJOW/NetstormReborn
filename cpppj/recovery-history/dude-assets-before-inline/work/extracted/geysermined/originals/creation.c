// ==== FUN_004266c0 @ 004266c0

void __cdecl FUN_004266c0(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int *piVar4;
  undefined1 local_100 [256];
  
  iVar3 = (uint)*(ushort *)(param_3 + 4) * 0x32;
  piVar4 = (int *)(iVar3 + DAT_005c8464);
  if ((*(byte *)(iVar3 + 0xb + DAT_005c8464) & 1) == 0) {
    iVar3 = FUN_004558f0(1000,7000);
    uVar2 = FUN_004a89c0(local_100,"geyserMined%d.wav",iVar3 / 1000);
    FUN_004a9d70(*(undefined4 *)((int)piVar4 + 0xe),*(undefined4 *)((int)piVar4 + 0x12),uVar2,0);
    FUN_0049a840(DAT_005412c0);
    uVar2 = FUN_004e49c0();
    FUN_004ac220(piVar4,uVar2);
    if ((DAT_00540bc4 != 0) && ((short)piVar4[3] == 0)) {
      uVar2 = *(undefined4 *)((int)piVar4 + 0x12);
      uVar1 = *(undefined4 *)((int)piVar4 + 0xe);
      (**(code **)(*piVar4 + 0x10))(0x1000);
      iVar3 = FUN_0046f6e0(&DAT_00568b60,uVar1,uVar2);
      if (iVar3 == 0) {
        piVar4 = (int *)FUN_004af530(DAT_005411a8,0);
        (**(code **)(*piVar4 + 0x90))(uVar1,uVar2,0);
      }
    }
  }
  return;
}


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


// ==== FUN_004558f0 @ 004558f0

int __cdecl FUN_004558f0(int param_1,int param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % (uint)(param_2 - param_1) + param_1;
}


// ==== FUN_0046f6e0 @ 0046f6e0

void __thiscall FUN_0046f6e0(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0(uVar1);
  FUN_0046f680(uVar2,uVar1);
  return;
}


// ==== FUN_0046f680 @ 0046f680

undefined4 FUN_0046f680(int param_1,int param_2)

{
  uint uVar1;
  
  if ((((-1 < param_1) && (-1 < param_2)) && (param_1 < 0x100)) &&
     (((param_2 < 0x100 &&
       (uVar1 = (uint)*(ushort *)(DAT_005c84bc + (param_2 * 0x100 + param_1) * 2), uVar1 != 0)) &&
      (*(byte *)(uVar1 * 0x32 + 10 + DAT_005c8464) == DAT_005411dc)))) {
    return 1;
  }
  return 0;
}


// ==== FUN_004214a0 @ 004214a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004214a0(int param_1)

{
  int iVar1;
  
  iVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((((*(byte *)(iVar1 + 10) == _DAT_005411a0) && ((*(byte *)(iVar1 + 0x28) & 8) == 0)) &&
      (DAT_005e4794 != 0)) && (7 < (*(byte *)(iVar1 + 0xc) >> 3 & 0xf))) {
    FUN_004e0620("num >= 0 && num <= 7","\\Ns\\O\\Bridge.cpp",0xff);
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
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


