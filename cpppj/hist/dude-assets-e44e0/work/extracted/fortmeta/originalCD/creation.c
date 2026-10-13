// ==== FUN_00428fc0 @ 00428fc0

void __thiscall FUN_00428fc0(void *this)

{
  byte bVar1;
  undefined1 uVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_0042907b;
  *unaff_FS_OFFSET = &local_10;
  DAT_005178d4 = DAT_005178d4 + 1;
  local_8 = 0xffffffff;
  FUN_00429085();
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_00517f94,s__ns_o_template_cpp_00517f80,0x413);
  }
  piVar4 = (int *)FUN_0044c4e0(*(undefined4 *)this,s_Badges_00517f98,1);
  if ((*piVar4 != 0) && (piVar4[2] != 0)) {
    bVar1 = *(byte *)(piVar4[1] + *piVar4);
    piVar4[1] = piVar4[1] + 1;
    for (uVar5 = (uint)bVar1; uVar5 != 0; uVar5 = uVar5 - 1) {
      iVar3 = piVar4[1];
      uVar2 = *(undefined1 *)(iVar3 + *piVar4);
      piVar4[1] = iVar3 + 1;
      piVar4[1] = iVar3 + 2;
      FUN_00475660(uVar2,*(undefined1 *)(iVar3 + 1 + *piVar4));
    }
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00429085 @ 00429085

void __cdecl FUN_00429085(void)

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


// ==== FUN_00475660 @ 00475660

void __cdecl FUN_00475660(undefined4 param_1,int param_2)

{
  int iVar1;
  
  if ((param_2 < 0) || (0xff < param_2)) {
    FUN_00490040(s_count_>_0____count_<_256_0052d2ec,s_metaDisplay_cpp_0052d2dc,0x10df);
  }
  iVar1 = FUN_00475560(param_1);
  if (iVar1 != -1) {
    (&DAT_0052aef8)[iVar1 * 0x37] = param_2;
  }
  return;
}


// ==== FUN_00475560 @ 00475560

int __cdecl FUN_00475560(int param_1)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = 0;
  piVar2 = &DAT_0052ae20;
  do {
    if (*piVar2 == param_1) {
      return iVar1;
    }
    piVar2 = piVar2 + 0x37;
    iVar1 = iVar1 + 1;
  } while (piVar2 < s__w_E_s_Players_in_Zone__d_0052c630);
  return -1;
}


// ==== FUN_004293d0 @ 004293d0

void __thiscall FUN_004293d0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_00429498;
  *unaff_FS_OFFSET = &local_10;
  DAT_005178d4 = DAT_005178d4 + 1;
  local_8 = 0;
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_005180b4,s__ns_o_template_cpp_005180a0,0x460);
  }
  iVar1 = FUN_00426fe0(this);
  if (iVar1 == 0) {
    FUN_00490040(s_isMine___005180cc,s__ns_o_template_cpp_005180b8,0x461);
  }
  uVar2 = FUN_00406d90();
  piVar3 = (int *)FUN_0044c4e0(*(undefined4 *)this,s_Technology_005180d8,1);
  if ((*piVar3 != 0) && (piVar3[2] != 0)) {
    FUN_00427e00(this);
    FUN_00426dd0(uVar2,piVar3,0x11,1);
    FUN_00426830();
  }
  FUN_004b2e70();
  local_8 = 0xffffffff;
  FUN_004294a2();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00426fe0 @ 00426fe0

bool __thiscall FUN_00426fe0(void *this)

{
  return &DAT_00564bd0 + DAT_0050f6d4 * 5 == this;
}


// ==== FUN_00426fa0 @ 00426fa0

int __thiscall FUN_00426fa0(void *this)

{
  int iVar1;
  
  iVar1 = ((int)this + -0x564bd0) / 0x14;
  if ((iVar1 < 1) || (8 < iVar1)) {
    FUN_00490040(s_n>_FIRST_REAL_PLAYER____n<MAX_PL_005179d8,s__ns_o_template_cpp_005179c4,0x159);
  }
  return iVar1;
}


// ==== FUN_004294a2 @ 004294a2

void __cdecl FUN_004294a2(void)

{
  FUN_00429f30();
  return;
}


// ==== FUN_00427e00 @ 00427e00

void __thiscall FUN_00427e00(void *this)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  undefined4 *unaff_FS_OFFSET;
  bool bVar9;
  int local_18;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_00427f30;
  *unaff_FS_OFFSET = &local_10;
  DAT_005178d4 = DAT_005178d4 + 1;
  local_8 = 0;
  piVar2 = (int *)FUN_0044c4e0(*(undefined4 *)this,s_TypeNames_00517d6c,1);
  if ((*piVar2 == 0) || (piVar2[2] == 0)) {
    DAT_005178d8 = (undefined4 *)0x0;
  }
  else {
    bVar9 = DAT_005178d8 != (undefined4 *)0x0;
    uVar6 = (uint)*(byte *)(piVar2[1] + *piVar2);
    piVar2[1] = piVar2[1] + 1;
    if (bVar9) {
      FUN_00490040(s__typeConversion_00517d8c,s__ns_o_template_cpp_00517d78,600);
    }
    DAT_005178d8 = (undefined4 *)FUN_004f1650(0xff);
    puVar8 = DAT_005178d8;
    for (iVar4 = 0x3f; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)puVar8 = 0;
    *(undefined1 *)((int)puVar8 + 2) = 0;
    iVar4 = piVar2[1];
    iVar1 = *piVar2;
    if (piVar2[2] < iVar4) {
      piVar2[2] = iVar4;
    }
    iVar7 = 0;
    local_18 = 0;
    if (0 < DAT_0051cbf0) {
      do {
        iVar3 = FUN_004267d0(DAT_0051c960 + iVar7 + 4);
        iVar5 = 0;
        piVar2 = (int *)(iVar1 + iVar4);
        if (uVar6 != 0) {
          do {
            if (*piVar2 == iVar3) break;
            iVar5 = iVar5 + 1;
            piVar2 = piVar2 + 1;
          } while (iVar5 < (int)uVar6);
        }
        if (iVar5 < (int)uVar6) {
          *(undefined1 *)(iVar5 + (int)DAT_005178d8) = (undefined1)local_18;
        }
        iVar7 = iVar7 + 0x1d4;
        local_18 = local_18 + 1;
      } while (local_18 < DAT_0051cbf0);
    }
  }
  local_8 = 0xffffffff;
  FUN_00427f3a();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00426830 @ 00426830

void __cdecl FUN_00426830(void)

{
  if (DAT_005178d8 != 0) {
    FUN_004f1a50(DAT_005178d8);
  }
  DAT_005178d8 = 0;
  return;
}


