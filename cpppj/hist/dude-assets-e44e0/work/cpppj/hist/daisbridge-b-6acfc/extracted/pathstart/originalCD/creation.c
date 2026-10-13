// ==== FUN_0047d150 @ 0047d150

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * __cdecl FUN_0047d150(int param_1,float param_2,float param_3)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  undefined4 uVar5;
  float *pfVar6;
  float unaff_EBX;
  uint uVar7;
  int *piVar8;
  int unaff_retaddr;
  float local_1c;
  undefined4 uStack_18;
  
  if (param_2 <= _DAT_00503ca8) {
    param_2 = _DAT_00503ca8;
  }
  if (param_3 <= _DAT_00503ca8) {
    param_3 = _DAT_00503ca8;
  }
  if (_DAT_00503cac <= param_2) {
    param_2 = _DAT_00503cac;
  }
  if (_DAT_00503cac <= param_3) {
    param_3 = _DAT_00503cac;
  }
  piVar8 = (int *)(DAT_005395dc + param_1 * 0x24);
  uVar7 = (uint)*(byte *)((int)piVar8 + 10);
  FUN_004abae0(piVar8);
  iVar2 = FUN_0047d020(uVar7);
  bVar1 = false;
  if (((DAT_0051ca5c == uVar7) || (DAT_0051cae8 == uVar7)) || (DAT_0051cb5c == uVar7)) {
    bVar1 = true;
  }
  else if ((DAT_0052da98 != 0) && (uVar7 == DAT_0051cbbc)) {
    FUN_00440a80(&param_2);
    param_2 = param_2 - _DAT_00503ca4;
    param_3 = param_3 - _DAT_00503ca4;
  }
  local_1c = 0.0;
  if (DAT_00540a2c != 0) {
    iVar3 = FUN_004eb310(param_1,iVar2,0,0);
    if (iVar3 != 0) {
      iVar3 = FUN_004af4e0(iVar3 * 0x24 + DAT_005395dc);
      local_1c = *(float *)(iVar3 + 0x2f10);
      FUN_0048f7b0(iVar3,0);
    }
  }
  if (iVar2 == 0) {
    FUN_00490040(s_processType_0052dae4,s__Ns_o_pathprocess_cpp_0052dacc,0xb3);
  }
  iVar3 = 0;
  piVar4 = (int *)(**(code **)(iVar2 * 0x1d4 + 0x1b0 + DAT_0051c960))(0,0);
  iVar2 = *(int *)((int)piVar8 + 0x12);
  (**(code **)(*piVar4 + 0x20))(param_1,param_2,param_3,*(float *)((int)piVar8 + 0xe));
  piVar4[0xbc2] = 0;
  piVar4[0xbc3] = (int)unaff_EBX;
  piVar4[0xbc4] = iVar2;
  piVar4[0xbc5] = unaff_retaddr;
  piVar4[0xbc7] = param_1;
  piVar4[0xbc8] = (int)param_2;
  if (piVar4[4] < 1) {
    if (piVar4 != (int *)0x0) {
      (**(code **)*piVar4)(1);
    }
    return (int *)0x0;
  }
  if (bVar1) {
    unaff_EBX = *(float *)((int)piVar8 + 0xe);
    local_1c = *(float *)((int)piVar8 + 0x12);
    FUN_00440a80(&stack0xffffffe0);
    if (0 < piVar4[4]) {
      pfVar6 = (float *)(piVar4 + 5);
      do {
        if ((*pfVar6 != unaff_EBX) || (pfVar6[1] != local_1c)) break;
        *pfVar6 = unaff_EBX;
        iVar3 = iVar3 + 1;
        pfVar6[1] = local_1c;
        pfVar6 = pfVar6 + 2;
      } while (iVar3 < piVar4[4]);
    }
  }
  uVar5 = FUN_004abf50(piVar8);
  (**(code **)(*piVar4 + 0x30))(0,uVar5);
  (**(code **)(*piVar4 + 0x2c))(uStack_18,0,unaff_EBX);
  (**(code **)(*piVar8 + 0x44))(unaff_EBX,local_1c,piVar4);
  if (piVar4 != (int *)0x0) {
    (**(code **)(*piVar8 + 0x40))();
  }
  return piVar4;
}


// ==== FUN_0047d020 @ 0047d020

undefined4 __cdecl FUN_0047d020(int param_1)

{
  if (param_1 == DAT_0051cb58) {
    return DAT_0051ca00;
  }
  if ((param_1 == DAT_0051cb68) || (param_1 == DAT_0051cbb4)) {
    return DAT_0051c9f8;
  }
  if ((*(uint *)(param_1 * 0x1d4 + 0xec + DAT_0051c960) & 0x20000) != 0) {
    return DAT_0051ca08;
  }
  if (DAT_0051cb5c == param_1) {
    return DAT_0051c9bc;
  }
  if (DAT_0051cb60 == param_1) {
    return DAT_0051ca04;
  }
  if (DAT_0051ca5c == param_1) {
    return DAT_0051c9c0;
  }
  if (param_1 == DAT_0051cbbc) {
    return DAT_0051ca38;
  }
  if (param_1 == DAT_0051cae8) {
    return DAT_0051c9fc;
  }
  FUN_00490040(&DAT_0052dac8,s__Ns_o_pathprocess_cpp_0052dab0,0x79);
  return 0;
}


