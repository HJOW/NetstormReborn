// ==== FUN_00437390 @ 00437390

void __thiscall FUN_00437390(void *this,undefined4 *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar2 = FUN_004abf50();
  iVar3 = (**(code **)(*(int *)this + 0x78))();
  iVar1 = *(int *)(iVar2 + 0xdc);
  iVar2 = *(int *)(iVar1 + 8 + iVar3 * 8) + -0x24 + iVar1;
  iVar1 = *(int *)(iVar1 + 8 + (uint)*(byte *)((int)this + 0x22) * 8) + -0x24 + iVar1;
  uStack_8 = *(undefined4 *)((int)this + 0xe);
  uStack_4 = *(undefined4 *)((int)this + 0x12);
  FUN_0043fa10(&uStack_8,(int)*(short *)(iVar1 + 0x20) + (int)*(short *)(iVar2 + 0x20),
               (int)*(short *)(iVar1 + 0x22) + (int)*(short *)(iVar2 + 0x22));
  *param_2 = uStack_8;
  param_2[1] = uStack_4;
  return;
}


// ==== FUN_0043fa10 @ 0043fa10

void __thiscall FUN_0043fa10(void *this,int param_2,int param_3)

{
  float *extraout_ECX;
  float *extraout_ECX_00;
  float10 fVar1;
  
  if (DAT_00546778 == 0) {
    fVar1 = (float10)param_2 / (float10)DAT_00565c44;
  }
  else {
    fVar1 = (float10)__adj_fdiv_m32(DAT_00565c44);
    this = extraout_ECX;
  }
  *(float *)this = (float)(fVar1 + (float10)*(float *)this);
  if (DAT_00546778 == 0) {
    fVar1 = (float10)param_3 / (float10)DAT_00565c48;
  }
  else {
    fVar1 = (float10)__adj_fdiv_m32(DAT_00565c48);
    this = extraout_ECX_00;
  }
  *(float *)((int)this + 4) = (float)(fVar1 + (float10)*(float *)((int)this + 4));
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1c4) + *(int *)((int)this + 0x1b8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1b8);
  return;
}


// ==== FUN_00444b50 @ 00444b50

float * __thiscall FUN_00444b50(void *this,float *param_2,float param_3,float param_4)

{
  float *pfVar1;
  float fVar2;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  int local_8;
  int local_4;
  
  FUN_004442b0(this,&local_8,&local_4,1);
  local_1c = param_3 - (float)local_8;
  local_18 = param_4 - (float)local_4;
  pfVar1 = param_2 + 2;
  *param_2 = param_3;
  param_2[1] = param_4;
  *pfVar1 = param_3;
  param_2[3] = param_4;
  local_14 = local_1c;
  local_10 = local_18;
  FUN_0043f950(&local_14);
  if (((((int)*param_2 < 1) || (local_c = (float)DAT_0052e9a8, local_c <= *param_2)) ||
      ((int)param_2[1] < 1)) || (local_c <= param_2[1])) {
    *param_2 = local_14;
    param_2[1] = local_10;
  }
  else {
    fVar2 = *param_2;
    if (local_14 <= *param_2) {
      fVar2 = local_14;
    }
    *param_2 = fVar2;
    fVar2 = param_2[1];
    if (local_10 <= param_2[1]) {
      fVar2 = local_10;
    }
    param_2[1] = fVar2;
  }
  FUN_0043f950(&local_1c);
  if (((0 < (int)*pfVar1) && (*pfVar1 < (float)DAT_0052e9a8)) &&
     ((0 < (int)param_2[3] && (param_2[3] < (float)DAT_0052e9a8)))) {
    fVar2 = *pfVar1;
    if (*pfVar1 <= local_1c) {
      fVar2 = local_1c;
    }
    *pfVar1 = fVar2;
    fVar2 = param_2[3];
    if (param_2[3] <= local_18) {
      fVar2 = local_18;
    }
    param_2[3] = fVar2;
    return param_2;
  }
  *pfVar1 = local_1c;
  param_2[3] = local_18;
  return param_2;
}


