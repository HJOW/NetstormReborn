// ==== FUN_0044a5e0 @ 0044a5e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0044a5e0(void *this,int param_2,int param_3,float param_4)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  void *pvVar4;
  int *piVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  void *pvVar9;
  float10 fVar10;
  float local_14;
  void *local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9fdb;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412ec);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  pvVar9 = local_10;
  if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    local_10 = *(void **)(uVar2 + 0x12);
    local_14 = *(float *)(uVar2 + 0xe) - _DAT_00501908;
    pvVar9 = (void *)((float)local_10 - _DAT_00501908);
  }
  if (param_2 == 0x15) {
    iVar1 = FUN_004b26f0((int)(uVar2 - DAT_005c8464) / 0x32,DAT_005412d0,0,0);
    uVar3 = iVar1 * 0x32 + DAT_005c8464;
    if ((((DAT_00540bc4 != 0) && (DAT_005c8464 + 0x32U <= uVar3)) &&
        (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
       (*(int *)(&DAT_00595200 +
                ((*(byte *)(uVar3 + 0xc) & 0x7f) + (uint)*(byte *)(uVar2 + 0x22) * 9) * 4) != 0)) {
      FUN_00448bd0(this);
      ExceptionList = local_c;
      return (float10)_DAT_00502350;
    }
    iVar1 = FUN_004ad710(this);
    if (iVar1 != 0xf) {
      uVar8 = 0x14;
      iVar1 = FUN_004558f0(0,100000);
      if (iVar1 / 1000 < DAT_0053269c) {
        uVar8 = 0x16;
        FUN_004afc90(this,0xffffffff,0);
      }
      pvVar4 = operator_new(0x28);
      local_4 = 0;
      if (pvVar4 != (void *)0x0) {
        FUN_00496f00(pvVar4,uVar8,((int)this - DAT_005c8464) / 0x32,0x3da3d70a);
      }
      local_4 = 0xffffffff;
      if (((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))
         && (iVar1 = FUN_004ad710(uVar2), iVar1 != 10)) {
        iVar1 = FUN_004558f0(0,5,0);
        FUN_004a9d70(local_14,pvVar9,(&PTR_s_chainRattle23_wav_005326a0)[iVar1]);
        iVar1 = FUN_004558f0(0,3000,0);
        FUN_004a9d70(local_14,pvVar9,(&PTR_s_priestStruggleFade01_wav_005326b4)[iVar1 / 1000]);
      }
      fVar10 = (float10)FUN_00455930(0,0x40000000);
      ExceptionList = local_c;
      return fVar10 + (float10)DAT_00532698;
    }
  }
  else {
    if ((param_2 == 0x14) || (param_2 == 0x16)) {
      iVar1 = FUN_004ad710(this);
      if (iVar1 == 0xe) {
        iVar1 = (uint)(param_2 == 0x14) * 2 + -1;
        iVar6 = FUN_004afc90(this,iVar1,0);
        if (iVar6 == 0) goto LAB_0044aa48;
        if (iVar1 == -1) {
          FUN_004afc90(this,1,0);
        }
      }
      ExceptionList = local_c;
      return (float10)_DAT_00500fc4;
    }
    if (param_2 != 0x17) {
      if (param_2 != 0x18) {
        fVar10 = (float10)FUN_0044b900(this,param_2,param_3,param_4);
        ExceptionList = local_c;
        return fVar10;
      }
      iVar1 = FUN_00448bb0(this);
      piVar5 = (int *)(iVar1 * 0x32 + DAT_005c8464);
      iVar1 = FUN_004ad710(piVar5);
      if (iVar1 == 10) {
        if (DAT_00540bc4 != 0) {
          (**(code **)(*piVar5 + 0x10))(0x400000);
          ExceptionList = local_10;
          return (float10)_DAT_00502350;
        }
      }
      else {
        iVar1 = FUN_004afc90(this,1,0);
        if (iVar1 == 0) {
          if (((param_3 < 3) || (iVar1 = FUN_004558f0(0,100000), iVar1 / 1000 < 0x32)) &&
             (iVar1 = FUN_0040ea40(piVar5), iVar1 != 0)) {
            FUN_004486d0(piVar5,param_3);
          }
          fVar10 = (float10)FUN_00455930(0,0x3f800000);
          if (fVar10 < (float10)param_4) {
            FUN_004a9d70(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         "altarBurnCollapse.wav",0);
          }
        }
        else {
          iVar6 = FUN_00448bb0(this);
          iVar1 = DAT_005c8464;
          iVar6 = iVar6 * 0x32 + DAT_005c8464;
          iVar7 = FUN_0040ea40(iVar6);
          if (iVar7 != 0) {
            if ((DAT_00540bc4 != 0) &&
               (iVar1 = FUN_004b26f0((iVar6 - iVar1) / 0x32,DAT_005412d0,0,0), iVar1 != 0)) {
              (**(code **)(*(int *)(iVar1 * 0x32 + DAT_005c8464) + 0x10))(8);
            }
            FUN_004605d0(((int)this - DAT_005c8464) / 0x32);
            FUN_004a9d70(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         "explosion.wav",0);
            iVar1 = FUN_004ac550(iVar6);
            FUN_004acee0(iVar6,*(undefined4 *)(iVar1 + 0x158),0);
            iVar1 = FUN_004ac550(this);
            FUN_004acee0(this,*(undefined4 *)(iVar1 + 0x160),0);
            ExceptionList = local_c;
            return (float10)_DAT_00501908;
          }
        }
      }
    }
  }
LAB_0044aa48:
  ExceptionList = local_c;
  return (float10)param_4;
}


// ==== FUN_004486d0 @ 004486d0

void __thiscall FUN_004486d0(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004ad710();
  iVar2 = FUN_004ac550(this);
  if (param_2 < 3) {
    if (param_2 == 0) {
      iVar1 = (**(code **)(*(int *)this + 0x80))();
      iVar1 = 0xe - iVar1;
    }
    FUN_004acee0(this,*(int *)(iVar2 + 0x130 + iVar1 * 4) + param_2,0);
    return;
  }
  FUN_004558f0(3000,*(int *)(iVar2 + 0x170 + iVar1 * 4) * 1000);
  return;
}


// ==== FUN_00448bb0 @ 00448bb0

void __thiscall FUN_00448bb0(void *this)

{
  FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),DAT_005412ec);
  return;
}


