// ==== FUN_004ad710 @ 004ad710

int __thiscall FUN_004ad710(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4) + -0x41;
}


// ==== FUN_004b22d0 @ 004b22d0

void __thiscall FUN_004b22d0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((DAT_005e4794 != 0) && (DAT_00594fc8 == 0)) && ((*(byte *)(iVar1 + 0xb) & 2) != 0)) &&
     ((DAT_00540bc4 == 0 && (*(int *)((int)this + 0xc) == 0)))) {
    FUN_004e0620("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy"
                 ,"\\Ns\\O\\Squidfinder.cpp",0x158);
  }
  *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 + 6);
  FUN_004b1b80(this);
  return;
}


// ==== FUN_004b1b80 @ 004b1b80

void __thiscall FUN_004b1b80(void *this)

{
  int iVar1;
  
  do {
    do {
      iVar1 = *(int *)((int)this + 4);
      *(int *)((int)this + 8) = iVar1;
      if (iVar1 == 0) {
        *(undefined4 *)((int)this + 4) = 0;
        *(undefined4 *)((int)this + 8) = 0;
        return;
      }
      *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 * 0x32 + 4 + DAT_005c8464);
      iVar1 = *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
    } while ((iVar1 != 0) && (0x45 < *(byte *)(iVar1 + 10)));
    iVar1 = (*(code *)**(undefined4 **)this)(iVar1);
  } while (iVar1 == 0);
  return;
}


// ==== FUN_004b2390 @ 004b2390

int __thiscall FUN_004b2390(void *this)

{
  if ((*(byte *)(*(int *)((int)this + 8) * 0x32 + 10 + DAT_005c8464) != DAT_00541074) &&
     (*(int *)((int)this + 8) != 0)) {
    FUN_004e0620("isDepend() || sid == 0","\\Ns\\O\\Squidfinder.cpp",0x170);
  }
  return *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
}


// ==== FUN_004b1bd0 @ 004b1bd0

bool __thiscall FUN_004b1bd0(void *this)

{
  return *(byte *)(*(int *)((int)this + 8) * 0x32 + 10 + DAT_005c8464) == DAT_00541074;
}


// ==== FUN_00453740 @ 00453740

void FUN_00453740(void)

{
  FUN_004b1bd0();
  return;
}


// ==== FUN_004ae510 @ 004ae510

undefined4 __thiscall FUN_004ae510(void *this)

{
  undefined4 uVar1;
  
  if (*(ushort *)((int)this + 0x12) != 0) {
                    /* WARNING: Could not recover jumptable at 0x004ae528. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    uVar1 = (*(code *)**(undefined4 **)((uint)*(ushort *)((int)this + 0x12) * 0x32 + DAT_005c8464))
                      ();
    return uVar1;
  }
  return 0;
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


// ==== FUN_0041c5b0 @ 0041c5b0

undefined4 __thiscall FUN_0041c5b0(void *this,int param_2)

{
  undefined4 local_60 [24];
  
  local_60[0] = 0;
  local_60[1] = 1;
  local_60[2] = 2;
  local_60[3] = 3;
  local_60[4] = 4;
  local_60[5] = 5;
  local_60[6] = 6;
  local_60[7] = 7;
  local_60[8] = 8;
  local_60[9] = 9;
  local_60[10] = 10;
  local_60[0xb] = 0xf;
  local_60[0xc] = 0x14;
  local_60[0xd] = 0x19;
  local_60[0xe] = 0x1e;
  local_60[0xf] = 0x23;
  local_60[0x10] = 0x28;
  local_60[0x11] = 0x2d;
  local_60[0x12] = 0x32;
  local_60[0x13] = 0x3c;
  local_60[0x14] = 0x46;
  local_60[0x15] = 0x50;
  local_60[0x16] = 0x5a;
  local_60[0x17] = 100;
  if ((-1 < param_2) && (param_2 < 8)) {
    return local_60[*(byte *)(param_2 + 0x4f + (int)this)];
  }
  return 0;
}


