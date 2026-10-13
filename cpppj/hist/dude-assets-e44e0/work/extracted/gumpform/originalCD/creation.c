// ==== FUN_004afd60 @ 004afd60

undefined4 * FUN_004afd60(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00505a80;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004b04a0 @ 004b04a0

undefined2 __fastcall FUN_004b04a0(int param_1)

{
  return *(undefined2 *)(param_1 + 0x10);
}


// ==== FUN_004b04b0 @ 004b04b0

undefined2 __fastcall FUN_004b04b0(int param_1)

{
  return *(undefined2 *)(param_1 + 0xe);
}


// ==== FUN_004b04c0 @ 004b04c0

void __thiscall FUN_004b04c0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x10) = param_2;
  return;
}


// ==== FUN_004b04d0 @ 004b04d0

void __thiscall FUN_004b04d0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xe) = param_2;
  return;
}


// ==== FUN_004afd40 @ 004afd40

void __fastcall FUN_004afd40(int param_1)

{
  if (*(int *)(param_1 + 0x12) != 0) {
    FUN_00492be0(*(int *)(param_1 + 0x12),*(undefined2 *)(param_1 + 0x10));
  }
  return;
}


// ==== FUN_004afd20 @ 004afd20

void __fastcall FUN_004afd20(int param_1)

{
  if (*(int *)(param_1 + 0x12) != 0) {
    FUN_00492be0(*(int *)(param_1 + 0x12),*(undefined2 *)(param_1 + 0x10));
  }
  return;
}


// ==== FUN_004b04e0 @ 004b04e0

void FUN_004b04e0(undefined4 param_1,undefined4 param_2)

{
  FUN_004ac2e0(param_1,param_2);
  return;
}


// ==== FUN_004ac2e0 @ 004ac2e0

void __thiscall FUN_004ac2e0(int *param_1,int param_2)

{
  int iVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  uint unaff_retaddr;
  
  if (param_2 == 0) {
    FUN_00490040(s_intoSid_00539e5c,s__ns_o_Squid_cpp_00539e4c,0x3c2);
  }
  uVar3 = FUN_004abae0();
  if ((uVar3 & 0x44200) != 0) {
    FUN_00490040(s___getGenus______gEMPLACEMENT_gVO_00539e74,s__ns_o_Squid_cpp_00539e64,0x3c3);
  }
  (**(code **)(*param_1 + 8))(param_2);
  uVar2 = *(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  iVar1 = param_2 * 0x24 + DAT_005395dc;
  if (((int)param_1 - DAT_005395dc) / 0x24 == (uint)uVar2) {
    FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
  }
  iVar4 = DAT_005395dc;
  *(ushort *)(param_1 + 1) = uVar2;
  if (uVar2 != 0) {
    (**(code **)(*(int *)((uint)uVar2 * 0x24 + iVar4) + 0xc))(((int)param_1 - iVar4) / 0x24);
    iVar4 = DAT_005395dc;
  }
  *(short *)(iVar1 + 6) = (short)(((int)param_1 - iVar4) / 0x24);
  (**(code **)(*param_1 + 0xc))(0);
  *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 8;
  FUN_004abfb0(unaff_retaddr | 0x100);
  if (*(byte *)((int)param_1 + 10) == DAT_0051c978) {
    FUN_00490040(s_getType____contentFormType_00539eb8,s__ns_o_Squid_cpp_00539ea8,0x3d2);
  }
  if (0x45 < *(byte *)((int)param_1 + 10)) {
    FUN_00490040(s_isForm___00539ee4,s__ns_o_Squid_cpp_00539ed4,0x3d5);
  }
  return;
}


// ==== FUN_004ac160 @ 004ac160

void __fastcall FUN_004ac160(int *param_1)

{
  int *piVar1;
  ushort uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  
  if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
    FUN_00490040(s_containedFlag_00539dc8,s__ns_o_Squid_cpp_00539db8,0x39a);
  }
  iVar3 = (**(code **)*param_1)();
  if (iVar3 == 0) {
    FUN_00490040(s_parentSid_00539de8,s__ns_o_Squid_cpp_00539dd8,0x39c);
  }
  piVar1 = (int *)(iVar3 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
    FUN_00490040(s__p_isFree___00539e04,s__ns_o_Squid_cpp_00539df4,0x39f);
  }
  if ((*(byte *)((int)piVar1 + 0xb) & 2) == 0) {
    uVar4 = ((int)param_1 - DAT_005395dc) / 0x24;
    if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
      FUN_00490040(s_containedFlag_00539e20,s__ns_o_Squid_cpp_00539e10,0x3a8);
    }
    uVar2 = *(ushort *)(param_1 + 1);
    uVar6 = (uint)uVar2;
    iVar3 = (**(code **)(*param_1 + 4))();
    if (iVar3 != 0) {
      iVar5 = iVar3 * 0x24 + DAT_005395dc;
      if ((iVar5 - DAT_005395dc) / 0x24 == uVar6) {
        FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
      }
      *(ushort *)(iVar5 + 4) = uVar2;
    }
    if (uVar6 != 0) {
      (**(code **)(*(int *)(DAT_005395dc + uVar6 * 0x24) + 0xc))(iVar3);
    }
    if (*(ushort *)((int)piVar1 + 6) == uVar4) {
      if (iVar3 != 0) {
        FUN_00490040(s_prev____0_00539e40,s__ns_o_Squid_cpp_00539e30,0x3b7);
      }
      *(ushort *)((int)piVar1 + 6) = uVar2;
    }
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xf7;
    if ((0x45 < *(byte *)((int)param_1 + 10)) || (*(byte *)((int)param_1 + 10) == DAT_0051c978)) {
      (**(code **)(*piVar1 + 0x24))(uVar4,2);
    }
  }
  return;
}