// ==== FUN_00448bd0 @ 00448bd0

void __thiscall FUN_00448bd0(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412ec);
  uVar2 = iVar1 * 0x32 + DAT_005c8464;
  if (DAT_005e4794 != 0) {
    iVar1 = FUN_004ad710(this);
    if (iVar1 != 0xc) {
      iVar1 = FUN_004ad710(this);
      if (iVar1 != 0xb) goto LAB_00448c2d;
    }
    FUN_004e0620("!isInvisibleFrame() && !isSacrifice()",".\\Dais.cpp",0x578);
  }
LAB_00448c2d:
  if ((DAT_005c8464 + 0x32U <= uVar2) && (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    FUN_00448500(uVar2);
  }
  (**(code **)(*(int *)this + 0x48))(0);
  iVar1 = FUN_004ac550(this);
  *(undefined4 *)((int)this + 0x24) = *(undefined4 *)(iVar1 + 0x16c);
  (**(code **)(*(int *)this + 0x4c))(0);
  return;
}


// ==== FUN_00448500 @ 00448500

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00448500(void *this)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  int *piVar4;
  float *pfVar5;
  int *piVar6;
  undefined1 auStack_14 [8];
  undefined1 auStack_c [12];
  
  iVar2 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412e8);
  uVar3 = iVar2 * 0x32 + DAT_005c8464;
  if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3)) {
    FUN_004e0620("altar.isValid()",".\\Dais.cpp",0x3cb);
  }
  if (DAT_005e4794 != 0) {
    iVar2 = FUN_004ad710(uVar3);
    if (iVar2 != 0xc) {
      iVar2 = FUN_004ad710(uVar3);
      if (iVar2 != 0xb) goto LAB_00448590;
    }
    FUN_004e0620("!altar.isInvisibleFrame() && !altar.isSacrifice()",".\\Dais.cpp",0x3cc);
  }
