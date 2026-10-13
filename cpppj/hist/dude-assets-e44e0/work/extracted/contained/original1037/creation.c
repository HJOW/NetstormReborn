// ==== FUN_004eb310 @ 004eb310

uint FUN_004eb310(int param_1,uint param_2,int param_3,int param_4)

{
  int *piVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  
  uVar2 = *(ushort *)(param_1 * 0x24 + 6 + DAT_005395dc);
  iVar5 = DAT_005395dc;
  do {
    uVar3 = (uint)uVar2;
    if (uVar3 == 0) {
      return 0;
    }
    piVar1 = (int *)(uVar3 * 0x24 + iVar5);
    if ((*(byte *)(uVar3 * 0x24 + 10 + iVar5) == param_2) &&
       ((*(byte *)((int)piVar1 + 0xb) & 2) == 0)) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return uVar3;
        }
        if ((DAT_00540a28 != 0) ||
           ((iVar4 = ((int)piVar1 - iVar5) / 0x24, 4 < iVar4 && (iVar4 < 6000)))) {
          (**(code **)(*piVar1 + 0x10))(0);
          iVar5 = DAT_005395dc;
        }
      }
      param_3 = param_3 + -1;
    }
    uVar2 = *(ushort *)(piVar1 + 1);
  } while( true );
}