// ==== FUN_004af270 @ 004af270

void __fastcall FUN_004af270(undefined4 *param_1)

{
  int iVar1;
  
  if (0x45 < *(byte *)((int)param_1 + 10)) {
    FUN_00490040(s_isForm___0053a4fc,s__ns_o_Squid_cpp_0053a4ec,0xa10);
  }
  if ((param_1 < (undefined4 *)(DAT_005395dc + 0x24U)) ||
     ((undefined4 *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___0053a518,s__ns_o_Squid_cpp_0053a508,0xa11);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) != 0) {
    FUN_00490040(s__isVoid___0053a534,s__ns_o_Squid_cpp_0053a524,0xa12);
  }
  *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
  iVar1 = (**(code **)*param_1)();
  if ((*(byte *)(iVar1 * 0x24 + 0xb + DAT_005395dc) & 1) == 0) {
    FUN_004ac160();
  }
  return;
}


// ==== FUN_004abfb0 @ 004abfb0

void __thiscall FUN_004abfb0(int *param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___00539cf4,s__ns_o_Squid_cpp_00539ce4,0x352);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_00490040(s_isVoid___00539d10,s__ns_o_Squid_cpp_00539d00,0x353);
  }
  iVar2 = DAT_005395dc;
  bVar1 = *(byte *)((int)param_1 + 0xb);
  *(byte *)((int)param_1 + 0xb) = bVar1 & 0xfb;
  if ((bVar1 & 0x10) != 0) {
    FUN_00490040(s__xmitJustCreatedFlag_00539d2c,s__ns_o_Squid_cpp_00539d1c,0x356);
    iVar2 = DAT_005395dc;
  }
  if (DAT_00540a28 == 0) {
    if ((param_2 & 8) == 0) {
      param_2 = param_2 | 0x10;
    }
  }
  else {
    iVar2 = ((int)param_1 - iVar2) / 0x24;
    if ((iVar2 < 5) || (5999 < iVar2)) {
      if ((param_2 & 0x10) == 0) {
        param_2 = param_2 | 8;
      }
    }
    else {
      if ((param_2 & 8) != 0) {
        FUN_00490040(s___popFlags___pfSERVER_POP__00539d54,s__ns_o_Squid_cpp_00539d44,0x35b);
      }
      param_2 = param_2 | 0x10;
    }
    if ((param_2 & 0x50) == 0) {
      if ((param_2 & 7) != 0) {
        *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 0x10;
      }
      FUN_004abb40(0xffffffff,0xffffffff,0,param_2);
      *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xef;
    }
  }
  iVar2 = DAT_00539600;
  DAT_00539600 = DAT_00539600 + 1;
  (**(code **)(*param_1 + 0x20))(param_2);
  if (iVar2 != DAT_00539600) {
    FUN_00490040(s_curPostPop____basePostPop_00539d80,s__ns_o_Squid_cpp_00539d70,0x389);
  }
  return;
}


