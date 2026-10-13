// ==== FUN_004bf340 @ 004bf340

void __thiscall FUN_004bf340(void *this)

{
  undefined4 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb888;
  local_c = ExceptionList;
  if ((DAT_00594fbc != 0) && (DAT_005c85d4 == 0)) {
    DAT_005c89c4 = 99;
  }
  DAT_005c89b8 = DAT_005c89b8 + 1;
  local_4 = 0;
  ExceptionList = &local_c;
  if (*(int *)this == 0) {
    ExceptionList = &local_c;
    FUN_004e0620(&DAT_0051476c,"\\Ns\\O\\Template.cpp",0x506);
  }
  uVar1 = FUN_0044ceb0(*(undefined4 *)this,"Money",0);
  FUN_00483300(uVar1);
  DAT_005c89b8 = DAT_005c89b8 + -1;
  ExceptionList = local_c;
  return;
}


// ==== FUN_00483300 @ 00483300

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00483300(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *(int *)(param_1 + 4) + 4;
  *(int *)(param_1 + 4) = iVar1;
  if (*(int *)(param_1 + 8) < iVar1) {
    FUN_004e0620("pos<=end","c:\\ns\\o\\memstream.h",0x2e);
  }
  if (DAT_00594fb8 != 0) {
    FUN_0040edb0(_DAT_00540c70,0x483d9c80);
  }
  if ((DAT_00594fbc != 0) && (DAT_005c85d4 == 0)) {
    iVar1 = FUN_0041c7d0();
    iVar3 = 1;
    iVar2 = 0xb4;
    do {
      if ((*(byte *)(iVar2 + 0x50 + DAT_0059534c) & 0x20) == 0) {
        FUN_00447810(iVar3,iVar1 + 4000);
      }
      iVar2 = iVar2 + 0xb4;
      iVar3 = iVar3 + 1;
    } while (iVar2 < 0x654);
  }
  return;
}


// ==== FUN_0041c7d0 @ 0041c7d0

undefined4 __cdecl FUN_0041c7d0(void)

{
  undefined4 local_c [3];
  
  local_c[0] = 1000;
  local_c[1] = 0x9c4;
  local_c[2] = 3000;
  return local_c[DAT_0052f7ee];
}


// ==== FUN_00447810 @ 00447810

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00447810(int param_1,int param_2)

{
  float local_4;
  
  if ((DAT_00595344 != 0) && (param_1 == _DAT_00540c70)) {
    local_4 = (float)param_2;
    if (DAT_005c85a4 != 0) {
      local_4 = 194162.0;
    }
    FUN_0040edb0(_DAT_00540c70,local_4);
    FUN_0043dad0();
    return;
  }
  if ((0 < param_1) && (param_1 < 9)) {
    FUN_0040edb0();
    return;
  }
  return;
}


// ==== FUN_0040edb0 @ 0040edb0

void __cdecl FUN_0040edb0(int param_1,int param_2)

{
  uint *puVar1;
  byte bVar2;
  uint uVar3;
  byte bVar4;
  short *psVar5;
  int iVar6;
  int local_8;
  
  if (DAT_0054db24 == 0) {
    FUN_0040eba0();
  }
  local_8 = DAT_00568af8;
  bVar4 = 0;
  psVar5 = &DAT_00531890;
  do {
    uVar3 = 1 << ((byte)local_8 & 0x1f);
    puVar1 = (uint *)((&DAT_00594fd0)[*psVar5] + param_1 * 4);
    if ((param_2 >> (bVar4 & 0x1f) & 1U) == 0) {
      *puVar1 = *puVar1 & ~uVar3;
    }
    else {
      *puVar1 = *puVar1 | uVar3;
    }
    iVar6 = 5;
    do {
      bVar2 = FUN_004558c0(0x20);
      puVar1 = (uint *)((&DAT_00594fd0)[*psVar5] + param_1 * 4);
      iVar6 = iVar6 + -1;
      *puVar1 = *puVar1 | 1 << (bVar2 & 0x1f) & ~uVar3;
    } while (iVar6 != 0);
    local_8 = local_8 + 1;
    bVar4 = bVar4 + 1;
    psVar5 = psVar5 + 1;
  } while ((int)psVar5 < 0x5318d0);
  return;
}


