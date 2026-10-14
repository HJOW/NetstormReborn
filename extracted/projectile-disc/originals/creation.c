// ==== FUN_00469540 @ 00469540

float10 __thiscall FUN_00469540(void *this,undefined4 param_2,undefined4 param_3,float param_4)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004ac550();
  iVar2 = *(int *)(iVar1 + 0x114) / *(int *)(iVar1 + 0x110);
  iVar1 = *(int *)((int)this + 0x24);
  FUN_004acee0(this,((iVar1 + 1) % iVar2 - iVar1 % iVar2) + iVar1,0);
  return (float10)param_4;
}


// ==== FUN_004437f0 @ 004437f0

void __cdecl FUN_004437f0(void)

{
  return;
}


// ==== FUN_004ace20 @ 004ace20

void __thiscall FUN_004ace20(void *this)

{
  undefined4 uVar1;
  
  uVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  FUN_0049af80(uVar1);
  return;
}


