// ==== FUN_004b0d30 @ 004b0d30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004b0d30(int param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  
  if (DAT_00594fc8 != 0) {
    DAT_005c8488 = DAT_005c8488 + -1;
    return;
  }
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  if ((*(uint *)(iVar2 + 0xec) & (DAT_005424b8 | 8)) != 0) {
    if (DAT_00540414 == 0) goto LAB_004b0e4c;
    if (((param_2 & 3) != 0) && ((param_2 & 0x2000000) == 0)) {
      FUN_00462d40((param_1 - DAT_005c8464) / 0x32);
    }
  }
  if (DAT_00540414 != 0) {
    if (((param_2 & 0x203) == 0) || ((param_2 & 0x2000000) != 0)) {
      iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
      if (((*(uint *)(iVar2 + 0xe8) & 0x800) != 0) &&
         (((param_2 & 1) != 0 && ((param_2 & 0x2000000) != 0)))) {
        FUN_004ad3e0(DAT_00540410);
      }
    }
    else {
      if ((param_2 & 0x800) != 0) {
        FUN_004e0620("!(popFlags&pfDESTROY_GRAPH)","\\Ns\\O\\Squid.cpp",0x80b);
      }
      iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
      if ((*(uint *)(iVar2 + 0xe8) & 0x800) != 0) {
        FUN_004633c0((param_1 - DAT_005c8464) / 0x32);
      }
    }
  }
LAB_004b0e4c:
  iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
  uVar1 = *(uint *)(iVar2 + 0xec);
  if (((param_2 & 1) != 0) && ((*(byte *)(param_1 + 0x28) & 9) == 0)) {
    _DAT_0059508c = FUN_004e49c0();
    iVar2 = FUN_0049a840(*(undefined1 *)(param_1 + 10));
    if ((*(uint *)(iVar2 + 0xe8) & 0x10000000) != 0) {
      FUN_00472f70((param_1 - DAT_005c8464) / 0x32);
    }
    if ((uVar1 & 0x4200) != 0) {
      FUN_00490050(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32);
      FUN_0044f770((param_1 - DAT_005c8464) / 0x32);
    }
    FUN_004166e0(*(undefined1 *)(param_1 + 0x22),(param_1 - DAT_005c8464) / 0x32,1);
    if (*(byte *)(param_1 + 0x22) == DAT_00540c70) {
      FUN_004c25e0(*(undefined1 *)(param_1 + 10));
    }
    FUN_004c25b0(*(undefined1 *)(param_1 + 10));
    FUN_004da650();
  }
  DAT_005c8488 = DAT_005c8488 + -1;
  return;
}


// ==== FUN_004c25e0 @ 004c25e0

void FUN_004c25e0(int param_1)

{
  (&DAT_005c94d0)[param_1] = (&DAT_005c94d0)[param_1] + 1;
  (&DAT_005c98d0)[param_1] = (&DAT_005c98d0)[param_1] + 1;
  return;
}


// ==== FUN_004c25b0 @ 004c25b0

void FUN_004c25b0(int param_1)

{
  (&DAT_005c9cd0)[param_1] = (&DAT_005c9cd0)[param_1] + 1;
  return;
}


// ==== FUN_004da650 @ 004da650

void FUN_004da650(void)

{
  int iVar1;
  
  if ((((DAT_005caea4 != 0) &&
       (iVar1 = FUN_004b1fa0(DAT_005caf38,DAT_005caf3c,DAT_005caea4), iVar1 != 0)) &&
      ((DAT_005caea0 == 0 || (DAT_005caea0 == iVar1)))) &&
     (FUN_004d5c90(iVar1,0,1), (*(byte *)(iVar1 * 0x32 + 0x28 + DAT_005c8464) & 9) == 0)) {
    DAT_005caea4 = 0;
  }
  return;
}


// ==== FUN_00490050 @ 00490050

void FUN_00490050(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  if (param_1 != 0) {
    piVar1 = (int *)(param_1 * 0xb4 + DAT_0059534c);
    if (piVar1 == (int *)0x0) {
      FUN_004e0620(&DAT_005078ac,"\\Ns\\O\\Player.cpp",0x313);
      return;
    }
    if (piVar1[2] < piVar1[1]) {
      *(undefined4 *)(*piVar1 + piVar1[2] * 4) = param_2;
      piVar1[2] = piVar1[2] + 1;
    }
  }
  return;
}


