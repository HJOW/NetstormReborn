// ==== FUN_004b15f0 @ 004b15f0

void __cdecl FUN_004b15f0(undefined4 param_1,int param_2)

{
  if (*(int *)(param_2 + 4) == 0) {
    FUN_004e0620("process->formSid","\\Ns\\O\\Squid.cpp",0xb3c);
  }
  FUN_004b1510();
  return;
}


// ==== FUN_004ae280 @ 004ae280

void __thiscall FUN_004ae280(void *this,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540bc0 != 0) {
    piVar1 = (int *)FUN_00471aa0(*(undefined4 *)((int)this + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_004b1510 @ 004b1510

void __cdecl FUN_004b1510(int param_1,int param_2)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  undefined4 uVar4;
  
  if (((param_1 < 5) || (14999 < param_1)) && ((param_2 < 5 || (14999 < param_2)))) {
    uVar4 = 0;
    if (DAT_00540bc0 == 0) {
      FUN_004e0620("server","\\Ns\\O\\Squid.cpp",0xb29);
    }
  }
  else {
    uVar4 = 2;
  }
  piVar1 = (int *)FUN_004af530(DAT_00541074,uVar4);
  piVar2 = (int *)FUN_004af530(DAT_00541074,uVar4);
  iVar3 = (int)piVar2 - DAT_005c8464;
  *(short *)(piVar1 + 5) =
       ((short)(iVar3 / 0x32) + (short)(iVar3 >> 0x1f)) -
       (short)((longlong)iVar3 * 0x51eb851f >> 0x3f);
  *(undefined2 *)((int)piVar1 + 0x12) = 0;
  iVar3 = (int)piVar1 - DAT_005c8464;
  *(short *)((int)piVar2 + 0x12) =
       ((short)(iVar3 / 0x32) + (short)(iVar3 >> 0x1f)) -
       (short)((longlong)iVar3 * 0x51eb851f >> 0x3f);
  *(undefined2 *)(piVar2 + 5) = 0;
  (**(code **)(*piVar1 + 0x74))(param_2,0);
  (**(code **)(*piVar2 + 0x74))(param_1,0);
  return;
}


// ==== FUN_004ae780 @ 004ae780

void __cdecl FUN_004ae780(int param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  int *piVar2;
  uint uVar3;
  uint uVar4;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  uVar3 = param_2 * 0x32 + DAT_005c8464;
  uVar4 = param_1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_2 * 0x32 + 0xb + DAT_005c8464) & 2) == 0) {
    if ((uVar4 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar4)) {
      FUN_004e0620("watcher.isValid()","\\Ns\\O\\Squid.cpp",0xb48);
    }
    if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3)) {
      FUN_004e0620("watched.isValid()","\\Ns\\O\\Squid.cpp",0xb49);
    }
    if (*(byte *)(uVar4 + 10) == DAT_00541074) {
      FUN_004e0620("watcher.getType() != dependFormType","\\Ns\\O\\Squid.cpp",0xb4b);
    }
    if (*(byte *)(uVar3 + 10) < 0x46) {
      FUN_004e0620("!watched.isForm()","\\Ns\\O\\Squid.cpp",0xb4c);
    }
    if (param_1 == param_2) {
      FUN_004e0620("watcherSid != watchedSid","\\Ns\\O\\Squid.cpp",0xb4e);
    }
    local_10[0] = &PTR_FUN_005019e8;
    local_4 = 0;
    FUN_004b22d0(local_10,param_2);
    while (local_8 != 0) {
      iVar1 = FUN_004b1bd0(local_10);
      if (iVar1 != 0) {
        iVar1 = FUN_004b2390(local_10);
        if (*(ushort *)(iVar1 + 0x12) == 0) {
          iVar1 = 0;
        }
        else {
          iVar1 = (*(code *)**(undefined4 **)((uint)*(ushort *)(iVar1 + 0x12) * 0x32 + DAT_005c8464)
                  )();
        }
        if (iVar1 == param_1) {
          piVar2 = (int *)FUN_004b2390(local_10);
          (**(code **)(*piVar2 + 0x10))(param_3);
        }
      }
      FUN_004b1b80(local_10);
    }
  }
  return;
}


