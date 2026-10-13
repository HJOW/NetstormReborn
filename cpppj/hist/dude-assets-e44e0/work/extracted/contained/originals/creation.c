// ==== FUN_004b1a90 @ 004b1a90

uint FUN_004b1a90(int param_1,uint param_2,int param_3,int param_4)

{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  int *piVar4;
  int iVar5;
  
  uVar1 = *(ushort *)(param_1 * 0x32 + 6 + DAT_005c8464);
  iVar5 = DAT_005c8464;
  do {
    uVar3 = (uint)uVar1;
    if (uVar3 == 0) {
      return 0;
    }
    piVar4 = (int *)(uVar3 * 0x32 + iVar5);
    if ((*(byte *)(uVar3 * 0x32 + 10 + iVar5) == param_2) &&
       ((*(byte *)((int)piVar4 + 0xb) & 2) == 0)) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return uVar3;
        }
        if ((DAT_00540bc0 != 0) ||
           ((iVar2 = ((int)piVar4 - iVar5) / 0x32, 4 < iVar2 && (iVar2 < 15000)))) {
          (**(code **)(*piVar4 + 0x10))(0);
          iVar5 = DAT_005c8464;
        }
      }
      param_3 = param_3 + -1;
    }
    uVar1 = *(ushort *)(piVar4 + 1);
  } while( true );
}


