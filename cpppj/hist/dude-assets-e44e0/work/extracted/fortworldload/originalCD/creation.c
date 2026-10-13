// ==== FUN_004330c0 @ 004330c0

void __cdecl FUN_004330c0(int param_1)

{
  int iVar1;
  undefined1 *puVar2;
  undefined1 local_90 [24];
  undefined1 local_78 [120];
  
  iVar1 = 0x13;
  FUN_00490430(local_90);
  FUN_004904d0(local_90,&DAT_005484c0);
  puVar2 = local_78;
  do {
    FUN_0040f1e0(puVar2);
    iVar1 = iVar1 + -1;
    puVar2 = puVar2 + 6;
  } while (-1 < iVar1);
  FUN_00427c00(&DAT_00564bd0 + DAT_0050f6d4 * 5,local_78);
  FUN_00490980(local_90,local_78,0,0x14);
  FUN_00490530(local_90,DAT_00505eb0);
  if (param_1 == 0) {
    FUN_00428d50(&DAT_00564bd0 + DAT_0050f6d4 * 5,0,0x14);
    FUN_004297f0(&DAT_00564bd0 + DAT_0050f6d4 * 5);
    FUN_00428fc0(&DAT_00564bd0 + DAT_0050f6d4 * 5);
    FUN_004293d0(&DAT_00564bd0 + DAT_0050f6d4 * 5);
  }
  else {
    iVar1 = FUN_004bd4c0(&DAT_00583188,0);
    if (iVar1 != 0) {
      FUN_004906c0(local_90,0);
    }
  }
  FUN_0045bd60(0);
  return;
}


// ==== FUN_00427c00 @ 00427c00

