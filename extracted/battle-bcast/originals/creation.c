// ==== FUN_004e2e30 @ 004e2e30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e2e30(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = 1;
  iVar2 = 0xb4;
  do {
    if ((((*(byte *)(iVar2 + 0x50 + DAT_0059534c) & 1) != 0) && ((&DAT_005e4a18)[iVar1] != 0)) &&
       (iVar1 != _DAT_00540c70)) {
      (**(code **)(*(int *)(&DAT_005e4a18)[iVar1] + 4))(param_1);
    }
    iVar1 = iVar1 + 1;
    iVar2 = iVar2 + 0xb4;
  } while (iVar1 < 9);
  return;
}


// ==== FUN_004e2e80 @ 004e2e80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e2e80(int param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (DAT_00540bc0 == 0) {
    FUN_004e0620("server","\\Ns\\O\\Ztrans.cpp",0x1c3);
  }
  iVar1 = _DAT_00540c70;
  iVar2 = 1;
  iVar3 = 0xb4;
  do {
    if (((((*(byte *)(iVar3 + 0x50 + DAT_0059534c) & 1) != 0) && (iVar2 != param_1)) &&
        (iVar2 != iVar1)) && ((int *)(&DAT_005e4a18)[iVar2] != (int *)0x0)) {
      (**(code **)(*(int *)(&DAT_005e4a18)[iVar2] + 4))(param_2);
    }
    iVar2 = iVar2 + 1;
    iVar3 = iVar3 + 0xb4;
  } while (iVar2 < 9);
  return;
}


