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


// ==== FUN_0040bc50 @ 0040bc50

void FUN_0040bc50(int param_1)

{
  (&DAT_005109a0)[param_1] = (&DAT_005109a0)[param_1] + 1;
  (&DAT_00510da0)[param_1] = (&DAT_00510da0)[param_1] + 1;
  return;
}


// ==== FUN_0040bc20 @ 0040bc20

void FUN_0040bc20(int param_1)

{
  (&DAT_005111a0)[param_1] = (&DAT_005111a0)[param_1] + 1;
  return;
}


// ==== FUN_0041eac0 @ 0041eac0

void FUN_0041eac0(void)

{
  int iVar1;
  int iVar2;
  
  if ((DAT_005134c8 != 0) &&
     (iVar1 = FUN_004eb4b0(DAT_00549b38,DAT_00549b3c,DAT_005134c8), iVar1 != 0)) {
    iVar2 = FUN_0041ea30();
    if ((iVar2 != 0) && (iVar2 = FUN_0041ea30(), iVar2 != iVar1)) {
      return;
    }
    FUN_0041eda0(iVar1,0,1);
    if ((*(byte *)(DAT_005395dc + iVar1 * 0x24 + 0x23) & 9) == 0) {
      FUN_0041eab0();
    }
  }
  return;
}


// ==== FUN_004071f0 @ 004071f0

void FUN_004071f0(int param_1,undefined4 param_2)

{
  int iVar1;
  int *piVar2;
  
  if (param_1 != 0) {
    piVar2 = (int *)(DAT_0050f834 + param_1 * 0xac);
    if (piVar2 == (int *)0x0) {
      FUN_00490040(&DAT_0050fb04,s__ns_o_Player_cpp_0050faf0,0x2c3);
    }
    else if (piVar2[2] < piVar2[1]) {
      iVar1 = piVar2[2] + 1;
      piVar2[2] = iVar1;
      *(undefined4 *)(*piVar2 + -4 + iVar1 * 4) = param_2;
    }
  }
  return;
}


// ==== FUN_0045ec90 @ 0045ec90

void FUN_0045ec90(undefined4 param_1)

{
  if (DAT_00566fe8 < DAT_00566fe4) {
    *(undefined4 *)(DAT_00566fe0 + DAT_00566fe8 * 4) = param_1;
    DAT_00566fe8 = DAT_00566fe8 + 1;
  }
  DAT_00520c10 = DAT_00520c10 + 1;
  return;
}


// ==== FUN_0048a360 @ 0048a360

void FUN_0048a360(int param_1)

{
  bool bVar1;
  int iVar2;
  int *piVar3;
  
  iVar2 = 0;
  piVar3 = DAT_00567648;
  if (0 < DAT_00567650) {
    do {
      if (*piVar3 == param_1) {
        bVar1 = true;
        goto LAB_0048a393;
      }
      iVar2 = iVar2 + 1;
      piVar3 = piVar3 + 1;
    } while (iVar2 < DAT_00567650);
  }
  bVar1 = false;
LAB_0048a393:
  if ((!bVar1) && (DAT_00567650 < DAT_0056764c)) {
    DAT_00567648[DAT_00567650] = param_1;
    DAT_00567650 = DAT_00567650 + 1;
  }
  DAT_00520c10 = DAT_00520c10 + 1;
  return;
}


// ==== FUN_004d8230 @ 004d8230

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d8230(undefined4 param_1,int param_2,int param_3)

