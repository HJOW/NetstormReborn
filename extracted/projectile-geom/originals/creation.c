// ==== FUN_0046a780 @ 0046a780

void __thiscall FUN_0046a780(void *this,undefined4 *param_2)

{
  byte bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uVar2 = FUN_004ac550();
  bVar1 = *(byte *)((int)this + 0x28);
  uVar3 = (**(code **)(*(int *)this + 0x78))();
  iVar4 = FUN_00419850(uVar2,bVar1 & 1,uVar3);
  iVar5 = FUN_00419850(uVar2,*(byte *)((int)this + 0x28) & 1,*(undefined4 *)((int)this + 0x24));
  uStack_8 = *(undefined4 *)((int)this + 0xe);
  uStack_4 = *(undefined4 *)((int)this + 0x12);
  FUN_0041d180(&uStack_8,(int)*(short *)(iVar5 + 0x20) + (int)*(short *)(iVar4 + 0x20),
               (int)*(short *)(iVar5 + 0x22) + (int)*(short *)(iVar4 + 0x22));
  *param_2 = uStack_8;
  param_2[1] = uStack_4;
  return;
}


// ==== FUN_0041d180 @ 0041d180

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d180(void *this,int param_2,int param_3)

{
  *(float *)this = (float)param_2 / _DAT_0059a92c + *(float *)this;
  *(float *)((int)this + 4) = (float)param_3 / _DAT_0059a930 + *(float *)((int)this + 4);
  return;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1d8);
  return;
}


// ==== FUN_0049af10 @ 0049af10

float * __thiscall FUN_0049af10(void *this,float *param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  
  fVar1 = param_3 - (float)*(int *)((int)this + 0x1d4);
  fVar2 = param_4 - (float)(*(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8));
  param_2[1] = param_4;
  param_2[3] = param_4;
  *param_2 = param_3;
  param_2[2] = param_3;
  FUN_0040e880(param_2,fVar1,fVar2);
  FUN_0040e8f0(param_2,fVar1,fVar2);
  return param_2;
}


// ==== FUN_0040e880 @ 0040e880

void __thiscall FUN_0040e880(void *this,float param_2,float param_3)

{
  int iVar1;
  
  FUN_0041d0d0(&param_2);
  iVar1 = FUN_0040e800(this);
  if (iVar1 == 0) {
    *(float *)this = param_2;
    *(float *)((int)this + 4) = param_3;
    return;
  }
  if (*(float *)this < param_2) {
    param_2 = *(float *)this;
  }
  *(float *)this = param_2;
  if (param_3 <= *(float *)((int)this + 4)) {
    *(float *)((int)this + 4) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 4) = *(undefined4 *)((int)this + 4);
  return;
}


// ==== FUN_0040e8f0 @ 0040e8f0

void __thiscall FUN_0040e8f0(void *this,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  
  FUN_0041d0d0(&param_2);
  pfVar1 = (float *)((int)this + 8);
  iVar2 = FUN_0040e800(pfVar1);
  if (iVar2 == 0) {
    *pfVar1 = param_2;
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  if (param_2 < *pfVar1) {
    param_2 = *pfVar1;
  }
  *pfVar1 = param_2;
  if (*(float *)((int)this + 0xc) <= param_3) {
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)this + 0xc);
  return;
}


