// ==== FUN_0044ee30 @ 0044ee30

undefined4 __thiscall FUN_0044ee30(void *this,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined **local_64 [13];
  int local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0044efb8;
  *unaff_FS_OFFSET = &local_10;
  if (0x20 < param_2) {
    param_2 = 0x40;
  }
  local_24 = *(undefined4 *)((int)this + 0xe);
  uVar1 = FUN_004bdc50(&DAT_00583188,local_24,*(undefined4 *)((int)this + 0x12));
  local_24 = *(undefined4 *)((int)this + 0xe);
  local_20 = *(undefined4 *)((int)this + 0x12);
  local_64[0] = &PTR_FUN_005003d0;
  local_2c = 0;
  local_28 = 0;
  FUN_00440a80(&local_24);
  iVar2 = __ftol();
  iVar3 = __ftol();
  FUN_004eae20(local_64,iVar3 - param_2,iVar2 - param_2,param_2 + iVar3,param_2 + iVar2);
  local_1c = FUN_004f1650(0x50);
  local_18 = 0x14;
  local_14 = 0;
  local_8 = 1;
  while (local_30 != 0) {
    iVar2 = FUN_0044e230(local_30,uVar1,*(undefined1 *)((int)this + 0x20));
    if (iVar2 != 0) {
      if (local_14 < local_18) {
        *(int *)(local_1c + local_14 * 4) = (local_30 * 0x24) / 0x24;
        local_14 = local_14 + 1;
      }
      if (local_18 <= local_14) break;
    }
    FUN_004eafe0(local_64);
  }
  if (local_14 == 0) {
    if (param_2 == 0x40) {
      local_8 = 0xffffffff;
      FUN_0044efc2();
      uVar1 = 0;
      goto LAB_0044ef87;
    }
  }
  else {
    iVar2 = FUN_0048cbd0(local_14);
    (**(code **)(*(int *)(*(int *)(local_1c + iVar2 * 4) * 0x24 + DAT_005395dc) + 0x10))(0x100000);
  }
  local_8 = 0xffffffff;
  FUN_0044efc2();
  uVar1 = 1;
LAB_0044ef87:
  *unaff_FS_OFFSET = local_10;
  return uVar1;
}


// ==== FUN_0044efc2 @ 0044efc2

void __cdecl FUN_0044efc2(void)

{
  int unaff_EBP;
  
  FUN_00407c50(unaff_EBP + -0x18);
  return;
}


// ==== FUN_0044e230 @ 0044e230

bool __cdecl FUN_0044e230(int param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  int iVar8;
  bool bVar9;
  
  bVar7 = false;
  iVar8 = DAT_005395dc + param_1 * 0x24;
  uVar1 = *(undefined4 *)(iVar8 + 0x12);
  uVar2 = *(undefined4 *)(iVar8 + 0xe);
  uVar3 = FUN_004abae0(iVar8);
  if ((uVar3 & 0x128b302) != 0) {
    return false;
  }
  iVar4 = FUN_004bdc50(&DAT_00583188,uVar2,uVar1);
  if (iVar4 != param_2) {
    return false;
  }
  iVar5 = __ftol();
  iVar6 = __ftol();
  iVar4 = iVar6 * 0x100 + iVar5;
  if ((*(byte *)(DAT_0052fe48 + iVar4) & 2) == 0) {
    uVar3 = FUN_004abac0(iVar8);
    if ((uVar3 & 0x200000) == 0) goto LAB_0044e30b;
  }
  else {
    if ((((iVar5 < 0) || (iVar6 < 0)) || (0xff < iVar5)) || (0xff < iVar6)) {
      uVar3 = 0;
    }
    else {
      uVar3 = (uint)*(ushort *)(DAT_0052d590 + iVar4 * 2);
    }
    if ((uVar3 == 0) || (*(byte *)(uVar3 * 0x24 + 10 + DAT_005395dc) == DAT_0051cbb8))
    goto LAB_0044e30b;
  }
  bVar7 = true;
LAB_0044e30b:
  bVar9 = false;
  if (bVar7) {
    bVar9 = *(byte *)(iVar8 + 0x20) != param_3;
  }
  return bVar9;
}


// ==== FUN_004bdba0 @ 004bdba0

int __thiscall FUN_004bdba0(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  iVar3 = 0;
  if (0 < *(int *)this) {
    do {
      iVar1 = FUN_004bd4c0(this,iVar3);
      if (((iVar1 != 0) && (iVar1 = FUN_004bda70(this,iVar3), iVar1 != param_2)) &&
         (iVar1 = FUN_004bda70(this,iVar3), iVar1 != 0)) {
        iVar2 = iVar2 + 1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < *(int *)this);
  }
  return iVar2;
}


// ==== FUN_004bda70 @ 004bda70

undefined4 __thiscall FUN_004bda70(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 == 0) {
    return 0;
  }
  return *(undefined4 *)(param_2 * 0x20 + 0xc + (int)this);
}


// ==== FUN_004bd4c0 @ 004bd4c0

undefined4 __thiscall FUN_004bd4c0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd480(param_2);
  if (iVar1 != 0) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __thiscall FUN_004abac0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_00440a80 @ 00440a80

void __thiscall FUN_00440a80(void *this)

{
  int iVar1;
  
  iVar1 = __ftol();
  *(float *)this = (float)iVar1;
  iVar1 = __ftol();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


