// ==== FUN_004af530 @ 004af530

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * FUN_004af530(int param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  undefined1 uVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  if (param_1 == DAT_005412e0) {
    FUN_004e0620("type != fakeThreeByThreeSurfaceType","\\Ns\\O\\Squid.cpp",0x189);
  }
  iVar1 = FUN_004af1d0(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar1 < 0x59d9) goto LAB_004af5a1;
    uVar6 = 0x194;
    pcVar5 = "sid < FIRST_PREDICTABLE_SID";
  }
  else {
    if (23000 < iVar1) goto LAB_004af5a1;
    uVar6 = 0x191;
    pcVar5 = "sid >= FIRST_PREDICTABLE_SID";
  }
  FUN_004e0620(pcVar5,"\\Ns\\O\\Squid.cpp",uVar6);
LAB_004af5a1:
  iVar2 = FUN_0049a840(param_1);
  uVar4 = (undefined1)param_1;
  if (*(code **)(iVar2 + 0x1d0) != (code *)0x0) {
    piVar3 = (int *)(**(code **)(iVar2 + 0x1d0))(iVar1);
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  piVar3 = (int *)(iVar1 * 0x32 + DAT_005c8464);
  if (piVar3 != (int *)0x0) {
    *piVar3 = (int)&PTR_LAB_00501dd0;
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  uRam0000000a = uVar4;
  (**(code **)(_DAT_00000000 + 0x28))();
  return (int *)0x0;
}


// ==== FUN_004af610 @ 004af610

int * FUN_004af610(uint param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  undefined1 local_100 [256];
  
  if ((param_2 < 15000) || (DAT_005c847c <= param_2)) {
    FUN_004e0620("(FIRST_SERVER_SID <= sid && sid < numSquids)","\\Ns\\O\\Squid.cpp",0x1a7);
  }
  piVar3 = (int *)(param_2 * 0x32 + DAT_005c8464);
  if ((*(byte *)((int)piVar3 + 0xb) & 1) == 0) {
    if (*(byte *)((int)piVar3 + 10) != param_1) {
      iVar1 = FUN_0049a840((uint)*(byte *)((int)piVar3 + 10));
      iVar1 = FUN_0049a840(param_1,iVar1 + 4);
      uVar2 = FUN_004a89c0(local_100,"Invalid take. Server type %s, client type %s",iVar1 + 4);
      FUN_00436d20(uVar2);
      FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Squid.cpp",0x1ba);
    }
    (**(code **)(*piVar3 + 0x48))(0);
  }
  *(byte *)((int)piVar3 + 0xb) = *(byte *)((int)piVar3 + 0xb) & 0xfc;
  iVar1 = FUN_0049a840(param_1);
  if (*(code **)(iVar1 + 0x1d0) == (code *)0x0) {
    piVar3 = (int *)(param_2 * 0x32 + DAT_005c8464);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_00501dd0;
    }
  }
  else {
    piVar3 = (int *)(**(code **)(iVar1 + 0x1d0))(param_2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  *(byte *)((int)piVar3 + 0xb) = *(byte *)((int)piVar3 + 0xb) | 4;
  (**(code **)(*piVar3 + 0x2c))();
  return piVar3;
}


// ==== FUN_004b0c60 @ 004b0c60

void __fastcall FUN_004b0c60(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x7d4);
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_004e0620("!isContained()","\\Ns\\O\\Squid.cpp",0x7d5);
  }
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  *(undefined1 *)(param_1 + 0x23) = *(undefined1 *)(iVar1 + 0x104);
  if (DAT_00540bc0 != 0) {
    *(undefined1 *)(param_1 + 0x22) = 0;
    iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
    if ((*(uint *)(iVar1 + 0xec) & 0x400000) == 0) {
      iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
      uVar2 = *(uint *)(iVar1 + 0xe8) & 0x10;
    }
    else {
      uVar2 = *(byte *)(param_1 + 0x28) & 1;
    }
    if (uVar2 != 0) {
      uVar3 = FUN_0049fce0();
      *(undefined4 *)(param_1 + 0x1a) = uVar3;
    }
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_004e0620("!isContained()","\\Ns\\O\\Squid.cpp",0x7df);
  }
  return;
}


