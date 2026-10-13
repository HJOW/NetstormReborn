// ==== FUN_00491bb0 @ 00491bb0

undefined4 __thiscall FUN_00491bb0(void *this)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 unaff_ESI;
  
  uVar3 = *(undefined4 *)((int)this + 0xe);
  if (DAT_00594fbc != 0) {
    iVar1 = FUN_004e49c0();
    iVar2 = FUN_004e49c0();
    if ((*(byte *)(iVar2 + iVar1 * 0x100 + DAT_005c7c44) & 6) == 0) {
      if (DAT_00540bc4 != 0) {
        (**(code **)(*(int *)this + 0x48))(0);
        FUN_0041d7a0(&stack0xfffffff4);
        (**(code **)(*(int *)this + 0x90))(unaff_ESI,uVar3,0);
      }
      return 0xb;
    }
    iVar1 = *(int *)((int)this + 0x1a);
    iVar2 = FUN_004adcb0(this);
    if ((iVar1 < iVar2 / 2) && (DAT_00540bc4 != 0)) {
      (**(code **)(*(int *)this + 0x48))(0);
      FUN_0041d7a0(&stack0xfffffff4);
      (**(code **)(*(int *)this + 0x90))(unaff_ESI,uVar3,0);
      return 0xb;
    }
  }
  uVar3 = thunk_FUN_0042a5f0();
  return uVar3;
}


// ==== FUN_00491f70 @ 00491f70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00491f70(void *this,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  float10 fVar4;
  
  if (DAT_00540bc4 != 0) {
    iVar2 = *(int *)((int)this + 0x1a);
    iVar1 = FUN_004adcb0();
    if ((((iVar2 < iVar1) && (DAT_005c89b8 == 0)) && (*(char *)((int)this + 0x22) == '\0')) &&
       (DAT_00594fa4 == 0)) {
      iVar2 = FUN_0046f950(&DAT_00568b60,*(undefined4 *)((int)this + 0xe),
                           *(undefined4 *)((int)this + 0x12));
      iVar1 = iVar2 * 0x32 + DAT_005c8464;
      if ((iVar2 != 0) && (*(char *)(iVar1 + 0x22) == *(char *)((int)this + 0x22))) {
        fVar4 = (float10)FUN_0041d840(*(undefined4 *)((int)this + 0xe),
                                      *(undefined4 *)((int)this + 0x12),iVar2);
        if (fVar4 < (float10)_DAT_005012d0 != (fVar4 == (float10)_DAT_005012d0)) {
          FUN_004afa70(iVar1,*(undefined1 *)((int)this + 10),*(undefined2 *)((int)this + 0xc),0x20,0
                       ,0);
          (**(code **)(*(int *)this + 0x10))(0);
          return 0x3e9;
        }
      }
    }
    if (((DAT_00540bc4 != 0) && (DAT_005c89b8 == 0)) &&
       ((*(char *)((int)this + 0x22) == '\0' && (DAT_00594fa4 == 0)))) {
      iVar2 = *(int *)((int)this + 0x1a);
      iVar1 = FUN_004adcb0(this);
      if (iVar1 / 2 <= iVar2) {
        FUN_00491db0(this);
        return 0x3e9;
      }
    }
  }
  uVar3 = FUN_00429800(this,param_2,param_3);
  return uVar3;
}


// ==== FUN_00429800 @ 00429800

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00429800(void *this,int param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  undefined4 uVar7;
  int *piVar8;
  float *pfVar9;
  uint local_ac [3];
  undefined4 local_a0;
  int local_9c;
  int local_98;
  int local_94;
  float local_90 [4];
  undefined1 local_80 [52];
  uint local_4c;
  undefined1 local_40 [52];
  int local_c;
  
  uVar1 = FUN_004ac200();
  if ((uVar1 & 0x30000) == 0) {
    FUN_004e0620("isCarrier()","\\Ns\\O\\Carrier.cpp",0x6ca);
  }
  uVar1 = FUN_004ac200(this);
  if ((uVar1 & 0x30000) == 0) {
    return 1000;
  }
  uVar1 = FUN_004ac200(this);
  if ((((uVar1 & 0x200000) != 0) && (iVar2 = FUN_00427ce0(this), iVar2 != 0)) &&
     (*(int *)(iVar2 + 0x186d4) != 0)) {
    return 1000;
  }
  if (*(char *)((int)this + 0x22) == '\0') {
    return 1000;
  }
  local_94 = 0;
  local_9c = FUN_004ac9f0(this,0x20,DAT_005412c0,0);
  iVar2 = FUN_004b2070(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),0x10000000
                      );
  FUN_004b2070(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),0x2000);
  local_98 = FUN_004263a0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1);
  iVar3 = FUN_004ac9f0(this,0x20,DAT_005412d0,0);
  if (iVar3 != 0) {
    local_98 = 0;
    iVar2 = 0;
    local_9c = 0;
  }
  iVar4 = FUN_004b21d0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1,
                       DAT_005412ec);
  if ((iVar4 != 0) &&
     (((*(byte *)(iVar4 * 0x32 + 0x28 + DAT_005c8464) & 1) != 0 ||
      (iVar5 = FUN_00448740(iVar4 * 0x32 + DAT_005c8464,*(undefined4 *)((int)this + 0xe),
                            *(undefined4 *)((int)this + 0x12)), iVar5 == 0)))) {
    iVar4 = 0;
  }
  FUN_00426900(this,local_ac,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
  local_ac[2] = FUN_004ac9f0(this,0x20,0,0);
  FUN_004ac9f0(this,1,0,0);
  iVar5 = FUN_00427ce0(this);
  if ((iVar5 == 0) || (*(int *)(iVar5 + 0x186d8) == 0)) {
    uVar1 = FUN_004ac1e0(this);
    iVar5 = local_98;
    if ((((uVar1 & 0x1000000) == 0) && (local_ac[0] != 0)) &&
       ((local_ac[1] != 0 || (local_ac[2] == 0)))) {
      FUN_00428e00(this);
    }
    else {
      if (iVar4 != 0) {
        if (DAT_00540bc4 != 0) {
          if (iVar3 != 0) {
            iVar2 = iVar4 * 0x32 + DAT_005c8464;
            piVar8 = (int *)(iVar3 * 0x32 + DAT_005c8464);
            if ((*(int *)(&DAT_00595200 +
                         ((uint)*(byte *)((int)this + 0x22) * 9 +
                         (uint)*(byte *)(iVar4 * 0x32 + 0x22 + DAT_005c8464)) * 4) != 0) &&
               (iVar3 = FUN_004ac9f0(iVar2,0x20,DAT_005412d0,0), iVar3 == 0)) {
              FUN_004afa70(iVar2,DAT_005412d0,(short)piVar8[3],0x20,0,0);
              (**(code **)(*piVar8 + 0x10))(0);
            }
          }
          FUN_00429010(this,0x40a00000,0);
          return 0x3e9;
        }
        goto LAB_00429b95;
      }
      if ((local_98 == 0) || (local_9c == 0)) {
        if (iVar2 != 0) {
          iVar3 = FUN_00462c80(((int)this - DAT_005c8464) / 0x32);
          iVar4 = FUN_00462c80(iVar2);
          if (iVar4 == iVar3) {
            if (DAT_00540bc4 == 0) goto LAB_00429b95;
            iVar3 = FUN_0040f100(this);
            if (iVar3 == 0) {
              if ((param_3 == 0) || (iVar3 = FUN_004274b0(this,param_3), iVar3 == 0)) {
                param_3 = FUN_00428ea0(this);
              }
              FUN_004261d0(((int)this - DAT_005c8464) / 0x32,iVar2,param_3);
              if (param_3 != 0) {
                local_94 = 1;
                FUN_004276a0(this,param_3);
              }
            }
            if (((DAT_00540bc4 != 0) && (local_9c == 0)) &&
               (*(short *)(iVar2 * 0x32 + 0xc + DAT_005c8464) != 0)) {
              FUN_004afa70(this,DAT_005412c0,0,0x20,0,0);
              FUN_0040ac20(local_ac + 2);
              local_a0 = CONCAT22(local_a0._2_2_,(short)iVar2);
              FUN_004e2e30(local_ac + 2);
              FUN_004266c0(0,0,local_ac + 2);
            }
            if (local_94 != 0) {
              return 0x3e9;
            }
          }
        }
      }
      else if (*(char *)(local_98 * 0x32 + 0x22 + DAT_005c8464) == *(char *)((int)this + 0x22)) {
        iVar2 = FUN_00462c80(((int)this - DAT_005c8464) / 0x32);
        iVar3 = FUN_00462c80(iVar5);
        if (iVar3 == iVar2) {
          if (DAT_00540bc4 == 0) goto LAB_00429b95;
          iVar2 = FUN_0040f100(this);
          if (iVar2 == 0) {
            if ((param_2 == 0) || (iVar2 = FUN_00427580(this,param_2), iVar2 == 0)) {
              param_2 = FUN_00429420(this);
              FUN_004261d0(((int)this - DAT_005c8464) / 0x32,param_2,0);
            }
            if (param_2 != 0) {
              FUN_00427600(this,param_2);
              return 0x3e9;
            }
          }
        }
      }
    }
  }
  if ((DAT_00540bc4 != 0) &&
     ((uVar1 = FUN_004ac200(this), (uVar1 & 0x200000) == 0 ||
      (iVar2 = FUN_004b21d0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1,
                            DAT_005412ec), iVar2 == 0)))) {
    FUN_004202f0(local_80,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),1);
    while (local_4c != 0) {
      uVar1 = FUN_004ac200(local_4c * 0x32 + DAT_005c8464);
      if ((((uVar1 & 0x30000) != 0) && (local_4c != ((int)this - DAT_005c8464) / 0x32)) &&
         (local_4c != local_ac[0])) {
        uVar7 = FUN_00429010(this,0x3f800000,0xffffffff);
        return uVar7;
      }
      if ((*(byte *)(local_4c * 0x32 + 10 + DAT_005c8464) == DAT_005411a8) &&
         (iVar2 = FUN_0040f100(this), iVar2 == 0)) {
        uVar7 = FUN_00429010(this,0x40000000,0x1da1a);
        return uVar7;
      }
      FUN_004b1810(local_80);
    }
    iVar2 = FUN_0040f100(this);
    if ((iVar2 == 0) && (uVar1 = FUN_004ac200(this), (uVar1 & 0x20000) == 0)) {
      FUN_0040d830(local_90,8,2,FUN_0042b1a0);
      local_90[0] = *(float *)((int)this + 0xe);
      local_90[1] = *(float *)((int)this + 0x12);
      local_90[2] = *(float *)((int)this + 0xe);
      local_90[3] = *(float *)((int)this + 0x12) + _DAT_00501444;
      local_ac[2] = 0x3f800000;
      local_a0 = 0x40000000;
      local_ac[0] = 0x4200;
      local_ac[1] = 0x200;
      iVar2 = 0;
      pfVar9 = local_90;
      do {
        FUN_004202f0(local_40,*pfVar9,pfVar9[1],1);
        if (local_c != 0) {
          uVar1 = local_ac[iVar2];
          do {
            uVar6 = FUN_004ac200(local_c * 0x32 + DAT_005c8464);
            if ((uVar1 & uVar6) != 0) {
              uVar7 = FUN_00429010(this,local_ac[iVar2 + 2],0x1da1a);
              return uVar7;
            }
            FUN_004b1810(local_40);
          } while (local_c != 0);
        }
        iVar2 = iVar2 + 1;
        pfVar9 = pfVar9 + 2;
      } while (iVar2 < 2);
    }
  }
