// ==== FUN_00443fe0 @ 00443fe0

void FUN_00443fe0(undefined4 param_1,undefined4 param_2)

{
  void *pvVar1;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004f9ecb;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  pvVar1 = operator_new(0x38);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    FUN_00443c40(param_1,param_2);
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_00443c40 @ 00443c40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 * __thiscall FUN_00443c40(undefined4 *param_1,undefined4 param_2,float param_3)

{
  byte bVar1;
  bool bVar2;
  double dVar3;
  undefined4 uVar4;
  void *pvVar5;
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  puStack_8 = &LAB_004f9eb6;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined8 *)(param_1 + 4) = _DAT_00500460;
  *(undefined8 *)(param_1 + 6) = _DAT_00500460;
  *(undefined1 *)(param_1 + 2) = 0xff;
  param_1[10] = &PTR_FUN_005019ec;
  param_1[0xc] = 0;
  param_1[0xb] = 0;
  bVar2 = param_3 == _DAT_00500fc4;
  local_4 = 0;
  *param_1 = &PTR_LAB_0050701c;
  param_1[10] = &PTR_LAB_00506ffc;
  if (bVar2) {
    FUN_00423840();
  }
  else {
    FUN_00423900(param_2);
  }
  uVar4 = DAT_0055b4d4;
  bVar1 = *(byte *)(param_1 + 2);
  bVar2 = param_3 == _DAT_00500fc4;
  param_1[4] = DAT_0055b4d0;
  *(byte *)(param_1 + 2) = bVar1 | 2;
  param_1[5] = uVar4;
  if (bVar2) {
    dVar3 = (double)CONCAT44(DAT_0055b4d4,DAT_0055b4d0) + _DAT_00501450;
    *(byte *)(param_1 + 2) = bVar1 | 6;
    param_3 = 10.0;
    *(double *)(param_1 + 6) = dVar3;
  }
  dVar3 = (double)CONCAT44(DAT_0055b4d4,DAT_0055b4d0);
  *(byte *)(param_1 + 2) = *(byte *)(param_1 + 2) | 4;
  *(double *)(param_1 + 6) = (double)(param_3 + (float)dVar3);
  FUN_0041bd20(DAT_005410e8,param_2,0,0x10);
  if (param_3 <= _DAT_005012d0) {
    param_1[0xd] = 0;
  }
  else {
    uVar4 = thunk_FUN_004a8d50("jimBuild.wav");
    param_1[0xd] = uVar4;
    pvVar5 = operator_new(0x28);
    local_4._0_1_ = 1;
    if (pvVar5 != (void *)0x0) {
      FUN_004ab240(param_2,param_1[0xd],0,1);
    }
    local_4 = (uint)local_4._1_3_ << 8;
  }
  FUN_004da650();
  ExceptionList = local_c;
  return param_1;
}


// ==== FUN_004428c0 @ 004428c0

void FUN_004428c0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int *piVar4;
  int iVar5;
  
  if (DAT_005e4794 != 0) {
    FUN_0041bfa0();
    iVar1 = FUN_004ae080();
    if (iVar1 == 0) {
      FUN_004e0620("getForm().isRobust()","\\Ns\\O\\Construction.cpp",0x1c4);
    }
  }
  FUN_0041bfb0();
  iVar2 = FUN_004adcb0();
  FUN_0041bfb0();
  iVar1 = FUN_004e49c0();
  iVar5 = iVar2;
  if (DAT_0055a49c == 0) {
    if (DAT_0055a498 != 0) {
      FUN_0041bfb0();
      uVar3 = FUN_004ac200();
      if ((uVar3 & 0x200) != 0) goto LAB_0044296f;
    }
    iVar5 = iVar1;
    if (iVar1 < iVar2) goto LAB_00442971;
  }
LAB_0044296f:
  iVar1 = iVar2;
