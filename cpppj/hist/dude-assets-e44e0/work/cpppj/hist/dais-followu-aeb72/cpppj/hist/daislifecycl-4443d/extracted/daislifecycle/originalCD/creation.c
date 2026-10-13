// ==== FUN_004836b0 @ 004836b0

void __thiscall FUN_004836b0(void *this,uint param_2)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  int *piVar7;
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  undefined4 uStack_54;
  undefined **appuStack_50 [13];
  int iStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  undefined1 local_10 [16];
  
  if ((param_2 & 0x400000) == 0) {
    iVar1 = FUN_00483c50();
    piVar7 = (int *)(iVar1 * 0x24 + DAT_005395dc);
    if ((((int *)(DAT_005395dc + 0x24U) <= piVar7) &&
        (piVar7 < (int *)(DAT_005395f4 * 0x24 + DAT_005395dc))) &&
       ((*(byte *)((int)piVar7 + 0xb) & 2) == 0)) {
      (**(code **)(*piVar7 + 0x10))(param_2);
    }
  }
  puVar2 = (undefined4 *)FUN_004aeec0(this,local_10,0);
  uStack_60 = *puVar2;
  uStack_5c = puVar2[1];
  uStack_58 = puVar2[2];
  uStack_54 = puVar2[3];
  uStack_18 = 0;
  appuStack_50[0] = &PTR_FUN_005003d0;
  uStack_14 = 0;
  FUN_00440a50(&uStack_60);
  FUN_00440a80(&uStack_58);
  uVar3 = __ftol();
  uVar4 = __ftol(uVar3);
  uVar5 = __ftol(uVar4);
  uVar6 = __ftol(uVar5);
  FUN_004eae20(appuStack_50,uVar6,uVar5,uVar4,uVar3);
  while (iStack_1c != 0) {
    piVar7 = (int *)(DAT_005395dc + iStack_1c * 0x24);
    if (((*(byte *)((int)piVar7 + 10) == DAT_0051cbdc) && ((*(byte *)((int)piVar7 + 0x23) & 1) != 0)
        ) && ((DAT_00540a2c != 0 ||
              ((iVar1 = (iStack_1c * 0x24) / 0x24, 4 < iVar1 && (iVar1 < 6000)))))) {
      (**(code **)(*piVar7 + 0x10))(0);
    }
    FUN_004eafe0(appuStack_50);
  }
  FUN_004615e0(this,param_2);
  return;
}


// ==== FUN_004835c0 @ 004835c0

void __thiscall FUN_004835c0(void *this,uint param_2)

