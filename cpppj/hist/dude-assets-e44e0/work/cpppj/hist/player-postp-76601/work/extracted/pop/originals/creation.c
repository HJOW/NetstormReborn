// ==== FUN_004b02d0 @ 004b02d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004b02d0(int *param_1,float param_2,float param_3,uint param_4)

{
  byte bVar1;
  bool bVar2;
  bool bVar3;
  byte bVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  short *psVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  undefined4 unaff_EBX;
  undefined4 unaff_EBP;
  int iVar15;
  undefined4 unaff_ESI;
  int iVar16;
  uint uVar17;
  int iVar18;
  undefined8 uVar19;
  int local_14;
  
  if (((((int *)(DAT_005c8464 + 0x32U) <= param_1) &&
       (param_1 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
      ((*(byte *)((int)param_1 + 0xb) & 4) != 0)) && ((*(byte *)((int)param_1 + 0xb) & 8) == 0)) {
    if (((param_2 <= _DAT_00500fc4) || ((float)DAT_00531928 <= param_2)) ||
       ((param_3 <= _DAT_00500fc4 || ((float)DAT_00531928 <= param_3)))) {
      if (DAT_00594fc8 == 0) {
        iVar5 = FUN_0049a840();
        FUN_004c2630(0x1d,"Error %s popped to( %5.1f,%5.1f). Set to( 10,10)\n",iVar5 + 4,
                     (double)param_2,(double)param_3);
      }
      param_2 = 10.0;
      param_3 = 10.0;
    }
    uVar19 = CONCAT44(unaff_EBX,unaff_ESI);
    FUN_004210a0(param_2,param_3);
    iVar5 = FUN_0049a840();
    uVar12 = *(uint *)(iVar5 + 0xec);
    if (((uVar12 & 6) != 0) && ((*(byte *)(param_1 + 10) & 8) == 0)) {
      FUN_00481510(*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),uVar12,
                   uVar19);
    }
    if (((((uVar12 & (DAT_005424b8 | 0x400ff)) != 0) ||
         (iVar6 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10)),
         (*(uint *)(iVar6 + 0xe8) & 0x800) != 0)) && ((*(byte *)(param_1 + 10) & 8) == 0)) &&
       ((*(byte *)((int)param_1 + 0xb) & 8) == 0)) {
      FUN_004e49c0();
      iVar6 = FUN_004e49c0();
      FUN_004e49c0();
      iVar7 = FUN_004e49c0();
      iVar15 = (iVar6 - *(int *)(iVar5 + 0x1d4)) + 1;
      iVar16 = (iVar7 - *(int *)(iVar5 + 0x1d8)) + 1;
      iVar5 = FUN_0040e800();
      if (iVar5 == 0) {
        FUN_004e0620("start.isValid()","\\Ns\\O\\Squid.cpp",0x6b7);
      }
      if (iVar16 <= iVar7) {
        local_14 = iVar16 * 0x100;
        iVar5 = iVar15;
        iVar18 = iVar16;
        do {
          for (; iVar5 <= iVar6; iVar5 = iVar5 + 1) {
            bVar4 = FUN_004afd30(iVar5,iVar18);
            if (((iVar5 == iVar15) || (iVar5 == iVar6)) ||
               ((iVar18 == iVar16 || (bVar3 = true, iVar18 == iVar7)))) {
              bVar3 = false;
            }
            if (bVar4 != 0) {
              iVar8 = FUN_004e49c0();
              iVar9 = FUN_004e49c0();
              bVar1 = *(byte *)(iVar9 + iVar8 * 0x100 + DAT_005c7c44);
              if ((bVar4 & bVar1) != 0) {
                return;
              }
              *(byte *)(local_14 + DAT_005c7c44 + iVar5) = bVar4 | bVar1;
            }
            if (((iVar5 == iVar15) || (iVar5 == iVar6)) && ((iVar18 != iVar16 && (iVar18 != iVar7)))
               ) {
              bVar2 = true;
            }
            else {
              bVar2 = false;
            }
            if (((DAT_005424b8 & uVar12) != 0) && (bVar2)) {
              iVar8 = FUN_0040da00(iVar5,iVar18);
              uVar17 = iVar8 * 0x32 + DAT_005c8464;
              if ((DAT_005c8464 + 0x32U <= uVar17) &&
                 (((uVar17 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464) &&
                   (iVar8 = FUN_00425df0(), iVar8 != 0)) && (!bVar3)))) {
                iVar8 = FUN_0046f6e0(*(undefined4 *)((int)param_1 + 0xe),
                                     *(undefined4 *)((int)param_1 + 0x12));
                if (iVar8 == 0) {
                  *(byte *)(uVar17 + 0xc) = *(byte *)(uVar17 + 0xc) & 0xf9;
                }
                else {
                  *(ushort *)(uVar17 + 0xc) = *(ushort *)(uVar17 + 0xc) & 0xfffb | 2;
                }
                uVar19 = CONCAT44(unaff_EBP,(int)(uVar17 - DAT_005c8464) / 0x32);
                FUN_004214a0(uVar19);
                unaff_EBP = (undefined4)((ulonglong)uVar19 >> 0x20);
              }
            }
            iVar8 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
            if ((*(uint *)(iVar8 + 0xe8) & 0x800) != 0) {
              iVar8 = FUN_0040eaf0((float)iVar5,(float)iVar18);
              if (iVar8 != 0) {
                iVar8 = FUN_0040eaf0((float)iVar5,(float)iVar18);
                iVar8 = iVar8 * 0x32 + DAT_005c8464;
                if (DAT_005e4794 != 0) {
                  FUN_004e0620("false","\\Ns\\O\\Squid.cpp",0x6e8);
                }
                iVar13 = iVar8 - DAT_005c8464;
                iVar9 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
                iVar14 = (int)param_1 - DAT_005c8464;
                iVar8 = FUN_0049a840(*(undefined1 *)(iVar8 + 10));
                FUN_004c2630(0x1d,"ERROR: gridSid %d( %s) replacing %d( %s)\n",iVar14 / 0x32,
                             iVar9 + 4,iVar13 / 0x32,iVar8 + 4);
              }
            }
          }
          iVar18 = iVar18 + 1;
          local_14 = local_14 + 0x100;
          iVar5 = iVar15;
        } while (iVar18 <= iVar7);
      }
    }
    uVar10 = FUN_004ace40();
    psVar11 = (short *)FUN_004b2a90(*(undefined4 *)((int)param_1 + 0xe),
                                    *(undefined4 *)((int)param_1 + 0x12),uVar10);
    FUN_004aba70(*psVar11);
    iVar5 = (int)param_1 - DAT_005c8464;
    *psVar11 = ((short)(iVar5 / 0x32) + (short)(iVar5 >> 0x1f)) -
               (short)((longlong)iVar5 * 0x51eb851f >> 0x3f);
    if (*(char *)((int)param_1 + 0x21) == '\0') {
      FUN_004acd20(DAT_0050c038);
    }
    if ((param_4 & 0x2000) == 0) {
      (**(code **)(*param_1 + 0x88))();
    }
    else {
      (**(code **)(*param_1 + 0x8c))();
    }
    if ((*(byte *)(param_1 + 10) & 0x80) == 0) {
      uVar12 = (**(code **)(*param_1 + 0x30))();
      param_4 = param_4 | uVar12;
      *(byte *)(param_1 + 10) = *(byte *)(param_1 + 10) | 0x80;
    }
    FUN_004ac560(param_4);
    if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
      FUN_004e0620("!isContained()","\\Ns\\O\\Squid.cpp",0x71a);
    }
  }
  return;
}


