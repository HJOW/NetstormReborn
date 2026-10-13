// ==== FUN_004950f0 @ 004950f0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004950f0(int param_1,uint param_2)

{
  float fVar1;
  int iVar2;
  void *pvVar3;
  uint uVar4;
  int iVar5;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_004fb25b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    ExceptionList = &pvStack_c;
    if ((param_2 & 1) != 0) {
      ExceptionList = &pvStack_c;
      FUN_00414450((param_1 - DAT_005c8464) / 0x32);
      iVar2 = FUN_004afb40(0x25a);
      if (iVar2 == 0) {
        pvVar3 = operator_new(0x28);
        uStack_4 = 0;
        if (pvVar3 != (void *)0x0) {
          iVar5 = param_1 - DAT_005c8464;
          FUN_0049a840(DAT_005412d0);
          iVar2 = FUN_0049fce0();
          fVar1 = (float)(iVar2 / 6) * _DAT_0050f20c;
          iVar2 = FUN_004adcb0();
          FUN_00496e80(0x25a,iVar5 / 0x32,fVar1 / (float)iVar2);
        }
        uStack_4 = 0xffffffff;
      }
    }
    if ((((*(byte *)(param_1 + 0xc) & 0x7f) == *(byte *)(param_1 + 0x22)) &&
        (uVar4 = (uint)*(byte *)(param_1 + 0x22), uVar4 != 0)) && (uVar4 < 9)) {
      *(undefined4 *)(&DAT_00595428 + uVar4 * 4) = 0;
    }
  }
  FUN_00427720(param_2);
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    iVar2 = FUN_00427030();
    if (iVar2 != 0) {
      if ((((param_2 & 0x800) == 0) &&
          (uVar4 = FUN_0040d9d0(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12)),
          (uVar4 & 6) == 0)) && (iVar2 = FUN_004ad710(), iVar2 != 9)) {
        FUN_004941f0();
      }
      FUN_00493d30();
      ExceptionList = pvStack_c;
      return;
    }
    FUN_00491980();
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00414450 @ 00414450

void __thiscall FUN_00414450(int *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar1 = param_1[2];
  if (iVar1 < param_1[1]) {
    iVar2 = 0;
    if (0 < iVar1) {
      piVar3 = (int *)*param_1;
      do {
        if (*piVar3 == param_2) {
          return;
        }
        iVar2 = iVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (iVar2 < param_1[2]);
    }
    *(int *)(*param_1 + iVar1 * 4) = param_2;
    param_1[2] = param_1[2] + 1;
  }
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049fce0 @ 0049fce0

int __fastcall FUN_0049fce0(int *param_1)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)(param_1 + 0x3a) & 0x10) == 0)) &&
     (((int)param_1 - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)param_1 - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
  }
  return *param_1;
}


// ==== FUN_004adcb0 @ 004adcb0

undefined4 __fastcall FUN_004adcb0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) == 0) {
    FUN_0049a840(*(undefined1 *)(param_1 + 10));
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  if ((*(byte *)(param_1 + 0x28) & 1) != 0) {
    FUN_0049a840(DAT_005412e8);
    uVar2 = FUN_0049fce0();
    return uVar2;
  }
  return 0;
}


// ==== FUN_00427030 @ 00427030

undefined4 __fastcall FUN_00427030(int param_1)

