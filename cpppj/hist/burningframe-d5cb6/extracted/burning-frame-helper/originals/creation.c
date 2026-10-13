// ==== FUN_00418400 @ 00418400

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00418400(void *this)

{
  undefined8 uVar1;
  undefined4 local_8;
  undefined4 uStack_4;
  
  uVar1 = _DAT_00500460;
  *(undefined4 *)this = DAT_0055b4d0;
  local_8 = (undefined4)uVar1;
  *(undefined4 *)((int)this + 4) = DAT_0055b4d4;
  uStack_4 = (undefined4)((ulonglong)uVar1 >> 0x20);
  *(undefined4 *)((int)this + 8) = local_8;
  *(undefined4 *)((int)this + 0xc) = uStack_4;
  return;
}


// ==== FUN_00418430 @ 00418430

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00418430(void *this)

{
  float10 fVar1;
  
  fVar1 = ((float10)(double)CONCAT44(DAT_0055b4d4,DAT_0055b4d0) - (float10)*(double *)this) *
          (float10)_DAT_00501450;
  *(float *)((int)this + 0x10) = (float)fVar1;
  *(float *)((int)this + 0x14) = (float)(fVar1 * (float10)_DAT_0059a934);
  *(undefined4 *)this = DAT_0055b4d0;
  *(undefined4 *)((int)this + 4) = DAT_0055b4d4;
  *(double *)((int)this + 8) =
       (double)(*(float *)((int)this + 0x10) + (float)*(double *)((int)this + 8));
  return;
}


// ==== FUN_00423a70 @ 00423a70

void * __thiscall FUN_00423a70(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *(float *)this = (float)iVar1;
  iVar1 = FUN_004e49c0();
  *(float *)((int)this + 4) = (float)iVar1;
  return this;
}


// ==== FUN_004239f0 @ 004239f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004239f0(void *this,float param_2,float param_3)

{
  float fVar1;
  
  fVar1 = param_2;
  if (*(float *)this < param_2) {
    fVar1 = *(float *)this;
  }
  *(float *)this = fVar1;
  fVar1 = param_3;
  if (*(float *)((int)this + 4) < param_3) {
    fVar1 = *(float *)((int)this + 4);
  }
  *(float *)((int)this + 4) = fVar1;
  param_2 = param_2 + _DAT_005012d0;
  if (param_2 < *(float *)((int)this + 8)) {
    param_2 = *(float *)((int)this + 8);
  }
  *(float *)((int)this + 8) = param_2;
  if (*(float *)((int)this + 0xc) <= param_3 + _DAT_005012d0) {
    *(float *)((int)this + 0xc) = param_3 + _DAT_005012d0;
    return;
  }
  *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)this + 0xc);
  return;
}


// ==== FUN_00487800 @ 00487800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00487800(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 local_8;
  undefined4 local_4;
  
  *(float *)this = *(float *)this - _DAT_0059a938;
  *(float *)((int)this + 4) = *(float *)((int)this + 4) - _DAT_0059a93c;
  local_8 = FUN_004e49c0();
  local_4 = FUN_004e49c0();
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  FUN_004c6b80(&local_8,uVar1,uVar2,param_4);
  return;
}


// ==== FUN_00487ba0 @ 00487ba0

void __thiscall
FUN_00487ba0(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  uVar3 = FUN_004e49c0();
  uVar4 = FUN_004e49c0();
  FUN_00499310(uVar1,uVar2,uVar3,uVar4,0);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  uVar3 = FUN_004e49c0();
  uVar4 = FUN_004e49c0();
  FUN_00499310(uVar1,uVar2,uVar3,uVar4,0);
  *(undefined4 *)this = param_2;
  *(undefined4 *)((int)this + 4) = param_3;
  *(undefined4 *)((int)this + 8) = param_4;
  *(undefined4 *)((int)this + 0xc) = param_5;
  return;
}


// ==== FUN_00455930 @ 00455930

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_00455930(float param_1,float param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (float10)(DAT_00532710 >> 0x10) * (float10)_DAT_005092cc *
         ABS((float10)param_2 - (float10)param_1) + (float10)param_1;
}


// ==== FUN_004a97e0 @ 004a97e0

void __cdecl FUN_004a97e0(int *param_1)

{
  int iVar1;
  
  if ((param_1 != (int *)0x0) && (*param_1 != 0)) {
    iVar1 = FUN_004a8a80(param_1);
    if (iVar1 != 0) {
      FUN_004a8ad0(param_1);
      return;
    }
  }
  return;
}


// ==== FUN_004a7db0 @ 004a7db0

