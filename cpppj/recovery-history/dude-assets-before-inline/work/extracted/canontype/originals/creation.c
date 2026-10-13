// ==== FUN_00425c20 @ 00425c20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * __thiscall
FUN_00425c20(int *param_1,int param_2,int param_3,int param_4,int param_5,int param_6,int param_7)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int extraout_ECX;
  
  iVar3 = param_2;
  param_1[0x14] = param_7;
  param_1[2] = param_4 / 2;
  *param_1 = param_2;
  param_1[1] = param_3;
  param_1[3] = 0;
  param_1[0xb] = param_5;
  param_1[0xc] = param_6;
  param_1[0xd] = 0;
  param_1[7] = 1;
  if (param_2 == DAT_00541204) {
    param_1[0xd] = (int)(&DAT_005300f0 + param_3 * 0x48);
  }
  else if (param_2 == _DAT_005411a0) {
    param_1[0xd] = (int)(&DAT_0052f998 + param_3 * 0x12);
  }
  else if (param_2 == DAT_005411d0) {
    param_1[0xd] = (int)(&DAT_00531410 + param_3 * 0x48);
  }
  else if (param_2 == DAT_005412cc) {
    param_1[0xd] = (int)(&DAT_005314a0 + param_3 * 0x48);
  }
  else {
    iVar2 = FUN_00425830(param_2);
    if (iVar2 == 0) {
      param_1[0xd] = 0;
      param_2 = 1;
    }
    else {
      param_1[0xd] = (int)(&DAT_005314e8 + extraout_ECX * 0x48);
    }
  }
  iVar3 = FUN_0049a840(iVar3);
  param_1[10] = iVar3;
  iVar3 = param_1[0xd];
  iVar2 = param_2;
  if (iVar3 != 0) {
    param_2 = *(int *)(iVar3 + 8);
    iVar2 = *(int *)(iVar3 + 4);
  }
  uVar1 = param_1[2];
  iVar3 = iVar2;
  if ((uVar1 & 1) == 0) {
    iVar3 = param_2;
  }
  param_1[0xe] = iVar3;
  iVar3 = param_2;
  if ((uVar1 & 1) == 0) {
    iVar3 = iVar2;
  }
  param_1[0xf] = iVar3;
  iVar3 = (iVar2 + -1) * *(int *)(&DAT_00531570 + uVar1 * 4);
  param_1[0x10] = iVar3;
  param_1[0x11] = (param_2 + -1) * *(int *)(&DAT_00531580 + uVar1 * 4);
  if (param_1[3] != 0) {
    param_1[0x10] = (iVar2 - iVar3) + -1;
  }
  param_1[0x13] = 0;
  param_1[0x12] = 0;
  param_1[5] = 1;
  FUN_00425860();
  return param_1;
}


// ==== FUN_00425860 @ 00425860

void __fastcall FUN_00425860(undefined4 *param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 local_14;
  int local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if ((int)param_1[0xf] <= (int)param_1[0x13]) {
      iVar4 = *(int *)(&DAT_00531530 + param_1[2] * 4);
      iVar1 = iVar4;
      if (param_1[3] != 0) {
        iVar1 = -iVar4;
      }
      if (iVar4 == 0) {
        iVar4 = param_1[0x10];
      }
      else {
        iVar4 = (iVar1 + param_1[0xe] + param_1[0x10]) % (int)param_1[0xe];
      }
      param_1[0x10] = iVar4;
      if (*(int *)(&DAT_00531540 + param_1[2] * 4) == 0) {
        iVar4 = param_1[0x11];
      }
      else {
        iVar4 = (*(int *)(&DAT_00531540 + param_1[2] * 4) + param_1[0xe] + param_1[0x11]) %
                (int)param_1[0xe];
      }
      param_1[0x11] = iVar4;
      iVar4 = param_1[0x12];
      param_1[0x12] = iVar4 + 1;
      if ((int)param_1[0xe] <= iVar4 + 1) {
        param_1[5] = 0;
        return;
      }
      param_1[0x13] = 0;
    }
    param_1[4] = 0xffffffff;
    if (param_1[0xd] == 0) {
      if (param_1[0x14] == 0) {
        param_1[4] = *(undefined4 *)(param_1[10] + 0x118);
      }
      else {
        param_1[4] = param_1[1];
        if (-1 < (int)param_1[1]) {
          iVar4 = FUN_0049a840(*param_1);
          if ((int)param_1[4] < *(int *)(iVar4 + 0x114)) goto LAB_004259a2;
        }
        FUN_004e0620("0 <= frame && frame < ::getType(type)->numFrames","\\Ns\\O\\Canondecoder.cpp",
                     0x305);
      }
    }
    else {
      iVar4 = FUN_00425700(param_1[0x10],param_1[0x11],&local_10,&local_14,param_1 + 6);
      if (iVar4 != 0) {
        uVar2 = param_1[2];
        if ((param_1[3] != 0) && (uVar2 = uVar2 + 2 & 0x80000003, (int)uVar2 < 0)) {
          uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
        }
        uVar3 = FUN_0049a940((&DAT_0053148c)[(uVar2 * 0x10 + local_10) * 4],
                             (&DAT_005315cc)[uVar2 * 0x40],local_14);
        param_1[4] = uVar3;
      }
    }
LAB_004259a2:
    if (((DAT_005e4794 != 0) && (param_1[4] != -1)) &&
       ((*(byte *)(*(int *)(param_1[10] + 0x124) + 3 + param_1[4] * 4) & 0x20) != 0)) {
      FUN_004e0620("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)",
                   "\\Ns\\O\\Canondecoder.cpp",0x325);
    }
    local_c = param_1[0x12] * param_1[7];
    local_8 = (float)(int)(param_1[7] * param_1[0x13]) + (float)param_1[0xb];
    param_1[8] = local_8;
    local_4 = (float)local_c + (float)param_1[0xc];
    param_1[9] = local_4;
    iVar4 = *(int *)(&DAT_00531550 + param_1[2] * 4);
    iVar1 = iVar4;
    if (param_1[3] != 0) {
      iVar1 = -iVar4;
    }
    if (iVar4 == 0) {
      iVar4 = param_1[0x10];
    }
    else {
      iVar4 = (iVar1 + param_1[0xf] + param_1[0x10]) % (int)param_1[0xf];
    }
    param_1[0x10] = iVar4;
    if (*(int *)(&DAT_00531560 + param_1[2] * 4) == 0) {
      iVar4 = param_1[0x11];
    }
    else {
      iVar4 = (*(int *)(&DAT_00531560 + param_1[2] * 4) + param_1[0xf] + param_1[0x11]) %
              (int)param_1[0xf];
    }
    param_1[0x11] = iVar4;
    param_1[0x13] = param_1[0x13] + 1;
    if (param_1[4] != -1) {
      return;
    }
  } while( true );
}


