// ==== FUN_004acee0 @ 004acee0

void __thiscall FUN_004acee0(int *param_1,int param_2,uint param_3)

{
  char cVar1;
  int iVar2;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_004fb580;
  local_10 = ExceptionList;
  ExceptionList = &local_10;
  if ((param_3 != 0) && (ExceptionList = &local_10, param_3 != 0x2000)) {
    ExceptionList = &local_10;
    FUN_004e0620("popFlags == 0 || popFlags == pfANIM_DIRTY","\\Ns\\O\\Squid.cpp",0x508);
  }
  cVar1 = *(char *)((int)param_1 + 0x21);
  local_8 = 0;
  iVar2 = FUN_004ace40();
  if (iVar2 == cVar1) {
    if ((param_3 & 0x2000) == 0) {
      (**(code **)(*param_1 + 0x88))();
    }
    else {
      (**(code **)(*param_1 + 0x8c))();
    }
    param_1[9] = param_2;
    if ((param_3 & 0x2000) == 0) {
      (**(code **)(*param_1 + 0x88))();
      ExceptionList = local_10;
      return;
    }
    (**(code **)(*param_1 + 0x8c))();
  }
  else if (param_2 != param_1[9]) {
    (**(code **)(*param_1 + 0x48))(param_3);
    param_1[9] = param_2;
    (**(code **)(*param_1 + 0x4c))(param_3 | 0x50);
    ExceptionList = local_10;
    return;
  }
  ExceptionList = local_10;
  return;
}


// ==== FUN_0041c0d0 @ 0041c0d0

void __thiscall FUN_0041c0d0(int *param_1,undefined4 param_2)

{
  (**(code **)(*param_1 + 0x90))
            (*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),param_2);
  return;
}


// ==== FUN_004ad670 @ 004ad670

void __fastcall FUN_004ad670(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ad672. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