// ==== FUN_004ad4a0 @ 004ad4a0

void __fastcall FUN_004ad4a0(int param_1)

{
  int iVar1;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x7f6);
  }
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  *(undefined1 *)(param_1 + 0x23) = *(undefined1 *)(iVar1 + 0x104);
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_004e0620("!isContained()","\\Ns\\O\\Squid.cpp",0x7fa);
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049fce0 @ 0049fce0

int __fastcall FUN_0049fce0(int *param_1)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)(param_1 + 0x3a) & 0x10) == 0)) &&
     (((int)param_1 - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)param_1 - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
  }
  return *param_1;
}


// ==== FUN_004afe50 @ 004afe50

void __thiscall FUN_004afe50(int *param_1,uint param_2)

{
  byte bVar1;
  ushort uVar2;
  uint uVar3;
  bool bVar4;
  int iVar5;
  byte bVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  ushort *puVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int local_28;
  
  if (*(byte *)((int)param_1 + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x5d8);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_004e0620("!isContained()","\\Ns\\O\\Squid.cpp",0x5d9);
  }
  if ((param_1 < (int *)(DAT_005c8464 + 0x32U)) ||
     ((int *)(DAT_005c847c * 0x32 + DAT_005c8464) <= param_1)) {
    FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0x5e0);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 5) == 0) {
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
    iVar14 = (int)param_1 - DAT_005c8464;
    iVar7 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
    uVar3 = *(uint *)(iVar7 + 0xec);
    if (((uVar3 & 6) != 0) && ((*(byte *)(param_1 + 10) & 8) == 0)) {
      FUN_00481510(*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),uVar3);
    }
    if ((((uVar3 & (DAT_005424b8 | 0x400ff)) != 0) ||
        (iVar8 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10)),
        (*(uint *)(iVar8 + 0xe8) & 0x800) != 0)) && ((*(byte *)(param_1 + 10) & 8) == 0)) {
      if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) goto LAB_004b01c2;
      FUN_004e49c0();
      iVar8 = FUN_004e49c0();
      FUN_004e49c0();
      iVar9 = FUN_004e49c0();
      iVar15 = (iVar8 - *(int *)(iVar7 + 0x1d4)) + 1;
      iVar7 = (iVar9 - *(int *)(iVar7 + 0x1d8)) + 1;
      if (iVar7 <= iVar9) {
        iVar16 = iVar7 * 0x100;
        iVar5 = iVar15;
        local_28 = iVar7;
        do {
          for (; iVar5 <= iVar8; iVar5 = iVar5 + 1) {
            bVar6 = FUN_004afd30(iVar5,local_28);
            if (bVar6 != 0) {
              iVar10 = FUN_004e49c0();
              iVar11 = FUN_004e49c0();
              bVar1 = *(byte *)(iVar11 + iVar10 * 0x100 + DAT_005c7c44);
              if ((DAT_005e4794 != 0) && ((bVar1 & bVar6) != bVar6)) {
                FUN_004e0620("(flags & effectiveGenus) == ( unsigned char)effectiveGenus",
                             "\\Ns\\O\\Squid.cpp",0x617);
              }
              *(byte *)(iVar16 + DAT_005c7c44 + iVar5) = ~bVar6 & bVar1;
            }
            if (((iVar5 == iVar15) || (iVar5 == iVar8)) &&
               ((local_28 != iVar7 && (local_28 != iVar9)))) {
              bVar4 = true;
            }
            else {
              bVar4 = false;
            }
            if (((DAT_005424b8 & uVar3) != 0) && (bVar4)) {
              if ((iVar5 < 0) || (((iVar16 < 0 || (0xff < iVar5)) || (0xffff < iVar16)))) {
                uVar12 = 0;
              }
              else {
                uVar12 = (uint)*(ushort *)(DAT_005c84bc + (iVar16 + iVar5) * 2);
              }
              uVar12 = uVar12 * 0x32 + DAT_005c8464;
              if ((DAT_005c8464 + 0x32U <= uVar12) &&
                 (uVar12 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
                *(byte *)(uVar12 + 0xc) = *(byte *)(uVar12 + 0xc) & 0xf9;
                FUN_004214a0((int)(uVar12 - DAT_005c8464) / 0x32);
              }
            }
            iVar10 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
            if ((((*(uint *)(iVar10 + 0xe8) & 0x800) != 0) && (DAT_005e4794 != 0)) &&
               (iVar10 = FUN_0040eaf0((float)iVar5,(float)local_28),
               iVar10 != ((int)param_1 - DAT_005c8464) / 0x32)) {
              FUN_004e0620("getGridSid( (float)fx, (float)fy) == getSid()","\\Ns\\O\\Squid.cpp",
                           0x629);
            }
          }
          local_28 = local_28 + 1;
          iVar16 = iVar16 + 0x100;
          iVar5 = iVar15;
        } while (local_28 <= iVar9);
      }
    }
    if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
