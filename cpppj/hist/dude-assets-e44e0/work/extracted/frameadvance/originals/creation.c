// ==== FUN_004afc90 @ 004afc90

undefined4 __thiscall FUN_004afc90(int param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  iVar1 = *(int *)(param_1 + 0x24);
  iVar3 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  iVar3 = (int)*(char *)(*(int *)(iVar3 + 0x124) + iVar1 * 4);
  iVar1 = *(int *)(iVar2 + 0x6c + iVar3 * 4);
  iVar2 = *(int *)(iVar2 + 0x2c + iVar3 * 4);
  iVar3 = (param_2 - iVar2) + *(int *)(param_1 + 0x24);
  uVar4 = 0;
  if (iVar3 < iVar1) {
    if (-1 < iVar3) goto LAB_004afce6;
  }
  else {
    iVar1 = -iVar1;
  }
  iVar3 = iVar3 + iVar1;
  uVar4 = 1;
LAB_004afce6:
  FUN_004acee0(iVar2 + iVar3,param_3);
  return uVar4;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


