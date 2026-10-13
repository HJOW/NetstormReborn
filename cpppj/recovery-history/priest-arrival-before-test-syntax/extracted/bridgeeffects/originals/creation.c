// ==== FUN_004221b0 @ 004221b0

void __thiscall FUN_004221b0(int param_1,undefined4 param_2)

{
  int local_c;
  
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    FUN_00422230(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),1,1,0);
    while (local_c != 0) {
      if (*(byte *)(local_c * 0x32 + DAT_005c8464 + 10) == DAT_005412c4) {
        FUN_00422290();
      }
      FUN_004b1810();
    }
  }
  FUN_004b0950(param_2);
  return;
}


// ==== FUN_00422290 @ 00422290

void __fastcall FUN_00422290(int *param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar1 = (uint)*(ushort *)(param_1 + 3) * 0x32 + DAT_005c8464;
  uVar2 = (uint)*(ushort *)(param_1 + 2) * 0x32 + DAT_005c8464;
  if ((((DAT_005c8464 + 0x32U <= uVar1) &&
       (uVar3 = DAT_005c8464 + DAT_005c847c * 0x32, uVar1 < uVar3)) &&
      (DAT_005c8464 + 0x32U <= uVar2)) &&
     (((uVar2 < uVar3 && ((*(byte *)(uVar1 + 0x28) & 9) == 0)) &&
      (((*(byte *)(uVar1 + 0xb) & 2) != 0 || ((*(byte *)(uVar2 + 0xb) & 2) != 0)))))) {
    (**(code **)(*param_1 + 0x10))(0);
  }
  return;
}


// ==== FUN_00422300 @ 00422300

void __thiscall FUN_00422300(int param_1,undefined4 param_2)

{
  uint uVar1;
  int local_c;
  
  if ((*(byte *)(param_1 + 0x28) & 9) == 0) {
    FUN_00460600((param_1 - DAT_005c8464) / 0x32);
    FUN_004a9d70(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),"bridgeFall.wav",0);
    FUN_004202f0(*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),0);
    while (local_c != 0) {
      uVar1 = FUN_004ac200();
      if ((uVar1 & 0x10000) != 0) {
        (**(code **)(*(int *)(local_c * 0x32 + DAT_005c8464) + 200))();
      }
      FUN_004b1810();
    }
  }
  FUN_004b0840(param_2);
  return;
}


// ==== FUN_00421f90 @ 00421f90

void FUN_00421f90(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  
  if (DAT_005e4794 != 0) {
    uVar1 = FUN_004ac200();
    if ((uVar1 & 4) == 0) {
      FUN_004e0620("s.getGenus() & gBRIDGE","\\Ns\\O\\Bridge.cpp",0x323);
    }
  }
  FUN_00421530(param_2,param_3);
  return;
}


// ==== FUN_00421530 @ 00421530

void __fastcall FUN_00421530(int param_1)

{
  uint uVar1;
  int iVar2;
  void *pvVar3;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004f94cb;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  uVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  pvVar3 = operator_new(0x28);
  local_4 = 0;
  if (pvVar3 != (void *)0x0) {
    FUN_00496e80(0x2692,(param_1 - DAT_005c8464) / 0x32,(float)(int)(uVar1 & 0xff | iVar2 << 8));
  }
  ExceptionList = local_c;
  return;
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


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00422230 @ 00422230

undefined4 * __thiscall
FUN_00422230(undefined4 *param_1,undefined4 param_2,undefined4 param_3,int param_4,int param_5,
            uint param_6)

{
  int iVar1;
  int iVar2;
  
  param_1[0xe] = param_6 & 1;
  *param_1 = &PTR_LAB_00502318;
  param_1[0xf] = param_6 & 4;
  FUN_0041d7a0();
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  FUN_004b16d0(iVar2 - param_4,iVar1 - param_5,iVar2 + param_4,iVar1 + param_5);
  return param_1;
}


// ==== FUN_004202f0 @ 004202f0

undefined4 * __thiscall
FUN_004202f0(undefined4 *param_1,undefined4 param_2,undefined4 param_3,uint param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  param_1[0xe] = param_4 & 1;
  *param_1 = &PTR_LAB_00502318;
  param_1[0xf] = param_4 & 4;
  FUN_0041d770();
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  FUN_004b16d0(uVar2,uVar1,uVar2,uVar1);
  return param_1;
}


// ==== FUN_0041d770 @ 0041d770

void __fastcall FUN_0041d770(float *param_1)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *param_1 = (float)iVar1;
  iVar1 = FUN_004e49c0();
  param_1[1] = (float)iVar1;
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