LAB_00448590:
  if (DAT_00540bc4 == 0) {
    FUN_004e0620(&DAT_00502348,".\\Dais.cpp",0x3cd);
  }
  iVar2 = FUN_004b26f0(((int)this - DAT_005c8464) / 0x32,DAT_005412d0,0,0);
  if (iVar2 != 0) {
    piVar4 = (int *)FUN_004af530(DAT_005412d0,0);
    piVar6 = (int *)(iVar2 * 0x32 + DAT_005c8464);
    FUN_004ac220(piVar4,*(undefined2 *)(iVar2 * 0x32 + 0xc + DAT_005c8464));
    (**(code **)(*piVar4 + 0x74))(0);
    iVar2 = FUN_004adea0(this,auStack_14);
    uVar1 = *(undefined4 *)(iVar2 + 4);
    pfVar5 = (float *)FUN_004adea0(this,auStack_c);
    (**(code **)(*piVar4 + 0x90))(*pfVar5 + _DAT_00500ee4,uVar1,0);
    FUN_00429010(piVar4,0x40a00000,0);
    (**(code **)(*piVar6 + 0x10))(0);
  }
  FUN_004a9cb0("priestFree.wav",0,0,0,1,0);
  iVar2 = FUN_004479c0(this);
  uVar3 = iVar2 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar3) && (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    FUN_00491b60(uVar3);
  }
  return;
}


// ==== FUN_0044afd0 @ 0044afd0

void __thiscall FUN_0044afd0(void *this,uint param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined **local_50 [2];
  int local_48;
  undefined4 local_44;
  undefined1 local_40 [52];
  int local_c;
  
  if ((*(byte *)((int)this + 0x28) & 9) == 0) {
    if ((param_2 & 0x400000) != 0) {
      local_50[0] = &PTR_LAB_00503ac4;
      local_44 = 1;
      uVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                           DAT_005412e8);
      FUN_004b22d0(local_50,uVar1);
      while (local_48 != 0) {
        uVar1 = FUN_004b2340(local_50);
        iVar2 = FUN_004ae2d0(uVar1);
        iVar3 = FUN_0041bff0(iVar2);
        if (((iVar3 == DAT_00541128) || (iVar3 = FUN_0041bff0(iVar2), iVar3 == DAT_00541164)) &&
           (*(int *)(iVar2 + 0x18) == 0x18)) {
          FUN_00449b30(this);
        }
        FUN_004b1b80(local_50);
      }
    }
    iVar2 = FUN_004479c0(this);
    uVar4 = iVar2 * 0x32 + DAT_005c8464;
    if ((DAT_005c8464 + 0x32U <= uVar4) && (uVar4 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
      FUN_00491b60(uVar4);
    }
    puVar5 = (undefined4 *)FUN_004ade70(this,local_50,0);
    FUN_0041def0(local_40,*puVar5,puVar5[1],puVar5[2],puVar5[3],0);
    while (local_c != 0) {
      if (*(byte *)(DAT_005c8464 + 10 + local_c * 0x32) == DAT_005412e8) {
        (**(code **)(*(int *)(DAT_005c8464 + local_c * 0x32) + 0x10))(0x48);
      }
      FUN_004b1810(local_40);
    }
    iVar2 = 1;
    do {
      FUN_00495060(iVar2);
      iVar2 = iVar2 + 1;
    } while (iVar2 < 9);
  }
  FUN_0044b4b0(this,param_2);
  return;
}


// ==== FUN_0044a480 @ 0044a480

undefined4 __thiscall FUN_0044a480(void *this)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  
  iVar1 = FUN_004b1fa0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                       DAT_005412ec);
  piVar2 = (int *)(iVar1 * 0x32 + DAT_005c8464);
  if (((int *)(DAT_005c8464 + 0x32U) <= piVar2) &&
     (piVar2 < (int *)(DAT_005c847c * 0x32 + DAT_005c8464))) {
                    /* WARNING: Could not recover jumptable at 0x0044a4bb. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    uVar3 = (**(code **)(*piVar2 + 0x80))();
    return uVar3;
  }
  return 0;
}


// ==== FUN_004b26f0 @ 004b26f0

int __cdecl FUN_004b26f0(undefined4 param_1,uint param_2,int param_3,int param_4)

{
  int iVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_00500ff4;
  local_4 = 0;
  FUN_004b2270(local_10,param_1);
  do {
    if (local_8 == 0) {
      return 0;
    }
    iVar1 = local_8 * 0x32;
    if (*(ushort *)(DAT_005c8464 + 0x12 + iVar1) == param_2) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return local_8;
        }
        if ((DAT_00540bc0 != 0) || ((4 < iVar1 / 0x32 && (iVar1 / 0x32 < 15000)))) {
          (**(code **)(*(int *)(DAT_005c8464 + iVar1) + 0x10))(0);
        }
      }
      param_3 = param_3 + -1;
    }
    FUN_004b1b20(local_10);
  } while( true );
}


