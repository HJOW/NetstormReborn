// ==== FUN_00447e40 @ 00447e40

void __thiscall FUN_00447e40(void *this,int param_2,int param_3)

{
  float fVar1;
  float fVar2;
  int iVar3;
  float *pfVar4;
  void *pvVar5;
  undefined4 uVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  undefined1 auStack_1c [8];
  undefined1 auStack_14 [8];
  void *pvStack_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f5b;
  pvStack_c = ExceptionList;
  iVar7 = *(int *)((int)this + 0x24);
  iVar9 = param_2 * 0x32 + DAT_005c8464;
  ExceptionList = &pvStack_c;
  iVar3 = (**(code **)(*(int *)this + 0x80))();
  iVar7 = iVar7 - *(int *)(&DAT_00532620 + iVar3 * 4);
  if (iVar7 < 0xb) {
    uVar8 = 0;
  }
  else if (iVar7 < 0x16) {
    uVar8 = 1;
  }
  else if (iVar7 < 0x21) {
    uVar8 = 2;
  }
  else if (iVar7 < 0x2c) {
    uVar8 = 3;
  }
  else {
    if (0x36 < iVar7) {
      ExceptionList = pvStack_c;
      return;
    }
    uVar8 = 4;
  }
  fVar1 = *(float *)(iVar9 + 0xe);
  fVar2 = *(float *)(iVar9 + 0x12);
  pfVar4 = (float *)FUN_00447b30(this,auStack_1c,uVar8);
  if ((fVar1 != *pfVar4) || (fVar2 != pfVar4[1])) {
    if (3 < uVar8) {
      ExceptionList = pvStack_c;
      return;
    }
    fVar1 = *(float *)(iVar9 + 0xe);
    fVar2 = *(float *)(iVar9 + 0x12);
    pfVar4 = (float *)FUN_00447b30(this,auStack_14,uVar8 + 1);
    if (fVar1 != *pfVar4) {
      ExceptionList = pvStack_c;
      return;
    }
    if (fVar2 != pfVar4[1]) {
      ExceptionList = pvStack_c;
      return;
    }
  }
  iVar7 = FUN_00447c00(this,*(undefined4 *)(iVar9 + 0xe),*(undefined4 *)(iVar9 + 0x12));
  if (iVar7 == 0) {
    if (param_3 == 0) {
      FUN_00491ca0(param_2,uVar8);
      FUN_004a9d70(*(undefined4 *)(iVar9 + 0xe),*(undefined4 *)(iVar9 + 0x12),
                   (&PTR_s_forWind2_wav_00532650)[uVar8],0);
      pvVar5 = operator_new(0x28);
      uStack_4 = 0;
      if (pvVar5 != (void *)0x0) {
        FUN_00496f00(pvVar5,0x307,((int)this - DAT_005c8464) / 0x32,(&DAT_00532664)[uVar8]);
        ExceptionList = pvStack_c;
        return;
      }
    }
    else {
      iVar3 = FUN_004479c0(this);
      iVar7 = DAT_005c8464;
      iVar3 = iVar3 * 0x32 + DAT_005c8464;
      iVar9 = FUN_0040ea40(iVar3);
      if (iVar9 != 0) {
        uVar6 = FUN_00447890(this,0xffffffff);
        FUN_00491cf0((iVar3 - iVar7) / 0x32,uVar6);
      }
      if ((DAT_00540bc4 != 0) && (iVar7 = FUN_00447dc0(this), iVar7 == 0)) {
        FUN_00447ca0(this);
      }
    }
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00447890 @ 00447890

char __thiscall FUN_00447890(void *this,int param_2)

{
  int iVar1;
  
  if (param_2 == -1) {
    param_2 = *(int *)((int)this + 0x24);
  }
  iVar1 = (**(code **)(*(int *)this + 0x80))();
  param_2 = param_2 - *(int *)(&DAT_00532620 + iVar1 * 4);
  if (param_2 < 0xb) {
    return '\0';
  }
  if (param_2 < 0x16) {
    return '\x01';
  }
  if (param_2 < 0x21) {
    return '\x02';
  }
  if (param_2 < 0x2c) {
    return '\x03';
  }
  return (0x36 < param_2) + '\x04';
}


// ==== FUN_004478f0 @ 004478f0

char __thiscall FUN_004478f0(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_004ad710();
  if (iVar2 != 0xd) {
    if (iVar2 == 0xc) {
      return '\x02';
    }
    if (iVar2 == 0xb) {
      return '\x03';
    }
    if (0x3f < iVar1) {
      return (0x77 < iVar1) + '\x02';
    }
  }
  return '\x01';
}


// ==== FUN_00447950 @ 00447950

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00447950(void *this,float *param_2)

{
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_004ade70(this,&local_10,0);
  *param_2 = local_10 - _DAT_005012d0;
  param_2[1] = local_c - _DAT_005012d0;
  param_2[2] = local_8 + _DAT_005012d0;
  param_2[3] = local_4 + _DAT_005012d0;
  return;
}


// ==== FUN_004479c0 @ 004479c0

int __thiscall FUN_004479c0(void *this)

{
  undefined4 *puVar1;
  int iVar2;
  undefined1 local_50 [16];
  undefined1 local_40 [52];
  int local_c;
  
  puVar1 = (undefined4 *)FUN_00447950(this,local_50);
  FUN_0041def0(local_40,*puVar1,puVar1[1],puVar1[2],puVar1[3],0);
  while( true ) {
    if (local_c == 0) {
      return 0;
    }
    if (((*(byte *)(DAT_005c8464 + 10 + local_c * 0x32) == DAT_005412d0) &&
        (*(char *)(DAT_005c8464 + local_c * 0x32 + 0x22) == *(char *)((int)this + 0x22))) &&
       (iVar2 = FUN_00491a20(), iVar2 == 0)) break;
    FUN_004b1810(local_40);
  }
  return local_c;
}


// ==== FUN_00447aa0 @ 00447aa0

void __thiscall FUN_00447aa0(void *this,undefined4 *param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 local_8;
  undefined4 local_4;
  
  if (4 < param_3) {
    FUN_004e0620("numBridgesMissing <= 4",".\\Dais.cpp",0x19c);
  }
  local_8 = *(undefined4 *)((int)this + 0xe);
  local_4 = *(undefined4 *)((int)this + 0x12);
  iVar1 = *(int *)(&DAT_00532630 + param_3 * 4) + DAT_00532624;
  uVar3 = 0;
  uVar2 = FUN_004ac550(this,0,iVar1);
  iVar1 = FUN_00419850(uVar2,uVar3,iVar1);
  FUN_0041d180(&local_8,(int)*(short *)(iVar1 + 0x20),(int)*(short *)(iVar1 + 0x22));
  *param_2 = local_8;
  param_2[1] = local_4;
  return;
}


// ==== FUN_00447b30 @ 00447b30

undefined4 * __thiscall FUN_00447b30(void *this,undefined4 *param_2,int param_3)

{
  byte bVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_3 < 5) {
    FUN_00447aa0(this,&local_8,param_3);
    uVar5 = 0;
    iVar3 = param_3;
    uVar2 = FUN_0049a840(DAT_005412f0,0,param_3);
    iVar3 = FUN_00419850(uVar2,uVar5,iVar3);
    FUN_0041d180(&local_8,(int)*(short *)(iVar3 + 0x20),(int)*(short *)(iVar3 + 0x22));
    if (param_3 == 4) {
      iVar3 = FUN_004e49c0();
      iVar4 = FUN_004e49c0();
      bVar1 = *(byte *)(iVar4 + iVar3 * 0x100 + DAT_005c7c44);
      if (((bVar1 & 6) != 0) && ((bVar1 & 8) == 0)) {
        FUN_0041d180(&local_8,DAT_00532648,DAT_0053264c);
      }
    }
    *param_2 = local_8;
    param_2[1] = local_4;
    return param_2;
  }
  return param_2;
}


// ==== FUN_00447c00 @ 00447c00

void __thiscall FUN_00447c00(void *this,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 uVar4;
  
  iVar3 = *(int *)((int)this + 0x24);
  iVar1 = (**(code **)(*(int *)this + 0x80))();
  iVar3 = iVar3 - *(int *)(&DAT_00532620 + iVar1 * 4);
  if (iVar3 < 0xb) {
    iVar3 = 0;
  }
  else if (iVar3 < 0x16) {
    iVar3 = 1;
  }
  else if (iVar3 < 0x21) {
    iVar3 = 2;
  }
  else if (iVar3 < 0x2c) {
    iVar3 = 3;
  }
  else {
    iVar3 = (0x36 < iVar3) + 4;
  }
  uVar4 = 0;
  uVar2 = FUN_0049a840(DAT_005412f0,0,iVar3);
  iVar3 = FUN_00419850(uVar2,uVar4,iVar3);
  FUN_0041d180(&param_2,-(int)*(short *)(iVar3 + 0x20),-(int)*(short *)(iVar3 + 0x22));
  FUN_004b1fa0(param_2,param_3,DAT_005412f0);
  return;
}


// ==== FUN_00447ca0 @ 00447ca0

void __thiscall FUN_00447ca0(void *this)

{
  char cVar1;
  int *piVar2;
  int iVar3;
  undefined4 unaff_EBX;
  int iVar4;
  undefined4 uStack_8;
  
  piVar2 = (int *)FUN_004af530(DAT_005412f0,0);
  FUN_004adca0(piVar2,0);
  iVar4 = *(int *)((int)this + 0x24);
  iVar3 = (**(code **)(*(int *)this + 0x80))();
  iVar4 = iVar4 - *(int *)(&DAT_00532620 + iVar3 * 4);
  if (iVar4 < 0xb) {
    iVar4 = 0;
  }
  else if (iVar4 < 0x16) {
    iVar4 = 1;
  }
  else if (iVar4 < 0x21) {
    iVar4 = 2;
  }
  else if (iVar4 < 0x2c) {
    iVar4 = 3;
  }
  else {
    iVar4 = (0x36 < iVar4) + 4;
  }
  piVar2[9] = iVar4;
  if ((*(byte *)((int)piVar2 + 0xb) & 4) == 0) {
    FUN_004e0620("isVoid()","c:\\ns\\o\\squid.h",0x282);
  }
  *(byte *)(piVar2 + 10) = *(byte *)(piVar2 + 10) | 1;
  (**(code **)(*piVar2 + 0x74))(*(undefined1 *)((int)this + 0x22));
  iVar4 = *(int *)((int)this + 0x24);
  iVar3 = (**(code **)(*(int *)this + 0x80))();
  iVar4 = iVar4 - *(int *)(&DAT_00532620 + iVar3 * 4);
  if (iVar4 < 0xb) {
    cVar1 = '\0';
  }
  else if (iVar4 < 0x16) {
    cVar1 = '\x01';
  }
  else if (iVar4 < 0x21) {
    cVar1 = '\x02';
  }
  else if (iVar4 < 0x2c) {
    cVar1 = '\x03';
  }
  else {
    cVar1 = (0x36 < iVar4) + '\x04';
  }
  FUN_00447aa0(this,&stack0xfffffff4,cVar1);
  (**(code **)(*piVar2 + 0x90))(unaff_EBX,uStack_8,0);
  return;
}


// ==== FUN_00447dc0 @ 00447dc0

undefined4 __thiscall FUN_00447dc0(void *this)

{
  undefined4 *puVar1;
  undefined1 local_50 [16];
  undefined1 local_40 [52];
  int local_c;
  
  puVar1 = (undefined4 *)FUN_004ade70(this,local_50,0);
  FUN_0041def0(local_40,*puVar1,puVar1[1],puVar1[2],puVar1[3],0);
  while( true ) {
    if (local_c == 0) {
      return 0;
    }
    if ((*(byte *)(local_c * 0x32 + 10 + DAT_005c8464) == DAT_005412f0) &&
       ((*(byte *)(local_c * 0x32 + DAT_005c8464 + 0x28) & 1) != 0)) break;
    FUN_004b1810(local_40);
  }
  return 1;
}


// ==== FUN_0044b150 @ 0044b150

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044b150(void *this,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  float10 fVar5;
  undefined4 uVar6;
  
  if (param_2 == 0x305) {
    iVar3 = *(int *)((int)this + 0x24);
    iVar1 = iVar3 + 1;
    iVar2 = FUN_00447890(this,iVar1);
    iVar3 = FUN_00447890(this,iVar3);
    if (iVar3 == iVar2) {
      FUN_004acee0(this,iVar1,0);
      return (float10)_DAT_00532688;
    }
    FUN_004acee0(this,iVar1,0);
    return (float10)_DAT_00500fc4;
  }
  if (param_2 == 0x306) {
    fVar5 = (float10)FUN_0044a200(this);
    return fVar5;
  }
  if ((param_2 == 0x307) && (param_3 != 0)) {
    uVar6 = 1;
    uVar4 = FUN_004479c0(this,1);
    FUN_00447e40(this,uVar4,uVar6);
    return (float10)_DAT_00500fc4;
  }
  fVar5 = (float10)FUN_0044b900(this,param_2,param_3,param_4);
  return fVar5;
}


// ==== FUN_00448930 @ 00448930

void __thiscall FUN_00448930(void *this,uint param_2)

{
  void *pvVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f5b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if ((param_2 & 1) != 0) {
    ExceptionList = &local_c;
    pvVar1 = operator_new(0x28);
    local_4 = 0;
    if (pvVar1 != (void *)0x0) {
      FUN_00496e80(pvVar1,0x304,((int)this - DAT_005c8464) / 0x32,0x3c23d70a);
    }
  }
  local_4 = 0xffffffff;
  if ((param_2 & 2) != 0) {
    iVar2 = FUN_00448a20(this);
    uVar3 = iVar2 * 0x32 + DAT_005c8464;
    if ((DAT_005c8464 + 0x32U <= uVar3) && (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      iVar4 = (int)this - DAT_005c8464;
      iVar2 = FUN_00448340();
      FUN_00443fe0(iVar4 / 0x32,(float)iVar2);
    }
  }
  FUN_004b0d30(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== FUN_00448a20 @ 00448a20

int __thiscall FUN_00448a20(void *this)

{
  int iVar1;
  undefined1 local_40 [52];
  int local_c;
  
  FUN_00422230(local_40,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1,1,0);
  while( true ) {
    if (local_c == 0) {
      return 0;
    }
    iVar1 = DAT_005c8464 + local_c * 0x32;
    if ((*(byte *)(iVar1 + 10) == DAT_005412ec) &&
       (*(char *)(iVar1 + 0x22) == *(char *)((int)this + 0x22))) break;
    FUN_004b1810(local_40);
  }
  return local_c;
}


// ==== FUN_00448340 @ 00448340

char __thiscall FUN_00448340(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  if ((((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
      (*(ushort *)(uVar2 + 0xc) != 0)) && (DAT_00594fa4 == 0)) {
    return (-((*(byte *)((uint)*(ushort *)(uVar2 + 0xc) * 0xb4 + DAT_0059534c + 0x50) & 1) != 0) &
           0x12U) + 7;
  }
  return '\n';
}


// ==== FUN_004ade70 @ 004ade70

undefined4 __thiscall FUN_004ade70(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  uVar1 = *(undefined4 *)((int)this + 0xe);
  uVar2 = *(undefined4 *)((int)this + 0x12);
  uVar3 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  FUN_0049ae80(uVar3,param_2,uVar1,uVar2,param_3);
  return param_2;
}


// ==== FUN_0049ae80 @ 0049ae80

float * __thiscall FUN_0049ae80(void *this,float *param_2,float param_3,float param_4,byte param_5)

{
  float fVar1;
  int iVar2;
  
  if ((param_5 & 1) == 0) {
    iVar2 = *(int *)((int)this + 0x1d8);
  }
  else {
    iVar2 = *(int *)((int)this + 0x1e4) + *(int *)((int)this + 0x1d8);
  }
  fVar1 = param_3 - (float)(*(int *)((int)this + 0x1d4) + -1);
  param_2[1] = param_4;
  param_2[3] = param_4;
  *param_2 = param_3;
  param_2[2] = param_3;
  param_4 = param_4 - (float)(iVar2 + -1);
  FUN_0040e880(param_2,fVar1,param_4);
  FUN_0040e8f0(param_2,fVar1,param_4);
  return param_2;
}


// ==== FUN_0040e880 @ 0040e880

void __thiscall FUN_0040e880(void *this,float param_2,float param_3)

{
  int iVar1;
  
  FUN_0041d0d0(&param_2);
  iVar1 = FUN_0040e800(this);
  if (iVar1 == 0) {
    *(float *)this = param_2;
    *(float *)((int)this + 4) = param_3;
    return;
  }
  if (*(float *)this < param_2) {
    param_2 = *(float *)this;
  }
  *(float *)this = param_2;
  if (param_3 <= *(float *)((int)this + 4)) {
    *(float *)((int)this + 4) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 4) = *(undefined4 *)((int)this + 4);
  return;
}


// ==== FUN_0040e8f0 @ 0040e8f0

void __thiscall FUN_0040e8f0(void *this,float param_2,float param_3)

{
  float *pfVar1;
  int iVar2;
  
  FUN_0041d0d0(&param_2);
  pfVar1 = (float *)((int)this + 8);
  iVar2 = FUN_0040e800(pfVar1);
  if (iVar2 == 0) {
    *pfVar1 = param_2;
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  if (param_2 < *pfVar1) {
    param_2 = *pfVar1;
  }
  *pfVar1 = param_2;
  if (*(float *)((int)this + 0xc) <= param_3) {
    *(float *)((int)this + 0xc) = param_3;
    return;
  }
  *(undefined4 *)((int)this + 0xc) = *(undefined4 *)((int)this + 0xc);
  return;
}


// ==== FUN_004ad710 @ 004ad710

int __thiscall FUN_004ad710(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4) + -0x41;
}


// ==== FUN_00491a20 @ 00491a20

bool __thiscall FUN_00491a20(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)((int)this + 0x1a);
  iVar2 = FUN_004adcb0();
  return iVar1 < iVar2 / 2;
}


// ==== FUN_004adcb0 @ 004adcb0

undefined4 __thiscall FUN_004adcb0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) == 0) {
    uVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
    uVar2 = FUN_0049fce0(uVar2);
    return uVar2;
  }
  if ((*(byte *)((int)this + 0x28) & 1) != 0) {
    uVar2 = FUN_0049a840(DAT_005412e8);
    uVar2 = FUN_0049fce0(uVar2);
    return uVar2;
  }
  return 0;
}


// ==== FUN_0049fce0 @ 0049fce0

int __thiscall FUN_0049fce0(void *this)

{
  if (((DAT_005e4794 != 0) && ((*(byte *)((int)this + 0xe8) & 0x10) == 0)) &&
     (((int)this - DAT_0059ab20) / 500 != DAT_005412ec)) {
    FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType",
                 "\\Ns\\O\\Rifttype.cpp",0x1e3);
  }
  if ((DAT_005ca8f0 != 0) && (((int)this - DAT_0059ab20) / 500 == DAT_005412d0)) {
    return (int)(*(int *)this + (*(int *)this >> 0x1f & 3U)) >> 2;
  }
  return *(int *)this;
}


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00419850 @ 00419850

int __thiscall FUN_00419850(void *this,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  
  if ((*(uint *)((int)this + 0xe8) & 0x440000) == 0) {
    if ((*(int *)((int)this + 0x114) <= param_3) || (param_3 < 0)) goto LAB_004198a8;
LAB_00419887:
    if (((*(int *)((int)this + 0xdc) != 0) && (*(int *)this != -0x22222223)) &&
       (*(int *)this != -0x32323233)) goto LAB_004198d0;
  }
  else {
    if ((param_3 < *(int *)((int)this + 0x114) * 2) && (-1 < param_3)) goto LAB_00419887;
LAB_004198a8:
    uVar1 = FUN_0049adc0(this,param_3,param_2);
    FUN_00436d40(uVar1);
  }
  FUN_004e0620("frameCheck( abstract, frame, false )","..\\o\\rifttype.h",0xe1);
LAB_004198d0:
  return *(int *)(*(int *)((int)this + 0xdc) + 8 + param_3 * 8) + -0x24 + *(int *)((int)this + 0xdc)
  ;
}


// ==== FUN_0041d180 @ 0041d180

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d180(void *this,int param_2,int param_3)

{
  *(float *)this = (float)param_2 / _DAT_0059a92c + *(float *)this;
  *(float *)((int)this + 4) = (float)param_3 / _DAT_0059a930 + *(float *)((int)this + 4);
  return;
}


// ==== FUN_0041d770 @ 0041d770

void __thiscall FUN_0041d770(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *(float *)this = (float)iVar1;
  iVar1 = FUN_004e49c0();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== FUN_0041d7a0 @ 0041d7a0

void __thiscall FUN_0041d7a0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *(float *)this = (float)iVar1;
  iVar1 = FUN_004e49c0();
  *(float *)((int)this + 4) = (float)iVar1;
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


// ==== FUN_004b1fa0 @ 004b1fa0

int __cdecl FUN_004b1fa0(undefined4 param_1,undefined4 param_2,uint param_3)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  iVar1 = FUN_0049a840(param_3);
  local_48 = param_1;
  local_44 = param_2;
  local_40[0] = &PTR_FUN_00502318;
  local_8 = (uint)((*(byte *)(iVar1 + 0xec) & 6) == 0);
  local_4 = 0;
  FUN_0041d770(&local_48);
  uVar2 = FUN_004e49c0();
  uVar3 = FUN_004e49c0();
  FUN_004b16d0(local_40,uVar3,uVar2,uVar3,uVar2);
  while( true ) {
    uVar4 = local_c * 0x32 + DAT_005c8464;
    if ((uVar4 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar4))
    break;
    if (*(byte *)(uVar4 + 10) == param_3) {
      return (local_c * 0x32) / 0x32;
    }
    FUN_004b1810(local_40);
  }
  return 0;
}


// ==== FUN_0041def0 @ 0041def0

void * __thiscall FUN_0041def0(void *this)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint in_stack_00000014;
  
  *(uint *)((int)this + 0x38) = in_stack_00000014 & 1;
  *(undefined ***)this = &PTR_FUN_00502318;
  *(uint *)((int)this + 0x3c) = in_stack_00000014 & 4;
  FUN_0041d770(&stack0x00000004);
  FUN_0041d7a0(&stack0x0000000c);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0(uVar1);
  uVar3 = FUN_004e49c0(uVar2);
  uVar4 = FUN_004e49c0(uVar3);
  FUN_004b16d0(this,uVar4,uVar3,uVar2,uVar1);
  return this;
}


