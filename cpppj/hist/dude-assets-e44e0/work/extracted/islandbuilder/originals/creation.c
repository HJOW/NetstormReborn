// ==== FUN_0046d9b0 @ 0046d9b0

void __thiscall FUN_0046d9b0(void *this)

{
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 4) = 0xffffffff;
  *(undefined4 *)((int)this + 8) = 0xffffffff;
  *(undefined4 *)((int)this + 0xc) = 0xffffffff;
  *(undefined4 *)((int)this + 0x10) = 0xffffffff;
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 0x14) = 0;
  return;
}


// ==== FUN_0046da20 @ 0046da20

void __thiscall FUN_0046da20(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  undefined1 local_10 [16];
  
  *(undefined4 *)this = param_2;
  puVar2 = (undefined4 *)FUN_00431fc0(local_10);
  *(undefined4 *)((int)this + 4) = *puVar2;
  *(undefined4 *)((int)this + 8) = puVar2[1];
  *(undefined4 *)((int)this + 0xc) = puVar2[2];
  uVar1 = puVar2[3];
  *(undefined4 *)((int)this + 0x14) = 1;
  *(undefined4 *)((int)this + 0x10) = uVar1;
  return;
}


// ==== FUN_0046dba0 @ 0046dba0

undefined4 __thiscall FUN_0046dba0(void *this,byte *param_2,undefined4 param_3,int param_4)

{
  float fVar1;
  float fVar2;
  undefined1 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined1 *puVar6;
  undefined4 *puVar7;
  uint uVar8;
  int iVar9;
  uint uVar10;
  float *pfVar11;
  float local_74 [4];
  float local_64;
  float local_60;
  undefined1 local_54 [20];
  int local_40;
  float local_34;
  float local_30;
  
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_004e0620("cRect.isValid()","\\Ns\\O\\Islandbuilder.cpp",99);
  }
  if (*(int *)this == 0) {
    FUN_004e0620(&DAT_0050be98,"\\Ns\\O\\Islandbuilder.cpp",100);
  }
  if (param_4 == 0) {
    uVar10 = param_2[2] & 0xf;
    uVar8 = (uint)(param_2[2] >> 4);
  }
  else {
    uVar10 = 0;
    uVar8 = 0;
  }
  if (*(undefined **)this == &DAT_0054c8a0) {
    param_3 = FUN_0046ee60(&DAT_00568b60,*(undefined4 *)((int)this + 0x14),param_3);
  }
  local_64 = (float)(int)(*(int *)((int)this + 4) + uVar10);
  local_74[0] = (float)(*(int *)((int)this + 8) + uVar8);
  local_60 = (float)(int)local_74[0];
  FUN_00425c20(local_54,DAT_00541204,*param_2 & 0x3f,*param_2 >> 6,local_64,local_60,0);
  iVar9 = 0;
  fVar1 = local_74[0];
  fVar2 = local_60;
  local_74[0] = local_34;
  local_60 = local_30;
  while (local_40 != 0) {
    local_64 = local_74[0];
    local_34 = local_74[0];
    local_30 = local_60;
    uVar4 = FUN_004e49c0();
    uVar5 = FUN_004e49c0();
    puVar6 = (undefined1 *)FUN_00432180(*(undefined4 *)this,uVar4,uVar5);
    *puVar6 = (char)param_3;
    *(undefined4 *)(puVar6 + 2) = *(undefined4 *)param_2;
    *(undefined2 *)(puVar6 + 6) = *(undefined2 *)(param_2 + 4);
    *(int *)(puVar6 + 8) = iVar9;
    uVar3 = FUN_00425c00(local_54);
    puVar6[1] = uVar3;
    FUN_00425860(local_54);
    iVar9 = iVar9 + 1;
    fVar1 = local_74[0];
    fVar2 = local_60;
    local_74[0] = local_34;
    local_60 = local_30;
  }
  if (*(undefined **)this == &DAT_0054c8a0) {
    pfVar11 = &local_64;
    local_74[0] = fVar1;
    local_60 = fVar2;
    uVar4 = FUN_00432210(&DAT_0054c8a0,local_74,param_3);
    puVar7 = (undefined4 *)FUN_0040d900(uVar4,pfVar11);
    FUN_0046f190(&DAT_00568b60,param_3,*puVar7,puVar7[1],puVar7[2],puVar7[3]);
  }
  return param_3;
}


