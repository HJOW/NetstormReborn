// ==== FUN_004b5f70 @ 004b5f70

void __cdecl FUN_004b5f70(int param_1)

{
  undefined1 *puVar1;
  int iVar2;
  undefined1 local_90 [24];
  undefined1 local_78 [2];
  undefined1 local_76 [118];
  
  FUN_0046d9b0(local_90);
  FUN_0046da20(local_90,&DAT_0054c8a0);
  puVar1 = local_76;
  iVar2 = 0x14;
  do {
    puVar1[-2] = 0;
    puVar1[-1] = 0;
    *puVar1 = 0;
    puVar1[1] = 0;
    puVar1[2] = 0;
    puVar1[3] = 0;
    puVar1 = puVar1 + 6;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  FUN_004bd660(&DAT_005c89f0 + DAT_00540cac * 5,local_78);
  FUN_0046dd70(local_90,local_78,0,0x14);
  FUN_0046da70(local_90,DAT_0050c038);
  if (param_1 == 0) {
    FUN_004be610(&DAT_005c89f0 + DAT_00540cac * 5,0,0x14);
    FUN_004bf340(&DAT_005c89f0 + DAT_00540cac * 5);
    FUN_004be8e0(&DAT_005c89f0 + DAT_00540cac * 5);
    FUN_004beee0(&DAT_005c89f0 + DAT_00540cac * 5);
  }
  else {
    iVar2 = FUN_0046ee30(&DAT_00568b60,0);
    if (iVar2 != 0) {
      FUN_0046e630(local_90,0);
      FUN_00463110(0);
      return;
    }
  }
  FUN_00463110(0);
  return;
}


// ==== FUN_004bd660 @ 004bd660

void __thiscall FUN_004bd660(void *this,undefined4 *param_2)

{
  int *piVar1;
  int iVar2;
  undefined4 *puVar3;
  
  if (*(int *)this == 0) {
    FUN_004e0620(&DAT_0051476c,"\\Ns\\O\\Template.cpp",0x22e);
  }
  if (param_2 == (undefined4 *)0x0) {
    FUN_004e0620("terrList","\\Ns\\O\\Template.cpp",0x22f);
  }
  FUN_0044ceb0(*(undefined4 *)this,"Territory",0);
  piVar1 = *(int **)(*(int *)this + 8);
  puVar3 = (undefined4 *)(piVar1[1] + *piVar1);
  for (iVar2 = 0x1e; iVar2 != 0; iVar2 = iVar2 + -1) {
    *param_2 = *puVar3;
    puVar3 = puVar3 + 1;
    param_2 = param_2 + 1;
  }
  iVar2 = piVar1[1];
  piVar1[1] = iVar2 + 0x78;
  if (piVar1[2] < iVar2 + 0x78) {
    FUN_004e0620("pos<=end","c:\\ns\\o\\memstream.h",0x2e);
  }
  return;
}


// ==== FUN_00463110 @ 00463110

void __cdecl FUN_00463110(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  short *psVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  byte *pbVar8;
  int iVar9;
  int local_8;
  int local_4;
  
  FUN_00462ac0();
  if (param_1 != 0) {
    FUN_00462b20();
  }
  psVar4 = (short *)FUN_00463330();
  iVar2 = ((int)psVar4 - DAT_00562c18) / 6;
  local_8 = 1;
  local_4 = DAT_005c847c;
  FUN_0040eb40(&local_8);
  iVar3 = local_4;
  iVar9 = local_8;
  do {
    do {
      iVar6 = DAT_005c8464;
      uVar1 = iVar9 * 0x32 + DAT_005c8464;
      if ((uVar1 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar1)) {
        local_8 = 1;
        local_4 = DAT_005c847c;
        FUN_0040eb40(&local_8);
        iVar3 = local_4;
        iVar9 = local_8;
        do {
          do {
            uVar1 = iVar9 * 0x32 + iVar6;
            if ((uVar1 < iVar6 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + iVar6) <= uVar1)) {
              *psVar4 = 0;
              psVar4[1] = 0;
              FUN_00462ac0();
              return;
            }
            uVar5 = FUN_004ac1e0(uVar1);
            if (((uVar5 & 0x800) != 0) && (iVar6 = FUN_004ad390(uVar1), iVar6 == iVar2)) {
              iVar6 = FUN_00463330();
              FUN_00462f10((int)(uVar1 - DAT_005c8464) / 0x32,(iVar6 - DAT_00562c18) / 6);
            }
            iVar9 = iVar9 + 1;
            iVar6 = DAT_005c8464;
          } while (iVar3 <= iVar9);
          pbVar8 = (byte *)(iVar9 * 0x32 + 0xb + DAT_005c8464);
          do {
            if ((*pbVar8 & 1) == 0) break;
            iVar9 = iVar9 + 1;
            pbVar8 = pbVar8 + 0x32;
          } while (iVar9 < iVar3);
        } while( true );
      }
      uVar5 = FUN_004ac1e0(uVar1);
      if (((uVar5 & 0x800) != 0) && ((param_1 != 0 || (iVar6 = FUN_004ad390(uVar1), iVar6 == 0xfe)))
         ) {
        local_4 = *(int *)(uVar1 + 0x12);
        local_8 = *(int *)(uVar1 + 0xe);
        iVar6 = FUN_004e49c0();
        iVar7 = FUN_004e49c0();
        if ((*(byte *)(iVar7 + iVar6 * 0x100 + DAT_005c7c44) & 8) == 0) {
          FUN_004ad3e0(uVar1,iVar2);
          *psVar4 = *psVar4 + 1;
        }
      }
      iVar9 = iVar9 + 1;
    } while (iVar3 <= iVar9);
    pbVar8 = (byte *)(iVar9 * 0x32 + 0xb + DAT_005c8464);
    do {
      if ((*pbVar8 & 1) == 0) break;
      iVar9 = iVar9 + 1;
      pbVar8 = pbVar8 + 0x32;
    } while (iVar9 < iVar3);
  } while( true );
}


// ==== FUN_0046e630 @ 0046e630

void __thiscall FUN_0046e630(void *this)

{
  int iVar1;
  
  if (*(undefined **)this != &DAT_0054c8a0) {
    FUN_004e0620("inWorld()","\\Ns\\O\\Islandbuilder.cpp",0x57);
  }
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_004e0620("cRect.isValid()","\\Ns\\O\\Islandbuilder.cpp",0x58);
  }
  iVar1 = FUN_0040dfd0(DAT_005411b4,1);
  FUN_0046e000(this,0);
  if (iVar1 != 0) {
                    /* WARNING: Could not recover jumptable at 0x0046e6b4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (**(code **)(*(int *)(DAT_005c8464 + iVar1 * 0x32) + 0x10))();
    return;
  }
  return;
}