// ==== FUN_004abaa0 @ 004abaa0

void FUN_004abaa0(void)

{
  DAT_005395f8 = DAT_005395f8 + -1;
  return;
}


// ==== FUN_004aba90 @ 004aba90

void FUN_004aba90(void)

{
  DAT_005395fc = DAT_005395fc + -1;
  return;
}


// ==== FUN_004abab0 @ 004abab0

void FUN_004abab0(void)

{
  DAT_00539600 = DAT_00539600 + -1;
  return;
}


// ==== FUN_00492be0 @ 00492be0

void FUN_00492be0(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)(&DAT_0056b748)[param_1];
  if (piVar1 == (int *)0x0) {
    FUN_00490040(&DAT_0053600c,s_Gump_cpp_00536000,0x176);
  }
  (**(code **)(*piVar1 + 4))(param_2);
  return;
}


// ==== FUN_00492ff0 @ 00492ff0

void FUN_00492ff0(void)

{
  FUN_00493a40();
  return;
}


// ==== FUN_00492f80 @ 00492f80

void __thiscall FUN_00492f80(int param_1,int param_2)

{
  int *piVar1;
  int iVar2;
  
  if (param_2 != 0) {
    piVar1 = (int *)FUN_004ab390(DAT_0051c96c,2);
    *(undefined4 *)((int)piVar1 + 0x12) = *(undefined4 *)(param_1 + 0x1c);
    (**(code **)(*piVar1 + 0x74))(param_2,0);
    iVar2 = (int)piVar1 - DAT_005395dc;
    *(int *)(param_1 + 0x24) = param_2;
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40;
    *(int *)(param_1 + 0x28) = iVar2 / 0x24;
  }
  return;
}


// ==== FUN_004ab390 @ 004ab390

int * FUN_004ab390(int param_1,uint param_2)

{
  code *pcVar1;
  int iVar2;
  int *piVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  iVar2 = FUN_004aae40(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar2 < 14000) goto LAB_004ab3e2;
    uVar6 = 0x18b;
    pcVar5 = s__ns_o_Squid_cpp_005398f0;
    pcVar4 = s_sid_<_FIRST_PREDICTABLE_SID_00539900;
  }
  else {
    if (13999 < iVar2) goto LAB_004ab3e2;
    uVar6 = 0x188;
    pcVar5 = s__ns_o_Squid_cpp_005398c0;
    pcVar4 = s_sid_>__FIRST_PREDICTABLE_SID_005398d0;
  }
  FUN_00490040(pcVar4,pcVar5,uVar6);
