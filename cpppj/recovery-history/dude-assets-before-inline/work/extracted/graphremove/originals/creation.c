// ==== FUN_0040da00 @ 0040da00

undefined2 FUN_0040da00(int param_1,int param_2)

{
  if ((((-1 < param_1) && (-1 < param_2)) && (param_1 < 0x100)) && (param_2 < 0x100)) {
    return *(undefined2 *)(DAT_005c84bc + (param_2 * 0x100 + param_1) * 2);
  }
  return 0;
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


// ==== FUN_0040e880 @ 0040e880

void __thiscall FUN_0040e880(float *param_1,float param_2,float param_3)

{
  int iVar1;
  
  FUN_0041d0d0();
  iVar1 = FUN_0040e800();
  if (iVar1 == 0) {
    *param_1 = param_2;
    param_1[1] = param_3;
    return;
  }
  if (*param_1 < param_2) {
    param_2 = *param_1;
  }
  *param_1 = param_2;
  if (param_3 <= param_1[1]) {
    param_1[1] = param_3;
    return;
  }
  param_1[1] = param_1[1];
  return;
}


// ==== FUN_0040e8f0 @ 0040e8f0

void __thiscall FUN_0040e8f0(int param_1,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  
  FUN_0041d0d0();
  pfVar1 = (float *)(param_1 + 8);
  iVar2 = FUN_0040e800();
  if (iVar2 == 0) {
    *pfVar1 = param_2;
    *(float *)(param_1 + 0xc) = param_3;
    return;
  }
  if (param_2 < *pfVar1) {
    param_2 = *pfVar1;
  }
  *pfVar1 = param_2;
  if (*(float *)(param_1 + 0xc) <= param_3) {
    *(float *)(param_1 + 0xc) = param_3;
    return;
  }
  *(undefined4 *)(param_1 + 0xc) = *(undefined4 *)(param_1 + 0xc);
  return;
}


// ==== FUN_0040ea00 @ 0040ea00

void __thiscall FUN_0040ea00(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  iVar3 = 0;
  if (0 < param_1[2]) {
    do {
      iVar1 = *(int *)(*param_1 + iVar3 * 4);
      if (iVar1 == param_2) {
        iVar2 = iVar2 + 1;
      }
      else {
        *(int *)(*param_1 + (iVar3 - iVar2) * 4) = iVar1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < param_1[2]);
  }
  param_1[2] = param_1[2] - iVar2;
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


// ==== FUN_004166e0 @ 004166e0

void FUN_004166e0(int param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  
  if ((((DAT_00594fc8 == 0) && (0 < param_1)) && (iVar1 = param_1 * 0xb4 + DAT_0059534c, iVar1 != 0)
      ) && (*(int *)(iVar1 + 0xb0) != 0)) {
    FUN_00414de0(param_2,param_3);
  }
  return;
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


// ==== FUN_0041ce90 @ 0041ce90

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0041ce90(float *param_1,float param_2,float param_3,int param_4)

{
  param_2 = param_2 - *param_1;
  param_3 = param_3 - param_1[1];
  if (param_4 == 0) {
    if (param_2 <= _DAT_00500fc4) {
      if (param_2 < _DAT_00500fc4) {
        if (param_3 < _DAT_00500fc4) {
          return 7;
        }
        if (param_3 <= _DAT_00500fc4) {
          return 6;
        }
        return 5;
      }
      if (param_3 < _DAT_00500fc4) {
        return 0;
      }
      if (_DAT_00500fc4 < param_3) {
        return 4;
      }
      FUN_004e0620("false","\\Ns\\O\\Boardcoord.cpp",0xfd);
      return 0xffffffff;
    }
    if (param_3 < _DAT_00500fc4) {
      return 1;
    }
    if (_DAT_00500fc4 < param_3) {
      return 3;
    }
  }
  else {
    if (ABS(param_2) <= ABS(param_3)) {
      if (_DAT_00500fc4 < param_3) {
        return 4;
      }
      return 0;
    }
    if (param_2 <= _DAT_00500fc4) {
      return 6;
    }
  }
  return 2;
}


// ==== FUN_0041d0d0 @ 0041d0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_0041d0d0(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = _DAT_005012d0;
  if (_DAT_005012d0 <= *param_1) {
    fVar1 = *param_1;
  }
  *param_1 = fVar1;
  fVar2 = _DAT_005012d0;
  if (_DAT_005012d0 <= param_1[1]) {
    fVar2 = param_1[1];
  }
  param_1[1] = fVar2;
  if (_DAT_005022e8 < fVar1) {
    fVar1 = _DAT_005022e8;
  }
  *param_1 = fVar1;
  if (fVar2 <= _DAT_005022e8) {
    param_1[1] = fVar2;
    return;
  }
  param_1[1] = 255.0;
  return;
}


// ==== FUN_0041d770 @ 0041d770

void __fastcall FUN_0041d770(float *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *param_1 = (float)iVar1;
  iVar1 = FUN_004e49c0();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_0041d7a0 @ 0041d7a0

void __fastcall FUN_0041d7a0(float *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *param_1 = (float)iVar1;
  iVar1 = FUN_004e49c0();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_0041def0 @ 0041def0

undefined4 * __thiscall FUN_0041def0(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint in_stack_00000014;
  
  param_1[0xe] = in_stack_00000014 & 1;
  *param_1 = &PTR_LAB_00502318;
  param_1[0xf] = in_stack_00000014 & 4;
  FUN_0041d770();
  FUN_0041d7a0();
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0(uVar1);
  uVar3 = FUN_004e49c0(uVar2);
  uVar4 = FUN_004e49c0(uVar3);
  FUN_004b16d0(uVar4,uVar3,uVar2,uVar1);
  return param_1;
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


// ==== FUN_004214a0 @ 004214a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004214a0(int param_1)

{
  int iVar1;
  
  iVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((((*(byte *)(iVar1 + 10) == _DAT_005411a0) && ((*(byte *)(iVar1 + 0x28) & 8) == 0)) &&
      (DAT_005e4794 != 0)) && (7 < (*(byte *)(iVar1 + 0xc) >> 3 & 0xf))) {
    FUN_004e0620("num >= 0 && num <= 7","\\Ns\\O\\Bridge.cpp",0xff);
  }
  return;
}


// ==== FUN_00425df0 @ 00425df0

uint FUN_00425df0(void)

{
  uint uVar1;
  
  uVar1 = FUN_004ac1e0();
  return uVar1 & 0x800;
}


// ==== FUN_00434380 @ 00434380

void FUN_00434380(int param_1,int param_2,int *param_3,int *param_4,int param_5)

{
  int iVar1;
  
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_1 / iVar1) {
    param_1 = DAT_00531928 / iVar1 + -1;
  }
  else {
    param_1 = param_1 / (int)(&DAT_005424f8)[param_5];
  }
  *param_3 = param_1;
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_2 / iVar1) {
    *param_4 = DAT_00531928 / iVar1 + -1;
    return;
  }
  *param_4 = param_2 / (int)(&DAT_005424f8)[param_5];
  return;
}


// ==== FUN_00441e40 @ 00441e40

undefined4 FUN_00441e40(int param_1,int param_2,int param_3,int param_4,uint param_5)

{
  char cVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  
  uVar4 = param_5 & 0x80000001;
  bVar7 = uVar4 == 0;
  if ((int)uVar4 < 0) {
    bVar7 = (uVar4 - 1 | 0xfffffffe) == 0xffffffff;
  }
  if (bVar7) {
    cVar1 = *(char *)(*(int *)(param_1 + 0x124) + param_2 * 4);
    cVar2 = *(char *)(*(int *)(param_3 + 0x124) + param_4 * 4);
  }
  else {
    cVar1 = *(char *)(*(int *)(param_1 + 0x124) + 1 + param_2 * 4);
    cVar2 = *(char *)(*(int *)(param_3 + 0x124) + 1 + param_4 * 4);
  }
  iVar6 = (int)cVar2;
  iVar5 = (int)cVar1;
  uVar4 = *(uint *)(param_1 + 0xec);
  if ((uVar4 & 0x40000) != 0) {
    iVar5 = 0x50;
  }
  uVar3 = *(uint *)(param_3 + 0xec);
  if ((uVar3 & 0x40000) != 0) {
    iVar6 = 0x50;
  }
  if (((uVar4 & 0x104) == 0) || ((uVar3 & 0x1044202) == 0)) {
    if (((uVar3 & 0x104) != 0) && ((uVar4 & 0x1044202) != 0)) {
      iVar5 = 0x41;
    }
  }
  else {
    iVar6 = 0x41;
  }
  if ((((&DAT_0052f8dc)[param_5] & (uint)*(ushort *)(iVar5 * 2 + 0x532502)) != 0) &&
     (((&DAT_0052f8dc)[*(int *)(&DAT_0052f8bc + param_5 * 4)] &
      (uint)*(ushort *)(iVar6 * 2 + 0x532502)) != 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_00445290 @ 00445290

void __thiscall FUN_00445290(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *param_1;
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  iVar2 = param_2;
  if ((param_2 <= iVar3) && (iVar2 = iVar1, param_4 <= iVar1)) {
    iVar2 = param_4;
  }
  *param_1 = iVar2;
  iVar1 = param_1[1];
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  iVar2 = param_3;
  if ((param_3 <= iVar3) && (iVar2 = iVar1, param_5 <= iVar1)) {
    iVar2 = param_5;
  }
  param_1[1] = iVar2;
  iVar1 = param_1[2];
  iVar3 = iVar1;
  if (param_4 <= iVar1) {
    iVar3 = param_4;
  }
  if ((param_2 <= iVar3) && (param_2 = iVar1, param_4 <= iVar1)) {
    param_2 = param_4;
  }
  param_1[2] = param_2;
  iVar1 = param_1[3];
  iVar3 = iVar1;
  if (param_5 <= iVar1) {
    iVar3 = param_5;
  }
  if (param_3 <= iVar3) {
    if (param_5 <= iVar1) {
      param_1[3] = param_5;
      return;
    }
    param_1[3] = iVar1;
    return;
  }
  param_1[3] = param_3;
  return;
}


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
}


// ==== FUN_0044f770 @ 0044f770

void FUN_0044f770(undefined4 param_1)

{
  if (DAT_0055a4dc < DAT_0055a4d8) {
    *(undefined4 *)(DAT_0055a4d4 + DAT_0055a4dc * 4) = param_1;
    DAT_0055a4dc = DAT_0055a4dc + 1;
  }
  DAT_0055a4a4 = DAT_0055a4a4 + 1;
  return;
}


// ==== FUN_00460cd0 @ 00460cd0

/* WARNING: Removing unreachable block (ram,0x00460d37) */
/* WARNING: Removing unreachable block (ram,0x00460d4f) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00460cd0(double *param_1)

{
  int iVar1;
  DWORD DVar2;
  uint uVar3;
  
  iVar1 = DAT_0055b4ac;
  if (DAT_0055b4b0 != 0) {
    *(undefined4 *)param_1 = DAT_0055b4f0;
    *(undefined4 *)((int)param_1 + 4) = DAT_0055b4f4;
    return;
  }
  DVar2 = timeGetTime();
  uVar3 = ((DVar2 - _DAT_0055b4a0) - iVar1) + DAT_0055b4a4 * 1000;
  *param_1 = ((double)(uVar3 / 1000) + (double)(uVar3 % 1000) * _DAT_0050a668) - _DAT_0055b4f8;
  return;
}


// ==== FUN_00462a50 @ 00462a50

void __fastcall FUN_00462a50(short *param_1)

{
  short sVar1;
  
  if (DAT_005e4794 != 0) {
    if (param_1[1] == 0) {
      FUN_004e0620("inUse","\\Ns\\O\\Graph.cpp",0x59);
    }
    if ((DAT_005e4794 != 0) && (*param_1 < 1)) {
      FUN_004e0620("numSurfaces > 0","\\Ns\\O\\Graph.cpp",0x5a);
    }
  }
  if ((param_1[1] != 0) && (0 < *param_1)) {
    sVar1 = *param_1 + -1;
    *param_1 = sVar1;
    if (sVar1 == 0) {
      *param_1 = 0;
      param_1[1] = 0;
    }
  }
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


// ==== FUN_00462bc0 @ 00462bc0

undefined4 FUN_00462bc0(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  if (DAT_005e4794 == 0) {
    if (param_1 == 0) {
      return 0;
    }
  }
  else if (param_1 == 0) {
    FUN_004e0620("mySid","\\Ns\\O\\Graph.cpp",0x93);
    return 0;
  }
  iVar1 = param_1 * 0x32 + DAT_005c8464;
  uVar2 = FUN_004ac1e0();
  if ((uVar2 & 0x800) != 0) {
    uVar3 = FUN_004ad390();
    return uVar3;
  }
  uVar3 = FUN_00462b80(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12));
  return uVar3;
}


// ==== FUN_00462c30 @ 00462c30

int FUN_00462c30(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  if (DAT_00540414 != 0) {
    iVar1 = FUN_0040eaf0(param_1,param_2);
    if (iVar1 != 0) {
      iVar1 = FUN_004ad390();
      if (iVar1 != 0xfe) {
        return DAT_00562c18 + iVar1 * 6;
      }
    }
  }
  return 0;
}


// ==== FUN_00462c80 @ 00462c80

int FUN_00462c80(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_00462bc0(param_1);
  if (iVar1 == 0xfe) {
    return 0;
  }
  return DAT_00562c18 + iVar1 * 6;
}


// ==== FUN_00462d40 @ 00462d40

void FUN_00462d40(void)

{
  undefined4 *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 local_50;
  undefined4 local_4c;
  int local_c;
  
  puVar1 = (undefined4 *)FUN_004ade70(&local_50,0);
  FUN_0041def0(*puVar1,puVar1[1],puVar1[2],puVar1[3],0);
  while (local_c != 0) {
    uVar2 = FUN_004ac1e0();
    if ((uVar2 & 0x800) != 0) {
      local_50 = *(undefined4 *)(local_c * 0x32 + 0xe + DAT_005c8464);
      local_4c = *(undefined4 *)(local_c * 0x32 + DAT_005c8464 + 0x12);
      iVar3 = FUN_004e49c0();
      iVar4 = FUN_004e49c0();
      if ((*(byte *)(iVar4 + iVar3 * 0x100 + DAT_005c7c44) & 8) != 0) {
        iVar3 = FUN_004ad390();
        if (iVar3 != DAT_00540410) {
          FUN_00462a50();
        }
        FUN_004ad3e0(DAT_00540410);
      }
    }
    FUN_004b1810();
  }
  return;
}


// ==== FUN_00462e50 @ 00462e50

void FUN_00462e50(int param_1)

{
  if (param_1 != 0xfe) {
    if ((param_1 < 0) || (0xfa < param_1)) {
      FUN_004e0620("graphNum >=0 && graphNum < MAX_GRAPHS","\\Ns\\O\\Graph.cpp",0x26f);
    }
    FUN_00462ac0();
    *(undefined2 *)(param_1 * 6 + 2 + DAT_00562c18) = 0;
    *(undefined2 *)(param_1 * 6 + DAT_00562c18) = 0;
  }
  return;
}


// ==== FUN_00462f10 @ 00462f10

int FUN_00462f10(undefined4 param_1,int param_2)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  short *psVar4;
  uint uVar5;
  undefined4 *puVar6;
  int iVar7;
  undefined4 *puVar8;
  int local_104;
  int local_cc;
  
  FUN_00462ac0();
  puVar6 = DAT_00562c20;
  if (DAT_00562c20 == (undefined4 *)0x0) {
    DAT_00562c20 = operator_new(0x4000);
    FUN_004437f0();
    puVar6 = DAT_00562c20;
    puVar8 = DAT_00562c20;
    for (iVar3 = 0x1000; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
  }
  *puVar6 = param_1;
  local_104 = 0;
  iVar3 = 1;
  do {
    iVar7 = puVar6[iVar3 + -1];
    iVar3 = iVar3 + -1;
    iVar1 = FUN_004ad390();
    if (iVar1 != param_2) {
      psVar4 = (short *)0x0;
      if (iVar1 != 0xfe) {
        psVar4 = (short *)(DAT_00562c18 + iVar1 * 6);
      }
      psVar2 = (short *)0x0;
      if (param_2 != 0xfe) {
        psVar2 = (short *)(DAT_00562c18 + param_2 * 6);
      }
      local_104 = local_104 + 1;
      if (psVar4 != (short *)0x0) {
        *psVar4 = *psVar4 + -1;
      }
      if (psVar2 != (short *)0x0) {
        *psVar2 = *psVar2 + 1;
      }
      FUN_004ad3e0(param_2);
      FUN_004b23e0(iVar7,8);
      while ((uVar5 = local_cc * 0x32 + DAT_005c8464, DAT_005c8464 + 0x32U <= uVar5 &&
             (uVar5 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))) {
        iVar1 = FUN_004ad390();
        if (iVar1 != param_2) {
          if ((DAT_005e4794 != 0) && ((local_cc * 0x32) / 0x32 == iVar7)) {
            FUN_004e0620("cFinder.s().getSid() != sid","\\Ns\\O\\Graph.cpp",0x176);
          }
          DAT_00562c20[iVar3] = (local_cc * 0x32) / 0x32;
          iVar3 = iVar3 + 1;
          if (0xfff < iVar3) {
            FUN_004e0620("d < FLOOD_STACK_SIZE","\\Ns\\O\\Graph.cpp",0x17a);
          }
        }
        FUN_004b1e70();
      }
    }
    puVar6 = DAT_00562c20;
  } while (0 < iVar3);
  iVar7 = 0;
  psVar4 = (short *)(DAT_00562c18 + 2);
  iVar3 = 0xfb;
  do {
    if (*psVar4 != 0) {
      iVar7 = iVar7 + 1;
    }
    psVar4 = psVar4 + 3;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  FUN_004c2630(0x20,"%d graphs in use\n",iVar7);
  FUN_00462ac0();
  return local_104;
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


// ==== FUN_004633c0 @ 004633c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004633c0(int param_1)

{
  uint uVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short *psVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  int local_210;
  int local_20c;
  int local_208;
  int local_1cc;
  int local_100 [64];
  
  FUN_00462ac0();
  fVar2 = *(float *)(param_1 * 0x32 + 0xe + DAT_005c8464);
  iVar10 = param_1 * 0x32 + DAT_005c8464;
  fVar3 = *(float *)(iVar10 + 0x12);
  if ((DAT_005e4794 != 0) && (iVar4 = FUN_0040e800(), iVar4 == 0)) {
    FUN_004e0620("pos.isValid()","\\Ns\\O\\Graph.cpp",0xbd);
  }
  if (fVar2 <= _DAT_00500fc4) {
    return;
  }
  if ((float)DAT_00531928 <= fVar2) {
    return;
  }
  if (fVar3 <= _DAT_00500fc4) {
    return;
  }
  if ((float)DAT_00531928 <= fVar3) {
    return;
  }
  if (DAT_005e4794 != 0) {
    if ((*(byte *)(iVar10 + 0xb) & 7) == 0) goto LAB_004634ab;
    FUN_004e0620("!s.isVoid() && !s.isDead() && !s.isFree()","\\Ns\\O\\Graph.cpp",0xc2);
  }
  if ((*(byte *)(iVar10 + 0xb) & 7) != 0) {
    return;
  }
LAB_004634ab:
  piVar11 = local_100;
  for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar11 = 0;
    piVar11 = piVar11 + 1;
  }
  iVar9 = 0;
  iVar4 = 0x7fffffff;
  local_208 = -0x7fffffff;
  FUN_004b23e0(param_1,8);
  do {
    uVar1 = local_1cc * 0x32 + DAT_005c8464;
    if ((uVar1 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar1)) {
      if (iVar9 == 0) {
        iVar4 = FUN_00463330();
        if (*(short *)(iVar4 + 2) != 1) {
          FUN_004e0620("g->inUse == true","\\Ns\\O\\Graph.cpp",0xf3);
        }
        FUN_00462f10((iVar10 - DAT_005c8464) / 0x32,(iVar4 - DAT_00562c18) / 6);
      }
      else {
        if ((iVar4 < 0) || (0xfa < iVar4)) {
          FUN_004e0620("biggestConnection >= 0 && biggestConnection < MAX_GRAPHS",
                       "\\Ns\\O\\Graph.cpp",0xfb);
        }
        psVar8 = (short *)(DAT_00562c18 + iVar4 * 6);
        if ((DAT_005e4794 != 0) && (psVar8[1] != 1)) {
          FUN_004e0620("g->inUse == true","\\Ns\\O\\Graph.cpp",0x100);
        }
        if (psVar8[1] == 0) {
          psVar8[1] = 1;
        }
        FUN_004ad3e0(iVar4);
        *psVar8 = *psVar8 + 1;
        if (1 < iVar9) {
          local_210 = 0;
          local_20c = 0;
          do {
            iVar10 = local_100[local_20c];
            if ((iVar10 != 0) && (iVar5 = FUN_004ad390(), iVar5 != iVar4)) {
              psVar8 = (short *)FUN_00462c80(iVar10);
              if ((DAT_005e4794 != 0) && (local_208 < *psVar8)) {
                FUN_004e0620("loser->numSurfaces <= biggestSize","\\Ns\\O\\Graph.cpp",0x113);
              }
              FUN_00462f10(iVar10,iVar4);
              local_210 = local_210 + 1;
              *psVar8 = 0;
              psVar8[1] = 0;
            }
            local_20c = local_20c + 1;
          } while (local_20c < 0x40);
          if ((DAT_005e4794 != 0) && (local_210 != iVar9 + -1)) {
            FUN_004e0620("numFlooded == numConnections - 1","\\Ns\\O\\Graph.cpp",0x11f);
          }
        }
      }
      iVar4 = 0;
      psVar8 = (short *)(DAT_00562c18 + 2);
      iVar10 = 0xfb;
      do {
        if (*psVar8 != 0) {
          iVar4 = iVar4 + 1;
        }
        psVar8 = psVar8 + 3;
        iVar10 = iVar10 + -1;
      } while (iVar10 != 0);
      FUN_004c2630(0x20,"%d graphs in use\n",iVar4);
      return;
    }
    iVar5 = FUN_004ad390();
    if (iVar5 != DAT_00540410) {
      iVar5 = 0;
      if (0 < iVar9) {
        do {
          iVar6 = FUN_004ad390();
          iVar7 = FUN_004ad390();
          if (iVar7 == iVar6) goto LAB_004635c2;
          iVar5 = iVar5 + 1;
        } while (iVar5 < iVar9);
      }
      local_100[iVar9] = (local_1cc * 0x32) / 0x32;
      iVar9 = iVar9 + 1;
      iVar5 = FUN_004ad390();
      iVar6 = (int)*(short *)(DAT_00562c18 + iVar5 * 6);
      if (local_208 < iVar6) {
        iVar4 = (iVar5 * 6) / 6;
        local_208 = iVar6;
      }
    }
LAB_004635c2:
    FUN_004b1e70();
  } while( true );
}


// ==== FUN_004637b0 @ 004637b0

void FUN_004637b0(char *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 char *param_5)

{
  uint uVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int iVar9;
  int *piVar10;
  undefined4 *puVar11;
  bool bVar12;
  undefined4 auStack_24c [5];
  undefined4 uStack_238;
  char *pcStack_234;
  char *pcStack_230;
  undefined1 *puStack_22c;
  undefined1 local_210 [8];
  undefined1 local_208 [8];
  int local_200 [77];
  int local_cc;
  
  if (DAT_00540414 != 0) {
    puStack_22c = (undefined1 *)0x4637cc;
    FUN_00462ac0();
    puStack_22c = (undefined1 *)param_2;
    pcStack_230 = param_1;
    pcStack_234 = (char *)0x4637e1;
    psVar2 = (short *)FUN_00462c30();
    pcStack_234 = param_5;
    uStack_238 = 0x4637f4;
    iVar3 = FUN_0049a840();
    if ((psVar2 != (short *)0x0) && (psVar2[1] != 0)) {
      puStack_22c = (undefined1 *)0x0;
      piVar10 = local_200;
      for (iVar6 = 0x40; iVar6 != 0; iVar6 = iVar6 + -1) {
        *piVar10 = 0;
        piVar10 = piVar10 + 1;
      }
      puVar11 = auStack_24c;
      puVar8 = (undefined4 *)register0x00000010;
      for (iVar6 = 8; puVar8 = puVar8 + 1, iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar11 = *puVar8;
        puVar11 = puVar11 + 1;
      }
      iVar6 = 0;
      FUN_004b24c0();
      while( true ) {
        uVar1 = local_cc * 0x32 + DAT_005c8464;
        if ((uVar1 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar1))
        break;
        local_200[iVar6] = (local_cc * 0x32) / 0x32;
        iVar6 = iVar6 + 1;
        if (0x3f < iVar6) {
          puStack_22c = (undefined1 *)0x1ef;
          pcStack_230 = "\\Ns\\O\\Graph.cpp";
          pcStack_234 = "numConnections < MAX_CONNECTIONS";
          uStack_238 = 0x463897;
          FUN_004e0620();
        }
        puStack_22c = (undefined1 *)0x4638a6;
        FUN_004b1e70();
      }
      puStack_22c = local_210;
      pcStack_230 = (char *)0x4638b2;
      FUN_00460cd0();
      bVar12 = DAT_00562c1c == 0;
      iVar7 = iVar6;
      if (iVar6 != 0) {
        do {
          iVar9 = 0;
          iVar4 = 0;
          if (0 < iVar6) {
            do {
              if (local_200[iVar4] != 0) {
                iVar9 = local_200[iVar4];
                local_200[iVar4] = 0;
                if (iVar9 != 0) goto LAB_00463910;
                break;
              }
              iVar4 = iVar4 + 1;
            } while (iVar4 < iVar6);
          }
          puStack_22c = (undefined1 *)0x20b;
          pcStack_230 = "\\Ns\\O\\Graph.cpp";
          pcStack_234 = "floodSid != 0";
          uStack_238 = 0x46390d;
          FUN_004e0620();
LAB_00463910:
          pcStack_230 = (char *)0x463916;
          puStack_22c = (undefined1 *)iVar9;
          iVar4 = FUN_00462bc0();
          if (iVar4 == 0xfe) {
            psVar5 = (short *)0x0;
          }
          else {
            psVar5 = (short *)(DAT_00562c18 + iVar4 * 6);
          }
          if (psVar5 == psVar2) {
            if ((DAT_00562c1c == 0) && (iVar7 < 2)) {
              if ((*(byte *)(iVar3 + 0xec) & 8) == 0) {
                bVar12 = true;
              }
            }
            else {
              puStack_22c = (undefined1 *)0x46395c;
              iVar4 = FUN_00463330();
              puStack_22c = (undefined1 *)((iVar4 - DAT_00562c18) / 6);
              pcStack_234 = (char *)0x463979;
              pcStack_230 = (char *)iVar9;
              FUN_00462f10();
            }
          }
          iVar7 = iVar7 + -1;
        } while (iVar7 != 0);
        if (param_5 == DAT_005412e0) {
          *psVar2 = *psVar2 + -9;
        }
        else {
          *psVar2 = *psVar2 + -1;
        }
      }
      if (!bVar12) {
        *psVar2 = 0;
        psVar2[1] = 0;
      }
      puStack_22c = local_208;
      pcStack_230 = (char *)0x4639b9;
      FUN_00460cd0();
      puStack_22c = (undefined1 *)0x4639c1;
      FUN_00462ac0();
    }
  }
  return;
}


// ==== FUN_00472f70 @ 00472f70

void FUN_00472f70(int param_1)

{
  bool bVar1;
  int iVar2;
  
  iVar2 = 0;
  if (0 < DAT_00592f8c) {
    do {
      if (*(int *)(DAT_00592f84 + iVar2 * 4) == param_1) {
        bVar1 = true;
        goto LAB_00472f94;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00592f8c);
  }
  bVar1 = false;
LAB_00472f94:
  if (DAT_005e4794 == 0) {
LAB_00472fc1:
    if (bVar1) goto LAB_00472fd6;
  }
  else if (bVar1) {
    FUN_004e0620("!alreadyThere","\\Ns\\O\\Mana.cpp",0x44);
    goto LAB_00472fc1;
  }
  if (DAT_00592f8c < DAT_00592f88) {
    *(int *)(DAT_00592f84 + DAT_00592f8c * 4) = param_1;
    DAT_00592f8c = DAT_00592f8c + 1;
  }
LAB_00472fd6:
  DAT_0055a4a4 = DAT_0055a4a4 + 1;
  return;
}


// ==== FUN_00481510 @ 00481510

void FUN_00481510(void)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  undefined4 local_4;
  
  if ((DAT_005949d0 != 0) && ((*(byte *)(DAT_005949d0 + 4) & 1) != 0)) {
    iVar1 = FUN_004e49c0();
    iVar3 = (int)(iVar1 + (iVar1 >> 0x1f & 3U)) >> 2;
    iVar1 = FUN_004e49c0();
    iVar1 = (int)(iVar1 + (iVar1 >> 0x1f & 3U)) >> 2;
    iVar4 = 0;
    piVar2 = &DAT_005949e0;
    if (0 < DAT_005949c0) {
      do {
        if ((*piVar2 == iVar3) && (piVar2[1] == iVar1)) {
          return;
        }
        iVar4 = iVar4 + 1;
        piVar2 = piVar2 + 3;
      } while (iVar4 < DAT_005949c0);
    }
    if (99 < DAT_005949c0) {
      DAT_005949d4 = 1;
      return;
    }
    (&DAT_005949e0)[DAT_005949c0 * 3] = iVar3;
    (&DAT_005949e4)[DAT_005949c0 * 3] = iVar1;
    (&DAT_005949e8)[DAT_005949c0 * 3] = local_4;
    DAT_005949c0 = DAT_005949c0 + 1;
  }
  return;
}


// ==== FUN_004834c0 @ 004834c0

void __thiscall FUN_004834c0(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  
  iVar1 = *param_1;
  if (param_2 <= *param_1) {
    iVar1 = param_2;
  }
  *param_1 = iVar1;
  iVar1 = param_1[1];
  if (param_3 <= param_1[1]) {
    iVar1 = param_3;
  }
  param_1[1] = iVar1;
  iVar1 = param_1[2];
  if (param_1[2] <= param_4) {
    iVar1 = param_4;
  }
  param_1[2] = iVar1;
  if (param_5 < param_1[3]) {
    param_1[3] = param_1[3];
    return;
  }
  param_1[3] = param_5;
  return;
}


// ==== FUN_00490050 @ 00490050

void FUN_00490050(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  if (param_1 != 0) {
    piVar1 = (int *)(param_1 * 0xb4 + DAT_0059534c);
    if (piVar1 == (int *)0x0) {
      FUN_004e0620(&DAT_005078ac,"\\Ns\\O\\Player.cpp",0x313);
      return;
    }
    if (piVar1[2] < piVar1[1]) {
      *(undefined4 *)(*piVar1 + piVar1[2] * 4) = param_2;
      piVar1[2] = piVar1[2] + 1;
    }
  }
  return;
}


// ==== FUN_004952f0 @ 004952f0

void FUN_004952f0(int param_1,int param_2)

{
  int *_Src;
  
  if ((param_1 < 5) || (_Src = &DAT_0059a698, 14999 < param_1)) {
    _Src = &DAT_0059a738;
  }
  _memmove(_Src + 2,_Src,0x98);
  *_Src = param_1;
  _Src[1] = param_2;
  return;
}


// ==== FUN_004970a0 @ 004970a0

void __thiscall
FUN_004970a0(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5)

{
  *param_1 = param_2;
  param_1[1] = param_3;
  param_1[2] = param_4;
  param_1[3] = param_5;
  return;
}


// ==== FUN_00497580 @ 00497580

void __thiscall FUN_00497580(int param_1,int *param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  uint *local_20;
  int local_14;
  
  if ((((*param_2 < DAT_005c78f0) && (param_2[1] < DAT_005c78f4)) && (0 < param_2[2])) &&
     (0 < param_2[3])) {
    if (99 < *(int *)(param_1 + 100)) {
      DAT_0059a8b0 = 1;
      return;
    }
    iVar6 = (param_2[3] - param_2[1]) * (param_2[2] - *param_2);
    local_14 = 0;
    if (0 < *(int *)(param_1 + 100)) {
      local_20 = (uint *)(param_1 + 0x6a8);
      piVar9 = (int *)(param_1 + 0x68);
      do {
        if (DAT_0059a8d0 != 0) break;
        uVar1 = *local_20;
        if ((uVar1 & 1) == 0) {
          iVar8 = (piVar9[2] - *piVar9) * (piVar9[3] - piVar9[1]);
          iVar2 = *piVar9;
          iVar3 = piVar9[1];
          iVar4 = piVar9[2];
          iVar5 = piVar9[3];
          FUN_004834c0(*param_2,param_2[1],param_2[2],param_2[3]);
          iVar7 = (iVar5 - iVar3) * (iVar4 - iVar2);
          if (((param_3 & 4) == 0) || (iVar7 - iVar6 != 0)) {
            if (((uVar1 & 4) != 0) && (iVar7 - iVar8 == 0)) {
              return;
            }
            if (iVar7 - ((int)(iVar7 + (iVar7 >> 0x1f & 7U)) >> 3) < iVar8 + iVar6) {
              FUN_004970a0(iVar2,iVar3,iVar4,iVar5);
              return;
            }
          }
          else {
            *local_20 = uVar1 | 1;
          }
        }
        local_14 = local_14 + 1;
        local_20 = local_20 + 1;
        piVar9 = piVar9 + 4;
      } while (local_14 < *(int *)(param_1 + 100));
    }
    piVar9 = (int *)(*(int *)(param_1 + 100) * 0x10 + 0x68 + param_1);
    *piVar9 = *param_2;
    piVar9[1] = param_2[1];
    piVar9[2] = param_2[2];
    piVar9[3] = param_2[3];
    *(uint *)(param_1 + 0x6a8 + *(int *)(param_1 + 100) * 4) = param_3 & 0xfffffffe;
    *(int *)(param_1 + 100) = *(int *)(param_1 + 100) + 1;
  }
  return;
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


// ==== FUN_00498ff0 @ 00498ff0

void FUN_00498ff0(int *param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar1 = FUN_004e49c0();
  iVar1 = iVar1 - DAT_0059a91c;
  iVar2 = FUN_004e49c0();
  iVar2 = iVar2 - DAT_0059a920;
  iVar3 = FUN_0049a840(param_2);
  if (*(int *)(iVar3 + 0xdc) == 0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[3] = 0;
    param_1[2] = 0;
    return;
  }
  iVar3 = FUN_00419850(0,param_3);
  iVar1 = iVar1 - (*(short *)(iVar3 + 0x1c) * DAT_0059a918 >> 0x10);
  iVar4 = *(short *)(iVar3 + 0x18) * DAT_0059a918;
  iVar2 = iVar2 - (*(short *)(iVar3 + 0x1e) * DAT_0059a918 >> 0x10);
  iVar3 = *(short *)(iVar3 + 0x1a) * DAT_0059a918;
  param_1[1] = iVar2;
  *param_1 = iVar1;
  param_1[3] = (iVar3 >> 0x10) + 1 + iVar2;
  param_1[2] = (iVar4 >> 0x10) + 1 + iVar1;
  return;
}


// ==== FUN_004990d0 @ 004990d0

void FUN_004990d0(undefined4 param_1,int param_2,undefined4 param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  int *piVar2;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined1 local_10 [16];
  
  if (DAT_0059a8b0 == 0) {
    FUN_00498ff0(&local_20,param_1,param_2,param_3,param_4);
    FUN_00445290(DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
    if ((param_5 & 0x10) != 0) {
      if (local_18 - local_20 < 0x13) {
        local_20 = local_20 + -9;
        local_18 = local_18 + 9;
      }
      local_1c = local_1c + -0xf;
      local_20 = local_20 + -3;
      local_18 = local_18 + 3;
    }
    if ((local_18 != local_20 && -1 < local_18 - local_20) &&
       (local_14 != local_1c && -1 < local_14 - local_1c)) {
      FUN_00497580(&local_20,param_5);
    }
    iVar1 = FUN_0049a840(param_1);
    if ((*(uint *)(iVar1 + 0xe8) & 0x40000) != 0) {
      piVar2 = (int *)FUN_00498ff0(local_10,param_1,*(int *)(iVar1 + 0x114) + param_2,param_3,
                                   param_4);
      local_20 = *piVar2;
      local_1c = piVar2[1];
      local_18 = piVar2[2];
      local_14 = piVar2[3];
      FUN_00445290(DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
      if ((local_18 != local_20 && -1 < local_18 - local_20) &&
         (local_14 != local_1c && -1 < local_14 - local_1c)) {
        FUN_00497580(&local_20,param_5);
      }
    }
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
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


// ==== FUN_0049ae80 @ 0049ae80

float * __thiscall FUN_0049ae80(int param_1,float *param_2,float param_3,float param_4,byte param_5)

{
  float fVar1;
  int iVar2;
  
  if ((param_5 & 1) == 0) {
    iVar2 = *(int *)(param_1 + 0x1d8);
  }
  else {
    iVar2 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
  }
  fVar1 = param_3 - (float)(*(int *)(param_1 + 0x1d4) + -1);
  param_2[1] = param_4;
  param_2[3] = param_4;
  *param_2 = param_3;
  param_2[2] = param_3;
  param_4 = param_4 - (float)(iVar2 + -1);
  FUN_0040e880(fVar1,param_4);
  FUN_0040e8f0(fVar1,param_4);
  return param_2;
}


// ==== FUN_0049c2e0 @ 0049c2e0

void FUN_0049c2e0(void)

{
  undefined4 uVar1;
  int iVar2;
  
  FUN_0049c230();
  DAT_00544cc4 = (FILE *)0x0;
  DAT_0059ab38 = 0;
  FUN_0049c160(PTR_DAT_00540dd0);
  FUN_0040cef0();
  _fclose(DAT_00544cc4);
  if (0 < DAT_00541348) {
    iVar2 = 0;
    do {
      FUN_0049b0d0();
      uVar1 = FUN_004e49c0();
      (&DAT_0059ab50)[iVar2] = uVar1;
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00541348);
  }
  if (DAT_0059ab48 == (void *)0x0) {
    FUN_004e0620("budArray","\\Ns\\O\\Rifttype.cpp",0x548);
  }
  FUN_0049c270();
  _free(DAT_0059ab48);
  DAT_0059ab48 = (void *)0x0;
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


// ==== FUN_004ab880 @ 004ab880

uint FUN_004ab880(int param_1,int param_2,int param_3,int param_4)

{
  uint uVar1;
  int iVar2;
  byte *pbVar3;
  byte *pbVar4;
  
  uVar1 = 0xffff;
  if (param_2 < 1) {
    param_2 = 1;
  }
  else if (0xff < param_2) {
    param_2 = 0xff;
  }
  if (param_4 < 1) {
    param_4 = 1;
  }
  else if (0xff < param_4) {
    param_4 = 0xff;
  }
  if (param_1 < 1) {
    param_1 = 1;
  }
  else if (0xff < param_1) {
    param_1 = 0xff;
  }
  if (param_3 < 1) {
    param_3 = 1;
  }
  else if (0xff < param_3) {
    param_3 = 0xff;
  }
  if (param_2 <= param_4) {
    pbVar3 = (byte *)(param_2 * 0x100 + DAT_005c7c44 + param_1);
    param_2 = (param_4 - param_2) + 1;
    do {
      if (param_1 <= param_3) {
        iVar2 = (param_3 - param_1) + 1;
        pbVar4 = pbVar3;
        do {
          uVar1 = uVar1 & *pbVar4;
          pbVar4 = pbVar4 + 1;
          iVar2 = iVar2 + -1;
        } while (iVar2 != 0);
      }
      pbVar3 = pbVar3 + 0x100;
      param_2 = param_2 + -1;
    } while (param_2 != 0);
  }
  return uVar1;
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


// ==== FUN_004abb10 @ 004abb10

void FUN_004abb10(int param_1)

{
  undefined1 *puVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  
  if (0 < param_1) {
    DAT_005c847c = param_1;
    DAT_005c8464 = _malloc(param_1 * 0x32);
  }
  uVar2 = DAT_005c847c * 0x32;
  puVar6 = DAT_005c8464;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  iVar4 = 2;
  if (2 < DAT_005c847c) {
    iVar5 = 100;
    do {
      *(byte *)(iVar5 + 0xb + (int)DAT_005c8464) = *(byte *)(iVar5 + 0xb + (int)DAT_005c8464) | 1;
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 0x32;
    } while (iVar4 < DAT_005c847c);
  }
  iVar4 = DAT_005c847c;
  DAT_005c8470 = (undefined1 *)((int)DAT_005c8464 + 0xe);
  DAT_005c8468 = DAT_005c8464;
  DAT_005c8478 = 0;
  iVar7 = 5;
  iVar5 = 0xfa;
  do {
    puVar6 = DAT_005c8464;
    iVar7 = iVar7 + 1;
    if (iVar7 % 15000 == iVar5 / 0x32) {
      FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
    }
    *(short *)((int)puVar6 + iVar5 + 4) = (short)(iVar7 % 15000);
    DAT_005c8478 = DAT_005c8478 + 1;
    iVar5 = iVar5 + 0x32;
  } while (iVar5 < 750000);
  iVar5 = 750000;
  iVar7 = 15000;
  do {
    puVar6 = DAT_005c8464;
    iVar7 = iVar7 + 1;
    if (iVar7 % 0x59d9 == iVar5 / 0x32) {
      FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
    }
    *(short *)((int)puVar6 + iVar5 + 4) = (short)(iVar7 % 0x59d9);
    DAT_005c8478 = DAT_005c8478 + 1;
    iVar5 = iVar5 + 0x32;
  } while (iVar5 < 0x118c62);
  iVar5 = 0x59d9;
  if (0x59d9 < iVar4) {
    param_1 = 0x118c62;
    do {
      puVar6 = DAT_005c8464;
      iVar5 = iVar5 + 1;
      if (iVar5 % iVar4 == param_1 / 0x32) {
        FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
      }
      *(short *)((int)puVar6 + param_1 + 4) = (short)(iVar5 % iVar4);
      DAT_005c8478 = DAT_005c8478 + 1;
      param_1 = param_1 + 0x32;
    } while (iVar5 < iVar4);
  }
  puVar6 = DAT_005c8468;
  if (((int)DAT_005c8468 - (int)DAT_005c8464) / 0x32 == 5) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(undefined2 *)(puVar6 + 1) = 5;
  puVar1 = DAT_005c8470;
  DAT_005c846c = (undefined1 *)((int)DAT_005c8464 + 0xb717e);
  if (((int)DAT_005c8470 - (int)DAT_005c8464) / 0x32 == 15000) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(undefined2 *)(puVar1 + 4) = 15000;
  DAT_005c8474 = DAT_005c8464 + 0x4630c;
  if (*(short *)(DAT_005c8464 + 0x4630d) != 0) {
    FUN_004e0620("serverFreeTail->getNext() == 0","\\Ns\\O\\Squid.cpp",0xc0);
  }
  DAT_005424b0 = 1;
  return;
}


// ==== FUN_004abe00 @ 004abe00

void FUN_004abe00(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int local_8;
  int local_4;
  
  iVar3 = 0;
  iVar7 = 0;
  local_8 = 0;
  iVar5 = 0;
  iVar6 = 15000;
  local_4 = 750000;
  iVar4 = DAT_005c8464;
  do {
    iVar2 = DAT_005c8470;
    iVar1 = local_4 + iVar4;
    if ((*(byte *)(local_4 + 0xb + iVar4) & 1) != 0) {
      if (iVar3 == 0) {
        if (iVar6 == (DAT_005c8470 - iVar4) / 0x32) {
          FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
        }
        *(short *)(iVar2 + 4) = (short)iVar6;
        local_8 = iVar6;
      }
      else {
        if (iVar6 == (iVar5 - iVar4) / 0x32) {
          FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
        }
        *(short *)(iVar5 + 4) = (short)iVar6;
      }
      DAT_005c8478 = DAT_005c8478 + 1;
      iVar3 = local_8;
      iVar4 = DAT_005c8464;
      iVar5 = iVar1;
      iVar7 = iVar6;
    }
    local_4 = local_4 + 0x32;
    iVar6 = iVar6 + 1;
  } while (local_4 < 0x118c62);
  if (iVar3 == 0) {
    FUN_004e0620("firstFree","\\Ns\\O\\Squid.cpp",0xe1);
    iVar4 = DAT_005c8464;
  }
  iVar5 = iVar7 * 0x32 + iVar4;
  iVar3 = iVar5 - iVar4 >> 0x1f;
  DAT_005c8474 = iVar5;
  if ((iVar5 - iVar4) / 0x32 + iVar3 == iVar3) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(undefined2 *)(iVar5 + 4) = 0;
  return;
}


// ==== FUN_004abf60 @ 004abf60

void FUN_004abf60(int param_1)

{
  undefined1 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  puVar3 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 4) == 0) {
    FUN_004e0620("p->isVoid()","\\Ns\\O\\Squid.cpp",0x15c);
  }
  if ((*(byte *)((int)puVar3 + 0xb) & 1) != 0) {
    FUN_004e0620("!p->isFree()","\\Ns\\O\\Squid.cpp",0x15d);
  }
  FUN_004952f0(param_1,*(undefined1 *)((int)puVar3 + 10));
  uVar1 = *(undefined1 *)((int)puVar3 + 10);
  puVar4 = puVar3;
  for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  *(undefined1 *)((int)puVar3 + 10) = uVar1;
  if ((DAT_00540bc0 == 0) && (14999 < param_1)) {
    *(byte *)((int)puVar3 + 0xb) = *(byte *)((int)puVar3 + 0xb) | 1;
    return;
  }
  if ((param_1 < 5) || (puVar4 = &DAT_005c846c, 14999 < param_1)) {
    puVar4 = &DAT_005c8474;
  }
  if ((4 < param_1) && (param_1 < 0x59d9)) {
    FUN_004aba70(param_1);
    *puVar4 = puVar3;
    FUN_004aba70(0);
    DAT_005c8478 = DAT_005c8478 + 1;
  }
  *(byte *)((int)puVar3 + 0xb) = *(byte *)((int)puVar3 + 0xb) | 3;
  return;
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __fastcall FUN_004ac1e0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004ac550 @ 004ac550

void __fastcall FUN_004ac550(int param_1)

{
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
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


// ==== FUN_004ad390 @ 004ad390

undefined1 __fastcall FUN_004ad390(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (*(byte *)(param_1 + 10) != DAT_005411c0)) &&
     (*(byte *)(param_1 + 10) != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x739);
  }
  return *(undefined1 *)(param_1 + 0x1e);
}


// ==== FUN_004ad3e0 @ 004ad3e0

void __thiscall FUN_004ad3e0(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = (uint)*(byte *)(param_1 + 10);
  iVar1 = FUN_0049a840(uVar2);
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (uVar2 != DAT_005411c0)) &&
     (uVar2 != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x74b);
  }
  if (((param_2 < 0) || (DAT_0054040c <= param_2)) &&
     ((param_2 != DAT_00540410 && ((uVar2 != DAT_005411c0 && (uVar2 != DAT_00541288)))))) {
    FUN_004e0620("( n >=0 && n < MaxGraphs) || ( n==invalidGraphNum) || type == fencemarkType || type == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x750);
  }
  *(char *)(param_1 + 0x1e) = (char)param_2;
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


// ==== FUN_004ad680 @ 004ad680

int __fastcall FUN_004ad680(int param_1)

{
  int iVar1;
  int iVar2;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x854);
  }
  iVar1 = *(int *)(param_1 + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
}


// ==== FUN_004ade70 @ 004ade70

undefined4 __thiscall FUN_004ade70(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = *(undefined4 *)(param_1 + 0xe);
  uVar2 = *(undefined4 *)(param_1 + 0x12);
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  FUN_0049ae80(param_2,uVar1,uVar2,param_3);
  return param_2;
}


// ==== FUN_004adea0 @ 004adea0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004adea0(int param_1,float *param_2)

{
  float fVar1;
  int *piVar2;
  int *piVar3;
  undefined4 uVar4;
  int local_10;
  int local_c [3];
  
  uVar4 = 0;
  piVar3 = local_c;
  piVar2 = &local_10;
  FUN_0049a840(*(undefined1 *)(param_1 + 10),piVar2,piVar3,0);
  FUN_0049a900(piVar2,piVar3,uVar4);
  fVar1 = *(float *)(param_1 + 0x12);
  *param_2 = *(float *)(param_1 + 0xe) - (float)local_10 * _DAT_00500ee4;
  param_2[1] = fVar1 - (float)local_c[0] * _DAT_00500ee4;
  return;
}


// ==== FUN_004af1d0 @ 004af1d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_004af1d0(uint param_1)

{
  ushort uVar1;
  ushort uVar2;
  byte *pbVar3;
  uint uVar4;
  int *piVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  undefined4 *puVar12;
  
  if (DAT_005c8458 != 0) {
    FUN_004e0620("disallowSquidAllocation == 0","\\Ns\\O\\Squid.cpp",0xe8);
  }
  if ((param_1 & 1) != 0) {
    if ((*(byte *)(DAT_005c8464 + 0x118c6d) & 1) == 0) {
      FUN_004e0620("predictableHead->isFree()","\\Ns\\O\\Squid.cpp",0xf0);
    }
    iVar10 = DAT_005424b0;
    uVar4 = DAT_005424b0 + 0x59d9;
    if (DAT_005c847c <= (int)uVar4) {
      FUN_004e0620("sid < numSquids","\\Ns\\O\\Squid.cpp",0xf2);
    }
    puVar7 = (undefined4 *)(uVar4 * 0x32 + DAT_005c8464);
    if ((*(byte *)(uVar4 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
      FUN_004e0620("me->isFree()","\\Ns\\O\\Squid.cpp",0xf4);
    }
    if (DAT_00540bc0 != 0) {
      if ((uint)*(ushort *)(puVar7 + 1) != iVar10 + 0x59daU) {
        FUN_004e0620("me->getNext() == sid+1","\\Ns\\O\\Squid.cpp",0xf7);
      }
      iVar10 = (uint)*(ushort *)(puVar7 + 1) * 0x32 + DAT_005c8464;
      if (iVar10 == 0) {
        FUN_004e0620("checkMe","\\Ns\\O\\Squid.cpp",0xf9);
      }
      if ((*(byte *)(iVar10 + 0xb) & 1) == 0) {
        FUN_004e0620("checkMe->isFree()","\\Ns\\O\\Squid.cpp",0xfa);
      }
      FUN_004aba70(*(undefined2 *)(puVar7 + 1));
    }
    puVar12 = puVar7;
    for (iVar10 = 0xc; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar12 = 0;
      puVar12 = puVar12 + 1;
    }
    *(undefined2 *)puVar12 = 0;
    *(byte *)((int)puVar7 + 0xb) = *(byte *)((int)puVar7 + 0xb) & 0xfc | 4;
    DAT_005424b0 = DAT_005424b0 + 1;
    DAT_005c8478 = DAT_005c8478 + -1;
    return uVar4;
  }
  if ((param_1 & 2) == 0) {
    if (DAT_00540bc0 == 0) {
      FUN_004e0620("server","\\Ns\\O\\Squid.cpp",0x10a);
    }
    piVar11 = &DAT_005c8470;
    piVar5 = &DAT_005c8474;
  }
  else {
    piVar11 = &DAT_005c8468;
    piVar5 = &DAT_005c846c;
  }
  iVar9 = DAT_005c847c;
  iVar10 = DAT_005c8464;
  if ((uint)*(ushort *)(*piVar11 + 4) * 0x32 + DAT_005c8464 == *piVar5) {
    if (DAT_00540bc4 != 0) {
      iVar6 = 0x32;
      FUN_0040eb40();
      iVar8 = 1;
      do {
        piVar5 = (int *)(iVar8 * 0x32 + iVar10);
        if (((piVar5 < (int *)(iVar10 + 0x32U)) || ((int *)(DAT_005c847c * 0x32 + iVar10) <= piVar5)
            ) || (iVar6 == 0)) break;
        if (*(byte *)((int)piVar5 + 10) == _DAT_005411a0) {
          (**(code **)(*piVar5 + 0x10))(0);
          iVar6 = iVar6 + -1;
          iVar10 = DAT_005c8464;
        }
        iVar8 = iVar8 + 1;
        if (iVar8 < iVar9) {
          pbVar3 = (byte *)(iVar8 * 0x32 + 0xb + iVar10);
          do {
            if ((*pbVar3 & 1) == 0) break;
            iVar8 = iVar8 + 1;
            pbVar3 = pbVar3 + 0x32;
          } while (iVar8 < iVar9);
        }
      } while( true );
    }
    if ((param_1 & 2) == 0) {
      if (DAT_00540bc0 == 0) {
        FUN_004e0620("server","\\Ns\\O\\Squid.cpp",0x139);
        iVar10 = DAT_005c8464;
      }
      piVar11 = &DAT_005c8470;
      piVar5 = &DAT_005c8474;
    }
    else {
      piVar11 = &DAT_005c8468;
      piVar5 = &DAT_005c846c;
    }
  }
  uVar1 = *(ushort *)(*piVar11 + 4);
  iVar9 = (uint)uVar1 * 0x32;
  puVar7 = (undefined4 *)(iVar9 + iVar10);
  if ((*(byte *)(iVar9 + 0xb + iVar10) & 1) == 0) {
    FUN_004e0620("p->isFree()","\\Ns\\O\\Squid.cpp",0x149);
    iVar10 = DAT_005c8464;
  }
  if (puVar7 == (undefined4 *)*piVar5) {
    FUN_004e0620("p != *freeTail","\\Ns\\O\\Squid.cpp",0x14a);
    iVar10 = DAT_005c8464;
  }
  iVar9 = *piVar11;
  uVar2 = *(ushort *)(puVar7 + 1);
  if ((uint)uVar2 == (iVar9 - iVar10) / 0x32) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(ushort *)(iVar9 + 4) = uVar2;
  puVar12 = puVar7;
  for (iVar10 = 0xc; iVar10 != 0; iVar10 = iVar10 + -1) {
    *puVar12 = 0;
    puVar12 = puVar12 + 1;
  }
  *(undefined2 *)puVar12 = 0;
  *(byte *)((int)puVar7 + 0xb) = *(byte *)((int)puVar7 + 0xb) & 0xfc | 4;
  DAT_005c8478 = DAT_005c8478 + -1;
  return (uint)uVar1;
}


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


// ==== FUN_004b16d0 @ 004b16d0

void __thiscall FUN_004b16d0(int *param_1,uint param_2,uint param_3,int param_4,int param_5)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  ushort *puVar4;
  
  if (param_4 == -1) {
    param_4 = DAT_00531928;
  }
  if (param_5 == -1) {
    param_5 = DAT_00531928;
  }
  param_2 = ((int)param_2 < 1) - 1 & param_2;
  param_1[1] = param_2;
  param_3 = ((int)param_3 < 1) - 1 & param_3;
  param_1[2] = param_3;
  iVar3 = DAT_00531928 + -1;
  if (param_4 < DAT_00531928 + -1) {
    iVar3 = param_4;
  }
  param_1[3] = iVar3;
  iVar3 = DAT_00531928 + -1;
  if (param_5 < DAT_00531928 + -1) {
    iVar3 = param_5;
  }
  param_1[4] = iVar3;
  if (param_1[0xe] == 0) {
    param_1[9] = 0;
  }
  else {
    param_1[9] = 1;
  }
  FUN_00434380(param_2,param_3,param_1 + 5,param_1 + 6,param_1[9]);
  piVar1 = param_1 + 8;
  piVar2 = param_1 + 7;
  FUN_00434380(param_1[3],param_1[4],piVar2,piVar1,param_1[9]);
  iVar3 = *piVar2;
  *piVar1 = *piVar1 + 1;
  *piVar2 = iVar3 + 1;
  if (iVar3 + 1 < 0) {
    *piVar2 = 0;
  }
  if (*piVar1 < 0) {
    *piVar1 = 0;
  }
  iVar3 = DAT_00531928 / (int)(&DAT_005424f8)[param_1[9]] + -1;
  if (iVar3 < *piVar2) {
    *piVar2 = iVar3;
  }
  iVar3 = DAT_00531928 / (int)(&DAT_005424f8)[param_1[9]] + -1;
  if (iVar3 < *piVar1) {
    *piVar1 = iVar3;
  }
  iVar3 = param_1[6];
  param_1[0xb] = iVar3;
  param_1[10] = param_1[5];
  param_1[0xd] = 0;
  puVar4 = (ushort *)FUN_004b2b00(param_1[5],iVar3,param_1[9]);
  param_1[0xc] = (uint)*puVar4;
  (**(code **)(*param_1 + 4))();
  return;
}


// ==== FUN_004b1810 @ 004b1810

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004b1810(undefined4 *param_1)

{
  byte bVar1;
  ushort uVar2;
  ushort *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  int *piVar9;
  undefined4 uVar10;
  int local_10;
  int local_c;
  float local_8;
  float local_4;
  
LAB_004b1820:
  do {
    if (param_1[0xc] == 0) {
      piVar9 = param_1 + 7;
      do {
        iVar7 = param_1[10];
        param_1[10] = iVar7 + 1;
        if (*piVar9 < iVar7 + 1) {
          iVar7 = param_1[0xb];
          param_1[10] = param_1[5];
          param_1[0xb] = iVar7 + 1;
          piVar8 = param_1 + 8;
          if ((int)param_1[8] < iVar7 + 1) {
            iVar7 = param_1[9] + 1;
            param_1[9] = iVar7;
            if (3 < iVar7) {
              param_1[0xd] = 0;
              return;
            }
            FUN_00434380(param_1[1],param_1[2],param_1 + 5,param_1 + 6,iVar7);
            FUN_00434380(param_1[3],param_1[4],piVar9,piVar8,param_1[9]);
            iVar7 = *piVar9;
            *piVar8 = *piVar8 + 1;
            *piVar9 = iVar7 + 1;
            if (iVar7 + 1 < 0) {
              *piVar9 = 0;
            }
            if (*piVar8 < 0) {
              *piVar8 = 0;
            }
            iVar7 = DAT_00531928 / (int)(&DAT_005424f8)[param_1[9]] + -1;
            if (iVar7 < *piVar9) {
              *piVar9 = iVar7;
            }
            iVar7 = DAT_00531928 / (int)(&DAT_005424f8)[param_1[9]] + -1;
            if (iVar7 < *piVar8) {
              *piVar8 = iVar7;
            }
            param_1[10] = param_1[5];
            param_1[0xb] = param_1[6];
            param_1[0xd] = 0;
          }
        }
        puVar3 = (ushort *)FUN_004b2b00(param_1[10],param_1[0xb],param_1[9]);
        uVar2 = *puVar3;
        param_1[0xc] = (uint)uVar2;
      } while (uVar2 == 0);
    }
    do {
      iVar7 = param_1[0xc];
      param_1[0xd] = iVar7;
      if (iVar7 == 0) goto LAB_004b1820;
      bVar1 = *(byte *)(iVar7 * 0x32 + DAT_005c8464 + 0x28);
      param_1[0xc] = (uint)*(ushort *)(iVar7 * 0x32 + 4 + DAT_005c8464);
    } while ((bVar1 & 8) != 0);
    local_4 = *(float *)(param_1[0xd] * 0x32 + 0x12 + DAT_005c8464);
    iVar7 = param_1[0xd] * 0x32 + DAT_005c8464;
    local_8 = *(float *)(iVar7 + 0xe);
    if ((((_DAT_00500fc4 < local_8) && (local_8 < (float)DAT_00531928)) && (_DAT_00500fc4 < local_4)
        ) && (local_4 < (float)DAT_00531928)) {
      uVar10 = param_1[0xf];
      piVar9 = &local_c;
      piVar8 = &local_10;
      FUN_0049a840(*(undefined1 *)(iVar7 + 10),piVar8,piVar9,uVar10);
      FUN_0049a900(piVar8,piVar9,uVar10);
      iVar7 = FUN_004e49c0();
      iVar4 = FUN_004e49c0();
      iVar5 = (iVar7 - local_10) + 1;
      iVar6 = (iVar4 - local_c) + 1;
      if (((((((int)param_1[1] <= iVar7) || ((int)param_1[1] <= iVar5)) &&
            ((iVar7 <= (int)param_1[3] || (iVar5 <= (int)param_1[3])))) &&
           (((int)param_1[2] <= iVar4 || ((int)param_1[2] <= iVar6)))) &&
          ((iVar4 <= (int)param_1[4] || (iVar6 <= (int)param_1[4])))) &&
         (iVar7 = (**(code **)*param_1)(param_1[0xd] * 0x32 + DAT_005c8464), iVar7 != 0)) {
        return;
      }
    }
  } while( true );
}


// ==== FUN_004b1c20 @ 004b1c20

void __fastcall FUN_004b1c20(undefined4 *param_1)

{
  float fVar1;
  float fVar2;
  uint uVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  
LAB_004b1c2a:
  do {
    param_1[0xd] = 0;
    while( true ) {
      do {
        iVar4 = param_1[0x1b];
        iVar6 = param_1[0x1a] + 1;
        param_1[0x1a] = iVar6;
        if ((iVar4 == param_1[0x1d]) || (iVar4 == param_1[0x1f])) {
          if ((int)param_1[0x1c] <= iVar6) {
            param_1[0x1b] = iVar4 + 1;
            if ((int)param_1[0x1f] < iVar4 + 1) {
              return;
            }
            param_1[0x1a] = param_1[0x1e];
          }
        }
        else if ((int)param_1[0x1c] < iVar6) {
          param_1[0x1b] = iVar4 + 1;
          if (iVar4 + 1 == param_1[0x1f]) {
            param_1[0x1a] = param_1[0x1e] + 1;
          }
          else {
            param_1[0x1a] = param_1[0x1e];
          }
        }
        iVar4 = param_1[0x1a];
        iVar6 = param_1[0x1b];
        if ((((iVar4 < 0) || (iVar6 < 0)) || (0xff < iVar4)) || (0xff < iVar6)) {
          uVar3 = 0;
        }
        else {
          uVar3 = (uint)*(ushort *)(DAT_005c84bc + (iVar6 * 0x100 + iVar4) * 2);
        }
        param_1[0xd] = uVar3;
      } while (uVar3 == 0);
      iVar4 = uVar3 * 0x32 + DAT_005c8464;
      fVar1 = *(float *)(iVar4 + 0xe);
      fVar2 = *(float *)(iVar4 + 0x12);
      if (((((float)param_1[0x15] < fVar1 != ((float)param_1[0x15] == fVar1)) &&
           (fVar1 < (float)param_1[0x17] != (fVar1 == (float)param_1[0x17]))) &&
          (((float)param_1[0x16] < fVar2 != ((float)param_1[0x16] == fVar2) &&
           (fVar2 < (float)param_1[0x18] != (fVar2 == (float)param_1[0x18]))))) ||
         (iVar4 = (**(code **)*param_1)(iVar4), iVar4 == 0)) break;
      iVar4 = 0;
      if (0 < (int)param_1[0x19]) {
        psVar5 = (short *)(param_1 + 0x20);
        do {
          if ((int)*psVar5 == param_1[0xd]) goto LAB_004b1c2a;
          iVar4 = iVar4 + 1;
          psVar5 = psVar5 + 1;
        } while (iVar4 < (int)param_1[0x19]);
      }
      if (param_1[0xd] != 0) {
        *(undefined2 *)((int)param_1 + param_1[0x19] * 2 + 0x80) = *(undefined2 *)(param_1 + 0xd);
        param_1[0x19] = param_1[0x19] + 1;
        return;
      }
    }
  } while( true );
}


// ==== FUN_004b1d70 @ 004b1d70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004b1d70(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  uVar1 = FUN_004e49c0();
  uVar1 = FUN_004e49c0(uVar1);
  uVar1 = FUN_004e49c0(uVar1);
  uVar1 = FUN_004e49c0(uVar1);
  uVar2 = FUN_004ab880(uVar1);
  if ((uVar2 & 8) != 0) {
    *(undefined4 *)(param_1 + 0x34) = 0;
    return;
  }
  if ((*(byte *)(param_1 + 0x48) & 8) != 0) {
    uVar1 = FUN_004e49c0();
    *(undefined4 *)(param_1 + 0x78) = uVar1;
    uVar3 = FUN_004e49c0();
    *(undefined4 *)(param_1 + 0x74) = uVar3;
    uVar4 = FUN_004e49c0();
    *(undefined4 *)(param_1 + 0x70) = uVar4;
    uVar4 = FUN_004e49c0();
    *(undefined4 *)(param_1 + 0x68) = uVar1;
    *(undefined4 *)(param_1 + 0x6c) = uVar3;
    *(undefined4 *)(param_1 + 0x7c) = uVar4;
    FUN_004b1c20();
    return;
  }
  uVar1 = FUN_004e49c0();
  uVar3 = FUN_004e49c0(uVar1);
  uVar4 = FUN_004e49c0(uVar3);
  uVar5 = FUN_004e49c0(uVar4);
  FUN_004b16d0(uVar5,uVar4,uVar3,uVar1);
  return;
}


// ==== FUN_004b1e70 @ 004b1e70

void __fastcall FUN_004b1e70(int param_1)

{
  if ((*(byte *)(param_1 + 0x48) & 8) != 0) {
    FUN_004b1c20();
    return;
  }
  FUN_004b1810();
  return;
}


// ==== FUN_004b1e80 @ 004b1e80

undefined4 __thiscall FUN_004b1e80(int param_1,int param_2)

{
  byte bVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uStack_10;
  undefined4 uStack_c;
  
  uStack_10 = *(undefined4 *)(param_2 + 0xe);
  uStack_c = *(undefined4 *)(param_2 + 0x12);
  iVar3 = FUN_004e49c0();
  iVar4 = FUN_004e49c0();
  bVar1 = *(byte *)(iVar4 + iVar3 * 0x100 + DAT_005c7c44);
  uVar5 = FUN_004ac1e0();
  if (((uVar5 & 0x800) != 0) || ((*(byte *)(param_1 + 0x48) & 2) != 0)) {
    if ((*(byte *)(param_1 + 0x48) & 8) == 0) {
      puVar6 = (undefined4 *)FUN_004ade70(&uStack_10,0);
      iVar3 = FUN_0041d9b0(*puVar6,puVar6[1],puVar6[2],puVar6[3]);
      if (iVar3 == 0) {
        return 0;
      }
    }
    if ((*(byte *)(param_1 + 0x48) & 1) == 0) {
      uVar7 = *(undefined4 *)(param_1 + 0x50);
      uVar8 = *(undefined4 *)(param_1 + 0x4c);
      uVar2 = *(undefined4 *)(param_2 + 0x24);
      uVar9 = 1;
      FUN_004adea0(&uStack_10);
      uVar7 = FUN_0041ce90(uVar8,uVar7,uVar9);
      uVar7 = FUN_004ac550(uVar2,*(undefined4 *)(param_1 + 0x40),*(undefined4 *)(param_1 + 0x44),
                           uVar7);
      iVar3 = FUN_00441e40(uVar7);
      if (iVar3 == 0) {
        return 0;
      }
    }
    if (((*(byte *)(param_2 + 0xb) & 2) == 0) &&
       ((((*(byte *)(param_1 + 0x48) & 4) == 0 || ((*(byte *)(param_2 + 0x28) & 1) == 0)) &&
        ((bVar1 & 8) == 0)))) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_004b23e0 @ 004b23e0

undefined4 * __thiscall FUN_004b23e0(undefined4 *param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined1 local_10 [16];
  
  param_1[0xe] = 0;
  param_1[0xf] = 0;
  param_1[0xd] = 0;
  *param_1 = &PTR_FUN_00513098;
  param_1[0x16] = 0xc61c4000;
  param_1[0x15] = 0xc61c4000;
  param_1[0x18] = 0xc61c4000;
  param_1[0x17] = 0xc61c4000;
  uVar5 = param_2 * 0x32 + DAT_005c8464;
  if ((uVar5 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar5)) {
    FUN_004e0620("s.isValid()","\\Ns\\O\\Squidfinder.cpp",0x182);
  }
  param_1[0x12] = param_3;
  if ((param_3 & 8) != 0) {
    param_1[0x19] = 0;
  }
  puVar3 = (undefined4 *)FUN_004ade70(local_10,0);
  uVar4 = puVar3[1];
  uVar1 = puVar3[2];
  uVar2 = puVar3[3];
  param_1[0x15] = *puVar3;
  param_1[0x16] = uVar4;
  param_1[0x17] = uVar1;
  param_1[0x18] = uVar2;
  uVar4 = FUN_004ac550();
  param_1[0x10] = uVar4;
  param_1[0x11] = *(undefined4 *)(uVar5 + 0x24);
  puVar3 = (undefined4 *)FUN_004adea0(local_10);
  uVar4 = *puVar3;
  param_1[0x14] = puVar3[1];
  param_1[0x13] = uVar4;
  FUN_004b1d70();
  return param_1;
}


// ==== FUN_004b24c0 @ 004b24c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 * __thiscall
FUN_004b24c0(undefined4 *param_1,float param_2,float param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9,
            uint param_10)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  int local_14;
  undefined1 local_10 [16];
  
  param_1[0xe] = 0;
  param_1[0xf] = 0;
  param_1[0xd] = 0;
  *param_1 = &PTR_FUN_00513098;
  param_1[0x16] = 0xc61c4000;
  param_1[0x15] = 0xc61c4000;
  param_1[0x18] = 0xc61c4000;
  param_1[0x17] = 0xc61c4000;
  param_1[0x12] = param_10;
  if ((param_10 & 8) != 0) {
    param_1[0x19] = 0;
  }
  uVar3 = FUN_0049a840(param_6);
  param_1[0x11] = param_9;
  param_1[0x10] = uVar3;
  puVar4 = (undefined4 *)FUN_0049ae80(local_10,param_2,param_3,0);
  uVar3 = *puVar4;
  uVar1 = puVar4[1];
  uVar2 = puVar4[2];
  param_1[0x18] = puVar4[3];
  param_1[0x15] = uVar3;
  param_1[0x16] = uVar1;
  param_1[0x17] = uVar2;
  FUN_0049a900(&param_10,&local_14,0);
  param_1[0x13] = param_2 - (float)(int)param_10 * _DAT_00500ee4;
  param_1[0x14] = param_3 - (float)local_14 * _DAT_00500ee4;
  FUN_004b1d70();
  return param_1;
}


// ==== FUN_004b2660 @ 004b2660

int FUN_004b2660(undefined4 param_1,int *param_2)

{
  uint uVar1;
  int iVar2;
  int local_cc;
  byte local_b8;
  
  uVar1 = FUN_004ac200();
  if ((uVar1 & 4) != 0) {
    iVar2 = 0;
    FUN_004b23e0(param_1,8);
    while (local_cc != 0) {
      if ((param_2 != (int *)0x0) && (param_2[2] < param_2[1])) {
        *(int *)(*param_2 + param_2[2] * 4) = local_cc;
        param_2[2] = param_2[2] + 1;
      }
      iVar2 = iVar2 + 1;
      if ((local_b8 & 8) == 0) {
        FUN_004b1810();
      }
      else {
        FUN_004b1c20();
      }
    }
    return iVar2;
  }
  return 0;
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


// ==== FUN_004b2b00 @ 004b2b00

int __thiscall FUN_004b2b00(int param_1,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_004e0620("level >=0 && level < HASH_LEVEL_COUNT","\\Ns\\O\\Squidhash.cpp",0x5c);
  }
  return *(int *)(param_1 + 0xc + param_4 * 4) +
         ((DAT_00531928 * param_3) / (int)(&DAT_005424f8)[param_4] + param_2) * 2;
}


// ==== FUN_004c25b0 @ 004c25b0

void FUN_004c25b0(int param_1)

{
  (&DAT_005c9cd0)[param_1] = (&DAT_005c9cd0)[param_1] + 1;
  return;
}


// ==== FUN_004c25e0 @ 004c25e0

void FUN_004c25e0(int param_1)

{
  (&DAT_005c94d0)[param_1] = (&DAT_005c94d0)[param_1] + 1;
  (&DAT_005c98d0)[param_1] = (&DAT_005c98d0)[param_1] + 1;
  return;
}


// ==== FUN_004c2630 @ 004c2630

void FUN_004c2630(int param_1,char *param_2)

{
  bool bVar1;
  uint uVar2;
  char cVar3;
  char *pcVar4;
  uint uVar5;
  char *pcVar6;
  undefined2 *puVar7;
  
  if ((-1 < param_1) && (param_1 <= DAT_00542658)) {
    pcVar6 = (char *)&DAT_005ca4d0;
    if (*param_2 == '~') {
      cVar3 = '~';
      do {
        *pcVar6 = cVar3;
        pcVar6[1] = param_2[1];
        pcVar6 = pcVar6 + 2;
        param_2 = param_2 + 2;
        cVar3 = *param_2;
      } while (cVar3 == '~');
    }
    uVar2 = DAT_005ca8d4;
    if (0 < (int)DAT_005ca8d4) {
      uVar5 = DAT_005ca8d4 >> 2;
      pcVar4 = pcVar6;
      while (uVar5 != 0) {
        uVar5 = uVar5 - 1;
        builtin_strncpy(pcVar4,"    ",4);
        pcVar4 = pcVar4 + 4;
      }
      for (uVar5 = uVar2 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar4 = ' ';
        pcVar4 = pcVar4 + 1;
      }
      pcVar6 = pcVar6 + uVar2;
    }
    if (param_2 != pcVar6) {
      FUN_004de490(pcVar6,param_2,&stack0x0000000c);
    }
    if (DAT_005ca8d0 == (FILE *)0x0) {
      DAT_005ca8d0 = (FILE *)&DAT_00543770;
    }
    pcVar6 = (char *)&DAT_005ca4d0;
    do {
      pcVar4 = pcVar6;
      pcVar6 = pcVar4 + 1;
    } while (*pcVar4 != '\0');
    bVar1 = false;
    pcVar6 = pcVar4 + -0x5ca4d1;
    if (-1 < (int)(pcVar4 + -0x5ca4d1)) {
      do {
        if ((pcVar6[0x5ca4d0] != '\n') && (pcVar6[0x5ca4d0] != '\r')) {
          if (!bVar1) goto LAB_004c2722;
          break;
        }
        pcVar4 = pcVar6 + -1;
        pcVar6[0x5ca4d0] = '\0';
        bVar1 = true;
        pcVar6 = pcVar4;
      } while (-1 < (int)pcVar4);
      puVar7 = (undefined2 *)&DAT_005ca4cf;
      do {
        pcVar6 = (char *)((int)puVar7 + 1);
        puVar7 = (undefined2 *)((int)puVar7 + 1);
      } while (*pcVar6 != '\0');
      *puVar7 = DAT_00504b0c;
    }
LAB_004c2722:
    FUN_00434f40(&DAT_005ca4d0);
    if ((DAT_005ca8d0 != (FILE *)0x0) && (DAT_005ca8dc != 0)) {
      _fprintf((FILE *)&DAT_00543790,"%s",&DAT_005ca4d0);
    }
    _fprintf(DAT_005ca8d0,"%s",&DAT_005ca4d0);
    if (DAT_005ca8d8 != 0) {
      _fflush(DAT_005ca8d0);
    }
  }
  return;
}


// ==== FUN_004da650 @ 004da650

void FUN_004da650(void)

{
  int iVar1;
  
  if ((((DAT_005caea4 != 0) &&
       (iVar1 = FUN_004b1fa0(DAT_005caf38,DAT_005caf3c,DAT_005caea4), iVar1 != 0)) &&
      ((DAT_005caea0 == 0 || (DAT_005caea0 == iVar1)))) &&
     (FUN_004d5c90(iVar1,0,1), (*(byte *)(iVar1 * 0x32 + 0x28 + DAT_005c8464) & 9) == 0)) {
    DAT_005caea4 = 0;
  }
  return;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


// ==== _memmove @ 004e5b40

/* Library Function - Single Match
    _memmove
   
   Libraries: Visual Studio 2003 Debug, Visual Studio 2003 Release */

void * __cdecl _memmove(void *_Dst,void *_Src,size_t _Size)

{
  uint uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  if ((_Src < _Dst) && (_Dst < (void *)(_Size + (int)_Src))) {
    puVar3 = (undefined4 *)((_Size - 4) + (int)_Src);
    puVar4 = (undefined4 *)((_Size - 4) + (int)_Dst);
    if (((uint)puVar4 & 3) == 0) {
      uVar1 = _Size >> 2;
      uVar2 = _Size & 3;
      if (7 < uVar1) {
        for (; uVar1 != 0; uVar1 = uVar1 - 1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + -1;
          puVar4 = puVar4 + -1;
        }
        switch(uVar2) {
        case 0:
          return _Dst;
        case 2:
          goto switchD_004e5cfb_caseD_2;
        case 3:
          goto switchD_004e5cfb_caseD_3;
        }
        goto switchD_004e5cfb_caseD_1;
      }
    }
    else {
      switch(_Size) {
      case 0:
        goto switchD_004e5cfb_caseD_0;
      case 1:
        goto switchD_004e5cfb_caseD_1;
      case 2:
        goto switchD_004e5cfb_caseD_2;
      case 3:
        goto switchD_004e5cfb_caseD_3;
      default:
        uVar1 = _Size - ((uint)puVar4 & 3);
        switch((uint)puVar4 & 3) {
        case 1:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          puVar3 = (undefined4 *)((int)puVar3 + -1);
          uVar1 = uVar1 >> 2;
          puVar4 = (undefined4 *)((int)puVar4 - 1);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
          break;
        case 2:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          puVar3 = (undefined4 *)((int)puVar3 + -2);
          puVar4 = (undefined4 *)((int)puVar4 - 2);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
          break;
        case 3:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
          puVar3 = (undefined4 *)((int)puVar3 + -3);
          puVar4 = (undefined4 *)((int)puVar4 - 3);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
        }
      }
    }
    switch(uVar1) {
    case 7:
      puVar4[7 - uVar1] = puVar3[7 - uVar1];
    case 6:
      puVar4[6 - uVar1] = puVar3[6 - uVar1];
    case 5:
      puVar4[5 - uVar1] = puVar3[5 - uVar1];
    case 4:
      puVar4[4 - uVar1] = puVar3[4 - uVar1];
    case 3:
      puVar4[3 - uVar1] = puVar3[3 - uVar1];
    case 2:
      puVar4[2 - uVar1] = puVar3[2 - uVar1];
    case 1:
      puVar4[1 - uVar1] = puVar3[1 - uVar1];
      puVar3 = puVar3 + -uVar1;
      puVar4 = puVar4 + -uVar1;
    }
    switch(uVar2) {
    case 1:
switchD_004e5cfb_caseD_1:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      return _Dst;
    case 2:
switchD_004e5cfb_caseD_2:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      return _Dst;
    case 3:
switchD_004e5cfb_caseD_3:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
      return _Dst;
    }
switchD_004e5cfb_caseD_0:
    return _Dst;
  }
  puVar3 = _Dst;
  if (((uint)_Dst & 3) == 0) {
    uVar1 = _Size >> 2;
    uVar2 = _Size & 3;
    if (7 < uVar1) {
      for (; uVar1 != 0; uVar1 = uVar1 - 1) {
        *puVar3 = *(undefined4 *)_Src;
        _Src = (undefined4 *)((int)_Src + 4);
        puVar3 = puVar3 + 1;
      }
      switch(uVar2) {
      case 0:
        return _Dst;
      case 2:
        goto switchD_004e5b75_caseD_2;
      case 3:
        goto switchD_004e5b75_caseD_3;
      }
      goto switchD_004e5b75_caseD_1;
    }
  }
  else {
    switch(_Size) {
    case 0:
      goto switchD_004e5b75_caseD_0;
    case 1:
      goto switchD_004e5b75_caseD_1;
    case 2:
      goto switchD_004e5b75_caseD_2;
    case 3:
      goto switchD_004e5b75_caseD_3;
    default:
      uVar1 = (_Size - 4) + ((uint)_Dst & 3);
      switch((uint)_Dst & 3) {
      case 1:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        *(undefined1 *)((int)_Dst + 1) = *(undefined1 *)((int)_Src + 1);
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)_Dst + 2) = *(undefined1 *)((int)_Src + 2);
        _Src = (void *)((int)_Src + 3);
        puVar3 = (undefined4 *)((int)_Dst + 3);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
        break;
      case 2:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)_Dst + 1) = *(undefined1 *)((int)_Src + 1);
        _Src = (void *)((int)_Src + 2);
        puVar3 = (undefined4 *)((int)_Dst + 2);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
        break;
      case 3:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        _Src = (void *)((int)_Src + 1);
        uVar1 = uVar1 >> 2;
        puVar3 = (undefined4 *)((int)_Dst + 1);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
      }
    }
  }
  switch(uVar1) {
  case 7:
    puVar3[uVar1 - 7] = *(undefined4 *)((int)_Src + (uVar1 - 7) * 4);
  case 6:
    puVar3[uVar1 - 6] = *(undefined4 *)((int)_Src + (uVar1 - 6) * 4);
  case 5:
    puVar3[uVar1 - 5] = *(undefined4 *)((int)_Src + (uVar1 - 5) * 4);
  case 4:
    puVar3[uVar1 - 4] = *(undefined4 *)((int)_Src + (uVar1 - 4) * 4);
  case 3:
    puVar3[uVar1 - 3] = *(undefined4 *)((int)_Src + (uVar1 - 3) * 4);
  case 2:
    puVar3[uVar1 - 2] = *(undefined4 *)((int)_Src + (uVar1 - 2) * 4);
  case 1:
    puVar3[uVar1 - 1] = *(undefined4 *)((int)_Src + (uVar1 - 1) * 4);
    _Src = (void *)((int)_Src + uVar1 * 4);
    puVar3 = puVar3 + uVar1;
  }
  switch(uVar2) {
  case 1:
switchD_004e5b75_caseD_1:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    return _Dst;
  case 2:
switchD_004e5b75_caseD_2:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)_Src + 1);
    return _Dst;
  case 3:
switchD_004e5b75_caseD_3:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)_Src + 1);
    *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)_Src + 2);
    return _Dst;
  }
switchD_004e5b75_caseD_0:
  return _Dst;
}


