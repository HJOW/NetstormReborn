// ==== FUN_00463110 @ 00463110

void FUN_00463110(int param_1)

{
  int iVar1;
  short *psVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  byte *pbVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  
  FUN_00462ac0();
  if (param_1 != 0) {
    FUN_00462b20();
  }
  psVar2 = (short *)FUN_00463330();
  iVar8 = DAT_005c847c;
  iVar1 = ((int)psVar2 - DAT_00562c18) / 6;
  FUN_0040eb40();
  iVar9 = 1;
  do {
    do {
      iVar5 = DAT_005c847c;
      iVar4 = DAT_005c8464;
      uVar3 = iVar9 * 0x32 + DAT_005c8464;
      if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3)) {
        FUN_0040eb40();
        iVar9 = 1;
        do {
          do {
            uVar3 = iVar9 * 0x32 + iVar4;
            if ((uVar3 < iVar4 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + iVar4) <= uVar3)) {
              *psVar2 = 0;
              psVar2[1] = 0;
              FUN_00462ac0();
              return;
            }
            uVar7 = FUN_004ac1e0();
            if (((uVar7 & 0x800) != 0) && (iVar8 = FUN_004ad390(), iVar8 == iVar1)) {
              iVar8 = FUN_00463330();
              FUN_00462f10((int)(uVar3 - DAT_005c8464) / 0x32,(iVar8 - DAT_00562c18) / 6);
            }
            iVar9 = iVar9 + 1;
            iVar4 = DAT_005c8464;
          } while (iVar5 <= iVar9);
          pbVar6 = (byte *)(iVar9 * 0x32 + 0xb + DAT_005c8464);
          do {
            if ((*pbVar6 & 1) == 0) break;
            iVar9 = iVar9 + 1;
            pbVar6 = pbVar6 + 0x32;
          } while (iVar9 < iVar5);
        } while( true );
      }
      uVar3 = FUN_004ac1e0();
      if (((uVar3 & 0x800) != 0) && ((param_1 != 0 || (iVar4 = FUN_004ad390(), iVar4 == 0xfe)))) {
        iVar4 = FUN_004e49c0();
        iVar5 = FUN_004e49c0();
        if ((*(byte *)(iVar5 + iVar4 * 0x100 + DAT_005c7c44) & 8) == 0) {
          FUN_004ad3e0(iVar1);
          *psVar2 = *psVar2 + 1;
        }
      }
      iVar9 = iVar9 + 1;
    } while (iVar8 <= iVar9);
    pbVar6 = (byte *)(iVar9 * 0x32 + 0xb + DAT_005c8464);
    do {
      if ((*pbVar6 & 1) == 0) break;
      iVar9 = iVar9 + 1;
      pbVar6 = pbVar6 + 0x32;
    } while (iVar9 < iVar8);
  } while( true );
}


// ==== FUN_00462b20 @ 00462b20

void FUN_00462b20(void)

{
  int iVar1;
  undefined2 *puVar2;
  
  iVar1 = 0;
  do {
    puVar2 = (undefined2 *)(DAT_00562c18 + iVar1);
    iVar1 = iVar1 + 6;
    *puVar2 = 0;
    puVar2[1] = 0;
  } while (iVar1 < 0x5e2);
  *(undefined2 *)(DAT_00562c18 + 2) = 1;
  *(undefined1 *)(DAT_00562c18 + 4) = 1;
  *(undefined2 *)(DAT_00562c18 + 0x5f4) = 0x7dfd;
  *(undefined2 *)(DAT_00562c18 + 0x5f6) = 0x7dfd;
  return;
}


// ==== FUN_00462ac0 @ 00462ac0

void FUN_00462ac0(void)

{
  if (DAT_005e4794 != 0) {
    if (*(short *)(DAT_00562c18 + 0x5f4) != 0x7dfd) {
      FUN_004e0620("graphs[INVALID_GRAPH].numSurfaces == 0x7DFD","\\Ns\\O\\Graph.cpp",0x68);
    }
    if ((DAT_005e4794 != 0) && (*(short *)(DAT_00562c18 + 0x5f6) != 0x7dfd)) {
      FUN_004e0620("graphs[INVALID_GRAPH].inUse == 0x7DFD","\\Ns\\O\\Graph.cpp",0x69);
    }
  }
  return;
}


// ==== FUN_0040eb40 @ 0040eb40

void __fastcall FUN_0040eb40(int *param_1)

{
  int iVar1;
  
  iVar1 = *param_1 + 1;
  *param_1 = iVar1;
  if (iVar1 < param_1[1]) {
    do {
      if ((*(byte *)(iVar1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
        return;
      }
      iVar1 = iVar1 + 1;
      *param_1 = iVar1;
    } while (iVar1 < param_1[1]);
  }
  return;
}


// ==== FUN_00463330 @ 00463330

int FUN_00463330(void)

{
  undefined2 *puVar1;
  int iVar2;
  short *psVar3;
  
  iVar2 = 0;
  psVar3 = (short *)(DAT_00562c18 + 2);
  do {
    if (*psVar3 == 0) goto LAB_00463391;
    iVar2 = iVar2 + 1;
    psVar3 = psVar3 + 3;
  } while (iVar2 < 0xfb);
  FUN_00463110(1);
  iVar2 = 0;
  psVar3 = (short *)(DAT_00562c18 + 2);
  while (*psVar3 != 0) {
    iVar2 = iVar2 + 1;
    psVar3 = psVar3 + 3;
    if (0xfa < iVar2) {
      FUN_004e0620("false","\\Ns\\O\\Graph.cpp",0x25d);
      return 0;
    }
  }
LAB_00463391:
  iVar2 = iVar2 * 6;
  puVar1 = (undefined2 *)(iVar2 + DAT_00562c18);
  *puVar1 = 0;
  puVar1[1] = 0;
  *(undefined2 *)(iVar2 + 2 + DAT_00562c18) = 1;
  return iVar2 + DAT_00562c18;
}


