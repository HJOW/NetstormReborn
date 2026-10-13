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


// ==== FUN_004ab440 @ 004ab440

int * FUN_004ab440(uint param_1,int param_2)

{
  byte bVar1;
  code *pcVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int *piVar6;
  undefined1 local_100 [256];
  
  if ((param_2 < 6000) || (DAT_005395f4 <= param_2)) {
    FUN_00490040(s__FIRST_SERVER_SID_<__sid_&&_sid_<_0053992c,s__ns_o_Squid_cpp_0053991c,0x19e);
  }
  piVar6 = (int *)(param_2 * 0x24 + DAT_005395dc);
  if ((*(byte *)((int)piVar6 + 0xb) & 1) == 0) {
    if (param_1 != *(byte *)((int)piVar6 + 10)) {
      uVar5 = FUN_00420560(local_100,s_Invalid_take__Server_type__s__cl_0053995c,
                           param_1 * 0x1d4 + DAT_0051c960 + 4,
                           (uint)*(byte *)((int)piVar6 + 10) * 0x1d4 + DAT_0051c960 + 4);
      FUN_004875b0(uVar5);
      FUN_00490040(&DAT_0053999c,s__ns_o_Squid_cpp_0053998c,0x1b1);
    }
    (**(code **)(*piVar6 + 0x48))(0);
  }
  iVar4 = DAT_005395dc;
  iVar3 = DAT_0051c960;
  bVar1 = *(byte *)((int)piVar6 + 0xb);
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfe;
  *(byte *)((int)piVar6 + 0xb) = bVar1 & 0xfc;
  pcVar2 = *(code **)(param_1 * 0x1d4 + 0x1b0 + iVar3);
  if (pcVar2 == (code *)0x0) {
    piVar6 = (int *)(param_2 * 0x24 + iVar4);
    if (piVar6 == (int *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      *piVar6 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar6 = (int *)(*pcVar2)(param_2);
  }
  *(byte *)((int)piVar6 + 0xb) = *(byte *)((int)piVar6 + 0xb) | 4;
  *(char *)((int)piVar6 + 10) = (char)param_1;
  (**(code **)(*piVar6 + 0x2c))();
  return piVar6;
}


// ==== FUN_004ae040 @ 004ae040

void __fastcall FUN_004ae040(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  bool bVar3;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a108,s__ns_o_Squid_cpp_0053a0f8,0x7a6);
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a124,s__ns_o_Squid_cpp_0053a114,0x7a7);
  }
  bVar3 = DAT_00540a28 != 0;
  *(undefined1 *)(param_1 + 0x21) =
       *(undefined1 *)(DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x104);
  if (bVar3) {
    *(undefined1 *)(param_1 + 0x20) = 0;
    iVar1 = FUN_004aecf0();
    if (iVar1 != 0) {
      uVar2 = FUN_00444720();
      FUN_004aed20(uVar2);
    }
  }
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a144,s__ns_o_Squid_cpp_0053a134,0x7b1);
  }
  return;
}


// ==== FUN_004ae120 @ 004ae120

void __fastcall FUN_004ae120(int param_1)

{
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a164,s__ns_o_Squid_cpp_0053a154,0x7c8);
  }
  *(undefined1 *)(param_1 + 0x21) =
       *(undefined1 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x104 + DAT_0051c960);
  if ((*(byte *)(param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___0053a180,s__ns_o_Squid_cpp_0053a170,0x7cc);
  }
  return;
}


// ==== FUN_00444720 @ 00444720

int __fastcall FUN_00444720(int *param_1)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
    }
  }
  return *param_1;
}


// ==== FUN_004aecf0 @ 004aecf0

byte __fastcall FUN_004aecf0(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = FUN_004abae0();
  if ((uVar2 & 0x400000) == 0) {
    bVar1 = FUN_004abac0();
    return bVar1 & 0x10;
  }
  return *(byte *)(param_1 + 0x23) & 1;
}


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __fastcall FUN_004abac0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004448e0 @ 004448e0

int __fastcall FUN_004448e0(int param_1)

