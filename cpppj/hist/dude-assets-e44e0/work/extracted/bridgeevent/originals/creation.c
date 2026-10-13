// ==== FUN_00422740 @ 00422740

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00422740(int *param_1,int param_2,undefined4 param_3,float param_4)

{
  uint uVar1;
  
  if (param_2 == 0x2691) {
    if (DAT_00540bc4 != 0) {
      (**(code **)(*param_1 + 0x10))(0);
      return (float10)_DAT_00502350;
    }
  }
  else {
    if (param_2 != 0x2692) {
      return (float10)param_4;
    }
    if (DAT_00540bc4 != 0) {
      uVar1 = FUN_004e49c0();
      FUN_004215d0((float)(uVar1 & 0xff),(float)((int)uVar1 >> 8));
      return (float10)_DAT_00502350;
    }
  }
  return (float10)_DAT_00500fc4;
}


// ==== FUN_004215d0 @ 004215d0

void __thiscall FUN_004215d0(int *param_1,float param_2,float param_3)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  int local_cc;
  
  if ((DAT_0054db80 == 0) && (DAT_00540bc4 != 0)) {
    fVar1 = *(float *)((int)param_1 + 0x12);
    fVar2 = *(float *)((int)param_1 + 0xe);
    iVar6 = FUN_004ac550();
    iVar7 = FUN_004ad680();
    if (iVar7 == 0x4a) {
      if (param_3 <= fVar1) {
        iVar7 = 0x4c;
      }
      else {
        iVar7 = 0x4e;
      }
    }
    else {
      if (iVar7 != 0x4b) {
        return;
      }
      iVar7 = 0x4d;
      if (param_2 <= fVar2) {
        iVar7 = 0x4f;
      }
    }
    iVar7 = *(int *)(iVar6 + 0x2c + iVar7 * 4);
    iVar3 = param_1[9];
    param_1[9] = iVar7;
    FUN_004b23e0(((int)param_1 - DAT_005c8464) / 0x32,0);
    param_1[9] = iVar3;
    if (local_cc == 0) {
      (**(code **)(*param_1 + 0x10))(0);
      return;
    }
    *(ushort *)(param_1 + 3) =
         *(ushort *)(param_1 + 3) ^
         (byte)((char)DAT_0052f960 * '\b' - 8U ^ *(byte *)(param_1 + 3)) & 0x78;
    FUN_004214a0(((int)param_1 - DAT_005c8464) / 0x32);
    if ((*(byte *)(*(int *)(iVar6 + 0x124) + 3 + param_1[9] * 4) & 0x40) == 0) {
      piVar8 = (int *)FUN_004af530(*(undefined1 *)((int)param_1 + 10),0);
      piVar8[9] = iVar7;
      (**(code **)(*piVar8 + 0x74))(*(undefined1 *)((int)param_1 + 0x22));
      FUN_004ac220((short)param_1[3]);
      *(short *)(piVar8 + 2) = (short)param_1[2];
      *(byte *)(piVar8 + 10) =
           *(byte *)(piVar8 + 10) ^ (*(byte *)(param_1 + 10) ^ *(byte *)(piVar8 + 10)) & 0x10;
      uVar4 = *(undefined4 *)((int)param_1 + 0xe);
      uVar5 = *(undefined4 *)((int)param_1 + 0x12);
      (**(code **)(*param_1 + 0x10))(0);
      (**(code **)(*piVar8 + 0x90))(uVar4,uVar5,0);
    }
  }
  return;
}


// ==== FUN_004ac550 @ 004ac550

void __fastcall FUN_004ac550(int param_1)

{
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004ad680 @ 004ad680

int __fastcall FUN_004ad680(int param_1)

{
  int iVar1;
  int iVar2;
  
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x854);
  }
  iVar1 = *(int *)(param_1 + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
}


// ==== FUN_004ac220 @ 004ac220

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004ac220(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xc) = param_2;
  if (*(byte *)(param_1 + 10) == _DAT_005411a0) {
    FUN_004214a0((param_1 - DAT_005c8464) / 0x32);
  }
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


// ==== FUN_0049b060 @ 0049b060

void __fastcall FUN_0049b060(int param_1)

{
  int *piVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  
  iVar4 = 0;
  *(undefined4 *)(param_1 + 300) = 0;
  piVar1 = (int *)(param_1 + 0x170);
  do {
    piVar1[-0x10] = -1;
    *piVar1 = 0;
    pcVar3 = *(char **)(param_1 + 0x124);
    iVar2 = 0;
    if (0 < *(int *)(param_1 + 0x114)) {
      do {
        if ((int)*pcVar3 == iVar4 + 0x41) {
          if (piVar1[-0x10] == -1) {
            *(int *)(param_1 + 300) = *(int *)(param_1 + 300) + 1;
            piVar1[-0x10] = iVar2;
          }
          *piVar1 = *piVar1 + 1;
        }
        iVar2 = iVar2 + 1;
        pcVar3 = pcVar3 + 4;
      } while (iVar2 < *(int *)(param_1 + 0x114));
    }
    iVar4 = iVar4 + 1;
    piVar1 = piVar1 + 1;
  } while (iVar4 < 0x10);
  return;
}


