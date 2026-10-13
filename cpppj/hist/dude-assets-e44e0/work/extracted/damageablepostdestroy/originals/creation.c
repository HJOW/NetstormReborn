// ==== FUN_0044b9e0 @ 0044b9e0

void __thiscall FUN_0044b9e0(void *this,uint param_2)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  void *local_9c [2];
  undefined1 local_94 [52];
  int local_60;
  undefined1 local_54 [8];
  undefined1 local_4c [52];
  int local_18;
  void *pvStack_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f9ffe;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  if ((param_2 & 0x300000) != 0) {
    iVar6 = (int)this - DAT_005c8464;
    ExceptionList = &pvStack_c;
    uVar1 = FUN_004ac200(this,1);
    FUN_0044b830(iVar6 / 0x32,uVar1);
  }
  if ((((((*(byte *)((int)this + 0x28) & 9) == 0) &&
        (uVar2 = FUN_004ac1e0(this), (uVar2 & 0x400) != 0)) &&
       (iVar6 = FUN_004b2070(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                             0x1000000), iVar6 != 0)) &&
      (uVar2 = FUN_004ac200(this), (uVar2 & 0x10000000) == 0)) &&
     (((param_2 & 0x200000) != 0 || (*(byte *)((int)this + 10) != DAT_005412a4)))) {
    FUN_0046e6c0(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),param_2,
                 *(undefined1 *)((int)this + 0x22));
  }
  if (((DAT_00594fbc != 0) && (iVar6 = FUN_004adc60(this), iVar6 != 0)) &&
     ((*(byte *)((int)this + 0x28) & 9) == 0)) {
    iVar6 = FUN_004abb00(param_2);
    if (((param_2 & 0x200000) == 0) &&
       (((iVar3 = FUN_004ac950(this), iVar3 == 0 || (iVar3 = FUN_004ac950(this), iVar3 == 1)) ||
        (uVar2 = FUN_004ac200(this), (uVar2 & 0x200) != 0)))) {
      iVar3 = FUN_004adcb0(this);
      iVar3 = iVar3 / 2 + 1;
      uVar2 = FUN_004ac200(this);
      if ((uVar2 & 0x200) != 0) {
        iVar3 = 400;
      }
      if ((param_2 & 0x400000) == 0) {
        iVar4 = FUN_004ac550(this);
        iVar4 = *(int *)(iVar4 + 0x1d4) / 2 + 1;
        puVar5 = (undefined4 *)FUN_004adea0(this,local_9c);
        FUN_00422230(local_4c,*puVar5,puVar5[1],iVar4,iVar4,1);
        while (local_18 != 0) {
          iVar4 = FUN_004adc60(local_18 * 0x32 + DAT_005c8464);
          if (((iVar4 != 0) &&
              (iVar4 = local_18 * 0x32 + DAT_005c8464, (*(byte *)(iVar4 + 0xb) & 2) == 0)) &&
             ((*(byte *)(iVar4 + 0x28) & 1) == 0)) {
            local_9c[0] = operator_new(0x28);
            iVar4 = 0;
            local_4 = 0;
            if (local_9c[0] != (void *)0x0) {
              iVar4 = FUN_00496e80(local_9c[0],DAT_005326c0,(local_18 * 0x32) / 0x32,0);
            }
            local_4 = 0xffffffff;
            *(int *)(iVar4 + 0x20) = iVar6 * 0x10000 + iVar3;
          }
          FUN_004b1810(local_4c);
        }
      }
    }
    uVar2 = FUN_004ac200(this);
    if ((uVar2 & 0x130000) == 0) {
      puVar5 = (undefined4 *)FUN_004adea0(this,local_54);
      FUN_00422230(local_94,*puVar5,puVar5[1],2,2,0);
      while (local_60 != 0) {
        uVar2 = FUN_004ac200(local_60 * 0x32 + DAT_005c8464);
        if ((uVar2 & 4) != 0) {
          iVar6 = FUN_004224c0(local_60);
          if ((iVar6 == 0) || (iVar6 = FUN_00421fe0(local_60), iVar6 != 0)) {
            iVar6 = FUN_00422560(local_60);
            if (iVar6 == 0) {
              FUN_00421db0(local_60);
            }
          }
          else if (DAT_00540bc4 != 0) {
            (**(code **)(*(int *)(local_60 * 0x32 + DAT_005c8464) + 0x10))(0);
          }
        }
        FUN_004b1810(local_94);
      }
    }
  }
  FUN_004b0840(this,param_2);
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_0044b830 @ 0044b830

