// ==== FUN_004aefa0 @ 004aefa0

void __thiscall FUN_004aefa0(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  if (DAT_00540a24 == 0) {
    if ((param_2 == 0) || ((0 < param_2 && (param_2 < 9)))) goto LAB_004af000;
    uVar6 = 0x9c1;
    pcVar5 = s__ns_o_Squid_cpp_0053a44c;
    pcVar4 = s_newPlayerId____INVALID_PLAYER_ID_0053a45c;
  }
  else {
    if ((0 < param_2) && (param_2 < 0x28)) goto LAB_004af000;
    uVar6 = 0x9be;
    pcVar5 = s__ns_o_Squid_cpp_0053a404;
    pcVar4 = s_0_<_newPlayerId_&&_newPlayerId_<_0053a414;
  }
  FUN_00490040(pcVar4,pcVar5,uVar6);
LAB_004af000:
  if ((DAT_00540a1c != 0) || (DAT_00540a20 != 0)) {
    uVar3 = FUN_004abae0();
    if ((uVar3 & 0x4200) != 0) {
      FUN_00407250(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24);
    }
  }
  iVar2 = DAT_00540a20;
  iVar1 = DAT_00540a1c;
  *(char *)(param_1 + 0x20) = (char)param_2;
  if ((iVar1 != 0) || (iVar2 != 0)) {
    uVar3 = FUN_004abae0();
    if (((uVar3 & 0x4200) != 0) &&
       (((*(byte *)(param_1 + 0xb) & 4) == 0 && ((*(byte *)(param_1 + 0x23) & 8) == 0)))) {
      FUN_004071f0(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24);
    }
  }
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004071f0 @ 004071f0

void FUN_004071f0(int param_1,undefined4 param_2)

{
  int iVar1;
  int *piVar2;
  
  if (param_1 != 0) {
    piVar2 = (int *)(DAT_0050f834 + param_1 * 0xac);
    if (piVar2 == (int *)0x0) {
      FUN_00490040(&DAT_0050fb04,s__ns_o_Player_cpp_0050faf0,0x2c3);
    }
    else if (piVar2[2] < piVar2[1]) {
      iVar1 = piVar2[2] + 1;
      piVar2[2] = iVar1;
      *(undefined4 *)(*piVar2 + -4 + iVar1 * 4) = param_2;
    }
  }
  return;
}


// ==== FUN_00407250 @ 00407250

void FUN_00407250(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  
  if (param_1 != 0) {
    piVar6 = (int *)(DAT_0050f834 + param_1 * 0xac);
    if (piVar6 == (int *)0x0) {
      FUN_00490040(&DAT_0050fb1c,s__ns_o_Player_cpp_0050fb08,0x2cd);
    }
    iVar5 = 0;
    if (piVar6 != (int *)0x0) {
      iVar4 = 0;
      iVar1 = piVar6[2];
      if (0 < iVar1) {
        iVar2 = *piVar6;
        do {
          iVar3 = *(int *)(iVar2 + iVar4 * 4);
          if (iVar3 == param_2) {
            iVar5 = iVar5 + 1;
          }
          else {
            *(int *)(iVar2 + (iVar4 - iVar5) * 4) = iVar3;
          }
          iVar4 = iVar4 + 1;
        } while (iVar4 < iVar1);
      }
      piVar6[2] = iVar1 - iVar5;
    }
  }
  return;
}


