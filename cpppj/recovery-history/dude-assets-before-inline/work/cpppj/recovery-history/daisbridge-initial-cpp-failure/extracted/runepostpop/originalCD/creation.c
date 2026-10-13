// ==== FUN_00484520 @ 00484520

void __thiscall FUN_00484520(void *this,uint param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined **local_58 [13];
  int local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00484620;
  *unaff_FS_OFFSET = &local_10;
  if ((param_2 & 1) != 0) {
    local_14 = FUN_004f1650(0x28);
    local_8 = 0;
    if (local_14 != 0) {
      FUN_0048efd0(local_14,0x304,((int)this - DAT_005395dc) / 0x24,0x3c23d70a);
    }
  }
  local_8 = 0xffffffff;
  if ((param_2 & 2) != 0) {
    local_14 = *(int *)((int)this + 0x12);
    local_58[0] = &PTR_FUN_005003d0;
    local_18 = *(undefined4 *)((int)this + 0xe);
    local_20 = 0;
    local_1c = 0;
    FUN_00440a80(&local_18);
    iVar2 = __ftol();
    iVar3 = __ftol();
    FUN_004eae20(local_58,iVar3 + -1,iVar2 + -1,iVar3 + 1,iVar2 + 1);
    iVar2 = DAT_005395dc;
    while (DAT_005395dc = iVar2, local_24 != 0) {
      if ((*(byte *)(local_24 * 0x24 + 10 + iVar2) == DAT_0051cbd8) &&
         (*(char *)((int)this + 0x20) == *(char *)(local_24 * 0x24 + iVar2 + 0x20)))
      goto LAB_0048463e;
      FUN_004eafe0(local_58);
      iVar2 = DAT_005395dc;
    }
    local_24 = 0;
LAB_0048463e:
    uVar1 = local_24 * 0x24 + iVar2;
    if ((iVar2 + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + iVar2))) {
      local_14 = FUN_00482990();
      FUN_004d06a0(((int)this - iVar2) / 0x24,(float)local_14);
    }
  }
  FUN_004ae180(this,param_2);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00482990 @ 00482990

int __thiscall FUN_00482990(void *this)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = FUN_004816a0();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if (((uVar1 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)) ||
     (*(ushort *)(uVar1 + 0xc) == 0)) {
    return 10;
  }
  if (DAT_0052e170 != 0) {
    if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
      return 10;
    }
    if (DAT_0052e170 != 0) {
      return 10;
    }
  }
  return (-(uint)((*(uint *)((uint)*(ushort *)(uVar1 + 0xc) * 0xac + DAT_0050f834 + 0x50) & 1) == 0)
         & 0xffffffee) + 0x19;
}


// ==== FUN_004816a0 @ 004816a0

void __thiscall FUN_004816a0(void *this)

{
  FUN_004eb4b0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),DAT_0051cbd4);
  return;
}


