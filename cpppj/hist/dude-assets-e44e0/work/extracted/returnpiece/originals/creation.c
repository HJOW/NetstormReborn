// ==== FUN_004468b0 @ 004468b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004468b0(uint param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  int iVar2;
  void *pvVar3;
  void *local_18 [4];
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004f9f1b;
  local_18[3] = ExceptionList;
  if (DAT_0055a284 == _DAT_005411a0) {
    ExceptionList = local_18 + 3;
    uVar1 = FUN_004558f0(0,10000);
    uVar1 = FUN_004257c0(uVar1);
    FUN_004afa70(DAT_0055a284,uVar1,param_1 & 0xfffffeff,0x10,0);
    ExceptionList = local_18[3];
    return;
  }
  param_1 = param_1 & 0xfffffeff;
  if (DAT_005318f4 == 0) {
    param_1 = param_1 | 0x100;
  }
  ExceptionList = local_18 + 3;
  iVar2 = FUN_004afa70(DAT_0055a284,0,param_1,0x10,param_3 - 1U & ((int)(param_3 - 1U) < 0) - 1);
  if (DAT_005318f4 == 0) {
    iVar2 = iVar2 * 0x32 + DAT_005c8464;
    local_18[0] = (void *)0x41200000;
    local_18[1] = (void *)0x40a00000;
    local_18[2] = (void *)0x3f800000;
    pvVar3 = operator_new(0x28);
    local_4 = 0;
    if (pvVar3 != (void *)0x0) {
      if (DAT_00594fb8 == 0) {
        param_2 = local_18[DAT_0052f7e0._2_1_];
      }
      else {
        param_2 = (void *)0x38d1b717;
      }
      FUN_00496e80(0,(iVar2 - DAT_005c8464) / 0x32,param_2);
    }
  }
  ExceptionList = local_18[3];
  return;
}


// ==== FUN_004afa70 @ 004afa70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall
FUN_004afa70(int param_1,undefined2 param_2,undefined2 param_3,int param_4,uint param_5,
            undefined1 param_6)

{
  undefined4 uVar1;
  int *piVar2;
  
  if (param_4 == 0) {
    param_4 = 0x20;
  }
  uVar1 = 0;
  if ((param_5 & 0x20) == 0) {
    if ((param_5 & 0x10) != 0) {
      uVar1 = 2;
    }
  }
  else {
    uVar1 = 1;
  }
  piVar2 = (int *)FUN_004af530(DAT_00541088,uVar1);
  *(undefined2 *)((int)piVar2 + 0x12) = param_2;
  *(undefined2 *)(piVar2 + 3) = param_3;
  if (*(byte *)((int)piVar2 + 10) == _DAT_005411a0) {
    FUN_004214a0(((int)piVar2 - DAT_005c8464) / 0x32);
  }
  *(undefined1 *)((int)piVar2 + 0x1e) = param_6;
  *(short *)(piVar2 + 5) = (short)param_4;
  (**(code **)(*piVar2 + 0x50))((param_1 - DAT_005c8464) / 0x32,param_5);
  return ((int)piVar2 - DAT_005c8464) / 0x32;
}


// ==== FUN_004558f0 @ 004558f0

int FUN_004558f0(int param_1,int param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % (uint)(param_2 - param_1) + param_1;
}


// ==== FUN_004257c0 @ 004257c0

int FUN_004257c0(int param_1)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  
  if (DAT_005453d4 == 0) {
    piVar2 = &DAT_0052f998;
    DAT_005453d4 = 0;
    do {
      piVar1 = piVar2 + 0x12;
      iVar3 = *piVar2;
      piVar2 = piVar2 + 0x24;
      DAT_005453d4 = DAT_005453d4 + *piVar1 + iVar3;
    } while ((int)piVar2 < 0x5300e8);
  }
  iVar4 = 0;
  iVar3 = 0;
  piVar2 = &DAT_0052f9e0;
  while( true ) {
    if (param_1 % DAT_005453d4 <= iVar4 + piVar2[-0x12]) {
      return iVar3;
    }
    iVar4 = iVar4 + piVar2[-0x12] + *piVar2;
    if (param_1 % DAT_005453d4 <= iVar4) break;
    piVar2 = piVar2 + 0x24;
    iVar3 = iVar3 + 2;
    if (0x53012f < (int)piVar2) {
      return iVar3;
    }
  }
  return iVar3 + 1;
}


// ==== FUN_004af530 @ 004af530

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * FUN_004af530(int param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  undefined1 uVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  if (param_1 == DAT_005412e0) {
    FUN_004e0620("type != fakeThreeByThreeSurfaceType","\\Ns\\O\\Squid.cpp",0x189);
  }
  iVar1 = FUN_004af1d0(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar1 < 0x59d9) goto LAB_004af5a1;
    uVar6 = 0x194;
    pcVar5 = "sid < FIRST_PREDICTABLE_SID";
  }
  else {
    if (23000 < iVar1) goto LAB_004af5a1;
    uVar6 = 0x191;
    pcVar5 = "sid >= FIRST_PREDICTABLE_SID";
  }
  FUN_004e0620(pcVar5,"\\Ns\\O\\Squid.cpp",uVar6);
LAB_004af5a1:
  iVar2 = FUN_0049a840(param_1);
  uVar4 = (undefined1)param_1;
  if (*(code **)(iVar2 + 0x1d0) != (code *)0x0) {
    piVar3 = (int *)(**(code **)(iVar2 + 0x1d0))(iVar1);
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  piVar3 = (int *)(iVar1 * 0x32 + DAT_005c8464);
  if (piVar3 != (int *)0x0) {
    *piVar3 = (int)&PTR_LAB_00501dd0;
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  uRam0000000a = uVar4;
  (**(code **)(_DAT_00000000 + 0x28))();
  return (int *)0x0;
}


// ==== FUN_004aea40 @ 004aea40

undefined4 * FUN_004aea40(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00512208;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004437f0 @ 004437f0

void FUN_004437f0(void)

{
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