// ==== FUN_004ac560 @ 004ac560

void __thiscall FUN_004ac560(int *param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  
  if ((param_1 < (int *)(DAT_005c8464 + 0x32U)) ||
     ((int *)(DAT_005c847c * 0x32 + DAT_005c8464) <= param_1)) {
    FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0x35b);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_004e0620("isVoid()","\\Ns\\O\\Squid.cpp",0x35c);
  }
  bVar1 = *(byte *)((int)param_1 + 0xb);
  *(byte *)((int)param_1 + 0xb) = bVar1 & 0xfb;
  if ((bVar1 & 0x10) != 0) {
    FUN_004e0620("!xmitJustCreatedFlag","\\Ns\\O\\Squid.cpp",0x35f);
  }
  if (DAT_00540bc0 == 0) {
    if ((param_2 & 8) == 0) {
      param_2 = param_2 | 0x10;
    }
  }
  else {
    iVar2 = ((int)param_1 - DAT_005c8464) / 0x32;
    if ((iVar2 < 5) || (14999 < iVar2)) {
      if ((param_2 & 0x10) == 0) {
        param_2 = param_2 | 8;
      }
    }
    else {
      if ((param_2 & 8) != 0) {
        FUN_004e0620("!(popFlags & pfSERVER_POP)","\\Ns\\O\\Squid.cpp",0x364);
      }
      param_2 = param_2 | 0x10;
    }
    if ((param_2 & 0x50) == 0) {
      if ((param_2 & 7) != 0) {
        *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 0x10;
      }
      FUN_004ac260(0xffffffff,0xffffffff,0,param_2);
      *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xef;
    }
  }
  iVar2 = DAT_005c8488;
  DAT_005c8488 = DAT_005c8488 + 1;
  (**(code **)(*param_1 + 0x20))(param_2);
  if (iVar2 != DAT_005c8488) {
    FUN_004e0620("curPostPop == basePostPop","\\Ns\\O\\Squid.cpp",0x392);
  }
  return;
}


