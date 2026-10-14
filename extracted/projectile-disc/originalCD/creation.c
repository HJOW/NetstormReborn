// ==== FUN_00437290 @ 00437290

float10 __thiscall FUN_00437290(void *this,undefined4 param_2,undefined4 param_3,float param_4)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004abf50();
  uVar2 = (uint)*(byte *)((int)this + 0x22);
  iVar1 = *(int *)(iVar1 + 0x114) / *(int *)(iVar1 + 0x110);
  FUN_004acbc0(this,((int)(uVar2 + 1) % iVar1 - (int)((longlong)(ulonglong)uVar2 % (longlong)iVar1))
                    + uVar2,0);
  return (float10)param_4;
}


// ==== FUN_00401ca0 @ 00401ca0

void __cdecl FUN_00401ca0(void)

{
  return;
}


// ==== FUN_004acaf0 @ 004acaf0

void __thiscall FUN_004acaf0(void)

{
  undefined4 uVar1;
  
  uVar1 = FUN_004abf50();
  FUN_00444cb0(uVar1);
  return;
}


