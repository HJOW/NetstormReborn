// ==== FUN_004ad490 @ 004ad490

void __thiscall FUN_004ad490(int *param_1,float param_2,float param_3,uint param_4)

{
  ushort uVar1;
  uint uVar2;
  bool bVar3;
  bool bVar4;
  int iVar5;
  byte bVar6;
  undefined2 uVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  uint uVar15;
  ushort *puVar16;
  int iVar17;
  int iVar18;
  
  if ((param_1 < (int *)(DAT_005395dc + 0x24U)) ||
     ((int *)(DAT_005395f4 * 0x24 + DAT_005395dc) <= param_1)) {
    FUN_00490040();
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_00490040();
  }
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040();
  }
  if (((((int)param_2 < 1) || ((float)DAT_0052e9a8 <= param_2)) || ((int)param_3 < 1)) ||
     (bVar3 = false, (float)DAT_0052e9a8 <= param_3)) {
    bVar3 = true;
  }
  if (bVar3) {
    if (DAT_00540a24 == 0) {
      iVar8 = FUN_004abf50((double)param_2,(double)param_3);
      FUN_0048ca70(0x1d,s_Error__s_popped_to___5_1f__5_1f__0053a018,iVar8 + 4);
    }
    param_2 = 10.0;
    param_3 = 10.0;
  }
  *(float *)((int)param_1 + 0xe) = param_2;
  *(float *)((int)param_1 + 0x12) = param_3;
  uVar7 = __ftol();
  *(undefined2 *)((int)param_1 + 0x16) = uVar7;
  uVar7 = __ftol();
  *(undefined2 *)(param_1 + 6) = uVar7;
  iVar8 = FUN_004abf50();
  uVar2 = *(uint *)(iVar8 + 0xec);
  if (((uVar2 & 6) != 0) && ((*(byte *)((int)param_1 + 0x23) & 8) == 0)) {
    FUN_00442f30();
  }
  if ((((uVar2 & (DAT_005395cc | 0x400ff)) != 0) || (uVar9 = FUN_004abac0(), (uVar9 & 0x800) != 0))
     && (((*(byte *)((int)param_1 + 0x23) & 8) == 0 && ((*(byte *)((int)param_1 + 0xb) & 8) == 0))))
  {
    __ftol();
    iVar10 = __ftol();
    __ftol();
    iVar11 = __ftol();
    iVar12 = (iVar10 - *(int *)(iVar8 + 0x1b4)) + 1;
    iVar8 = (iVar11 - *(int *)(iVar8 + 0x1b8)) + 1;
    if (((((int)(float)iVar12 < 1) || ((float)DAT_0052e9a8 <= (float)iVar12)) ||
        ((int)(float)iVar8 < 1)) || ((float)DAT_0052e9a8 <= (float)iVar8)) {
      FUN_00490040();
    }
    if (iVar8 <= iVar11) {
      iVar17 = iVar8 * 0x100;
      iVar5 = iVar12;
      iVar18 = iVar8;
      do {
        for (; iVar5 <= iVar10; iVar5 = iVar5 + 1) {
          bVar6 = FUN_004acfc0(iVar5,iVar18);
          if (((iVar5 == iVar12) || (iVar10 == iVar5)) || ((iVar18 == iVar8 || (iVar18 == iVar11))))
          {
            bVar3 = false;
          }
          else {
            bVar3 = true;
          }
          if (bVar6 != 0) {
            iVar13 = __ftol();
            iVar14 = __ftol();
            *(byte *)(DAT_0052fe48 + iVar5 + iVar17) =
                 *(byte *)(iVar13 * 0x100 + DAT_0052fe48 + iVar14) | bVar6;
          }
          if (((iVar5 != iVar12) && (iVar10 != iVar5)) ||
             ((iVar18 == iVar8 || (bVar4 = true, iVar18 == iVar11)))) {
            bVar4 = false;
          }
          if (((DAT_005395cc & uVar2) != 0) && (bVar4)) {
            if ((iVar5 < 0) || (((iVar17 < 0 || (0xff < iVar5)) || (0xffff < iVar17)))) {
              uVar9 = 0;
            }
            else {
              uVar9 = (uint)*(ushort *)(DAT_0052d590 + (iVar17 + iVar5) * 2);
            }
            uVar9 = DAT_005395dc + uVar9 * 0x24;
            if (((DAT_005395dc + 0x24U <= uVar9) &&
                (uVar9 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) &&
               ((uVar15 = FUN_004abac0(), (uVar15 & 0x800) != 0 && (!bVar3)))) {
              iVar13 = FUN_004bdd80(*(float *)((int)param_1 + 0xe),
                                    *(undefined4 *)((int)param_1 + 0x12));
              if (iVar13 == 0) {
                *(ushort *)(uVar9 + 0xc) = *(ushort *)(uVar9 + 0xc) & 0xfff9;
              }
              else {
                *(ushort *)(uVar9 + 0xc) = *(ushort *)(uVar9 + 0xc) & 0xfffb | 2;
              }
              FUN_00448c10();
            }
          }
          uVar9 = FUN_004abac0();
          if ((uVar9 & 0x800) != 0) {
            iVar13 = __ftol();
            iVar14 = __ftol();
            if (((-1 < iVar13) && (-1 < iVar14)) &&
               ((iVar13 < 0x100 &&
                ((iVar14 < 0x100 && (*(short *)(DAT_005670cc + (iVar14 * 0x100 + iVar13) * 2) != 0))
                )))) {
              FUN_004abf50();
              iVar13 = FUN_004abf50();
              FUN_0048ca70(0x1d,s_ERROR__gridSid__d___s__replacing_0053a06c,
                           ((int)param_1 - DAT_005395dc) / 0x24,iVar13 + 4);
            }
          }
        }
        iVar17 = iVar17 + 0x100;
        iVar18 = iVar18 + 1;
        iVar5 = iVar12;
      } while (iVar18 <= iVar11);
    }
  }
  FUN_004acb00();
  puVar16 = (ushort *)FUN_00479d20();
  uVar1 = *puVar16;
  if (((int)param_1 - DAT_005395dc) / 0x24 == (uint)uVar1) {
    FUN_00490040();
  }
  iVar8 = DAT_005395dc;
  *(ushort *)(param_1 + 1) = uVar1;
  *puVar16 = (ushort)(((int)param_1 - iVar8) / 0x24);
  if (*(char *)((int)param_1 + 0x1f) == '\0') {
    FUN_004aca80();
  }
  if ((param_4 & 0x2000) == 0) {
    (**(code **)(*param_1 + 0x88))();
  }
  else {
    (**(code **)(*param_1 + 0x8c))();
  }
  if ((*(byte *)((int)param_1 + 0x23) & 0x80) == 0) {
    (**(code **)(*param_1 + 0x30))();
    *(byte *)((int)param_1 + 0x23) = *(byte *)((int)param_1 + 0x23) | 0x80;
  }
  FUN_004abfb0();
  if ((*(byte *)((int)param_1 + 0xb) & 8) != 0) {
    FUN_00490040();
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


// ==== FUN_004ae180 @ 004ae180

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ae180(int param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  
  if (DAT_00540a24 == 0) {
    uVar1 = FUN_004abae0();
    if ((((((DAT_005395cc | 8) & uVar1) != 0) && (DAT_005207f8 != 0)) && ((param_2 & 3) != 0)) &&
       ((param_2 & 0x2000000) == 0)) {
      FUN_0045c210((param_1 - DAT_005395dc) / 0x24);
    }
    if (DAT_005207f8 != 0) {
      if (((param_2 & 0x203) == 0) || ((param_2 & 0x2000000) != 0)) {
        uVar1 = FUN_004abac0();
        if (((uVar1 & 0x800) != 0) && (((param_2 & 1) != 0 && ((param_2 & 0x2000000) != 0)))) {
          FUN_004adbd0(DAT_005207f4);
        }
      }
      else {
        if ((param_2 & 0x800) != 0) {
          FUN_00490040(s___popFlags_pfDESTROY_GRAPH__0053a1a0,s__ns_o_Squid_cpp_0053a190,0x7dd);
        }
        uVar1 = FUN_004abac0();
        if ((uVar1 & 0x800) != 0) {
          FUN_0045b7d0((param_1 - DAT_005395dc) / 0x24);
        }
      }
    }
    uVar1 = FUN_004abae0();
    if (((param_2 & 1) != 0) && ((*(byte *)(param_1 + 0x23) & 9) == 0)) {
      FUN_004147a0(*(undefined1 *)(param_1 + 10));
      _DAT_00512114 = __ftol();
      uVar2 = FUN_004abac0();
      if ((uVar2 & 0x10000000) != 0) {
        FUN_0048a360((param_1 - DAT_005395dc) / 0x24);
      }
      if ((uVar1 & 0x4200) != 0) {
        FUN_004071f0(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24);
        FUN_0045ec90((param_1 - DAT_005395dc) / 0x24);
      }
      FUN_004d8230(*(undefined1 *)(param_1 + 0x20),(param_1 - DAT_005395dc) / 0x24,1);
      uVar1 = FUN_004015d0();
      if (uVar1 == *(byte *)(param_1 + 0x20)) {
        FUN_0040bc50(*(undefined1 *)(param_1 + 10));
      }
      FUN_0040bc20(*(undefined1 *)(param_1 + 10));
      FUN_0041eac0();
    }
  }
  FUN_004abab0(param_2);
  return;
}


// ==== FUN_004ae0f0 @ 004ae0f0

uint __fastcall FUN_004ae0f0(int param_1)

{
  byte bVar1;
  uint uVar2;
  
  uVar2 = 0;
  bVar1 = *(byte *)(param_1 + 0x23);
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


// ==== FUN_004ae380 @ 004ae380

void __fastcall FUN_004ae380(int param_1)

{
  undefined1 uVar1;
  
  uVar1 = *(undefined1 *)(param_1 + 0x22);
  FUN_004abf50();
  FUN_00456a00(*(undefined1 *)(param_1 + 10),uVar1,*(undefined4 *)(param_1 + 0xe),
               *(undefined4 *)(param_1 + 0x12),((*(byte *)(param_1 + 0x23) & 4) == 0) - 1U & 0x10);
  return;
}


// ==== FUN_004ae3e0 @ 004ae3e0

void __fastcall FUN_004ae3e0(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ae3e2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_004aca80 @ 004aca80

void __thiscall FUN_004aca80(int param_1,undefined2 param_2)

{
  uint uVar1;
  
  uVar1 = FUN_004abae0();
  if (((uVar1 & 0x1000006) != 0) && ((uVar1 & 4) == 0)) {
    *(undefined2 *)(param_1 + 8) = param_2;
  }
  return;
}


// ==== FUN_004acfc0 @ 004acfc0

uint FUN_004acfc0(int param_1,int param_2)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  uint local_c;
  int local_8;
  int local_4;
  
  local_c = FUN_004abae0();
  FUN_004ae480(&local_8,&local_4);
  iVar2 = __ftol();
  iVar3 = __ftol();
  iVar6 = (iVar3 - local_4) + 1;
  iVar5 = (iVar2 - local_8) + 1;
  if ((((param_1 == iVar5) || (param_1 == iVar2)) || (param_2 == iVar6)) ||
     (bVar1 = true, param_2 == iVar3)) {
    bVar1 = false;
  }
  uVar4 = FUN_004abae0();
  if ((((DAT_005395cc & uVar4) != 0) && (bVar1)) &&
     (((uVar4 & 0x400000) == 0 ||
      ((param_1 * 2 - iVar5 != iVar2 || (iVar3 - iVar6 < (param_2 - iVar6) * 2)))))) {
    local_c = local_c | 8;
  }
  return local_c;
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __fastcall FUN_004abac0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004acb00 @ 004acb00

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004acb00(int param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = FUN_004abf50();
  if ((*(byte *)(iVar4 + 0xec) & 6) != 0) {
    *(undefined1 *)(param_1 + 0x1f) = 0;
    return;
  }
  iVar4 = *(int *)(iVar4 + 0xdc);
  iVar3 = *(int *)(iVar4 + 8 + (uint)*(byte *)(param_1 + 0x22) * 8);
  fVar1 = *(float *)(iVar3 + -0x20 + iVar4);
  fVar2 = *(float *)(iVar3 + -0x24 + iVar4);
  if (fVar1 <= fVar2) {
    fVar1 = fVar2;
  }
  if (fVar1 <= _DAT_005058a4) {
    *(undefined1 *)(param_1 + 0x1f) = 1;
    return;
  }
  if ((int)fVar1 < 0x40800001) {
    *(undefined1 *)(param_1 + 0x1f) = 2;
    return;
  }
  *(undefined1 *)(param_1 + 0x1f) = 3;
  return;
}


// ==== FUN_004ae480 @ 004ae480

void FUN_004ae480(undefined4 param_1,undefined4 param_2)

{
  FUN_004442b0(param_1,param_2,0);
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1c4) + *(int *)(param_1 + 0x1b8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1b8);
  return;
}


// ==== FUN_00479d20 @ 00479d20

int __thiscall FUN_00479d20(int param_1,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_00490040(s_level_>_0____level_<_HASH_LEVEL__0052d5bc,s__ns_o_SquidHash_cpp_0052d5a8,0x52);
  }
  iVar1 = (&DAT_0052d598)[param_4];
  iVar2 = __ftol();
  iVar2 = (iVar2 / iVar1) * DAT_0052e9a8;
  iVar3 = __ftol();
  return (iVar2 / iVar1 + iVar3 / iVar1) * 2 + *(int *)(param_1 + 0xc + param_4 * 4);
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_004abab0 @ 004abab0

void FUN_004abab0(void)

{
  DAT_00539600 = DAT_00539600 + -1;
  return;
}


// ==== FUN_00456a00 @ 00456a00

void FUN_00456a00(int param_1,int param_2,undefined4 param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  int *piVar2;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined1 local_10 [16];
  
  if (DAT_0052039c == 0) {
    FUN_00456930(&local_20,param_1,param_2,param_3,param_4);
    iVar1 = DAT_00583e28;
    if (local_20 <= DAT_00583e28) {
      iVar1 = local_20;
    }
    local_20 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_20 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_1c <= DAT_00583e2c) {
      iVar1 = local_1c;
    }
    local_1c = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_1c = DAT_00583e24;
    }
    iVar1 = DAT_00583e28;
    if (local_18 <= DAT_00583e28) {
      iVar1 = local_18;
    }
    local_18 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_18 = DAT_00583e20;
    }
    iVar1 = DAT_00583e2c;
    if (local_14 <= DAT_00583e2c) {
      iVar1 = local_14;
    }
    local_14 = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_14 = DAT_00583e24;
    }
    if ((param_5 & 0x10) != 0) {
      if (local_18 - local_20 < 0x13) {
        local_20 = local_20 + -9;
        local_18 = local_18 + 9;
      }
      local_1c = local_1c + -9;
      local_20 = local_20 + -3;
      local_18 = local_18 + 3;
    }
    if ((local_18 != local_20 && -1 < local_18 - local_20) &&
       (local_14 != local_1c && -1 < local_14 - local_1c)) {
      FUN_00457320(&local_20,param_5);
    }
    iVar1 = DAT_0051c960 + param_1 * 0x1d4;
    if ((*(byte *)(iVar1 + 0xea) & 4) != 0) {
      piVar2 = (int *)FUN_00456930(local_10,param_1,*(int *)(iVar1 + 0x114) + param_2,param_3,
                                   param_4);
      local_20 = DAT_00583e28;
      if (*piVar2 <= DAT_00583e28) {
        local_20 = *piVar2;
      }
      if (local_20 <= DAT_00583e20) {
        local_20 = DAT_00583e20;
      }
      local_1c = DAT_00583e2c;
      if (piVar2[1] <= DAT_00583e2c) {
        local_1c = piVar2[1];
      }
      if (local_1c <= DAT_00583e24) {
        local_1c = DAT_00583e24;
      }
      local_18 = DAT_00583e28;
      if (piVar2[2] <= DAT_00583e28) {
        local_18 = piVar2[2];
      }
      if (local_18 <= DAT_00583e20) {
        local_18 = DAT_00583e20;
      }
      local_14 = DAT_00583e2c;
      if (piVar2[3] <= DAT_00583e2c) {
        local_14 = piVar2[3];
      }
      if (local_14 <= DAT_00583e24) {
        local_14 = DAT_00583e24;
      }
      if ((local_18 != local_20 && -1 < local_18 - local_20) &&
         (local_14 != local_1c && -1 < local_14 - local_1c)) {
        FUN_00457320(&local_20,param_5);
      }
    }
  }
  return;
}