// ==== FUN_004b0d30 @ 004b0d30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004b0d30(int param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  
  if (DAT_00594fc8 != 0) {
    DAT_005c8488 = DAT_005c8488 + -1;
    return;
  }
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar2 + 0xec) & (DAT_005424b8 | 8)) != 0) {
    if (DAT_00540414 == 0) goto LAB_004b0e4c;
    if (((param_2 & 3) != 0) && ((param_2 & 0x2000000) == 0)) {
      FUN_00462d40((param_1 - DAT_005c8464) / 0x32);
    }
  }
  if (DAT_00540414 != 0) {
    if (((param_2 & 0x203) == 0) || ((param_2 & 0x2000000) != 0)) {
      iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
      if (((*(uint *)(iVar2 + 0xe8) & 0x800) != 0) &&
         (((param_2 & 1) != 0 && ((param_2 & 0x2000000) != 0)))) {
        FUN_004ad3e0(DAT_00540410);
      }
    }
    else {
      if ((param_2 & 0x800) != 0) {
        FUN_004e0620("!(popFlags&pfDESTROY_GRAPH)","\\Ns\\O\\Squid.cpp",0x80b);
      }
      iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
      if ((*(uint *)(iVar2 + 0xe8) & 0x800) != 0) {
        FUN_004633c0((param_1 - DAT_005c8464) / 0x32);
      }
    }
  }
LAB_004b0e4c:
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  uVar1 = *(uint *)(iVar2 + 0xec);
  if (((param_2 & 1) != 0) && ((*(byte *)(param_1 + 0x28) & 9) == 0)) {
    _DAT_0059508c = FUN_004e49c0();
    iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
    if ((*(uint *)(iVar2 + 0xe8) & 0x10000000) != 0) {
      FUN_00472f70((param_1 - DAT_005c8464) / 0x32);
    }
    if ((uVar1 & 0x4200) != 0) {
      FUN_00490050(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32);
      FUN_0044f770((param_1 - DAT_005c8464) / 0x32);
    }
    FUN_004166e0(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32,1);
    if (*(byte *)(param_1 + 0x22) == DAT_00540c70) {
      FUN_004c25e0(*(undefined1 *)(param_1 + 10));
    }
    FUN_004c25b0(*(undefined1 *)(param_1 + 10));
    FUN_004da650();
  }
  DAT_005c8488 = DAT_005c8488 + -1;
  return;
}


// ==== FUN_004ad470 @ 004ad470

uint __fastcall FUN_004ad470(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  bVar1 = *(byte *)(param_1 + 0x28);
  uVar2 = 0;
  if (((bVar1 & 1) == 0) && ((bVar1 & 8) == 0)) {
    return 1;
  }
  if ((bVar1 & 1) != 0) {
    uVar2 = 2;
  }
  if ((bVar1 & 8) != 0) {
    uVar2 = uVar2 | 4;
  }
  return uVar2;
}


// ==== FUN_004ad500 @ 004ad500

void __fastcall FUN_004ad500(int param_1)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar1 = *(int *)(param_1 + 0x24);
  if (DAT_005e4794 == 0) goto LAB_004ad580;
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    if ((piVar2[0x45] <= iVar1) || (iVar1 < 0)) goto LAB_004ad54f;
LAB_004ad5ea:
    if (((piVar2[0x37] != 0) && (*piVar2 != -0x22222223)) && (*piVar2 != -0x32323233))
    goto LAB_004ad580;
  }
  else {
    if ((iVar1 < piVar2[0x45] * 2) && (-1 < iVar1)) goto LAB_004ad5ea;
LAB_004ad54f:
    if (DAT_005e4794 != 0) {
      uVar3 = FUN_0049adc0(iVar1,*(byte *)(param_1 + 0x28) & 1);
      FUN_00436d40(uVar3);
    }
  }
  FUN_004e0620("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp",
               0x846);
LAB_004ad580:
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    iVar4 = piVar2[0x45];
  }
  else {
    iVar4 = piVar2[0x45] * 2;
  }
  if ((iVar1 < iVar4) && (-1 < iVar1)) {
    if ((piVar2[0x37] != 0) && ((*piVar2 != -0x22222223 && (*piVar2 != -0x32323233)))) {
      FUN_004990d0(*(undefined1 *)(param_1 + 10),iVar1,*(undefined4 *)(param_1 + 0xe),
                   *(undefined4 *)(param_1 + 0x12),(*(byte *)(param_1 + 0x28) >> 2 & 1) << 4);
    }
  }
  else if (DAT_005e4794 != 0) {
    uVar3 = FUN_0049adc0(iVar1,*(byte *)(param_1 + 0x28) & 1);
    FUN_00436d40(uVar3);
    return;
  }
  return;
}


// ==== FUN_004ad670 @ 004ad670

void __fastcall FUN_004ad670(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ad672. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_004210a0 @ 004210a0

void __thiscall FUN_004210a0(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined2 uVar1;
  
  *(undefined4 *)(param_1 + 0xe) = param_2;
  *(undefined4 *)(param_1 + 0x12) = param_3;
  uVar1 = FUN_004e49c0();
  *(undefined2 *)(param_1 + 0x16) = uVar1;
  uVar1 = FUN_004e49c0();
  *(undefined2 *)(param_1 + 0x18) = uVar1;
  return;
}


// ==== FUN_004acd20 @ 004acd20

void __thiscall FUN_004acd20(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x1000006) == 0) {
    FUN_004e0620("genus & (gISLAND|gBRIDGE|gISLAND3x3)","\\Ns\\O\\Squid.cpp",0x4b9);
  }
  else if ((*(uint *)(iVar1 + 0xec) & 4) == 0) {
    *(undefined2 *)(param_1 + 8) = (undefined2)param_2;
  }
  else if (param_2 != DAT_0050c038) {
    FUN_004e0620("newId == invalidIslandId","\\Ns\\O\\Squid.cpp",0x4c1);
    return;
  }
  return;
}


