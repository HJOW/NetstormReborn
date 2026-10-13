// ==== FUN_0044a3a0 @ 0044a3a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044a3a0(void *this,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  float10 fVar3;
  
  if (param_2 == 0x304) {
    iVar1 = FUN_00448a20();
    uVar2 = iVar1 * 0x32 + DAT_005c8464;
    if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      FUN_00449f40(uVar2,((int)this - DAT_005c8464) / 0x32);
    }
    (**(code **)(*(int *)this + 0x10))(0x48);
    return (float10)_DAT_00502350;
  }
  fVar3 = (float10)FUN_00496e70(param_2,param_3,param_4);
  return fVar3;
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


// ==== FUN_00449f40 @ 00449f40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00449f40(void *this)

{
  float fVar1;
  undefined *puVar2;
  int iVar3;
  uint uVar4;
  void *pvVar5;
  undefined4 uVar6;
  double *pdVar7;
  byte bVar8;
  int iVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  double dStack_1c;
  undefined1 auStack_14 [8];
  void *pvStack_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004f9fc6;
  pvStack_c = ExceptionList;
  iVar9 = *(int *)((int)this + 0x24);
  ExceptionList = &pvStack_c;
  iVar3 = (**(code **)(*(int *)this + 0x80))();
  iVar9 = iVar9 - *(int *)(&DAT_00532620 + iVar3 * 4);
  if (iVar9 < 0xb) {
    bVar8 = 0;
  }
  else if (iVar9 < 0x16) {
    bVar8 = 1;
  }
  else if (iVar9 < 0x21) {
    bVar8 = 2;
  }
  else if (iVar9 < 0x2c) {
    bVar8 = 3;
  }
  else {
    bVar8 = (0x36 < iVar9) + 4;
  }
  iVar9 = FUN_004479c0(this);
  uVar4 = iVar9 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar4) && (uVar4 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    FUN_00491b60(uVar4);
  }
  if (bVar8 < 4) {
    if (DAT_00540bc4 != 0) {
      FUN_00449900(this,0,0);
    }
    FUN_00448080(this);
    ExceptionList = pvStack_c;
    return;
  }
  FUN_004a9d70(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),"itIsDone2.wav",0)
  ;
  iVar9 = FUN_00448170(((int)this - DAT_005c8464) / 0x32,4,*(undefined1 *)((int)this + 10));
  fVar1 = *(float *)(&DAT_00532680 + (uint)(iVar9 != 0) * 4);
  puVar2 = (&PTR_s_priestSacrifice2_wav_00532678)[iVar9 != 0];
  pvVar5 = operator_new(0x28);
  dStack_1c = (double)CONCAT44(dStack_1c._4_4_,pvVar5);
  iVar3 = 0;
  uStack_4 = 0;
  if (pvVar5 != (void *)0x0) {
    iVar3 = (int)this - DAT_005c8464;
    uVar11 = 0xc;
    uVar10 = 0;
    uVar6 = thunk_FUN_004a8d50(puVar2,0,0xc);
    iVar3 = FUN_004ab240(pvVar5,iVar3 / 0x32,uVar6,uVar10,uVar11);
  }
  uStack_4 = 0xffffffff;
  if (iVar3 != 0) {
    pdVar7 = (double *)FUN_00460d70(&dStack_1c);
    dStack_1c = (double)fVar1 + *pdVar7;
    *(double *)(iVar3 + 0x20) = dStack_1c;
  }
  pvVar5 = operator_new(0x28);
  dStack_1c = (double)CONCAT44(dStack_1c._4_4_,pvVar5);
  uStack_4 = 1;
  if (pvVar5 == (void *)0x0) {
    iVar3 = 0;
  }
  else {
    iVar3 = (int)this - DAT_005c8464;
    uVar11 = 0xc;
    uVar10 = 0;
    uVar6 = thunk_FUN_004a8d50("thunderCrack.wav",0,0xc);
    iVar3 = FUN_004ab240(pvVar5,iVar3 / 0x32,uVar6,uVar10,uVar11);
  }
  uStack_4 = 0xffffffff;
  if (iVar3 != 0) {
    pdVar7 = (double *)FUN_00460d70(auStack_14);
    dStack_1c = *pdVar7 + _DAT_00507918;
    *(double *)(iVar3 + 0x20) = dStack_1c;
  }
  if (iVar9 != 0) {
    FUN_004483c0(this);
    ExceptionList = pvStack_c;
    return;
  }
  iVar9 = FUN_00448220();
  if ((iVar9 != 0) || (DAT_00594fa4 != 0)) {
    iVar9 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         DAT_005412e8);
    uVar4 = iVar9 * 0x32 + DAT_005c8464;
    if ((DAT_005c8464 + 0x32U <= uVar4) && (uVar4 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      FUN_00448c80(uVar4);
    }
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_0044a200 @ 0044a200

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044a200(void *this)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(DAT_005412fc);
  iVar3 = (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
  iVar4 = *(int *)(iVar2 + 0x2c + iVar3 * 4);
  iVar2 = *(int *)(iVar2 + 0x170 + (iVar3 + -0x41) * 4);
  if ((iVar4 <= iVar1) && (iVar1 < iVar2 + iVar4)) {
    iVar4 = ((iVar1 - iVar4) + 1) % iVar2 + iVar4;
  }
  if (iVar1 <= iVar4) {
    FUN_004acee0(this,iVar4,0);
    return (float10)_DAT_00507930;
  }
  *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) & 0xfd;
  FUN_00449b30();
  return (float10)_DAT_00500fc4;
}


