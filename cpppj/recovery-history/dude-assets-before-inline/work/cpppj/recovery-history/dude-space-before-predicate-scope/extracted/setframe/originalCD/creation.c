// ==== FUN_004acbc0 @ 004acbc0

void __thiscall FUN_004acbc0(int *param_1,uint param_2,uint param_3)

{
  char cVar1;
  int iVar2;
  
  if ((param_3 != 0) && (param_3 != 0x2000)) {
    FUN_00490040(s_popFlags____0____popFlags____pfA_00539f00,s__ns_o_Squid_cpp_00539ef0,0x4f9);
  }
  cVar1 = *(char *)((int)param_1 + 0x1f);
  iVar2 = FUN_004acb00();
  if (iVar2 != cVar1) {
    if (*(byte *)((int)param_1 + 0x22) != param_2) {
      (**(code **)(*param_1 + 0x48))(param_3);
      *(undefined1 *)((int)param_1 + 0x22) = (undefined1)param_2;
      (**(code **)(*param_1 + 0x4c))(param_3 | 0x50);
    }
    return;
  }
  if ((param_3 & 0x2000) == 0) {
    (**(code **)(*param_1 + 0x88))();
  }
  else {
    (**(code **)(*param_1 + 0x8c))();
  }
  *(undefined1 *)((int)param_1 + 0x22) = (undefined1)param_2;
  if ((param_3 & 0x2000) != 0) {
    (**(code **)(*param_1 + 0x8c))();
    return;
  }
  (**(code **)(*param_1 + 0x88))();
  return;
}


// ==== FUN_00401c60 @ 00401c60

void __thiscall FUN_00401c60(int *param_1,undefined4 param_2)

{
  (**(code **)(*param_1 + 0x90))
            (*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),param_2);
  return;
}


// ==== FUN_004ae3e0 @ 004ae3e0

void __fastcall FUN_004ae3e0(int *param_1)

{
                    /* WARNING: Could not recover jumptable at 0x004ae3e2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0x88))();
  return;
}


