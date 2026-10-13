// ==== FUN_00449a20 @ 00449a20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_00449a20(int *param_1,int param_2,undefined4 param_3,float param_4)

{
  undefined1 uVar1;
  byte bVar2;
  float fVar3;
  float fVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  undefined1 uVar9;
  int iVar10;
  undefined4 unaff_ESI;
  float10 fVar11;
  int iStack_cc;
  
  iVar6 = DAT_00540a2c;
  if (param_2 == 0x2691) {
    if (DAT_00540a2c == 0) {
      fVar11 = (float10)_DAT_005019e0;
    }
    else {
      (**(code **)(*param_1 + 0x10))(0);
      fVar11 = (float10)_DAT_005019dc;
    }
  }
  else if (param_2 == 0x2692) {
    if (DAT_00540a2c == 0) {
      fVar11 = (float10)_DAT_005019e0;
    }
    else {
      uVar5 = __ftol();
      if ((DAT_0052e928 == 0) && (iVar6 != 0)) {
        fVar3 = *(float *)((int)param_1 + 0x12);
        iVar10 = 0;
        fVar4 = *(float *)((int)param_1 + 0xe);
        iVar6 = FUN_004abf50();
        iVar7 = FUN_004ae3f0();
        if (iVar7 == 0x4a) {
          if ((float)((int)uVar5 >> 8) <= fVar3) {
            iVar10 = 0x4c;
          }
          else {
            iVar10 = 0x4e;
          }
        }
        else if ((iVar7 == 0x4b) && (iVar10 = 0x4d, (float)(uVar5 & 0xff) <= fVar4)) {
          iVar10 = 0x4f;
        }
        if (iVar10 != 0) {
          uVar1 = *(undefined1 *)((int)param_1 + 0x22);
          uVar9 = (undefined1)*(undefined4 *)(iVar6 + 0x2c + iVar10 * 4);
          *(undefined1 *)((int)param_1 + 0x22) = uVar9;
          FUN_004ebad0(((int)param_1 - DAT_005395dc) / 0x24,0);
          *(undefined1 *)((int)param_1 + 0x22) = uVar1;
          if (iStack_cc == 0) {
            (**(code **)(*param_1 + 0x10))(0);
            return (float10)_DAT_005019dc;
          }
          *(ushort *)(param_1 + 3) =
               ((short)DAT_0051f05c * 8 - 8U ^ *(ushort *)(param_1 + 3)) & 0x78 ^
               *(ushort *)(param_1 + 3);
          FUN_00448c10(((int)param_1 - DAT_005395dc) / 0x24);
          if ((*(byte *)(*(int *)(iVar6 + 0x124) + 3 + (uint)*(byte *)((int)param_1 + 0x22) * 4) &
              0x40) == 0) {
            piVar8 = (int *)FUN_004ab390(*(undefined1 *)((int)param_1 + 10),0);
            *(undefined1 *)((int)piVar8 + 0x22) = uVar9;
            (**(code **)(*piVar8 + 0x74))((char)param_1[8]);
            FUN_004abb00((short)param_1[3]);
            bVar2 = *(byte *)((int)param_1 + 0x23);
            *(short *)(piVar8 + 2) = (short)param_1[2];
            iVar6 = *param_1;
            fVar3 = *(float *)((int)param_1 + 0xe);
            *(byte *)((int)piVar8 + 0x23) =
                 (bVar2 ^ *(byte *)((int)piVar8 + 0x23)) & 0x10 ^ *(byte *)((int)piVar8 + 0x23);
            (**(code **)(iVar6 + 0x10))(0);
            (**(code **)(*piVar8 + 0x90))(unaff_ESI,fVar3,0);
          }
        }
      }
      fVar11 = (float10)_DAT_005019dc;
    }
  }
  else {
    fVar11 = (float10)param_4;
  }
  return fVar11;
}


// ==== FUN_004abf50 @ 004abf50

int __fastcall FUN_004abf50(int param_1)

{
  return DAT_0051c960 + (uint)*(byte *)(param_1 + 10) * 0x1d4;
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __fastcall FUN_004ae3f0(int param_1)

{
  if (*(byte *)(param_1 + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)(param_1 + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)(param_1 + 0x22) * 4);
}


// ==== FUN_004abb00 @ 004abb00

void __thiscall FUN_004abb00(int param_1,undefined2 param_2)

{
  *(undefined2 *)(param_1 + 0xc) = param_2;
  if (*(byte *)(param_1 + 10) == DAT_0051ca8c) {
    FUN_00448c10((param_1 - DAT_005395dc) / 0x24);
  }
  return;
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


// ==== FUN_00444da0 @ 00444da0

void __fastcall FUN_00444da0(int param_1)

{
  int *piVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  
  iVar2 = 0;
  piVar1 = (int *)(param_1 + 0x130);
  *(undefined4 *)(param_1 + 300) = 0;
  do {
    iVar4 = 0;
    *piVar1 = -1;
    piVar1[0x10] = 0;
    pcVar3 = *(char **)(param_1 + 0x124);
    if (0 < *(int *)(param_1 + 0x114)) {
      do {
        if (*pcVar3 - iVar2 == 0x41) {
          if (*piVar1 == -1) {
            *(int *)(param_1 + 300) = *(int *)(param_1 + 300) + 1;
            *piVar1 = iVar4;
          }
          piVar1[0x10] = piVar1[0x10] + 1;
        }
        iVar4 = iVar4 + 1;
        pcVar3 = pcVar3 + 4;
      } while (iVar4 < *(int *)(param_1 + 0x114));
    }
    piVar1 = piVar1 + 1;
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x10);
  return;
}