// ==== FUN_00448080 @ 00448080

void __thiscall FUN_00448080(void *this)

{
  char cVar1;
  int iVar2;
  int iVar3;
  void *local_14;
  undefined4 uStack_10;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f3b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  local_14 = operator_new(0x28);
  local_4 = 0;
  if (local_14 != (void *)0x0) {
    FUN_00496f00(local_14,0x305,((int)this - DAT_005c8464) / 0x32,0x3c23d70a);
  }
  iVar3 = *(int *)((int)this + 0x24);
  local_4 = 0xffffffff;
  iVar2 = (**(code **)(*(int *)this + 0x80))();
  iVar3 = iVar3 - *(int *)(&DAT_00532620 + iVar2 * 4);
  if (iVar3 < 0xb) {
    cVar1 = '\0';
  }
  else if (iVar3 < 0x16) {
    cVar1 = '\x01';
  }
  else if (iVar3 < 0x21) {
    cVar1 = '\x02';
  }
  else if (iVar3 < 0x2c) {
    cVar1 = '\x03';
  }
  else {
    cVar1 = (0x36 < iVar3) + '\x04';
  }
  FUN_00447b30(this,&local_14,cVar1);
  FUN_004a9d70(local_14,uStack_10,"altarBurnCollapse.wav",0);
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_004483c0 @ 004483c0

void __thiscall FUN_004483c0(void *this)

{
  int iVar1;
  uint uVar2;
  void *pvVar3;
  undefined4 uVar4;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f7b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  *(byte *)((int)this + 0x28) = *(byte *)((int)this + 0x28) | 2;
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    iVar1 = FUN_004ac550(uVar2);
    FUN_004acee0(uVar2,*(undefined4 *)(iVar1 + 0x160),0);
  }
  iVar1 = (**(code **)(*(int *)this + 0x80))();
  if (iVar1 == 1) {
    uVar4 = 0;
  }
  else {
    if (iVar1 != 2) goto LAB_00448458;
    uVar4 = 0x2d;
  }
  FUN_004acee0(this,uVar4,0);
LAB_00448458:
  pvVar3 = operator_new(0x28);
  uStack_4 = 0;
  if (pvVar3 != (void *)0x0) {
    FUN_00496f00(pvVar3,0x306,((int)this - DAT_005c8464) / 0x32,0x3c23d70a);
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00491b60 @ 00491b60

void __thiscall FUN_00491b60(void *this)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_004ad710();
  if ((9 < iVar1) && (iVar1 < 0xf)) {
    iVar1 = FUN_004afb40(this,0x25c);
    if (iVar1 != 0) {
      FUN_0041bf10(iVar1,0);
    }
    uVar2 = 0;
    iVar1 = FUN_004ac550(this,0);
    FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x140),uVar2);
  }
  return;
}