// ==== FUN_00425830 @ 00425830

undefined4 FUN_00425830(int param_1)

{
  if ((((param_1 != DAT_00541264) && (param_1 != DAT_0054125c)) && (param_1 != DAT_00541288)) &&
     (param_1 != DAT_00541290)) {
    return 0;
  }
  return 1;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00425700 @ 00425700

undefined4 __thiscall
FUN_00425700(int param_1,int param_2,int param_3,uint *param_4,int *param_5,uint *param_6)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = *(uint *)(param_1 + 0xc + (*(int *)(param_1 + 4) * param_3 + param_2) * 4);
  uVar2 = uVar1 >> 0x18;
  *param_6 = uVar2;
  if ((uVar2 != 0) && ((uVar2 < 0x61 || (0x7a < uVar2)))) {
    FUN_004e0620("num==0 || (\'a\' <= num && num <= \'z\')","\\Ns\\O\\Canondecoder.cpp",0x263);
  }
  *param_6 = *param_6 - 0x61;
  *param_4 = uVar1 >> 8 & 0xff;
  *param_5 = (uVar1 & 0xff) - 0x30;
  uVar1 = *param_4;
  if (uVar1 == 0x2e) {
    return 0;
  }
  if (((int)uVar1 < 0x41) || (0x50 < (int)uVar1)) {
    FUN_004e0620("\'A\' <= orientation && orientation <= \'P\'","\\Ns\\O\\Canondecoder.cpp",0x26e);
  }
  if ((*param_5 < 0) || (10 < *param_5)) {
    FUN_004e0620("0 <= variation && variation <= 10","\\Ns\\O\\Canondecoder.cpp",0x26f);
  }
  return 1;
}


// ==== FUN_0049a940 @ 0049a940

int __thiscall FUN_0049a940(int param_1,char param_2,char param_3,char param_4)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = *(char **)(param_1 + 0x124);
  iVar1 = 0;
  if (0 < *(int *)(param_1 + 0x114)) {
    do {
      if (((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)(param_1 + 0x114));
  }
  FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Rifttype.cpp",299);
  return -1;
}


// ==== FUN_00425b90 @ 00425b90

int * __thiscall FUN_00425b90(undefined4 *param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  undefined4 uVar6;
  int local_8;
  int local_4;
  
  uVar6 = 0;
  piVar5 = &local_4;
  piVar4 = &local_8;
  FUN_0049a840(*param_1,piVar4,piVar5,0);
  FUN_0049a900(piVar4,piVar5,uVar6);
  iVar2 = FUN_004e49c0();
  iVar3 = FUN_004e49c0();
  iVar1 = param_1[0xe];
  *param_2 = iVar2 - param_1[0xf] * local_8;
  param_2[1] = iVar3 - iVar1 * local_4;
  param_2[2] = iVar2;
  param_2[3] = iVar3;
  return param_2;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1e4) + *(int *)(param_1 + 0x1d8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1d8);
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


// ==== FUN_00425c00 @ 00425c00

int __fastcall FUN_00425c00(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = param_1[4];
  iVar2 = FUN_0049a840(*param_1);
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
}