// ==== FUN_004b16d0 @ 004b16d0

void __thiscall FUN_004b16d0(void *this,uint param_2,uint param_3,int param_4,int param_5)

{
  int *piVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  ushort *puVar5;
  
  if (param_4 == -1) {
    param_4 = DAT_00531928;
  }
  if (param_5 == -1) {
    param_5 = DAT_00531928;
  }
  param_2 = ((int)param_2 < 1) - 1 & param_2;
  *(uint *)((int)this + 4) = param_2;
  param_3 = ((int)param_3 < 1) - 1 & param_3;
  *(uint *)((int)this + 8) = param_3;
  iVar4 = DAT_00531928 + -1;
  if (param_4 < DAT_00531928 + -1) {
    iVar4 = param_4;
  }
  *(int *)((int)this + 0xc) = iVar4;
  iVar4 = DAT_00531928 + -1;
  if (param_5 < DAT_00531928 + -1) {
    iVar4 = param_5;
  }
  *(int *)((int)this + 0x10) = iVar4;
  if (*(int *)((int)this + 0x38) == 0) {
    *(undefined4 *)((int)this + 0x24) = 0;
  }
  else {
    *(undefined4 *)((int)this + 0x24) = 1;
  }
  FUN_00434380(param_2,param_3,(int)this + 0x14,(undefined4 *)((int)this + 0x18),
               *(undefined4 *)((int)this + 0x24));
  piVar1 = (int *)((int)this + 0x20);
  piVar2 = (int *)((int)this + 0x1c);
  FUN_00434380(*(undefined4 *)((int)this + 0xc),*(undefined4 *)((int)this + 0x10),piVar2,piVar1,
               *(undefined4 *)((int)this + 0x24));
  iVar4 = *piVar2;
  *piVar1 = *piVar1 + 1;
  *piVar2 = iVar4 + 1;
  if (iVar4 + 1 < 0) {
    *piVar2 = 0;
  }
  if (*piVar1 < 0) {
    *piVar1 = 0;
  }
  iVar4 = DAT_00531928 / (int)(&DAT_005424f8)[*(int *)((int)this + 0x24)] + -1;
  if (iVar4 < *piVar2) {
    *piVar2 = iVar4;
  }
  iVar4 = DAT_00531928 / (int)(&DAT_005424f8)[*(int *)((int)this + 0x24)] + -1;
  if (iVar4 < *piVar1) {
    *piVar1 = iVar4;
  }
  uVar3 = *(undefined4 *)((int)this + 0x18);
  *(undefined4 *)((int)this + 0x2c) = uVar3;
  *(undefined4 *)((int)this + 0x28) = *(undefined4 *)((int)this + 0x14);
  *(undefined4 *)((int)this + 0x34) = 0;
  puVar5 = (ushort *)
           FUN_004b2b00(&DAT_00542508,*(undefined4 *)((int)this + 0x14),uVar3,
                        *(undefined4 *)((int)this + 0x24));
  *(uint *)((int)this + 0x30) = (uint)*puVar5;
  (**(code **)(*(int *)this + 4))();
  return;
}