{
  int iVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_00483646;
  *unaff_FS_OFFSET = &local_10;
  if ((param_2 & 1) != 0) {
    iVar1 = FUN_004f1650(0x28);
    local_8 = 0;
    if (iVar1 != 0) {
      FUN_0048efd0(iVar1,0x15,((int)this - DAT_005395dc) / 0x24,DAT_0052e344);
    }
  }
  local_8 = 0xffffffff;
  FUN_004621c0(this,param_2);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00483660 @ 00483660

void __thiscall FUN_00483660(void *this,undefined4 param_2)

{
  FUN_00461b40(this,param_2);
  return;
}


// ==== FUN_00481360 @ 00481360

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00481360(void *this,uint param_2)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  
  iVar7 = DAT_0050f834;
  if (DAT_0052e170 == 0) {
    if (((DAT_00564dbc & 0x7fffffff) == 0 && _DAT_00564db8 == 0) && ((param_2 & 7) != 0)) {
      FUN_004acbc0(this,*(undefined4 *)
                         (&DAT_0052e2b8 +
                         (uint)*(byte *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0x69 +
                                        DAT_0050f834) * 4),0);
    }
    bVar1 = *(byte *)((int)this + 0x20);
    cVar2 = (**(code **)(*(int *)this + 0x80))();
    iVar7 = DAT_0050f834;
    *(char *)((uint)bVar1 * 0xac + 0x69 + DAT_0050f834) = cVar2 + -1;
  }
  iVar5 = DAT_005395dc;
  if (((param_2 & 2) != 0) && (*(byte *)((int)this + 0x20) != 0)) {
    iVar6 = (uint)*(byte *)((int)this + 0x20) * 0xac + iVar7;
    iVar3 = ((int)this - DAT_005395dc) / 0x24;
    *(int *)(iVar6 + 0x8c) = iVar3;
    *(int *)(iVar6 + 0x90) = iVar3;
  }
  if (((param_2 & 1) != 0) && (*(byte *)((int)this + 0x20) != 0)) {
    iVar7 = (uint)*(byte *)((int)this + 0x20) * 0xac + iVar7;
    iVar3 = ((int)this - iVar5) / 0x24;
    *(int *)(iVar7 + 0x84) = iVar3;
    *(int *)(iVar7 + 0x88) = iVar3;
  }
  if ((param_2 & 1) != 0) {
    if (DAT_00540a2c != 0) {
      if ((DAT_005178d4 != 0) && (iVar7 = FUN_004812a0(this,0xffffffff), iVar7 != 0)) {
        (**(code **)(*(int *)this + 0x48))(0);
        iVar7 = (**(code **)(*(int *)this + 0x80))();
        *(char *)((int)this + 0x22) = s_MissingMission__s__0052e2a4[iVar7 * 4 + 0x10];
        (**(code **)(*(int *)this + 0x4c))(0);
      }
      piVar4 = (int *)FUN_004ab390(DAT_0051cbd4,0);
      (**(code **)(*piVar4 + 0x74))(*(undefined1 *)((int)this + 0x20));
      (**(code **)(*piVar4 + 0x90))
                (*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),0);
      iVar5 = DAT_005395dc;
    }
    if ((*(byte *)((int)this + 0x20) == DAT_0050f6c8) && (DAT_005178d4 == 0)) {
      iVar7 = __ftol();
      iVar3 = __ftol();
      if ((iVar7 < 0) || (((iVar3 < 0 || (0xff < iVar7)) || (0xff < iVar3)))) {
        uVar8 = 0;
      }
      else {
        uVar8 = (uint)*(ushort *)(DAT_005670cc + (iVar3 * 0x100 + iVar7) * 2);
      }
      uVar8 = uVar8 * 0x24 + iVar5;
      if ((((iVar5 + 0x24U <= uVar8) && (uVar8 < (uint)(DAT_005395f4 * 0x24 + iVar5))) &&
          ((iVar7 = FUN_004acad0(uVar8), iVar7 != 0 &&
           ((iVar7 = *(int *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0x70 + DAT_0050f834),
            iVar5 = FUN_004acaa0(uVar8), iVar7 != iVar5 &&
            (iVar7 = FUN_004a9400(s_noShowWarnCantUpgrade_0052e4ac), iVar7 == 0)))))) &&
         (DAT_0052e170 == 0)) {
        FUN_004c3dc0(s_WarnCantUpgradeAltarOffHomeIslan_0052e4c4);
      }
    }
  }
  FUN_004621c0(this,param_2);
  return;
}


// ==== FUN_00481620 @ 00481620

void __thiscall FUN_00481620(void *this,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  
  if (*(byte *)((int)this + 0x20) != 0) {
    iVar2 = DAT_0050f834 + (uint)*(byte *)((int)this + 0x20) * 0xac;
    iVar1 = ((int)this - DAT_005395dc) / 0x24;
    if (*(int *)(iVar2 + 0x8c) == iVar1) {
      *(undefined4 *)(iVar2 + 0x8c) = 0;
      *(undefined4 *)(iVar2 + 0x90) = 0;
    }
    if (*(int *)(iVar2 + 0x84) == iVar1) {
      *(undefined4 *)(iVar2 + 0x84) = 0;
      *(undefined4 *)(iVar2 + 0x88) = 0;
    }
  }
  FUN_00461b40(this,param_2);
  return;
}