// ==== FUN_0040eba0 @ 0040eba0

void __cdecl FUN_0040eba0(void)

{
  undefined2 uVar1;
  undefined4 uVar2;
  int iVar3;
  void *pvVar4;
  undefined2 *puVar5;
  undefined4 *puVar6;
  
  FUN_004559d0();
  uVar2 = FUN_004558c0(100);
  iVar3 = 0;
  DAT_00568af8 = uVar2;
  do {
    (&DAT_00531890)[iVar3] = (short)iVar3;
    iVar3 = iVar3 + 1;
  } while (iVar3 < 0x20);
  puVar5 = &DAT_00531890;
  DAT_0054d4e0 = uVar2;
  do {
    iVar3 = FUN_004558c0(0x20);
    uVar1 = *puVar5;
    *puVar5 = (&DAT_00531890)[iVar3];
    puVar5 = puVar5 + 1;
    (&DAT_00531890)[iVar3] = uVar1;
  } while ((int)puVar5 < 0x5318d0);
  iVar3 = 0;
  DAT_00568af8 = uVar2;
  do {
    (&DAT_00531890)[iVar3] = (short)iVar3;
    iVar3 = iVar3 + 1;
  } while (iVar3 < 0x20);
  puVar6 = &DAT_00594fd0;
  do {
    pvVar4 = operator_new(0x24);
    *puVar6 = pvVar4;
    puVar6 = puVar6 + 1;
  } while ((int)puVar6 < 0x595050);
  DAT_0054db24 = 1;
  return;
}


// ==== FUN_004558c0 @ 004558c0

uint __cdecl FUN_004558c0(uint param_1)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % param_1;
}


// ==== FUN_0040ec50 @ 0040ec50

void __cdecl FUN_0040ec50(int param_1,uint *param_2)

{
  int iVar1;
  byte bVar2;
  byte bVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  short *psVar7;
  int iVar8;
  
  if (DAT_0054db24 == 0) {
    FUN_0040eba0();
  }
  *param_2 = 0;
  iVar8 = DAT_00568af8 + 2;
  psVar7 = &DAT_00531892;
  iVar5 = DAT_00568af8;
  iVar6 = 2;
  do {
    bVar2 = (byte)iVar6;
    if ((*(uint *)((&DAT_00594fd0)[psVar7[-1]] + param_1 * 4) & 1 << ((byte)iVar5 & 0x1f)) == 0) {
      uVar4 = *param_2 & ~(1 << (bVar2 - 2 & 0x1f));
    }
    else {
      uVar4 = *param_2 | 1 << (bVar2 - 2 & 0x1f);
    }
    *param_2 = uVar4;
    bVar3 = (byte)iVar8;
    if ((*(uint *)((&DAT_00594fd0)[*psVar7] + param_1 * 4) & 1 << (bVar3 - 1 & 0x1f)) == 0) {
      uVar4 = *param_2 & ~(1 << (bVar2 - 1 & 0x1f));
    }
    else {
      uVar4 = *param_2 | 1 << (bVar2 - 1 & 0x1f);
    }
    *param_2 = uVar4;
    if ((*(uint *)((&DAT_00594fd0)[psVar7[1]] + param_1 * 4) & 1 << (bVar3 & 0x1f)) == 0) {
      uVar4 = *param_2 & ~(1 << (bVar2 & 0x1f));
    }
    else {
      uVar4 = *param_2 | 1 << (bVar2 & 0x1f);
    }
    *param_2 = uVar4;
    if ((*(uint *)((&DAT_00594fd0)[psVar7[2]] + param_1 * 4) & 1 << (bVar3 + 1 & 0x1f)) == 0) {
      uVar4 = *param_2 & ~(1 << (bVar2 + 1 & 0x1f));
    }
    else {
      uVar4 = *param_2 | 1 << (bVar2 + 1 & 0x1f);
    }
    *param_2 = uVar4;
    iVar5 = iVar5 + 4;
    iVar1 = iVar6 + 2;
    psVar7 = psVar7 + 4;
    iVar8 = iVar8 + 4;
    iVar6 = iVar6 + 4;
  } while (iVar1 < 0x20);
  return;
}


