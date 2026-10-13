// ==== FUN_00490430 @ 00490430

void * __thiscall FUN_00490430(void *this)

{
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 4) = 0xffffffff;
  *(undefined4 *)((int)this + 8) = 0xffffffff;
  *(undefined4 *)((int)this + 0x14) = 0;
  *(undefined4 *)((int)this + 0xc) = 0xffffffff;
  *(undefined4 *)((int)this + 0x10) = 0xffffffff;
  return this;
}


// ==== FUN_004904d0 @ 004904d0

void __thiscall FUN_004904d0(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  undefined1 local_10 [16];
  
  *(undefined4 *)this = param_2;
  puVar2 = (undefined4 *)FUN_00403d20(local_10);
  *(undefined4 *)((int)this + 4) = *puVar2;
  *(undefined4 *)((int)this + 8) = puVar2[1];
  *(undefined4 *)((int)this + 0xc) = puVar2[2];
  uVar1 = puVar2[3];
  *(undefined4 *)((int)this + 0x14) = 1;
  *(undefined4 *)((int)this + 0x10) = uVar1;
  return;
}


// ==== FUN_00490520 @ 00490520

bool __thiscall FUN_00490520(void *this)

{
  return *(undefined **)this == &DAT_005484c0;
}


// ==== FUN_00490750 @ 00490750

undefined4 __thiscall FUN_00490750(void *this,byte *param_2,undefined4 param_3,int param_4)

{
  undefined2 uVar1;
  undefined1 uVar2;
  int iVar3;
  undefined1 *puVar4;
  int *piVar5;
  byte bVar6;
  undefined1 local_7c;
  float local_64;
  float local_60;
  undefined1 local_54 [20];
  int local_40;
  
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_00490040(s_cRect_isValid___00535bb4,s__ns_o_islandbuilder_cpp_00535b9c,0x5f);
  }
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_00535bdc,s__ns_o_islandbuilder_cpp_00535bc4,0x60);
  }
  if (param_4 == 0) {
    bVar6 = param_2[2] & 0xf;
  }
  else {
    bVar6 = 0;
  }
  local_64 = (float)(uint)bVar6;
  if (param_4 == 0) {
    bVar6 = param_2[2] >> 4;
  }
  else {
    bVar6 = 0;
  }
  iVar3 = FUN_00490520(this);
  if (iVar3 != 0) {
    param_3 = FUN_004bd500(&DAT_00583188,*(undefined4 *)((int)this + 0x14),param_3);
  }
  local_64 = (float)(*(int *)((int)this + 4) + (int)local_64);
  local_60 = (float)(int)(*(int *)((int)this + 8) + (uint)bVar6);
  iVar3 = 0;
  FUN_0041fcf0(local_54,DAT_0051caf0,*param_2 & 0x3f,*param_2 >> 6,local_64,local_60,0);
  while (local_40 != 0) {
    local_64 = (float)__ftol();
    local_60 = (float)__ftol();
    puVar4 = (undefined1 *)FUN_00404090(*(undefined4 *)this,local_64,local_60);
    uVar1 = *(undefined2 *)(param_2 + 4);
    *(int *)(puVar4 + 8) = iVar3;
    local_7c = (undefined1)param_3;
    *puVar4 = local_7c;
    iVar3 = iVar3 + 1;
    *(undefined4 *)(puVar4 + 2) = *(undefined4 *)param_2;
    *(undefined2 *)(puVar4 + 6) = uVar1;
    uVar2 = FUN_00420370(local_54);
    puVar4[1] = uVar2;
    FUN_0041feb0(local_54);
  }
  iVar3 = FUN_00490520(this);
  if (iVar3 != 0) {
    piVar5 = (int *)FUN_004040d0(*(undefined4 *)this,&local_64,param_3);
    FUN_004bd900(&DAT_00583188,param_3,(float)(*piVar5 << 4),(float)(piVar5[1] << 4),
                 (float)(piVar5[2] << 4),(float)(piVar5[3] << 4));
  }
  return param_3;
}


// ==== FUN_00490980 @ 00490980

void __thiscall FUN_00490980(void *this,int param_2,int param_3,int param_4)

{
  byte *pbVar1;
  
  if (param_4 == param_3) {
    FUN_00490040(s_iStart____iEnd_00535bfc,s__ns_o_islandbuilder_cpp_00535be4,0x92);
  }
  if (param_3 < param_4) {
    pbVar1 = (byte *)(param_2 + 1 + param_3 * 6);
    do {
      if (((*pbVar1 & 1) != 0) && ((*pbVar1 & 2) == 0)) {
        FUN_00490750(this,pbVar1 + -1,param_3,0);
      }
      pbVar1 = pbVar1 + 6;
      param_3 = param_3 + 1;
    } while (param_3 < param_4);
  }
  return;
}


