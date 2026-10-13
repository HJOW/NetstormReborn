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


// ==== FUN_004b2b00 @ 004b2b00

int __thiscall FUN_004b2b00(int param_1,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_004e0620("level >=0 && level < HASH_LEVEL_COUNT","\\Ns\\O\\Squidhash.cpp",0x5c);
  }
  return *(int *)(param_1 + 0xc + param_4 * 4) +
         ((DAT_00531928 * param_3) / (int)(&DAT_005424f8)[param_4] + param_2) * 2;
}


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
}


