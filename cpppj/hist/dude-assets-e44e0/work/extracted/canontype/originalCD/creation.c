// ==== FUN_0041fcf0 @ 0041fcf0

int * __thiscall
FUN_0041fcf0(int *param_1,int param_2,int param_3,uint param_4,int param_5,int param_6,int param_7)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  int local_4;
  
  *param_1 = param_2;
  param_1[0x14] = param_7;
  param_1[1] = param_3;
  param_1[3] = 0;
  if ((param_4 & 1) != 0) {
    FUN_00490040(s___rot_1_____0_00516858,s__ns_o_CanonDecoder_cpp_00516840,0x2ae);
  }
  param_1[0xd] = 0;
  param_1[7] = 1;
  bVar5 = param_2 == DAT_0051caf0;
  param_1[2] = (int)param_4 / 2;
  param_1[0xb] = param_5;
  param_1[0xc] = param_6;
  if (bVar5) {
    param_1[0xd] = (int)(&DAT_005151d8 + param_3 * 0x48);
  }
  else if (param_2 == DAT_0051ca8c) {
    param_1[0xd] = (int)(&DAT_00514a80 + param_3 * 0x12);
  }
  else if (param_2 == DAT_0051cabc) {
    param_1[0xd] = (int)(&DAT_005164f8 + param_3 * 0x48);
  }
  else if (param_2 == DAT_0051cbb8) {
    param_1[0xd] = (int)(&DAT_00516588 + param_3 * 0x48);
  }
  else {
    iVar2 = FUN_0041fcc0(param_2);
    if (iVar2 == 0) {
      param_1[0xd] = 0;
      local_4 = 1;
    }
    else {
      param_1[0xd] = (int)(&DAT_005165d0 + param_3 * 0x48);
    }
  }
  iVar2 = param_1[0xd];
  param_1[10] = *param_1 * 0x1d4 + DAT_0051c960;
  iVar3 = local_4;
  if (iVar2 != 0) {
    local_4 = *(int *)(iVar2 + 4);
    iVar3 = *(int *)(iVar2 + 8);
  }
  uVar1 = param_1[2];
  param_1[0xe] = local_4;
  if ((uVar1 & 1) == 0) {
    param_1[0xe] = iVar3;
  }
  param_1[0xf] = iVar3;
  if ((uVar1 & 1) == 0) {
    param_1[0xf] = local_4;
  }
  iVar4 = *(int *)(&DAT_00516660 + uVar1 * 4) * (local_4 + -1);
  iVar2 = *(int *)(&DAT_00516670 + uVar1 * 4);
  param_1[0x10] = iVar4;
  param_1[0x11] = iVar2 * (iVar3 + -1);
  if (param_1[3] != 0) {
    param_1[0x10] = (local_4 - iVar4) + -1;
  }
  param_1[0x13] = 0;
  param_1[0x12] = 0;
  param_1[5] = 1;
  FUN_0041feb0();
  return param_1;
}


// ==== FUN_0041feb0 @ 0041feb0

int __fastcall FUN_0041feb0(int *param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if (param_1[0xf] <= param_1[0x13]) {
      iVar2 = *(int *)(&DAT_00516620 + param_1[2] * 4);
      if (param_1[3] != 0) {
        iVar2 = -iVar2;
      }
      iVar4 = param_1[2] * 4;
      if (*(int *)(&DAT_00516620 + iVar4) != 0) {
        param_1[0x10] = (param_1[0x10] + iVar2 + param_1[0xe]) % param_1[0xe];
      }
      if (*(int *)(&DAT_00516630 + iVar4) != 0) {
        param_1[0x11] =
             (param_1[0x11] + *(int *)(&DAT_00516630 + iVar4) + param_1[0xe]) % param_1[0xe];
      }
      iVar2 = param_1[0x12] + 1;
      param_1[0x12] = iVar2;
      if (param_1[0xe] <= iVar2) {
        param_1[5] = 0;
        return iVar2;
      }
      param_1[0x13] = 0;
    }
    param_1[4] = -1;
    if (param_1[0xd] == 0) {
      if (param_1[0x14] == 0) {
        param_1[4] = *(int *)(param_1[10] + 0x118);
      }
      else {
        iVar2 = param_1[1];
        param_1[4] = iVar2;
        if ((iVar2 < 0) || (*(int *)(*param_1 * 0x1d4 + 0x114 + DAT_0051c960) <= iVar2)) {
          FUN_00490040(s_0_<__frame_&&_frame_<_::getType(_00516880,s__ns_o_CanonDecoder_cpp_00516868
                       ,0x300);
        }
      }
    }
    else {
      iVar2 = FUN_0041fbb0(param_1[0x10],param_1[0x11],&local_c,&local_10,param_1 + 6);
      if (iVar2 != 0) {
        if (param_1[3] == 0) {
          uVar6 = *(undefined4 *)(&DAT_005166bc + param_1[2] * 0x40);
          uVar5 = *(undefined4 *)(&DAT_0051657c + (param_1[2] * 0x10 + local_c) * 4);
        }
        else {
          uVar3 = (int)(param_1[2] + 2U) >> 0x1f;
          iVar2 = ((param_1[2] + 2U ^ uVar3) - uVar3 & 3 ^ uVar3) - uVar3;
          uVar6 = *(undefined4 *)(&DAT_005166bc + iVar2 * 0x40);
          uVar5 = *(undefined4 *)(&DAT_0051657c + (iVar2 * 0x10 + local_c) * 4);
        }
        iVar2 = FUN_004442f0(uVar5,uVar6,local_10);
        param_1[4] = iVar2;
      }
    }
    local_8 = (float)(param_1[7] * param_1[0x13]) + (float)param_1[0xb];
    local_4 = (float)(param_1[0x12] * param_1[7]) + (float)param_1[0xc];
    param_1[8] = (int)local_8;
    param_1[9] = (int)local_4;
    iVar2 = *(int *)(&DAT_00516640 + param_1[2] * 4);
    if (param_1[3] != 0) {
      iVar2 = -iVar2;
    }
    iVar4 = param_1[2] * 4;
    if (*(int *)(&DAT_00516640 + iVar4) != 0) {
      param_1[0x10] = (param_1[0x10] + iVar2 + param_1[0xf]) % param_1[0xf];
    }
    iVar2 = 0;
    if (*(int *)(&DAT_00516650 + iVar4) != 0) {
      iVar1 = param_1[0xf];
      iVar4 = param_1[0x11] + *(int *)(&DAT_00516650 + iVar4) + iVar1;
      iVar2 = iVar4 / iVar1;
      param_1[0x11] = iVar4 % iVar1;
    }
    param_1[0x13] = param_1[0x13] + 1;
  } while (param_1[4] == -1);
  return iVar2;
}