LAB_004b01c2:
      FUN_004ac6b0();
      FUN_004ad230(((int)param_1 - DAT_005c8464) / 0x32);
      return;
    }
    iVar7 = 0;
    bVar4 = false;
    do {
      if (bVar4) goto LAB_004b0294;
      puVar13 = (ushort *)
                FUN_004b2a90(*(undefined4 *)((int)param_1 + 0xe),
                             *(undefined4 *)((int)param_1 + 0x12),iVar7);
      uVar2 = *puVar13;
      uVar3 = 0;
      while (uVar12 = (uint)uVar2, uVar12 != 0) {
        iVar8 = uVar12 * 0x32 + DAT_005c8464;
        if (uVar12 == iVar14 / 0x32) {
          if (uVar3 == 0) {
            *puVar13 = *(ushort *)(iVar8 + 4);
          }
          else {
            FUN_004aba70(*(undefined2 *)(iVar8 + 4));
          }
          bVar4 = true;
          break;
        }
        uVar3 = uVar12;
        uVar2 = *(ushort *)(iVar8 + 4);
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 4);
    if (!bVar4) {
      FUN_004e0620("found","\\Ns\\O\\Squid.cpp",0x65d);
    }
LAB_004b0294:
    if ((param_2 & 0x2000) != 0) {
      (**(code **)(*param_1 + 0x8c))();
      return;
    }
    (**(code **)(*param_1 + 0x88))();
  }
  return;
}


// ==== FUN_0049ebb0 @ 0049ebb0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0049ebb0(void)