{
  return (param_1 - DAT_0051c960) / 0x1d4;
}


// ==== FUN_004ad0b0 @ 004ad0b0

void __thiscall FUN_004ad0b0(int *param_1,uint param_2)

{
  ushort uVar1;
  uint uVar2;
  bool bVar3;
  int iVar4;
  int iVar5;
  byte bVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  ushort *puVar14;
  int iVar15;
  int iStack_1c;
  
  if (*(byte *)((int)param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___00539f78,s__ns_o_Squid_cpp_00539f68,0x5bf);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040(s__isContained___00539f94,s__ns_o_Squid_cpp_00539f84,0x5c0);
  }
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040(s_isValid___00539fb4,s__ns_o_Squid_cpp_00539fa4,0x5c7);
  }
  iVar5 = DAT_005395dc;
  if ((*(byte *)((int)param_1 + 0xb) & 5) == 0) {
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
    iVar7 = FUN_004abf50();
    uVar2 = *(uint *)(iVar7 + 0xec);
    if (((uVar2 & 6) != 0) && ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) {
      FUN_00442f30(*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),uVar2);
    }
    if (((((uVar2 & (DAT_005395cc | 0x400ff)) != 0) ||
         (uVar8 = FUN_004abac0(), (uVar8 & 0x800) != 0)) &&
        ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) && ((*(byte *)((int)param_1 + 0xb) & 8) == 0))
    {
      __ftol();
      iVar9 = __ftol();
      __ftol();
      iVar10 = __ftol();
      iVar11 = (iVar9 - *(int *)(iVar7 + 0x1b4)) + 1;
      iVar7 = (iVar10 - *(int *)(iVar7 + 0x1b8)) + 1;
      if (iVar7 <= iVar10) {
        iVar15 = iVar7 * 0x100;
        iVar4 = iVar11;
        iStack_1c = iVar7;
        do {
          for (; iVar4 <= iVar9; iVar4 = iVar4 + 1) {
            bVar6 = FUN_004acfc0(iVar4,iStack_1c);
            if (bVar6 != 0) {
              iVar12 = __ftol();
              iVar13 = __ftol();
              *(byte *)(DAT_0052fe48 + iVar4 + iVar15) =
                   *(byte *)(iVar12 * 0x100 + iVar13 + DAT_0052fe48) & ~bVar6;
            }
            if (((iVar4 != iVar11) && (iVar4 != iVar9)) ||
               ((iStack_1c == iVar7 || (bVar3 = true, iStack_1c == iVar10)))) {
              bVar3 = false;
            }
            if (((DAT_005395cc & uVar2) != 0) && (bVar3)) {
              if ((iVar4 < 0) || (((iVar15 < 0 || (0xff < iVar4)) || (0xffff < iVar15)))) {
                uVar8 = 0;
              }
              else {
                uVar8 = (uint)*(ushort *)(DAT_0052d590 + (iVar15 + iVar4) * 2);
              }
              uVar8 = DAT_005395dc + uVar8 * 0x24;
              if ((DAT_005395dc + 0x24U <= uVar8) &&
                 (uVar8 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
                *(ushort *)(uVar8 + 0xc) = *(ushort *)(uVar8 + 0xc) & 0xfff9;
                FUN_00448c10((int)(uVar8 - DAT_005395dc) / 0x24);
              }
            }
            FUN_004abac0();
          }
          iVar15 = iVar15 + 0x100;
          iStack_1c = iStack_1c + 1;
          iVar4 = iVar11;
        } while (iStack_1c <= iVar10);
      }
    }
    if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
      FUN_004ac160();
      FUN_004acf40(((int)param_1 - DAT_005395dc) / 0x24);
      return;
    }
    bVar3 = false;
    iVar7 = 0;
    do {
      if (bVar3) break;
      puVar14 = (ushort *)
                FUN_00479d20(*(undefined4 *)((int)param_1 + 0xe),
                             *(undefined4 *)((int)param_1 + 0x12),iVar7);
      uVar1 = *puVar14;
      uVar2 = 0;
      while (uVar8 = (uint)uVar1, uVar8 != 0) {
        iVar9 = uVar8 * 0x24 + DAT_005395dc;
        if (((int)param_1 - iVar5) / 0x24 == uVar8) {
          if (uVar2 == 0) {
            *puVar14 = *(ushort *)(iVar9 + 4);
          }
          else {
            uVar1 = *(ushort *)(iVar9 + 4);
            iVar9 = uVar2 * 0x24 + DAT_005395dc;
            if ((iVar9 - DAT_005395dc) / 0x24 == (uint)uVar1) {
              FUN_00490040(s_n____getSid___0053a9c0,s__ns_o_squid_h_00538d20,0x10c);
            }
            *(ushort *)(iVar9 + 4) = uVar1;
          }
          bVar3 = true;
          break;
        }
        uVar2 = uVar8;
        uVar1 = *(ushort *)(iVar9 + 4);
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 4);
    if ((param_2 & 0x2000) != 0) {
      (**(code **)(*param_1 + 0x8c))();
      return;
    }
    (**(code **)(*param_1 + 0x88))();
  }
  return;
}