LAB_00429b95:
  if ((DAT_005e4794 != 0) && (iVar2 = FUN_004afd00(this,1), iVar2 == 0)) {
    FUN_004e0620("isRobust()","\\Ns\\O\\Carrier.cpp",0x79c);
  }
  return 1000;
}


// ==== thunk_FUN_0042a5f0 @ 0042ab70

undefined4 __fastcall thunk_FUN_0042a5f0(int *param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined **appuStack_40 [13];
  int iStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar2 = (**(code **)(*param_1 + 0xcc))();
  if (iVar2 != 0) {
    return 0xc;
  }
  uStack_48 = *(undefined4 *)((int)param_1 + 0xe);
  uStack_44 = *(undefined4 *)((int)param_1 + 0x12);
  appuStack_40[0] = &PTR_FUN_00502318;
  uStack_8 = 0;
  uStack_4 = 0;
  FUN_0041d770(&uStack_48);
  uVar3 = FUN_004e49c0();
  uVar4 = FUN_004e49c0();
  FUN_004b16d0(appuStack_40,uVar4,uVar3,uVar4,uVar3);
  do {
    if (iStack_c == 0) {
      uVar3 = FUN_004ac990();
      return uVar3;
    }
    uVar5 = (uint)*(byte *)(iStack_c * 0x32 + 10 + DAT_005c8464);
    uVar1 = iStack_c * 0x32 + DAT_005c8464;
    if ((uVar5 == DAT_005412c8) && (iStack_c != ((int)param_1 - DAT_005c8464) / 0x32)) {
      if (((DAT_005c8464 + 0x32U <= uVar1) && (uVar1 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))
         && (*(byte *)(uVar1 + 10) == DAT_005412c8)) {
        iVar2 = FUN_00429ed0();
LAB_0042a713:
        if (iVar2 == 0xc) {
          return 0xc;
        }
      }
    }
    else if (((uVar5 == DAT_00541344) &&
             (((iStack_c != ((int)param_1 - DAT_005c8464) / 0x32 && (DAT_005c8464 + 0x32U <= uVar1))
              && (uVar1 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))))) &&
            (*(byte *)(uVar1 + 10) == DAT_00541344)) {
      iVar2 = FUN_0042a0d0();
      goto LAB_0042a713;
    }
    FUN_004b1810(appuStack_40);
  } while( true );
}


// ==== FUN_0042a5f0 @ 0042a5f0

undefined4 __fastcall FUN_0042a5f0(int *param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined **appuStack_40 [13];
  int iStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar2 = (**(code **)(*param_1 + 0xcc))();
  if (iVar2 != 0) {
    return 0xc;
  }
  uStack_48 = *(undefined4 *)((int)param_1 + 0xe);
  uStack_44 = *(undefined4 *)((int)param_1 + 0x12);
  appuStack_40[0] = &PTR_FUN_00502318;
  uStack_8 = 0;
  uStack_4 = 0;
  FUN_0041d770(&uStack_48);
  uVar3 = FUN_004e49c0();
  uVar4 = FUN_004e49c0();
  FUN_004b16d0(appuStack_40,uVar4,uVar3,uVar4,uVar3);
  do {
    if (iStack_c == 0) {
      uVar3 = FUN_004ac990();
      return uVar3;
    }
    uVar5 = (uint)*(byte *)(iStack_c * 0x32 + 10 + DAT_005c8464);
    uVar1 = iStack_c * 0x32 + DAT_005c8464;
    if ((uVar5 == DAT_005412c8) && (iStack_c != ((int)param_1 - DAT_005c8464) / 0x32)) {
      if (((DAT_005c8464 + 0x32U <= uVar1) && (uVar1 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))
         && (*(byte *)(uVar1 + 10) == DAT_005412c8)) {
        iVar2 = FUN_00429ed0();
LAB_0042a713:
        if (iVar2 == 0xc) {
          return 0xc;
        }
      }
    }
    else if (((uVar5 == DAT_00541344) &&
             (((iStack_c != ((int)param_1 - DAT_005c8464) / 0x32 && (DAT_005c8464 + 0x32U <= uVar1))
              && (uVar1 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))))) &&
            (*(byte *)(uVar1 + 10) == DAT_00541344)) {
      iVar2 = FUN_0042a0d0();
      goto LAB_0042a713;
    }
    FUN_004b1810(appuStack_40);
  } while( true );
}


