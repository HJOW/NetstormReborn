// ==== FUN_004eae20 @ 004eae20

void __thiscall FUN_004eae20(int *param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int iVar6;
  int iVar7;
  
  iVar2 = DAT_0052e9a8;
  if (param_4 == -1) {
    param_4 = DAT_0052e9a8;
  }
  if (param_5 == -1) {
    param_5 = DAT_0052e9a8;
  }
  if (param_2 < 1) {
    param_2 = 0;
  }
  param_1[1] = param_2;
  if (param_3 < 1) {
    param_3 = 0;
  }
  param_1[2] = param_3;
  iVar1 = iVar2 + -1;
  if (iVar1 <= param_4) {
    param_4 = iVar1;
  }
  param_1[3] = param_4;
  if (iVar1 <= param_5) {
    param_5 = iVar1;
  }
  param_1[4] = param_5;
  param_1[9] = 1;
  if (param_1[0xe] == 0) {
    param_1[9] = 0;
  }
  iVar1 = param_1[9];
  iVar2 = iVar2 / (int)(&DAT_0052d598)[iVar1] + -1;
  iVar3 = param_2 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_2 / (int)(&DAT_0052d598)[iVar1]) {
    iVar3 = iVar2;
  }
  param_1[5] = iVar3;
  iVar6 = param_3 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_3 / (int)(&DAT_0052d598)[iVar1]) {
    iVar6 = iVar2;
  }
  param_1[6] = iVar6;
  iVar7 = param_4 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_4 / (int)(&DAT_0052d598)[iVar1]) {
    iVar7 = iVar2;
  }
  param_1[7] = iVar7;
  iVar4 = param_5 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_5 / (int)(&DAT_0052d598)[iVar1]) {
    iVar4 = iVar2;
  }
  param_1[8] = iVar4;
  param_1[7] = iVar7 + 1;
  param_1[8] = iVar4 + 1;
  if (iVar7 + 1 < 0) {
    param_1[7] = 0;
  }
  if (iVar4 + 1 < 0) {
    param_1[8] = 0;
  }
  if (iVar2 < param_1[7]) {
    param_1[7] = iVar2;
  }
  if (iVar2 < param_1[8]) {
    param_1[8] = iVar2;
  }
  param_1[10] = iVar3;
  param_1[0xb] = iVar6;
  param_1[0xd] = 0;
  puVar5 = (ushort *)FUN_00479d90(iVar3,iVar6,param_1[9]);
  param_1[0xc] = (uint)*puVar5;
  (**(code **)(*param_1 + 4))();
  return;
}


// ==== FUN_004eafe0 @ 004eafe0

void __fastcall FUN_004eafe0(undefined4 *param_1)

{
  float *pfVar1;
  byte bVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  ushort *puVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int local_8;
  int local_4;
  
  iVar10 = DAT_005395dc;
LAB_004eafef:
  do {
    uVar9 = param_1[0xc];
    while (uVar9 == 0) {
      iVar10 = param_1[10];
      param_1[10] = iVar10 + 1;
      if ((int)param_1[7] < iVar10 + 1) {
        param_1[10] = param_1[5];
        iVar10 = param_1[0xb];
        param_1[0xb] = iVar10 + 1;
        if ((int)param_1[8] < iVar10 + 1) {
          iVar10 = param_1[9] + 1;
          param_1[9] = iVar10;
          if (3 < iVar10) {
            param_1[0xd] = 0;
            return;
          }
          iVar7 = DAT_0052e9a8 / (int)(&DAT_0052d598)[iVar10] + -1;
          iVar8 = (int)param_1[1] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[1] / (int)(&DAT_0052d598)[iVar10]) {
            iVar8 = iVar7;
          }
          param_1[5] = iVar8;
          iVar11 = (int)param_1[2] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[2] / (int)(&DAT_0052d598)[iVar10]) {
            iVar11 = iVar7;
          }
          param_1[6] = iVar11;
          iVar12 = (int)param_1[3] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[3] / (int)(&DAT_0052d598)[iVar10]) {
            iVar12 = iVar7;
          }
          param_1[7] = iVar12;
          iVar5 = (int)param_1[4] / (int)(&DAT_0052d598)[iVar10];
          if (iVar7 <= (int)param_1[4] / (int)(&DAT_0052d598)[iVar10]) {
            iVar5 = iVar7;
          }
          param_1[8] = iVar5;
          param_1[7] = iVar12 + 1;
          param_1[8] = iVar5 + 1;
          if (iVar12 + 1 < 0) {
            param_1[7] = 0;
          }
          if (iVar5 + 1 < 0) {
            param_1[8] = 0;
          }
          if (iVar7 < (int)param_1[7]) {
            param_1[7] = iVar7;
          }
          if (iVar7 < (int)param_1[8]) {
            param_1[8] = iVar7;
          }
          param_1[0xb] = iVar11;
          param_1[0xd] = 0;
          param_1[10] = iVar8;
        }
      }
      puVar6 = (ushort *)FUN_00479d90(param_1[10],param_1[0xb],param_1[9]);
      iVar10 = DAT_005395dc;
      uVar9 = (uint)*puVar6;
      param_1[0xc] = uVar9;
    }
    do {
      iVar7 = param_1[0xc];
      param_1[0xd] = iVar7;
      if (iVar7 == 0) goto LAB_004eafef;
      iVar7 = iVar7 * 0x24 + iVar10;
      bVar2 = *(byte *)(iVar7 + 0x23);
      param_1[0xc] = (uint)*(ushort *)(iVar7 + 4);
    } while ((bVar2 & 8) != 0);
    pfVar1 = (float *)(param_1[0xd] * 0x24 + 0xe + iVar10);
    fVar3 = *pfVar1;
    fVar4 = pfVar1[1];
    if (((((int)fVar3 < 1) || ((float)DAT_0052e9a8 <= fVar3)) || ((int)fVar4 < 1)) ||
       ((float)DAT_0052e9a8 <= fVar4)) {
      FUN_00490040(s_pos_isValid___00541890,s__ns_o_SquidFinder_cpp_00541878,0x89);
    }
    FUN_004442b0(&local_8,&local_4,param_1[0xf]);
    iVar7 = __ftol();
    iVar8 = __ftol();
    iVar12 = (iVar7 - local_8) + 1;
    iVar11 = (iVar8 - local_4) + 1;
    iVar10 = DAT_005395dc;
    if (((((((int)param_1[1] <= iVar7) || ((int)param_1[1] <= iVar12)) &&
          ((iVar7 <= (int)param_1[3] || (iVar12 <= (int)param_1[3])))) &&
         (((int)param_1[2] <= iVar8 || ((int)param_1[2] <= iVar11)))) &&
        ((iVar8 <= (int)param_1[4] || (iVar11 <= (int)param_1[4])))) &&
       (iVar7 = (**(code **)*param_1)(param_1[0xd] * 0x24 + DAT_005395dc), iVar10 = DAT_005395dc,
       iVar7 != 0)) {
      return;
    }
  } while( true );
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


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_00479d90 @ 00479d90

int __thiscall FUN_00479d90(int param_1,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_00490040(s_level_>_0____level_<_HASH_LEVEL__0052d5f8,s__ns_o_SquidHash_cpp_0052d5e4,0x5c);
  }
  return ((DAT_0052e9a8 * param_3) / (int)(&DAT_0052d598)[param_4] + param_2) * 2 +
         *(int *)(param_1 + 0xc + param_4 * 4);
}


// ==== FUN_0040f180 @ 0040f180

undefined4 FUN_0040f180(void)

{
  return 1;
}