// ==== FUN_00403d20 @ 00403d20

void __cdecl FUN_00403d20(undefined4 *param_1)

{
  *param_1 = 1;
  param_1[1] = 1;
  param_1[2] = 0xf;
  param_1[3] = 0xf;
  return;
}


// ==== FUN_00404090 @ 00404090

int __thiscall FUN_00404090(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *(int *)((int)this + 4) + -1;
  if (param_2 < 1) {
    param_2 = 0;
  }
  if (param_2 <= iVar2) {
    iVar2 = param_2;
  }
  iVar1 = *(int *)((int)this + 8) + -1;
  if (param_3 < 1) {
    param_3 = 0;
  }
  if (param_3 <= iVar1) {
    iVar1 = param_3;
  }
  return (int)this + (iVar2 * 0x10 + iVar1) * 0xc + 0xc;
}


// ==== FUN_004040d0 @ 004040d0

void __thiscall FUN_004040d0(void *this,int *param_2,int param_3)

{
  bool bVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_18;
  int local_c;
  
  iVar2 = FUN_004bd4c0(&DAT_00583188,param_3);
  if (iVar2 == 0) {
    FUN_00490040(s_islandList_exists_islandId__0050f540,s__Ns_o_ChunkMap_cpp_0050f52c,0xbe);
  }
  iVar5 = 0x10;
  iVar6 = 0;
  iVar2 = 0;
  local_c = 0x10;
  bVar1 = false;
  local_18 = 0;
  if (0 < *(int *)((int)this + 8)) {
    do {
      iVar4 = 0;
      if (0 < *(int *)((int)this + 4)) {
        do {
          pcVar3 = (char *)FUN_00404090(this,iVar4,local_18);
          if (*pcVar3 == param_3) {
            bVar1 = true;
            if (iVar4 <= iVar5) {
              iVar5 = iVar4;
            }
            if (local_18 <= local_c) {
              local_c = local_18;
            }
            if (iVar6 <= iVar4) {
              iVar6 = iVar4;
            }
            if (iVar2 <= local_18) {
              iVar2 = local_18;
            }
          }
          iVar4 = iVar4 + 1;
        } while (iVar4 < *(int *)((int)this + 4));
      }
      local_18 = local_18 + 1;
    } while (local_18 < *(int *)((int)this + 8));
  }
  if (!bVar1) {
    FUN_00490040(s_found_0050f570,s__Ns_o_ChunkMap_cpp_0050f55c,0xce);
  }
  *param_2 = iVar5;
  param_2[1] = local_c;
  param_2[2] = iVar6 + 1;
  param_2[3] = iVar2 + 1;
  return;
}


// ==== FUN_004bd480 @ 004bd480

undefined4 __thiscall FUN_004bd480(int *param_1,int param_2)

{
  if ((-1 < param_2) && (param_2 < *param_1)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004bd4c0 @ 004bd4c0

undefined4 __thiscall FUN_004bd4c0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd480(param_2);
  if (iVar1 != 0) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_004bd500 @ 004bd500

void __thiscall FUN_004bd500(void *this,undefined4 param_2,undefined1 param_3)

{
  int iVar1;
  
  iVar1 = FUN_004bd4f0();
  *(int *)this = *(int *)this + 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 8) = 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 0xc) = param_2;
  *(char *)((int)this + iVar1 * 0x20 + 0x11) = (char)param_2;
  *(undefined1 *)((int)this + iVar1 * 0x20 + 0x10) = param_3;
  return;
}


// ==== FUN_004bd900 @ 004bd900

void __thiscall
FUN_004bd900(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 != 0) {
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x14) = param_3;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x18) = param_4;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x1c) = param_5;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x20) = param_6;
  }
  return;
}


// ==== FUN_0041fcf0 @ 0041fcf0