// ==== FUN_004b1810 @ 004b1810

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004b1810(void *this)

{
  byte bVar1;
  ushort uVar2;
  ushort *puVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  int *piVar10;
  undefined4 uVar11;
  int local_10;
  int local_c;
  float local_8;
  float local_4;
  
LAB_004b1820:
  do {
    if (*(int *)((int)this + 0x30) == 0) {
      piVar10 = (int *)((int)this + 0x1c);
      do {
        iVar7 = *(int *)((int)this + 0x28) + 1;
        *(int *)((int)this + 0x28) = iVar7;
        if (*piVar10 < iVar7) {
          iVar7 = *(int *)((int)this + 0x2c) + 1;
          *(undefined4 *)((int)this + 0x28) = *(undefined4 *)((int)this + 0x14);
          *(int *)((int)this + 0x2c) = iVar7;
          piVar9 = (int *)((int)this + 0x20);
          if (*(int *)((int)this + 0x20) < iVar7) {
            iVar7 = *(int *)((int)this + 0x24) + 1;
            *(int *)((int)this + 0x24) = iVar7;
            if (3 < iVar7) {
              *(undefined4 *)((int)this + 0x34) = 0;
              return;
            }
            FUN_00434380(*(undefined4 *)((int)this + 4),*(undefined4 *)((int)this + 8),
                         (int)this + 0x14,(undefined4 *)((int)this + 0x18),iVar7);
            FUN_00434380(*(undefined4 *)((int)this + 0xc),*(undefined4 *)((int)this + 0x10),piVar10,
                         piVar9,*(undefined4 *)((int)this + 0x24));
            iVar7 = *piVar10;
            *piVar9 = *piVar9 + 1;
            *piVar10 = iVar7 + 1;
            if (iVar7 + 1 < 0) {
              *piVar10 = 0;
            }
            if (*piVar9 < 0) {
              *piVar9 = 0;
            }
            iVar7 = DAT_00531928 / (int)(&DAT_005424f8)[*(int *)((int)this + 0x24)] + -1;
            if (iVar7 < *piVar10) {
              *piVar10 = iVar7;
            }
            iVar7 = DAT_00531928 / (int)(&DAT_005424f8)[*(int *)((int)this + 0x24)] + -1;
            if (iVar7 < *piVar9) {
              *piVar9 = iVar7;
            }
            *(undefined4 *)((int)this + 0x28) = *(undefined4 *)((int)this + 0x14);
            *(undefined4 *)((int)this + 0x2c) = *(undefined4 *)((int)this + 0x18);
            *(undefined4 *)((int)this + 0x34) = 0;
          }
        }
        puVar3 = (ushort *)
                 FUN_004b2b00(&DAT_00542508,*(undefined4 *)((int)this + 0x28),
                              *(undefined4 *)((int)this + 0x2c),*(undefined4 *)((int)this + 0x24));
        uVar2 = *puVar3;
        *(uint *)((int)this + 0x30) = (uint)uVar2;
      } while (uVar2 == 0);
    }
    do {
      iVar7 = *(int *)((int)this + 0x30);
      *(int *)((int)this + 0x34) = iVar7;
      if (iVar7 == 0) goto LAB_004b1820;
      bVar1 = *(byte *)(iVar7 * 0x32 + DAT_005c8464 + 0x28);
      *(uint *)((int)this + 0x30) = (uint)*(ushort *)(iVar7 * 0x32 + 4 + DAT_005c8464);
    } while ((bVar1 & 8) != 0);
    iVar7 = *(int *)((int)this + 0x34) * 0x32;
    local_4 = *(float *)(iVar7 + 0x12 + DAT_005c8464);
    iVar7 = iVar7 + DAT_005c8464;
    local_8 = *(float *)(iVar7 + 0xe);
    if ((((_DAT_00500fc4 < local_8) && (local_8 < (float)DAT_00531928)) && (_DAT_00500fc4 < local_4)
        ) && (local_4 < (float)DAT_00531928)) {
      uVar11 = *(undefined4 *)((int)this + 0x3c);
      piVar10 = &local_c;
      piVar9 = &local_10;
      uVar4 = FUN_0049a840(*(undefined1 *)(iVar7 + 10),piVar9,piVar10,uVar11);
      FUN_0049a900(uVar4,piVar9,piVar10,uVar11);
      iVar7 = FUN_004e49c0();
      iVar5 = FUN_004e49c0();
      iVar6 = (iVar7 - local_10) + 1;
      iVar8 = (iVar5 - local_c) + 1;
      if ((((((*(int *)((int)this + 4) <= iVar7) || (*(int *)((int)this + 4) <= iVar6)) &&
            ((iVar7 <= *(int *)((int)this + 0xc) || (iVar6 <= *(int *)((int)this + 0xc))))) &&
           ((*(int *)((int)this + 8) <= iVar5 || (*(int *)((int)this + 8) <= iVar8)))) &&
          ((iVar5 <= *(int *)((int)this + 0x10) || (iVar8 <= *(int *)((int)this + 0x10))))) &&
         (iVar7 = (*(code *)**(undefined4 **)this)(*(int *)((int)this + 0x34) * 0x32 + DAT_005c8464)
         , iVar7 != 0)) {
        return;
      }
    }
  } while( true );
}


