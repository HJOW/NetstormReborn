// ==== FUN_00449660 @ 00449660

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00449660(int param_1,int param_2,byte param_3)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  
  iVar3 = param_2 * 0x32 + DAT_005c8464;
  if (((param_3 & 1) != 0) && (*(ushort *)(iVar3 + 0x12) == DAT_005412d0)) {
    iVar1 = FUN_004b1fa0(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),DAT_005412e8
                        );
    if (iVar1 != 0) {
      piVar2 = (int *)(iVar1 * 0x32 + DAT_005c8464);
      (**(code **)(*piVar2 + 0x48))(0);
      iVar1 = FUN_004ac550(piVar2);
      piVar2[9] = *(int *)(iVar1 + 0x168);
      FUN_004ac220(piVar2,*(byte *)(iVar3 + 0xc) & 0x7f);
      (**(code **)(*piVar2 + 0x4c))(0);
    }
    if (*(byte *)(param_1 + 0x22) == _DAT_00540c70) {
      FUN_00452fe0(DAT_00542a50,0,0);
    }
  }
  return;
}


