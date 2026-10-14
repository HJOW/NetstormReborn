// ==== FUN_004dcda0 @ 004dcda0

void __fastcall FUN_004dcda0(int *param_1)

{
  if ((short)param_1[3] == 0) {
    if ((DAT_005e1844 & 1) == 0) {
      DAT_005e1844 = DAT_005e1844 | 1;
      DAT_005e1834 = 0xc;
      DAT_005e1838 = 0x1d;
      DAT_005e183c = 0xc;
      DAT_005e1840 = 0x17;
    }
    FUN_00499240(*(undefined1 *)((int)param_1 + 10),param_1[9],*(undefined4 *)((int)param_1 + 0xe),
                 *(undefined4 *)((int)param_1 + 0x12),0,DAT_005e1834,DAT_005e1838,DAT_005e183c,
                 DAT_005e1840);
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x004dce31. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_004dcf80 @ 004dcf80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004dcf80(int *param_1)

{
  if ((short)param_1[3] == 0) {
    if ((_DAT_005e1858 & 1) == 0) {
      _DAT_005e1858 = _DAT_005e1858 | 1;
      DAT_005e1848 = 0x27;
      DAT_005e184c = 0x1e;
      DAT_005e1850 = 0x21;
      DAT_005e1854 = 0x22;
    }
    FUN_00499240(*(undefined1 *)((int)param_1 + 10),param_1[9],*(undefined4 *)((int)param_1 + 0xe),
                 *(undefined4 *)((int)param_1 + 0x12),0,DAT_005e1848,DAT_005e184c,DAT_005e1850,
                 DAT_005e1854);
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x004dd00e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== thunk_FUN_004ad500 @ 004dbcd0

void __thiscall thunk_FUN_004ad500(void *this)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  
  iVar1 = *(int *)((int)this + 0x24);
  if (DAT_005e4794 == 0) goto LAB_004ad580;
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)((int)this + 10));
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
      uVar3 = FUN_0049adc0(piVar2,iVar1,*(byte *)((int)this + 0x28) & 1);
      FUN_00436d40(uVar3);
    }
  }
  FUN_004e0620("getTypeStruct()->frameCheck( isAbstract(), theFrame, true )","\\Ns\\O\\Squid.cpp",
               0x846);
LAB_004ad580:
  piVar2 = (int *)FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((piVar2[0x3a] & 0x440000U) == 0) {
    iVar4 = piVar2[0x45];
  }
  else {
    iVar4 = piVar2[0x45] * 2;
  }
  if ((iVar1 < iVar4) && (-1 < iVar1)) {
    if ((piVar2[0x37] != 0) && ((*piVar2 != -0x22222223 && (*piVar2 != -0x32323233)))) {
      FUN_004990d0(*(undefined1 *)((int)this + 10),iVar1,*(undefined4 *)((int)this + 0xe),
                   *(undefined4 *)((int)this + 0x12),(*(byte *)((int)this + 0x28) >> 2 & 1) << 4);
    }
  }
  else if (DAT_005e4794 != 0) {
    uVar3 = FUN_0049adc0(piVar2,iVar1,*(byte *)((int)this + 0x28) & 1);
    FUN_00436d40(uVar3);
    return;
  }
  return;
}


// ==== FUN_004dd150 @ 004dd150

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004dd150(int *param_1)

{
  if ((short)param_1[3] == 0) {
    if ((_DAT_005e186c & 1) == 0) {
      _DAT_005e186c = _DAT_005e186c | 1;
      DAT_005e185c = 0x1b;
      DAT_005e1860 = 0;
      DAT_005e1864 = 0x1c;
      DAT_005e1868 = 0x1e;
    }
    FUN_00499240(*(undefined1 *)((int)param_1 + 10),param_1[9],*(undefined4 *)((int)param_1 + 0xe),
                 *(undefined4 *)((int)param_1 + 0x12),0,DAT_005e185c,DAT_005e1860,DAT_005e1864,
                 DAT_005e1868);
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x004dd1de. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


