// ==== FUN_00489080 @ 00489080

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * __cdecl FUN_00489080(int param_1,float param_2,float param_3)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  int iVar5;
  int *piVar6;
  undefined4 uVar7;
  undefined4 *puVar8;
  float *pfVar9;
  int iVar10;
  int iVar11;
  float unaff_ESI;
  uint uVar12;
  int unaff_retaddr;
  float local_64;
  float fStack_60;
  undefined4 local_5c;
  undefined4 local_54;
  float local_50;
  float local_4c;
  int iStack_20;
  undefined4 uStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  int iStack_c;
  int iStack_8;
  int iStack_4;
  
  if (param_2 <= _DAT_00500fc4) {
    param_2 = 0.0;
  }
  if (param_3 <= _DAT_00500fc4) {
    param_3 = 0.0;
  }
  if (_DAT_00503b50 <= param_2) {
    param_2 = 254.0;
  }
  if (_DAT_00503b50 <= param_3) {
    param_3 = 254.0;
  }
  iVar11 = param_1 * 0x32;
  uVar12 = (uint)*(byte *)(DAT_005c8464 + 10 + iVar11);
  piVar1 = (int *)(DAT_005c8464 + iVar11);
  FUN_004ac200(piVar1);
  local_64 = (float)FUN_00488fe0(uVar12);
  local_54 = 0;
  if (((uVar12 == DAT_00541170) || (uVar12 == DAT_005411fc)) || (uVar12 == DAT_00541270)) {
    local_54 = 0x10;
  }
  else if ((DAT_00595180 != 0) && (uVar12 == DAT_005412d0)) {
    FUN_0041d7a0(&param_2);
    param_2 = param_2 - _DAT_00500ee4;
    param_3 = param_3 - _DAT_00500ee4;
  }
  local_50 = *(float *)((int)piVar1 + 0xe);
  local_4c = *(float *)((int)piVar1 + 0x12);
  iVar10 = 0;
  local_5c = 0;
  if (DAT_00540bc4 != 0) {
    iVar5 = FUN_004b1a90(param_1,local_64,0,0);
    if (iVar5 != 0) {
      if (local_50 < param_2 != (local_50 == param_2)) {
        local_50 = local_50 - (float)_DAT_00503350;
      }
      if (local_4c < param_3 != (local_4c == param_3)) {
        local_4c = local_4c - (float)_DAT_00503350;
      }
      iVar5 = FUN_004ae2d0(iVar5 * 0x32 + DAT_005c8464);
      local_5c = *(undefined4 *)(iVar5 + 0x186d0);
      iVar10 = *(int *)(iVar5 + 0x186cc);
      FUN_0041bf10(iVar5,0);
    }
  }
  if (local_64 == 0.0) {
    FUN_004e0620("processType","\\Ns\\O\\Pathprocess.cpp",0xe5);
  }
  iVar5 = FUN_0049a840(local_64);
  piVar6 = (int *)(**(code **)(iVar5 + 0x1d0))(0,0);
  (**(code **)(*piVar6 + 0x20))(param_1,param_2,param_3,local_50,local_4c);
  piVar6[0x61b4] = (int)unaff_ESI;
  piVar6[0x61b9] = param_1;
  piVar6[0x61b2] = 0;
  piVar6[0x61b3] = iVar10;
  piVar6[0x61b5] = unaff_retaddr;
  piVar6[0x61ba] = (int)param_2;
  piVar6[0x61b7] = iStack_c;
  piVar6[0x61b8] = iStack_8;
  if (piVar6[4] < 1) {
    iVar11 = DAT_005c8464 + iVar11;
    puVar8 = (undefined4 *)FUN_004ade70(iVar11,&local_64,0);
    FUN_0041def0(&local_54,*puVar8,puVar8[1],puVar8[2],puVar8[3],1);
    bVar4 = false;
    while( true ) {
      uVar12 = iStack_20 * 0x32 + DAT_005c8464;
      if ((uVar12 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar12))
      break;
      uVar12 = FUN_004ac200();
      if ((uVar12 & 0x8c044200) != 0) {
        FUN_00443800(*(undefined4 *)(iVar11 + 0xe),*(undefined4 *)(iVar11 + 0x12),
                     *(undefined1 *)(iVar11 + 10),*(undefined1 *)(iVar11 + 0x22));
        bVar4 = true;
      }
      FUN_004b1810(&local_54);
    }
    (**(code **)*piVar6)(1);
    if (DAT_005951b0 != 0) {
      DAT_005951b0 = 0;
      return (int *)0x0;
    }
    if (bVar4) {
      DAT_005951b0 = 1;
      FUN_00489080(uStack_14,uStack_10,iStack_c,iStack_8,iStack_4,unaff_retaddr,param_1,param_2);
    }
    return (int *)0x0;
  }
  if (iStack_4 != 0) {
    fVar2 = *(float *)((int)piVar1 + 0xe);
    fVar3 = *(float *)((int)piVar1 + 0x12);
    FUN_0041d7a0(&stack0xffffff90);
    iVar11 = 0;
    unaff_ESI = fVar2;
    if (0 < piVar6[4]) {
      pfVar9 = (float *)(piVar6 + 5);
      do {
        if ((*pfVar9 != fVar2) || (pfVar9[1] != fVar3)) break;
        *pfVar9 = local_64;
        pfVar9[1] = fStack_60;
        iVar11 = iVar11 + 1;
        pfVar9 = pfVar9 + 2;
        unaff_ESI = local_64;
      } while (iVar11 < piVar6[4]);
    }
  }
  iVar11 = *piVar6;
  uVar7 = FUN_004ac550(piVar1);
  (**(code **)(iVar11 + 0x30))(0,uVar7);
  (**(code **)(*piVar6 + 0x2c))(uStack_18,0,unaff_ESI);
  (**(code **)(*piVar1 + 0x44))(iStack_20,uStack_1c,piVar6);
  (**(code **)(*piVar1 + 0x40))();
  return piVar6;
}


// ==== FUN_00488fe0 @ 00488fe0

undefined4 __cdecl FUN_00488fe0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(param_1);
  if ((param_1 != DAT_0054126c) && (param_1 != DAT_00541344)) {
    if ((param_1 == DAT_0054127c) || (param_1 == DAT_005412c8)) {
      return DAT_00541108;
    }
    if ((*(uint *)(iVar1 + 0xec) & 0x20000) != 0) {
      return DAT_00541118;
    }
    if (param_1 == DAT_00541270) {
      return DAT_005410cc;
    }
    if (param_1 == DAT_00541274) {
      return DAT_00541114;
    }
    if (param_1 == DAT_00541170) {
      return DAT_005410d0;
    }
    if (param_1 == DAT_005412d0) {
      return DAT_00541148;
    }
    if (param_1 == DAT_005411fc) {
      return DAT_0054110c;
    }
  }
  return DAT_00541110;
}


