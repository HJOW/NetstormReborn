// ==== FUN_0047feb0 @ 0047feb0

void __thiscall
FUN_0047feb0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  FUN_0047ec60(this,param_2,param_3,param_4,0,param_5,param_6);
  return;
}


// ==== FUN_00480910 @ 00480910

void __thiscall
FUN_00480910(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  FUN_0047ec60(this,param_2,param_3,param_4,0,param_5,param_6);
  return;
}


// ==== FUN_004e6e70 @ 004e6e70

void FUN_004e6e70(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_48 = param_1;
  local_44 = param_2;
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = 1;
  local_4 = 0;
  FUN_00440a50(&local_48);
  uVar1 = __ftol();
  uVar2 = __ftol();
  FUN_004eae20(local_40,uVar2,uVar1,uVar2,uVar1);
  while (local_c != 0) {
    if (*(byte *)(local_c * 0x24 + 10 + DAT_005395dc) == DAT_0051cb2c) {
      *(int *)(param_3 + 0x2f0c) = local_c;
    }
    FUN_004eafe0(local_40);
  }
  return;
}


// ==== FUN_0040c510 @ 0040c510

void __thiscall FUN_0040c510(void *this)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_0040c570(this,0x25c);
  FUN_0040c570(this,0x25d);
  uVar2 = 0;
  iVar1 = FUN_004abf50(this,0);
  FUN_004acbc0(this,*(undefined4 *)(iVar1 + 0x118),uVar2);
  return;
}


// ==== FUN_00401c30 @ 00401c30

void FUN_00401c30(void)

{
  return;
}


// ==== FUN_004ac480 @ 004ac480

void __cdecl FUN_004ac480(void)

{
  return;
}


// ==== FUN_0040c570 @ 0040c570

void __thiscall FUN_0040c570(void *this,undefined4 param_2)

{
  int iVar1;
  
  do {
    iVar1 = FUN_004ac890(this,param_2);
    if (iVar1 == 0) {
      return;
    }
    FUN_0048f7b0(iVar1,0);
  } while (iVar1 != 0);
  return;
}


// ==== FUN_004aeec0 @ 004aeec0

undefined4 __thiscall FUN_004aeec0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  
  uVar3 = *(undefined4 *)((int)this + 0xe);
  uVar4 = *(undefined4 *)((int)this + 0x12);
  uVar2 = param_2;
  uVar1 = FUN_004abf50(this,param_2,uVar3,uVar4,param_3);
  FUN_004449d0(uVar1,uVar2,uVar3,uVar4,param_3);
  return param_2;
}


// ==== FUN_004449d0 @ 004449d0

float * __thiscall FUN_004449d0(void *this,float *param_2,float param_3,float param_4,uint param_5)

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
  
  FUN_004442b0(this,&local_8,&local_4,param_5 & 1);
  local_1c = (float)(1 - local_8) + param_3;
  pfVar1 = param_2 + 2;
  *param_2 = param_3;
  *pfVar1 = param_3;
  param_2[1] = param_4;
  param_2[3] = param_4;
  local_18 = (float)(1 - local_4) + param_4;
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


