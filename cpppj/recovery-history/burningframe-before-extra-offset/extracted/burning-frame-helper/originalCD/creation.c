// ==== FUN_0045e280 @ 0045e280

void __thiscall FUN_0045e280(void *this)

{
  undefined4 uVar1;
  
  uVar1 = DAT_005484ac;
  *(undefined4 *)this = DAT_005484a8;
  *(undefined4 *)((int)this + 4) = uVar1;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  return;
}


// ==== FUN_0045e2b0 @ 0045e2b0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0045e2b0(void *this)

{
  undefined4 uVar1;
  float10 fVar2;
  
  fVar2 = ((float10)(double)CONCAT44(DAT_005484ac,DAT_005484a8) - (float10)*(double *)this) *
          (float10)_DAT_00502588;
  *(float *)((int)this + 0x10) = (float)fVar2;
  *(float *)((int)this + 0x14) = (float)(fVar2 * (float10)_DAT_00565c4c);
  uVar1 = DAT_005484ac;
  *(undefined4 *)this = DAT_005484a8;
  *(undefined4 *)((int)this + 4) = uVar1;
  *(double *)((int)this + 8) =
       (double)(*(float *)((int)this + 0x10) + (float)*(double *)((int)this + 8));
  return;
}


// ==== FUN_0045c630 @ 0045c630

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0045c630(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 local_8;
  undefined4 local_4;
  
  *(float *)this = *(float *)this - _DAT_00565c50;
  *(float *)((int)this + 4) = *(float *)((int)this + 4) - _DAT_00565c54;
  local_8 = __ftol();
  local_4 = __ftol();
  uVar1 = __ftol();
  uVar2 = __ftol();
  FUN_004cdf30(&local_8,uVar1,uVar2,param_4);
  return;
}


// ==== FUN_0045ce80 @ 0045ce80

void __thiscall
FUN_0045ce80(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  
  uVar1 = __ftol();
  uVar2 = __ftol();
  uVar3 = __ftol();
  uVar4 = __ftol();
  FUN_00456d10(uVar1,uVar2,uVar3,uVar4,0);
  uVar1 = __ftol();
  uVar2 = __ftol();
  uVar3 = __ftol();
  uVar4 = __ftol();
  FUN_00456d10(uVar1,uVar2,uVar3,uVar4,0);
  *(undefined4 *)this = param_2;
  *(undefined4 *)((int)this + 4) = param_3;
  *(undefined4 *)((int)this + 8) = param_4;
  *(undefined4 *)((int)this + 0xc) = param_5;
  return;
}


// ==== FUN_0048cc50 @ 0048cc50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __cdecl FUN_0048cc50(float param_1,float param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return ABS((float10)param_2 - (float10)param_1) *
         (float10)(DAT_005308ec >> 0x10 & 0xffff) * (float10)_DAT_005045f8 + (float10)param_1;
}


// ==== FUN_004386f0 @ 004386f0

void __cdecl FUN_004386f0(int *param_1)

{
  int *piVar1;
  undefined1 *puStack_c;
  undefined1 local_4 [4];
  
  puStack_c = local_4;
  if (param_1 == (int *)0x0) {
    return;
  }
  if (*param_1 == 0) {
    return;
  }
  piVar1 = (int *)*param_1;
  if (piVar1 == DAT_0051a6fc) {
    return;
  }
  (**(code **)(*piVar1 + 0x24))(piVar1);
  if (((uint)puStack_c & 1) != 0) {
    if (*param_1 == 0) {
      FUN_00490040(s_buffer_0051abc8,s_sound_cpp_0051abd0,0x9b);
    }
    piVar1 = (int *)*param_1;
    if ((piVar1 == (int *)0x0) ||
       ((**(code **)(*piVar1 + 0x24))(piVar1,&puStack_c), ((uint)puStack_c & 2) != 0)) {
      FUN_00490040(s__isLost___0051abdc,s_sound_cpp_0051abd0,0x9c);
    }
    piVar1 = (int *)*param_1;
    if (piVar1 != DAT_0051a6fc) {
      (**(code **)(*piVar1 + 0x48))(piVar1);
      (**(code **)(*(int *)*param_1 + 0x34))((int *)*param_1,0);
      DAT_0051a738 = DAT_0051a738 + -1;
    }
    return;
  }
  return;
}


// ==== FUN_0045b100 @ 0045b100

