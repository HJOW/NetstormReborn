// ==== FUN_004c6b80 @ 004c6b80

void __thiscall FUN_004c6b80(void *this,undefined4 param_2,undefined4 param_3,int param_4)

{
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  local_c = DAT_005ca9d0 + param_4;
  local_10 = DAT_005ca9cc + param_4;
  local_4 = DAT_005ca9d8 - param_4;
  local_8 = DAT_005ca9d4 - param_4;
  FUN_00463f80(&local_10,*(undefined4 *)this,*(undefined4 *)((int)this + 4),param_2,param_3);
  return;
}


// ==== FUN_00499310 @ 00499310

void __cdecl FUN_00499310(int param_1,int param_2,int param_3,int param_4,undefined4 param_5)

{
  FUN_00445290(&param_1,DAT_005ca9cc,DAT_005ca9d0,DAT_005ca9d4,DAT_005ca9d8);
  if ((param_3 != param_1 && -1 < param_3 - param_1) &&
     (param_4 != param_2 && -1 < param_4 - param_2)) {
    FUN_00497580(DAT_0059a8bc,&param_1,param_5);
  }
  return;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_00419850 @ 00419850

int __thiscall FUN_00419850(void *this,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  
  if ((*(uint *)((int)this + 0xe8) & 0x440000) == 0) {
    if ((*(int *)((int)this + 0x114) <= param_3) || (param_3 < 0)) goto LAB_004198a8;
LAB_00419887:
    if (((*(int *)((int)this + 0xdc) != 0) && (*(int *)this != -0x22222223)) &&
       (*(int *)this != -0x32323233)) goto LAB_004198d0;
  }
  else {
    if ((param_3 < *(int *)((int)this + 0x114) * 2) && (-1 < param_3)) goto LAB_00419887;
LAB_004198a8:
    uVar1 = FUN_0049adc0(this,param_3,param_2);
    FUN_00436d40(uVar1);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_004198d0:
  return *(int *)(*(int *)((int)this + 0xdc) + 8 + param_3 * 8) + -0x24 + *(int *)((int)this + 0xdc)
  ;
}


// ==== FUN_004daa70 @ 004daa70

void __cdecl FUN_004daa70(int param_1,undefined4 param_2,int param_3,int param_4)

{
  if (param_1 != 0) {
    FUN_00401d92(&DAT_005b5ddc,param_1,param_2,param_3 - DAT_005b5de0,param_4 - DAT_005b5de4);
  }
  return;
}


// ==== FUN_004daab0 @ 004daab0

void __cdecl FUN_004daab0(int param_1,undefined4 param_2,int param_3,int param_4)

{
  if (param_1 != 0) {
    FUN_0040229f(&DAT_005b5ddc,param_1,param_2,param_3 - DAT_005b5de0,param_4 - DAT_005b5de4);
  }
  return;
}


// ==== FUN_004dabe0 @ 004dabe0

void __cdecl FUN_004dabe0(undefined4 param_1)

{
  FUN_0040227e(param_1);
  return;
}


