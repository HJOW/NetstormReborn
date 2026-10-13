// ==== FUN_004950f0 @ 004950f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004950f0(int param_1,uint param_2)

{
  float fVar1;
  int iVar2;
  void *pvVar3;
  uint uVar4;
  int iVar5;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_004fb25b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    ExceptionList = &pvStack_c;
    if ((param_2 & 1) != 0) {
      ExceptionList = &pvStack_c;
      FUN_00414450((param_1 - DAT_005c8464) / 0x32);
      iVar2 = FUN_004afb40(0x25a);
      if (iVar2 == 0) {
        pvVar3 = operator_new(0x28);
        uStack_4 = 0;
        if (pvVar3 != (void *)0x0) {
          iVar5 = param_1 - DAT_005c8464;
          FUN_0049a840(DAT_005412d0);
          iVar2 = FUN_0049fce0();
          fVar1 = (float)(iVar2 / 6) * _DAT_0050f20c;
          iVar2 = FUN_004adcb0();
          FUN_00496e80(0x25a,iVar5 / 0x32,fVar1 / (float)iVar2);
        }
        uStack_4 = 0xffffffff;
      }
    }
    if ((((*(byte *)(param_1 + 0xc) & 0x7f) == *(byte *)(param_1 + 0x22)) &&
        (uVar4 = (uint)*(byte *)(param_1 + 0x22), uVar4 != 0)) && (uVar4 < 9)) {
      *(undefined4 *)(&DAT_00595428 + uVar4 * 4) = 0;
    }
  }
  FUN_00427720(param_2);
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    iVar2 = FUN_00427030();
    if (iVar2 != 0) {
      if ((((param_2 & 0x800) == 0) &&
          (uVar4 = FUN_0040d9d0(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12)),
          (uVar4 & 6) == 0)) && (iVar2 = FUN_004ad710(), iVar2 != 9)) {
        FUN_004941f0();
      }
      FUN_00493d30();
      ExceptionList = pvStack_c;
      return;
    }
    FUN_00491980();
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00414450 @ 00414450

void __thiscall FUN_00414450(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar1 = param_1[2];
  if (iVar1 < param_1[1]) {
    iVar2 = 0;
    if (0 < iVar1) {
      piVar3 = (int *)*param_1;
      do {
        if (*piVar3 == param_2) {
          return;
        }
        iVar2 = iVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (iVar2 < param_1[2]);
    }
    *(int *)(*param_1 + iVar1 * 4) = param_2;
    param_1[2] = param_1[2] + 1;
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049fce0 @ 0049fce0

int __fastcall FUN_0049fce0(int *param_1)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)(param_1 + 0x3a) & 0x10) == 0)) &&
     (((int)param_1 - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)param_1 - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
  }
  return *param_1;
}


// ==== FUN_004adcb0 @ 004adcb0

undefined4 __fastcall FUN_004adcb0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) == 0) {
    FUN_0049a840(*(undefined1 *)(param_1 + 10));
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  if ((*(byte *)(param_1 + 0x28) & 1) != 0) {
    FUN_0049a840(DAT_005412e8);
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  return 0;
}


