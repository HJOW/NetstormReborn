// ==== FUN_004816c0 @ 004816c0

void __thiscall FUN_004816c0(void *this,int param_2,byte param_3)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  
  iVar3 = DAT_005395dc + param_2 * 0x24;
  if (((param_3 & 1) != 0) && (*(ushort *)(iVar3 + 0x12) == DAT_0051cbbc)) {
    iVar1 = FUN_004816a0(this);
    if (iVar1 != 0) {
      piVar2 = (int *)(DAT_005395dc + iVar1 * 0x24);
      (**(code **)(*piVar2 + 0x48))(0);
      iVar1 = FUN_004abf50(piVar2);
      *(undefined1 *)((int)piVar2 + 0x22) = *(undefined1 *)(iVar1 + 0x168);
      FUN_004abb00(piVar2,*(ushort *)(iVar3 + 0xc) & 0x7f);
      (**(code **)(*piVar2 + 0x4c))(0);
    }
    if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
      FUN_004a6aa0(DAT_00512bd8,0,0);
    }
  }
  return;
}


