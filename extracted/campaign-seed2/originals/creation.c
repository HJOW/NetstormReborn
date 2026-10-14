// ==== FUN_004559d0 @ 004559d0

void __cdecl FUN_004559d0(void)

{
  undefined4 uVar1;
  
  uVar1 = DAT_005c89a8;
  DAT_005c89a8 = 1;
  FUN_00455970();
  DAT_005c89a8 = uVar1;
  return;
}


// ==== FUN_00455970 @ 00455970

void __cdecl FUN_00455970(void)

{
  __time32_t _Var1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  if (DAT_005c89a8 == 0) {
    FUN_004e649e(0x38d535);
  }
  else {
    _Var1 = FID_conflict___time32((__time32_t *)0x0);
    FUN_004e649e(_Var1);
  }
  iVar4 = 100;
  do {
    _rand();
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  iVar4 = _rand();
  iVar2 = _rand();
  iVar3 = _rand();
  DAT_00532710 = iVar4 * iVar2 * iVar3;
  return;
}