{
  float fVar1;
  float fVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined **ppuVar9;
  char local_100 [256];
  
  uVar3 = FUN_00459c60(0xfffffffe,PTR_DAT_00540cec,0xfffffffe,PTR_DAT_00540cf8,0xfffffffd,
                       "_shapes.shp",0,0);
  iVar4 = FUN_0041ad10(uVar3);
  if (iVar4 == 0) {
    FUN_004e0620("filename","\\Ns\\O\\Rifttype.cpp",0xa8);
  }
  DAT_0059ab24 = FUN_004dd580(iVar4);
  if (DAT_0059ab24 == 0) {
    FUN_004e0620("masterShapeDatabasePtr","\\Ns\\O\\Rifttype.cpp",0xaa);
  }
  iVar4 = DAT_00541348;
  DAT_0059ab20 = operator_new(DAT_00541348 * 500);
  if (DAT_0059ab20 == (void *)0x0) {
    DAT_0059ab20 = (void *)0x0;
  }
  else if (-1 < iVar4 + -1) {
    puVar7 = (undefined4 *)((int)DAT_0059ab20 + 0x9c);
    do {
      puVar8 = puVar7 + -0x27;
      for (iVar5 = 0x7d; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      puVar7[-1] = 5;
      *puVar7 = 10;
      puVar7[0x21] = 0xffffffff;
      puVar7[0x23] = 0xffffffff;
      puVar7[0x54] = 0xffffffff;
      puVar7 = puVar7 + 0x7d;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  FUN_0049c2e0();
  *(undefined1 **)(DAT_00541078 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004ae3d0;
  *(undefined1 **)(DAT_00541074 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004ae760;
  *(undefined1 **)(DAT_00541088 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004aea40;
  *(undefined1 **)(DAT_0054107c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004ae9f0;
  iVar4 = 10;
  if (10 < DAT_0054116c) {
    iVar5 = 5000;
    ppuVar9 = &PTR_s_evilRaySystemProcessType_00540fa0;
    do {
      _strncpy((char *)(iVar5 + 4 + (int)DAT_0059ab20),*ppuVar9,0x14);
      iVar4 = iVar4 + 1;
      ppuVar9 = ppuVar9 + 1;
      iVar5 = iVar5 + 500;
    } while (iVar4 < DAT_0054116c);
  }
  *(undefined1 **)(DAT_005410cc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048be40;
  *(undefined1 **)(DAT_005410d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bf30;
  *(undefined1 **)(DAT_00541108 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bed0;
  *(undefined1 **)(DAT_0054110c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bea0;
  *(undefined1 **)(DAT_00541110 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bf90;
  *(undefined1 **)(DAT_00541114 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bf00;
  *(undefined1 **)(DAT_00541118 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048be70;
  *(undefined1 **)(DAT_00541148 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bf60;
  *(code **)(DAT_00541154 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00420ad0;
  *(undefined1 **)(DAT_00541170 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00451450;
  *(undefined1 **)(DAT_00541174 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046a930;
  *(undefined1 **)(DAT_00541178 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045b580;
  *(undefined1 **)(DAT_0054117c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045fbd0;
  *(undefined1 **)(DAT_00541184 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0041c300;
  *(undefined1 **)(DAT_00541188 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0041c300;
  *(undefined1 **)(DAT_0054118c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0041c300;
  *(undefined1 **)(DAT_00541190 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048ef00;
  *(undefined1 **)(DAT_00541194 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048f060;
  *(undefined1 **)(_DAT_005411a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00422ba0;
  *(undefined1 **)(DAT_005411a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046a6c0;
  *(undefined1 **)(DAT_005411ac * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00454db0;
  *(undefined1 **)(DAT_005411b0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00454db0;
  *(undefined1 **)(DAT_005411b4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00454db0;
  *(undefined1 **)(DAT_005411b8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00454db0;
  *(undefined1 **)(DAT_005411c0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00455af0;
  *(undefined1 **)(DAT_005411d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00443b70;
  *(undefined1 **)(DAT_005411d4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00443b90;
  *(undefined1 **)(DAT_005411e4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00486820;
  *(undefined1 **)(DAT_005411e8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00462610;
  *(undefined1 **)(DAT_00541338 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00462610;
  *(undefined1 **)(DAT_0054133c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00462610;
  *(undefined1 **)(DAT_005411ec * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00462610;
  *(undefined1 **)(DAT_005411f4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004737c0;
  *(undefined1 **)(DAT_005411fc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045acd0;
  *(undefined1 **)(DAT_00541214 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042b010;
  *(undefined1 **)(DAT_00541218 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004625f0;
  *(undefined1 **)(DAT_0054121c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e40;
  *(undefined1 **)(DAT_00541220 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e40;
  *(undefined1 **)(DAT_00541224 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e40;
  *(undefined1 **)(DAT_00541228 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e60;
  *(undefined1 **)(DAT_0054122c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e80;
  *(undefined1 **)(DAT_00541230 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00421040;
  *(undefined1 **)(DAT_00541340 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004dd0d0;
  *(undefined1 **)(DAT_00541344 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042aff0;
  *(undefined1 **)(DAT_00541234 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00421060;
  *(undefined1 **)(DAT_00541238 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00421080;
  *(undefined1 **)(DAT_0054123c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00462630;
  *(undefined1 **)(DAT_00541240 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004857a0;
  *(undefined1 **)(DAT_00541244 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004dd0a0;
  *(undefined1 **)(DAT_00541248 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004dced0;
  *(undefined1 **)(DAT_0054124c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004dd280;
  *(undefined1 **)(DAT_00541250 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00454de0;
  *(undefined1 **)(DAT_00541258 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046b810;
  *(undefined1 **)(DAT_0054125c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046b810;
  *(undefined1 **)(DAT_00541264 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046ae50;
  *(undefined1 **)(DAT_00541268 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046a6c0;
  *(undefined1 **)(DAT_00541270 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00443fc0;
  *(undefined1 **)(DAT_00541274 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042afb0;
  *(undefined1 **)(DAT_00541278 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00457e80;
  *(undefined1 **)(DAT_0054127c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042af90;
  *(undefined1 **)(DAT_00541280 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045fbb0;
  *(undefined1 **)(DAT_00541284 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045c260;
  *(undefined1 **)(DAT_00541288 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046add0;
  *(undefined1 **)(DAT_0054128c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042b010;
  *(undefined1 **)(DAT_00541290 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048f080;
  *(undefined1 **)(DAT_00541294 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045c240;
  *(undefined1 **)(DAT_00541298 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045fbf0;
  *(undefined1 **)(DAT_0054129c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048eab0;
  *(undefined1 **)(DAT_005412a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042b030;
  *(undefined1 **)(DAT_005412a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048ef00;
  *(undefined1 **)(DAT_005412a8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0046adf0;
  *(undefined1 **)(DAT_005412ac * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048ea90;
  *(undefined1 **)(DAT_005412b0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048ed80;
  *(code **)(DAT_005412bc * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004429e0;
  *(undefined1 **)(DAT_005412c0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00486870;
  *(undefined1 **)(DAT_005412c8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0042afd0;
  *(undefined1 **)(DAT_005412cc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00443bb0;
  *(undefined1 **)(DAT_005412d0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004952a0;
  *(code **)(DAT_005412d4 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004429e0;
  *(undefined1 **)(DAT_005412d8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045ae00;
  *(undefined1 **)(DAT_005412dc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004794e0;
  *(undefined1 **)(DAT_005412e4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045d2a0;
  *(undefined1 **)(DAT_00541310 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420fa0;
  *(undefined1 **)(DAT_00541318 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420fc0;
  *(undefined1 **)(DAT_0054131c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420fe0;
  *(undefined1 **)(DAT_005412e8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044aae0;
  *(undefined1 **)(DAT_005412ec * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044b200;
  *(undefined1 **)(DAT_005412f0 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044a460;
  *(undefined1 **)(DAT_005412f4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044ab00;
  *(undefined1 **)(DAT_005412f8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420e40;
  *(undefined1 **)(DAT_005412fc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0044a420;
  *(undefined1 **)(DAT_00541300 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004573b0;
  *(undefined1 **)(DAT_00541304 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420ea0;
  *(undefined1 **)(DAT_00541308 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00421000;
  *(undefined1 **)(DAT_0054130c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00421020;
  *(undefined1 **)(DAT_00541314 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420f20;
  *(undefined1 **)(DAT_00541320 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420f80;
  *(undefined1 **)(DAT_00541324 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420ec0;
  *(undefined1 **)(DAT_00541328 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420ee0;
  *(undefined1 **)(DAT_0054132c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420f00;
  *(undefined1 **)(DAT_00541330 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420f60;
  *(undefined1 **)(DAT_00541334 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00420f40;
  *(undefined1 **)(DAT_00541098 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_004536a0;
  *(code **)(DAT_0054109c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458700;
  *(code **)(DAT_005410a0 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458760;
  *(undefined1 **)(DAT_005410a4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00470b00;
  *(undefined1 **)(DAT_005410c8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00487ef0;
  *(code **)(DAT_00541138 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_004741d0;
  *(code **)(DAT_0054113c * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00474900;
  *(code **)(DAT_00541150 * 500 + 0x1d0 + (int)DAT_0059ab20) = FUN_00458700;
  *(undefined1 **)(DAT_005411bc * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0049a810;
  *(undefined1 **)(DAT_00541200 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0045ade0;
  *(undefined1 **)(DAT_00541208 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00479180;
  *(undefined1 **)(DAT_0054120c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00479180;
  *(undefined1 **)(DAT_00541210 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048f0b0;
  *(undefined1 **)(DAT_0054126c * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_0048bfc0;
  *(undefined1 **)(DAT_005412b8 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00491660;
  *(undefined1 **)(DAT_005412c4 * 500 + 0x1d0 + (int)DAT_0059ab20) = &LAB_00422720;
  iVar4 = 0;
  _DAT_0059ab18 = 0.0;
  _DAT_0059ab1c = 0.0;
  if (3 < DAT_00541348) {
    iVar5 = (DAT_00541348 - 4U >> 2) + 1;
    pfVar6 = (float *)((int)DAT_0059ab20 + 0x238);
    iVar4 = iVar5 * 4;
    do {
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[-0x67] == 1.4013e-45) {
        fVar1 = pfVar6[-0x7d] + _DAT_00500fc8;
        if (pfVar6[-0x7d] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fae1:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[-0x67] == 0.0) &&
              (fVar2 = pfVar6[-0x7d] + _DAT_00501908, pfVar6[-0x7d] + _DAT_00501908 <= _DAT_0059ab1c
              )) goto LAB_0049fae1;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x16] == 1.4013e-45) {
        fVar1 = *pfVar6 + _DAT_00500fc8;
        if (*pfVar6 + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fb29:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x16] == 0.0) &&
              (fVar2 = *pfVar6 + _DAT_00501908, *pfVar6 + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fb29;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x93] == 1.4013e-45) {
        fVar1 = pfVar6[0x7d] + _DAT_00500fc8;
        if (pfVar6[0x7d] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fb7c:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x93] == 0.0) &&
              (fVar2 = pfVar6[0x7d] + _DAT_00501908, pfVar6[0x7d] + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fb7c;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x110] == 1.4013e-45) {
        fVar1 = pfVar6[0xfa] + _DAT_00500fc8;
        if (pfVar6[0xfa] + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fbcf:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x110] == 0.0) &&
              (fVar2 = pfVar6[0xfa] + _DAT_00501908, pfVar6[0xfa] + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fbcf;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      pfVar6 = pfVar6 + 500;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  if (iVar4 < DAT_00541348) {
    pfVar6 = (float *)(iVar4 * 500 + 0x44 + (int)DAT_0059ab20);
    iVar4 = DAT_00541348 - iVar4;
    do {
      fVar1 = _DAT_0059ab18;
      fVar2 = _DAT_0059ab1c;
      if (pfVar6[0x16] == 1.4013e-45) {
        fVar1 = *pfVar6 + _DAT_00500fc8;
        if (*pfVar6 + _DAT_00500fc8 <= _DAT_0059ab18) {
LAB_0049fc36:
          fVar1 = _DAT_0059ab18;
          fVar2 = _DAT_0059ab1c;
        }
      }
      else if ((pfVar6[0x16] == 0.0) &&
              (fVar2 = *pfVar6 + _DAT_00501908, *pfVar6 + _DAT_00501908 <= _DAT_0059ab1c))
      goto LAB_0049fc36;
      _DAT_0059ab1c = fVar2;
      _DAT_0059ab18 = fVar1;
      pfVar6 = pfVar6 + 0x7d;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  uVar3 = FUN_004e49c0();
  _sprintf(local_100,"global.crossbowCost=%d",uVar3);
  FUN_00441580(local_100);
  return;
}