// ==== FUN_00449900 @ 00449900

void __thiscall FUN_00449900(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  uint uVar5;
  int iStack_10;
  int iStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  if (((param_2 != 0) || (param_2 = FUN_004479c0(), param_2 != 0)) &&
     (iVar1 = FUN_00449710(this,param_2), iVar1 != 0)) {
    iVar1 = *(int *)((int)this + 0x24);
    iVar2 = (**(code **)(*(int *)this + 0x80))();
    iVar1 = iVar1 - *(int *)(&DAT_00532620 + iVar2 * 4);
    if (((iVar1 < 0xb) || (iVar1 < 0x16)) || ((iVar1 < 0x21 || ((iVar1 < 0x2c || (iVar1 < 0x37))))))
    {
      if (param_2 == 0) {
        param_2 = FUN_004479c0(this);
      }
      uVar5 = param_2 * 0x32 + DAT_005c8464;
      if ((DAT_005c8464 + 0x32U <= uVar5) && (uVar5 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
        if (param_3 == 0) {
          iVar1 = FUN_00447890(this,0xffffffff);
          iVar1 = iVar1 + 1;
        }
        else {
          iVar1 = FUN_00447890(this,0xffffffff);
        }
        iVar2 = FUN_00447890(this,0xffffffff);
        if (iVar2 < 5) {
          FUN_00447b30(this,&iStack_10,iVar1);
          uStack_8 = *(undefined4 *)(uVar5 + 0xe);
          uStack_4 = *(undefined4 *)(uVar5 + 0x12);
          iVar2 = FUN_004e49c0();
          iVar3 = FUN_004e49c0();
          if (iVar2 == iVar3) {
            uStack_4 = *(undefined4 *)(uVar5 + 0x12);
            uStack_8 = *(undefined4 *)(uVar5 + 0xe);
            iVar2 = FUN_004e49c0();
            iVar3 = FUN_004e49c0();
            if (iVar2 == iVar3) {
              FUN_00447e40(this,(int)(uVar5 - DAT_005c8464) / 0x32,0);
              return;
            }
          }
          if (DAT_00540bc4 != 0) {
            iVar2 = FUN_00427d30(uVar5);
            if (iVar2 + 1 < 4) {
              piVar4 = (int *)FUN_00489080((int)(uVar5 - DAT_005c8464) / 0x32,iStack_10,iStack_c,1,0
                                           ,((int)this - DAT_005c8464) / 0x32,iVar1,0);
              if (DAT_005e4794 == 0) {
                if (piVar4 == (int *)0x0) {
                  return;
                }
              }
              else if (piVar4 == (int *)0x0) {
                FUN_004e0620(&DAT_005078ac,".\\Dais.cpp",0x18e);
                return;
              }
              iVar1 = piVar4[4];
              piVar4[iVar1 * 2 + 3] = iStack_10;
              (piVar4 + iVar1 * 2 + 3)[1] = iStack_c;
              (**(code **)(*piVar4 + 0x30))(0,0);
              piVar4[0x61b6] = iVar2 + 1;
            }
          }
        }
      }
    }
  }
  return;
}


// ==== FUN_00449710 @ 00449710

undefined4 __thiscall FUN_00449710(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  undefined4 uStack_14;
  undefined **appuStack_10 [2];
  int iStack_8;
  undefined4 uStack_4;
  
  iVar4 = *(int *)((int)this + 0x24);
  iVar1 = (**(code **)(*(int *)this + 0x80))();
  iVar4 = iVar4 - *(int *)(&DAT_00532620 + iVar1 * 4);
  if (((((10 < iVar4) && (0x15 < iVar4)) && (0x20 < iVar4)) && ((0x2b < iVar4 && (iVar4 < 0x37))))
     && (iVar4 = FUN_00447dc0(this), iVar4 != 0)) {
    return 0;
  }
  iVar4 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  uVar2 = iVar4 * 0x32 + DAT_005c8464;
  if (((uVar2 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar2)) ||
     ((iVar4 = FUN_004ad710(uVar2), iVar4 == 0xc || (iVar4 = FUN_004ad710(uVar2), iVar4 == 0xb)))) {
    return 0;
  }
  uStack_14 = 0;
  appuStack_10[0] = &PTR_FUN_00500ff4;
  uStack_4 = 0;
  FUN_004b2270(appuStack_10,((int)this - DAT_005c8464) / 0x32);
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  if ((DAT_005e4794 != 0) && (uVar3 = FUN_004ac200(iVar4), (uVar3 & 0x200000) == 0)) {
    FUN_004e0620("priest.getGenus() & gPRIEST",".\\Dais.cpp",0x140);
  }
  uVar3 = FUN_004ac200(iVar4);
  if (((uVar3 & 0x200000) != 0) && (*(char *)(iVar4 + 0x22) == *(char *)((int)this + 0x22))) {
    while( true ) {
      uVar3 = iStack_8 * 0x32 + DAT_005c8464;
      if (uVar3 < DAT_005c8464 + 0x32U) {
        return 0;
      }
      if ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3) {
        return 0;
      }
      if (((*(byte *)(uVar3 + 0x14) & 0x20) != 0) && (*(ushort *)(uVar3 + 0x12) == DAT_005412d0))
      break;
      FUN_004b1b20(appuStack_10);
    }
    if (*(int *)(&DAT_00595200 +
                ((*(byte *)(iStack_8 * 0x32 + 0xc + DAT_005c8464) & 0x7f) +
                (uint)*(byte *)(uVar2 + 0x22) * 9) * 4) == 0) {
      uStack_14 = 1;
    }
  }
  return uStack_14;
}