// ==== FUN_0046dd70 @ 0046dd70

void __thiscall FUN_0046dd70(void *this,int param_2,int param_3,int param_4)

{
  byte *pbVar1;
  
  if (param_3 == param_4) {
    FUN_004e0620("iStart != iEnd","\\Ns\\O\\Islandbuilder.cpp",0x96);
  }
  if (param_3 < param_4) {
    pbVar1 = (byte *)(param_2 + 1 + param_3 * 6);
    do {
      if (((*pbVar1 & 1) != 0) && ((*pbVar1 & 2) == 0)) {
        FUN_0046dba0(this,pbVar1 + -1,param_3,0);
      }
      param_3 = param_3 + 1;
      pbVar1 = pbVar1 + 6;
    } while (param_3 < param_4);
  }
  return;
}


// ==== FUN_00431fc0 @ 00431fc0

undefined4 * __cdecl FUN_00431fc0(undefined4 *param_1)

{
  *param_1 = 1;
  param_1[1] = 1;
  param_1[2] = 0xf;
  param_1[3] = 0xf;
  return param_1;
}


// ==== FUN_00432180 @ 00432180

void * __thiscall FUN_00432180(void *this,uint param_2,uint param_3)

{
  uint uVar1;
  
  if (*(int *)this == 0) {
    FUN_004e0620("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
  }
  if (((((int)param_2 < 0) || ((int)param_3 < 0)) || (*(int *)((int)this + 4) <= (int)param_2)) ||
     (*(int *)((int)this + 8) <= (int)param_3)) {
    FUN_004e0620("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
  }
  param_2 = ((int)param_2 < 1) - 1 & param_2;
  uVar1 = *(int *)((int)this + 4) - 1;
  if ((int)uVar1 <= (int)param_2) {
    param_2 = uVar1;
  }
  param_3 = ((int)param_3 < 1) - 1 & param_3;
  uVar1 = *(int *)((int)this + 8) - 1;
  if ((int)uVar1 <= (int)param_3) {
    param_3 = uVar1;
  }
  return (void *)((int)this + (param_2 * 0x10 + 1 + param_3) * 0xc);
}


// ==== FUN_00432210 @ 00432210

void __thiscall FUN_00432210(void *this,int *param_2,int param_3)

{
  int iVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  int local_10;
  int local_c;
  int local_4;
  
  iVar1 = FUN_0046ee30(&DAT_00568b60,param_3);
  if (iVar1 == 0) {
    FUN_004e0620("islandList.exists(islandId)","\\Ns\\O\\Chunkmap.cpp",0xc2);
  }
  iVar4 = 0;
  local_10 = 0x10;
  local_c = 0x10;
  iVar3 = 0;
  local_4 = 0;
  iVar1 = 0;
  if (0 < *(int *)((int)this + 8)) {
    do {
      iVar1 = 0;
      if (0 < *(int *)((int)this + 4)) {
        do {
          pcVar2 = (char *)FUN_00432180(this,iVar1,iVar4);
          if (*pcVar2 == param_3) {
            if (iVar1 <= local_10) {
              local_10 = iVar1;
            }
            if (iVar4 <= local_c) {
              local_c = iVar4;
            }
            if (iVar3 <= iVar1) {
              iVar3 = iVar1;
            }
            if (local_4 <= iVar4) {
              local_4 = iVar4;
            }
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 < *(int *)((int)this + 4));
      }
      iVar4 = iVar4 + 1;
      iVar1 = local_4;
    } while (iVar4 < *(int *)((int)this + 8));
  }
  *param_2 = local_10;
  param_2[1] = local_c;
  param_2[2] = iVar3 + 1;
  param_2[3] = iVar1 + 1;
  return;
}


// ==== FUN_0040d900 @ 0040d900

void __thiscall FUN_0040d900(void *this,float *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = *(int *)((int)this + 4);
  iVar2 = *(int *)((int)this + 8);
  iVar3 = *(int *)((int)this + 0xc);
  *param_2 = (float)(*(int *)this << 4);
  param_2[1] = (float)(iVar1 << 4);
  param_2[2] = (float)(iVar2 << 4);
  param_2[3] = (float)(iVar3 << 4);
  return;
}


// ==== FUN_0046ee30 @ 0046ee30

undefined4 __thiscall FUN_0046ee30(void *this,int param_2)

{
  if ((-1 < param_2) && (param_2 < *(int *)this)) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_0046ee60 @ 0046ee60

void __thiscall FUN_0046ee60(void *this,undefined4 param_2,undefined1 param_3)

{
  int iVar1;
  
  iVar1 = *(int *)this;
  *(int *)this = iVar1 + 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 0xc) = param_2;
  *(char *)((int)this + iVar1 * 0x20 + 0x11) = (char)param_2;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 8) = 1;
  *(undefined1 *)((int)this + iVar1 * 0x20 + 0x10) = param_3;
  return;
}


// ==== FUN_0046f190 @ 0046f190

void __thiscall
FUN_0046f190(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2c0);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (param_2 = param_2 * 0x20, *(int *)(param_2 + 8 + (int)this) != 0)) {
    *(undefined4 *)((int)this + param_2 + 0x14) = param_3;
    *(undefined4 *)((int)this + param_2 + 0x18) = param_4;
    *(undefined4 *)((int)this + param_2 + 0x1c) = param_5;
    *(undefined4 *)((int)this + param_2 + 0x20) = param_6;
  }
  return;
}


