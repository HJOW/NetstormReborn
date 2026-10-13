// ==== FUN_00451470 @ 00451470

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00451470(float param_1,float param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  float local_48;
  float local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_48 = param_1 - _DAT_005012d0;
  local_40[0] = &PTR_FUN_00502318;
  local_44 = param_2 - _DAT_005012d0;
  local_8 = 0;
  local_4 = 0;
  FUN_0041d7a0(&local_48);
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  FUN_004b16d0(local_40,iVar2 + -4,iVar1 + -4,iVar2 + 4,iVar1 + 4);
  while( true ) {
    uVar3 = local_c * 0x32 + DAT_005c8464;
    if ((uVar3 < DAT_005c8464 + 0x32U) || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar3))
    break;
    uVar3 = FUN_004ac200();
    if ((uVar3 & 0x400) != 0) {
      FUN_00450db0(local_c * 0x32 + DAT_005c8464);
    }
    FUN_004b1810(local_40);
  }
  return;
}


// ==== FUN_00450db0 @ 00450db0

void __thiscall FUN_00450db0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  if ((((*(byte *)((int)this + 0xb) & 2) == 0) && (*(short *)((int)this + 0xc) != 0x5107)) &&
     (*(short *)((int)this + 0xc) != 0x5105)) {
    FUN_00450670(this,0x5107,0);
    iVar1 = FUN_004b1a90(((int)this - DAT_005c8464) / 0x32,DAT_005410a8,0,0);
    if (iVar1 == 0) {
      FUN_00424030(((int)this - DAT_005c8464) / 0x32);
    }
    uVar3 = 1;
    uVar2 = FUN_0048fd30(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         &DAT_0059a9d8,*(undefined1 *)((int)this + 0x22),1);
    FUN_004508b0(this,uVar2,uVar3);
  }
  return;
}


// ==== FUN_00450670 @ 00450670

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00450670(void *this,int param_2,float param_3)

{
  int iVar1;
  void *pvVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fa15b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if ((*(short *)((int)this + 0xc) == 0x5105) && (ExceptionList = &local_c, param_2 != 0x5105)) {
    ExceptionList = &local_c;
    FUN_004e0620("task == taskFALL",".\\Dude.cpp",0x9c);
  }
  iVar1 = FUN_004afbe0(this,0xfff0,0x5100);
  if (iVar1 == 0) {
    pvVar2 = operator_new(0x28);
    local_4 = 0;
    if (pvVar2 == (void *)0x0) {
      iVar1 = 0;
    }
    else {
      iVar1 = FUN_00496e80(pvVar2,param_2,((int)this - DAT_005c8464) / 0x32,0x40a00000);
    }
    local_4 = 0xffffffff;
  }
  else {
    *(int *)(iVar1 + 0x18) = param_2;
    *(undefined4 *)(iVar1 + 0x20) = 0;
  }
  *(double *)(iVar1 + 0x10) = (double)(param_3 + (float)_DAT_0055b4d0);
  FUN_004ac220(this,param_2);
  ExceptionList = local_c;
  return;
}


// ==== FUN_004508b0 @ 004508b0

undefined4 __thiscall FUN_004508b0(void *this,int param_2,undefined4 param_3)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined1 local_8 [8];
  
  if (param_2 != 0) {
    if ((*(byte *)(param_2 * 0x32 + 0xb + DAT_005c8464) & 3) != 0) {
      FUN_004e0620("!s.isDead() && !s.isFree()",".\\Dude.cpp",0xde);
    }
    puVar1 = (undefined4 *)FUN_004505a0(local_8,param_2);
    uVar2 = FUN_00450760(this,*puVar1,puVar1[1],param_3);
    return uVar2;
  }
  return 0;
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


// ==== FUN_00424030 @ 00424030

undefined4 __cdecl FUN_00424030(undefined4 param_1)

{
  void *pvVar1;
  undefined4 uVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f962b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  pvVar1 = operator_new(0x90);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    uVar2 = FUN_00423b20(pvVar1,param_1,0);
    ExceptionList = local_c;
    return uVar2;
  }
  ExceptionList = local_c;
  return 0;
}


// ==== FUN_0048fd30 @ 0048fd30

undefined4 __cdecl FUN_0048fd30(undefined4 param_1,undefined4 param_2,int *param_3,uint param_4)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  float10 fVar6;
  float local_4;
  
  uVar2 = param_4;
  piVar1 = param_3;
  uVar4 = 0;
  iVar5 = 0;
  local_4 = 9999.0;
  if (0 < param_3[2]) {
    do {
      iVar3 = *(int *)(*piVar1 + iVar5 * 4) * 0x32 + DAT_005c8464;
      if ((*(byte *)(iVar3 + 0x22) == uVar2) &&
         (fVar6 = (float10)FUN_0041cfc0(&param_1,*(undefined4 *)(iVar3 + 0xe),
                                        *(undefined4 *)(iVar3 + 0x12)), fVar6 < (float10)local_4)) {
        local_4 = (float)fVar6;
        uVar4 = *(undefined4 *)(*piVar1 + iVar5 * 4);
      }
      iVar5 = iVar5 + 1;
    } while (iVar5 < piVar1[2]);
  }
  return uVar4;
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


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_0043d9b0 @ 0043d9b0

undefined4 FUN_0043d9b0(void)

{
  return 0;
}