void __cdecl FUN_0044b830(int param_1,uint param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  if (((param_1 != 0) && (iVar1 = param_1 * 0x32 + DAT_005c8464, (*(byte *)(iVar1 + 0x28) & 1) == 0)
      ) && ((param_2 & 0x174200) != 0)) {
    uVar3 = 1;
    if ((param_2 & 0x4200) != 0) {
      uVar3 = 5;
    }
    if ((param_2 & 0x20000) != 0) {
      uVar3 = 2;
    }
    uVar2 = (uint)*(byte *)(iVar1 + 10);
    if (((uVar2 != DAT_0054126c) && (uVar2 != DAT_005412c8)) &&
       ((uVar2 != DAT_00541298 && ((uVar2 != DAT_00541280 && (uVar2 != DAT_00541344)))))) {
      if (uVar2 == DAT_00541274) {
        uVar3 = 3;
      }
      if (0x14 < DAT_0055a4f8) {
        uVar3 = uVar3 / 2;
      }
      iVar1 = 0;
      if (uVar3 != 0) {
        do {
          if (param_3 == 0) {
            FUN_00450c20(param_1,iVar1);
          }
          else {
            FUN_00450b30();
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 < (int)uVar3);
      }
    }
  }
  return;
}


// ==== FUN_00450c20 @ 00450c20

void __cdecl FUN_00450c20(int param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined1 local_8 [8];
  
  if (param_1 == 0) {
    FUN_004e0620("destSid",".\\Dude.cpp",0x1d5);
    FUN_004e0620("destSid",".\\Dude.cpp",0x1db);
  }
  iVar2 = param_1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
    puVar1 = (undefined4 *)FUN_004505a0(local_8,param_1);
    iVar2 = FUN_00450a20(*puVar1,puVar1[1],*(undefined1 *)(iVar2 + 0x22));
    if (iVar2 != 0) {
      if ((*(byte *)(iVar2 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
        FUN_00450670(iVar2 * 0x32 + DAT_005c8464,0x5104,
                     (float)param_2 * (float)s_333_drop_value____0_00501784._0_4_);
      }
    }
  }
  return;
}


// ==== FUN_00450b30 @ 00450b30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_00450b30(int param_1,int param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  float10 fVar6;
  
  if (param_1 == 0) {
    FUN_004e0620("srcSid",".\\Dude.cpp",0x1c4);
  }
  iVar4 = param_1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
    fVar1 = *(float *)(iVar4 + 0x12);
    fVar2 = *(float *)(iVar4 + 0xe);
    iVar3 = FUN_004ac550(iVar4);
    fVar5 = (float10)FUN_00455930(0x3dcccccd,(float)*(int *)(iVar3 + 0x1d4) - _DAT_005022ec);
    iVar3 = FUN_004ac550(iVar4);
    fVar6 = (float10)FUN_00455930(0x3dcccccd,(float)*(int *)(iVar3 + 0x1d8) - _DAT_005022ec);
    iVar4 = FUN_00450a20((float)((float10)fVar2 - fVar5),(float)((float10)fVar1 - fVar6),
                         *(undefined1 *)(iVar4 + 0x22));
    if (iVar4 != 0) {
      if ((*(byte *)(iVar4 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
        FUN_00450670(iVar4 * 0x32 + DAT_005c8464,0x5103,(float)param_2 * _DAT_005022ec);
      }
    }
  }
  return;
}


// ==== FUN_00450a20 @ 00450a20

int __cdecl FUN_00450a20(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  undefined4 unaff_retaddr;
  
  if ((DAT_0055a4f8 < DAT_0055a4f4) && (DAT_0055a4e8 == 0)) {
    piVar1 = (int *)FUN_004af530(DAT_00541170,2);
    piVar1[9] = 0;
    (**(code **)(*piVar1 + 0x74))(param_3);
    (**(code **)(*piVar1 + 0x90))(unaff_retaddr,param_1,0);
    return ((int)piVar1 - DAT_005c8464) / 0x32;
  }
  return 0;
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


// ==== FUN_004505a0 @ 004505a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004505a0(float *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  float local_4;
  
  if (param_2 == 0) {
    FUN_004e0620(&DAT_00501390,".\\Dude.cpp",0x29);
  }
  local_4 = *(float *)(param_2 * 0x32 + 0x12 + DAT_005c8464);
  iVar4 = param_2 * 0x32 + DAT_005c8464;
  FUN_004ac550(iVar4);
  iVar2 = FUN_004e49c0();
  fVar1 = (float)iVar2 + _DAT_00500ee4;
  iVar2 = FUN_004ac550(iVar4);
  local_4 = local_4 - (float)*(int *)(iVar2 + 0x1d8);
  uVar3 = FUN_004ac200(iVar4);
  if ((DAT_005424b8 & uVar3) == 0) {
    local_4 = local_4 - _DAT_00500ee4;
  }
  else {
    uVar3 = FUN_004ac200(iVar4);
    if ((uVar3 & 0x10040000) != 0) {
      local_4 = local_4 + _DAT_005012d0;
    }
  }
  *param_1 = fVar1;
  param_1[1] = local_4;
  return;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __thiscall FUN_004ac200(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __thiscall FUN_004ac1e0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_004ac950 @ 004ac950

undefined4 __thiscall FUN_004ac950(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(undefined4 *)(iVar1 + 0x9c);
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


// ==== FUN_004adc60 @ 004adc60

uint __thiscall FUN_004adc60(void *this)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x400000) != 0) {
    return *(byte *)((int)this + 0x28) & 1;
  }
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  return *(uint *)(iVar1 + 0xe8) & 0x10;
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


// ==== FUN_004adea0 @ 004adea0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004adea0(void *this,float *param_2)

{
  float fVar1;
  undefined4 uVar2;
  int *piVar3;
  int *piVar4;
  undefined4 uVar5;
  int local_10;
  int local_c [3];
  
  uVar5 = 0;
  piVar4 = local_c;
  piVar3 = &local_10;
  uVar2 = FUN_0049a840(*(undefined1 *)((int)this + 10),piVar3,piVar4,0);
  FUN_0049a900(uVar2,piVar3,piVar4,uVar5);
  fVar1 = *(float *)((int)this + 0x12);
  *param_2 = *(float *)((int)this + 0xe) - (float)local_10 * _DAT_00500ee4;
  param_2[1] = fVar1 - (float)local_c[0] * _DAT_00500ee4;
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


// ==== FUN_004abb00 @ 004abb00

uint __cdecl FUN_004abb00(int param_1)

{
  return param_1 >> 0x10 & 0xf;
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


// ==== FUN_004224c0 @ 004224c0

byte __cdecl FUN_004224c0(int param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  if (param_1 != 0) {
    iVar2 = param_1 * 0x32 + DAT_005c8464;
    uVar3 = FUN_004ac200(iVar2);
    if ((uVar3 & 4) != 0) {
      iVar1 = *(int *)(iVar2 + 0x24);
      iVar2 = FUN_004ac550(iVar2);
      return *(byte *)(*(int *)(iVar2 + 0x124) + 3 + iVar1 * 4) & 0x20;
    }
  }
  return 0;
}


// ==== FUN_00421fe0 @ 00421fe0

bool __cdecl FUN_00421fe0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_004afb40(param_1 * 0x32 + DAT_005c8464,0x2692);
  return iVar1 != 0;
}


// ==== FUN_00422560 @ 00422560

byte __cdecl FUN_00422560(int param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  if (param_1 != 0) {
    iVar2 = param_1 * 0x32 + DAT_005c8464;
    uVar3 = FUN_004ac200(iVar2);
    if ((uVar3 & 4) != 0) {
      iVar1 = *(int *)(iVar2 + 0x24);
      iVar2 = FUN_004ac550(iVar2);
      return *(byte *)(*(int *)(iVar2 + 0x124) + 3 + iVar1 * 4) & 0x40;
    }
  }
  return 0;
}


// ==== FUN_00421db0 @ 00421db0

undefined4 __cdecl FUN_00421db0(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int *piVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  
  piVar5 = (int *)(param_1 * 0x32 + DAT_005c8464);
  *(ushort *)(piVar5 + 3) =
       *(ushort *)(piVar5 + 3) ^
       (byte)((char)DAT_0052f960 * '\b' - 8U ^ *(byte *)(param_1 * 0x32 + 0xc + DAT_005c8464)) &
       0x78;
  FUN_004214a0(((int)piVar5 - DAT_005c8464) / 0x32);
  iVar1 = FUN_004ac550(piVar5);
  iVar2 = (int)*(char *)(*(int *)(iVar1 + 0x124) + 2 + piVar5[9] * 4);
  if (iVar2 < 0x14) {
    if (9 < iVar2) {
      return 0;
    }
  }
  else {
    iVar2 = 1;
  }
  uVar3 = CONCAT31((int3)((uint)iVar2 >> 8),(char)iVar2 + '\n');
  uVar7 = 0x20;
  uVar6 = 0x50;
  uVar4 = FUN_004ad680(piVar5,0x50,uVar3,0x20);
  iVar1 = FUN_0049a9e0(iVar1,uVar4,uVar6,uVar3,uVar7);
  if (iVar1 == -1) {
    return 0;
  }
  piVar5[9] = iVar1;
  (**(code **)(*piVar5 + 0x88))();
  return 1;
}


// ==== FUN_004214a0 @ 004214a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004214a0(int param_1)

{
  int iVar1;
  
  iVar1 = param_1 * 0x32 + DAT_005c8464;
  if ((((*(byte *)(iVar1 + 10) == _DAT_005411a0) && ((*(byte *)(iVar1 + 0x28) & 8) == 0)) &&
      (DAT_005e4794 != 0)) && (7 < (*(byte *)(iVar1 + 0xc) >> 3 & 0xf))) {
    FUN_004e0620("num >= 0 && num <= 7","\\Ns\\O\\Bridge.cpp",0xff);
  }
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


// ==== FUN_0049a9e0 @ 0049a9e0

int __thiscall FUN_0049a9e0(void *this,char param_2,char param_3,char param_4,uint param_5)

{
  int iVar1;
  char *pcVar2;
  
  pcVar2 = *(char **)((int)this + 0x124);
  iVar1 = 0;
  if (0 < *(int *)((int)this + 0x114)) {
    do {
      if ((((*pcVar2 == param_2) && (pcVar2[1] == param_3)) && (pcVar2[2] == param_4)) &&
         ((param_5 & (int)pcVar2[3]) != 0)) {
        return iVar1;
      }
      iVar1 = iVar1 + 1;
      pcVar2 = pcVar2 + 4;
    } while (iVar1 < *(int *)((int)this + 0x114));
  }
  return -1;
}


// ==== FUN_0046e6c0 @ 0046e6c0

void __cdecl FUN_0046e6c0(float param_1,float param_2,uint param_3,uint param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int local_60;
  undefined1 local_54 [4];
  undefined4 local_50;
  undefined4 local_4c;
  uint local_48;
  uint local_44;
  undefined1 local_40 [52];
  int local_c;
  
  iVar3 = 0;
  if (DAT_0054db84 != 0) {
    return;
  }
  if ((DAT_00540bc0 != 0) && ((param_3 & 0x50) == 0)) {
    FUN_0040a740(local_54);
    local_50 = FUN_004e49c0();
    local_4c = FUN_004e49c0();
    local_48 = param_3;
    local_44 = param_4;
    FUN_004e2e30(local_54);
  }
  iVar1 = FUN_0049a840(DAT_005411d0);
  FUN_0041def0(local_40,param_1 - (float)(*(int *)(iVar1 + 0x1d4) + -1),
               param_2 - (float)(*(int *)(iVar1 + 0x1d8) + -1),param_1,param_2,0);
  iVar1 = 0;
  local_60 = 0;
  while ((uVar2 = local_c * 0x32 + DAT_005c8464, DAT_005c8464 + 0x32U <= uVar2 &&
         (uVar2 < (uint)(DAT_005c847c * 0x32 + DAT_005c8464)))) {
    if (*(byte *)(uVar2 + 0x22) == param_4) {
      uVar2 = (uint)*(byte *)(uVar2 + 10);
      if ((uVar2 == DAT_005412cc) || (uVar2 == DAT_005411d4)) {
        if ((param_3 & 0x1000) == 0) {
          FUN_00460600(local_c);
          FUN_004a9d70(*(undefined4 *)(local_c * 0x32 + 0xe + DAT_005c8464),
                       *(undefined4 *)(local_c * 0x32 + DAT_005c8464 + 0x12),"islandFall.wav",0);
        }
        (**(code **)(*(int *)(local_c * 0x32 + DAT_005c8464) + 0x10))(param_3 | 0x48);
        if (uVar2 == DAT_005412cc) {
          iVar1 = iVar1 + 1;
        }
        else {
          local_60 = local_60 + 1;
        }
      }
      if (uVar2 == DAT_005411d8) {
        (**(code **)(*(int *)(local_c * 0x32 + DAT_005c8464) + 0x10))(param_3 | 0x48);
      }
      if (uVar2 == DAT_005411d0) {
        (**(code **)(*(int *)(local_c * 0x32 + DAT_005c8464) + 0x10))(param_3);
        iVar3 = iVar3 + 1;
      }
      uVar2 = FUN_004ac200(local_c * 0x32 + DAT_005c8464);
      if ((uVar2 & 0x10000) != 0) {
        (**(code **)(*(int *)(local_c * 0x32 + DAT_005c8464) + 200))();
      }
    }
    FUN_004b1810(local_40);
  }
  if (iVar1 == 0) {
    if ((iVar3 == 0) && (local_60 == 0)) {
      return;
    }
  }
  else if (iVar1 == 9) goto LAB_0046e8f6;
  FUN_004e0620("noIslandTypeCount == 9","\\Ns\\O\\Islanddropper.cpp",0x7b);
LAB_0046e8f6:
  if (iVar3 != 1) {
    FUN_004e0620("islandTypeCount == 1","\\Ns\\O\\Islanddropper.cpp",0x7c);
  }
  if (local_60 != 1) {
    FUN_004e0620("islandStalagTypeCount == 1","\\Ns\\O\\Islanddropper.cpp",0x7d);
  }
  return;
}


// ==== FUN_004b0840 @ 004b0840

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004b0840(void *this,uint param_2)

{
  int iVar1;
  undefined4 *puVar2;
  undefined1 local_50 [16];
  undefined1 local_40 [52];
  int local_c;
  
  if ((((DAT_00540414 != 0) && ((*(byte *)((int)this + 0x28) & 8) == 0)) && ((param_2 & 0x800) == 0)
      ) && (iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10)),
           (*(uint *)(iVar1 + 0xec) & (DAT_005424b8 | 8)) != 0)) {
    puVar2 = (undefined4 *)FUN_004ade70(this,local_50,0);
    FUN_0041def0(local_40,*puVar2,puVar2[1],puVar2[2],puVar2[3],0);
    while (local_c != 0) {
      iVar1 = FUN_0049a840(*(undefined1 *)(local_c * 0x32 + DAT_005c8464 + 10));
      if ((*(uint *)(iVar1 + 0xe8) & 0x800) != 0) {
        FUN_004633c0(local_c);
      }
      FUN_004b1810(local_40);
    }
  }
  if ((*(byte *)((int)this + 0x28) & 9) == 0) {
    _DAT_0059508c = FUN_004e49c0();
  }
  DAT_005c8484 = DAT_005c8484 + -1;
  return;
}


// ==== FUN_00496e80 @ 00496e80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00496e80(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb2d8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  uVar1 = _DAT_00500460;
  *(undefined ***)this = &PTR_FUN_0050f420;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  local_4 = 0;
  FUN_0041bd20(this,DAT_00541128,param_3,0,0x50);
  *(undefined4 *)((int)this + 0x1c) = param_4;
  uVar1 = _DAT_0055b4d0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined8 *)((int)this + 0x10) = uVar1;
  *(undefined4 *)((int)this + 0x18) = param_2;
  ExceptionList = local_c;
  return this;
}