// ==== FUN_00425c20 @ 00425c20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall
FUN_00425c20(void *this,int param_2,int param_3,int param_4,undefined4 param_5,undefined4 param_6,
            undefined4 param_7)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  int extraout_ECX;
  int iVar4;
  
  iVar4 = param_2;
  *(undefined4 *)((int)this + 0x50) = param_7;
  *(int *)((int)this + 8) = param_4 / 2;
  *(int *)this = param_2;
  *(int *)((int)this + 4) = param_3;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x2c) = param_5;
  *(undefined4 *)((int)this + 0x30) = param_6;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x1c) = 1;
  if (param_2 == DAT_00541204) {
    *(undefined **)((int)this + 0x34) = &DAT_005300f0 + param_3 * 0x48;
  }
  else if (param_2 == _DAT_005411a0) {
    *(undefined4 **)((int)this + 0x34) = &DAT_0052f998 + param_3 * 0x12;
  }
  else if (param_2 == DAT_005411d0) {
    *(undefined **)((int)this + 0x34) = &DAT_00531410 + param_3 * 0x48;
  }
  else if (param_2 == DAT_005412cc) {
    *(undefined **)((int)this + 0x34) = &DAT_005314a0 + param_3 * 0x48;
  }
  else {
    iVar2 = FUN_00425830(param_2);
    if (iVar2 == 0) {
      *(undefined4 *)((int)this + 0x34) = 0;
      param_2 = 1;
    }
    else {
      *(undefined **)((int)this + 0x34) = &DAT_005314e8 + extraout_ECX * 0x48;
    }
  }
  uVar3 = FUN_0049a840(iVar4);
  *(undefined4 *)((int)this + 0x28) = uVar3;
  iVar4 = *(int *)((int)this + 0x34);
  iVar2 = param_2;
  if (iVar4 != 0) {
    param_2 = *(int *)(iVar4 + 8);
    iVar2 = *(int *)(iVar4 + 4);
  }
  uVar1 = *(uint *)((int)this + 8);
  iVar4 = iVar2;
  if ((uVar1 & 1) == 0) {
    iVar4 = param_2;
  }
  *(int *)((int)this + 0x38) = iVar4;
  iVar4 = param_2;
  if ((uVar1 & 1) == 0) {
    iVar4 = iVar2;
  }
  *(int *)((int)this + 0x3c) = iVar4;
  iVar4 = (iVar2 + -1) * *(int *)(&DAT_00531570 + uVar1 * 4);
  *(int *)((int)this + 0x40) = iVar4;
  *(int *)((int)this + 0x44) = (param_2 + -1) * *(int *)(&DAT_00531580 + uVar1 * 4);
  if (*(int *)((int)this + 0xc) != 0) {
    *(int *)((int)this + 0x40) = (iVar2 - iVar4) + -1;
  }
  *(undefined4 *)((int)this + 0x4c) = 0;
  *(undefined4 *)((int)this + 0x48) = 0;
  *(undefined4 *)((int)this + 0x14) = 1;
  FUN_00425860(this);
  return this;
}