// ==== FUN_0044f770 @ 0044f770

void FUN_0044f770(undefined4 param_1)

{
  if (DAT_0055a4dc < DAT_0055a4d8) {
    *(undefined4 *)(DAT_0055a4d4 + DAT_0055a4dc * 4) = param_1;
    DAT_0055a4dc = DAT_0055a4dc + 1;
  }
  DAT_0055a4a4 = DAT_0055a4a4 + 1;
  return;
}


// ==== FUN_00472f70 @ 00472f70

void FUN_00472f70(int param_1)

{
  bool bVar1;
  int iVar2;
  
  iVar2 = 0;
  if (0 < DAT_00592f8c) {
    do {
      if (*(int *)(DAT_00592f84 + iVar2 * 4) == param_1) {
        bVar1 = true;
        goto LAB_00472f94;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00592f8c);
  }
  bVar1 = false;
LAB_00472f94:
  if (DAT_005e4794 == 0) {
LAB_00472fc1:
    if (bVar1) goto LAB_00472fd6;
  }
  else if (bVar1) {
    FUN_004e0620("!alreadyThere","\\Ns\\O\\Mana.cpp",0x44);
    goto LAB_00472fc1;
  }
  if (DAT_00592f8c < DAT_00592f88) {
    *(int *)(DAT_00592f84 + DAT_00592f8c * 4) = param_1;
    DAT_00592f8c = DAT_00592f8c + 1;
  }
LAB_00472fd6:
  DAT_0055a4a4 = DAT_0055a4a4 + 1;
  return;
}


// ==== FUN_004166e0 @ 004166e0

void FUN_004166e0(int param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  
  if ((((DAT_00594fc8 == 0) && (0 < param_1)) && (iVar1 = param_1 * 0xb4 + DAT_0059534c, iVar1 != 0)
      ) && (*(int *)(iVar1 + 0xb0) != 0)) {
    FUN_00414de0(param_2,param_3);
  }
  return;
}


// ==== FUN_004ad3e0 @ 004ad3e0

void __thiscall FUN_004ad3e0(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = (uint)*(byte *)(param_1 + 10);
  iVar1 = FUN_0049a840(uVar2);
  if ((((*(uint *)(iVar1 + 0xe8) & 0x800) == 0) && (uVar2 != DAT_005411c0)) &&
     (uVar2 != DAT_00541288)) {
    FUN_004e0620("getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x74b);
  }
  if (((param_2 < 0) || (DAT_0054040c <= param_2)) &&
     ((param_2 != DAT_00540410 && ((uVar2 != DAT_005411c0 && (uVar2 != DAT_00541288)))))) {
    FUN_004e0620("( n >=0 && n < MaxGraphs) || ( n==invalidGraphNum) || type == fencemarkType || type == windArcherType"
                 ,"\\Ns\\O\\Squid.cpp",0x750);
  }
  *(char *)(param_1 + 0x1e) = (char)param_2;
  return;
}


// ==== FUN_0049c2e0 @ 0049c2e0

void FUN_0049c2e0(void)

{
  undefined4 uVar1;
  int iVar2;
  
  FUN_0049c230();
  DAT_00544cc4 = (FILE *)0x0;
  DAT_0059ab38 = 0;
  FUN_0049c160(PTR_DAT_00540dd0);
  FUN_0040cef0();
  _fclose(DAT_00544cc4);
  if (0 < DAT_00541348) {
    iVar2 = 0;
    do {
      FUN_0049b0d0();
      uVar1 = FUN_004e49c0();
      (&DAT_0059ab50)[iVar2] = uVar1;
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00541348);
  }
  if (DAT_0059ab48 == (void *)0x0) {
    FUN_004e0620("budArray","\\Ns\\O\\Rifttype.cpp",0x548);
  }
  FUN_0049c270();
  _free(DAT_0059ab48);
  DAT_0059ab48 = (void *)0x0;
  return;
}


