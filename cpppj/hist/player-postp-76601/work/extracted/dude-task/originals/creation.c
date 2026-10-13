// ==== FUN_00450670 @ 00450670

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00450670(void *this,int param_2,float param_3)

{
  int iVar1;
  void *pvVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fa15b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if ((*(short *)((int)this + 0xc) == 0x5105) && (ExceptionList = &local_c, param_2 != 0x5105)) {
    ExceptionList = &local_c;
    FUN_004e0620("task == taskFALL",".\\Dude.cpp",0x9c);
  }
  iVar1 = FUN_004afbe0(this,0xfff0,0x5100);
  if (iVar1 == 0) {
    pvVar2 = operator_new(0x28);
    local_4 = 0;
    if (pvVar2 == (void *)0x0) {
      iVar1 = 0;
    }
    else {
      iVar1 = FUN_00496e80(pvVar2,param_2,((int)this - DAT_005c8464) / 0x32,0x40a00000);
    }
    local_4 = 0xffffffff;
  }
  else {
    *(int *)(iVar1 + 0x18) = param_2;
    *(undefined4 *)(iVar1 + 0x20) = 0;
  }
  *(double *)(iVar1 + 0x10) = (double)(param_3 + (float)_DAT_0055b4d0);
  FUN_004ac220(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== FUN_004ac220 @ 004ac220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ac220(void *this,undefined2 param_2)

{
  *(undefined2 *)((int)this + 0xc) = param_2;
  if (*(byte *)((int)this + 10) == _DAT_005411a0) {
    FUN_004214a0(((int)this - DAT_005c8464) / 0x32);
  }
  return;
}


// ==== FUN_00496e80 @ 00496e80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00496e80(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb2d8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  uVar1 = _DAT_00500460;
  *(undefined ***)this = &PTR_FUN_0050f420;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  local_4 = 0;
  FUN_0041bd20(this,DAT_00541128,param_3,0,0x50);
  *(undefined4 *)((int)this + 0x1c) = param_4;
  uVar1 = _DAT_0055b4d0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  *(undefined4 *)((int)this + 0x18) = param_2;
  ExceptionList = local_c;
  return this;
}


