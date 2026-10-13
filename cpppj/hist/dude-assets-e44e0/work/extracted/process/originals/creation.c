// ==== FUN_0041bd00 @ 0041bd00

void __fastcall FUN_0041bd00(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_005019ec;
  param_1[1] = 0xffffcfc7;
  param_1[2] = 0xffffcfc7;
  return;
}


// ==== FUN_0041bd20 @ 0041bd20

int __thiscall FUN_0041bd20(int *param_1,int param_2,int param_3,int param_4,uint param_5)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int *piVar4;
  
  if ((param_2 < 10) || (DAT_0054116c <= param_2)) {
    FUN_004e0620("isProcessType(_processType)","\\Ns\\O\\Baseprocess.cpp",0x1c);
  }
  param_1[2] = param_3;
  param_1[1] = param_4;
  iVar1 = param_3 * 0x32 + DAT_005c8464;
  if ((*(byte *)(param_3 * 0x32 + 0xb + DAT_005c8464) & 3) == 0) {
    iVar2 = FUN_00471950(param_1);
    iVar1 = param_1[1];
    if (iVar1 == 0) {
      uVar3 = 0;
      if ((param_5 & 0x10) != 0) {
        uVar3 = 2;
      }
      piVar4 = (int *)FUN_004af530(DAT_00541078,uVar3);
      param_1[1] = ((int)piVar4 - DAT_005c8464) / 0x32;
      FUN_004ae2e0(param_2);
      *(int *)((int)piVar4 + 0x12) = iVar2;
      (**(code **)(*piVar4 + 0x74))(param_1[2],param_5 | 0x80);
      iVar1 = (**(code **)(*param_1 + 0x14))();
      return iVar1;
    }
    piVar4 = (int *)(iVar1 * 0x32 + DAT_005c8464);
    if ((*(byte *)(iVar1 * 0x32 + 0xb + DAT_005c8464) & 1) != 0) {
      FUN_004af610(DAT_00541078,iVar1);
      *(char *)((int)piVar4 + 10) = (char)param_2;
      *(int *)((int)piVar4 + 0x12) = iVar2;
      (**(code **)(*piVar4 + 0x74))(param_1[2],param_5 | 0x80);
    }
    iVar1 = (**(code **)*piVar4)();
    if (iVar1 != param_1[2]) {
      FUN_004e0620("s.getParentSid() == attachToSid","\\Ns\\O\\Baseprocess.cpp",0x5e);
    }
    if (*(int *)((int)piVar4 + 0x12) != iVar2) {
      FUN_004e0620("s.getPid() == pid","\\Ns\\O\\Baseprocess.cpp",0x5f);
    }
    iVar1 = (**(code **)(*param_1 + 0x14))();
  }
  return iVar1;
}


// ==== FUN_0041be80 @ 0041be80

undefined4 __fastcall FUN_0041be80(int param_1)

{
  uint uVar1;
  
  uVar1 = *(int *)(param_1 + 4) * 0x32 + DAT_005c8464;
  if ((*(int *)(param_1 + 4) != 0) &&
     ((uVar1 < DAT_005c8464 + 0x32U || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar1)))) {
    FUN_004e0620("formSid == 0 || form.isValid()","\\Ns\\O\\Baseprocess.cpp",0x69);
  }
  uVar1 = *(int *)(param_1 + 8) * 0x32 + DAT_005c8464;
  if ((*(int *)(param_1 + 8) != 0) &&
     ((uVar1 < DAT_005c8464 + 0x32U || ((uint)(DAT_005c847c * 0x32 + DAT_005c8464) <= uVar1)))) {
    FUN_004e0620("attachToSid == 0 || s.isValid()","\\Ns\\O\\Baseprocess.cpp",0x6c);
  }
  return 1;
}


// ==== FUN_0041bf10 @ 0041bf10

void __thiscall FUN_0041bf10(int param_1,uint param_2)

{
  int *piVar1;
  
  if ((DAT_00540bc4 != 0) ||
     (((4 < *(int *)(param_1 + 4) && (*(int *)(param_1 + 4) < 15000)) || ((param_2 & 0x10) != 0))))
  {
    piVar1 = (int *)(*(int *)(param_1 + 4) * 0x32 + DAT_005c8464);
    if ((piVar1 < (int *)(DAT_005c8464 + 0x32U)) ||
       ((int *)(DAT_005c847c * 0x32 + DAT_005c8464) <= piVar1)) {
      FUN_004e0620("s.isValid()","\\Ns\\O\\Baseprocess.cpp",0x98);
    }
    if ((*(byte *)((int)piVar1 + 0xb) & 2) != 0) {
      FUN_004e0620("!s.isDead()","\\Ns\\O\\Baseprocess.cpp",0x99);
    }
    (**(code **)(*piVar1 + 0x10))(param_2);
  }
  return;
}


// ==== FUN_0041bfa0 @ 0041bfa0

int __fastcall FUN_0041bfa0(int param_1)

{
  return *(int *)(param_1 + 4) * 0x32 + DAT_005c8464;
}


// ==== FUN_0041bfb0 @ 0041bfb0

int __fastcall FUN_0041bfb0(int param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 8) * 0x32 + DAT_005c8464;
  if (*(short *)(iVar1 + 6) == 0) {
    FUN_004e0620("parent->getHead() != 0","\\Ns\\O\\Baseprocess.cpp",0xa4);
  }
  return iVar1;
}


// ==== FUN_0041bff0 @ 0041bff0

undefined1 __fastcall FUN_0041bff0(int param_1)

{
  return *(undefined1 *)(*(int *)(param_1 + 4) * 0x32 + 10 + DAT_005c8464);
}


// ==== FUN_0041c010 @ 0041c010

void __fastcall FUN_0041c010(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = (*(code *)**(undefined4 **)(DAT_005c8464 + *(int *)(param_1 + 4) * 0x32))();
  iVar2 = iVar1 * 0x32 + DAT_005c8464;
  if ((*(byte *)(iVar1 * 0x32 + 0xb + DAT_005c8464) & 2) != 0) {
    FUN_004e0620("!parent.isDead()","\\Ns\\O\\Baseprocess.cpp",0xb0);
  }
  if ((*(byte *)(iVar2 + 0xb) & 1) != 0) {
    FUN_004e0620("!parent.isFree()","\\Ns\\O\\Baseprocess.cpp",0xb1);
  }
  (**(code **)(*(int *)(DAT_005c8464 + *(int *)(param_1 + 4) * 0x32) + 0x48))(0);
  (**(code **)(*(int *)(DAT_005c8464 + *(int *)(param_1 + 4) * 0x32) + 0x74))(iVar1,0);
  return;
}


// ==== FUN_00471950 @ 00471950

int FUN_00471950(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined1 local_100 [256];
  
  FUN_0049c2c0();
  iVar3 = 0;
  iVar1 = 1;
  while( true ) {
    if (((&DAT_0056b808)[iVar1] == 0) && (iVar3 == 0)) {
      iVar3 = iVar1;
    }
    if ((&DAT_0056b808)[iVar1] == param_1) break;
    iVar1 = iVar1 + 1;
    if (39999 < iVar1) {
      if (iVar3 == 0) {
        FUN_00471890();
        uVar2 = FUN_004a89c0(local_100,
                             "Out of processes. A process count trace has been dumped\nto procSumm.txt.  Please deliver this file with the bug report.\n"
                            );
        FUN_00436d20(uVar2);
        FUN_004e0620("firstSlot != 0","\\Ns\\O\\Kernel.cpp",0x7a);
      }
      (&DAT_0056b808)[iVar3] = param_1;
      return iVar3;
    }
  }
  return iVar1;
}


