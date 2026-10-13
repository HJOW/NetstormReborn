// ==== FUN_0041d9b0 @ 0041d9b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_0041d9b0(float param_1,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *pfVar5;
  int iVar6;
  undefined1 local_10 [16];
  
  fVar4 = param_1;
  fVar1 = param_1 - _DAT_005012d0;
  fVar2 = param_2 - _DAT_005012d0;
  fVar3 = param_4 + _DAT_005012d0;
  param_1 = 0.0;
  pfVar5 = (float *)FUN_0041d8c0(local_10,fVar1,param_2,param_3 + _DAT_005012d0,param_4);
  iVar6 = FUN_0040e800();
  if (iVar6 != 0) {
    iVar6 = FUN_0040e800();
    if (((iVar6 != 0) && (*pfVar5 < pfVar5[2] != (*pfVar5 == pfVar5[2]))) &&
       (pfVar5[1] < pfVar5[3] != (pfVar5[1] == pfVar5[3]))) {
      param_1 = 1.4013e-45;
    }
  }
  pfVar5 = (float *)FUN_0041d8c0(local_10,fVar4,fVar2,param_3,fVar3);
  iVar6 = FUN_0040e800();
  if (iVar6 != 0) {
    iVar6 = FUN_0040e800();
    if (((iVar6 != 0) && (*pfVar5 < pfVar5[2] != (*pfVar5 == pfVar5[2]))) &&
       (pfVar5[1] < pfVar5[3] != (pfVar5[1] == pfVar5[3]))) {
      param_1 = (float)((int)param_1 + 1);
    }
  }
  return param_1 == 1.4013e-45;
}


// ==== FUN_0041d8c0 @ 0041d8c0

float * __thiscall
FUN_0041d8c0(float *param_1,float *param_2,float param_3,float param_4,float param_5,float param_6)

{
  float local_10;
  float local_c;
  float local_4;
  
  if (param_3 < *param_1) {
    param_3 = *param_1;
  }
  if (param_1[1] <= param_4) {
    local_4 = param_4;
  }
  else {
    local_4 = param_1[1];
  }
  if (param_5 <= param_1[2]) {
    local_10 = param_5;
  }
  else {
    local_10 = param_1[2];
  }
  if (param_6 <= param_1[3]) {
    local_c = param_6;
  }
  else {
    local_c = param_1[3];
  }
  if ((param_3 < local_10 != (param_3 == local_10)) && (local_4 < local_c != (local_4 == local_c)))
  {
    param_2[1] = local_4;
    param_2[3] = local_4;
    *param_2 = param_3;
    param_2[2] = param_3;
    FUN_0040e880(local_10,local_c);
    FUN_0040e8f0(local_10,local_c);
    return param_2;
  }
  param_2[1] = -10000.0;
  *param_2 = -10000.0;
  param_2[3] = -10000.0;
  param_2[2] = -10000.0;
  return param_2;
}


