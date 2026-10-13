// ==== FUN_0048fdb0 @ 0048fdb0

undefined4 FUN_0048fdb0(uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  int *piVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_24;
  undefined4 *local_18;
  undefined4 local_14;
  int local_10;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_004fb1b8;
  local_c = ExceptionList;
  piVar5 = (int *)(param_1 * 0xb4 + DAT_0059534c);
  ExceptionList = &local_c;
  if (piVar5 == (int *)0x0) {
    ExceptionList = &local_c;
    FUN_004e0620(&DAT_005078ac,"\\Ns\\O\\Player.cpp",0x2d1);
  }
  iVar2 = FUN_00462b80(param_3,param_4);
  if (iVar2 == DAT_00540410) {
    ExceptionList = local_c;
    return 0;
  }
  local_18 = operator_new(0xa0);
  iVar8 = 0;
  local_14 = 0x28;
  local_10 = 0;
  local_4 = 0;
  bVar1 = false;
  local_24 = 0;
  if (0 < piVar5[2]) {
    do {
      iVar6 = *(int *)(*piVar5 + local_24 * 4) * 0x32 + DAT_005c8464;
      iVar3 = FUN_0049a840(param_2);
      if ((*(byte *)(iVar3 + 0xec) & 6) == 0) {
        iVar3 = FUN_004ac9f0(0x10,param_2,0);
        if (iVar3 != 0) {
          bVar1 = true;
          goto LAB_0048feac;
        }
        if (DAT_0054db14 != 0) goto LAB_0048feac;
      }
      else {
LAB_0048feac:
        iVar3 = FUN_00462bc0((iVar6 - DAT_005c8464) / 0x32);
        if ((iVar3 == iVar2) && (iVar8 < 0x28)) {
          local_18[iVar8] = (iVar6 - DAT_005c8464) / 0x32;
          iVar8 = iVar8 + 1;
          local_10 = iVar8;
        }
      }
      local_24 = local_24 + 1;
    } while (local_24 < piVar5[2]);
    if ((iVar8 != 0) || (bVar1)) {
      iVar3 = 0;
      if (0 < DAT_0055a714) {
        do {
          iVar6 = *(int *)(DAT_0055a70c + iVar3 * 4) * 0x32;
          iVar7 = iVar6 + DAT_005c8464;
          if (((*(byte *)(iVar6 + 0x22 + DAT_005c8464) == param_1) &&
              (iVar6 = FUN_00462bc0((iVar7 - DAT_005c8464) / 0x32), iVar6 == iVar2)) &&
             (iVar8 < 0x28)) {
            local_18[iVar8] = (iVar7 - DAT_005c8464) / 0x32;
            iVar8 = iVar8 + 1;
            local_10 = iVar8;
          }
          iVar3 = iVar3 + 1;
        } while (iVar3 < DAT_0055a714);
      }
      if (iVar8 != 0) {
        if (iVar8 != 1) {
          uVar4 = FUN_0048fd30(param_3,param_4,&local_18,param_1);
          local_4 = 0xffffffff;
          _free(local_18);
          ExceptionList = local_c;
          return uVar4;
        }
        uVar4 = *local_18;
        local_4 = 0xffffffff;
        _free(local_18);
        ExceptionList = local_c;
        return uVar4;
      }
    }
  }
  local_4 = 0xffffffff;
  _free(local_18);
  ExceptionList = local_c;
  return 0;
}


// ==== FUN_0048fd30 @ 0048fd30

undefined4 FUN_0048fd30(undefined4 param_1,undefined4 param_2,int *param_3,uint param_4)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  float10 fVar4;
  float local_4;
  
  uVar2 = 0;
  iVar3 = 0;
  local_4 = 9999.0;
  if (0 < param_3[2]) {
    do {
      iVar1 = *(int *)(*param_3 + iVar3 * 4) * 0x32 + DAT_005c8464;
      if ((*(byte *)(iVar1 + 0x22) == param_4) &&
         (fVar4 = (float10)FUN_0041cfc0(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12)),
         fVar4 < (float10)local_4)) {
        local_4 = (float)fVar4;
        uVar2 = *(undefined4 *)(*param_3 + iVar3 * 4);
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < param_3[2]);
  }
  return uVar2;
}


// ==== FUN_00462b80 @ 00462b80

undefined4 FUN_00462b80(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  if (DAT_00540414 != 0) {
    iVar1 = FUN_0040eaf0(param_1,param_2);
    if (iVar1 != 0) {
      uVar2 = FUN_004ad390();
      return uVar2;
    }
  }
  return 0xfe;
}


// ==== FUN_00462bc0 @ 00462bc0

undefined4 FUN_00462bc0(int param_1)

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
  uVar2 = FUN_004ac1e0();
  if ((uVar2 & 0x800) != 0) {
    uVar3 = FUN_004ad390();
    return uVar3;
  }
  uVar3 = FUN_00462b80(*(undefined4 *)(iVar1 + 0xe),*(undefined4 *)(iVar1 + 0x12));
  return uVar3;
}


// ==== FUN_0049a840 @ 0049a840

int FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong FUN_004e49c0(void)

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


// ==== FUN_0041cfc0 @ 0041cfc0

float10 __thiscall FUN_0041cfc0(float *param_1,float param_2,float param_3)

{
  return SQRT(((float10)param_1[1] - (float10)param_3) * ((float10)param_1[1] - (float10)param_3) +
              ((float10)*param_1 - (float10)param_2) * ((float10)*param_1 - (float10)param_2));
}


// ==== FUN_004ad390 @ 004ad390

undefined1 __fastcall FUN_004ad390(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (*(byte *)(param_1 + 10) != DAT_005411c0)) &&
     (*(byte *)(param_1 + 10) != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x739);
  }
  return *(undefined1 *)(param_1 + 0x1e);
}


// ==== FUN_004ac1e0 @ 004ac1e0

undefined4 __fastcall FUN_004ac1e0(int param_1)

{
  int iVar1;
  
  iVar1 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  return *(undefined4 *)(iVar1 + 0xe8);
}


// ==== FUN_0040eaf0 @ 0040eaf0

undefined2 FUN_0040eaf0(void)

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


// ==== FUN_004ac9f0 @ 004ac9f0

int __thiscall FUN_004ac9f0(int param_1,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  int local_8;
  
  FUN_004b2270((param_1 - DAT_005c8464) / 0x32);
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
    FUN_004b1b20();
  } while( true );
}


// ==== FUN_004b2270 @ 004b2270

void __thiscall FUN_004b2270(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = param_2 * 0x32 + DAT_005c8464;
  if ((((DAT_005e4794 != 0) && ((*(byte *)(iVar1 + 0xb) & 2) != 0)) && (DAT_00540bc4 == 0)) &&
     (*(int *)(param_1 + 0xc) == 0)) {
    FUN_004e0620("!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy",
                 "\\Ns\\O\\Squidfinder.cpp",0x13c);
  }
  *(uint *)(param_1 + 4) = (uint)*(ushort *)(iVar1 + 6);
  FUN_004b1b20();
  return;
}


// ==== FUN_004b1b20 @ 004b1b20

void __fastcall FUN_004b1b20(undefined4 *param_1)

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
    } while ((iVar1 != 0) && (*(byte *)(iVar1 + 10) != DAT_00541088));
    iVar1 = (**(code **)*param_1)(iVar1);
  } while (iVar1 == 0);
  return;
}


// ==== FUN_0044daa0 @ 0044daa0

undefined4 FUN_0044daa0(void)

{
  return 1;
}