// ==== FUN_004afd30 @ 004afd30

uint __thiscall FUN_004afd30(int param_1,int param_2,int param_3)

{
  uint uVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_10;
  int local_c;
  int local_8;
  undefined4 local_4;
  
  iVar3 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  uVar1 = *(uint *)(iVar3 + 0xec);
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  FUN_0049a900(&local_10,&local_c,0);
  local_8 = *(undefined4 *)(param_1 + 0xe);
  local_4 = *(undefined4 *)(param_1 + 0x12);
  iVar3 = FUN_004e49c0();
  local_8 = FUN_004e49c0();
  iVar6 = (iVar3 - local_10) + 1;
  iVar5 = (local_8 - local_c) + 1;
  if ((((param_2 == iVar6) || (param_2 == iVar3)) || (param_3 == iVar5)) ||
     (bVar2 = true, param_3 == local_8)) {
    bVar2 = false;
  }
  iVar4 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((((*(uint *)(iVar4 + 0xec) & DAT_005424b8) != 0) && (bVar2)) &&
     (((*(uint *)(iVar4 + 0xec) & 0x400000) == 0 ||
      ((param_2 * 2 - iVar6 != iVar3 ||
       (iVar3 = (param_3 - iVar5) * 2, iVar3 - (local_8 - iVar5) != 0 && local_8 - iVar5 <= iVar3)))
      ))) {
    return uVar1 | 8;
  }
  return uVar1;
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __fastcall FUN_004ac1e0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_004aba70 @ 004aba70

void __thiscall FUN_004aba70(int param_1,int param_2)

{
  if (param_2 == (param_1 - DAT_005c8464) / 0x32) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(short *)(param_1 + 4) = (short)param_2;
  return;
}


// ==== FUN_004b2a90 @ 004b2a90

int __thiscall FUN_004b2a90(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_004e0620("level >=0 && level < HASH_LEVEL_COUNT","\\Ns\\O\\Squidhash.cpp",0x52);
  }
  iVar1 = (&DAT_005424f8)[param_4];
  iVar2 = FUN_004e49c0();
  iVar2 = (iVar2 / iVar1) * DAT_00531928;
  iVar3 = FUN_004e49c0();
  return *(int *)(param_1 + 0xc + param_4 * 4) + (iVar2 / iVar1 + iVar3 / iVar1) * 2;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00419850 @ 00419850

int __thiscall FUN_00419850(int *param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  
  if ((param_1[0x3a] & 0x440000U) == 0) {
    if ((param_1[0x45] <= param_3) || (param_3 < 0)) goto LAB_004198a8;
LAB_00419887:
    if (((param_1[0x37] != 0) && (*param_1 != -0x22222223)) && (*param_1 != -0x32323233))
    goto LAB_004198d0;
  }
  else {
    if ((param_3 < param_1[0x45] * 2) && (-1 < param_3)) goto LAB_00419887;
LAB_004198a8:
    uVar1 = FUN_0049adc0(param_3,param_2);
    FUN_00436d40(uVar1);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_004198d0:
  return *(int *)(param_1[0x37] + 8 + param_3 * 8) + -0x24 + param_1[0x37];
}


// ==== FUN_0040e800 @ 0040e800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __fastcall FUN_0040e800(float *param_1)

{
  if ((((_DAT_00500fc4 < *param_1) && (*param_1 < (float)DAT_00531928)) &&
      (_DAT_00500fc4 < param_1[1])) && (param_1[1] < (float)DAT_00531928)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0040da00 @ 0040da00

undefined2 FUN_0040da00(int param_1,int param_2)

{
  if ((((-1 < param_1) && (-1 < param_2)) && (param_1 < 0x100)) && (param_2 < 0x100)) {
    return *(undefined2 *)(DAT_005c84bc + (param_2 * 0x100 + param_1) * 2);
  }
  return 0;
}


// ==== FUN_004ace40 @ 004ace40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004ace40(int param_1)

{
  float fVar1;
  undefined1 uVar2;
  int iVar3;
  float *pfVar4;
  
  iVar3 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(byte *)(iVar3 + 0xec) & 6) != 0) {
    *(undefined1 *)(param_1 + 0x21) = 0;
    return;
  }
  pfVar4 = (float *)FUN_00419850(0,*(undefined4 *)(param_1 + 0x24));
  if (*pfVar4 <= pfVar4[1]) {
    fVar1 = pfVar4[1];
  }
  else {
    fVar1 = *pfVar4;
  }
  if (fVar1 < _DAT_00501444 != (fVar1 == _DAT_00501444)) {
    *(undefined1 *)(param_1 + 0x21) = 1;
    return;
  }
  uVar2 = 2;
  if (fVar1 < _DAT_00502314 == (fVar1 == _DAT_00502314)) {
    uVar2 = 3;
  }
  *(undefined1 *)(param_1 + 0x21) = uVar2;
  return;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1d8);
  return;
}


// ==== FUN_0040eaf0 @ 0040eaf0

undefined2 FUN_0040eaf0(void)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((((-1 < iVar1) && (-1 < iVar2)) && (iVar1 < 0x100)) && (iVar2 < 0x100)) {
    return *(undefined2 *)(DAT_00542514 + (iVar2 * 0x100 + iVar1) * 2);
  }
  return 0;
}


