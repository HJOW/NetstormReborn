// ==== FUN_00443110 @ 00443110

int __thiscall FUN_00443110(void *this)

{
  int iVar1;
  
  if ((*(byte *)((int)this + 10) == DAT_0051cb74) && ((*(byte *)((int)this + 0x23) & 8) == 0)) {
    iVar1 = FUN_004adbc0();
    return iVar1 * 2;
  }
  iVar1 = FUN_004ae3f0();
  iVar1 = iVar1 * 2 + -0x98;
  if ((iVar1 < 0) || (7 < iVar1)) {
    iVar1 = 0;
  }
  return iVar1;
}


// ==== FUN_004adbc0 @ 004adbc0

undefined1 __thiscall FUN_004adbc0(void *this)

{
  return *(undefined1 *)((int)this + 0x1c);
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __thiscall FUN_004ae3f0(void *this)

{
  if (*(byte *)((int)this + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)((int)this + 0x22) * 4);
}


// ==== FUN_0042b230 @ 0042b230

void __thiscall FUN_0042b230(void *this,undefined4 param_2)

{
  FUN_004ace20(this,param_2);
  (**(code **)(*(int *)this + 0x88))();
  return;
}


// ==== FUN_0047ac20 @ 0047ac20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 __thiscall FUN_0047ac20(void *this,undefined4 param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  undefined4 uVar6;
  float10 fVar7;
  
  switch(param_2) {
  case 1:
    goto LAB_0047ade7;
  case 2:
    iVar4 = (**(code **)(*(int *)this + 0x80))();
    iVar1 = FUN_004abf50(this);
    iVar2 = (**(code **)(*(int *)this + 0x80))();
    iVar3 = FUN_004abf50(this);
    uVar6 = 0;
    iVar2 = FUN_0048cbd0(*(undefined4 *)(iVar3 + 0x170 + iVar2 * 4),0);
    FUN_004acbc0(this,*(int *)(iVar1 + 0x130 + iVar4 * 4) + iVar2,uVar6);
    break;
  case 3:
    if (param_3 != 0) {
      FUN_0048ca70(3,s_Upgrading__0052d828);
      FUN_0047a320(this);
      iVar4 = FUN_004d71a0(this);
      if (iVar4 != 0) {
        FUN_004d8910(*(undefined1 *)((int)this + 0x20),((int)this - DAT_005395dc) / 0x24);
      }
      return (float10)_DAT_00503288;
    }
    piVar5 = (int *)FUN_004ab390(*(undefined1 *)((int)this + 10),2);
    uVar6 = FUN_0047a1c0(this);
    (**(code **)(*piVar5 + 0x84))(uVar6);
    *(byte *)((int)piVar5 + 0x23) = *(byte *)((int)piVar5 + 0x23) | 8;
    if ((*(byte *)((int)piVar5 + 0xb) & 4) == 0) {
      FUN_00490040(s_isVoid___00514a24,s____o_squid_h_00514a30,0x268);
    }
    *(byte *)((int)piVar5 + 0x23) = *(byte *)((int)piVar5 + 0x23) | 1;
    FUN_004abb00(piVar5,0);
    (**(code **)(*piVar5 + 0x74))(*(undefined1 *)((int)this + 0x20));
    (**(code **)(*piVar5 + 0x90))
              (*(undefined4 *)((int)this + 0xe),*(float *)((int)this + 0x12) + _DAT_0050328c,0);
    FUN_0048ca70(4,s_making_the_client_upgrade_pictur_0052d804);
    FUN_004d06a0(((int)piVar5 - DAT_005395dc) / 0x24,param_4);
    FUN_004af940(((int)this - DAT_005395dc) / 0x24,((int)piVar5 - DAT_005395dc) / 0x24);
LAB_0047ade7:
    FUN_004acce0(this,1,0);
    break;
  case 4:
    if (param_3 != 0) {
      return (float10)_DAT_00503290;
    }
  }
  fVar7 = (float10)FUN_00461a40(this,param_2,param_3,param_4);
  return fVar7;
}


// ==== FUN_00456c10 @ 00456c10

void __cdecl
FUN_00456c10(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,int param_6,int param_7,int param_8,int param_9)

{
  int iVar1;
  int local_10;
  int local_c;
  int local_8;
  int local_4;
  
  if (DAT_0052039c == 0) {
    FUN_00456930(&local_10,param_1,param_2,param_3,param_4);
    iVar1 = local_10 + param_6;
    if (DAT_00583e28 <= local_10 + param_6) {
      iVar1 = DAT_00583e28;
    }
    local_10 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_10 = DAT_00583e20;
    }
    iVar1 = local_c + param_7;
    if (DAT_00583e2c <= local_c + param_7) {
      iVar1 = DAT_00583e2c;
    }
    local_c = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_c = DAT_00583e24;
    }
    iVar1 = local_8 - param_8;
    if (DAT_00583e28 <= local_8 - param_8) {
      iVar1 = DAT_00583e28;
    }
    local_8 = iVar1;
    if (iVar1 <= DAT_00583e20) {
      local_8 = DAT_00583e20;
    }
    iVar1 = local_4 - param_9;
    if (DAT_00583e2c <= local_4 - param_9) {
      iVar1 = DAT_00583e2c;
    }
    local_4 = iVar1;
    if (iVar1 <= DAT_00583e24) {
      local_4 = DAT_00583e24;
    }
    if ((local_8 != local_10 && -1 < local_8 - local_10) &&
       (local_4 != local_c && -1 < local_4 - local_c)) {
      FUN_00457320(DAT_005203a8,&local_10,param_5);
    }
  }
  return;
}


