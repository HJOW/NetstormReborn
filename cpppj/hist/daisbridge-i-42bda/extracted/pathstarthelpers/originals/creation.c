// ==== FUN_0048bb10 @ 0048bb10

void __thiscall
FUN_0048bb10(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  FUN_0048a660(this,param_2,param_3,param_4,0,param_5,param_6);
  return;
}


// ==== FUN_00427750 @ 00427750

void FUN_00427750(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_44 = param_2;
  local_48 = param_1;
  local_40[0] = &PTR_FUN_00502318;
  local_8 = 1;
  local_4 = 0;
  FUN_0041d770(&local_48);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  FUN_004b16d0(local_40,uVar2,uVar1,uVar2,uVar1);
  while (local_c != 0) {
    if (*(byte *)(local_c * 0x32 + 10 + DAT_005c8464) == DAT_00541240) {
      *(int *)(param_3 + 0x186cc) = local_c;
    }
    FUN_004b1810(local_40);
  }
  return;
}


// ==== FUN_00493f20 @ 00493f20

void __thiscall FUN_00493f20(void *this)

{
  int iVar1;
  undefined4 uVar2;
  
  while( true ) {
    iVar1 = FUN_004afb40(this,0x25c);
    if (iVar1 == 0) break;
    FUN_0041bf10(iVar1,0);
  }
  while( true ) {
    iVar1 = FUN_004afb40(this,0x25d);
    uVar2 = 0;
    if (iVar1 == 0) break;
    FUN_0041bf10(iVar1,0);
  }
  iVar1 = FUN_004ac550(this,0);
  FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x118),uVar2);
  return;
}


// ==== FUN_00480260 @ 00480260

void FUN_00480260(void)

{
  return;
}


// ==== FUN_004437f0 @ 004437f0

void __cdecl FUN_004437f0(void)

{
  return;
}


// ==== FUN_004ade70 @ 004ade70

undefined4 __thiscall FUN_004ade70(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  uVar1 = *(undefined4 *)((int)this + 0xe);
  uVar2 = *(undefined4 *)((int)this + 0x12);
  uVar3 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  FUN_0049ae80(uVar3,param_2,uVar1,uVar2,param_3);
  return param_2;
}


// ==== FUN_0049ae80 @ 0049ae80

float * __thiscall FUN_0049ae80(void *this,float *param_2,float param_3,float param_4,byte param_5)

{
  float fVar1;
  int iVar2;
  
  if ((param_5 & 1) == 0) {
    iVar2 = *(int *)((int)this + 0x1d8);
  }
  else {
    iVar2 = *(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8);
  }
  fVar1 = param_3 - (float)(*(int *)((int)this + 0x1d4) + -1);
  param_2[1] = param_4;
  param_2[3] = param_4;
  *param_2 = param_3;
  param_2[2] = param_3;
  param_4 = param_4 - (float)(iVar2 + -1);
  FUN_0040e880(param_2,fVar1,param_4);
  FUN_0040e8f0(param_2,fVar1,param_4);
  return param_2;
}


// ==== FUN_0041def0 @ 0041def0

void * __thiscall FUN_0041def0(void *this)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint in_stack_00000014;
  
  *(uint *)((int)this + 0x38) = in_stack_00000014 & 1;
  *(undefined ***)this = &PTR_FUN_00502318;
  *(uint *)((int)this + 0x3c) = in_stack_00000014 & 4;
  FUN_0041d770(&stack0x00000004);
  FUN_0041d7a0(&stack0x0000000c);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0(uVar1);
  uVar3 = FUN_004e49c0(uVar2);
  uVar4 = FUN_004e49c0(uVar3);
  FUN_004b16d0(this,uVar4,uVar3,uVar2,uVar1);
  return this;
}