// ==== FUN_00425df0 @ 00425df0

uint FUN_00425df0(void)

{
  uint uVar1;
  
  uVar1 = FUN_004ac1e0();
  return uVar1 & 0x800;
}


// ==== FUN_00498160 @ 00498160

bool __thiscall FUN_00498160(undefined4 param_1)

{
  int unaff_EBX;
  int unaff_ESI;
  bool bVar1;
  int in_stack_00000018;
  int in_stack_00000020;
  void *in_stack_000000a8;
  int in_stack_000000b8;
  
  FUN_004daa70(*(undefined4 *)(unaff_EBX + 0xdc),param_1);
  if ((in_stack_00000020 < 200) && (in_stack_00000018 < 200)) {
    bVar1 = *(char *)((int)&DAT_005caff0 + in_stack_000000b8 + (in_stack_00000020 * 200 - unaff_ESI)
                     ) != -1;
    FUN_004a2c70();
  }
  else {
    FUN_004a2c70();
    bVar1 = false;
  }
  ExceptionList = in_stack_000000a8;
  return bVar1;
}


// ==== FUN_00498220 @ 00498220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00498220(undefined4 *param_1)

{
  byte bVar1;
  int *piVar2;
  undefined4 uVar3;
  byte bVar4;
  uint uVar5;
  int *piVar6;
  int iVar7;
  undefined4 uVar8;
  int iVar9;
  undefined4 uVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  undefined1 *puVar21;
  int local_144;
  int local_140;
  int local_13c;
  int local_130;
  int *local_12c;
  uint local_120;
  undefined4 *local_11c;
  undefined4 local_118;
  int local_114;
  int *local_110;
  undefined1 local_10c [4];
  undefined1 local_108 [260];
  
  piVar2 = (int *)param_1[2];
  uVar5 = (uint)*(byte *)((int)piVar2 + 10);
  local_11c = param_1;
  piVar6 = (int *)FUN_0049a840(uVar5);
  if (piVar6[0x37] == 0) {
    return;
  }
  local_144 = piVar2[9];
  bVar1 = *(byte *)(piVar2 + 10);
  bVar4 = bVar1 >> 4;
  local_118 = *(undefined4 *)((int)piVar2 + 0xe);
  local_114 = *(undefined4 *)((int)piVar2 + 0x12);
  local_120 = piVar6[0x3b];
  iVar17 = *(short *)((int)piVar2 + 0x16) - DAT_0059a91c;
  iVar11 = (short)piVar2[6] - DAT_0059a920;
  local_110 = piVar6;
  iVar7 = FUN_004e49c0();
  iVar7 = iVar7 - DAT_0059a91c;
  local_114 = FUN_004e49c0();
  local_114 = local_114 - DAT_0059a920;
  if ((iVar7 != iVar17) || (local_114 != iVar11)) {
    uVar8 = FUN_004a89c0(local_108,"ty=%s fr=%d %d=%d %d=%d\n",piVar6 + 1,local_144,iVar7,iVar17,
                         local_114,iVar11);
    FUN_00436d20(uVar8);
  }
  if (iVar7 != iVar17) {
    FUN_004e0620("sc.x == sc_x",".\\Renderer.cpp",0xe9);
  }
  if (local_114 != iVar11) {
    FUN_004e0620("sc.y == sc_y",".\\Renderer.cpp",0xea);
  }
  iVar18 = iVar17 - piVar6[0x77];
  iVar7 = iVar11 - piVar6[0x78];
  if ((*(byte *)((int)local_11c + 0xf) & 4) != 0) {
    if ((bVar4 & 1) != 0) {
      return;
    }
    if (DAT_005318ec == 0) {
      *(int *)(DAT_005412d0 * 500 + 0xdc + DAT_0059ab20) = piVar6[0x37];
    }
    iVar11 = piVar6[0x45];
    if (iVar11 <= local_144) {
      local_144 = local_144 - iVar11;
    }
    if (DAT_0059a8ac != 0) {
      return;
    }
    if ((bVar1 & 1) != 0) {
      return;
    }
    FUN_004dab90(piVar6[0x37],iVar11 + local_144,iVar18,iVar7,&DAT_00557738);
    return;
  }
  if (uVar5 == DAT_005412e8) {
    uVar12 = (uint)*(ushort *)(piVar2 + 3);
  }
  else {
    uVar12 = (uint)*(byte *)((int)piVar2 + 0x22);
  }
  if ((bVar1 >> 1 & 1) == 0) {
    if (DAT_0059a8ac == 0) {
      if ((bVar1 & 1) != 0) goto LAB_004984f1;
      if ((bVar4 & 1) != 0) {
        if (uVar12 == DAT_00540c70) {
          FUN_004dab90(piVar6[0x37],local_144,iVar18,iVar7,&DAT_00556238);
        }
        goto LAB_004986ea;
      }
      if ((*(int *)(&DAT_00557940 + uVar5 * 4) == 0) || (uVar12 == 0)) {
        FUN_004daa70(piVar6[0x37],local_144,iVar18,iVar7);
      }
      else {
        FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar12 * 4) * 0x100 +
                     *(int *)(&DAT_00557940 + uVar5 * 4));
        FUN_004daab0(piVar6[0x37],local_144,iVar18,iVar7);
      }
    }
    else {
      if ((bVar1 & 1) != 0) {
LAB_004984f1:
        FUN_004dab90(piVar6[0x37],local_144,iVar18,iVar7,&DAT_00556438);
        goto LAB_004986ea;
      }
      if ((bVar4 & 1) != 0) {
        if (uVar12 == DAT_00540c70) {
          FUN_004dab90(piVar6[0x37],local_144,iVar18,iVar7,&DAT_00556238);
        }
        goto LAB_004986ea;
      }
      if ((((DAT_005424b4 & local_120) == 0) && (uVar5 != DAT_005412e8)) && (uVar5 != DAT_005411a8))
      {
        if ((*(int *)(&DAT_00557940 + uVar5 * 4) == 0) || (uVar12 == 0)) {
          FUN_004daa70(piVar6[0x37],local_144,iVar18,iVar7);
        }
        else {
          FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar12 * 4) * 0x100 +
                       *(int *)(&DAT_00557940 + uVar5 * 4));
          FUN_004daab0(piVar6[0x37],local_144,iVar18,iVar7);
        }
      }
      else {
        FUN_004dab90(piVar6[0x37],local_144,iVar18,iVar7,&DAT_00556238);
      }
    }