// ==== FUN_00434380 @ 00434380

void FUN_00434380(int param_1,int param_2,int *param_3,int *param_4,int param_5)

{
  int iVar1;
  
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_1 / iVar1) {
    param_1 = DAT_00531928 / iVar1 + -1;
  }
  else {
    param_1 = param_1 / (int)(&DAT_005424f8)[param_5];
  }
  *param_3 = param_1;
  iVar1 = (&DAT_005424f8)[param_5];
  if (DAT_00531928 / iVar1 + -1 < param_2 / iVar1) {
    *param_4 = DAT_00531928 / iVar1 + -1;
    return;
  }
  *param_4 = param_2 / (int)(&DAT_005424f8)[param_5];
  return;
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


// ==== FUN_004b2b00 @ 004b2b00

int __thiscall FUN_004b2b00(void *this,int param_2,int param_3,int param_4)

{
  if ((param_4 < 0) || (3 < param_4)) {
    FUN_004e0620("level >=0 && level < HASH_LEVEL_COUNT","\\Ns\\O\\Squidhash.cpp",0x5c);
  }
  return *(int *)((int)this + param_4 * 4 + 0xc) +
         ((DAT_00531928 * param_3) / (int)(&DAT_005424f8)[param_4] + param_2) * 2;
}


// ==== FUN_004b1b20 @ 004b1b20