// ==== FUN_00448170 @ 00448170

int __cdecl FUN_00448170(undefined4 param_1,uint param_2,uint param_3)

{
  bool bVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_00500ff4;
  local_4 = 1;
  FUN_004b2270(local_10,param_1);
  while( true ) {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(local_8 * 0x32 + 0x12 + DAT_005c8464) != param_3)) {
      bVar1 = false;
    }
    if (((param_2 == 0) || ((param_2 & *(ushort *)(local_8 * 0x32 + 0x14 + DAT_005c8464)) != 0)) &&
       (bVar1)) break;
    FUN_004b1b20(local_10);
  }
  return (local_8 * 0x32) / 0x32;
}


// ==== FUN_00448220 @ 00448220

undefined2 __fastcall FUN_00448220(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_00448170((param_1 - DAT_005c8464) / 0x32,4,0);
  if (iVar1 != 0) {
    return *(undefined2 *)(iVar1 * 0x32 + 0x12 + DAT_005c8464);
  }
  return 0;
}


// ==== FUN_00449b30 @ 00449b30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00449b30(void *this)

{
  ushort uVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined1 auStack_108 [4];
  undefined1 auStack_104 [4];
  undefined1 auStack_100 [256];
  
  FUN_00452fe0(DAT_00542a70,0,0);
  iVar4 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  if (iVar4 == 0) {
    FUN_004e0620("altarSid",".\\Dais.cpp",0x2ba);
  }
  iVar6 = iVar4 * 0x32 + DAT_005c8464;
  iVar4 = FUN_004ac550(iVar6);
  FUN_004acee0(iVar6,*(undefined4 *)(iVar4 + 0x16c),0);
  uVar1 = *(ushort *)(iVar6 + 0xc);
  if ((DAT_00594fa4 == 0) &&
     (iVar4 = FUN_00448170(((int)this - DAT_005c8464) / 0x32,4,*(undefined1 *)((int)this + 10)),
     iVar4 != 0)) {
    uVar8 = 0;
    iVar4 = (**(code **)(*(int *)this + 0x80))(0);
    FUN_004acee0(this,(&DAT_00532624)[iVar4],uVar8);
    iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c;
    uVar8 = (**(code **)(*(int *)this + 0x80))();
    *(undefined4 *)(iVar4 + 0xa0) = uVar8;
    iVar4 = FUN_00448170(((int)this - DAT_005c8464) / 0x32,0,*(undefined1 *)((int)this + 10));
    piVar5 = (int *)(iVar4 * 0x32 + DAT_005c8464);
    if (((int *)(DAT_005c8464 + 0x32U) <= piVar5) &&
       (piVar5 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      (**(code **)(*piVar5 + 0x10))(0x400048);
    }
    iVar4 = FUN_00448170(((int)this - DAT_005c8464) / 0x32,0,DAT_005412d0);
    piVar5 = (int *)(iVar4 * 0x32 + DAT_005c8464);
    if (((int *)(DAT_005c8464 + 0x32U) <= piVar5) &&
       (piVar5 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      (**(code **)(*piVar5 + 0x10))(0x400048);
      FUN_004ac220(iVar6,0);
    }
    if (*(byte *)((int)this + 0x22) == _DAT_00540c70) {
      FUN_004cf960("AltarUpgrade");
    }
  }
  else {
    iVar4 = FUN_00448170(((int)this - DAT_005c8464) / 0x32,4,0);
    if (iVar4 == 0) {
      uVar7 = 0;
    }
    else {
      uVar7 = (uint)*(ushort *)(iVar4 * 0x32 + 0x12 + DAT_005c8464);
    }
    if (DAT_00594fa4 == 0) {
      FUN_00448260(uVar7,*(undefined1 *)((int)this + 0x22));
    }
    *(uint *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0xa0 + DAT_0059534c) = uVar7;
  }
  iVar4 = 1;
  do {
    FUN_00495060(iVar4);
    iVar4 = iVar4 + 1;
  } while (iVar4 < 9);
  if (DAT_00594fa4 == 0) {
    if (*(byte *)((int)this + 0x22) == _DAT_00540c70) {
      FUN_0044fc00();
      FUN_004beb30(&DAT_005c89f0 + DAT_00540cac * 5);
      FUN_004bfac0();
    }
    if (uVar1 == _DAT_00540c70) {
      cVar3 = FUN_0048f550((uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c);
      FUN_004cef50(auStack_108,(int)cVar3);
      iVar4 = (uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c;
      uVar8 = FUN_00448fd0();
      uVar8 = FUN_004a89c0(auStack_100,"MultiplayerPriestDead(%s,%s,%d)",auStack_108,iVar4 + 0xc,
                           uVar8);
      FUN_004cf960(uVar8);
    }
    else if (*(byte *)((int)this + 0x22) != _DAT_00540c70) {
      iVar4 = (uint)uVar1 * 0xb4;
      cVar3 = FUN_0048f550(iVar4 + DAT_0059534c);
      FUN_004cef50(auStack_104,(int)cVar3);
      cVar3 = FUN_0048f550((uint)*(byte *)((int)this + 0x22) * 0xb4 + DAT_0059534c);
      FUN_004cef50(auStack_108,(int)cVar3);
      uVar8 = FUN_004a89c0(auStack_100,"MultiplayerOtherPriestDead(%s,%s,%s,%s)",auStack_104,
                           iVar4 + 0xc + DAT_0059534c,auStack_108,
                           (uint)*(byte *)((int)this + 0x22) * 0xb4 + 0xc + DAT_0059534c);
      FUN_004cf960(uVar8);
    }
    if (DAT_00594fa4 == 0) {
      return;
    }
  }
  if (_DAT_00540c70 == *(byte *)((int)this + 0x22)) {
    FUN_0040eec0(0x459c4000);
    bVar2 = false;
    iVar4 = 1;
    do {
      iVar6 = FUN_004922e0(iVar4);
      if ((iVar6 == 0) && (*(int *)(&DAT_00595200 + (iVar4 + _DAT_00540c70 * 9) * 4) == 0)) {
        bVar2 = true;
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < 9);
    if (bVar2) {
      FUN_004cf960("SacrificeForStormpower");
    }
  }
  return;
}


// ==== FUN_00427ce0 @ 00427ce0

undefined4 __thiscall FUN_00427ce0(void *this)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = (int)this - DAT_005c8464;
  uVar1 = FUN_00488fe0(*(undefined1 *)((int)this + 10),0,0);
  iVar2 = FUN_004b1a90(iVar2 / 0x32,uVar1);
  if (iVar2 != 0) {
    uVar1 = FUN_004ae2d0(iVar2 * 0x32 + DAT_005c8464);
    return uVar1;
  }
  return 0;
}


// ==== FUN_00427d30 @ 00427d30

undefined4 __thiscall FUN_00427d30(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = FUN_00427ce0();
  if (iVar2 != 0) {
    if (DAT_005e4794 != 0) {
      iVar1 = *(int *)(iVar2 + 0x186d8);
      if (iVar1 < 0) {
        FUN_004e0620("0 <= p->moveAwayDepth","\\Ns\\O\\Carrier.cpp",0x87b);
      }
      if ((DAT_005e4794 != 0) && (3 < iVar1)) {
        FUN_004e0620("p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH","\\Ns\\O\\Carrier.cpp",0x87c);
      }
    }
    return *(undefined4 *)(iVar2 + 0x186d8);
  }
  return 0;
}


// ==== FUN_00496e70 @ 00496e70

float10 FUN_00496e70(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_0044b900 @ 0044b900

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044b900(void *this,int param_2,uint param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  float10 fVar3;
  
  if (param_2 == DAT_005326c0) {
    FUN_00470ac0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
    iVar1 = (**(code **)(*(int *)this + 0x98))(param_3 & 0xffff,(int)param_3 >> 0x10,0);
    if (iVar1 == 0) {
      uVar2 = FUN_004ac200(this);
      if ((uVar2 & 0x200000) == 0) {
        return (float10)_DAT_00502350;
      }
    }
  }
  else {
    if (param_2 != DAT_005326c4) {
      fVar3 = (float10)FUN_00496e70(param_2,param_3,param_4);
      return fVar3;
    }
    if ((*(byte *)((int)this + 0x28) & 1) != 0) {
      iVar1 = FUN_004b1a90(((int)this - DAT_005c8464) / 0x32,DAT_005410e8,0,0);
      if (iVar1 == 0) {
        FUN_00443fe0(((int)this - DAT_005c8464) / 0x32,0);
      }
    }
  }
  return (float10)_DAT_00500fc4;
}


// ==== FUN_00448260 @ 00448260

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00448260(int param_1,int param_2)

{
  undefined4 uVar1;
  bool bVar2;
  
  bVar2 = param_2 == _DAT_00540c70;
  *(undefined1 *)(param_2 * 0xb4 + 0x71 + DAT_0059534c) = 0;
  if (param_1 != DAT_005412c0) {
    if (param_1 == DAT_005411cc) {
      if (bVar2) {
        FUN_0044fa00(0);
        return;
      }
    }
    else {
      if (bVar2) {
        thunk_FUN_00492a30();
        DAT_0055a420 = param_1;
        return;
      }
      FUN_0048f380(param_2,param_1);
    }
    return;
  }
  if (bVar2) {
    FUN_0040eec0(0x459c4000);
    FUN_004cf960("SacrificeForStormpower");
    return;
  }
  FUN_0040ef30(param_2);
  uVar1 = FUN_004e49c0();
  FUN_00447810(param_2,uVar1);
  return;
}


// ==== FUN_00448c80 @ 00448c80

void __thiscall FUN_00448c80(void *this)

{
  int iVar1;
  void *pvVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9f7b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),DAT_005412ec);
  iVar1 = FUN_004ac550(this);
  FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x15c),0);
  pvVar2 = operator_new(0x28);
  local_4 = 0;
  if (pvVar2 != (void *)0x0) {
    FUN_00496f00(pvVar2,0x18,((int)this - DAT_005c8464) / 0x32,0x3df5c28f);
  }
  ExceptionList = local_c;
  return;
}


