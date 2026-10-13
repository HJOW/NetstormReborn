// ==== FUN_0048aa00 @ 0048aa00

void __thiscall FUN_0048aa00(void *this)

{
  bool bVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  void *pvVar5;
  int iVar6;
  int iVar7;
  undefined1 auStack_4c [52];
  int iStack_18;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  pcStack_8 = FUN_004fb08b;
  pvStack_c = ExceptionList;
  piVar2 = (int *)(*(int *)((int)this + 8) * 0x32 + DAT_005c8464);
  if (((DAT_00540bc4 != 0) ||
      ((ExceptionList = &pvStack_c, 4 < *(int *)((int)this + 4) &&
       (ExceptionList = &pvStack_c, *(int *)((int)this + 4) < 15000)))) &&
     (ExceptionList = &pvStack_c, (*(byte *)((int)piVar2 + 0xb) & 2) == 0)) {
    ExceptionList = &pvStack_c;
    (**(code **)(*piVar2 + 0x48))(0);
    (**(code **)(*piVar2 + 0x4c))(0);
  }
  if ((((*(int *)((int)this + 0x186d4) != 0) &&
       (uVar3 = *(int *)((int)this + 0x186d4) * 0x32 + DAT_005c8464, DAT_005c8464 + 0x32U <= uVar3))
      && (uVar3 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464))) &&
     (uVar3 = FUN_004ac200(uVar3), (uVar3 & 0x404200) != 0)) {
    iVar4 = FUN_0041bfb0(this);
    FUN_00422230(auStack_4c,*(undefined4 *)(iVar4 + 0xe),*(undefined4 *)(iVar4 + 0x12),2,2,0);
    while (iStack_18 != 0) {
      if (iStack_18 == *(int *)((int)this + 0x186d4)) {
        pvVar5 = operator_new(0x28);
        uStack_4 = 0;
        if (pvVar5 != (void *)0x0) {
          FUN_00496f00(pvVar5,DAT_005326c4,*(undefined4 *)((int)this + 0x186d4),0x3f800000);
        }
        uStack_4 = 0xffffffff;
        *(undefined4 *)((int)this + 0x186d4) = 0;
        break;
      }
      FUN_004b1810(auStack_4c);
    }
  }
  if ((*(byte *)((int)piVar2 + 0xb) & 2) == 0) {
    iVar4 = FUN_004263a0(*(undefined4 *)((int)piVar2 + 0xe),*(undefined4 *)((int)piVar2 + 0x12),1);
    iVar6 = FUN_004ac9f0(piVar2,0x20,DAT_005412c0,0);
    if (((iVar4 != 0) && (iVar6 != 0)) &&
       (*(char *)(iVar4 * 0x32 + 0x22 + DAT_005c8464) == *(char *)((int)piVar2 + 0x22))) {
      iVar7 = FUN_00462c80(((int)piVar2 - DAT_005c8464) / 0x32);
      iVar4 = FUN_00462c80(iVar4);
      if ((iVar4 == iVar7) && (DAT_00540bc4 != 0)) {
        (**(code **)(*(int *)(iVar6 * 0x32 + DAT_005c8464) + 0x10))(0);
      }
    }
  }
  if (*(byte *)((int)piVar2 + 10) == DAT_005412d0) {
    iVar4 = FUN_0041bfb0(this);
    iVar4 = FUN_004b21d0(*(undefined4 *)(iVar4 + 0xe),*(undefined4 *)(iVar4 + 0x12),2,DAT_005412ec);
    if ((iVar4 != 0) && ((*(byte *)(iVar4 * 0x32 + 0x28 + DAT_005c8464) & 8) == 0)) {
      FUN_00447e40(iVar4 * 0x32 + DAT_005c8464,*(undefined4 *)((int)this + 8),0);
    }
  }
  if (((DAT_00540bc4 != 0) && (*(int *)((int)this + 0x186d4) != 0)) &&
     (piVar2 = (int *)(*(int *)((int)this + 0x186d4) * 0x32 + DAT_005c8464),
     (*(byte *)((int)piVar2 + 0xb) & 1) == 0)) {
    if (((*(byte *)(piVar2 + 10) & 1) == 0) ||
       (uVar3 = FUN_004ac200(piVar2), (uVar3 & 0x404200) == 0)) {
      bVar1 = false;
    }
    else {
      bVar1 = true;
    }
    if (DAT_005e4794 == 0) {
      if (!bVar1) {
        ExceptionList = pvStack_c;
        return;
      }
    }
    else if (!bVar1) {
      FUN_004e0620("shouldDestroy","\\Ns\\O\\Pathprocess.cpp",0x437);
      ExceptionList = pvStack_c;
      return;
    }
    FUN_00442a40(*(undefined1 *)((int)piVar2 + 10),*(undefined1 *)((int)piVar2 + 0x22));
    (**(code **)(*piVar2 + 0x10))(0);
  }
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_0041bfb0 @ 0041bfb0