void __thiscall FUN_00427c00(void *this,undefined4 *param_2)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  pcStack_c = FUN_00427cc8;
  *unaff_FS_OFFSET = &local_10;
  DAT_005178d4 = DAT_005178d4 + 1;
  local_8 = 0xffffffff;
  FUN_00427cd2();
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_00517cf8,s__ns_o_template_cpp_00517ce4,0x226);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_00490040(s_terrList_00517d10,s__ns_o_template_cpp_00517cfc,0x227);
  }
  FUN_0044c4e0(*(undefined4 *)this,s_Territory_00517d1c,0);
  piVar1 = *(int **)(*(int *)this + 8);
  iVar3 = piVar1[1];
  puVar4 = (undefined4 *)(*piVar1 + iVar3);
  for (iVar2 = 0x1e; iVar2 != 0; iVar2 = iVar2 + -1) {
    *param_2 = *puVar4;
    puVar4 = puVar4 + 1;
    param_2 = param_2 + 1;
  }
  iVar3 = iVar3 + 0x78;
  piVar1[1] = iVar3;
  if (piVar1[2] < iVar3) {
    FUN_00490040(s_pos<_end_00512100,s__ns_o_memstream_h_00518314,0x2e);
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_00427cd2 @ 00427cd2

void __cdecl FUN_00427cd2(void)

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


// ==== FUN_0040f1e0 @ 0040f1e0

void * __thiscall FUN_0040f1e0(void *this)

{
  byte bVar1;
  
  *(byte *)this = *(byte *)this & 0xc0;
  *(undefined1 *)this = 0;
  bVar1 = *(byte *)((int)this + 1);
  *(byte *)((int)this + 1) = bVar1 & 0xfe;
  *(byte *)((int)this + 1) = bVar1 & 0xfc;
  *(byte *)((int)this + 1) = bVar1 & 0xf8;
  *(byte *)((int)this + 1) = bVar1 & 0xf0;
  *(undefined1 *)((int)this + 1) = 0;
  *(byte *)((int)this + 2) = *(byte *)((int)this + 2) & 0xf0;
  *(undefined1 *)((int)this + 2) = 0;
  *(undefined1 *)((int)this + 3) = 0;
  *(undefined1 *)((int)this + 4) = 0;
  *(undefined1 *)((int)this + 5) = 0;
  return this;
}


// ==== FUN_0045bd60 @ 0045bd60

void __cdecl FUN_0045bd60(int param_1)

{
  uint uVar1;
  int iVar2;
  short *psVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined4 uVar9;
  int iVar10;
  
  FUN_0045b560();
  if (param_1 != 0) {
    FUN_0045b570();
  }
  psVar3 = (short *)FUN_0045c390();
  iVar10 = 2;
  iVar4 = FUN_0045b4e0(psVar3);
  iVar2 = DAT_005395f4;
  if (2 < DAT_005395f4) {
    do {
      if ((*(byte *)(iVar10 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) break;
      iVar10 = iVar10 + 1;
    } while (iVar10 < DAT_005395f4);
  }
  iVar8 = DAT_005395dc;
  if (DAT_005395dc + 0x24U <= (uint)(iVar10 * 0x24 + DAT_005395dc)) {
    do {
      uVar1 = iVar10 * 0x24 + iVar8;
      if ((uint)(DAT_005395f4 * 0x24 + iVar8) <= uVar1) break;
      uVar5 = FUN_004abac0(uVar1);
      iVar8 = DAT_005395dc;
      if (((uVar5 & 0x800) != 0) &&
         ((param_1 != 0 || (iVar6 = FUN_004adbc0(uVar1), iVar8 = DAT_005395dc, iVar6 == 0xfe)))) {
        iVar8 = DAT_005395dc;
        iVar6 = __ftol();
        iVar7 = __ftol();
        if ((*(byte *)(iVar6 * 0x100 + iVar7 + DAT_0052fe48) & 8) == 0) {
          FUN_004adbd0(uVar1,iVar4);
          iVar8 = DAT_005395dc;
          *psVar3 = *psVar3 + 1;
        }
      }
      do {
        iVar10 = iVar10 + 1;
        if (iVar2 <= iVar10) break;
      } while ((*(byte *)(iVar10 * 0x24 + 0xb + iVar8) & 1) != 0);
    } while (iVar8 + 0x24U <= (uint)(iVar10 * 0x24 + iVar8));
  }
  iVar2 = DAT_005395f4;
  iVar10 = 2;
  if (2 < DAT_005395f4) {
    do {
      if ((*(byte *)(iVar10 * 0x24 + 0xb + iVar8) & 1) == 0) break;
      iVar10 = iVar10 + 1;
    } while (iVar10 < DAT_005395f4);
  }
  if (iVar8 + 0x24U <= (uint)(iVar10 * 0x24 + iVar8)) {
    do {
      uVar1 = iVar10 * 0x24 + iVar8;
      if ((uint)(DAT_005395f4 * 0x24 + iVar8) <= uVar1) break;
      uVar5 = FUN_004abac0(uVar1);
      if (((uVar5 & 0x800) != 0) && (iVar8 = FUN_004adbc0(uVar1), iVar8 == iVar4)) {
        uVar9 = FUN_0045c390();
        uVar9 = FUN_0045b4e0(uVar9);
        FUN_0045bb90((int)(uVar1 - DAT_005395dc) / 0x24,uVar9);
      }
      do {
        iVar10 = iVar10 + 1;
        if (iVar2 <= iVar10) break;
      } while ((*(byte *)(iVar10 * 0x24 + 0xb + DAT_005395dc) & 1) != 0);
      iVar8 = DAT_005395dc;
    } while (DAT_005395dc + 0x24U <= (uint)(iVar10 * 0x24 + DAT_005395dc));
  }
  FUN_0045b4d0(psVar3);
  FUN_0045b520();
  FUN_0045b560();
  return;
}


// ==== FUN_004906c0 @ 004906c0

void __thiscall FUN_004906c0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_00490520();
  if (iVar1 == 0) {
    FUN_00490040(s_inWorld___00535b68,s__ns_o_islandbuilder_cpp_00535b50,0x53);
  }
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_00490040(s_cRect_isValid___00535b8c,s__ns_o_islandbuilder_cpp_00535b74,0x54);
  }
  iVar1 = FUN_0042c840(DAT_0051caa0,1);
  FUN_00490c50(this,0);
  if (iVar1 != 0) {
    (**(code **)(*(int *)(iVar1 * 0x24 + DAT_005395dc) + 0x10))(0);
  }
  return;
}