void __thiscall FUN_004b1b20(void *this)

{
  int iVar1;
  
  do {
    do {
      iVar1 = *(int *)((int)this + 4);
      *(int *)((int)this + 8) = iVar1;
      if (iVar1 == 0) {
        *(undefined4 *)((int)this + 4) = 0;
        *(undefined4 *)((int)this + 8) = 0;
        return;
      }
      *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 * 0x32 + 4 + DAT_005c8464);
      iVar1 = *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
    } while ((iVar1 != 0) && (*(byte *)(iVar1 + 10) != DAT_00541088));
    iVar1 = (*(code *)**(undefined4 **)this)(iVar1);
  } while (iVar1 == 0);
  return;
}


// ==== FUN_0040ea40 @ 0040ea40

undefined4 __thiscall FUN_0040ea40(void *this)

{
  if (((void *)(DAT_005c8464 + 0x32U) <= this) &&
     (this < (void *)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    return 1;
  }
  return 0;
}


// ==== FUN_004adca0 @ 004adca0

void __thiscall FUN_004adca0(void *this,undefined4 param_2)

{
  *(undefined4 *)((int)this + 0x1a) = param_2;
  return;
}


// ==== FUN_00491ca0 @ 00491ca0

void __cdecl FUN_00491ca0(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  uVar2 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    uVar3 = 0;
    iVar1 = FUN_004ac550(uVar2,0);
    FUN_004acee0(uVar2,*(undefined4 *)(iVar1 + 0x158 + param_2 * 4),uVar3);
  }
  return;
}


