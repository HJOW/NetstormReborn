// ==== FUN_004d4d00 @ 004d4d00

int __thiscall FUN_004d4d00(void *this)

{
  int iVar1;
  
  if ((*(byte *)((int)this + 10) == DAT_00541288) && ((*(byte *)((int)this + 0x28) & 8) == 0)) {
    iVar1 = FUN_004ad390();
    return iVar1 << 1;
  }
  iVar1 = FUN_004ad680();
  iVar1 = iVar1 * 2 + -0x98;
  if ((iVar1 < 0) || (7 < iVar1)) {
    iVar1 = 0;
  }
  return iVar1;
}


// ==== FUN_004ad390 @ 004ad390

undefined1 __thiscall FUN_004ad390(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (*(byte *)((int)this + 10) != DAT_005411c0)) &&
     (*(byte *)((int)this + 10) != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x739);
  }
  return *(undefined1 *)((int)this + 0x1e);
}


// ==== FUN_004ad680 @ 004ad680

int __thiscall FUN_004ad680(void *this)

{
  int iVar1;
  int iVar2;
  
  if (*(byte *)((int)this + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x854);
  }
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
}


// ==== FUN_00475290 @ 00475290

void __thiscall FUN_00475290(void *this,undefined4 param_2)

{
  FUN_004ad110(this,param_2);
  (**(code **)(*(int *)this + 0x88))();
  return;
}


// ==== FUN_004549c0 @ 004549c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_004549c0(void *this,undefined4 param_2,int param_3,undefined4 param_4)

{
  int *piVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  float10 fVar6;
  
  switch(param_2) {
  case 2:
    iVar5 = (**(code **)(*(int *)this + 0x80))();
    iVar3 = FUN_004ac550(this);
    iVar5 = *(int *)(iVar3 + 0x130 + iVar5 * 4);
    iVar3 = (**(code **)(*(int *)this + 0x80))();
    iVar4 = FUN_004ac550(this);
    uVar2 = 0;
    iVar3 = FUN_004558c0(*(undefined4 *)(iVar4 + 0x170 + iVar3 * 4),0);
    FUN_004acee0(this,iVar3 + iVar5,uVar2);
    break;
  case 3:
    if (param_3 != 0) {
      FUN_004c2630(3,"Upgrading!\n");
      FUN_004545e0(this);
      iVar5 = FUN_0040f100(this);
      if (iVar5 != 0) {
        FUN_00413b70(*(undefined1 *)((int)this + 0x22),((int)this - DAT_005c8464) / 0x32);
      }
      return (float10)_DAT_00502350;
    }
    piVar1 = (int *)FUN_004af530(*(undefined1 *)((int)this + 10),2);
    iVar5 = *piVar1;
    uVar2 = FUN_00453750(this);
    (**(code **)(iVar5 + 0x84))(uVar2);
    *(byte *)(piVar1 + 10) = *(byte *)(piVar1 + 10) | 8;
    FUN_0042b050(piVar1,1);
    FUN_004ac220(piVar1,0);
    (**(code **)(*piVar1 + 0x74))(*(undefined1 *)((int)this + 0x22));
    (**(code **)(*piVar1 + 0x90))
              (*(undefined4 *)((int)this + 0xe),*(float *)((int)this + 0x12) + _DAT_00509144,0);
    FUN_004c2630(4,"making the client upgrade picture");
    FUN_00443fe0(((int)piVar1 - DAT_005c8464) / 0x32,param_4);
    FUN_004b1510(((int)this - DAT_005c8464) / 0x32,((int)piVar1 - DAT_005c8464) / 0x32);
  case 1:
    FUN_004afc90(this,1,0);
    break;
  case 4:
    if (param_3 != 0) {
      return (float10)_DAT_00500fc4;
    }
    break;
  case 5:
    iVar5 = (**(code **)(*(int *)this + 0x80))();
    if (2 < iVar5) {
      FUN_0045a6c0(((int)this - DAT_005c8464) / 0x32,2,1);
    }
  }
  fVar6 = (float10)FUN_0044b900(this,param_2,param_3,param_4);
  return fVar6;
}


// ==== FUN_00499240 @ 00499240

void __cdecl
FUN_00499240(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,int param_6,int param_7,int param_8,int param_9)

{
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  if (DAT_0059a8b0 == 0) {
    FUN_00498ff0(&local_10,param_1,param_2,param_3,param_4);
    local_10 = local_10 + param_6;
    local_c = local_c + param_7;
    local_8 = local_8 - param_8;
    local_4 = local_4 - param_9;
    FUN_00445290(&local_10,DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
    if ((local_8 != local_10 && -1 < local_8 - local_10) &&
       (local_4 != local_c && -1 < local_4 - local_c)) {
      FUN_00497580(DAT_0059a8bc,&local_10,param_5);
    }
  }
  return;
}