// ==== FUN_004719e0 @ 004719e0

void FUN_004719e0(int param_1)

{
  if ((&DAT_0056b808)[param_1] == 0) {
    FUN_004e0620("processes[pid] != NULL","\\Ns\\O\\Kernel.cpp",0x82);
  }
  if ((undefined4 *)(&DAT_0056b808)[param_1] != (undefined4 *)0x0) {
    (*(code *)**(undefined4 **)(&DAT_0056b808)[param_1])(1);
  }
  (&DAT_0056b808)[param_1] = 0;
  return;
}


// ==== FUN_00471a30 @ 00471a30

void FUN_00471a30(void)

{
  int *piVar1;
  
  piVar1 = &DAT_0056b80c;
  do {
    if ((int *)*piVar1 != (int *)0x0) {
      (**(code **)(*(int *)*piVar1 + 0x18))();
    }
    piVar1 = piVar1 + 1;
  } while ((int)piVar1 < 0x592908);
  return;
}


// ==== FUN_00471aa0 @ 00471aa0

undefined4 FUN_00471aa0(int param_1)

{
  return (&DAT_0056b808)[param_1];
}


// ==== FUN_00496cb0 @ 00496cb0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00496cb0(int *param_1)

{
  float fVar1;
  int *piVar2;
  undefined4 *puVar3;
  uint uVar4;
  float10 fVar5;
  float fVar6;
  
  piVar2 = (int *)FUN_0041bfb0();
  if ((*(byte *)((int)piVar2 + 0xb) & 3) == 0) {
    if (DAT_005318ec == 8) {
      puVar3 = (undefined4 *)FUN_0041bfb0();
      *puVar3 = 0;
      puVar3[1] = 0;
      *(undefined2 *)(puVar3 + 2) = 0;
    }
    if ((*(byte *)((int)piVar2 + 10) == DAT_00541240) &&
       (_DAT_00503350 < *(double *)(param_1 + 4) - _DAT_0055b4d0)) {
      *(double *)(param_1 + 4) = _DAT_0055b4d0;
    }
    if (((*(byte *)(piVar2 + 10) & 1) != 0) && (uVar4 = FUN_004ac200(), (uVar4 & 0x404200) == 0)) {
      return;
    }
    if ((*(double *)(param_1 + 4) <= _DAT_0055b4d0) && ((*(byte *)((int)piVar2 + 0xb) & 4) == 0)) {
      if ((*param_1 == -0x22222223) &&
         (FUN_004e0620("*((int*)this) != 0xdddddddd","\\Ns\\O\\Regular.cpp",0x4b),
         *param_1 == -0x22222223)) {
        return;
      }
      fVar5 = (float10)(**(code **)(*piVar2 + 0x5c))(param_1[6],param_1[8],param_1[7]);
      fVar6 = (float)fVar5;
      if (fVar5 < (float10)_DAT_00500fc4 == (fVar5 == (float10)_DAT_00500fc4)) {
        if ((*param_1 == -0x22222223) &&
           (FUN_004e0620("*((int*)this) != 0xdddddddd","\\Ns\\O\\Regular.cpp",0x61),
           *param_1 == -0x22222223)) {
          return;
        }
        param_1[7] = (int)fVar6;
        fVar1 = (float)_DAT_0055b4d0;
        param_1[8] = param_1[8] + 1;
        *(double *)(param_1 + 4) = (double)(fVar6 + fVar1);
      }
      else if (fVar6 == _DAT_00500fc4) {
        FUN_0041bf10(0);
        return;
      }
    }
  }
  return;
}


// ==== FUN_00496e70 @ 00496e70

float10 FUN_00496e70(undefined4 param_1,undefined4 param_2,float param_3)

{
  return (float10)param_3;
}


// ==== FUN_00496e80 @ 00496e80

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 * __thiscall
FUN_00496e80(undefined4 *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_004fb2d8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  param_1[2] = 0;
  param_1[1] = 0;
  uVar1 = _DAT_00500460;
  *param_1 = &PTR_FUN_0050f420;
  *(undefined8 *)(param_1 + 4) = uVar1;
  local_4 = 0;
  FUN_0041bd20(DAT_00541128,param_3,0,0x50);
  param_1[7] = param_4;
  uVar1 = _DAT_0055b4d0;
  param_1[8] = 0;
  *(undefined8 *)(param_1 + 4) = uVar1;
  param_1[6] = param_2;
  ExceptionList = local_c;
  return param_1;
}


// ==== FUN_00468230 @ 00468230

void * __thiscall FUN_00468230(void *param_1,byte param_2)

{
  thunk_FUN_0041bd00();
  if ((param_2 & 1) != 0) {
    _free(param_1);
  }
  return param_1;
}


// ==== FUN_00418500 @ 00418500

void * __thiscall FUN_00418500(void *param_1,byte param_2)

{
  FUN_0041bd00();
  if ((param_2 & 1) != 0) {
    _free(param_1);
  }
  return param_1;
}


// ==== FUN_004ad000 @ 004ad000

uint __fastcall FUN_004ad000(int param_1)

{
  return *(uint *)(param_1 + 0xe) & 0xffff;
}


// ==== FUN_004ad010 @ 004ad010

undefined2 __fastcall FUN_004ad010(int param_1)

{
  return *(undefined2 *)(param_1 + 0x10);
}


// ==== FUN_004ad020 @ 004ad020

void __thiscall FUN_004ad020(int param_1,uint param_2)

{
  *(undefined2 *)(param_1 + 0xe) = 0;
  *(uint *)(param_1 + 0xe) = *(uint *)(param_1 + 0xe) | param_2 & 0xffff;
  return;
}


// ==== FUN_004ad040 @ 004ad040

void __thiscall FUN_004ad040(int param_1,int param_2)

{
  *(undefined2 *)(param_1 + 0x10) = 0;
  *(uint *)(param_1 + 0xe) = *(uint *)(param_1 + 0xe) | param_2 << 0x10;
  return;
}


// ==== FUN_004abac0 @ 004abac0

undefined2 __fastcall FUN_004abac0(int param_1)

{
  return *(undefined2 *)(param_1 + 0xe);
}


// ==== FUN_004abad0 @ 004abad0

void __thiscall FUN_004abad0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x10) = param_2;
  return;
}


// ==== FUN_004abae0 @ 004abae0

void __thiscall FUN_004abae0(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xe) = param_2;
  return;
}


// ==== FUN_004ae240 @ 004ae240

void __thiscall FUN_004ae240(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_00471aa0(*(undefined4 *)(param_1 + 0x12));
  (**(code **)(*piVar1 + 0xc))(param_2);
  FUN_004719e0(*(undefined4 *)(param_1 + 0x12));
  DAT_005c8480 = DAT_005c8480 + -1;
  return;
}


// ==== FUN_004ac1b0 @ 004ac1b0

void FUN_004ac1b0(void)

{
  DAT_005c8484 = DAT_005c8484 + -1;
  return;
}


// ==== FUN_004ae2b0 @ 004ae2b0