/* WARNING: Removing unreachable block (ram,0x0045b42a) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 * __cdecl FUN_0045b100(int param_1,int *param_2,int *param_3,int *param_4,int *param_5)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int *piVar7;
  int iVar8;
  uint uVar9;
  int *piVar10;
  undefined4 *puVar11;
  undefined4 **ppuVar12;
  int *piVar13;
  int local_5c [5];
  undefined4 *local_48;
  int local_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  
  piVar7 = (int *)(DAT_005395dc + param_1 * 0x24);
  uVar6 = (uint)*(byte *)(piVar7 + 8);
  iVar1 = FUN_004abf50(piVar7);
  if ((((((*(byte *)((int)piVar7 + 0xb) & 1) == 0) && (iVar3 = *piVar7, iVar3 != 0)) &&
       (iVar3 != -0x22222223)) &&
      ((iVar3 != -0x32323233 && ((int)(uint)*(byte *)((int)piVar7 + 0x22) < *(int *)(iVar1 + 0x114))
       ))) && ((iVar1 != 0 && (iVar3 = *(int *)(iVar1 + 0xdc), iVar3 != 0)))) {
    iVar4 = *(int *)(iVar3 + 8 + (uint)*(byte *)((int)piVar7 + 0x22) * 8);
    iVar8 = *(short *)(iVar4 + -0xc + iVar3) * DAT_00565c30 >> 0x10;
    iVar3 = iVar4 + -0x24 + iVar3;
    iVar4 = *(short *)(iVar3 + 0x1a) * DAT_00565c30 >> 0x10;
    *param_2 = iVar8;
    *param_3 = iVar4;
    *param_4 = *(short *)(iVar3 + 0x1c) * DAT_00565c30 >> 0x10;
    *param_5 = *(short *)(iVar3 + 0x1e) * DAT_00565c30 >> 0x10;
    puVar2 = (undefined4 *)FUN_004f1650(iVar4 * iVar8);
    local_5c[0] = *param_3;
    iVar3 = *param_2;
    uVar9 = local_5c[0] * iVar3;
    puVar11 = puVar2;
    for (uVar5 = uVar9 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *puVar11 = 0xffffffff;
      puVar11 = puVar11 + 1;
    }
    for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined1 *)puVar11 = 0xff;
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
    ppuVar12 = &local_48;
    for (iVar4 = 0x12; iVar4 != 0; iVar4 = iVar4 + -1) {
      *ppuVar12 = (undefined4 *)0x0;
      ppuVar12 = ppuVar12 + 1;
    }
    local_3c = 1;
    local_38 = 1;
    local_40 = local_5c[0] + -1;
    piVar10 = (int *)&DAT_0055bb18;
    piVar13 = local_5c;
    local_48 = puVar2;
    local_44 = iVar3 + -1;
    for (iVar4 = 5; iVar4 != 0; iVar4 = iVar4 + -1) {
      *piVar13 = *piVar10;
      piVar10 = piVar10 + 1;
      piVar13 = piVar13 + 1;
    }
    DAT_0055bb18 = &local_48;
    DAT_0055bb1c = 0;
    _DAT_0055bb20 = 0;
    _DAT_0055bb24 = *param_2 + -1;
    _DAT_0055bb28 = *param_3 + -1;
    iVar3 = FUN_004abf50(piVar7);
    if ((*(byte *)((int)piVar7 + 0x23) & 2) == 0) {
      if ((*(int *)(&DAT_005365c8 + (uint)*(byte *)((int)piVar7 + 10) * 4) == 0) || (uVar6 == 0)) {
        FUN_004d64e0(*(undefined4 *)(iVar3 + 0xdc),*(undefined1 *)((int)piVar7 + 0x22),*param_4,
                     *param_5);
      }
      else {
        FUN_004d6630((&DAT_005365a0)[uVar6] * 0x100 +
                     *(int *)(&DAT_005365c8 + (uint)*(byte *)((int)piVar7 + 10) * 4));
        FUN_004d6510(*(undefined4 *)(iVar3 + 0xdc),*(undefined1 *)((int)piVar7 + 0x22),*param_4,
                     *param_5);
      }
    }
    else {
      iVar4 = (**(code **)(*piVar7 + 0x78))();
      if (iVar4 != -1) {
        if ((*(int *)(&DAT_005365c8 + (uint)*(byte *)((int)piVar7 + 10) * 4) == 0) || (uVar6 == 0))
        {
          FUN_004d64e0(*(undefined4 *)(iVar1 + 0xdc),iVar4,*param_4,*param_5);
        }
        else {
          FUN_004d6630((&DAT_005365a0)[uVar6] * 0x100 +
                       *(int *)(&DAT_005365c8 + (uint)*(byte *)((int)piVar7 + 10) * 4));
          FUN_004d6510(*(undefined4 *)(iVar3 + 0xdc),*(undefined1 *)((int)piVar7 + 0x22),*param_4,
                       *param_5);
        }
      }
      (**(code **)(*piVar7 + 0xa4))(*param_4,*param_5);
    }
    piVar7 = local_5c;
    piVar10 = (int *)&DAT_0055bb18;
    for (iVar3 = 5; iVar3 != 0; iVar3 = iVar3 + -1) {
      *piVar10 = *piVar7;
      piVar7 = piVar7 + 1;
      piVar10 = piVar10 + 1;
    }
    if (DAT_005207b0 != 0) {
      *param_4 = *param_4 + (*(int *)(iVar1 + 0x1bc) * DAT_00565c30 >> 0x10);
      *param_5 = *param_5 + (*(int *)(iVar1 + 0x1c0) * DAT_00565c30 >> 0x10);
    }
    return puVar2;
  }
  *param_2 = 8;
  *param_3 = 8;
  *param_4 = 4;
  *param_5 = 4;
  puVar2 = (undefined4 *)FUN_004f1650(0x40);
  iVar1 = *param_3;
  iVar3 = *param_2;
  puVar11 = puVar2;
  for (uVar6 = (uint)(iVar1 * iVar3) >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
    *puVar11 = 0xffffffff;
    puVar11 = puVar11 + 1;
  }
  for (uVar6 = iVar1 * iVar3 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
    *(undefined1 *)puVar11 = 0xff;
    puVar11 = (undefined4 *)((int)puVar11 + 1);
  }
  return puVar2;
}


