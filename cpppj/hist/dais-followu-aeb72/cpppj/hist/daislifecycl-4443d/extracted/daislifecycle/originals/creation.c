// ==== FUN_0044a4d0 @ 0044a4d0

void __thiscall FUN_0044a4d0(void *this,uint param_2)

{
  int iVar1;
  int *piVar2;
  undefined4 *puVar3;
  undefined1 local_50 [16];
  undefined1 auStack_40 [52];
  int iStack_c;
  
  if ((param_2 & 0x400000) == 0) {
    iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         DAT_005412ec);
    piVar2 = (int *)(iVar1 * 0x32 + DAT_005c8464);
    if ((((int *)(DAT_005c8464 + 0x32U) <= piVar2) &&
        (piVar2 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
       ((*(byte *)((int)piVar2 + 0xb) & 2) == 0)) {
      (**(code **)(*piVar2 + 0x10))(param_2);
    }
  }
  puVar3 = (undefined4 *)FUN_004ade70(this,local_50,0);
  FUN_0041def0(auStack_40,*puVar3,puVar3[1],puVar3[2],puVar3[3],0);
  while (iStack_c != 0) {
    iVar1 = iStack_c * 0x32;
    if (((*(byte *)(DAT_005c8464 + 10 + iVar1) == DAT_005412f0) &&
        ((*(byte *)((int *)(DAT_005c8464 + iVar1) + 10) & 1) != 0)) &&
       ((DAT_00540bc4 != 0 || ((4 < iVar1 / 0x32 && (iVar1 / 0x32 < 15000)))))) {
      (**(code **)(*(int *)(DAT_005c8464 + iVar1) + 0x10))(0);
    }
    FUN_004b1810(auStack_40);
  }
  FUN_0044b4b0(this,param_2);
  return;
}


// ==== FUN_00448ad0 @ 00448ad0

void __thiscall FUN_00448ad0(void *this,uint param_2)

{
  void *pvVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f5b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if ((param_2 & 1) != 0) {
    ExceptionList = &local_c;
    pvVar1 = operator_new(0x28);
    local_4 = 0;
    if (pvVar1 != (void *)0x0) {
      FUN_00496e80(pvVar1,0x15,((int)this - DAT_005c8464) / 0x32,DAT_00532698);
    }
  }
  local_4 = 0xffffffff;
  FUN_0044bf80(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== thunk_FUN_0044b9e0 @ 00448b60

void __thiscall thunk_FUN_0044b9e0(void)

{
  FUN_0044b9e0();
  return;
}


// ==== FUN_00449320 @ 00449320

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00449320(void *this,uint param_2)

{
  char cVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  
  if (DAT_00594fa4 == 0) {
    if ((_DAT_005c85f0 == _DAT_00500460) && ((param_2 & 7) != 0)) {
      FUN_004acee0(this,(&DAT_00532624)
                        [*(byte *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x71 + DAT_0059534c)],
                   0);
    }
    iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c;
    cVar1 = (**(code **)(*(int *)this + 0x80))();
    *(char *)(iVar4 + 0x71) = cVar1 + -1;
  }
  if (((param_2 & 2) != 0) && (*(byte *)((int)this + 0x22) != 0)) {
    *(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x94 + DAT_0059534c) =
         ((int)this - DAT_005c8464) / 0x32;
    iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4;
    *(undefined4 *)(iVar4 + DAT_0059534c + 0x98) = *(undefined4 *)(iVar4 + 0x94 + DAT_0059534c);
  }
  if (((param_2 & 1) != 0) && (*(byte *)((int)this + 0x22) != 0)) {
    *(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x8c + DAT_0059534c) =
         ((int)this - DAT_005c8464) / 0x32;
    iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4;
    *(undefined4 *)(iVar4 + DAT_0059534c + 0x90) = *(undefined4 *)(iVar4 + 0x8c + DAT_0059534c);
  }
  if ((param_2 & 1) != 0) {
    if (DAT_00540bc4 != 0) {
      if (DAT_005c89b8 != 0) {
        iVar4 = FUN_00447890(this,0xffffffff);
        if (iVar4 != 0) {
          (**(code **)(*(int *)this + 0x48))(0);
          iVar4 = (**(code **)(*(int *)this + 0x80))();
          *(undefined4 *)((int)this + 0x24) = *(undefined4 *)(&DAT_00532620 + iVar4 * 4);
          (**(code **)(*(int *)this + 0x4c))(0);
        }
      }
      piVar2 = (int *)FUN_004af530(DAT_005412e8,0);
      (**(code **)(*piVar2 + 0x74))(*(undefined1 *)((int)this + 0x22));
      (**(code **)(*piVar2 + 0x90))
                (*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),0);
    }
    if ((*(byte *)((int)this + 0x22) == _DAT_00540c70) && (DAT_005c89b8 == 0)) {
      iVar4 = FUN_0040eaf0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
      uVar5 = iVar4 * 0x32 + DAT_005c8464;
      if ((DAT_005c8464 + 0x32U <= uVar5) && (uVar5 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
        iVar4 = FUN_004acdf0(uVar5);
        if (iVar4 != 0) {
          iVar3 = (uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c;
          iVar4 = FUN_004acda0(uVar5);
          if (iVar4 != *(int *)(iVar3 + 0x78)) {
            iVar4 = FUN_00441090("noShowWarnCantUpgrade");
            if ((iVar4 == 0) && (DAT_00594fa4 == 0)) {
              FUN_004cf960("WarnCantUpgradeAltarOffHomeIsland");
            }
          }
        }
      }
    }
  }
  FUN_0044bf80(this,param_2);
  return;
}


// ==== FUN_00449580 @ 00449580

void __fastcall FUN_00449580(int param_1)

{
  int iVar1;
  
  if (*(byte *)(param_1 + 0x22) != 0) {
    if (*(int *)((uint)*(byte *)(param_1 + 0x22) * 0xb4 + 0x94 + DAT_0059534c) ==
        (param_1 - DAT_005c8464) / 0x32) {
      *(undefined4 *)((uint)*(byte *)(param_1 + 0x22) * 0xb4 + 0x94 + DAT_0059534c) = 0;
      iVar1 = (uint)*(byte *)(param_1 + 0x22) * 0xb4 + DAT_0059534c;
      *(undefined4 *)(iVar1 + 0x98) = *(undefined4 *)(iVar1 + 0x94);
    }
    if (*(int *)((uint)*(byte *)(param_1 + 0x22) * 0xb4 + 0x8c + DAT_0059534c) ==
        (param_1 - DAT_005c8464) / 0x32) {
      *(undefined4 *)((uint)*(byte *)(param_1 + 0x22) * 0xb4 + 0x8c + DAT_0059534c) = 0;
      iVar1 = (uint)*(byte *)(param_1 + 0x22) * 0xb4 + DAT_0059534c;
      *(undefined4 *)(iVar1 + 0x90) = *(undefined4 *)(iVar1 + 0x8c);
    }
  }
  FUN_0044b9e0();
  return;
}