// ==== FUN_0041fcc0 @ 0041fcc0

undefined4 FUN_0041fcc0(int param_1)

{
  if ((((DAT_0051cb50 != param_1) && (DAT_0051cb48 != param_1)) && (DAT_0051cb74 != param_1)) &&
     (DAT_0051cb7c != param_1)) {
    return 0;
  }
  return 1;
}


// ==== FUN_0041fbb0 @ 0041fbb0

undefined4 __thiscall
FUN_0041fbb0(int param_1,int param_2,int param_3,uint *param_4,uint *param_5,uint *param_6)

{
  uint uVar1;
  uint uVar2;
  
  uVar1 = *(uint *)(param_1 + 0xc + (*(int *)(param_1 + 4) * param_3 + param_2) * 4);
  uVar2 = uVar1 >> 0x18;
  *param_6 = uVar2;
  if ((uVar2 != 0) && ((uVar2 < 0x61 || (0x7a < uVar2)))) {
    FUN_00490040(s_num__0______a__<__num_&&_num_<____00516798,s__ns_o_CanonDecoder_cpp_00516780,
                 0x25e);
  }
  uVar2 = uVar1 >> 8 & 0xff;
  *param_6 = *param_6 - 0x61;
  *param_4 = uVar2;
  *param_5 = uVar1 & 0xff;
  *param_5 = (uVar1 & 0xff) - 0x30;
  if (uVar2 == 0x2e) {
    return 0;
  }
  if ((uVar2 < 0x41) || (0x50 < uVar2)) {
    FUN_00490040(s__A__<__orientation_&&_orientatio_005167d8,s__ns_o_CanonDecoder_cpp_005167c0,0x269
                );
  }
  if (((int)*param_5 < 0) || (10 < (int)*param_5)) {
    FUN_00490040(s_0_<__variation_&&_variation_<__1_0051681c,s__ns_o_CanonDecoder_cpp_00516804,0x26a
                );
  }
  return 1;
}


// ==== FUN_004442f0 @ 004442f0

int __thiscall FUN_004442f0(int param_1,char param_2,char param_3,char param_4)

{
  int iVar1;
  char *pcVar2;
  
  iVar1 = 0;
  pcVar2 = *(char **)(param_1 + 0x124);
  if (0 < *(int *)(param_1 + 0x114)) {
    do {
      if (((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)(param_1 + 0x114));
  }
  FUN_00490040(&DAT_0051d7ec,s__ns_o_RiftType_cpp_0051d7d8,0x123);
  return -1;
}


// ==== FUN_004202e0 @ 004202e0

void __thiscall FUN_004202e0(int param_1,int *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int local_18;
  int local_14 [3];
  int local_8;
  
  FUN_004442b0(&local_18,local_14,0);
  iVar2 = __ftol();
  local_8 = iVar2;
  iVar3 = __ftol();
  iVar1 = *(int *)(param_1 + 0x38);
  *param_2 = iVar2 - *(int *)(param_1 + 0x3c) * local_18;
  param_2[1] = iVar3 - iVar1 * local_14[0];
  param_2[2] = local_8;
  param_2[3] = iVar3;
  return;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(int param_1,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)(param_1 + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)(param_1 + 0x1c4) + *(int *)(param_1 + 0x1b8);
    return;
  }
  *param_3 = *(int *)(param_1 + 0x1b8);
  return;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_00420370 @ 00420370

int __fastcall FUN_00420370(int *param_1)

{
  return (int)*(char *)(*(int *)(*param_1 * 0x1d4 + 0x124 + DAT_0051c960) + param_1[4] * 4);
}


