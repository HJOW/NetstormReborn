// ==== FUN_00407400 @ 00407400

int __cdecl FUN_00407400(int param_1)

{
  int iVar1;
  int *piVar2;
  undefined4 unaff_ESI;
  
  if (DAT_0050f828 == 0) {
    FUN_00490040(s_playerIdKnown_0050fb44,s__ns_o_Player_cpp_0050fb30,0x301);
  }
  iVar1 = FUN_004eb4b0((float)(param_1 + 10),
                       (float)(int)((-(uint)(DAT_00537e8c == 0) & 0xfffffff7) + 10),DAT_0051cba4);
  if (iVar1 != 0) {
    FUN_00490040(s__findSquid_bc_playerType__0050fb68,s__ns_o_Player_cpp_0050fb54,0x304);
  }
  piVar2 = (int *)FUN_004ab390(DAT_0051cba4,2);
  (**(code **)(*piVar2 + 0x74))(param_1);
  *(undefined1 *)((int)piVar2 + 0x22) = 0;
  (**(code **)(*piVar2 + 0x90))(unaff_ESI,(float)(param_1 + 10),0);
  return ((int)piVar2 - DAT_005395dc) / 0x24;
}


// ==== FUN_00407cc0 @ 00407cc0

void __thiscall FUN_00407cc0(void *this,uint param_2)

{
  byte bVar1;
  int iVar2;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_00407d94;
  *unaff_FS_OFFSET = &local_10;
  bVar1 = *(byte *)((int)this + 0x20);
  *(int *)((uint)bVar1 * 0xac + 0x94 + DAT_0050f834) = ((int)this - DAT_005395dc) / 0x24;
  if ((((param_2 & 1) != 0) && (bVar1 == DAT_0050f6c8)) && (DAT_00540a1c == 0)) {
    iVar2 = FUN_004f1650(0x28);
    local_8 = 0;
    if (iVar2 != 0) {
      FUN_0048efd0(iVar2,1,((int)this - DAT_005395dc) / 0x24,0x40a00000);
    }
  }
  local_8 = 0xffffffff;
  if (((param_2 & 1) != 0) && (DAT_00537e8c != 0)) {
    *(undefined1 *)((int)this + 0x22) = 1;
  }
  FUN_004ae180(this,param_2);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00406d90 @ 00406d90

undefined4 __cdecl FUN_00406d90(void)

{
  if (DAT_0050f828 == 0) {
    return 0;
  }
  return *(undefined4 *)(DAT_0050f6c8 * 0xac + 0x94 + DAT_0050f834);
}


// ==== FUN_00406dc0 @ 00406dc0

undefined4 __cdecl FUN_00406dc0(int param_1)

{
  if (DAT_0050f828 == 0) {
    FUN_00490040(s_playerIdKnown_0050fa98,s__ns_o_Player_cpp_0050fa84,0x24d);
  }
  return *(undefined4 *)(param_1 * 0xac + 0x94 + DAT_0050f834);
}


