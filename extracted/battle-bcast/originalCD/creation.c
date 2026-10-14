// ==== FUN_004c0420 @ 004c0420

void __cdecl FUN_004c0420(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  
  iVar2 = 1;
  iVar4 = 0xac;
  piVar3 = &DAT_0053d614;
  iVar1 = DAT_0050f834;
  do {
    if ((((*(byte *)(iVar4 + 0x50 + iVar1) & 1) != 0) && ((int *)*piVar3 != (int *)0x0)) &&
       (DAT_0050f6c8 != iVar2)) {
      (**(code **)(*(int *)*piVar3 + 4))(param_1);
      iVar1 = DAT_0050f834;
    }
    piVar3 = piVar3 + 1;
    iVar4 = iVar4 + 0xac;
    iVar2 = iVar2 + 1;
  } while (iVar4 < 0x60c);
  return;
}


// ==== FUN_004c0480 @ 004c0480

void __cdecl FUN_004c0480(int param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  
  if (DAT_00540a28 == 0) {
    FUN_00490040(s_server_0053d9fc,s__Ns_o_ztrans_cpp_0053d9e8,0x1c3);
  }
  iVar1 = DAT_0050f6c8;
  iVar4 = 1;
  piVar3 = &DAT_0053d614;
  iVar5 = 0xac;
  iVar2 = DAT_0050f834;
  do {
    if (((((*(byte *)(iVar5 + 0x50 + iVar2) & 1) != 0) && (param_1 != iVar4)) && (iVar4 != iVar1))
       && ((int *)*piVar3 != (int *)0x0)) {
      (**(code **)(*(int *)*piVar3 + 4))(param_2);
      iVar2 = DAT_0050f834;
    }
    piVar3 = piVar3 + 1;
    iVar5 = iVar5 + 0xac;
    iVar4 = iVar4 + 1;
  } while (iVar5 < 0x60c);
  return;
}