{
  int *piVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  float10 fVar9;
  undefined4 local_10 [2];
  byte *local_8;
  int local_4;
  
  iVar3 = FUN_004d81f0(param_1);
  if (iVar3 != 0) {
    iVar4 = DAT_005395dc + param_2 * 0x24;
    if (param_3 == 0) {
      piVar1 = (int *)(iVar3 + 0x6d4 + (uint)*(byte *)(iVar4 + 10) * 4);
      *piVar1 = *piVar1 + -1;
    }
    else {
      piVar1 = (int *)(iVar3 + 0x6d4 + (uint)*(byte *)(iVar4 + 10) * 4);
      *piVar1 = *piVar1 + 1;
    }
    local_8 = (byte *)(iVar4 + 10);
    uVar5 = FUN_004abae0();
    if (param_3 == 0) {
      if ((uVar5 & 0x200000) != 0) {
        if (*(int *)(iVar3 + 0x2a0) == param_2) {
          *(undefined4 *)(iVar3 + 0x2a0) = 0;
        }
        if (*(int *)(iVar3 + 0x29c) == param_2) {
          *(undefined4 *)(iVar3 + 0x29c) = 0;
        }
      }
      if ((uVar5 & 0x30000) != 0) {
        FUN_004e37e0(param_2,&local_4,local_10);
        if (local_4 != 0) {
          FUN_004e3320(local_4);
        }
        iVar6 = FUN_004dd400(param_2);
        if (iVar6 != 0) {
          FUN_004def00(param_2);
          *(double *)(iVar3 + 0x5e0) = _DAT_005484a8 + _DAT_00506cb0;
        }
      }
      if (((uVar5 & 0x200) != 0) && (*(int *)(iVar3 + 0x294) == param_2)) {
        *(undefined4 *)(iVar3 + 0x294) = 0;
      }
      FUN_004def00(param_2);
      FUN_004def00(param_2);
      if (*(int *)(iVar3 + 0x3f4) == param_2) {
        *(undefined4 *)(iVar3 + 0x3f4) = 0;
      }
      if (*(int *)(iVar3 + 0x3f8) == param_2) {
        *(undefined4 *)(iVar3 + 0x3f8) = 0;
      }
    }
    if ((uVar5 & 0x400000) != 0) {
      if (param_3 == 0) {
        if (*(int *)(iVar3 + 0x2a4) == param_2) {
          *(undefined4 *)(iVar3 + 0x2a4) = 0;
        }
      }
      else {
        *(int *)(iVar3 + 0x2a4) = param_2;
      }
    }
    if ((uVar5 & 0x4000000) != 0) {
      if (param_3 == 0) {
        FUN_004def00(param_2);
      }
      else {
        bVar2 = false;
        if ((*(int *)(iVar3 + 0x294) != 0) &&
           (fVar9 = (float10)FUN_004e33e0(param_2,*(int *)(iVar3 + 0x294)),
           fVar9 < (float10)_DAT_00506cb8)) {
          bVar2 = true;
        }
        iVar6 = 0;
        if (0 < *(int *)(iVar3 + 0x308)) {
          iVar8 = 0;
          do {
            fVar9 = (float10)FUN_004e33e0(param_2,*(undefined4 *)(*(int *)(iVar3 + 0x300) + iVar8));
            if (fVar9 < (float10)_DAT_00506cb8) {
              bVar2 = true;
            }
            iVar8 = iVar8 + 4;
            iVar6 = iVar6 + 1;
          } while (iVar6 < *(int *)(iVar3 + 0x308));
        }
        if (bVar2) {
          FUN_004e32e0(param_2);
        }
      }
    }
    if (((uVar5 & 0x30000) != 0) && ((uVar5 & 0x200000) == 0)) {
      FUN_004d72d0(iVar3 + 0x2dc,param_2,param_3);
      local_10[0] = *(undefined4 *)(iVar4 + 0xe);
      uVar7 = __ftol();
      uVar7 = __ftol(uVar7);
      iVar4 = FUN_004abf50(uVar7);
      FUN_004dabe0(iVar3,DAT_00540c24,s_Built__s_at___d__d__005410fc,iVar4 + 4);
    }
    if ((uVar5 & 0x800) != 0) {
      FUN_004d72d0(iVar3 + 0x2f4,param_2,param_3);
      *(undefined4 *)(iVar3 + 0x5b8) = 0;
      *(undefined4 *)(iVar3 + 0x5bc) = 0;
    }
    if (((uVar5 & 0x4000) != 0) && (*local_8 != DAT_0051cb3c)) {
      FUN_004d72d0(iVar3 + 0x300,param_2,param_3);
      iVar4 = 0;
      FUN_004d72d0(iVar3 + 0x30c,param_2,param_3);
      if (param_3 == 0) {
        FUN_004e32a0();
        FUN_004d72d0(iVar3 + 0x318,param_2,0);
        if (0 < *(int *)(iVar3 + 0x308)) {
          iVar6 = 0;
          do {
            uVar7 = *(undefined4 *)(*(int *)(iVar3 + 0x300) + iVar6);
            iVar8 = FUN_004dd400(uVar7);
            if (iVar8 == 0) {
              FUN_004e2930(uVar7);
            }
            iVar6 = iVar6 + 4;
            iVar4 = iVar4 + 1;
          } while (iVar4 < *(int *)(iVar3 + 0x308));
        }
      }
      *(uint *)(iVar3 + 0x2c8) = (uint)(*(int *)(iVar3 + 0x3b0) <= *(int *)(iVar3 + 0x308));
    }
    if ((uVar5 & 0x40800) != 0) {
      if (param_3 == 0) {
        iVar4 = FUN_004dd400(param_2);
        if (iVar4 != 0) {
          FUN_004def00(param_2);
        }
      }
      else {
        if (*(int *)(iVar3 + 0x2ec) <= *(int *)(iVar3 + 0x2f0)) {
          FUN_004def00(**(undefined4 **)(iVar3 + 0x2e8));
        }
        FUN_004e2930(param_2);
        *(int *)(iVar3 + 0x3c4) = *(int *)(iVar3 + 0x3c4) + 3;
      }
    }
    if ((param_3 != 0) && ((uVar5 & 0x4200) != 0)) {
      *(undefined4 *)(iVar3 + 0x5c0) = 0;
      *(undefined4 *)(iVar3 + 0x5c4) = 0;
    }
  }
  return;
}


// ==== FUN_004d81f0 @ 004d81f0

int FUN_004d81f0(int param_1)

{
  int iVar1;
  
  if ((((DAT_00540a24 != 0) || (param_1 < 1)) || (iVar1 = DAT_0050f834 + param_1 * 0xac, iVar1 == 0)
      ) || (iVar1 = *(int *)(iVar1 + 0xa8), iVar1 == 0)) {
    iVar1 = 0;
  }
  return iVar1;
}


// ==== FUN_004015d0 @ 004015d0

undefined4 FUN_004015d0(void)

{
  return DAT_0050f6c8;
}


// ==== FUN_004147a0 @ 004147a0

float10 FUN_004147a0(int param_1)

{
  return (float10)*(float *)(param_1 * 0x1d4 + 0xc4 + DAT_0051c960);
}


// ==== FUN_004adbd0 @ 004adbd0

void __thiscall FUN_004adbd0(int param_1,undefined1 param_2)

{
  *(undefined1 *)(param_1 + 0x1c) = param_2;
  return;
}


// ==== FUN_004abab0 @ 004abab0

void FUN_004abab0(void)

{
  DAT_00539600 = DAT_00539600 + -1;
  return;
}