// ==== FUN_004434d0 @ 004434d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004434d0(void)

{
  undefined *puVar1;
  float fVar2;
  float fVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined **ppuVar8;
  undefined4 *unaff_FS_OFFSET;
  char local_114 [256];
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00444218;
  *unaff_FS_OFFSET = &local_10;
  uVar4 = FUN_0049c1a0(0xfffffffe,PTR_DAT_00510940,0xfffffffe,PTR_DAT_0051094c,0xfffffffd,
                       s__shapes_shp_0051d73c,0,0);
  iVar5 = FUN_004bb6d0(uVar4);
  if (iVar5 == 0) {
    FUN_00490040(s_filename_0051d75c,s__ns_o_RiftType_cpp_0051d748,0xa0);
  }
  DAT_0051cbf4 = FUN_004be710(iVar5);
  if (DAT_0051cbf4 == 0) {
    FUN_00490040(s_masterShapeDatabasePtr_0051d77c,s__ns_o_RiftType_cpp_0051d768,0xa2);
  }
  iVar5 = DAT_0051cbf0;
  iVar6 = FUN_004f1650(DAT_0051cbf0 * 0x1d4);
  local_8 = 0;
  local_14 = iVar6;
  if (iVar6 == 0) {
    iVar6 = 0;
  }
  else {
    while (iVar5 = iVar5 + -1, -1 < iVar5) {
      FUN_00444230();
    }
  }
  local_8 = 0xffffffff;
  DAT_0051c960 = iVar6;
  FUN_00446080();
  iVar5 = DAT_0051c960;
  *(undefined1 **)(DAT_0051c968 * 0x1d4 + DAT_0051c960 + 0x1b0) = &LAB_004af5c0;
  *(undefined1 **)(DAT_0051c964 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004af920;
  *(undefined1 **)(DAT_0051c978 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004afe00;
  ppuVar8 = &PTR_s_evilRaySystemProcessType_0051c890;
  *(undefined1 **)(DAT_0051c96c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004afd60;
  iVar6 = 0x1248;
  for (iVar7 = 10; iVar7 < DAT_0051ca58; iVar7 = iVar7 + 1) {
    puVar1 = *ppuVar8;
    ppuVar8 = ppuVar8 + 1;
    FUN_004f2440(iVar6 + iVar5 + 4,puVar1,0x14);
    iVar5 = DAT_0051c960;
    iVar6 = iVar6 + 0x1d4;
  }
  *(undefined1 **)(DAT_0051c9bc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f310;
  *(undefined1 **)(DAT_0051c9c0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fc60;
  *(undefined1 **)(DAT_0051c9f8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f850;
  *(undefined1 **)(DAT_0051c9fc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f680;
  *(undefined1 **)(DAT_0051ca00 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fdb0;
  *(undefined1 **)(DAT_0051ca04 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fbd0;
  *(undefined1 **)(DAT_0051ca08 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047f3a0;
  *(undefined1 **)(DAT_0051ca38 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fcf0;
  *(undefined1 **)(DAT_0051ca44 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0045f760;
  *(undefined1 **)(DAT_0051ca5c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e8b10;
  *(undefined1 **)(DAT_0051ca60 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435ff0;
  *(undefined1 **)(DAT_0051ca64 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497950;
  *(undefined1 **)(DAT_0051ca68 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004979c0;
  *(undefined1 **)(DAT_0051ca70 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca74 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca78 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00459eb0;
  *(undefined1 **)(DAT_0051ca7c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07d0;
  *(undefined1 **)(DAT_0051ca80 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07f0;
  *(undefined1 **)(DAT_0051ca8c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004491a0;
  *(undefined1 **)(DAT_0051ca90 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fd0;
  *(undefined1 **)(DAT_0051ca98 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051ca9c * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caa0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caa4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047a190;
  *(undefined1 **)(DAT_0051caac * 0x1d4 + 0x1b0 + iVar5) = &LAB_004783c0;
  *(undefined1 **)(DAT_0051cabc * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0330;
  *(undefined1 **)(DAT_0051cac0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0380;
  *(undefined1 **)(DAT_0051cad0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00415d10;
  *(undefined1 **)(DAT_0051cad4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cad8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d60;
  *(undefined1 **)(DAT_0051cae0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0048aeb0;
  *(undefined1 **)(DAT_0051cae8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047ca20;
  *(undefined1 **)(DAT_0051cb00 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7bc0;
  *(undefined1 **)(DAT_0051cb04 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416aa0;
  *(undefined1 **)(DAT_0051cb08 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004601a0;
  *(undefined1 **)(DAT_0051cb0c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004601d0;
  *(undefined1 **)(DAT_0051cb10 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460230;
  *(undefined1 **)(DAT_0051cb14 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460260;
  *(undefined1 **)(DAT_0051cb18 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460290;
  *(undefined1 **)(DAT_0051cb1c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004602c0;
  *(undefined1 **)(DAT_0051cb20 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004602f0;
  *(undefined1 **)(DAT_0051cb24 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460320;
  *(undefined1 **)(DAT_0051cb28 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00416d80;
  *(undefined1 **)(DAT_0051cb2c * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414d50;
  *(undefined1 **)(DAT_0051cb30 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fda0;
  *(undefined1 **)(DAT_0051cb34 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fd70;
  *(undefined1 **)(DAT_0051cb38 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044fdd0;
  *(undefined1 **)(DAT_0051cb3c * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047b540;
  *(undefined1 **)(DAT_0051cb44 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f40;
  *(undefined1 **)(DAT_0051cb48 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f40;
  *(undefined1 **)(DAT_0051cb50 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fa0;
  *(undefined1 **)(DAT_0051cb54 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435fd0;
  *(undefined1 **)(DAT_0051cb5c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0c80;
  *(undefined1 **)(DAT_0051cb60 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7940;
  *(undefined1 **)(DAT_0051cb64 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004783f0;
  *(undefined1 **)(DAT_0051cb68 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7920;
  *(undefined1 **)(DAT_0051cb6c * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497930;
  *(undefined1 **)(DAT_0051cb70 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004979a0;
  iVar6 = DAT_0051cb78;
  *(undefined1 **)(DAT_0051cb74 * 0x1d4 + iVar5 + 0x1b0) = &LAB_00435f60;
  *(undefined1 **)(iVar6 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7bc0;
  *(undefined1 **)(DAT_0051cb7c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c0810;
  *(undefined1 **)(DAT_0051cb80 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497980;
  *(undefined1 **)(DAT_0051cb84 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00497910;
  *(undefined1 **)(DAT_0051cb88 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00491420;
  *(undefined1 **)(DAT_0051cb8c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7be0;
  *(undefined1 **)(DAT_0051cb90 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07d0;
  *(undefined1 **)(DAT_0051cb94 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00435f80;
  *(undefined1 **)(DAT_0051cb98 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00491440;
  *(undefined1 **)(DAT_0051cb9c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004c07b0;
  *(undefined1 **)(DAT_0051cba8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414110;
  *(undefined1 **)(DAT_0051cbac * 0x1d4 + 0x1b0 + iVar5) = &LAB_00414c20;
  *(undefined1 **)(DAT_0051cbb4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004e7960;
  *(undefined1 **)(DAT_0051cbb8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004d0400;
  *(undefined1 **)(DAT_0051cbbc * 0x1d4 + 0x1b0 + iVar5) = &LAB_0040d790;
  *(undefined1 **)(DAT_0051cbc0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044bfb0;
  *(undefined1 **)(DAT_0051cbc4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047c880;
  *(undefined1 **)(DAT_0051cbc8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de60;
  *(undefined1 **)(DAT_0051cbd0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004990f0;
  *(undefined1 **)(DAT_0051cbd4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483d80;
  *(undefined1 **)(DAT_0051cbd8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483500;
  *(undefined1 **)(DAT_0051cbdc * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483560;
  *(undefined1 **)(DAT_0051cbe0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483da0;
  *(undefined1 **)(DAT_0051cbe4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00460200;
  *(undefined1 **)(DAT_0051cbe8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00483520;
  *(undefined1 **)(DAT_0051cbec * 0x1d4 + 0x1b0 + iVar5) = &LAB_00478410;
  *(undefined1 **)(DAT_0051c988 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004b36a0;
  *(undefined1 **)(DAT_0051c98c * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf190;
  *(undefined1 **)(DAT_0051c990 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf290;
  *(undefined1 **)(DAT_0051c994 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00455b90;
  *(undefined1 **)(DAT_0051c9b8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0045d3d0;
  *(code **)(DAT_0051ca28 * 0x1d4 + 0x1b0 + iVar5) = FUN_0048b950;
  *(code **)(DAT_0051ca2c * 0x1d4 + 0x1b0 + iVar5) = FUN_0048ba20;
  *(undefined1 **)(DAT_0051ca40 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004cf210;
  *(undefined1 **)(DAT_0051caa8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0044a4c0;
  *(undefined1 **)(DAT_0051caec * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047c5a0;
  *(undefined1 **)(DAT_0051caf4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de00;
  *(undefined1 **)(DAT_0051caf8 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0046de30;
  *(undefined1 **)(DAT_0051cafc * 0x1d4 + 0x1b0 + iVar5) = &LAB_00401bf0;
  *(undefined1 **)(DAT_0051cb58 * 0x1d4 + 0x1b0 + iVar5) = &LAB_0047fd90;
  *(undefined1 **)(DAT_0051cba4 * 0x1d4 + 0x1b0 + iVar5) = &LAB_00407950;
  iVar6 = DAT_0051cbf0;
  _DAT_0051c6e8 = 0.0;
  _DAT_0051c6ec = 0.0;
  *(undefined1 **)(DAT_0051cbb0 * 0x1d4 + 0x1b0 + iVar5) = &LAB_004487a0;
  for (iVar7 = 0; iVar7 < iVar6; iVar7 = iVar7 + 1) {
    fVar2 = _DAT_0051c6e8;
    fVar3 = _DAT_0051c6ec;
    if (*(int *)(iVar5 + 0x9c) == 1) {
      fVar2 = *(float *)(iVar5 + 0x44) + _DAT_0050198c;
      if (fVar2 <= _DAT_0051c6e8) {
        fVar2 = _DAT_0051c6e8;
      }
    }
    else if ((*(int *)(iVar5 + 0x9c) == 0) &&
            (fVar3 = *(float *)(iVar5 + 0x44) + _DAT_00501990, fVar3 <= _DAT_0051c6ec)) {
      fVar3 = _DAT_0051c6ec;
    }
    _DAT_0051c6ec = fVar3;
    _DAT_0051c6e8 = fVar2;
    iVar5 = iVar5 + 0x1d4;
  }
  uVar4 = __ftol();
  _sprintf(local_114,s_global_crossbowCost__d_0051d794,uVar4);
  FUN_004a99b0(local_114);
  *unaff_FS_OFFSET = local_10;
  return;
}