undefined4 * __cdecl FUN_004a7db0(int param_1,int *param_2,int *param_3,int *param_4,int *param_5)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  undefined4 *puVar9;
  uint uVar10;
  int iVar11;
  int *piVar12;
  undefined4 *puVar13;
  undefined4 **ppuVar14;
  undefined4 *local_148;
  int local_144;
  int local_140;
  undefined4 local_13c;
  undefined4 local_138;
  undefined1 local_100 [256];
  
  uVar5 = (uint)*(byte *)(param_1 * 0x32 + 0x22 + DAT_005c8464);
  piVar12 = (int *)(param_1 * 0x32 + DAT_005c8464);
  iVar6 = FUN_004ac550(piVar12);
  if ((((((*(byte *)((int)piVar12 + 0xb) & 1) != 0) || (iVar8 = *piVar12, iVar8 == 0)) ||
       (iVar8 == -0x22222223)) || ((iVar8 == -0x32323233 || (piVar12[9] < 0)))) ||
     ((*(int *)(iVar6 + 0x114) <= piVar12[9] || (*(int *)(iVar6 + 0xdc) == 0)))) {
    uVar7 = FUN_004a89c0(local_100,"%d %d %d %d %d",param_1,piVar12[9],
                         *(undefined1 *)((int)piVar12 + 10),*(byte *)((int)piVar12 + 0xb) & 1,
                         *(byte *)((int)piVar12 + 0xb) >> 1 & 1);
    FUN_00436d20(uVar7);
    FUN_004e0620(&DAT_0050161c,".\\Shapetobuffer.cpp",0x7b);
  }
  iVar8 = FUN_00419850(iVar6,0,piVar12[9]);
  *param_2 = *(short *)(iVar8 + 0x18) * DAT_0059a918 >> 0x10;
  *param_3 = *(short *)(iVar8 + 0x1a) * DAT_0059a918 >> 0x10;
  *param_4 = *(short *)(iVar8 + 0x1c) * DAT_0059a918 >> 0x10;
  *param_5 = *(short *)(iVar8 + 0x1e) * DAT_0059a918 >> 0x10;
  puVar9 = operator_new(*param_2 * *param_3);
  iVar8 = *param_2;
  iVar11 = *param_3;
  puVar13 = puVar9;
  for (uVar10 = (uint)(iVar8 * iVar11) >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
    *puVar13 = 0xffffffff;
    puVar13 = puVar13 + 1;
  }
  for (uVar10 = iVar8 * iVar11 & 3; uVar4 = DAT_005b5dec, uVar3 = DAT_005b5de8, uVar2 = DAT_005b5de4
      , uVar1 = DAT_005b5de0, uVar7 = DAT_005b5ddc, uVar10 != 0; uVar10 = uVar10 - 1) {
    *(undefined1 *)puVar13 = 0xff;
    puVar13 = (undefined4 *)((int)puVar13 + 1);
  }
  iVar8 = *param_3;
  ppuVar14 = &local_148;
  for (iVar11 = 0x12; iVar11 != 0; iVar11 = iVar11 + -1) {
    *ppuVar14 = (undefined4 *)0x0;
    ppuVar14 = ppuVar14 + 1;
  }
  local_144 = *param_2 + -1;
  local_13c = 1;
  local_138 = 1;
  local_140 = iVar8 + -1;
  DAT_005b5ddc = &local_148;
  DAT_005b5de0 = 0;
  DAT_005b5de4 = 0;
  DAT_005b5de8 = *param_2 + -1;
  DAT_005b5dec = *param_3 + -1;
  local_148 = puVar9;
  iVar8 = FUN_004ac550(piVar12);
  if ((*(byte *)(piVar12 + 10) & 2) == 0) {
    if ((*(int *)(&DAT_00557940 + (uint)*(byte *)((int)piVar12 + 10) * 4) == 0) || (uVar5 == 0)) {
      FUN_004daa70(*(undefined4 *)(iVar8 + 0xdc),piVar12[9],*param_4,*param_5);
    }
    else {
      FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar5 * 4) * 0x100 +
                   *(int *)(&DAT_00557940 + (uint)*(byte *)((int)piVar12 + 10) * 4));
      FUN_004daab0(*(undefined4 *)(iVar8 + 0xdc),piVar12[9],*param_4,*param_5);
    }
  }
  else {
    iVar11 = (**(code **)(*piVar12 + 0x78))();
    if (iVar11 != -1) {
      if ((*(int *)(&DAT_00557940 + (uint)*(byte *)((int)piVar12 + 10) * 4) == 0) || (uVar5 == 0)) {
        FUN_004daa70(*(undefined4 *)(iVar6 + 0xdc),iVar11,*param_4,*param_5);
      }
      else {
        FUN_004dabe0(*(int *)(&DAT_00531b08 + uVar5 * 4) * 0x100 +
                     *(int *)(&DAT_00557940 + (uint)*(byte *)((int)piVar12 + 10) * 4));
        FUN_004daab0(*(undefined4 *)(iVar8 + 0xdc),piVar12[9],*param_4,*param_5);
      }
    }
    (**(code **)(*piVar12 + 0xa4))(*param_4,*param_5);
  }
  DAT_005b5ddc = (undefined4 **)uVar7;
  DAT_005b5de0 = uVar1;
  DAT_005b5de4 = uVar2;
  DAT_005b5de8 = uVar3;
  DAT_005b5dec = uVar4;
  if (DAT_00542414 != 0) {
    *param_4 = *param_4 + (*(int *)(iVar6 + 0x1dc) * DAT_0059a918 >> 0x10);
    *param_5 = *param_5 + (*(int *)(iVar6 + 0x1e0) * DAT_0059a918 >> 0x10);
  }
  return puVar9;
}


