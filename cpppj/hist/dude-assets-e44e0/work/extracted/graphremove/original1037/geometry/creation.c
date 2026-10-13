// ==== FUN_0043fe10 @ 0043fe10

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_0043fe10(float param_1,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  int iVar5;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  local_c = param_2;
  local_4 = param_4;
  local_10 = param_1 - _DAT_00501764;
  local_8 = param_3 + _DAT_00501764;
  fVar1 = param_2 - _DAT_00501764;
  iVar5 = 0;
  fVar2 = param_4 + _DAT_00501764;
  pfVar4 = (float *)FUN_0043fa70(&local_10,local_10,param_2,local_8,param_4);
  if (((((((0 < (int)*pfVar4) && (fVar3 = (float)DAT_0052e9a8, *pfVar4 < fVar3)) &&
         (0 < (int)pfVar4[1])) && ((pfVar4[1] < fVar3 && (0 < (int)pfVar4[2])))) &&
       ((pfVar4[2] < fVar3 && ((0 < (int)pfVar4[3] && (pfVar4[3] < fVar3)))))) &&
      (*pfVar4 <= pfVar4[2])) && (pfVar4[1] <= pfVar4[3])) {
    iVar5 = 1;
  }
  pfVar4 = (float *)FUN_0043fa70(&local_10,param_1,fVar1,param_3,fVar2);
  if (((((0 < (int)*pfVar4) && (fVar1 = (float)DAT_0052e9a8, *pfVar4 < fVar1)) &&
       (0 < (int)pfVar4[1])) &&
      (((pfVar4[1] < fVar1 && (0 < (int)pfVar4[2])) &&
       ((pfVar4[2] < fVar1 && ((0 < (int)pfVar4[3] && (pfVar4[3] < fVar1)))))))) &&
     ((*pfVar4 <= pfVar4[2] && (pfVar4[1] <= pfVar4[3])))) {
    iVar5 = iVar5 + 1;
  }
  return iVar5 == 1;
}


// ==== FUN_0043fa70 @ 0043fa70

float * __thiscall
FUN_0043fa70(float *param_1,float *param_2,float param_3,float param_4,float param_5,float param_6)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar2 = *param_1;
  if (*param_1 <= param_3) {
    fVar2 = param_3;
  }
  fVar3 = param_1[1];
  if (param_1[1] <= param_4) {
    fVar3 = param_4;
  }
  fVar4 = param_1[2];
  if (param_5 <= param_1[2]) {
    fVar4 = param_5;
  }
  fVar5 = param_1[3];
  if (param_6 <= param_1[3]) {
    fVar5 = param_6;
  }
  if ((fVar2 <= fVar4) && (fVar3 <= fVar5)) {
    pfVar1 = param_2 + 2;
    *param_2 = fVar2;
    param_2[1] = fVar3;
    *pfVar1 = fVar2;
    param_2[3] = fVar3;
    FUN_0043f950();
    if (((((int)*param_2 < 1) || ((float)DAT_0052e9a8 <= *param_2)) || ((int)param_2[1] < 1)) ||
       ((float)DAT_0052e9a8 <= param_2[1])) {
      *param_2 = fVar4;
      param_2[1] = fVar5;
    }
    else {
      fVar2 = *param_2;
      if (fVar4 <= *param_2) {
        fVar2 = fVar4;
      }
      *param_2 = fVar2;
      fVar2 = param_2[1];
      if (fVar5 <= param_2[1]) {
        fVar2 = fVar5;
      }
      param_2[1] = fVar2;
    }
    FUN_0043f950();
    if (((0 < (int)*pfVar1) && (*pfVar1 < (float)DAT_0052e9a8)) &&
       ((0 < (int)param_2[3] && (param_2[3] < (float)DAT_0052e9a8)))) {
      fVar2 = *pfVar1;
      if (*pfVar1 <= fVar4) {
        fVar2 = fVar4;
      }
      *pfVar1 = fVar2;
      fVar2 = param_2[3];
      if (param_2[3] <= fVar5) {
        fVar2 = fVar5;
      }
      param_2[3] = fVar2;
      return param_2;
    }
    *pfVar1 = fVar4;
    param_2[3] = fVar5;
    return param_2;
  }
  param_2[1] = -10000.0;
  *param_2 = -10000.0;
  param_2[3] = -10000.0;
  param_2[2] = -10000.0;
  return param_2;
}