LAB_004986f6:
    if (((DAT_005319c0 != 0) && ((piVar6[0x3a] & 0x400U) != 0)) &&
       ((DAT_00594fbc != 0 &&
        (((uVar12 != 0 && (iVar9 = FUN_0040eaf0(*local_11c,local_11c[1]), iVar9 != 0)) &&
         (*(byte *)(iVar9 * 0x32 + 10 + DAT_005c8464) != DAT_005412cc)))))) {
      iVar9 = FUN_0049a840(DAT_005411d8,*(int *)(&DAT_00531b08 + uVar12 * 4) + -1,iVar18,iVar7);
      FUN_004daa70(*(undefined4 *)(iVar9 + 0xdc));
    }
    if ((*(byte *)(piVar2 + 10) & 0x20) != 0) {
      iVar9 = FUN_0049a840(DAT_00541230);
      FUN_0049a900(&local_120,&local_11c,0);
      FUN_004daa70(*(undefined4 *)(iVar9 + 0xdc),*(undefined4 *)(iVar9 + 0x118),
                   iVar18 - (int)local_120 / 2,iVar7 - (int)local_11c / 2);
    }
  }
  else {
    local_130 = piVar6[0x46];
    iVar9 = (**(code **)(*piVar2 + 0x78))();
    if (iVar9 != -1) {
      local_130 = iVar9;
    }
    if (DAT_0059a8ac == 0) {
      if ((bVar1 & 1) == 0) {
        if (iVar9 == -1) {
LAB_004986b3:
          (**(code **)(*piVar2 + 0xa4))(iVar18,iVar7);
        }
        else if ((bVar4 & 1) == 0) {
          if ((*(int *)(&DAT_00557940 + uVar5 * 4) == 0) || (uVar12 == 0)) {
            FUN_004daa70(piVar6[0x37],iVar9,iVar18,iVar7);
            goto LAB_004986b3;
          }
          FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar12 * 4) * 0x100 +
                       *(int *)(&DAT_00557940 + uVar5 * 4));
          FUN_004daab0(piVar6[0x37],iVar9,iVar18,iVar7);
          (**(code **)(*piVar2 + 0xa4))(iVar18,iVar7);
        }
        else {
          if (uVar12 != DAT_00540c70) goto LAB_004986b3;
          FUN_004dab90(piVar6[0x37],iVar9,iVar18,iVar7,&DAT_00556238);
          (**(code **)(*piVar2 + 0xa4))(iVar18,iVar7);
        }
        goto LAB_004986ea;
      }
