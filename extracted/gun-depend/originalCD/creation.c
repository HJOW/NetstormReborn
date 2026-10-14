// ==== FUN_00434aa0 @ 00434aa0

void __thiscall FUN_00434aa0(void *this)

{
  int iVar1;
  
  if (DAT_00540a28 == 0) {
    FUN_00490040(s_server_0051a420,s__ns_o_GunProcess_cpp_0051a408,0x1a9);
  }
  iVar1 = (int)this + 0x28;
  if (this == (void *)0x0) {
    iVar1 = 0;
  }
  FUN_004afa10(*(undefined2 *)((int)this + 0x18),iVar1);
  return;
}


// ==== FUN_004afa10 @ 004afa10

void __cdecl FUN_004afa10(undefined4 param_1,int param_2)

{
  if (*(int *)(param_2 + 4) == 0) {
    FUN_00490040(s_process_>formSid_0053a818,s__ns_o_Squid_cpp_0053a808,0xaf3);
  }
  FUN_004af940(param_1,*(undefined4 *)(param_2 + 4));
  return;
}


// ==== FUN_004af490 @ 004af490

void __thiscall FUN_004af490(int param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540a28 != 0) {
    piVar1 = (int *)FUN_004ed640(*(undefined4 *)(param_1 + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_004afa50 @ 004afa50

void __cdecl FUN_004afa50(int param_1,int param_2,undefined4 param_3)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  int *piVar5;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  uVar2 = param_2 * 0x24 + DAT_005395dc;
  if ((*(byte *)(uVar2 + 0xb) & 2) == 0) {
    if ((uVar1 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)) {
      FUN_00490040(s_watcher_isValid___0053a83c,s__ns_o_Squid_cpp_0053a82c,0xaff);
    }
    if ((uVar2 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar2)) {
      FUN_00490040(s_watched_isValid___0053a860,s__ns_o_Squid_cpp_0053a850,0xb00);
    }
    if (*(byte *)(uVar1 + 10) == DAT_0051c964) {
      FUN_00490040(s_watcher_getType______dependFormT_0053a884,s__ns_o_Squid_cpp_0053a874,0xb02);
    }
    if (*(byte *)(uVar2 + 10) < 0x46) {
      FUN_00490040(s__watched_isForm___0053a8b8,s__ns_o_Squid_cpp_0053a8a8,0xb03);
    }
    if (param_2 == param_1) {
      FUN_00490040(s_watcherSid____watchedSid_0053a8dc,s__ns_o_Squid_cpp_0053a8cc,0xb05);
    }
    local_4 = 0;
    local_10[0] = &PTR_FUN_00502184;
    FUN_004eb960(local_10,param_2);
    while (local_8 != 0) {
      iVar3 = FUN_004eba70(local_10);
      if (iVar3 != 0) {
        uVar4 = FUN_004eba30(local_10);
        iVar3 = FUN_004af700(uVar4);
        if (iVar3 == param_1) {
          piVar5 = (int *)FUN_004eba30(local_10);
          (**(code **)(*piVar5 + 0x10))(param_3);
        }
      }
      FUN_004eb990(local_10);
    }
  }
  return;
}


