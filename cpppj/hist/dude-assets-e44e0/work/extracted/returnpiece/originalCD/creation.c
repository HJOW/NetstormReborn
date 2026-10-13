// ==== FUN_0043e930 @ 0043e930

void FUN_0043e930(uint param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_1c [4];
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_1c[3] = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0043ea6f;
  *unaff_FS_OFFSET = local_1c + 3;
  if (DAT_0051b630 == DAT_0051ca8c) {
    uVar1 = FUN_0048cc10(0,10000);
    uVar1 = FUN_0041fc70(uVar1);
    FUN_004ac4d0(DAT_0051b630,uVar1,param_1 & 0xfffffeff,0x10,0);
  }
  else {
    param_1 = param_1 & 0xfffffeff;
    if (DAT_0052e918 == 0) {
      param_1 = param_1 | 0x100;
    }
    param_3 = param_3 + -1;
    if (param_3 < 1) {
      param_3 = 0;
    }
    iVar2 = FUN_004ac4d0(DAT_0051b630,0,param_1,0x10,param_3);
    if (DAT_0052e918 == 0) {
      local_1c[0] = 0x41200000;
      local_1c[1] = 0x40a00000;
      local_1c[2] = 0x3f800000;
      iVar3 = DAT_005395dc + iVar2 * 0x24;
      iVar2 = FUN_004f1650(0x28);
      local_8 = 0;
      if (iVar2 != 0) {
        if (DAT_00540a1c == 0) {
          local_1c[2] = local_1c[DAT_00565b00._1_1_];
        }
        else {
          local_1c[2] = 0x38d1b717;
        }
        FUN_0048efd0(0,(iVar3 - DAT_005395dc) / 0x24,local_1c[2]);
      }
    }
  }
  *unaff_FS_OFFSET = local_1c[3];
  return;
}


// ==== FUN_004ac4d0 @ 004ac4d0

int __thiscall
FUN_004ac4d0(int param_1,undefined2 param_2,undefined4 param_3,int param_4,uint param_5,
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
  piVar2 = (int *)FUN_004ab390(DAT_0051c978,uVar1);
  *(undefined2 *)((int)piVar2 + 0x12) = param_2;
  FUN_004abb00(param_3);
  *(short *)(piVar2 + 5) = (short)param_4;
  param_1 = param_1 - DAT_005395dc;
  *(undefined1 *)((int)piVar2 + 0x1a) = param_6;
  (**(code **)(*piVar2 + 0x50))(param_1 / 0x24,param_5);
  return ((int)piVar2 - DAT_005395dc) / 0x24;
}


// ==== FUN_0048cc10 @ 0048cc10

int FUN_0048cc10(int param_1,int param_2)

{
  if (DAT_005308ec == 0) {
    DAT_005308ec = 0xbad0bad;
  }
  DAT_005308ec = DAT_005308ec * 0x10003 + 3;
  return param_1 + (DAT_005308ec >> 0x10 & 0xffffU) % (uint)(param_2 - param_1);
}


// ==== FUN_0041fc70 @ 0041fc70

int FUN_0041fc70(int param_1)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
  if (DAT_00516618 == 0) {
    piVar1 = &DAT_00514a80;
    DAT_00516618 = 0;
    do {
      DAT_00516618 = DAT_00516618 + *piVar1;
      piVar1 = piVar1 + 0x12;
    } while (piVar1 < &DAT_005151d0);
  }
  iVar3 = 0;
  iVar2 = 0;
  piVar1 = &DAT_00514a80;
  do {
    iVar3 = iVar3 + *piVar1;
    if (param_1 % DAT_00516618 <= iVar3) {
      return iVar2;
    }
    piVar1 = piVar1 + 0x12;
    iVar2 = iVar2 + 1;
  } while (piVar1 < &DAT_005151d0);
  return iVar2;
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


// ==== FUN_004ab390 @ 004ab390

int * FUN_004ab390(int param_1,uint param_2)

{
  code *pcVar1;
  int iVar2;
  int *piVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 uVar6;
  
  iVar2 = FUN_004aae40(param_2);
  if ((param_2 & 1) == 0) {
    if (iVar2 < 14000) goto LAB_004ab3e2;
    uVar6 = 0x18b;
    pcVar5 = s__ns_o_Squid_cpp_005398f0;
    pcVar4 = s_sid_<_FIRST_PREDICTABLE_SID_00539900;
  }
  else {
    if (13999 < iVar2) goto LAB_004ab3e2;
    uVar6 = 0x188;
    pcVar5 = s__ns_o_Squid_cpp_005398c0;
    pcVar4 = s_sid_>__FIRST_PREDICTABLE_SID_005398d0;
  }
  FUN_00490040(pcVar4,pcVar5,uVar6);
LAB_004ab3e2:
  pcVar1 = *(code **)(param_1 * 0x1d4 + 0x1b0 + DAT_0051c960);
  if (pcVar1 == (code *)0x0) {
    piVar3 = (int *)(DAT_005395dc + iVar2 * 0x24);
    if (piVar3 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      *piVar3 = (int)&PTR_LAB_005058d8;
    }
  }
  else {
    piVar3 = (int *)(*pcVar1)(iVar2);
  }
  *(char *)((int)piVar3 + 10) = (char)param_1;
  (**(code **)(*piVar3 + 0x28))();
  return piVar3;
}


// ==== FUN_004afe00 @ 004afe00

undefined4 * FUN_004afe00(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)(DAT_005395dc + param_1 * 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = &PTR_LAB_00505b00;
    return puVar1;
  }
  return (undefined4 *)0x0;
}


// ==== FUN_004b0470 @ 004b0470

void FUN_004b0470(void)

{
  return;
}


