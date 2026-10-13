// ==== FUN_004297f0 @ 004297f0

void __thiscall FUN_004297f0(void *this)

{
  undefined4 uVar1;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_00429868;
  DAT_005178d4 = DAT_005178d4 + 1;
  *unaff_FS_OFFSET = &local_10;
  local_8 = 0;
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_00518154,s__ns_o_template_cpp_00518140,0x4a7);
  }
  uVar1 = FUN_0044c4e0(*(undefined4 *)this,s_Money_00518158,0);
  FUN_00414720(uVar1);
  local_8 = 0xffffffff;
  FUN_00429872();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00414720 @ 00414720

void __cdecl FUN_00414720(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 4) + 4;
  *(int *)(param_1 + 4) = iVar1;
  if (*(int *)(param_1 + 8) < iVar1) {
    FUN_00490040(s_pos<_end_00512100,s__Ns_o_memstream_h_005120ec,0x2e);
  }
  if (DAT_00540a1c != 0) {
    DAT_005120d4 = 100000.0;
  }
  if (((DAT_00540a20 != 0) && (DAT_0051894c == 0)) &&
     ((*(byte *)(DAT_0050f6c8 * 0xac + 0x50 + DAT_0050f834) & 0x20) == 0)) {
    DAT_005120d4 = (float)DAT_005120d0;
  }
  return;
}


// ==== FUN_00429872 @ 00429872

void __cdecl FUN_00429872(void)

{
  FUN_00429f30();
  return;
}


// ==== FUN_00429f30 @ 00429f30

void __cdecl FUN_00429f30(void)

{
  DAT_005178d4 = DAT_005178d4 + -1;
  return;
}