{
  int iVar1;
  
  if ((*(byte *)(param_1 + 0x28) & 0x20) == 0) {
    iVar1 = FUN_00492090((param_1 - DAT_005c8464) / 0x32);
    if (iVar1 == 0) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_00492090 @ 00492090

undefined4 FUN_00492090(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = param_1 * 0x32 + DAT_005c8464;
  uVar1 = FUN_004ac200();
  if ((uVar1 & 0x200000) == 0) {
    return 0;
  }
  iVar3 = *(int *)(iVar4 + 0x1a);
  iVar2 = FUN_004adcb0();
  if (iVar2 / 2 <= iVar3) {
    iVar3 = FUN_004e49c0();
    iVar2 = FUN_004e49c0();
    if (((*(byte *)(iVar2 + iVar3 * 0x100 + DAT_005c7c44) & 6) != 0) &&
       ((*(byte *)(iVar4 + 0x28) & 0x20) == 0)) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_00494580 @ 00494580

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00494580(int *param_1,int param_2,int param_3,float param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  float10 fVar5;
  
  if (param_2 == 0x259) {
    if (param_3 != 0) {
      (**(code **)(*param_1 + 0x10))(0x100000);
      return (float10)_DAT_00502350;
    }
    return (float10)_DAT_00501908;
  }
  if (param_2 == 0x25a) {
    DAT_00540cb8 = FUN_004558c0(9);
    iVar3 = DAT_0054d4e0 + 3;
    if (0x2580 < DAT_0054d4e0 + 3) {
      iVar3 = DAT_0054d4e0 + -0x257d;
    }
    DAT_0054d4e0 = iVar3;
    iVar3 = *(int *)((int)param_1 + 0x1a);
    iVar1 = FUN_004adcb0();
    if (iVar3 < iVar1) {
      iVar3 = *(int *)((int)param_1 + 0x1a);
      FUN_0049a840(DAT_005412d0);
      iVar1 = FUN_0049fce0();
      iVar2 = FUN_004adcb0();
      if (iVar2 < iVar1 / 6 + iVar3) {
        iVar3 = FUN_004adcb0();
      }
      else {
        iVar3 = *(int *)((int)param_1 + 0x1a);
        FUN_0049a840(DAT_005412d0);
        iVar1 = FUN_0049fce0();
        iVar3 = iVar1 / 6 + iVar3;
      }
      FUN_004942b0(iVar3);
      (**(code **)(*param_1 + 0x88))();
      FUN_0044c060();
    }
    FUN_00491680();
    iVar3 = FUN_00491a80();
    if (iVar3 != 0) {
      iVar3 = (int)param_1 - DAT_005c8464;
      uVar4 = FUN_00488fe0(*(undefined1 *)((int)param_1 + 10),0,0);
      iVar3 = FUN_004b1a90(iVar3 / 0x32,uVar4);
      if (iVar3 == 0) {
        FUN_00491db0();
      }
    }
    if (DAT_00568af8 != DAT_0054d4e0) {
      DAT_005318ec = DAT_00540cb8;
    }
    if (DAT_005c89c4 != 0) {
      fVar5 = (float10)FUN_0040ee90();
      if ((float10)_DAT_00505514 < fVar5) {
        DAT_005318ec = DAT_00540cb8;
      }
      DAT_005c89c4 = DAT_005c89c4 + -1;
    }
    if (DAT_00594fd0 == 0) {
      DAT_005318ec = DAT_00540cb8;
    }
    if (DAT_00531890 == -1) {
      DAT_005318ec = DAT_00540cb8;
    }
LAB_00494737:
    return (float10)param_4;
  }
  if (param_2 != 0x25d) {
    if (param_2 == 0x25b) {
      fVar5 = (float10)FUN_00493e60();
      return fVar5;
    }
    if (param_2 == 0x25c) {
      fVar5 = (float10)FUN_00494160();
      return fVar5;
    }
    fVar5 = (float10)FUN_0042a750(param_2,param_3,param_4);
    return fVar5;
  }
  *(int *)((int)param_1 + 0x2e) = param_3;
  iVar3 = (int)param_1 - DAT_005c8464;
  uVar4 = FUN_00488fe0(*(undefined1 *)((int)param_1 + 10),0,0);
  iVar3 = FUN_004b1a90(iVar3 / 0x32,uVar4);
  iVar1 = FUN_00427030();
  if ((iVar1 == 0) && (iVar3 == 0)) {
    iVar3 = FUN_0049a840(*(undefined4 *)((int)param_1 + 0x2a));
    if (param_3 != *(int *)(iVar3 + 0x1bc)) goto LAB_00494737;
    if (DAT_00540bc4 != 0) {
      iVar3 = FUN_004acac0(1,DAT_005412f8,0);
      if (iVar3 == 0) {
        FUN_004264d0(((int)param_1 - DAT_005c8464) / 0x32);
        FUN_004afa70(*(undefined4 *)((int)param_1 + 0x2a),0,1,0,0);
      }
    }
    uVar4 = 0;
    iVar3 = FUN_004ac550(0);
    FUN_004acee0(*(undefined4 *)(iVar3 + 0x118),uVar4);
  }
  return (float10)_DAT_00500fc4;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong FUN_004e49c0(void)

{
  ulonglong uVar1;
  uint uVar2;
  float fVar3;
  float10 in_ST0;
  undefined4 local_20;
  undefined4 uStack_1c;
  
  uVar1 = (ulonglong)ROUND(in_ST0);
  local_20 = (uint)uVar1;
  uStack_1c = (float)(uVar1 >> 0x20);
  fVar3 = (float)in_ST0;
  if ((local_20 != 0) || (fVar3 = uStack_1c, (uVar1 & 0x7fffffff00000000) != 0)) {
    if ((int)fVar3 < 0) {
      uVar1 = uVar1 + (0x80000000 < (uint)-(float)(in_ST0 - (float10)(longlong)uVar1));
    }
    else {
      uVar2 = (uint)(0x80000000 < (uint)(float)(in_ST0 - (float10)(longlong)uVar1));
      uVar1 = CONCAT44((int)uStack_1c - (uint)(local_20 < uVar2),local_20 - uVar2);
    }
  }
  return uVar1;
}


// ==== FUN_004942b0 @ 004942b0

void __thiscall FUN_004942b0(int *param_1,undefined4 param_2)

{
  undefined4 uVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = *(int *)((int)param_1 + 0x1a);
  iVar3 = FUN_004adcb0();
  uVar1 = *(undefined4 *)((int)param_1 + 0x1a);
  bVar2 = iVar4 < iVar3 / 2;
  FUN_004adca0(param_2);
  iVar4 = *(int *)((int)param_1 + 0x1a);
  iVar3 = FUN_004adcb0();
  if (iVar4 < iVar3 / 2 != bVar2) {
    if (DAT_00540bc4 != 0) {
      if ((bVar2) &&
         (*(int *)((uint)*(byte *)((int)param_1 + 0x22) * 0xb4 + 0x7c + DAT_0059534c) == 0)) {
        FUN_004adca0(uVar1);
        return;
      }
      (**(code **)(*param_1 + 0x48))(0);
      iVar4 = *(int *)((int)param_1 + 0x1a);
      iVar3 = FUN_004adcb0();
      if (iVar4 < iVar3 / 2) {
        FUN_0041d7a0();
      }
      (**(code **)(*param_1 + 0x4c))(0);
      return;
    }
    iVar4 = FUN_004adcb0();
    iVar4 = iVar4 / 2;
    if (bVar2) {
      iVar4 = iVar4 + -1;
    }
    FUN_004adca0(iVar4);
  }
  return;
}


// ==== FUN_004adca0 @ 004adca0

void __thiscall FUN_004adca0(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_0041d7a0 @ 0041d7a0

void __fastcall FUN_0041d7a0(float *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *param_1 = (float)iVar1;
  iVar1 = FUN_004e49c0();
  param_1[1] = (float)iVar1;
  return;
}


// ==== FUN_0040d9d0 @ 0040d9d0

undefined1 FUN_0040d9d0(void)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  return *(undefined1 *)(iVar2 + iVar1 * 0x100 + DAT_005c7c44);
}


// ==== FUN_004ad710 @ 004ad710

int __fastcall FUN_004ad710(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)(param_1 + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4) + -0x41;
}


// ==== FUN_004941f0 @ 004941f0

void __fastcall FUN_004941f0(int *param_1)

{
  void *pvVar1;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004fb25b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  FUN_00493d30();
  pvVar1 = operator_new(0x28);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    FUN_00496f00(0x25b,((int)param_1 - DAT_005c8464) / 0x32,0x3c23d70a);
  }
  local_4 = 0xffffffff;
  if (DAT_00540bc4 != 0) {
    (**(code **)(*param_1 + 0x48))(0);
    (**(code **)(*param_1 + 0x4c))(0x800);
  }
  FUN_004a9d70(*(undefined4 *)((int)param_1 + 0xe),*(undefined4 *)((int)param_1 + 0x12),
               "priestFall.wav",0);
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00493d30 @ 00493d30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00493d30(int param_1)

{
  int iVar1;
  int *piVar2;
  void *pvVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined1 auStack_10c [256];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_004fb21e;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  iVar1 = FUN_004918e0();
  if (iVar1 == 0) {
    piVar2 = (int *)FUN_004af530(DAT_005412f4,2);
    uVar4 = *(undefined4 *)(param_1 + 0xe);
    uVar5 = *(undefined4 *)(param_1 + 0x12);
    (**(code **)(*piVar2 + 0x74))(*(undefined1 *)(param_1 + 0x22));
    (**(code **)(*piVar2 + 0x90))(uVar4,uVar5,0);
    pvVar3 = operator_new(0x28);
    uStack_4 = 0;
    if (pvVar3 != (void *)0x0) {
      iVar1 = (int)piVar2 - DAT_005c8464;
      uVar6 = 1;
      uVar5 = 0;
      uVar4 = thunk_FUN_004a8d50("priestForceField.wav",0,1);
      FUN_004ab240(iVar1 / 0x32,uVar4,uVar5,uVar6);
    }
    uStack_4 = 0xffffffff;
    if (((*(byte *)(param_1 + 0x22) == DAT_00540c70) && (DAT_005c89b8 == 0)) &&
       (_DAT_005c85f0 == _DAT_00500460)) {
      FUN_004a9cb0("ourPriestImmobile.wav",0,0,0,1,0);
      uVar4 = FUN_004a89c0(auStack_10c,"PriestImmobile");
      FUN_004cf960(uVar4);
    }
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_00491980 @ 00491980

void FUN_00491980(void)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = FUN_004918e0();
  piVar2 = (int *)(iVar1 * 0x32 + DAT_005c8464);
  if (((int *)(DAT_005c8464 + 0x32U) <= piVar2) &&
     (piVar2 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    (**(code **)(*piVar2 + 0x10))(0);
  }
  return;
}


