// ==== FUN_004ae280 @ 004ae280

void __thiscall FUN_004ae280(void *this,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540bc0 != 0) {
    piVar1 = (int *)FUN_00471aa0(*(undefined4 *)((int)this + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_00471aa0 @ 00471aa0

undefined4 __cdecl FUN_00471aa0(int param_1)

{
  return (&DAT_0056b808)[param_1];
}


