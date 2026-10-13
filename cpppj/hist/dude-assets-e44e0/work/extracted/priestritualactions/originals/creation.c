// ==== FUN_00491ca0 @ 00491ca0

void __cdecl FUN_00491ca0(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  uVar2 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    uVar3 = 0;
    iVar1 = FUN_004ac550(uVar2,0);
    FUN_004acee0(uVar2,*(undefined4 *)(iVar1 + 0x158 + param_2 * 4),uVar3);
  }
  return;
}


// ==== FUN_00491cf0 @ 00491cf0

void __cdecl FUN_00491cf0(int param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  void *pvVar4;
  undefined4 uVar5;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb1db;
  local_c = ExceptionList;
  uVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar1) && (uVar2 = DAT_005c8464 + DAT_005c847c * 0x32, uVar1 < uVar2)
     ) {
    ExceptionList = &local_c;
    if ((DAT_005c8464 + 0x32U <= uVar1) && (ExceptionList = &local_c, uVar1 < uVar2)) {
      uVar5 = 0;
      ExceptionList = &local_c;
      iVar3 = FUN_004ac550(uVar1,0);
      FUN_004acee0(uVar1,*(undefined4 *)(iVar3 + 0x158 + param_2 * 4),uVar5);
    }
    pvVar4 = operator_new(0x28);
    local_4 = 0;
    if (pvVar4 != (void *)0x0) {
      FUN_00496f00(pvVar4,0x25c,(int)(uVar1 - DAT_005c8464) / 0x32,0x3c23d70a);
    }
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