LAB_004986cb:
      iVar13 = piVar6[0x37];
      puVar21 = &DAT_00556438;
      iVar9 = local_130;
    }
    else {
      if ((bVar1 & 1) != 0) goto LAB_004986cb;
      if ((bVar4 & 1) == 0) {
        if ((((DAT_005424b4 & local_120) == 0) && (uVar5 != DAT_005412e8)) &&
           (uVar5 != DAT_005411a8)) {
          if (iVar9 != -1) {
            if ((*(int *)(&DAT_00557940 + uVar5 * 4) == 0) || (uVar12 == 0)) {
              FUN_004daa70(piVar6[0x37],iVar9,iVar18,iVar7);
            }
            else {
              FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar12 * 4) * 0x100 +
                           *(int *)(&DAT_00557940 + uVar5 * 4));
              FUN_004daab0(piVar6[0x37],iVar9,iVar18,iVar7);
            }
          }
          (**(code **)(*piVar2 + 0xa4))(iVar18,iVar7);
        }
        else {
          FUN_004dab90(piVar6[0x37],local_130,iVar18,iVar7,&DAT_00556238);
        }
        goto LAB_004986f6;
      }
      if (uVar12 != DAT_00540c70) goto LAB_004986ea;
      iVar13 = piVar6[0x37];
      puVar21 = &DAT_00556238;
    }
    FUN_004dab90(iVar13,iVar9,iVar18,iVar7,puVar21);
