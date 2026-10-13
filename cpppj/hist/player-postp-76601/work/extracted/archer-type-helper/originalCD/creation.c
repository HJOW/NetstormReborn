// ==== FUN_004204a0 @ 004204a0

int __thiscall FUN_004204a0(undefined4 param_1,int *param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  iVar1 = *param_2;
  while( true ) {
    if (iVar1 == 0) {
      return -1;
    }
    iVar1 = FUN_004f2980(*param_2,param_3);
    if (iVar1 == 0) break;
    param_2 = param_2 + 1;
    iVar2 = iVar2 + 1;
    iVar1 = *param_2;
  }
  return iVar2;
}


