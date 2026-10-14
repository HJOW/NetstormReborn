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


// ==== FUN_004e649e @ 004e649e

void __cdecl FUN_004e649e(ulong param_1)

{
  _ptiddata p_Var1;
  
  p_Var1 = __getptd();
  p_Var1->_holdrand = param_1;
  return;
}


// ==== _rand @ 004e64ab

/* Library Function - Single Match
    _rand
   
   Libraries: Visual Studio 2003 Release, Visual Studio 2005 Release, Visual Studio 2008 Release */

int __cdecl _rand(void)

{
  _ptiddata p_Var1;
  uint uVar2;
  
  p_Var1 = __getptd();
  uVar2 = p_Var1->_holdrand * 0x343fd + 0x269ec3;
  p_Var1->_holdrand = uVar2;
  return uVar2 >> 0x10 & 0x7fff;
}


