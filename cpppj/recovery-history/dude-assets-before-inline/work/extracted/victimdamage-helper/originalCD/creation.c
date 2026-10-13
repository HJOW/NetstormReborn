// ==== FUN_0040c800 @ 0040c800

undefined4 __thiscall FUN_0040c800(void)

{
  int iVar1;
  
  iVar1 = FUN_004ae4b0();
  if ((9 < iVar1) && (iVar1 < 0xf)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004ae4b0 @ 004ae4b0

int __thiscall FUN_004ae4b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)((int)this + 0x22) * 4) + -0x41;
}


// ==== FUN_004eb960 @ 004eb960

void __thiscall FUN_004eb960(void *this,int param_2)

{
  *(uint *)((int)this + 4) = (uint)*(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  FUN_004eb990();
  return;
}


// ==== FUN_004eb990 @ 004eb990

void __thiscall FUN_004eb990(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar2 = DAT_005395dc;
  do {
    do {
      iVar1 = *(int *)((int)this + 4);
      *(int *)((int)this + 8) = iVar1;
      if (iVar1 == 0) {
        *(undefined4 *)((int)this + 4) = 0;
        *(undefined4 *)((int)this + 8) = 0;
        return;
      }
      iVar1 = iVar1 * 0x24 + iVar2;
      *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 + 4);
    } while ((iVar1 != 0) && (0x45 < *(byte *)(iVar1 + 10)));
    iVar1 = (*(code *)**(undefined4 **)this)(iVar1);
    iVar2 = DAT_005395dc;
  } while (iVar1 == 0);
  return;
}


// ==== FUN_004eba30 @ 004eba30

int __thiscall FUN_004eba30(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004eba70();
  if ((iVar1 == 0) && (*(int *)((int)this + 8) != 0)) {
    FUN_00490040(s_isDepend______sid____0_005418e8,s__ns_o_SquidFinder_cpp_005418d0,0x16c);
  }
  return DAT_005395dc + *(int *)((int)this + 8) * 0x24;
}


// ==== FUN_004eba70 @ 004eba70

bool __thiscall FUN_004eba70(void *this)

{
  return *(byte *)(*(int *)((int)this + 8) * 0x24 + 10 + DAT_005395dc) == DAT_0051c964;
}


// ==== FUN_0040f190 @ 0040f190

void FUN_0040f190(void)

{
  FUN_004eba70();
  return;
}


// ==== FUN_004af700 @ 004af700

undefined4 __thiscall FUN_004af700(void *this)

{
  undefined4 uVar1;
  
  if (*(ushort *)((int)this + 0x12) == 0) {
    return 0;
  }
                    /* WARNING: Could not recover jumptable at 0x004af71d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  uVar1 = (*(code *)**(undefined4 **)((uint)*(ushort *)((int)this + 0x12) * 0x24 + DAT_005395dc))();
  return uVar1;
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