LAB_004986ea:
    if ((bVar4 & 1) == 0) goto LAB_004986f6;
  }
  if ((*(byte *)(piVar2 + 10) & 4) == 0) {
    return;
  }
  if (DAT_00594fc8 != 0) {
    return;
  }
  iVar7 = FUN_00419850(bVar1 & 1,local_144);
  if ((piVar6[0x3a] & 0x440000U) == 0) {
    if (piVar6[0x45] < 1) goto LAB_00498817;
LAB_00498c82:
    if (((piVar6[0x37] != 0) && (*piVar6 != -0x22222223)) && (*piVar6 != -0x32323233))
    goto LAB_00498841;
  }
  else {
    if (0 < piVar6[0x45] << 1) goto LAB_00498c82;
LAB_00498817:
    uVar8 = FUN_0049adc0(0,bVar1 & 1);
    FUN_00436d40(uVar8);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_00498841:
  iVar18 = (int)*(short *)(iVar7 + 0x1a);
  iVar15 = (int)*(short *)(iVar7 + 0x1c);
  iVar13 = (int)*(short *)(iVar7 + 0x1e);
  iVar19 = (int)*(short *)(iVar7 + 0x18);
  _DAT_005cae80 = ((int)piVar2 - DAT_005c8464) / 0x32;
  FUN_0049a900(&local_118,local_10c,0);
  iVar20 = (iVar19 - iVar15) + iVar17;
  iVar14 = iVar17 - iVar15;
  iVar16 = (iVar18 - iVar13) + iVar11;
  iVar9 = (int)((iVar20 - iVar14) + (iVar20 - iVar14 >> 0x1f & 3U)) >> 2;
  iVar7 = (iVar16 - iVar11) + -1 + iVar13;
  uVar8 = *(undefined4 *)(&DAT_005319c4 + *(int *)(&DAT_00531b08 + uVar12 * 4) * 4);
  local_11c = (undefined4 *)((int)(iVar7 + (iVar7 >> 0x1f & 3U)) >> 2);
  iVar7 = iVar16 - (int)local_11c;
  FUN_004dade0(iVar14,iVar7,iVar14,iVar16,uVar8);
  FUN_004dade0(iVar14,iVar16,iVar9 + iVar14,iVar16,uVar8);
  FUN_004dade0(iVar20,iVar7,iVar20,iVar16,uVar8);
  FUN_004dade0(iVar20 - iVar9,iVar16,iVar20,iVar16,uVar8);
  iVar14 = iVar14 + 1;
  iVar20 = iVar20 + -1;
  iVar16 = iVar16 + -1;
  iVar7 = iVar16 - (int)local_11c;
  FUN_004dade0(iVar14,iVar7,iVar14,iVar16,uVar8);
  FUN_004dade0(iVar14,iVar16,iVar9 + iVar14,iVar16,uVar8);
  FUN_004dade0(iVar20,iVar7,iVar20,iVar16,uVar8);
  FUN_004dade0(iVar20 - iVar9,iVar16,iVar20,iVar16,uVar8);
  iVar9 = 0;
  iVar7 = FUN_004adc60();
  uVar8 = DAT_005b5dc4;
  if (iVar7 != 0) {
    iVar7 = *(int *)((int)piVar2 + 0x1a);
    iVar9 = FUN_0049fce0();
    if (iVar7 <= iVar9 / 2) {
      uVar8 = DAT_0059afd8;
    }
    iVar7 = *(int *)((int)piVar2 + 0x1a);
    iVar9 = FUN_0049fce0();
    if (iVar7 <= (int)(iVar9 + (iVar9 >> 0x1f & 3U)) >> 2) {
      uVar8 = DAT_005b5e58;
    }
    uVar3 = *(undefined4 *)((int)piVar2 + 0x1a);
    uVar10 = FUN_0049fce0(iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
    FUN_004972f0(0,uVar8,uVar3,uVar10);
    iVar9 = 1;
  }
  if (*(byte *)((int)piVar2 + 10) == DAT_005412d0) {
    iVar16 = (((int)piVar2 - DAT_005c8464) / 0x32) * 0x32 + DAT_005c8464;
    iVar7 = FUN_0049a840(*(undefined4 *)((int)piVar2 + 0x2a));
    iVar14 = FUN_00491af0();
    if (iVar14 != 0) {
      FUN_004972f0(iVar9,0x7b,*(int *)(iVar7 + 0x1bc) - *(int *)(iVar16 + 0x2e),
                   *(int *)(iVar7 + 0x1bc),iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
      iVar9 = iVar9 + 1;
    }
  }
  iVar7 = FUN_004afb40(0x493);
  if (iVar7 != 0) {
    iVar14 = FUN_004ac9f0(1,0,0);
    iVar14 = FUN_0049a840(*(undefined2 *)(iVar14 * 0x32 + 0x12 + DAT_005c8464));
    iVar14 = *(int *)(iVar14 + 0x1b8) * 10;
    FUN_004972f0(iVar9,0xbe,iVar14 - *(int *)(iVar7 + 0x20),iVar14,iVar15,iVar13,iVar19,iVar18,
                 iVar17,iVar11);
    iVar9 = iVar9 + 1;
  }
  if ((*(byte *)((int)piVar2 + 10) == DAT_00541280) &&
     (iVar7 = FUN_004b1a90(((int)piVar2 - DAT_005c8464) / 0x32,DAT_005410ec,0,0), iVar7 != 0)) {
    FUN_004ae2d0();
    uVar8 = FUN_004e49c0(0x14,iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
    FUN_004972f0(iVar9,0xd9,uVar8);
    iVar9 = iVar9 + 1;
  }
  if ((*(byte *)((int)piVar2 + 10) == DAT_00541298) &&
     (iVar7 = FUN_004b1a90(((int)piVar2 - DAT_005c8464) / 0x32,DAT_005410ec,0,0), iVar7 != 0)) {
    FUN_004ae2d0();
    uVar8 = FUN_004e49c0(0x3c,iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
    FUN_004972f0(iVar9,9,uVar8);
    iVar9 = iVar9 + 1;
  }
  if ((*(byte *)((int)piVar2 + 10) == DAT_0054117c) &&
     (iVar7 = FUN_004b1a90(((int)piVar2 - DAT_005c8464) / 0x32,DAT_005410ec,0,0), iVar7 != 0)) {
    FUN_004ae2d0();
    uVar8 = FUN_004e49c0(0x3c,iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
    FUN_004972f0(iVar9,0x6b,uVar8);
    iVar9 = iVar9 + 1;
  }
  if ((*(byte *)(piVar2 + 10) & 0x30) != 0) {
    local_140 = 0;
    local_120 = 0;
    local_12c = &DAT_0056b80c;
    do {
      if ((*local_12c != 0) && (iVar7 = FUN_0041bff0(), iVar7 == DAT_00541154)) {
        iVar7 = *local_12c;
        local_13c = 0;
        if (0 < *(int *)(iVar7 + 0x14)) {
          piVar6 = *(int **)(iVar7 + 0xc);
          do {
            if (*piVar6 == ((int)piVar2 - DAT_005c8464) / 0x32) {
              if ((*(uint *)(iVar7 + 0x20) & 1) == 0) {
                if (((*(uint *)(iVar7 + 0x20) & 2) != 0) &&
                   (uVar5 = FUN_004e49c0(), (int)local_120 <= (int)uVar5)) {
                  local_120 = uVar5;
                }
              }
              else {
                iVar7 = FUN_004e49c0();
                if (local_140 <= iVar7) {
                  local_140 = iVar7;
                }
              }
              break;
            }
            local_13c = local_13c + 1;
            piVar6 = piVar6 + 1;
          } while (local_13c < *(int *)(iVar7 + 0x14));
        }
      }
      local_12c = local_12c + 1;
    } while ((int)local_12c < 0x592908);
    if ((*(byte *)(piVar2 + 10) & 0x10) != 0) {
      iVar7 = FUN_0049a840(DAT_0054122c);
      FUN_004972f0(iVar9,0xc3,local_140,*(undefined4 *)(iVar7 + 0x1cc),iVar15,iVar13,iVar19,iVar18,
                   iVar17,iVar11);
      iVar9 = iVar9 + 1;
    }
    if ((*(byte *)(piVar2 + 10) & 0x20) != 0) {
      iVar7 = FUN_0049a840(DAT_00541230);
      FUN_004972f0(iVar9,0xfe,local_120,*(undefined4 *)(iVar7 + 0x1cc),iVar15,iVar13,iVar19,iVar18,
                   iVar17,iVar11);
      iVar9 = iVar9 + 1;
    }
  }
  if (*(byte *)((int)piVar2 + 10) == DAT_00541240) {
    uVar8 = FUN_004e49c0(iVar15,iVar13,iVar19,iVar18,iVar17,iVar11);
    FUN_004972f0(iVar9,0xf0,(short)piVar2[3],uVar8);
  }
  return;
}