void * __thiscall
FUN_0041fcf0(void *this,int param_2,int param_3,uint param_4,undefined4 param_5,undefined4 param_6,
            undefined4 param_7)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  int local_4;
  
  *(int *)this = param_2;
  *(undefined4 *)((int)this + 0x50) = param_7;
  *(int *)((int)this + 4) = param_3;
  *(undefined4 *)((int)this + 0xc) = 0;
  if ((param_4 & 1) != 0) {
    FUN_00490040(s___rot_1_____0_00516858,s__ns_o_CanonDecoder_cpp_00516840,0x2ae);
  }
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x1c) = 1;
  bVar5 = param_2 == DAT_0051caf0;
  *(int *)((int)this + 8) = (int)param_4 / 2;
  *(undefined4 *)((int)this + 0x2c) = param_5;
  *(undefined4 *)((int)this + 0x30) = param_6;
  if (bVar5) {
    *(undefined **)((int)this + 0x34) = &DAT_005151d8 + param_3 * 0x48;
  }
  else if (param_2 == DAT_0051ca8c) {
    *(undefined4 **)((int)this + 0x34) = &DAT_00514a80 + param_3 * 0x12;
  }
  else if (param_2 == DAT_0051cabc) {
    *(undefined **)((int)this + 0x34) = &DAT_005164f8 + param_3 * 0x48;
  }
  else if (param_2 == DAT_0051cbb8) {
    *(undefined **)((int)this + 0x34) = &DAT_00516588 + param_3 * 0x48;
  }
  else {
    iVar2 = FUN_0041fcc0(param_2);
    if (iVar2 == 0) {
      *(undefined4 *)((int)this + 0x34) = 0;
      local_4 = 1;
    }
    else {
      *(undefined **)((int)this + 0x34) = &DAT_005165d0 + param_3 * 0x48;
    }
  }
  iVar2 = *(int *)((int)this + 0x34);
  *(int *)((int)this + 0x28) = *(int *)this * 0x1d4 + DAT_0051c960;
  iVar3 = local_4;
  if (iVar2 != 0) {
    local_4 = *(int *)(iVar2 + 4);
    iVar3 = *(int *)(iVar2 + 8);
  }
  uVar1 = *(uint *)((int)this + 8);
  *(int *)((int)this + 0x38) = local_4;
  if ((uVar1 & 1) == 0) {
    *(int *)((int)this + 0x38) = iVar3;
  }
  *(int *)((int)this + 0x3c) = iVar3;
  if ((uVar1 & 1) == 0) {
    *(int *)((int)this + 0x3c) = local_4;
  }
  iVar4 = *(int *)(&DAT_00516660 + uVar1 * 4) * (local_4 + -1);
  iVar2 = *(int *)(&DAT_00516670 + uVar1 * 4);
  *(int *)((int)this + 0x40) = iVar4;
  *(int *)((int)this + 0x44) = iVar2 * (iVar3 + -1);
  if (*(int *)((int)this + 0xc) != 0) {
    *(int *)((int)this + 0x40) = (local_4 - iVar4) + -1;
  }
  *(undefined4 *)((int)this + 0x4c) = 0;
  *(undefined4 *)((int)this + 0x48) = 0;
  *(undefined4 *)((int)this + 0x14) = 1;
  FUN_0041feb0(this);
  return this;
}


// ==== FUN_0041feb0 @ 0041feb0

