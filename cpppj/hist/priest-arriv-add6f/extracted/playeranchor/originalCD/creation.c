// ==== FUN_00406f80 @ 00406f80

undefined4 FUN_00406f80(uint param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int *piVar5;
  int iVar6;
  int iVar7;
  undefined4 *unaff_FS_OFFSET;
  int local_2c;
  undefined4 *local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  uStack_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004071c6;
  *unaff_FS_OFFSET = &uStack_10;
  piVar5 = (int *)(param_1 * 0xac + DAT_0050f834);
  if (piVar5 == (int *)0x0) {
    FUN_00490040(&DAT_0050faec,s__ns_o_Player_cpp_0050fad8,0x281);
  }
  local_14 = FUN_0045b670(param_3,param_4);
  if (DAT_005207f4 == local_14) {
    local_14 = 0;
  }
  else {
    iVar7 = 0;
    local_20 = (undefined4 *)FUN_004f1650(0xa0);
    local_18 = 0;
    bVar1 = false;
    local_1c = 0x28;
    local_8 = 1;
    for (local_2c = 0; local_2c < piVar5[2]; local_2c = local_2c + 1) {
      iVar6 = DAT_005395dc + *(int *)(*piVar5 + iVar7) * 0x24;
      if ((*(byte *)(param_2 * 0x1d4 + 0xec + DAT_0051c960) & 6) == 0) {
        iVar2 = FUN_004ac560(0x10,param_2,0);
        if (iVar2 != 0) {
          bVar1 = true;
          goto LAB_00407076;
        }
LAB_0040707a:
        if (DAT_0052e8a8 != 0) goto LAB_0040708c;
      }
      else {
        iVar2 = 1;
LAB_00407076:
        if (iVar2 == 0) goto LAB_0040707a;
LAB_0040708c:
        iVar2 = FUN_0045b6f0((iVar6 - DAT_005395dc) / 0x24);
        if ((iVar2 == local_14) && (local_18 < local_1c)) {
          local_20[local_18] = (iVar6 - DAT_005395dc) / 0x24;
          local_18 = local_18 + 1;
        }
      }
      iVar7 = iVar7 + 4;
    }
    if ((local_18 != 0) || (bVar1)) {
      iVar6 = 0;
      iVar7 = 0;
      if (0 < DAT_005670e8) {
        do {
          iVar2 = DAT_005395dc + *(int *)(DAT_005670e0 + iVar6) * 0x24;
          if (((*(byte *)(iVar2 + 0x20) == param_1) &&
              (iVar3 = FUN_0045b6f0((iVar2 - DAT_005395dc) / 0x24), iVar3 == local_14)) &&
             (local_18 < local_1c)) {
            local_20[local_18] = (iVar2 - DAT_005395dc) / 0x24;
            local_18 = local_18 + 1;
          }
          iVar6 = iVar6 + 4;
          iVar7 = iVar7 + 1;
        } while (iVar7 < DAT_005670e8);
      }
    }
    if (local_18 == 0) {
      local_8 = 0xffffffff;
      FUN_004071d0();
      uVar4 = FUN_004071d8();
      return uVar4;
    }
    if (local_18 == 1) {
      local_8 = 0xffffffff;
      local_14 = *local_20;
      FUN_004071d0();
    }
    else {
      local_14 = FUN_00406ef0(param_3,param_4,&local_20,param_1);
      local_8 = 0xffffffff;
      FUN_004071d0();
    }
  }
  *unaff_FS_OFFSET = uStack_10;
  return local_14;
}


// ==== FUN_00406ef0 @ 00406ef0

undefined4 FUN_00406ef0(undefined4 param_1,undefined4 param_2,int *param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  float10 fVar6;
  float local_10;
  
  local_10 = 9999.0;
  iVar3 = 0;
  uVar4 = 0;
  if (0 < param_3[2]) {
    iVar5 = 0;
    iVar2 = DAT_005395dc;
    do {
      iVar1 = *(int *)(*param_3 + iVar5) * 0x24 + iVar2;
      if (*(byte *)(iVar1 + 0x20) == param_4) {
        fVar6 = (float10)FUN_0043f750(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12));
        iVar2 = DAT_005395dc;
        if (fVar6 < (float10)local_10) {
          uVar4 = *(undefined4 *)(*param_3 + iVar5);
          local_10 = (float)fVar6;
        }
      }
      iVar5 = iVar5 + 4;
      iVar3 = iVar3 + 1;
    } while (iVar3 < param_3[2]);
  }
  return uVar4;
}


