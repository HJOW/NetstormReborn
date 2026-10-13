// ==== FUN_004e5150 @ 004e5150

undefined4 __fastcall FUN_004e5150(int param_1)

{
  int iVar1;
  
  if ((*(byte *)(param_1 + 0x23) & 0x20) == 0) {
    iVar1 = FUN_0040d7b0((param_1 - DAT_005395dc) / 0x24);
    if (iVar1 == 0) {
      return 0;
    }
  }
  return 1;
}


// ==== FUN_0040d7b0 @ 0040d7b0

undefined4 FUN_0040d7b0(int param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = DAT_005395dc + param_1 * 0x24;
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x200000) != 0) {
    iVar2 = FUN_004aed30();
    if ((int)*(short *)(iVar4 + 0x1a) < iVar2 / 2) {
      return 1;
    }
    iVar2 = __ftol();
    iVar3 = __ftol();
    if ((*(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48) & 6) == 0) {
      return 1;
    }
    if ((*(byte *)(iVar4 + 0x23) & 0x20) != 0) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_0040ccd0 @ 0040ccd0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0040ccd0(int *param_1,int param_2,int param_3,float param_4)

{
  short sVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  float10 fVar5;
  
  if (param_2 == 0x259) {
    if (param_3 != 0) {
      (**(code **)(*param_1 + 0x10))(0x100000);
      return (float10)_DAT_005003a4;
    }
    return (float10)_DAT_005003a8;
  }
  if (param_2 == 0x25a) {
    sVar1 = *(short *)((int)param_1 + 0x1a);
    iVar2 = FUN_004aed30();
    if (sVar1 < iVar2) {
      iVar2 = FUN_00444720();
      sVar1 = *(short *)((int)param_1 + 0x1a);
      iVar3 = FUN_004aed30();
      if (iVar3 < (int)sVar1 + iVar2 / 6) {
        iVar2 = FUN_004aed30();
      }
      else {
        iVar2 = FUN_00444720();
        iVar2 = iVar2 / 6 + (int)*(short *)((int)param_1 + 0x1a);
      }
      FUN_0040ca40(iVar2);
      (**(code **)(*param_1 + 0x88))();
      FUN_00462480();
    }
    iVar2 = FUN_0040c400();
    if (iVar2 != 0) {
      uVar4 = FUN_0047d020(*(undefined1 *)((int)param_1 + 10),0,0);
      iVar2 = FUN_004eb310(((int)param_1 - DAT_005395dc) / 0x24,uVar4);
      if (iVar2 == 0) {
        FUN_0040d030();
      }
    }
    return (float10)param_4;
  }
  if (param_2 == 0x25d) {
    uVar4 = FUN_0047d020(*(undefined1 *)((int)param_1 + 10),0,0);
    iVar2 = FUN_004eb310(((int)param_1 - DAT_005395dc) / 0x24,uVar4);
    iVar3 = FUN_004e5150();
    if ((iVar3 == 0) && (iVar2 == 0)) {
      if (param_3 == 0x3c) {
        if (DAT_00540a2c != 0) {
          iVar2 = FUN_0040ccb0(DAT_0051cbe4);
          if (iVar2 == 0) {
            FUN_004e4000(((int)param_1 - DAT_005395dc) / 0x24);
            FUN_004ac4d0(DAT_0051cbe4,0,1,0,0);
          }
        }
        uVar4 = 0;
        iVar2 = FUN_004abf50(0);
        FUN_004acbc0(*(undefined4 *)(iVar2 + 0x118),uVar4);
        return (float10)_DAT_00500390;
      }
      return (float10)param_4;
    }
    return (float10)_DAT_00500390;
  }
  if (param_2 == 0x25b) {
    fVar5 = (float10)FUN_0040c430();
    return fVar5;
  }
  if (param_2 == 0x25c) {
    fVar5 = (float10)FUN_0040c760();
    return fVar5;
  }
  fVar5 = (float10)FUN_004e4900(param_2,param_3,param_4);
  return fVar5;
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __fastcall FUN_004abae0(int param_1)

{
  return *(undefined4 *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004aed30 @ 004aed30

undefined4 __fastcall FUN_004aed30(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x400000) == 0) {
    uVar2 = FUN_00444720();
    return uVar2;
  }
  if ((*(byte *)(param_1 + 0x23) & 1) == 0) {
    return 0;
  }
  uVar2 = FUN_00444720();
  return uVar2;
}


// ==== FUN_00444720 @ 00444720

int __fastcall FUN_00444720(int *param_1)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*param_1 + (*param_1 >> 0x1f & 3U)) >> 2;
    }
  }
  return *param_1;
}


// ==== FUN_004448e0 @ 004448e0

int __fastcall FUN_004448e0(int param_1)

{
  return (param_1 - DAT_0051c960) / 0x1d4;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_0040ca40 @ 0040ca40

void __thiscall FUN_0040ca40(int *param_1,undefined4 param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = FUN_0040c3b0();
  sVar1 = *(short *)((int)param_1 + 0x1a);
  FUN_004aed20(param_2);
  iVar3 = FUN_0040c3b0();
  if (iVar3 != iVar2) {
    if (DAT_00540a2c != 0) {
      if ((iVar2 != 0) && (*(int *)((uint)*(byte *)(param_1 + 8) * 0xac + 0x74 + DAT_0050f834) == 0)
         ) {
        FUN_004aed20((int)sVar1);
        return;
      }
      (**(code **)(*param_1 + 0x48))(0);
      iVar2 = FUN_0040c3b0();
      if (iVar2 != 0) {
        FUN_00440a80();
      }
      (**(code **)(*param_1 + 0x4c))(0);
      return;
    }
    if (iVar2 != 0) {
      iVar2 = FUN_004aed30();
      FUN_004aed20(iVar2 / 2 + -1);
      return;
    }
    iVar2 = FUN_004aed30();
    FUN_004aed20(iVar2 / 2);
  }
  return;
}


// ==== FUN_004aed20 @ 004aed20

void __thiscall FUN_004aed20(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0x1a) = param_2;
  return;
}


// ==== FUN_0040c3b0 @ 0040c3b0

bool __fastcall FUN_0040c3b0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_004aed30();
  return (int)*(short *)(param_1 + 0x1a) < iVar1 / 2;
}


// ==== FUN_00440a80 @ 00440a80

void __fastcall FUN_00440a80(float *param_1)

{
  int iVar1;
  
  iVar1 = __ftol();
  *param_1 = (float)iVar1;
  iVar1 = __ftol();
  param_1[1] = (float)iVar1;
  return;
}


