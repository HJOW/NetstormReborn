// ==== FUN_004af490 @ 004af490

void __thiscall FUN_004af490(int param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540a28 != 0) {
    piVar1 = (int *)FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_004ed640 @ 004ed640

undefined4 __cdecl FUN_004ed640(int param_1)

{
  return (&DAT_005b7bb0)[param_1];
}