// ==== FUN_0040ac20 @ 0040ac20

void * __thiscall FUN_0040ac20(void *this)

{
  *(undefined1 *)((int)this + 1) = 0;
  *(undefined1 *)this = 0;
  *(undefined2 *)((int)this + 2) = 0;
  *(undefined1 *)((int)this + 1) = DAT_0052ee32;
  FUN_004df230(this,DAT_0052ee34);
  *(byte *)this = DAT_0052ee33 & 0x7f;
  return this;
}


// ==== FUN_0040d830 @ 0040d830

void FUN_0040d830(undefined4 param_1,undefined4 param_2,int param_3,code *param_4)

{
  if (-1 < param_3 + -1) {
    do {
      (*param_4)();
      param_3 = param_3 + -1;
    } while (param_3 != 0);
  }
  return;
}


// ==== FUN_0040f100 @ 0040f100

undefined4 __thiscall FUN_0040f100(void *this)

{
  undefined4 uVar1;
  
  uVar1 = 1;
  if ((*(byte *)((int)this + 0x22) == 0) ||
     (*(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0xb0 + DAT_0059534c) == 0)) {
    uVar1 = 0;
  }
  return uVar1;
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


// ==== FUN_0041d840 @ 0041d840

float10 __cdecl FUN_0041d840(float param_1,float param_2,int param_3)

{
  float10 fVar1;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_004ade70(param_3 * 0x32 + DAT_005c8464,&local_10,0);
  if (local_10 <= param_1) {
    local_10 = param_1 - local_8;
  }
  else {
    local_10 = local_10 - param_1;
  }
  if (local_c <= param_2) {
    fVar1 = (float10)param_2 - (float10)local_4;
  }
  else {
    fVar1 = (float10)local_c - (float10)param_2;
  }
  if (fVar1 <= (float10)local_10) {
    fVar1 = (float10)local_10;
  }
  return fVar1;
}


// ==== FUN_004202f0 @ 004202f0

void * __thiscall FUN_004202f0(void *this,undefined4 param_2,undefined4 param_3,uint param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  *(uint *)((int)this + 0x38) = param_4 & 1;
  *(undefined ***)this = &PTR_FUN_00502318;
  *(uint *)((int)this + 0x3c) = param_4 & 4;
  FUN_0041d770(&param_2);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  FUN_004b16d0(this,uVar2,uVar1,uVar2,uVar1);
  return this;
}


// ==== FUN_004261d0 @ 004261d0

void __cdecl FUN_004261d0(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_00488fe0(*(undefined1 *)(param_1 * 0x32 + DAT_005c8464 + 10),0,0);
  iVar2 = FUN_004b1a90(param_1,uVar1);
  if (iVar2 != 0) {
    iVar2 = FUN_004ae2d0(iVar2 * 0x32 + DAT_005c8464);
    *(undefined4 *)(iVar2 + 0x186cc) = param_2;
    *(undefined4 *)(iVar2 + 0x186d0) = param_3;
  }
  return;
}


// ==== FUN_004263a0 @ 004263a0

int __cdecl FUN_004263a0(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_40[0] = &PTR_FUN_00502318;
  local_4 = 0;
  local_c = 0;
  local_8 = 1;
  if (param_3 == 0) {
    uVar1 = FUN_004e49c0();
    uVar2 = FUN_004e49c0();
    uVar4 = uVar2;
    uVar3 = uVar1;
  }
  else {
    uVar3 = FUN_004e49c0();
    uVar4 = FUN_004e49c0(uVar3);
    uVar1 = FUN_004e49c0(uVar4);
    uVar2 = FUN_004e49c0(uVar1);
  }
  FUN_004b16d0(local_40,uVar2,uVar1,uVar4,uVar3);
  while( true ) {
    if (local_c == 0) {
      return 0;
    }
    iVar5 = DAT_005c8464 + local_c * 0x32;
    if ((((*(byte *)(iVar5 + 0x28) & 1) == 0) && ((*(byte *)(iVar5 + 0xb) & 2) == 0)) &&
       ((iVar5 = FUN_004ac550(), (*(uint *)(iVar5 + 0xec) & 0x200) != 0 ||
        (iVar5 = FUN_0049adc0(iVar5), iVar5 == DAT_00541250)))) break;
    FUN_004b1810(local_40);
  }
  return local_c;
}


// ==== FUN_004266c0 @ 004266c0

void __cdecl FUN_004266c0(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int *piVar4;
  undefined1 local_100 [256];
  
  iVar3 = (uint)*(ushort *)(param_3 + 4) * 0x32;
  piVar4 = (int *)(iVar3 + DAT_005c8464);
  if ((*(byte *)(iVar3 + 0xb + DAT_005c8464) & 1) == 0) {
    iVar3 = FUN_004558f0(1000,7000);
    uVar2 = FUN_004a89c0(local_100,"geyserMined%d.wav",iVar3 / 1000);
    FUN_004a9d70(*(undefined4 *)((int)piVar4 + 0xe),*(undefined4 *)((int)piVar4 + 0x12),uVar2,0);
    FUN_0049a840(DAT_005412c0);
    uVar2 = FUN_004e49c0();
    FUN_004ac220(piVar4,uVar2);
    if ((DAT_00540bc4 != 0) && ((short)piVar4[3] == 0)) {
      uVar2 = *(undefined4 *)((int)piVar4 + 0x12);
      uVar1 = *(undefined4 *)((int)piVar4 + 0xe);
      (**(code **)(*piVar4 + 0x10))(0x1000);
      iVar3 = FUN_0046f6e0(&DAT_00568b60,uVar1,uVar2);
      if (iVar3 == 0) {
        piVar4 = (int *)FUN_004af530(DAT_005411a8,0);
        (**(code **)(*piVar4 + 0x90))(uVar1,uVar2,0);
      }
    }
  }
  return;
}


// ==== FUN_00426900 @ 00426900

void __thiscall FUN_00426900(void *this,int *param_2,undefined4 param_3,undefined4 param_4)

{
  byte bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  FUN_0041d7a0(&param_3);
  local_44 = param_4;
  local_48 = param_3;
  local_40[0] = &PTR_FUN_00502318;
  local_8 = 0;
  local_4 = 0;
  FUN_0041d770(&local_48);
  uVar2 = FUN_004e49c0();
  uVar3 = FUN_004e49c0();
  FUN_004b16d0(local_40,uVar3,uVar2,uVar3,uVar2);
  do {
    if (local_c == 0) {
      *param_2 = 0;
      param_2[1] = 0;
      return;
    }
    uVar4 = FUN_004ac200(local_c * 0x32 + DAT_005c8464);
    if ((uVar4 & 0x20a000) != 0) {
      if ((uVar4 & 0x200000) == 0) {
LAB_00426a53:
        param_2[1] = uVar4 >> 0xd & 1;
        *param_2 = local_c;
        return;
      }
      iVar5 = FUN_00492090((local_c * 0x32) / 0x32);
      if ((*(char *)((int)this + 0x22) != *(char *)(local_c * 0x32 + 0x22 + DAT_005c8464)) &&
         (uVar6 = FUN_004ac200(this), (uVar6 & 0x200000) == 0)) {
        if (DAT_005453e4 != 0) {
          bVar1 = *(byte *)(local_c * 0x32 + 0x22 + DAT_005c8464);
          if ((*(byte *)((int)this + 0x22) != bVar1) &&
             (*(int *)(&DAT_00595200 + ((uint)*(byte *)((int)this + 0x22) * 9 + (uint)bVar1) * 4) !=
              0)) goto LAB_00426a53;
        }
        if (iVar5 != 0) goto LAB_00426a53;
      }
    }
    FUN_004b1810(local_40);
  } while( true );
}


// ==== FUN_004274b0 @ 004274b0

undefined4 __thiscall FUN_004274b0(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  
  iVar3 = param_2 * 0x32 + DAT_005c8464;
  if (((DAT_005e4794 != 0) && (iVar1 = FUN_004ac550(iVar3), (*(uint *)(iVar1 + 0xec) & 0x200) == 0))
     && (iVar1 = FUN_0049adc0(iVar1), iVar1 != DAT_00541250)) {
    FUN_004e0620("canProcessStormpower( sp.getTypeStruct() )","\\Ns\\O\\Carrier.cpp",0x599);
  }
  if (((*(char *)(iVar3 + 0x22) == *(char *)((int)this + 0x22)) &&
      ((*(byte *)(iVar3 + 0x28) & 9) == 0)) && ((*(byte *)(iVar3 + 0xb) & 2) == 0)) {
    uVar2 = FUN_004ac200(this);
    if ((uVar2 & 0x20000) == 0) {
      iVar1 = (int)this - DAT_005c8464;
      iVar3 = FUN_00462c80((iVar3 - DAT_005c8464) / 0x32);
      iVar1 = FUN_00462c80(iVar1 / 0x32);
      if (iVar3 != iVar1) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


// ==== FUN_00427580 @ 00427580

undefined4 __thiscall FUN_00427580(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((*(byte *)(iVar1 + 0x28) & 9) == 0) && ((*(byte *)(iVar1 + 0xb) & 2) == 0)) &&
     (*(byte *)(iVar1 + 10) == DAT_00541240)) {
    uVar2 = FUN_004ac200(this);
    if ((uVar2 & 0x20000) == 0) {
      iVar1 = FUN_00462c80(((int)this - DAT_005c8464) / 0x32);
      iVar3 = FUN_00462c80(param_2);
      if (iVar3 != iVar1) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


// ==== FUN_00427600 @ 00427600

undefined4 __thiscall FUN_00427600(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  int iVar6;
  undefined1 *puVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined1 local_18 [8];
  undefined1 local_10 [16];
  
  iVar6 = param_2 * 0x32 + DAT_005c8464;
  iVar1 = FUN_0040f100();
  if ((iVar1 == 0) && ((*(byte *)(iVar6 + 0xb) & 1) == 0)) {
    uVar2 = FUN_004ac200(iVar6);
    if ((uVar2 & 0x10000000) != 0) {
      iVar1 = (int)this - DAT_005c8464;
      uVar5 = *(undefined4 *)((int)this + 0x12);
      uVar8 = *(undefined4 *)((int)this + 0xe);
      uVar9 = 1;
      puVar7 = local_18;
      uVar3 = FUN_004ade70(iVar6,local_10,0);
      puVar4 = (undefined4 *)FUN_0041d530(uVar3,puVar7,uVar8,uVar5,uVar9);
      uVar5 = FUN_00489080(iVar1 / 0x32,*puVar4,puVar4[1],0,0,0,0,0);
      return uVar5;
    }
  }
  return 0;
}


// ==== FUN_004276a0 @ 004276a0

undefined4 __thiscall FUN_004276a0(void *this,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  undefined1 *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined1 local_18 [8];
  undefined1 local_10 [16];
  
  iVar1 = FUN_0040f100();
  if (iVar1 != 0) {
    return 0;
  }
  iVar1 = (int)this - DAT_005c8464;
  uVar4 = *(undefined4 *)((int)this + 0x12);
  uVar7 = 1;
  uVar6 = *(undefined4 *)((int)this + 0xe);
  puVar5 = local_18;
  uVar2 = FUN_004ade70(param_2 * 0x32 + DAT_005c8464,local_10,0);
  puVar3 = (undefined4 *)FUN_0041d530(uVar2,puVar5,uVar6,uVar4,uVar7);
  uVar4 = FUN_00489080(iVar1 / 0x32,*puVar3,puVar3[1],0,0,0,0,0);
  return uVar4;
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


// ==== FUN_00428e00 @ 00428e00

void __thiscall FUN_00428e00(void *this)

{
  int iVar1;
  int local_c;
  short local_8;
  undefined2 local_6;
  undefined1 local_4;
  
  FUN_00426900(this,&local_c,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
  if ((local_c != 0) && (iVar1 = local_c * 0x32 + DAT_005c8464, DAT_00540bc4 != 0)) {
    if (DAT_00594fa4 == 0) {
      FUN_004ac200(iVar1);
    }
    if (DAT_00540bc4 != 0) {
      FUN_0040a9a0(&local_c);
      local_4 = *(undefined1 *)(iVar1 + 10);
      iVar1 = (int)this - DAT_005c8464;
      local_8 = ((short)(iVar1 / 0x32) + (short)(iVar1 >> 0x1f)) -
                (short)((longlong)iVar1 * 0x51eb851f >> 0x3f);
      local_6 = (undefined2)local_c;
      FUN_004e2dc0(9,&local_c);
    }
  }
  return;
}


// ==== FUN_00428ea0 @ 00428ea0

undefined4 __thiscall FUN_00428ea0(void *this)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int local_1c;
  int *local_18;
  undefined4 local_14;
  int local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f96c8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  piVar2 = operator_new(2000);
  iVar6 = 0;
  local_14 = 500;
  local_10 = 0;
  iVar4 = *(int *)((uint)*(byte *)((int)this + 0x22) * 0xb4 + 0x7c + DAT_0059534c);
  local_4 = 0;
  local_18 = piVar2;
  if (iVar4 != 0) {
    iVar3 = FUN_004274b0(this,iVar4);
    if (iVar3 != 0) {
      iVar6 = 1;
      *piVar2 = iVar4;
      local_10 = 1;
    }
  }
  iVar3 = 0;
  local_1c = 0;
  iVar4 = DAT_0055a714;
  if (0 < DAT_0055a714) {
    do {
      if (DAT_0055a710 <= iVar3) break;
      if ((iVar4 != 0) && (iVar1 = *(int *)(DAT_0055a70c + (iVar3 % iVar4) * 4), iVar1 != 0)) {
        iVar4 = FUN_004274b0(this,iVar1);
        if ((iVar4 != 0) && (iVar6 < 500)) {
          local_18[iVar6] = iVar1;
          iVar6 = iVar6 + 1;
          local_10 = iVar6;
        }
        local_1c = local_1c + 1;
        iVar4 = DAT_0055a714;
      }
      iVar3 = iVar3 + 1;
    } while (local_1c < iVar4);
  }
  iVar4 = ((int)this - DAT_005c8464) / 0x32;
  uVar5 = FUN_004280a0(&local_18,iVar4,iVar4);
  local_4 = 0xffffffff;
  _free(local_18);
  ExceptionList = local_c;
  return uVar5;
}


// ==== FUN_00429010 @ 00429010

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00429010(void *this,float param_2,int param_3)

{
  bool bVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  double *pdVar7;
  uint uVar8;
  undefined4 *puVar9;
  float10 fVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  float fStack_18720;
  float fStack_1871c;
  float fStack_18718;
  float fStack_18714;
  float fStack_18710;
  float fStack_1870c;
  float fStack_18708;
  float fStack_18704;
  float fStack_18700;
  int iStack_186fc;
  undefined1 auStack_186f8 [8];
  undefined1 auStack_186f0 [52];
  int iStack_186bc;
  int aiStack_186b0 [10002];
  undefined4 local_ea68 [14999];
  undefined4 uStack_c;
  
  uStack_c = 0x429020;
  iStack_186fc = FUN_00427d30();
  iStack_186fc = iStack_186fc + 1;
  if ((3 < iStack_186fc) ||
     ((param_2 == _DAT_005012d0 && (uVar2 = FUN_004ac200(this), (uVar2 & 0x20000) != 0)))) {
    return 1000;
  }
  fStack_18704 = *(float *)((int)this + 0xe);
  fStack_18700 = *(float *)((int)this + 0x12);
  uVar2 = 0;
  fStack_1871c = 0.0;
  bVar1 = false;
  if (param_3 == 0x1da1a) {
    uVar2 = 10;
    bVar1 = true;
    fStack_1871c = 1.4013e-44;
LAB_004290a0:
    iVar3 = FUN_004558f0(0,5000);
    uVar8 = iVar3 / 1000;
    if (3 < uVar2) goto LAB_00429196;
  }
  else {
    if (param_3 == -1) goto LAB_004290a0;
    uVar8 = param_3 / 2;
  }
  do {
    fStack_18714 = (float)(int)(&DAT_0052f83c)[uVar8 * 2] * param_2 + fStack_18704;
    fStack_18710 = (float)(int)(&DAT_0052f85c)[uVar8 * 2] * param_2 + fStack_18700;
    fStack_1870c = fStack_18714;
    fStack_18708 = fStack_18710;
    iVar3 = FUN_004e49c0();
    iVar4 = FUN_004e49c0();
    if (((*(byte *)(iVar4 + iVar3 * 0x100 + DAT_005c7c44) & 0x10) == 0) &&
       (iVar3 = FUN_00489080(((int)this - DAT_005c8464) / 0x32,fStack_18714,fStack_18710,0,0,0,0,0),
       iVar3 != 0)) {
      *(int *)(iVar3 + 0x186d8) = iStack_186fc;
      return 0x3e9;
    }
    fStack_1871c = (float)((int)fStack_1871c + 1);
    uVar8 = uVar8 + 1 & 0x80000003;
    if ((int)uVar8 < 0) {
      uVar8 = (uVar8 - 1 | 0xfffffffc) + 1;
    }
  } while ((int)fStack_1871c < 4);
LAB_00429196:
  if (bVar1) {
    FUN_00422230(auStack_186f0,fStack_18704,fStack_18700,5,5,0);
    fStack_18718 = -10000.0;
    fStack_1871c = -10000.0;
    fStack_18720 = 10000.0;
    while (iStack_186bc != 0) {
      uVar2 = FUN_004ac1e0(iStack_186bc * 0x32 + DAT_005c8464);
      if ((uVar2 & 0x800) != 0) {
        fStack_1870c = *(float *)(iStack_186bc * 0x32 + 0xe + DAT_005c8464);
        iVar3 = iStack_186bc * 0x32 + DAT_005c8464;
        fStack_18708 = *(float *)(iVar3 + 0x12);
        iVar4 = FUN_004e49c0();
        iVar5 = FUN_004e49c0();
        if (((*(byte *)(iVar5 + iVar4 * 0x100 + DAT_005c7c44) & 0x10) == 0) &&
           (iVar3 = FUN_004b2070(*(undefined4 *)(iVar3 + 0xe),*(undefined4 *)(iVar3 + 0x12),0x10000)
           , iVar3 == 0)) {
          puVar9 = local_ea68;
          for (iVar4 = 10000; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar9 = 0;
            puVar9 = puVar9 + 1;
          }
          uVar11 = *(undefined4 *)(iStack_186bc * 0x32 + 0xe + DAT_005c8464);
          uVar12 = *(undefined4 *)(iStack_186bc * 0x32 + 0x12 + DAT_005c8464);
          aiStack_186b0[0] = iVar3;
          uVar6 = FUN_00488fb0(*(undefined1 *)((int)this + 10),((int)this - DAT_005c8464) / 0x32);
          FUN_00488af0(aiStack_186b0,fStack_18704,fStack_18700,uVar11,uVar12,uVar6);
          if (aiStack_186b0[0] != 0) {
            uVar12 = 0;
            iVar3 = FUN_004ac550(this,0);
            uVar11 = *(undefined4 *)(iVar3 + 0xe4);
            iVar3 = FUN_004ac550(this,uVar11);
            FUN_00488e20(aiStack_186b0,DAT_0055b4d0,DAT_0055b4d4,*(undefined4 *)(iVar3 + 0xe0),
                         uVar11,uVar12);
            pdVar7 = (double *)FUN_00425e20(aiStack_186b0,auStack_186f8,aiStack_186b0[0] + -1);
            fVar10 = (float10)*pdVar7 - (float10)(double)CONCAT44(DAT_0055b4d4,DAT_0055b4d0);
            if (fVar10 < (float10)fStack_18720) {
              fStack_18720 = (float)fVar10;
              fStack_18718 = *(float *)(iStack_186bc * 0x32 + 0x12 + DAT_005c8464);
              fStack_1871c = *(float *)(iStack_186bc * 0x32 + DAT_005c8464 + 0xe);
              fStack_18714 = fStack_1871c;
              fStack_18710 = fStack_18718;
            }
          }
        }
      }
      FUN_004b1810(auStack_186f0);
    }
    iVar3 = FUN_0040e800(&fStack_1871c);
    if ((iVar3 != 0) &&
       (iVar3 = FUN_00489080(((int)this - DAT_005c8464) / 0x32,fStack_1871c,fStack_18718,0,0,0,0,0),
       iVar3 != 0)) {
      *(int *)(iVar3 + 0x186d8) = iStack_186fc;
    }
  }
  return 0x3e9;
}


// ==== FUN_00429420 @ 00429420

undefined4 __thiscall FUN_00429420(void *this)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  void *local_18;
  undefined4 local_14;
  int local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f96c8;
  local_c = ExceptionList;
  iVar7 = 0;
  uVar8 = 0;
  ExceptionList = &local_c;
  iVar2 = FUN_00428ea0();
  uVar3 = iVar2 * 0x32 + DAT_005c8464;
  if ((DAT_005c8464 + 0x32U <= uVar3) && (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) {
    FUN_00462bc0(((int)this - DAT_005c8464) / 0x32);
    local_18 = operator_new(2000);
    iVar6 = 0;
    local_14 = 500;
    local_10 = 0;
    local_4 = 0;
    iVar5 = DAT_005950e4;
    if (0 < DAT_005950e4) {
      do {
        if ((((iVar5 != 0) && (iVar1 = *(int *)(DAT_005950dc + (iVar7 % iVar5) * 4), iVar1 != 0)) &&
            (iVar4 = FUN_00427580(this,iVar1), iVar5 = DAT_005950e4, iVar4 != 0)) && (iVar6 < 500))
        {
          *(int *)((int)local_18 + iVar6 * 4) = iVar1;
          iVar6 = iVar6 + 1;
          iVar5 = DAT_005950e4;
          local_10 = iVar6;
        }
        iVar7 = iVar7 + 1;
      } while (iVar7 < iVar5);
    }
    uVar8 = FUN_004280a0(&local_18,iVar2,((int)this - DAT_005c8464) / 0x32);
    local_4 = 0xffffffff;
    _free(local_18);
  }
  ExceptionList = local_c;
  return uVar8;
}


// ==== FUN_00429ed0 @ 00429ed0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00429ed0(void *this)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  undefined1 local_4c [4];
  short local_48;
  undefined2 local_46;
  short local_44;
  undefined1 local_40 [52];
  int local_c;
  
  if (DAT_00540bc4 != 0) {
    iVar1 = FUN_004ac9f0(this,0x20,0,0);
    if ((iVar1 == 0) && (DAT_005c85a4 == 0)) {
      FUN_004202f0(local_40,*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),0);
      iVar1 = (int)this - DAT_005c8464;
      while ((uVar2 = local_c * 0x32 + DAT_005c8464, DAT_005c8464 + 0x32U <= uVar2 &&
             (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))) {
        uVar2 = FUN_004ac200();
        if ((((uVar2 & 0x210000) != 0) &&
            (iVar5 = DAT_005c8464 + local_c * 0x32,
            *(int *)(&DAT_00595200 +
                    ((uint)*(byte *)(iVar5 + 0x22) * 9 + (uint)*(byte *)((int)this + 0x22)) * 4) ==
            0)) && (*(byte *)(iVar5 + 10) != DAT_005412c8)) {
          iVar5 = FUN_004ac9f0(iVar5,0x20,0,0);
          iVar3 = iVar5 * 0x32 + DAT_005c8464;
          if ((iVar5 != 0) &&
             ((*(ushort *)(iVar3 + 0x12) != DAT_005412d0 ||
              ((*(byte *)(iVar3 + 0xc) & 0x7f) != *(byte *)((int)this + 0x22))))) {
            FUN_0040acf0(local_4c);
            iVar3 = local_c * 0x32;
            local_44 = ((short)(iVar3 / 0x32) + (short)(iVar3 >> 0x1f)) -
                       (short)((longlong)iVar3 * 0x51eb851f >> 0x3f);
            local_46 = (undefined2)iVar5;
            local_48 = ((short)(iVar1 / 0x32) + (short)(iVar1 >> 0x1f)) -
                       (short)((longlong)iVar1 * 0x51eb851f >> 0x3f);
            FUN_004267c0(_DAT_00540c70,0,local_4c);
            FUN_004e2e30(local_4c);
            iVar1 = ((local_c * 0x32) / 0x32) * 0x32 + DAT_005c8464;
            iVar3 = iVar1 - DAT_005c8464;
            uVar4 = FUN_00488fe0(*(undefined1 *)(iVar1 + 10),0,1);
            FUN_004b1a90(iVar3 / 0x32,uVar4);
            iVar1 = FUN_0049a840(*(undefined2 *)(iVar5 * 0x32 + 0x12 + DAT_005c8464));
            if ((char)((uint)*(undefined4 *)(iVar1 + 0xec) >> 8) < '\0') {
              FUN_00428fd0(this);
            }
            return 0xc;
          }
        }
        FUN_004b1810(local_40);
      }
    }
  }
  return 10;
}


// ==== FUN_0042a0d0 @ 0042a0d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __fastcall FUN_0042a0d0(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  undefined1 local_4c [4];
  short local_48;
  undefined2 local_46;
  short local_44;
  undefined1 local_40 [52];
  int local_c;
  
  if (DAT_00540bc4 != 0) {
    iVar2 = FUN_004ac9f0(param_1,0x20,0,0);
    if ((iVar2 == 0) && (DAT_005c85a4 == 0)) {
      FUN_004202f0(local_40,*(undefined4 *)(param_1 + 0xe),*(undefined4 *)(param_1 + 0x12),0);
      iVar2 = param_1 - DAT_005c8464;
      if (0 < DAT_00545424) {
        DAT_00545424 = DAT_00545424 + -1;
      }
      while (((uVar1 = local_c * 0x32 + DAT_005c8464, DAT_005c8464 + 0x32U <= uVar1 &&
              (uVar1 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) && (DAT_00545424 == 0))) {
        if ((*(char *)(uVar1 + 0x22) == *(char *)(param_1 + 0x22)) &&
           (*(byte *)(uVar1 + 10) == DAT_00541344)) {
          iVar3 = FUN_004ac9f0(uVar1,0x20,0,0);
          if ((iVar3 != 0) && (*(ushort *)(iVar3 * 0x32 + 0x12 + DAT_005c8464) == DAT_005412c0)) {
            _DAT_00545420 = (local_c * 0x32) / 0x32;
            DAT_00545424 = 5;
            FUN_0040acf0(local_4c);
            iVar5 = local_c * 0x32;
            local_44 = ((short)(iVar5 / 0x32) + (short)(iVar5 >> 0x1f)) -
                       (short)((longlong)iVar5 * 0x51eb851f >> 0x3f);
            local_46 = (undefined2)iVar3;
            local_48 = ((short)(iVar2 / 0x32) + (short)(iVar2 >> 0x1f)) -
                       (short)((longlong)iVar2 * 0x51eb851f >> 0x3f);
            FUN_00426830(_DAT_00540c70,0,local_4c);
            FUN_004e2e30(local_4c);
            iVar5 = ((local_c * 0x32) / 0x32) * 0x32 + DAT_005c8464;
            iVar2 = iVar5 - DAT_005c8464;
            uVar4 = FUN_00488fe0(*(undefined1 *)(iVar5 + 10),0,1);
            FUN_004b1a90(iVar2 / 0x32,uVar4);
            iVar2 = FUN_00429420(iVar5);
            if (iVar2 != 0) {
              FUN_00427600(iVar5,iVar2);
            }
            iVar2 = FUN_0049a840(*(undefined2 *)(iVar3 * 0x32 + 0x12 + DAT_005c8464));
            if ((char)((uint)*(undefined4 *)(iVar2 + 0xec) >> 8) < '\0') {
              FUN_00428fd0(param_1);
            }
            return 0xc;
          }
        }
        FUN_004b1810(local_40);
      }
    }
  }
  return 10;
}


// ==== FUN_00448740 @ 00448740

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00448740(void *this,float param_2,float param_3)

{
  if (param_2 == *(float *)((int)this + 0xe) - _DAT_00501908) {
    if (param_3 == *(float *)((int)this + 0x12) - _DAT_00501908) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_00462c80 @ 00462c80

int __cdecl FUN_00462c80(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_00462bc0(param_1);
  if (iVar1 == 0xfe) {
    return 0;
  }
  return DAT_00562c18 + iVar1 * 6;
}


// ==== FUN_0046f950 @ 0046f950

undefined4 __thiscall FUN_0046f950(void *this)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((((-1 < iVar2) && (-1 < iVar1)) && (iVar2 < 0x100)) &&
     ((iVar1 < 0x100 &&
      (uVar4 = (uint)*(ushort *)(DAT_005c84bc + (iVar1 * 0x100 + iVar2) * 2), uVar4 != 0)))) {
    iVar1 = FUN_004acda0(uVar4 * 0x32 + DAT_005c8464);
    if (iVar1 != 0x7f) {
      uVar3 = FUN_0046f260(this,iVar1);
      return uVar3;
    }
  }
  return 0;
}


// ==== FUN_00491db0 @ 00491db0

void __thiscall FUN_00491db0(void *this)

{
  float fVar1;
  float fVar2;
  bool bVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  
  iVar4 = FUN_0046f950(&DAT_00568b60,*(undefined4 *)((int)this + 0xe),
                       *(undefined4 *)((int)this + 0x12));
  if (((iVar4 != 0) &&
      (iVar6 = *(int *)((int)this + 0x1a), iVar5 = FUN_004adcb0(this), iVar6 < iVar5)) &&
     (*(char *)(iVar4 * 0x32 + 0x22 + DAT_005c8464) == *(char *)((int)this + 0x22))) {
    iVar6 = FUN_004ac550(this);
    iVar5 = FUN_004e49c0();
    *(float *)(iVar6 + 0xe0) = *(float *)(iVar6 + 0xe0) + *(float *)(iVar6 + 0xe0);
    iVar4 = FUN_004894e0(((int)this - DAT_005c8464) / 0x32,iVar4,0,0);
    *(float *)(iVar6 + 0xe0) = (float)iVar5;
    if (iVar4 != 0) {
      return;
    }
  }
  bVar3 = false;
  iVar4 = FUN_0046f6e0(&DAT_00568b60,*(undefined4 *)((int)this + 0xe),
                       *(undefined4 *)((int)this + 0x12));
  do {
    fVar1 = *(float *)((int)this + 0xe);
    fVar2 = *(float *)((int)this + 0x12);
    fVar7 = (float10)FUN_00455930(0xc1a00000,0x41a00000);
    fVar1 = (float)(fVar7 + (float10)fVar1);
    fVar7 = (float10)FUN_00455930(0xc1a00000,0x41a00000);
    fVar2 = (float)(fVar7 + (float10)fVar2);
    if (iVar4 == 0) {
LAB_00491f13:
      iVar6 = FUN_00489080(((int)this - DAT_005c8464) / 0x32,fVar1,fVar2,0,0,0,0,0);
      if (iVar6 != 0) {
        return;
      }
      bVar3 = true;
    }
    else {
      iVar6 = FUN_0046f6e0(&DAT_00568b60,fVar1,fVar2);
      if (iVar6 != 0) {
        iVar6 = FUN_0046f530(fVar1,fVar2);
        iVar5 = FUN_0046f530(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12));
        if (iVar6 == iVar5) goto LAB_00491f13;
      }
    }
    if (bVar3) {
      return;
    }
  } while( true );
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __thiscall FUN_004ac1e0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004ac990 @ 004ac990

undefined4 __cdecl FUN_004ac990(void)

{
  return 10;
}


// ==== FUN_004ac9f0 @ 004ac9f0

int __thiscall FUN_004ac9f0(void *this,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_00500ff4;
  local_4 = 0;
  FUN_004b2270(local_10,((int)this - DAT_005c8464) / 0x32);
  do {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(DAT_005c8464 + 0x12 + local_8 * 0x32) != param_3)) {
      bVar1 = false;
    }
    if (((param_2 == 0) || ((param_2 & *(ushort *)(DAT_005c8464 + 0x14 + local_8 * 0x32)) != 0)) &&
       (bVar1)) {
      if (param_4 == 0) {
        return (local_8 * 0x32) / 0x32;
      }
      param_4 = param_4 + -1;
    }
    FUN_004b1b20(local_10);
  } while( true );
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


// ==== FUN_004afa70 @ 004afa70

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall
FUN_004afa70(void *this,undefined2 param_2,undefined2 param_3,int param_4,uint param_5,
            undefined1 param_6)

{
  undefined4 uVar1;
  int *piVar2;
  
  if (param_4 == 0) {
    param_4 = 0x20;
  }
  uVar1 = 0;
  if ((param_5 & 0x20) == 0) {
    if ((param_5 & 0x10) != 0) {
      uVar1 = 2;
    }
  }
  else {
    uVar1 = 1;
  }
  piVar2 = (int *)FUN_004af530(DAT_00541088,uVar1);
  *(undefined2 *)((int)piVar2 + 0x12) = param_2;
  *(undefined2 *)(piVar2 + 3) = param_3;
  if (*(byte *)((int)piVar2 + 10) == _DAT_005411a0) {
    FUN_004214a0(((int)piVar2 - DAT_005c8464) / 0x32);
  }
  *(undefined1 *)((int)piVar2 + 0x1e) = param_6;
  *(short *)(piVar2 + 5) = (short)param_4;
  (**(code **)(*piVar2 + 0x50))(((int)this - DAT_005c8464) / 0x32,param_5);
  return ((int)piVar2 - DAT_005c8464) / 0x32;
}


// ==== FUN_004afd00 @ 004afd00

undefined4 __thiscall FUN_004afd00(void *this,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  if (*(byte *)((int)this + 0x22) < 10) {
    iVar1 = FUN_004ac040(this,param_2);
    if (iVar1 != 0) {
      uVar2 = FUN_004ad730(this);
      return uVar2;
    }
  }
  return 0;
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


// ==== FUN_004b2070 @ 004b2070

int __cdecl FUN_004b2070(undefined4 param_1,undefined4 param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  local_48 = param_1;
  local_44 = param_2;
  local_40[0] = &PTR_FUN_00502318;
  local_8 = (uint)((param_3 & 6) == 0);
  local_4 = 0;
  FUN_0041d770(&local_48);
  uVar1 = FUN_004e49c0();
  uVar2 = FUN_004e49c0();
  FUN_004b16d0(local_40,uVar2,uVar1,uVar2,uVar1);
  while( true ) {
    uVar3 = local_c * 0x32 + DAT_005c8464;
    if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3))
    break;
    uVar3 = FUN_004ac200();
    if ((param_3 & uVar3) != 0) {
      return (local_c * 0x32) / 0x32;
    }
    FUN_004b1810(local_40);
  }
  return 0;
}


// ==== FUN_004b21d0 @ 004b21d0

int __cdecl FUN_004b21d0(undefined4 param_1,undefined4 param_2,undefined4 param_3,uint param_4)

{
  int iVar1;
  uint uVar2;
  undefined1 local_40 [52];
  int local_c;
  
  iVar1 = FUN_0049a840(param_4);
  FUN_00422230(local_40,param_1,param_2,param_3,param_3,(*(byte *)(iVar1 + 0xec) & 6) == 0);
  while( true ) {
    uVar2 = local_c * 0x32 + DAT_005c8464;
    if ((uVar2 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar2))
    break;
    if (*(byte *)(uVar2 + 10) == param_4) {
      return (local_c * 0x32) / 0x32;
    }
    FUN_004b1810(local_40);
  }
  return 0;
}


// ==== FUN_004e0620 @ 004e0620

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void __cdecl FUN_004e0620(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  code *pcVar1;
  char *pcVar2;
  FILE *_File;
  int iVar3;
  undefined4 *puVar4;
  char *pcVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 auStack_2058 [20];
  char local_2008;
  undefined4 local_2007;
  char local_1808 [1024];
  char local_1408 [1024];
  char local_1008 [4092];
  undefined4 uStack_c;
  
  puVar4 = (undefined4 *)&stack0xfffffffc;
  uStack_c = 0x4e0630;
  if (DAT_005e479c == 0) {
    DAT_005e479c = 1;
    _fprintf((FILE *)&DAT_00543790,"ZAssertion failed (%s) at %s line %d\n");
    _File = _fopen("assert.html","w+b");
    _setvbuf(_File,(char *)0x0,4,0);
    _fprintf(_File,"<html>\n<title>Netstorm Crash</title>\n<body>\n<h2>Crash Results</h2>\n");
    _fprintf(_File,"ZAssertion failed (%s) at %s line %d\n",param_1,param_2,param_3);
    iVar3 = FUN_00440c50();
    if (iVar3 != 0) {
      local_1008[0] = '\0';
      FUN_00441300();
      _fprintf(_File,local_1008);
    }
    _fprintf(_File,"</body></html>\n");
    _fclose(_File);
    FUN_004c2610();
    local_2008 = '\0';
    puVar7 = &local_2007;
    for (iVar3 = 0x1ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    *(undefined2 *)puVar7 = 0;
    *(undefined1 *)((int)puVar7 + 2) = 0;
    iVar3 = 0;
    do {
      puVar7 = puVar4 + 1;
      puVar4 = (undefined4 *)*puVar4;
      auStack_2058[iVar3] = *puVar7;
      iVar3 = iVar3 + 1;
    } while (iVar3 < 0x14);
    iVar6 = 0;
    if (0 < iVar3) {
      do {
        pcVar2 = &local_2008;
        do {
          pcVar5 = pcVar2;
          pcVar2 = pcVar5 + 1;
        } while (*pcVar5 != '\0');
        _sprintf(pcVar5,"%d.%08X ");
        if (((byte)iVar6 & 3) == 3) {
          pcVar2 = &local_2008;
          do {
            pcVar5 = pcVar2;
            pcVar2 = pcVar5 + 1;
          } while (*pcVar5 != '\0');
          _sprintf(pcVar5,"\r\n");
        }
        iVar6 = iVar6 + 1;
      } while (iVar6 < iVar3);
    }
    if (DAT_005c8450 != 0) {
      pcVar2 = &local_2008;
      do {
        pcVar5 = pcVar2;
        pcVar2 = pcVar5 + 1;
      } while (*pcVar5 != '\0');
      _sprintf(pcVar5,"lastRobust=%X sidType=%d\r\n");
    }
    pcVar2 = &local_2008;
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    _sprintf(pcVar5,"\r\nEBP = %08X\r\n");
    iVar3 = 0;
    do {
      pcVar2 = &local_2008;
      do {
        pcVar5 = pcVar2;
        pcVar2 = pcVar5 + 1;
      } while (*pcVar5 != '\0');
      _sprintf(pcVar5,"%08X ");
      if (((byte)iVar3 & 7) == 7) {
        pcVar2 = &local_2008;
        do {
          pcVar5 = pcVar2;
          pcVar2 = pcVar5 + 1;
        } while (*pcVar5 != '\0');
        _sprintf(pcVar5,"\r\n");
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < 0x28);
    _sprintf(local_1408,
             "An assertion has failed:\n\'%s\' in file %s, line %d version %d.%d.\nExtra info: %s\nStack:\r\n%s\nPlease copy this information and mail it to bugs@titanic.com\nThank you"
             ,param_1,param_2,param_3);
    if (DAT_0054dbb4 != 0) {
      FUN_004a4fe0();
    }
    _sprintf(local_1808,"\'%s\' in file %s, line %d version %d.%d\r\nExtra info: %s\r\n%s",param_1,
             param_2,param_3);
    DialogBoxParamA(DAT_0054dbf8,(LPCSTR)0x69,DAT_0054dbf4,FUN_004e0520,(LPARAM)local_1808);
    DAT_005e479c = 0;
  }
  if (DAT_00543608 != 0) {
    if (DAT_00594fa0 != 0) {
      FUN_00460df0();
    }
    pcVar1 = (code *)swi(3);
    (*pcVar1)();
    return;
  }
  if (DAT_005e4798 != (void *)0x0) {
    _free(DAT_005e4798);
    DAT_005e4798 = (void *)0x0;
  }
  return;
}


// ==== FUN_004e2e30 @ 004e2e30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004e2e30(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = 1;
  iVar2 = 0xb4;
  do {
    if ((((*(byte *)(iVar2 + 0x50 + DAT_0059534c) & 1) != 0) && ((&DAT_005e4a18)[iVar1] != 0)) &&
       (iVar1 != _DAT_00540c70)) {
      (**(code **)(*(int *)(&DAT_005e4a18)[iVar1] + 4))(param_1);
    }
    iVar1 = iVar1 + 1;
    iVar2 = iVar2 + 0xb4;
  } while (iVar1 < 9);
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


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
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


// ==== FUN_004ad680 @ 004ad680

int __thiscall FUN_004ad680(void *this)

{
  int iVar1;
  int iVar2;
  
  if (*(byte *)((int)this + 10) < 0x46) {
    FUN_004e0620("!isForm()","\\Ns\\O\\Squid.cpp",0x854);
  }
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return (int)*(char *)(*(int *)(iVar2 + 0x124) + iVar1 * 4);
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


// ==== FUN_004ae2d0 @ 004ae2d0

void __thiscall FUN_004ae2d0(void *this)

{
  FUN_00471aa0(*(undefined4 *)((int)this + 0x12));
  return;
}


// ==== FUN_00488fe0 @ 00488fe0

undefined4 __cdecl FUN_00488fe0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(param_1);
  if ((param_1 != DAT_0054126c) && (param_1 != DAT_00541344)) {
    if ((param_1 == DAT_0054127c) || (param_1 == DAT_005412c8)) {
      return DAT_00541108;
    }
    if ((*(uint *)(iVar1 + 0xec) & 0x20000) != 0) {
      return DAT_00541118;
    }
    if (param_1 == DAT_00541270) {
      return DAT_005410cc;
    }
    if (param_1 == DAT_00541274) {
      return DAT_00541114;
    }
    if (param_1 == DAT_00541170) {
      return DAT_005410d0;
    }
    if (param_1 == DAT_005412d0) {
      return DAT_00541148;
    }
    if (param_1 == DAT_005411fc) {
      return DAT_0054110c;
    }
  }
  return DAT_00541110;
}


// ==== FUN_004b1a90 @ 004b1a90

uint __cdecl FUN_004b1a90(int param_1,uint param_2,int param_3,int param_4)

{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  int *piVar4;
  int iVar5;
  
  uVar1 = *(ushort *)(param_1 * 0x32 + 6 + DAT_005c8464);
  iVar5 = DAT_005c8464;
  do {
    uVar3 = (uint)uVar1;
    if (uVar3 == 0) {
      return 0;
    }
    piVar4 = (int *)(uVar3 * 0x32 + iVar5);
    if ((*(byte *)(uVar3 * 0x32 + 10 + iVar5) == param_2) &&
       ((*(byte *)((int)piVar4 + 0xb) & 2) == 0)) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return uVar3;
        }
        if ((DAT_00540bc0 != 0) ||
           ((iVar2 = ((int)piVar4 - iVar5) / 0x32, 4 < iVar2 && (iVar2 < 15000)))) {
          (**(code **)(*piVar4 + 0x10))(0);
          iVar5 = DAT_005c8464;
        }
      }
      param_3 = param_3 + -1;
    }
    uVar1 = *(ushort *)(piVar4 + 1);
  } while( true );
}


// ==== FUN_00426fc0 @ 00426fc0

undefined4 __thiscall FUN_00426fc0(void *this)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_004ac200();
  if ((uVar1 & 0x320000) == 0) {
    iVar2 = FUN_004e49c0();
    iVar3 = FUN_004e49c0();
    if ((*(byte *)(iVar3 + iVar2 * 0x100 + DAT_005c7c44) & 6) == 0) {
      iVar2 = (**(code **)(*(int *)this + 200))();
      if (iVar2 != 0) {
        return 1;
      }
    }
  }
  return 0;
}


