// ==== FUN_004e43b0 @ 004e43b0

void FUN_004e43b0(undefined4 param_1)

{
  int iVar1;
  
  FUN_004615e0(param_1);
  if (DAT_00540a2c != 0) {
    iVar1 = FUN_004ac560(1,0,0);
    if (iVar1 != 0) {
      FUN_004151d0();
    }
  }
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


// ==== FUN_004615e0 @ 004615e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004615e0(int param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  float *pfVar5;
  int *piVar6;
  uint uVar7;
  int local_20;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  undefined1 local_8 [8];
  
  if (((*(byte *)(param_1 + 0x23) & 1) == 0) && ((*(byte *)(param_1 + 0x23) & 8) == 0)) {
    if ((param_2 & 0x200000) != 0) {
      FUN_00454800((param_1 - DAT_005395dc) / 0x24);
      FUN_00438c00(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),
                   s_Acollapse_wav_00520fe7 + 1,0);
    }
    if ((param_2 & 0x100000) != 0) {
      FUN_004547d0((param_1 - DAT_005395dc) / 0x24);
      FUN_00438c00(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),
                   s_explosion_wav_00520ff8,0);
    }
    iVar2 = __ftol();
    iVar3 = __ftol();
    if ((*(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48) & 6) != 0) {
      FUN_004eb8d0((param_1 - DAT_005395dc) / 0x24);
      while (local_20 != 0) {
        if (*(ushort *)(local_20 * 0x24 + 0x12 + DAT_005395dc) == DAT_0051cbbc) {
          FUN_00438b50(s_priestFree_wav_00521008,0,0,0,1,0);
        }
        FUN_004eb900();
      }
    }
    if ((DAT_00540a2c != 0) && ((param_2 & 0x800) == 0)) {
      iVar2 = FUN_004abf50();
      if (*(int *)(iVar2 + 0x1b4) == 1) {
        local_18 = *(float *)(param_1 + 0xe);
        pfVar5 = &local_18;
        local_14 = *(float *)(param_1 + 0x12);
      }
      else {
        pfVar5 = (float *)FUN_004aef30(local_8);
      }
      local_10 = *pfVar5;
      local_c = pfVar5[1];
      FUN_00440a80();
      FUN_004eb8d0((param_1 - DAT_005395dc) / 0x24);
      while (local_20 != 0) {
        iVar2 = DAT_005395dc + local_20 * 0x24;
        if (((*(byte *)(iVar2 + 0x14) & 0x20) != 0) &&
           ((uVar7 = (uint)*(ushort *)(iVar2 + 0x12), DAT_00540a20 != 0 ||
            ((*(uint *)(uVar7 * 0x1d4 + 0xec + DAT_0051c960) & DAT_005178c4) != 0)))) {
          if ((*(byte *)(uVar7 * 0x1d4 + 0xee + DAT_0051c960) & 0x20) == 0) {
            iVar2 = __ftol();
            iVar3 = __ftol();
            bVar1 = *(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48);
            if (((bVar1 & 0x10) == 0) && ((bVar1 & 6) != 0)) {
              piVar6 = (int *)FUN_004ab390(uVar7,0);
              FUN_004abb00(*(undefined2 *)(local_20 * 0x24 + 0xc + DAT_005395dc));
              (**(code **)(*piVar6 + 0x74))(0);
              (**(code **)(*piVar6 + 0x90))(local_14,local_10,0);
            }
          }
          else {
            local_18 = *(float *)(param_1 + 0xe);
            local_14 = *(float *)(param_1 + 0x12);
            uVar4 = FUN_004abae0();
            if ((uVar4 & 0x80400000) != 0) {
              pfVar5 = (float *)FUN_004aef30(local_8);
              local_18 = *pfVar5 + _DAT_00502b6c;
              local_14 = pfVar5[1] + _DAT_00502b6c;
            }
            FUN_00461430(local_18,local_14,uVar7,
                         *(undefined2 *)(local_20 * 0x24 + 0xc + DAT_005395dc));
          }
        }
        FUN_004eb900();
      }
    }
  }
  FUN_004add20(param_2);
  return;
}


// ==== FUN_004151d0 @ 004151d0

void FUN_004151d0(void)

{
  undefined4 *puVar1;
  int *piVar2;
  undefined2 extraout_var;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_1c;
  float local_18;
  float local_14;
  float local_10;
  int local_c;
  undefined1 local_8 [8];
  
  if ((((DAT_00540a2c != 0) && (DAT_0052e170 == 0)) && (DAT_00540a1c == 0)) && (DAT_00518904 == 0))
  {
    puVar1 = (undefined4 *)FUN_004047c0(local_8,0);
    piVar2 = (int *)FUN_00403df0(&local_2c,*puVar1,
                                 CONCAT22(extraout_var,*(undefined2 *)(puVar1 + 1)));
    local_c = piVar2[1] << 4;
    local_18 = (float)local_c;
    local_14 = (float)(piVar2[2] << 4);
    local_10 = (float)(piVar2[3] << 4);
    local_28 = (float)((1 - DAT_00537188) * 0x10);
    local_1c = DAT_00537188 << 4;
    local_2c = (float)(*piVar2 << 4) + local_28;
    local_28 = local_28 + local_18;
    local_24 = local_14 + (float)local_1c;
    local_20 = (float)local_1c + local_10;
    FUN_0042c930(local_2c,local_28,local_24,local_20,1);
  }
  return;
}