void __fastcall FUN_004ae2b0(int param_1)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_00471aa0(*(undefined4 *)(param_1 + 0x12));
  if (piVar1 != (int *)0x0) {
                    /* WARNING: Could not recover jumptable at 0x004ae2c4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (**(code **)(*piVar1 + 0x10))();
    return;
  }
  return;
}


// ==== FUN_004ac1d0 @ 004ac1d0

void FUN_004ac1d0(void)

{
  DAT_005c8488 = DAT_005c8488 + -1;
  return;
}


// ==== FUN_0041c100 @ 0041c100

void FUN_0041c100(void)

{
  return;
}


// ==== FUN_00435060 @ 00435060

undefined4 FUN_00435060(void)

{
  return 1;
}


// ==== FUN_004ae280 @ 004ae280

void __thiscall FUN_004ae280(int param_1,undefined4 param_2,undefined4 param_3)

{
  int *piVar1;
  
  if (DAT_00540bc0 != 0) {
    piVar1 = (int *)FUN_00471aa0(*(undefined4 *)(param_1 + 0x12));
    (**(code **)(*piVar1 + 0x1c))(2,param_3,param_2);
  }
  return;
}


// ==== FUN_004ac990 @ 004ac990

undefined4 FUN_004ac990(void)

{
  return 10;
}


// ==== FUN_004ac9a0 @ 004ac9a0

undefined4 FUN_004ac9a0(void)

{
  return 1000;
}


// ==== FUN_004ae140 @ 004ae140

void __fastcall FUN_004ae140(undefined4 *param_1)

{
  int iVar1;
  
  if (0x45 < *(byte *)((int)param_1 + 10)) {
    FUN_004e0620("isForm()","\\Ns\\O\\Squid.cpp",0xa59);
  }
  if ((param_1 < (undefined4 *)(DAT_005c8464 + 0x32U)) ||
     ((undefined4 *)(DAT_005c847c * 0x32 + DAT_005c8464) <= param_1)) {
    FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0xa5a);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) != 0) {
    FUN_004e0620("!isVoid()","\\Ns\\O\\Squid.cpp",0xa5b);
  }
  *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 4;
  iVar1 = (**(code **)*param_1)();
  if ((*(byte *)(iVar1 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
    FUN_004ac6b0();
  }
  return;
}


// ==== FUN_004ac560 @ 004ac560

void __thiscall FUN_004ac560(int *param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  
  if ((param_1 < (int *)(DAT_005c8464 + 0x32U)) ||
     ((int *)(DAT_005c847c * 0x32 + DAT_005c8464) <= param_1)) {
    FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0x35b);
  }
  if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
    FUN_004e0620("isVoid()","\\Ns\\O\\Squid.cpp",0x35c);
  }
  bVar1 = *(byte *)((int)param_1 + 0xb);
  *(byte *)((int)param_1 + 0xb) = bVar1 & 0xfb;
  if ((bVar1 & 0x10) != 0) {
    FUN_004e0620("!xmitJustCreatedFlag","\\Ns\\O\\Squid.cpp",0x35f);
  }
  if (DAT_00540bc0 == 0) {
    if ((param_2 & 8) == 0) {
      param_2 = param_2 | 0x10;
    }
  }
  else {
    iVar2 = ((int)param_1 - DAT_005c8464) / 0x32;
    if ((iVar2 < 5) || (14999 < iVar2)) {
      if ((param_2 & 0x10) == 0) {
        param_2 = param_2 | 8;
      }
    }
    else {
      if ((param_2 & 8) != 0) {
        FUN_004e0620("!(popFlags & pfSERVER_POP)","\\Ns\\O\\Squid.cpp",0x364);
      }
      param_2 = param_2 | 0x10;
    }
    if ((param_2 & 0x50) == 0) {
      if ((param_2 & 7) != 0) {
        *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 0x10;
      }
      FUN_004ac260(0xffffffff,0xffffffff,0,param_2);
      *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xef;
    }
  }
  iVar2 = DAT_005c8488;
  DAT_005c8488 = DAT_005c8488 + 1;
  (**(code **)(*param_1 + 0x20))(param_2);
  if (iVar2 != DAT_005c8488) {
    FUN_004e0620("curPostPop == basePostPop","\\Ns\\O\\Squid.cpp",0x392);
  }
  return;
}


// ==== FUN_004ac800 @ 004ac800

void __thiscall FUN_004ac800(int *param_1,int param_2)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  uint unaff_retaddr;
  
  if (param_2 != 0) {
    iVar2 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
    if ((*(uint *)(iVar2 + 0xec) & 0x44200) != 0) {
      FUN_004e0620("!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))","\\Ns\\O\\Squid.cpp",0x3d0);
    }
    (**(code **)(*param_1 + 8))(param_2);
    uVar1 = *(ushort *)(param_2 * 0x32 + 6 + DAT_005c8464);
    iVar2 = param_2 * 0x32 + DAT_005c8464;
    if ((uint)uVar1 == ((int)param_1 - DAT_005c8464) / 0x32) {
      FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
    }
    *(ushort *)(param_1 + 1) = uVar1;
    if (uVar1 != 0) {
      (**(code **)(*(int *)((uint)uVar1 * 0x32 + DAT_005c8464) + 0xc))
                (((int)param_1 - DAT_005c8464) / 0x32);
    }
    iVar3 = (int)param_1 - DAT_005c8464;
    *(short *)(iVar2 + 6) =
         ((short)(iVar3 / 0x32) + (short)(iVar3 >> 0x1f)) -
         (short)((longlong)iVar3 * 0x51eb851f >> 0x3f);
    (**(code **)(*param_1 + 0xc))(0);
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) | 8;
    FUN_004ac560(unaff_retaddr | 0x100);
    if (*(byte *)((int)param_1 + 10) == DAT_00541088) {
      FUN_004e0620("getType()!=contentFormType","\\Ns\\O\\Squid.cpp",0x3e0);
    }
    if (0x45 < *(byte *)((int)param_1 + 10)) {
      FUN_004e0620("isForm()","\\Ns\\O\\Squid.cpp",0x3e3);
    }
  }
  return;
}


// ==== FUN_004ae360 @ 004ae360

void __thiscall FUN_004ae360(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = (**(code **)*param_1)();
  iVar1 = (iVar1 * 0x32) / 0x32;
  if ((4 < iVar1) && (iVar1 < 15000)) {
    FUN_004e0620("!getParent().isClientSid()","\\Ns\\O\\Squid.cpp",0xac2);
  }
  piVar2 = (int *)FUN_00471aa0(*(undefined4 *)((int)param_1 + 0x12));
  (**(code **)(*piVar2 + 8))(param_2,0,param_3);
  return;
}


// ==== FUN_004ac340 @ 004ac340

undefined4 __thiscall
FUN_004ac340(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  ushort uVar4;
  
  iVar3 = FUN_004df590(DAT_0052edc6,DAT_0052edc8,DAT_0052edc7,0,param_2);
  iVar1 = (param_1 - DAT_005c8464) / 0x32;
  if ((4 < iVar1) && (iVar1 < 15000)) {
    FUN_004e0620("!isClientSid()","\\Ns\\O\\Squid.cpp",0x2f6);
  }
  uVar2 = (param_1 - DAT_005c8464) / 0x32;
  *(short *)(iVar3 + 4) = (short)uVar2;
  uVar4 = 0;
  if (DAT_005c845c == (uVar2 & 0xffff)) {
    uVar4 = (ushort)DAT_005c8460;
    DAT_005c845c = 0;
  }
  *(ushort *)(iVar3 + 6) = (ushort)((uint)param_4 >> 0x10) & 0xfff0 | uVar4;
  return 8;
}


// ==== FUN_004bcb30 @ 004bcb30

undefined4 FUN_004bcb30(void)

{
  FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Template.cpp",0x8b);
  return 0;
}


// ==== FUN_004ac120 @ 004ac120

void FUN_004ac120(void)

{
  FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Squid.cpp",0x208);
  return;
}


// ==== FUN_004ace20 @ 004ace20

void __fastcall FUN_004ace20(int param_1)

{
  FUN_0049a840(*(undefined1 *)(param_1 + 10));
  FUN_0049af80();
  return;
}


// ==== FUN_004ac9b0 @ 004ac9b0

undefined4 __thiscall FUN_004ac9b0(int *param_1,uint *param_2)

{
  if ((*param_2 & 0x3ffffff) == DAT_0054274c) {
    (**(code **)(*param_1 + 0x10))(0);
    return 1;
  }
  if ((*param_2 & 0x3ffffff) == DAT_005428b0) {
    (**(code **)(*param_1 + 0x70))();
    return 1;
  }
  return 0;
}


// ==== FUN_004ae3f0 @ 004ae3f0

void __fastcall FUN_004ae3f0(int param_1)

{
  FUN_004ac400();
  FUN_004c2630(0x1d,"~1     Process #~w%03d~n\n",*(undefined4 *)(param_1 + 0x12));
  return;
}


// ==== FUN_004b1440 @ 004b1440

void __thiscall FUN_004b1440(int param_1,uint param_2,uint param_3)

{
  int *piVar1;
  int iVar2;
  undefined4 uVar3;
  
  uVar3 = 0;
  if ((*(ushort *)(param_1 + 0x10) != param_2) && (uVar3 = 1, -1 < (char)param_3)) {
    FUN_004e0620("popFlags & pfPROCESS_DEBUG","\\Ns\\O\\Squid.cpp",0xa77);
  }
  if (DAT_00540bc0 != 0) {
    piVar1 = (int *)FUN_00471aa0(*(undefined4 *)(param_1 + 0x12));
    if ((param_3 & 0x50) == 0) {
      iVar2 = (**(code **)(*piVar1 + 8))(&DAT_005c7c50,uVar3,0);
      if (iVar2 == 0) {
        FUN_004e0620("transmitSize!=0","\\Ns\\O\\Squid.cpp",0xa7f);
        FUN_004ac800(param_2,param_3 | 0x40);
        return;
      }
      if (0 < iVar2) {
        if (0x7ff < iVar2) {
          FUN_004e0620("transmitSize < TRANSMIT_BUFFER_SIZE","\\Ns\\O\\Squid.cpp",0xa81);
        }
        FUN_004e2e30(&DAT_005c7c50);
      }
    }
    param_3 = param_3 | 0x40;
  }
  FUN_004ac800(param_2,param_3);
  return;
}


// ==== FUN_004ae3d0 @ 004ae3d0

undefined4 * FUN_004ae3d0(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_FUN_00512098;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004ae2e0 @ 004ae2e0

void __thiscall FUN_004ae2e0(int param_1,int param_2)

{
  if ((param_2 < 0) || (0xfe < param_2)) {
    FUN_004e0620("0 <= pType && pType < MAX_TYPES","\\Ns\\O\\Squid.cpp",0xab7);
  }
  if (*(byte *)(param_1 + 10) != DAT_00541078) {
    FUN_004e0620("typeNum == processFormType","\\Ns\\O\\Squid.cpp",0xab8);
  }
  *(char *)(param_1 + 10) = (char)param_2;
  return;
}


// ==== FUN_004afb40 @ 004afb40

int __thiscall FUN_004afb40(int param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  int local_8;
  
  FUN_004b22d0((param_1 - DAT_005c8464) / 0x32);
  do {
    if (local_8 == 0) {
      return 0;
    }
    uVar1 = (uint)*(byte *)(local_8 * 0x32 + 10 + DAT_005c8464);
    if ((uVar1 == DAT_00541128) || (uVar1 == DAT_00541164)) {
      iVar2 = FUN_004b2340();
      iVar2 = FUN_00471aa0(*(undefined4 *)(iVar2 + 0x12));
      if (*(int *)(iVar2 + 0x18) == param_2) {
        return iVar2;
      }
    }
    FUN_004b1b80();
  } while( true );
}


// ==== FUN_004afbe0 @ 004afbe0

int __thiscall FUN_004afbe0(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int local_8;
  
  FUN_004b22d0((param_1 - DAT_005c8464) / 0x32);
  do {
    if (local_8 == 0) {
      return 0;
    }
    uVar1 = (uint)*(byte *)(local_8 * 0x32 + 10 + DAT_005c8464);
    if ((uVar1 == DAT_00541128) || (uVar1 == DAT_00541164)) {
      iVar2 = FUN_004b2340();
      iVar2 = FUN_00471aa0(*(undefined4 *)(iVar2 + 0x12));
      if ((*(uint *)(iVar2 + 0x18) & param_2) == param_3) {
        return iVar2;
      }
    }
    FUN_004b1b80();
  } while( true );
}


// ==== FUN_004b22d0 @ 004b22d0

void __thiscall FUN_004b22d0(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((DAT_005e4794 != 0) && (DAT_00594fc8 == 0)) && ((*(byte *)(iVar1 + 0xb) & 2) != 0)) &&
     ((DAT_00540bc4 == 0 && (*(int *)(param_1 + 0xc) == 0)))) {
    FUN_004e0620("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy"
                 ,"\\Ns\\O\\Squidfinder.cpp",0x158);
  }
  *(uint *)(param_1 + 4) = (uint)*(ushort *)(iVar1 + 6);
  FUN_004b1b80();
  return;
}


// ==== FUN_004b1b80 @ 004b1b80

void __fastcall FUN_004b1b80(undefined4 *param_1)

{
  int iVar1;
  
  do {
    do {
      iVar1 = param_1[1];
      param_1[2] = iVar1;
      if (iVar1 == 0) {
        param_1[1] = 0;
        param_1[2] = 0;
        return;
      }
      param_1[1] = (uint)*(ushort *)(iVar1 * 0x32 + 4 + DAT_005c8464);
      iVar1 = param_1[2] * 0x32 + DAT_005c8464;
    } while ((iVar1 != 0) && (0x45 < *(byte *)(iVar1 + 10)));
    iVar1 = (**(code **)*param_1)(iVar1);
  } while (iVar1 == 0);
  return;
}


// ==== FUN_004b2340 @ 004b2340

int __fastcall FUN_004b2340(int param_1)

{
  uint uVar1;
  
  uVar1 = (uint)*(byte *)(*(int *)(param_1 + 8) * 0x32 + 10 + DAT_005c8464);
  if (((uVar1 < 10) || (DAT_0054116c <= (int)uVar1)) && (*(int *)(param_1 + 8) != 0)) {
    FUN_004e0620("isProcess() || sid == 0","\\Ns\\O\\Squidfinder.cpp",0x16b);
  }
  return *(int *)(param_1 + 8) * 0x32 + DAT_005c8464;
}


// ==== FUN_00421530 @ 00421530

void __fastcall FUN_00421530(int param_1)

{
  uint uVar1;
  int iVar2;
  void *pvVar3;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004f94cb;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  uVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  pvVar3 = operator_new(0x28);
  local_4 = 0;
  if (pvVar3 != (void *)0x0) {
    FUN_00496e80(0x2692,(param_1 - DAT_005c8464) / 0x32,(float)(int)(uVar1 & 0xff | iVar2 << 8));
  }
  ExceptionList = local_c;
  return;
}


// ==== FUN_00421f90 @ 00421f90

void FUN_00421f90(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  
  if (DAT_005e4794 != 0) {
    uVar1 = FUN_004ac200();
    if ((uVar1 & 4) == 0) {
      FUN_004e0620("s.getGenus() & gBRIDGE","\\Ns\\O\\Bridge.cpp",0x323);
    }
  }
  FUN_00421530(param_2,param_3);
  return;
}


// ==== FUN_00421fe0 @ 00421fe0

bool FUN_00421fe0(void)

{
  int iVar1;
  
  iVar1 = FUN_004afb40(0x2692);
  return iVar1 != 0;
}


// ==== FUN_0040b380 @ 0040b380

void FUN_0040b380(void)

{
  return;
}


// ==== FUN_00425e80 @ 00425e80

void FUN_00425e80(void)

{
  FUN_004b1bf0();
  return;
}


// ==== FUN_00441fa0 @ 00441fa0

undefined4 __fastcall FUN_00441fa0(int param_1)

{
  int iVar1;
  
  iVar1 = (param_1 - DAT_005c8464) / 0x32;
  if ((4 < iVar1) && (iVar1 < 15000)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004437f0 @ 004437f0

void FUN_004437f0(void)

{
  return;
}


// ==== FUN_004952f0 @ 004952f0

void FUN_004952f0(int param_1,int param_2)

{
  int *_Src;
  
  if ((param_1 < 5) || (_Src = &DAT_0059a698, 14999 < param_1)) {
    _Src = &DAT_0059a738;
  }
  _memmove(_Src + 2,_Src,0x98);
  *_Src = param_1;
  _Src[1] = param_2;
  return;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049c2c0 @ 0049c2c0

int FUN_0049c2c0(void)

{
  int iVar1;
  
  iVar1 = 0;
  if (DAT_0059ab20 != 0) {
    iVar1 = (DAT_0059af74 + 1) / DAT_00541348;
    DAT_0059af74 = (DAT_0059af74 + 1) % DAT_00541348;
  }
  return iVar1;
}


// ==== thunk_FUN_0041bd00 @ 004ab2c0

void __fastcall thunk_FUN_0041bd00(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_005019ec;
  param_1[1] = 0xffffcfc7;
  param_1[2] = 0xffffcfc7;
  return;
}


// ==== FUN_004aba70 @ 004aba70

void __thiscall FUN_004aba70(int param_1,int param_2)

{
  if (param_2 == (param_1 - DAT_005c8464) / 0x32) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(short *)(param_1 + 4) = (short)param_2;
  return;
}


// ==== FUN_004abb10 @ 004abb10

void FUN_004abb10(int param_1)

{
  undefined1 *puVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  
  if (0 < param_1) {
    DAT_005c847c = param_1;
    DAT_005c8464 = _malloc(param_1 * 0x32);
  }
  uVar2 = DAT_005c847c * 0x32;
  puVar6 = DAT_005c8464;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  iVar4 = 2;
  if (2 < DAT_005c847c) {
    iVar5 = 100;
    do {
      *(byte *)(iVar5 + 0xb + (int)DAT_005c8464) = *(byte *)(iVar5 + 0xb + (int)DAT_005c8464) | 1;
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 0x32;
    } while (iVar4 < DAT_005c847c);
  }
  iVar4 = DAT_005c847c;
  DAT_005c8470 = (undefined1 *)((int)DAT_005c8464 + 0xe);
  DAT_005c8468 = DAT_005c8464;
  DAT_005c8478 = 0;
  iVar7 = 5;
  iVar5 = 0xfa;
  do {
    puVar6 = DAT_005c8464;
    iVar7 = iVar7 + 1;
    if (iVar7 % 15000 == iVar5 / 0x32) {
      FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
    }
    *(short *)((int)puVar6 + iVar5 + 4) = (short)(iVar7 % 15000);
    DAT_005c8478 = DAT_005c8478 + 1;
    iVar5 = iVar5 + 0x32;
  } while (iVar5 < 750000);
  iVar5 = 750000;
  iVar7 = 15000;
  do {
    puVar6 = DAT_005c8464;
    iVar7 = iVar7 + 1;
    if (iVar7 % 0x59d9 == iVar5 / 0x32) {
      FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
    }
    *(short *)((int)puVar6 + iVar5 + 4) = (short)(iVar7 % 0x59d9);
    DAT_005c8478 = DAT_005c8478 + 1;
    iVar5 = iVar5 + 0x32;
  } while (iVar5 < 0x118c62);
  iVar5 = 0x59d9;
  if (0x59d9 < iVar4) {
    param_1 = 0x118c62;
    do {
      puVar6 = DAT_005c8464;
      iVar5 = iVar5 + 1;
      if (iVar5 % iVar4 == param_1 / 0x32) {
        FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
      }
      *(short *)((int)puVar6 + param_1 + 4) = (short)(iVar5 % iVar4);
      DAT_005c8478 = DAT_005c8478 + 1;
      param_1 = param_1 + 0x32;
    } while (iVar5 < iVar4);
  }
  puVar6 = DAT_005c8468;
  if (((int)DAT_005c8468 - (int)DAT_005c8464) / 0x32 == 5) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(undefined2 *)(puVar6 + 1) = 5;
  puVar1 = DAT_005c8470;
  DAT_005c846c = (undefined1 *)((int)DAT_005c8464 + 0xb717e);
  if (((int)DAT_005c8470 - (int)DAT_005c8464) / 0x32 == 15000) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(undefined2 *)(puVar1 + 4) = 15000;
  DAT_005c8474 = DAT_005c8464 + 0x4630c;
  if (*(short *)(DAT_005c8464 + 0x4630d) != 0) {
    FUN_004e0620("serverFreeTail->getNext() == 0","\\Ns\\O\\Squid.cpp",0xc0);
  }
  DAT_005424b0 = 1;
  return;
}


// ==== FUN_004abf60 @ 004abf60

void FUN_004abf60(int param_1)

{
  undefined1 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  puVar3 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if ((*(byte *)(param_1 * 0x32 + 0xb + DAT_005c8464) & 4) == 0) {
    FUN_004e0620("p->isVoid()","\\Ns\\O\\Squid.cpp",0x15c);
  }
  if ((*(byte *)((int)puVar3 + 0xb) & 1) != 0) {
    FUN_004e0620("!p->isFree()","\\Ns\\O\\Squid.cpp",0x15d);
  }
  FUN_004952f0(param_1,*(undefined1 *)((int)puVar3 + 10));
  uVar1 = *(undefined1 *)((int)puVar3 + 10);
  puVar4 = puVar3;
  for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  *(undefined1 *)((int)puVar3 + 10) = uVar1;
  if ((DAT_00540bc0 == 0) && (14999 < param_1)) {
    *(byte *)((int)puVar3 + 0xb) = *(byte *)((int)puVar3 + 0xb) | 1;
    return;
  }
  if ((param_1 < 5) || (puVar4 = &DAT_005c846c, 14999 < param_1)) {
    puVar4 = &DAT_005c8474;
  }
  if ((4 < param_1) && (param_1 < 0x59d9)) {
    FUN_004aba70(param_1);
    *puVar4 = puVar3;
    FUN_004aba70(0);
    DAT_005c8478 = DAT_005c8478 + 1;
  }
  *(byte *)((int)puVar3 + 0xb) = *(byte *)((int)puVar3 + 0xb) | 3;
  return;
}


// ==== FUN_004ac200 @ 004ac200

undefined4 __fastcall FUN_004ac200(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xec);
}


// ==== FUN_004ac6b0 @ 004ac6b0

void __fastcall FUN_004ac6b0(int *param_1)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int *piVar5;
  
  if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
    FUN_004e0620("containedFlag","\\Ns\\O\\Squid.cpp",0x3a3);
  }
  iVar3 = (**(code **)*param_1)();
  if (iVar3 == 0) {
    FUN_004e0620("parentSid","\\Ns\\O\\Squid.cpp",0x3a5);
  }
  piVar5 = (int *)(iVar3 * 0x32 + DAT_005c8464);
  if ((*(byte *)(iVar3 * 0x32 + 0xb + DAT_005c8464) & 1) != 0) {
    FUN_004e0620("!p.isFree()","\\Ns\\O\\Squid.cpp",0x3a8);
  }
  if ((*(byte *)((int)piVar5 + 0xb) & 2) == 0) {
    uVar2 = ((int)param_1 - DAT_005c8464) / 0x32;
    if ((*(byte *)((int)param_1 + 0xb) & 8) == 0) {
      FUN_004e0620("containedFlag","\\Ns\\O\\Squid.cpp",0x3b1);
    }
    uVar1 = *(ushort *)(param_1 + 1);
    uVar4 = (uint)uVar1;
    iVar3 = (**(code **)(*param_1 + 4))();
    if (iVar3 != 0) {
      FUN_004aba70(uVar4);
    }
    if (uVar4 != 0) {
      (**(code **)(*(int *)(uVar4 * 0x32 + DAT_005c8464) + 0xc))(iVar3);
    }
    if (*(ushort *)((int)piVar5 + 6) == uVar2) {
      if (iVar3 != 0) {
        FUN_004e0620("prev == 0","\\Ns\\O\\Squid.cpp",0x3c0);
      }
      *(ushort *)((int)piVar5 + 6) = uVar1;
    }
    *(byte *)((int)param_1 + 0xb) = *(byte *)((int)param_1 + 0xb) & 0xf7;
    if ((0x45 < *(byte *)((int)param_1 + 10)) || (*(byte *)((int)param_1 + 10) == DAT_00541088)) {
      (**(code **)(*piVar5 + 0x24))(uVar2,2);
    }
  }
  return;
}


// ==== FUN_004af1d0 @ 004af1d0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_004af1d0(uint param_1)

{
  ushort uVar1;
  ushort uVar2;
  byte *pbVar3;
  uint uVar4;
  int *piVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int *piVar11;
  undefined4 *puVar12;
  
  if (DAT_005c8458 != 0) {
    FUN_004e0620("disallowSquidAllocation == 0","\\Ns\\O\\Squid.cpp",0xe8);
  }
  if ((param_1 & 1) != 0) {
    if ((*(byte *)(DAT_005c8464 + 0x118c6d) & 1) == 0) {
      FUN_004e0620("predictableHead->isFree()","\\Ns\\O\\Squid.cpp",0xf0);
    }
    iVar10 = DAT_005424b0;
    uVar4 = DAT_005424b0 + 0x59d9;
    if (DAT_005c847c <= (int)uVar4) {
      FUN_004e0620("sid < numSquids","\\Ns\\O\\Squid.cpp",0xf2);
    }
    puVar7 = (undefined4 *)(uVar4 * 0x32 + DAT_005c8464);
    if ((*(byte *)(uVar4 * 0x32 + 0xb + DAT_005c8464) & 1) == 0) {
      FUN_004e0620("me->isFree()","\\Ns\\O\\Squid.cpp",0xf4);
    }
    if (DAT_00540bc0 != 0) {
      if ((uint)*(ushort *)(puVar7 + 1) != iVar10 + 0x59daU) {
        FUN_004e0620("me->getNext() == sid+1","\\Ns\\O\\Squid.cpp",0xf7);
      }
      iVar10 = (uint)*(ushort *)(puVar7 + 1) * 0x32 + DAT_005c8464;
      if (iVar10 == 0) {
        FUN_004e0620("checkMe","\\Ns\\O\\Squid.cpp",0xf9);
      }
      if ((*(byte *)(iVar10 + 0xb) & 1) == 0) {
        FUN_004e0620("checkMe->isFree()","\\Ns\\O\\Squid.cpp",0xfa);
      }
      FUN_004aba70(*(undefined2 *)(puVar7 + 1));
    }
    puVar12 = puVar7;
    for (iVar10 = 0xc; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar12 = 0;
      puVar12 = puVar12 + 1;
    }
    *(undefined2 *)puVar12 = 0;
    *(byte *)((int)puVar7 + 0xb) = *(byte *)((int)puVar7 + 0xb) & 0xfc | 4;
    DAT_005424b0 = DAT_005424b0 + 1;
    DAT_005c8478 = DAT_005c8478 + -1;
    return uVar4;
  }
  if ((param_1 & 2) == 0) {
    if (DAT_00540bc0 == 0) {
      FUN_004e0620("server","\\Ns\\O\\Squid.cpp",0x10a);
    }
    piVar11 = &DAT_005c8470;
    piVar5 = &DAT_005c8474;
  }
  else {
    piVar11 = &DAT_005c8468;
    piVar5 = &DAT_005c846c;
  }
  iVar9 = DAT_005c847c;
  iVar10 = DAT_005c8464;
  if ((uint)*(ushort *)(*piVar11 + 4) * 0x32 + DAT_005c8464 == *piVar5) {
    if (DAT_00540bc4 != 0) {
      iVar6 = 0x32;
      FUN_0040eb40();
      iVar8 = 1;
      do {
        piVar5 = (int *)(iVar8 * 0x32 + iVar10);
        if (((piVar5 < (int *)(iVar10 + 0x32U)) || ((int *)(DAT_005c847c * 0x32 + iVar10) <= piVar5)
            ) || (iVar6 == 0)) break;
        if (*(byte *)((int)piVar5 + 10) == _DAT_005411a0) {
          (**(code **)(*piVar5 + 0x10))(0);
          iVar6 = iVar6 + -1;
          iVar10 = DAT_005c8464;
        }
        iVar8 = iVar8 + 1;
        if (iVar8 < iVar9) {
          pbVar3 = (byte *)(iVar8 * 0x32 + 0xb + iVar10);
          do {
            if ((*pbVar3 & 1) == 0) break;
            iVar8 = iVar8 + 1;
            pbVar3 = pbVar3 + 0x32;
          } while (iVar8 < iVar9);
        }
      } while( true );
    }
    if ((param_1 & 2) == 0) {
      if (DAT_00540bc0 == 0) {
        FUN_004e0620("server","\\Ns\\O\\Squid.cpp",0x139);
        iVar10 = DAT_005c8464;
      }
      piVar11 = &DAT_005c8470;
      piVar5 = &DAT_005c8474;
    }
    else {
      piVar11 = &DAT_005c8468;
      piVar5 = &DAT_005c846c;
    }
  }
  uVar1 = *(ushort *)(*piVar11 + 4);
  iVar9 = (uint)uVar1 * 0x32;
  puVar7 = (undefined4 *)(iVar9 + iVar10);
  if ((*(byte *)(iVar9 + 0xb + iVar10) & 1) == 0) {
    FUN_004e0620("p->isFree()","\\Ns\\O\\Squid.cpp",0x149);
    iVar10 = DAT_005c8464;
  }
  if (puVar7 == (undefined4 *)*piVar5) {
    FUN_004e0620("p != *freeTail","\\Ns\\O\\Squid.cpp",0x14a);
    iVar10 = DAT_005c8464;
  }
  iVar9 = *piVar11;
  uVar2 = *(ushort *)(puVar7 + 1);
  if ((uint)uVar2 == (iVar9 - iVar10) / 0x32) {
    FUN_004e0620("n != getSid()","c:\\ns\\o\\squid.h",0x123);
  }
  *(ushort *)(iVar9 + 4) = uVar2;
  puVar12 = puVar7;
  for (iVar10 = 0xc; iVar10 != 0; iVar10 = iVar10 + -1) {
    *puVar12 = 0;
    puVar12 = puVar12 + 1;
  }
  *(undefined2 *)puVar12 = 0;
  *(byte *)((int)puVar7 + 0xb) = *(byte *)((int)puVar7 + 0xb) & 0xfc | 4;
  DAT_005c8478 = DAT_005c8478 + -1;
  return (uint)uVar1;
}


// ==== FUN_004af530 @ 004af530

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int * FUN_004af530(int param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  undefined1 uVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  if (param_1 == DAT_005412e0) {
    FUN_004e0620("type != fakeThreeByThreeSurfaceType","\\Ns\\O\\Squid.cpp",0x189);
  }
  iVar1 = FUN_004af1d0(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar1 < 0x59d9) goto LAB_004af5a1;
    uVar6 = 0x194;
    pcVar5 = "sid < FIRST_PREDICTABLE_SID";
  }
  else {
    if (23000 < iVar1) goto LAB_004af5a1;
    uVar6 = 0x191;
    pcVar5 = "sid >= FIRST_PREDICTABLE_SID";
  }
  FUN_004e0620(pcVar5,"\\Ns\\O\\Squid.cpp",uVar6);
LAB_004af5a1:
  iVar2 = FUN_0049a840(param_1);
  uVar4 = (undefined1)param_1;
  if (*(code **)(iVar2 + 0x1d0) != (code *)0x0) {
    piVar3 = (int *)(**(code **)(iVar2 + 0x1d0))(iVar1);
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  piVar3 = (int *)(iVar1 * 0x32 + DAT_005c8464);
  if (piVar3 != (int *)0x0) {
    *piVar3 = (int)&PTR_FUN_00501dd0;
    *(undefined1 *)((int)piVar3 + 10) = uVar4;
    (**(code **)(*piVar3 + 0x28))();
    return piVar3;
  }
  uRam0000000a = uVar4;
  (**(code **)(_DAT_00000000 + 0x28))();
  return (int *)0x0;
}


// ==== FUN_004af780 @ 004af780

void __thiscall FUN_004af780(int *param_1,uint param_2)

{
  byte bVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  uint unaff_retaddr;
  
  bVar1 = *(byte *)((int)param_1 + 0xb);
  if ((bVar1 & 2) == 0) {
    *(byte *)((int)param_1 + 0xb) = bVar1 | 2;
    if ((bVar1 & 1) != 0) {
      FUN_004e0620("!isFree()","\\Ns\\O\\Squid.cpp",0x24d);
    }
    if ((*(byte *)((int)param_1 + 0xb) & 1) == 0) {
      if (DAT_00540bc0 == 0) {
        if ((param_2 & 8) == 0) {
          iVar3 = FUN_00441fa0();
          if (iVar3 == 0) {
            FUN_004e0620("isClientSid()","\\Ns\\O\\Squid.cpp",0x269);
          }
          iVar3 = FUN_00441fa0();
          if (iVar3 == 0) {
            return;
          }
        }
      }
      else {
        iVar3 = FUN_00441fa0();
        if (iVar3 != 0) {
          param_2 = param_2 | 0x10;
        }
      }
      iVar3 = DAT_005c8480;
      DAT_005c8480 = DAT_005c8480 + 1;
      iVar4 = FUN_0049a840(*(undefined1 *)((int)param_1 + 10));
      FUN_004c2630(0x27,"destroying %s %d\n",iVar4 + 4,((int)param_1 - DAT_005c8464) / 0x32);
      (**(code **)(*param_1 + 0x14))(param_2);
      if (DAT_005c8480 != iVar3) {
        FUN_004e0620("baseDestroy == curDestroy","\\Ns\\O\\Squid.cpp",0x276);
      }
      uVar2 = *(ushort *)((int)param_1 + 6);
      iVar3 = (int)param_1 - DAT_005c8464;
      while (uVar2 != 0) {
        iVar4 = (uint)uVar2 * 0x32;
        piVar5 = (int *)(iVar4 + DAT_005c8464);
        if ((*(byte *)(iVar4 + 0xb + DAT_005c8464) & 1) != 0) {
          FUN_004e0620("!s.isFree()","\\Ns\\O\\Squid.cpp",0x27d);
        }
        if (((*(byte *)((int)piVar5 + 0xb) & 8) == 0) && (0x45 < *(byte *)((int)piVar5 + 10))) {
          FUN_004e0620("s.isContained() || s.isForm()","\\Ns\\O\\Squid.cpp",0x27e);
        }
        uVar2 = *(ushort *)(piVar5 + 1);
        if ((*(byte *)((int)piVar5 + 0xb) & 1) != 0) {
          FUN_004e0620("!s.isFree()","\\Ns\\O\\Squid.cpp",0x280);
        }
        (**(code **)(*piVar5 + 0x1c))(iVar3 / 0x32);
        if ((*(byte *)((int)piVar5 + 0xb) & 1) == 0) {
          (**(code **)(*piVar5 + 0x10))(unaff_retaddr | 0x40);
        }
      }
      if (((*(byte *)((int)param_1 + 0xb) & 1) == 0) ||
         (FUN_004e0620("!isFree()","\\Ns\\O\\Squid.cpp",0x28f),
         (*(byte *)((int)param_1 + 0xb) & 1) == 0)) {
        if (((unaff_retaddr & 0x50) == 0) &&
           ((DAT_00540bc0 != 0 && (iVar3 = FUN_00441fa0(), iVar3 == 0)))) {
          FUN_004ac260(0xffffffff,0xfffffffe,0,unaff_retaddr | 8);
        }
        iVar3 = FUN_004d5430();
        if (((int)param_1 - DAT_005c8464) / 0x32 == iVar3) {
          FUN_004d5c60();
        }
        if ((param_1 < (int *)(DAT_005c8464 + 0x32U)) ||
           ((int *)(DAT_005c847c * 0x32 + DAT_005c8464) <= param_1)) {
          FUN_004e0620("isValid()","\\Ns\\O\\Squid.cpp",0x2a2);
        }
        if ((*(byte *)((int)param_1 + 0xb) & 4) == 0) {
          (**(code **)(*param_1 + 0x48))(0);
        }
        iVar3 = DAT_005c8484;
        DAT_005c8484 = DAT_005c8484 + 1;
        (**(code **)(*param_1 + 0x18))(unaff_retaddr);
        if (iVar3 != DAT_005c8484) {
          FUN_004e0620("curPostDestroy == basePostDestroy","\\Ns\\O\\Squid.cpp",0x2a9);
        }
        FUN_004abf60(((int)param_1 - DAT_005c8464) / 0x32);
      }
    }
  }
  return;
}


// ==== FUN_004b1bf0 @ 004b1bf0

undefined4 __fastcall FUN_004b1bf0(int param_1)

{
  uint uVar1;
  
  uVar1 = (uint)*(byte *)(*(int *)(param_1 + 8) * 0x32 + 10 + DAT_005c8464);
  if ((9 < uVar1) && ((int)uVar1 < DAT_0054116c)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004d5430 @ 004d5430

undefined4 FUN_004d5430(void)

{
  return DAT_005caea0;
}


// ==== _memmove @ 004e5b40

/* Library Function - Single Match
    _memmove
   
   Libraries: Visual Studio 2003 Debug, Visual Studio 2003 Release */

void * __cdecl _memmove(void *_Dst,void *_Src,size_t _Size)

{
  uint uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  if ((_Src < _Dst) && (_Dst < (void *)(_Size + (int)_Src))) {
    puVar3 = (undefined4 *)((_Size - 4) + (int)_Src);
    puVar4 = (undefined4 *)((_Size - 4) + (int)_Dst);
    if (((uint)puVar4 & 3) == 0) {
      uVar1 = _Size >> 2;
      uVar2 = _Size & 3;
      if (7 < uVar1) {
        for (; uVar1 != 0; uVar1 = uVar1 - 1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + -1;
          puVar4 = puVar4 + -1;
        }
        switch(uVar2) {
        case 0:
          return _Dst;
        case 2:
          goto switchD_004e5cfb_caseD_2;
        case 3:
          goto switchD_004e5cfb_caseD_3;
        }
        goto switchD_004e5cfb_caseD_1;
      }
    }
    else {
      switch(_Size) {
      case 0:
        goto switchD_004e5cfb_caseD_0;
      case 1:
        goto switchD_004e5cfb_caseD_1;
      case 2:
        goto switchD_004e5cfb_caseD_2;
      case 3:
        goto switchD_004e5cfb_caseD_3;
      default:
        uVar1 = _Size - ((uint)puVar4 & 3);
        switch((uint)puVar4 & 3) {
        case 1:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          puVar3 = (undefined4 *)((int)puVar3 + -1);
          uVar1 = uVar1 >> 2;
          puVar4 = (undefined4 *)((int)puVar4 - 1);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
          break;
        case 2:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          puVar3 = (undefined4 *)((int)puVar3 + -2);
          puVar4 = (undefined4 *)((int)puVar4 - 2);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
          break;
        case 3:
          uVar2 = uVar1 & 3;
          *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
          *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
          uVar1 = uVar1 >> 2;
          *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
          puVar3 = (undefined4 *)((int)puVar3 + -3);
          puVar4 = (undefined4 *)((int)puVar4 - 3);
          if (7 < uVar1) {
            for (; uVar1 != 0; uVar1 = uVar1 - 1) {
              *puVar4 = *puVar3;
              puVar3 = puVar3 + -1;
              puVar4 = puVar4 + -1;
            }
            switch(uVar2) {
            case 0:
              return _Dst;
            case 2:
              goto switchD_004e5cfb_caseD_2;
            case 3:
              goto switchD_004e5cfb_caseD_3;
            }
            goto switchD_004e5cfb_caseD_1;
          }
        }
      }
    }
    switch(uVar1) {
    case 7:
      puVar4[7 - uVar1] = puVar3[7 - uVar1];
    case 6:
      puVar4[6 - uVar1] = puVar3[6 - uVar1];
    case 5:
      puVar4[5 - uVar1] = puVar3[5 - uVar1];
    case 4:
      puVar4[4 - uVar1] = puVar3[4 - uVar1];
    case 3:
      puVar4[3 - uVar1] = puVar3[3 - uVar1];
    case 2:
      puVar4[2 - uVar1] = puVar3[2 - uVar1];
    case 1:
      puVar4[1 - uVar1] = puVar3[1 - uVar1];
      puVar3 = puVar3 + -uVar1;
      puVar4 = puVar4 + -uVar1;
    }
    switch(uVar2) {
    case 1:
switchD_004e5cfb_caseD_1:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      return _Dst;
    case 2:
switchD_004e5cfb_caseD_2:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      return _Dst;
    case 3:
switchD_004e5cfb_caseD_3:
      *(undefined1 *)((int)puVar4 + 3) = *(undefined1 *)((int)puVar3 + 3);
      *(undefined1 *)((int)puVar4 + 2) = *(undefined1 *)((int)puVar3 + 2);
      *(undefined1 *)((int)puVar4 + 1) = *(undefined1 *)((int)puVar3 + 1);
      return _Dst;
    }
switchD_004e5cfb_caseD_0:
    return _Dst;
  }
  puVar3 = _Dst;
  if (((uint)_Dst & 3) == 0) {
    uVar1 = _Size >> 2;
    uVar2 = _Size & 3;
    if (7 < uVar1) {
      for (; uVar1 != 0; uVar1 = uVar1 - 1) {
        *puVar3 = *(undefined4 *)_Src;
        _Src = (undefined4 *)((int)_Src + 4);
        puVar3 = puVar3 + 1;
      }
      switch(uVar2) {
      case 0:
        return _Dst;
      case 2:
        goto switchD_004e5b75_caseD_2;
      case 3:
        goto switchD_004e5b75_caseD_3;
      }
      goto switchD_004e5b75_caseD_1;
    }
  }
  else {
    switch(_Size) {
    case 0:
      goto switchD_004e5b75_caseD_0;
    case 1:
      goto switchD_004e5b75_caseD_1;
    case 2:
      goto switchD_004e5b75_caseD_2;
    case 3:
      goto switchD_004e5b75_caseD_3;
    default:
      uVar1 = (_Size - 4) + ((uint)_Dst & 3);
      switch((uint)_Dst & 3) {
      case 1:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        *(undefined1 *)((int)_Dst + 1) = *(undefined1 *)((int)_Src + 1);
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)_Dst + 2) = *(undefined1 *)((int)_Src + 2);
        _Src = (void *)((int)_Src + 3);
        puVar3 = (undefined4 *)((int)_Dst + 3);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
        break;
      case 2:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        uVar1 = uVar1 >> 2;
        *(undefined1 *)((int)_Dst + 1) = *(undefined1 *)((int)_Src + 1);
        _Src = (void *)((int)_Src + 2);
        puVar3 = (undefined4 *)((int)_Dst + 2);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
        break;
      case 3:
        uVar2 = uVar1 & 3;
        *(undefined1 *)_Dst = *(undefined1 *)_Src;
        _Src = (void *)((int)_Src + 1);
        uVar1 = uVar1 >> 2;
        puVar3 = (undefined4 *)((int)_Dst + 1);
        if (7 < uVar1) {
          for (; uVar1 != 0; uVar1 = uVar1 - 1) {
            *puVar3 = *(undefined4 *)_Src;
            _Src = (undefined4 *)((int)_Src + 4);
            puVar3 = puVar3 + 1;
          }
          switch(uVar2) {
          case 0:
            return _Dst;
          case 2:
            goto switchD_004e5b75_caseD_2;
          case 3:
            goto switchD_004e5b75_caseD_3;
          }
          goto switchD_004e5b75_caseD_1;
        }
      }
    }
  }
  switch(uVar1) {
  case 7:
    puVar3[uVar1 - 7] = *(undefined4 *)((int)_Src + (uVar1 - 7) * 4);
  case 6:
    puVar3[uVar1 - 6] = *(undefined4 *)((int)_Src + (uVar1 - 6) * 4);
  case 5:
    puVar3[uVar1 - 5] = *(undefined4 *)((int)_Src + (uVar1 - 5) * 4);
  case 4:
    puVar3[uVar1 - 4] = *(undefined4 *)((int)_Src + (uVar1 - 4) * 4);
  case 3:
    puVar3[uVar1 - 3] = *(undefined4 *)((int)_Src + (uVar1 - 3) * 4);
  case 2:
    puVar3[uVar1 - 2] = *(undefined4 *)((int)_Src + (uVar1 - 2) * 4);
  case 1:
    puVar3[uVar1 - 1] = *(undefined4 *)((int)_Src + (uVar1 - 1) * 4);
    _Src = (void *)((int)_Src + uVar1 * 4);
    puVar3 = puVar3 + uVar1;
  }
  switch(uVar2) {
  case 1:
switchD_004e5b75_caseD_1:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    return _Dst;
  case 2:
switchD_004e5b75_caseD_2:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)_Src + 1);
    return _Dst;
  case 3:
switchD_004e5b75_caseD_3:
    *(undefined1 *)puVar3 = *(undefined1 *)_Src;
    *(undefined1 *)((int)puVar3 + 1) = *(undefined1 *)((int)_Src + 1);
    *(undefined1 *)((int)puVar3 + 2) = *(undefined1 *)((int)_Src + 2);
    return _Dst;
  }
switchD_004e5b75_caseD_0:
  return _Dst;
}