// ==== FUN_0045b670 @ 0045b670

undefined4 FUN_0045b670(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  short sVar4;
  
  if (DAT_005207f8 == 0) {
    return 0xfe;
  }
  iVar1 = __ftol();
  iVar2 = __ftol();
  if ((((iVar1 < 0) || (iVar2 < 0)) || (0xff < iVar1)) || (0xff < iVar2)) {
    sVar4 = 0;
  }
  else {
    sVar4 = *(short *)(DAT_005670cc + (iVar2 * 0x100 + iVar1) * 2);
  }
  if (sVar4 == 0) {
    return 0xfe;
  }
  uVar3 = FUN_004adbc0();
  return uVar3;
}


// ==== FUN_0045b6f0 @ 0045b6f0

undefined4 FUN_0045b6f0(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  
  if (param_1 == 0) {
    return 0;
  }
  iVar3 = DAT_005395dc + param_1 * 0x24;
  uVar1 = FUN_004abac0();
  if ((uVar1 & 0x800) != 0) {
    uVar2 = FUN_004adbc0();
    return uVar2;
  }
  uVar2 = FUN_0045b670(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12));
  return uVar2;
}


// ==== FUN_004071d0 @ 004071d0

void FUN_004071d0(void)

{
  FUN_00407c50();
  return;
}


// ==== FUN_004071d8 @ 004071d8

undefined4 FUN_004071d8(void)

{
  int unaff_EBP;
  undefined4 *unaff_FS_OFFSET;
  
  *unaff_FS_OFFSET = *(undefined4 *)(unaff_EBP + -0xc);
  return 0;
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


// ==== FUN_0043f750 @ 0043f750

float10 __thiscall FUN_0043f750(float *param_1,float param_2,float param_3)

{
  return SQRT(((float10)*param_1 - (float10)param_2) * ((float10)*param_1 - (float10)param_2) +
              ((float10)param_1[1] - (float10)param_3) * ((float10)param_1[1] - (float10)param_3));
}


// ==== FUN_004adbc0 @ 004adbc0

undefined1 __fastcall FUN_004adbc0(int param_1)

{
  return *(undefined1 *)(param_1 + 0x1c);
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __fastcall FUN_004abac0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_00407c50 @ 00407c50

void __fastcall FUN_00407c50(undefined4 param_1)

{
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puStack_c = &LAB_00407c87;
  *unaff_FS_OFFSET = &local_10;
  local_8 = 0xffffffff;
  FUN_00407c91(param_1);
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004ac560 @ 004ac560

int __thiscall FUN_004ac560(int param_1,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  int local_8;
  
  FUN_004eb8d0((param_1 - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(local_8 * 0x24 + 0x12 + DAT_005395dc) != param_3)) {
      bVar1 = false;
    }
    if ((param_2 != 0) && ((param_2 & *(ushort *)(local_8 * 0x24 + 0x14 + DAT_005395dc)) == 0)) {
      bVar1 = false;
    }
    if (bVar1) {
      if (param_4 == 0) {
        return (local_8 * 0x24) / 0x24;
      }
      param_4 = param_4 + -1;
    }
    FUN_004eb900();
  } while( true );
}


// ==== FUN_004eb8d0 @ 004eb8d0

void __thiscall FUN_004eb8d0(int param_1,int param_2)

{
  *(uint *)(param_1 + 4) = (uint)*(ushort *)(param_2 * 0x24 + 6 + DAT_005395dc);
  FUN_004eb900();
  return;
}


// ==== FUN_004eb900 @ 004eb900

void __fastcall FUN_004eb900(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  
  iVar2 = DAT_005395dc;
  do {
    do {
      iVar1 = param_1[1];
      param_1[2] = iVar1;
      if (iVar1 == 0) {
        param_1[1] = 0;
        param_1[2] = 0;
        return;
      }
      iVar1 = iVar1 * 0x24 + iVar2;
      param_1[1] = (uint)*(ushort *)(iVar1 + 4);
    } while ((iVar1 != 0) && (*(byte *)(iVar1 + 10) != DAT_0051c978));
    iVar1 = (**(code **)*param_1)(iVar1);
    iVar2 = DAT_005395dc;
  } while (iVar1 == 0);
  return;
}


// ==== FUN_0040f230 @ 0040f230

undefined4 FUN_0040f230(void)

{
  return 1;
}


