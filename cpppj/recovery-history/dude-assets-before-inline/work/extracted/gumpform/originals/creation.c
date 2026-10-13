// ==== FUN_004ae9f0 @ 004ae9f0

undefined4 * FUN_004ae9f0(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(param_1 * 0x32 + DAT_005c8464);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00512188;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004ad010 @ 004ad010

undefined2 __fastcall FUN_004ad010(int param_1)

{
  return *(undefined2 *)(param_1 + 0x10);
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


// ==== FUN_004ae9d0 @ 004ae9d0

void __fastcall FUN_004ae9d0(int param_1)

{
  if (*(int *)(param_1 + 0x12) != 0) {
    FUN_00464190(*(int *)(param_1 + 0x12),*(undefined2 *)(param_1 + 0x10));
  }
  return;
}


// ==== FUN_004ae9b0 @ 004ae9b0

void __fastcall FUN_004ae9b0(int param_1)

{
  if (*(int *)(param_1 + 0x12) != 0) {
    FUN_00464190(*(int *)(param_1 + 0x12),*(undefined2 *)(param_1 + 0x10));
  }
  return;
}


// ==== thunk_FUN_004ac800 @ 004af160

void thunk_FUN_004ac800(void)

{
  FUN_004ac800();
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


// ==== FUN_004ac1c0 @ 004ac1c0

void FUN_004ac1c0(void)

{
  DAT_005c8480 = DAT_005c8480 + -1;
  return;
}


// ==== FUN_004ac1b0 @ 004ac1b0

void FUN_004ac1b0(void)

{
  DAT_005c8484 = DAT_005c8484 + -1;
  return;
}


// ==== FUN_004ac1d0 @ 004ac1d0

void FUN_004ac1d0(void)

{
  DAT_005c8488 = DAT_005c8488 + -1;
  return;
}


// ==== FUN_00464190 @ 00464190

void FUN_00464190(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)(&DAT_00563e38)[param_1];
  if (piVar1 == (int *)0x0) {
    FUN_004e0620(&DAT_0050ac50,".\\Gump.cpp",0x176);
  }
  (**(code **)(*piVar1 + 4))(param_2);
  return;
}


// ==== FUN_00466740 @ 00466740

void FUN_00466740(void)

{
  FUN_00466480();
  return;
}


// ==== FUN_004644e0 @ 004644e0

void __thiscall FUN_004644e0(int param_1,int param_2)

{
  int *piVar1;
  
  if (param_2 != 0) {
    piVar1 = (int *)FUN_004af530(DAT_0054107c,2);
    *(undefined4 *)((int)piVar1 + 0x12) = *(undefined4 *)(param_1 + 0x1c);
    (**(code **)(*piVar1 + 0x74))(param_2,0);
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) | 0x40;
    *(int *)(param_1 + 0x24) = param_2;
    *(int *)(param_1 + 0x28) = ((int)piVar1 - DAT_005c8464) / 0x32;
  }
  return;
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
    *piVar3 = (int)&PTR_LAB_00501dd0;
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