// ==== FUN_00425860 @ 00425860

void __thiscall FUN_00425860(void *this)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  undefined4 local_14;
  int local_10;
  int local_c;
  float local_8;
  float local_4;
  
  do {
    if (*(int *)((int)this + 0x3c) <= *(int *)((int)this + 0x4c)) {
      iVar4 = *(int *)((int)this + 8) * 4;
      iVar5 = *(int *)(&DAT_00531530 + iVar4);
      iVar1 = iVar5;
      if (*(int *)((int)this + 0xc) != 0) {
        iVar1 = -iVar5;
      }
      if (iVar5 == 0) {
        iVar5 = *(int *)((int)this + 0x40);
      }
      else {
        iVar5 = (iVar1 + *(int *)((int)this + 0x38) + *(int *)((int)this + 0x40)) %
                *(int *)((int)this + 0x38);
      }
      *(int *)((int)this + 0x40) = iVar5;
      if (*(int *)(&DAT_00531540 + iVar4) == 0) {
        iVar5 = *(int *)((int)this + 0x44);
      }
      else {
        iVar5 = (*(int *)(&DAT_00531540 + iVar4) + *(int *)((int)this + 0x38) +
                *(int *)((int)this + 0x44)) % *(int *)((int)this + 0x38);
      }
      *(int *)((int)this + 0x44) = iVar5;
      iVar5 = *(int *)((int)this + 0x48) + 1;
      *(int *)((int)this + 0x48) = iVar5;
      if (*(int *)((int)this + 0x38) <= iVar5) {
        *(undefined4 *)((int)this + 0x14) = 0;
        return;
      }
      *(undefined4 *)((int)this + 0x4c) = 0;
    }
    *(undefined4 *)((int)this + 0x10) = 0xffffffff;
    if (*(int *)((int)this + 0x34) == 0) {
      if (*(int *)((int)this + 0x50) == 0) {
        *(undefined4 *)((int)this + 0x10) = *(undefined4 *)(*(int *)((int)this + 0x28) + 0x118);
      }
      else {
        *(int *)((int)this + 0x10) = *(int *)((int)this + 4);
        if (-1 < *(int *)((int)this + 4)) {
          iVar5 = FUN_0049a840(*(undefined4 *)this);
          if (*(int *)((int)this + 0x10) < *(int *)(iVar5 + 0x114)) goto LAB_004259a2;
        }
        FUN_004e0620("0 <= frame && frame < ::getType(type)->numFrames","\\Ns\\O\\Canondecoder.cpp",
                     0x305);
      }
    }
    else {
      iVar5 = FUN_00425700(*(undefined4 *)((int)this + 0x40),*(undefined4 *)((int)this + 0x44),
                           &local_10,&local_14,(int)this + 0x18);
      if (iVar5 != 0) {
        uVar2 = *(uint *)((int)this + 8);
        if ((*(int *)((int)this + 0xc) != 0) && (uVar2 = uVar2 + 2 & 0x80000003, (int)uVar2 < 0)) {
          uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
        }
        uVar3 = FUN_0049a940(*(undefined4 *)((int)this + 0x28),
                             (&DAT_0053148c)[(uVar2 * 0x10 + local_10) * 4],
                             (&DAT_005315cc)[uVar2 * 0x40],local_14);
        *(undefined4 *)((int)this + 0x10) = uVar3;
      }
    }
LAB_004259a2:
    if (((DAT_005e4794 != 0) && (*(int *)((int)this + 0x10) != -1)) &&
       ((*(byte *)(*(int *)(*(int *)((int)this + 0x28) + 0x124) + 3 + *(int *)((int)this + 0x10) * 4
                  ) & 0x20) != 0)) {
      FUN_004e0620("frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)",
                   "\\Ns\\O\\Canondecoder.cpp",0x325);
    }
    local_c = *(int *)((int)this + 0x48) * *(int *)((int)this + 0x1c);
    local_8 = (float)(*(int *)((int)this + 0x1c) * *(int *)((int)this + 0x4c)) +
              *(float *)((int)this + 0x2c);
    *(float *)((int)this + 0x20) = local_8;
    local_4 = (float)local_c + *(float *)((int)this + 0x30);
    *(float *)((int)this + 0x24) = local_4;
    iVar4 = *(int *)((int)this + 8) * 4;
    iVar5 = *(int *)(&DAT_00531550 + iVar4);
    iVar1 = iVar5;
    if (*(int *)((int)this + 0xc) != 0) {
      iVar1 = -iVar5;
    }
    if (iVar5 == 0) {
      iVar5 = *(int *)((int)this + 0x40);
    }
    else {
      iVar5 = (iVar1 + *(int *)((int)this + 0x3c) + *(int *)((int)this + 0x40)) %
              *(int *)((int)this + 0x3c);
    }
    *(int *)((int)this + 0x40) = iVar5;
    if (*(int *)(&DAT_00531560 + iVar4) == 0) {
      iVar5 = *(int *)((int)this + 0x44);
    }
    else {
      iVar5 = (*(int *)(&DAT_00531560 + iVar4) + *(int *)((int)this + 0x3c) +
              *(int *)((int)this + 0x44)) % *(int *)((int)this + 0x3c);
    }
    *(int *)((int)this + 0x44) = iVar5;
    *(int *)((int)this + 0x4c) = *(int *)((int)this + 0x4c) + 1;
    if (*(int *)((int)this + 0x10) != -1) {
      return;
    }
  } while( true );
}


// ==== FUN_00425830 @ 00425830

undefined4 __cdecl FUN_00425830(int param_1)

{
  if ((((param_1 != DAT_00541264) && (param_1 != DAT_0054125c)) && (param_1 != DAT_00541288)) &&
     (param_1 != DAT_00541290)) {
    return 0;
  }
  return 1;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

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

int __thiscall FUN_0049a940(void *this,char param_2,char param_3,char param_4)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = *(char **)((int)this + 0x124);
  iVar1 = 0;
  if (0 < *(int *)((int)this + 0x114)) {
    do {
      if (((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)((int)this + 0x114));
  }
  FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Rifttype.cpp",299);
  return -1;
}


// ==== FUN_0049a900 @ 0049a900

void __thiscall FUN_0049a900(void *this,undefined4 *param_2,int *param_3,int param_4)

{
  *param_2 = *(undefined4 *)((int)this + 0x1d4);
  if (param_4 != 0) {
    *param_3 = *(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8);
    return;
  }
  *param_3 = *(int *)((int)this + 0x1d8);
  return;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong __cdecl FUN_004e49c0(void)

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

int __thiscall FUN_00425c00(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x10);
  iVar2 = FUN_0049a840(*(undefined4 *)this);
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
}