LAB_00442971:
  if (iVar1 < 0) {
    iVar5 = 0;
  }
  else if (iVar2 <= iVar5) {
    iVar5 = iVar2;
  }
  iVar1 = iVar5;
  FUN_0041bfb0(iVar5);
  FUN_004adca0(iVar1);
  piVar4 = (int *)FUN_0041bfb0();
  (**(code **)(*piVar4 + 0x88))();
  iVar1 = FUN_0041bfb0();
  if ((*(byte *)(iVar1 + 0x28) & 1) == 0) {
    FUN_0041bf10(0);
  }
  if ((iVar5 == iVar2) && (DAT_00540bc4 != 0)) {
    FUN_0041bf10(0);
  }
  return;
}


// ==== FUN_00443e20 @ 00443e20

void __fastcall FUN_00443e20(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  undefined4 uVar6;
  
  FUN_0041bfb0();
  iVar1 = FUN_004adcb0();
  iVar2 = FUN_0041bfb0();
  iVar2 = *(int *)(iVar2 + 0x1a);
  FUN_004b1a90(*(undefined4 *)(param_1 + 0x30),DAT_0054112c,0,1);
  FUN_004da650();
  iVar3 = FUN_0041bfb0();
  if (*(byte *)(iVar3 + 0x22) == DAT_00540c70) {
    iVar3 = FUN_0041bfb0();
    iVar3 = (iVar3 - DAT_005c8464) / 0x32;
    if ((iVar3 < 5) || (14999 < iVar3)) {
      iVar3 = FUN_0041bfb0();
      FUN_004a9d70(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),"buildDone.wav",0);
      FUN_0041bfb0();
      iVar3 = FUN_004ac550();
      if (*(int *)(iVar3 + 0x90) != 0) {
        FUN_004a9550(*(int *)(iVar3 + 0x90),0,0,0,0,0);
      }
    }
  }
  if (DAT_00540bc4 != 0) {
    iVar3 = FUN_0041bfb0();
    iVar3 = (iVar3 - DAT_005c8464) / 0x32;
    if (((iVar3 < 5) || (14999 < iVar3)) && (iVar2 == iVar1)) {
      iVar2 = FUN_0041bfb0();
      if ((*(byte *)(iVar2 + 0x28) & 1) != 0) {
        piVar4 = (int *)FUN_0041bfb0();
        (**(code **)(*piVar4 + 0x48))(0);
        uVar6 = 0;
        FUN_0041bfb0(0);
        FUN_0042b050(uVar6);
        piVar4 = (int *)FUN_0041bfb0();
        (**(code **)(*piVar4 + 0x4c))(9);
        FUN_0041bfb0();
        uVar5 = FUN_004ac200();
        if ((DAT_005325b4 & uVar5) != 0) {
          iVar2 = FUN_0041bfb0();
          iVar1 = FUN_0041bfb0();
          iVar3 = FUN_0041bfb0();
          FUN_00443800(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),
                       *(undefined1 *)(iVar1 + 10),*(undefined1 *)(iVar2 + 0x22));
        }
      }
    }
  }
  return;
}


// ==== FUN_00444030 @ 00444030

void __fastcall FUN_00444030(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  FUN_00443e20();
  iVar2 = *(int *)(param_1 + 8) * 0x32;
  if (((*(byte *)(iVar2 + 0xb + DAT_005c8464) & 3) == 0) &&
     (*(byte *)(iVar2 + DAT_005c8464 + 10) != DAT_005412ec)) {
    FUN_0041bfb0();
    uVar1 = FUN_004adcb0();
    FUN_004adca0(uVar1);
  }
  return;
}


// ==== FUN_0041bfb0 @ 0041bfb0

int __fastcall FUN_0041bfb0(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 8) * 0x32 + DAT_005c8464;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_004e0620("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4);
  }
  return iVar1;
}


// ==== FUN_004adca0 @ 004adca0

void __thiscall FUN_004adca0(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004ac550 @ 004ac550

void __fastcall FUN_004ac550(int param_1)

{
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return;
}


// ==== FUN_0042b050 @ 0042b050

void __thiscall FUN_0042b050(int param_1,byte param_2)

{
  if ((*(byte *)(param_1 + 0xb) & 4) == 0) {
    FUN_004e0620("isVoid()","c:\\ns\\o\\squid.h",0x282);
  }
  *(byte *)(param_1 + 0x28) = *(byte *)(param_1 + 0x28) ^ (*(byte *)(param_1 + 0x28) ^ param_2) & 1;
  return;
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