LAB_004ab3e2:
  pcVar1 = *(code **)(param_1 * 0x1d4 + 0x1b0 + DAT_0051c960);
  if (pcVar1 == (code *)0x0) {
    piVar3 = (int *)(DAT_005395dc + iVar2 * 0x24);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar3 = (int *)(*pcVar1)(iVar2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  (**(code **)(*piVar3 + 0x28))();
  return piVar3;
}


// ==== FUN_004ab7e0 @ 004ab7e0

void __thiscall FUN_004ab7e0(int *param_1,uint param_2)

{
  int *piVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint unaff_retaddr;
  
  bVar2 = *(byte *)((int)param_1 + 0xb);
  if (((bVar2 & 2) == 0) &&
     (*(byte *)((int)param_1 + 0xb) = bVar2 | 2, iVar4 = DAT_005395f8, (bVar2 & 1) == 0)) {
    if ((DAT_00540a28 != 0) &&
       ((iVar3 = ((int)param_1 - DAT_005395dc) / 0x24, 4 < iVar3 && (iVar3 < 6000)))) {
      param_2 = param_2 | 0x10;
    }
    if (((DAT_00540a28 != 0) || ((param_2 & 8) != 0)) ||
       ((iVar3 = ((int)param_1 - DAT_005395dc) / 0x24, 4 < iVar3 && (iVar3 < 6000)))) {
      DAT_005395f8 = DAT_005395f8 + 1;
      iVar3 = FUN_004abf50(((int)param_1 - DAT_005395dc) / 0x24);
      FUN_0048ca70(0x27,s_destroying__s__d_00539a60,iVar3 + 4);
      (**(code **)(*param_1 + 0x14))(param_2);
      if (iVar4 != DAT_005395f8) {
        FUN_00490040(s_baseDestroy____curDestroy_00539a84,s__ns_o_Squid_cpp_00539a74,0x26d);
      }
      iVar4 = (int)param_1 - DAT_005395dc;
      uVar5 = (uint)*(ushort *)((int)param_1 + 6);
      while (uVar5 != 0) {
        piVar1 = (int *)(uVar5 * 0x24 + DAT_005395dc);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
          FUN_00490040(s__s_isFree___00539ab0,s__ns_o_Squid_cpp_00539aa0,0x274);
        }
        if (((*(byte *)((int)piVar1 + 0xb) & 8) == 0) && (0x45 < *(byte *)((int)piVar1 + 10))) {
          FUN_00490040(s_s_isContained______s_isForm___00539acc,s__ns_o_Squid_cpp_00539abc,0x275);
        }
        uVar5 = (uint)*(ushort *)(piVar1 + 1);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) != 0) {
          FUN_00490040(s__s_isFree___00539afc,s__ns_o_Squid_cpp_00539aec,0x277);
        }
        (**(code **)(*piVar1 + 0x1c))(iVar4 / 0x24);
        if ((*(byte *)((int)piVar1 + 0xb) & 1) == 0) {
          (**(code **)(*piVar1 + 0x10))(unaff_retaddr | 0x40);
        }
      }
      if ((*(byte *)((int)param_1 + 0xb) & 1) == 0) {
        if ((((unaff_retaddr & 0x50) == 0) && (DAT_00540a28 != 0)) &&
           ((iVar4 = ((int)param_1 - DAT_005395dc) / 0x24, iVar4 < 5 || (5999 < iVar4)))) {
          FUN_004abb40(0xffffffff,0xfffffffe,0,unaff_retaddr | 8);
        }
        iVar4 = (int)param_1 - DAT_005395dc;
        iVar3 = FUN_0041ea30();
        if (iVar4 / 0x24 == iVar3) {
          FUN_0041ed70();
        }
        if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
           ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
          FUN_00490040(s_isValid___00539b18,s__ns_o_Squid_cpp_00539b08,0x299);
        }
        if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
          (**(code **)(*param_1 + 0x48))(0);
        }
        iVar4 = DAT_005395fc;
        DAT_005395fc = DAT_005395fc + 1;
        (**(code **)(*param_1 + 0x18))(unaff_retaddr);
        if (iVar4 != DAT_005395fc) {
          FUN_00490040(s_curPostDestroy____basePostDestro_00539b34,s__ns_o_Squid_cpp_00539b24,0x2a0)
          ;
        }
        FUN_004ab250(((int)param_1 - DAT_005395dc) / 0x24);
      }
    }
  }
  return;
}


