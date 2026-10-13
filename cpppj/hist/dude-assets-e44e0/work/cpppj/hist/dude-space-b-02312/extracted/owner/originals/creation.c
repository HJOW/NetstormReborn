// ==== FUN_004adf00 @ 004adf00

void __thiscall FUN_004adf00(int param_1,int param_2)

{
  int iVar1;
  char *pcVar2;
  undefined4 uVar3;
  
  if (DAT_00594fc8 == 0) {
    if ((param_2 == 0) || ((0 < param_2 && (param_2 < 9)))) goto LAB_004adf50;
    uVar3 = 0xa02;
    pcVar2 = 
    "newPlayerId == INVALID_PLAYER_ID ||( FIRST_REAL_PLAYER <= newPlayerId && newPlayerId < MAX_PLAYERS)"
    ;
  }
  else {
    if ((0 < param_2) && (param_2 < 0x28)) goto LAB_004adf50;
    uVar3 = 0x9ff;
    pcVar2 = "0 < newPlayerId && newPlayerId < CL_NUM_CHAL_PLAYERS";
  }
  FUN_004e0620(pcVar2,"\\Ns\\O\\Squid.cpp",uVar3);
LAB_004adf50:
  if ((DAT_00594fb8 != 0) || (DAT_00594fbc != 0)) {
    iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
    if ((*(uint *)(iVar1 + 0xec) & 0x4200) != 0) {
      FUN_004900a0(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32);
    }
  }
  *(char *)(param_1 + 0x22) = (char)param_2;
  if ((DAT_00594fb8 != 0) || (DAT_00594fbc != 0)) {
    iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
    if (((*(uint *)(iVar1 + 0xec) & 0x4200) != 0) &&
       (((*(byte *)(param_1 + 0xb) & 4) == 0 && ((*(byte *)(param_1 + 0x28) & 8) == 0)))) {
      FUN_00490050(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32);
    }
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00490050 @ 00490050

void FUN_00490050(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  if (param_1 != 0) {
    piVar1 = (int *)(param_1 * 0xb4 + DAT_0059534c);
    if (piVar1 == (int *)0x0) {
      FUN_004e0620(&DAT_005078ac,"\\Ns\\O\\Player.cpp",0x313);
      return;
    }
    if (piVar1[2] < piVar1[1]) {
      *(undefined4 *)(*piVar1 + piVar1[2] * 4) = param_2;
      piVar1[2] = piVar1[2] + 1;
    }
  }
  return;
}


// ==== FUN_004900a0 @ 004900a0

void FUN_004900a0(int param_1,undefined4 param_2)

{
  if (param_1 != 0) {
    if (param_1 * 0xb4 + DAT_0059534c == 0) {
      FUN_004e0620(&DAT_005078ac,"\\Ns\\O\\Player.cpp",0x31d);
      return;
    }
    FUN_0040ea00(param_2);
  }
  return;
}


// ==== FUN_0040ea00 @ 0040ea00

void __thiscall FUN_0040ea00(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  iVar3 = 0;
  if (0 < param_1[2]) {
    do {
      iVar1 = *(int *)(*param_1 + iVar3 * 4);
      if (iVar1 == param_2) {
        iVar2 = iVar2 + 1;
      }
      else {
        *(int *)(*param_1 + (iVar3 - iVar2) * 4) = iVar1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < param_1[2]);
  }
  param_1[2] = param_1[2] - iVar2;
  return;
}


