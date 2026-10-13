// ==== FUN_004498e0 @ 004498e0

void __thiscall FUN_004498e0(int param_1,undefined4 param_2)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  int iStack_c;
  
  if ((*(byte *)(param_1 + 0x23) & 9) == 0) {
    FUN_00440a80();
    iVar2 = __ftol();
    iVar3 = __ftol();
    FUN_004eae20(iVar3 + -1,iVar2 + -1,iVar3 + 1,iVar2 + 1);
    while (iStack_c != 0) {
      piVar1 = (int *)(iStack_c * 0x24 + DAT_005395dc);
      if (*(byte *)(iStack_c * 0x24 + 10 + DAT_005395dc) == DAT_0051cbb0) {
        uVar4 = (uint)*(ushort *)(piVar1 + 3) * 0x24 + DAT_005395dc;
        uVar6 = (uint)*(ushort *)(piVar1 + 2) * 0x24 + DAT_005395dc;
        if ((((DAT_005395dc + 0x24U <= uVar4) &&
             (uVar5 = DAT_005395dc + DAT_005395f4 * 0x24, uVar4 < uVar5)) &&
            (DAT_005395dc + 0x24U <= uVar6)) &&
           (((uVar6 < uVar5 && ((*(byte *)(uVar4 + 0x23) & 9) == 0)) &&
            (((*(byte *)(uVar4 + 0xb) & 2) != 0 || ((*(byte *)(uVar6 + 0xb) & 2) != 0)))))) {
          (**(code **)(*piVar1 + 0x10))(0);
        }
      }
      FUN_004eafe0();
    }
  }
  FUN_004add20(param_2);
  return;
}


// ==== FUN_00449c60 @ 00449c60

void __thiscall FUN_00449c60(int param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  int local_c;
  
  if ((*(byte *)(param_1 + 0x23) & 9) == 0) {
    FUN_00454810((param_1 - DAT_005395dc) / 0x24);
    FUN_00438c00(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),
                 s_bridgeFall_wav_0051f0b0,0);
    FUN_00440a50();
    uVar1 = __ftol();
    uVar2 = __ftol();
    FUN_004eae20(uVar2,uVar1,uVar2,uVar1);
    while (local_c != 0) {
      uVar3 = FUN_004abae0();
      if ((uVar3 & 0x10000) != 0) {
        (**(code **)(*(int *)(local_c * 0x24 + DAT_005395dc) + 200))();
      }
      FUN_004eafe0();
    }
  }
  FUN_004adbe0(param_2);
  return;
}


// ==== FUN_004490b0 @ 004490b0

void FUN_004490b0(int param_1)

{
  undefined1 uVar1;
  undefined3 uVar2;
  int iVar3;
  int iVar4;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_00449150;
  *unaff_FS_OFFSET = &local_10;
  iVar4 = DAT_005395dc + param_1 * 0x24;
  uVar2 = __ftol();
  uVar1 = __ftol();
  iVar3 = FUN_004f1650(0x28);
  local_8 = 0;
  if (iVar3 != 0) {
    FUN_0048efd0(0x2692,(iVar4 - DAT_005395dc) / 0x24,(float)CONCAT31(uVar2,uVar1));
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_00440a50 @ 00440a50

void __fastcall FUN_00440a50(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_00440a80 @ 00440a80

void __fastcall FUN_00440a80(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