int __thiscall FUN_0041feb0(void *this)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  undefined4 local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if (*(int *)((int)this + 0x3c) <= *(int *)((int)this + 0x4c)) {
      iVar2 = *(int *)(&DAT_00516620 + *(int *)((int)this + 8) * 4);
      if (*(int *)((int)this + 0xc) != 0) {
        iVar2 = -iVar2;
      }
      iVar6 = *(int *)((int)this + 8) * 4;
      if (*(int *)(&DAT_00516620 + iVar6) != 0) {
        *(int *)((int)this + 0x40) =
             (*(int *)((int)this + 0x40) + iVar2 + *(int *)((int)this + 0x38)) %
             *(int *)((int)this + 0x38);
      }
      if (*(int *)(&DAT_00516630 + iVar6) != 0) {
        *(int *)((int)this + 0x44) =
             (*(int *)((int)this + 0x44) + *(int *)(&DAT_00516630 + iVar6) +
             *(int *)((int)this + 0x38)) % *(int *)((int)this + 0x38);
      }
      iVar2 = *(int *)((int)this + 0x48) + 1;
      *(int *)((int)this + 0x48) = iVar2;
      if (*(int *)((int)this + 0x38) <= iVar2) {
        *(undefined4 *)((int)this + 0x14) = 0;
        return iVar2;
      }
      *(undefined4 *)((int)this + 0x4c) = 0;
    }
    *(undefined4 *)((int)this + 0x10) = 0xffffffff;
    if (*(int *)((int)this + 0x34) == 0) {
      if (*(int *)((int)this + 0x50) == 0) {
        *(undefined4 *)((int)this + 0x10) = *(undefined4 *)(*(int *)((int)this + 0x28) + 0x118);
      }
      else {
        iVar2 = *(int *)((int)this + 4);
        *(int *)((int)this + 0x10) = iVar2;
        if ((iVar2 < 0) || (*(int *)(*(int *)this * 0x1d4 + 0x114 + DAT_0051c960) <= iVar2)) {
          FUN_00490040(s_0_<__frame_&&_frame_<_::getType(_00516880,s__ns_o_CanonDecoder_cpp_00516868
                       ,0x300);
        }
      }
    }
    else {
      iVar2 = FUN_0041fbb0(*(undefined4 *)((int)this + 0x40),*(undefined4 *)((int)this + 0x44),
                           &local_c,&local_10,(int)this + 0x18);
      if (iVar2 != 0) {
        if (*(int *)((int)this + 0xc) == 0) {
          uVar4 = *(undefined4 *)(&DAT_005166bc + *(int *)((int)this + 8) * 0x40);
          uVar7 = *(undefined4 *)(&DAT_0051657c + (*(int *)((int)this + 8) * 0x10 + local_c) * 4);
        }
        else {
          uVar3 = *(int *)((int)this + 8) + 2;
          uVar5 = (int)uVar3 >> 0x1f;
          iVar2 = ((uVar3 ^ uVar5) - uVar5 & 3 ^ uVar5) - uVar5;
          uVar4 = *(undefined4 *)(&DAT_005166bc + iVar2 * 0x40);
          uVar7 = *(undefined4 *)(&DAT_0051657c + (iVar2 * 0x10 + local_c) * 4);
        }
        uVar4 = FUN_004442f0(*(undefined4 *)((int)this + 0x28),uVar7,uVar4,local_10);
        *(undefined4 *)((int)this + 0x10) = uVar4;
      }
    }
    local_8 = (float)(*(int *)((int)this + 0x1c) * *(int *)((int)this + 0x4c)) +
              *(float *)((int)this + 0x2c);
    local_4 = (float)(*(int *)((int)this + 0x48) * *(int *)((int)this + 0x1c)) +
              *(float *)((int)this + 0x30);
    *(float *)((int)this + 0x20) = local_8;
    *(float *)((int)this + 0x24) = local_4;
    iVar2 = *(int *)(&DAT_00516640 + *(int *)((int)this + 8) * 4);
    if (*(int *)((int)this + 0xc) != 0) {
      iVar2 = -iVar2;
    }
    iVar6 = *(int *)((int)this + 8) * 4;
    if (*(int *)(&DAT_00516640 + iVar6) != 0) {
      *(int *)((int)this + 0x40) =
           (*(int *)((int)this + 0x40) + iVar2 + *(int *)((int)this + 0x3c)) %
           *(int *)((int)this + 0x3c);
    }
    iVar2 = 0;
    if (*(int *)(&DAT_00516650 + iVar6) != 0) {
      iVar1 = *(int *)((int)this + 0x3c);
      iVar6 = *(int *)((int)this + 0x44) + *(int *)(&DAT_00516650 + iVar6) + iVar1;
      iVar2 = iVar6 / iVar1;
      *(int *)((int)this + 0x44) = iVar6 % iVar1;
    }
    *(int *)((int)this + 0x4c) = *(int *)((int)this + 0x4c) + 1;
  } while (*(int *)((int)this + 0x10) == -1);
  return iVar2;
}


// ==== FUN_0041fcc0 @ 0041fcc0

undefined4 __cdecl FUN_0041fcc0(int param_1)

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

int __thiscall FUN_004442f0(void *this,char param_2,char param_3,char param_4)

{
  int iVar1;
  char *pcVar2;
  
  iVar1 = 0;
  pcVar2 = *(char **)((int)this + 0x124);
  if (0 < *(int *)((int)this + 0x114)) {
    do {
      if (((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)((int)this + 0x114));
  }
  FUN_00490040(&DAT_0051d7ec,s__ns_o_RiftType_cpp_0051d7d8,0x123);
  return -1;
}


// ==== FUN_004442b0 @ 004442b0

void __thiscall FUN_004442b0(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1b4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1c4) + *(int *)((int)this + 0x1b8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1b8);
  return;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_00420370 @ 00420370

int __thiscall FUN_00420370(void *this)

{
  return (int)*(char *)(*(int *)(*(int *)this * 0x1d4 + 0x124 + DAT_0051c960) +
                       *(int *)((int)this + 0x10) * 4);
}