// ==== FUN_00491cf0 @ 00491cf0

void __cdecl FUN_00491cf0(int param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  void *pvVar4;
  undefined4 uVar5;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb1db;
  local_c = ExceptionList;
  uVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar1) && (uVar2 = DAT_005c8464 + DAT_005c847c * 0x32, uVar1 < uVar2)
     ) {
    ExceptionList = &local_c;
    if ((DAT_005c8464 + 0x32U <= uVar1) && (ExceptionList = &local_c, uVar1 < uVar2)) {
      uVar5 = 0;
      ExceptionList = &local_c;
      iVar3 = FUN_004ac550(uVar1,0);
      FUN_004acee0(uVar1,*(undefined4 *)(iVar3 + 0x158 + param_2 * 4),uVar5);
    }
    pvVar4 = operator_new(0x28);
    local_4 = 0;
    if (pvVar4 != (void *)0x0) {
      FUN_00496f00(pvVar4,0x25c,(int)(uVar1 - DAT_005c8464) / 0x32,0x3c23d70a);
    }
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_0041d0d0 @ 0041d0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0041d0d0(void *this)

{
  float fVar1;
  float fVar2;
  
  fVar1 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)this) {
    fVar1 = *(float *)this;
  }
  *(float *)this = fVar1;
  fVar2 = _DAT_005012d0;
  if (_DAT_005012d0 <= *(float *)((int)this + 4)) {
    fVar2 = *(float *)((int)this + 4);
  }
  *(float *)((int)this + 4) = fVar2;
  if (_DAT_005022e8 < fVar1) {
    fVar1 = _DAT_005022e8;
  }
  *(float *)this = fVar1;
  if (fVar2 <= _DAT_005022e8) {
    *(float *)((int)this + 4) = fVar2;
    return;
  }
  *(undefined4 *)((int)this + 4) = 0x437f0000;
  return;
}


// ==== FUN_0040e800 @ 0040e800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0040e800(void *this)

{
  if ((((_DAT_00500fc4 < *(float *)this) && (*(float *)this < (float)DAT_00531928)) &&
      (_DAT_00500fc4 < *(float *)((int)this + 4))) &&
     (*(float *)((int)this + 4) < (float)DAT_00531928)) {
    return 1;
  }
  return 0;
}