int __thiscall FUN_0041bfb0(void *this)

{
  int iVar1;
  
  iVar1 = *(int *)((int)this + 8) * 0x32 + DAT_005c8464;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_004e0620("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4);
  }
  return iVar1;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_00422230 @ 00422230

void * __thiscall
FUN_00422230(void *this,undefined4 param_2,undefined4 param_3,int param_4,int param_5,uint param_6)

{
  int iVar1;
  int iVar2;
  
  *(uint *)((int)this + 0x38) = param_6 & 1;
  *(undefined ***)this = &PTR_FUN_00502318;
  *(uint *)((int)this + 0x3c) = param_6 & 4;
  FUN_0041d7a0(&param_2);
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  FUN_004b16d0(this,iVar2 - param_4,iVar1 - param_5,iVar2 + param_4,iVar1 + param_5);
  return this;
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


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
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


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_0049adc0 @ 0049adc0

int __thiscall FUN_0049adc0(void *this)

{
  return ((int)this - DAT_0059ab20) / 500;
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


// ==== FUN_004b2270 @ 004b2270

void __thiscall FUN_004b2270(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((DAT_005e4794 != 0) && ((*(byte *)(iVar1 + 0xb) & 2) != 0)) && (DAT_00540bc4 == 0)) &&
     (*(int *)((int)this + 0xc) == 0)) {
    FUN_004e0620("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",
                 "\\Ns\\O\\Squidfinder.cpp",0x13c);
  }
  *(uint *)((int)this + 4) = (uint)*(ushort *)(iVar1 + 6);
  FUN_004b1b20(this);
  return;
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


// ==== FUN_00462bc0 @ 00462bc0

undefined4 __cdecl FUN_00462bc0(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  if (DAT_005e4794 == 0) {
    if (param_1 == 0) {
      return 0;
    }
  }
  else if (param_1 == 0) {
    FUN_004e0620("mySid","\\Ns\\O\\Graph.cpp",0x93);
    return 0;
  }
  iVar1 = param_1 * 0x32 + DAT_005c8464;
  uVar2 = FUN_004ac1e0(iVar1);
  if ((uVar2 & 0x800) != 0) {
    uVar3 = FUN_004ad390(iVar1);
    return uVar3;
  }
  uVar3 = FUN_00462b80(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12));
  return uVar3;
}


// ==== FUN_00462b80 @ 00462b80

undefined4 __cdecl FUN_00462b80(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  if (DAT_00540414 != 0) {
    iVar1 = FUN_0040eaf0(param_1,param_2);
    if (iVar1 != 0) {
      uVar2 = FUN_004ad390(iVar1 * 0x32 + DAT_005c8464);
      return uVar2;
    }
  }
  return 0xfe;
}


// ==== FUN_0040eaf0 @ 0040eaf0

undefined2 __cdecl FUN_0040eaf0(void)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((((-1 < iVar1) && (-1 < iVar2)) && (iVar1 < 0x100)) && (iVar2 < 0x100)) {
    return *(undefined2 *)(DAT_00542514 + (iVar2 * 0x100 + iVar1) * 2);
  }
  return 0;
}


// ==== FUN_0041cfc0 @ 0041cfc0

float10 __thiscall FUN_0041cfc0(void *this,float param_2,float param_3)

{
  float10 fVar1;
  
  fVar1 = (float10)*(float *)((int)this + 4) - (float10)param_3;
  return SQRT(fVar1 * fVar1 +
              ((float10)*(float *)this - (float10)param_2) *
              ((float10)*(float *)this - (float10)param_2));
}


// ==== FUN_004ad390 @ 004ad390

undefined1 __thiscall FUN_004ad390(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (*(byte *)((int)this + 10) != DAT_005411c0)) &&
     (*(byte *)((int)this + 10) != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x739);
  }
  return *(undefined1 *)((int)this + 0x1e);
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __thiscall FUN_004ac1e0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
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


