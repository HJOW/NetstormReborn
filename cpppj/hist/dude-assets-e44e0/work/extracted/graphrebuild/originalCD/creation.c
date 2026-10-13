// ==== FUN_0045bd60 @ 0045bd60

void FUN_0045bd60(int param_1)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  undefined4 uVar9;
  int iVar10;
  
  FUN_0045b560();
  if (param_1 != 0) {
    FUN_0045b570();
  }
  psVar2 = (short *)FUN_0045c390();
  iVar10 = 2;
  iVar3 = FUN_0045b4e0();
  iVar1 = DAT_005395f4;
  if (2 < DAT_005395f4) {
    do {
      if ((*(byte *)(iVar10 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) break;
      iVar10 = iVar10 + 1;
    } while (iVar10 < DAT_005395f4);
  }
  iVar8 = DAT_005395dc;
  if (DAT_005395dc + 0x24U <= (uint)(iVar10 * 0x24 + DAT_005395dc)) {
    do {
      if ((uint)(DAT_005395f4 * 0x24 + iVar8) <= (uint)(iVar10 * 0x24 + iVar8)) break;
      uVar4 = FUN_004abac0();
      iVar8 = DAT_005395dc;
      if (((uVar4 & 0x800) != 0) &&
         ((param_1 != 0 || (iVar5 = FUN_004adbc0(), iVar8 = DAT_005395dc, iVar5 == 0xfe)))) {
        iVar8 = DAT_005395dc;
        iVar5 = __ftol();
        iVar6 = __ftol();
        if ((*(byte *)(iVar5 * 0x100 + iVar6 + DAT_0052fe48) & 8) == 0) {
          FUN_004adbd0(iVar3);
          iVar8 = DAT_005395dc;
          *psVar2 = *psVar2 + 1;
        }
      }
      do {
        iVar10 = iVar10 + 1;
        if (iVar1 <= iVar10) break;
      } while ((*(byte *)(iVar10 * 0x24 + 0xb + iVar8) & 1) != 0);
    } while (iVar8 + 0x24U <= (uint)(iVar10 * 0x24 + iVar8));
  }
  iVar1 = DAT_005395f4;
  iVar10 = 2;
  if (2 < DAT_005395f4) {
    do {
      if ((*(byte *)(iVar10 * 0x24 + 0xb + iVar8) & 1) == 0) break;
      iVar10 = iVar10 + 1;
    } while (iVar10 < DAT_005395f4);
  }
  if (iVar8 + 0x24U <= (uint)(iVar10 * 0x24 + iVar8)) {
    do {
      uVar4 = iVar10 * 0x24 + iVar8;
      if ((uint)(DAT_005395f4 * 0x24 + iVar8) <= uVar4) break;
      uVar7 = FUN_004abac0();
      if (((uVar7 & 0x800) != 0) && (iVar8 = FUN_004adbc0(), iVar8 == iVar3)) {
        FUN_0045c390();
        uVar9 = FUN_0045b4e0();
        FUN_0045bb90((int)(uVar4 - DAT_005395dc) / 0x24,uVar9);
      }
      do {
        iVar10 = iVar10 + 1;
        if (iVar1 <= iVar10) break;
      } while ((*(byte *)(iVar10 * 0x24 + 0xb + DAT_005395dc) & 1) != 0);
      iVar8 = DAT_005395dc;
    } while (DAT_005395dc + 0x24U <= (uint)(iVar10 * 0x24 + DAT_005395dc));
  }
  FUN_0045b4d0();
  FUN_0045b520();
  FUN_0045b560();
  return;
}


// ==== FUN_0045b570 @ 0045b570

void FUN_0045b570(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  do {
    iVar2 = iVar2 + 6;
    FUN_0045b4d0();
    iVar1 = DAT_005207f0;
  } while (iVar2 < 0x5e2);
  *(undefined2 *)(DAT_005207f0 + 2) = 1;
  *(undefined1 *)(iVar1 + 4) = 1;
  *(undefined2 *)(iVar1 + 0x5f4) = 0x7dfd;
  *(undefined2 *)(iVar1 + 0x5f6) = 0x7dfd;
  return;
}


// ==== FUN_0045b560 @ 0045b560

void FUN_0045b560(void)

{
  return;
}


// ==== FUN_0045b4d0 @ 0045b4d0

void __fastcall FUN_0045b4d0(undefined2 *param_1)

{
  param_1[1] = 0;
  *param_1 = 0;
  return;
}


// ==== FUN_0045b4e0 @ 0045b4e0

int __fastcall FUN_0045b4e0(int param_1)

{
  return (param_1 - DAT_005207f0) / 6;
}


// ==== FUN_0045b520 @ 0045b520

void FUN_0045b520(void)

{
  return;
}


// ==== FUN_0045c390 @ 0045c390

int FUN_0045c390(void)

{
  int iVar1;
  short *psVar2;
  
  iVar1 = 0;
  psVar2 = (short *)(DAT_005207f0 + 2);
  do {
    if (*psVar2 == 0) {
      FUN_0045b4d0();
      iVar1 = DAT_005207f0 + iVar1 * 6;
      *(undefined2 *)(iVar1 + 2) = 1;
      return iVar1;
    }
    psVar2 = psVar2 + 3;
    iVar1 = iVar1 + 1;
  } while (iVar1 < 0xfb);
  FUN_0045bd60(1);
  iVar1 = 0;
  psVar2 = (short *)(DAT_005207f0 + 2);
  do {
    if (*psVar2 == 0) {
      FUN_0045b4d0();
      iVar1 = DAT_005207f0 + iVar1 * 6;
      *(undefined2 *)(iVar1 + 2) = 1;
      return iVar1;
    }
    psVar2 = psVar2 + 3;
    iVar1 = iVar1 + 1;
  } while (iVar1 < 0xfb);
  FUN_00490040(s_false_0052097c,s__ns_o_graph_cpp_0052096c,0x25d);
  return 0;
}


