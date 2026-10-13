// ==== FUN_00423b20 @ 00423b20

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00423b20(void *this,undefined4 param_2,undefined4 param_3)

{
  void *pvVar1;
  undefined4 local_5064;
  undefined4 uStack_5060;
  undefined4 local_505c;
  undefined4 uStack_5058;
  undefined4 local_5054;
  undefined4 uStack_5050;
  undefined4 local_5044;
  undefined4 uStack_5040;
  undefined4 local_503c;
  undefined4 uStack_5038;
  undefined1 local_502c [20508];
  undefined4 uStack_10;
  void *local_c;
  code *pcStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f95e9;
  local_c = ExceptionList;
  uStack_10 = 0x423b3f;
  ExceptionList = &local_c;
  FUN_00488190(this,param_2,DAT_005410a8);
  local_4 = 0;
  *(undefined ***)this = &PTR_FUN_00503730;
  *(undefined4 *)((int)this + 0x6c) = param_2;
  *(undefined4 *)((int)this + 0x54) = 0;
  *(undefined4 *)((int)this + 0x5c) = 0;
  *(undefined4 *)((int)this + 0x60) = 0;
  *(undefined4 *)((int)this + 100) = 0;
  *(undefined4 *)((int)this + 0x68) = 0;
  *(undefined4 *)((int)this + 0x58) = 0;
  *(undefined4 *)((int)this + 0x7c) = 0;
  *(undefined4 *)((int)this + 0x78) = 0;
  *(undefined4 *)((int)this + 0x80) = 0;
  *(undefined4 *)((int)this + 0x50) = 0;
  FUN_00488100((int)this + 0x24);
  *(undefined4 *)((int)this + 0x70) = param_3;
  if (DAT_005453d0 == 0) {
    pvVar1 = operator_new(8);
    local_4._0_1_ = 1;
    if (pvVar1 == (void *)0x0) {
      DAT_005453d0 = 0;
    }
    else {
      DAT_005453d0 = FUN_00487cb0(pvVar1);
    }
    local_4 = (uint)local_4._1_3_ << 8;
    FUN_004437f0();
    local_5064 = (undefined4)_DAT_00501580;
    uStack_5060 = (undefined4)((ulonglong)_DAT_00501580 >> 0x20);
    local_505c = (undefined4)_DAT_00503728;
    uStack_5058 = (undefined4)((ulonglong)_DAT_00503728 >> 0x20);
    local_5054 = (undefined4)_DAT_00503720;
    uStack_5050 = (undefined4)((ulonglong)_DAT_00503720 >> 0x20);
    local_5044 = (undefined4)_DAT_00503718;
    uStack_5040 = (undefined4)((ulonglong)_DAT_00503718 >> 0x20);
    local_503c = (undefined4)_DAT_00503710;
    uStack_5038 = (undefined4)((ulonglong)_DAT_00503710 >> 0x20);
    FUN_0043ad20(local_502c,local_5064,uStack_5060,local_5044,uStack_5040,local_503c,uStack_5038,
                 local_5064,uStack_5060,local_505c,uStack_5058,local_5054,uStack_5050,8,
                 _DAT_00503678);
    FUN_00487d20(DAT_005453d0,local_502c,0,0);
  }
  ExceptionList = local_c;
  return this;
}


// ==== FUN_00488190 @ 00488190

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall FUN_00488190(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined8 uVar1;
  
  FUN_00487fd0(this,param_2,param_3);
  uVar1 = _DAT_00500460;
  *(undefined4 *)((int)this + 0x24) = 0;
  *(undefined4 *)((int)this + 0x28) = 0;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined8 *)((int)this + 0x30) = uVar1;
  *(undefined8 *)((int)this + 0x38) = _DAT_00500460;
  *(undefined4 *)((int)this + 0x48) = 0;
  *(undefined ***)this = &PTR_FUN_005036e8;
  return this;
}


// ==== FUN_00487fd0 @ 00487fd0

void * __thiscall FUN_00487fd0(void *this,int param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  pcStack_8 = FUN_004fb048;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x14) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined ***)this = &PTR_FUN_0050ddc4;
  local_4 = 0;
  puVar2 = DAT_00595138;
  do {
    DAT_00595138 = puVar2;
    iVar3 = DAT_0059513c;
    iVar1 = 0;
    if (0 < DAT_0059513c) {
      do {
        if (DAT_00595138[iVar1] == 0) {
          DAT_00595138[iVar1] = this;
          *(int *)((int)this + 0x1c) = iVar1;
          *(int *)((int)this + 0x20) =
               (int)*(char *)(*(int *)((int)this + 8) * 0x32 + 0x23 + DAT_005c8464);
          if (param_2 != 0) {
            FUN_0041bd20(this,param_3,param_2,0,0x10);
            *(int *)((int)this + 0x20) = (int)*(char *)(param_2 * 0x32 + 0x23 + DAT_005c8464);
          }
          ExceptionList = local_c;
          return this;
        }
        iVar1 = iVar1 + 1;
      } while (iVar1 < DAT_0059513c);
    }
    DAT_0059513c = DAT_0059513c + 0x28;
    if (DAT_0059513c < 0x28) {
      DAT_0059513c = 0x28;
    }
    puVar2 = operator_new(DAT_0059513c << 2);
    FUN_004437f0();
    puVar5 = puVar2;
    for (iVar1 = DAT_0059513c; puVar4 = DAT_00595138, puVar6 = puVar2, iVar1 != 0;
        iVar1 = iVar1 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    for (; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar6 = puVar6 + 1;
    }
    if (DAT_00595138 != (undefined4 *)0x0) {
      _free(DAT_00595138);
    }
  } while( true );
}


// ==== FUN_00488100 @ 00488100

void __thiscall FUN_00488100(void *this,uint param_2)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  
  if (*(void **)((int)this + 8) != (void *)0x0) {
    _free(*(void **)((int)this + 8));
  }
  param_2 = ((int)param_2 < 0) - 1 & param_2;
  *(uint *)this = param_2;
  puVar1 = operator_new(param_2 * 0x18);
  if (puVar1 == (undefined4 *)0x0) {
    puVar1 = (undefined4 *)0x0;
  }
  else {
    puVar2 = puVar1;
    if (-1 < (int)(param_2 - 1)) {
      do {
        *puVar2 = 0;
        puVar2[1] = 0;
        param_2 = param_2 - 1;
        puVar2 = puVar2 + 6;
      } while (param_2 != 0);
      *(undefined4 **)((int)this + 8) = puVar1;
      FUN_00487ab0(this);
      return;
    }
  }
  *(undefined4 **)((int)this + 8) = puVar1;
  FUN_00487ab0(this);
  return;
}


// ==== FUN_00487ab0 @ 00487ab0

void __thiscall FUN_00487ab0(void *this)

{
  int iVar1;
  undefined4 *puVar2;
  
  if (this == (void *)0x0) {
    FUN_004e0620("this != 0",".\\Particle.cpp",0xce);
  }
  if (*(int *)((int)this + 8) == 0) {
    FUN_004e0620("particles",".\\Particle.cpp",0xcf);
  }
  puVar2 = *(undefined4 **)((int)this + 8);
  for (iVar1 = (*(int *)this * 3 & 0x1fffffffU) << 1; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  for (iVar1 = 0; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined1 *)puVar2 = 0;
    puVar2 = (undefined4 *)((int)puVar2 + 1);
  }
  *(undefined4 *)((int)this + 4) = 0;
  return;
}


// ==== FUN_00487cb0 @ 00487cb0

void __thiscall FUN_00487cb0(void *this)

{
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 4) = 0;
  return;
}


// ==== FUN_00487d20 @ 00487d20

void __thiscall FUN_00487d20(void *this,int param_2,int param_3,int param_4)

{
  int iVar1;
  int iVar2;
  void *pvVar3;
  undefined4 *puVar4;
  
  if (*(void **)((int)this + 4) != (void *)0x0) {
    _free(*(void **)((int)this + 4));
  }
  if (param_3 == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = *(int *)(param_3 + 0x5010);
  }
  if (param_4 == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(param_4 + 0x5010);
  }
  iVar2 = iVar1 + iVar2 + *(int *)(param_2 + 0x5010);
  *(int *)this = iVar2;
  pvVar3 = operator_new(iVar2 * 4);
  *(void **)((int)this + 4) = pvVar3;
  iVar2 = 0;
  if (0 < *(int *)(param_2 + 0x5010)) {
    puVar4 = (undefined4 *)(param_2 + 0x38);
    do {
      *(undefined4 *)(*(int *)((int)this + 4) + iVar2 * 4) = *puVar4;
      iVar2 = iVar2 + 1;
      puVar4 = puVar4 + 0x14;
    } while (iVar2 < *(int *)(param_2 + 0x5010));
  }
  if (param_3 != 0) {
    if (iVar2 < *(int *)(param_3 + 0x5010) + *(int *)(param_2 + 0x5010)) {
      puVar4 = (undefined4 *)(param_3 + 0x38);
      do {
        *(undefined4 *)(*(int *)((int)this + 4) + iVar2 * 4) = *puVar4;
        iVar2 = iVar2 + 1;
        puVar4 = puVar4 + 0x14;
      } while (iVar2 < *(int *)(param_3 + 0x5010) + *(int *)(param_2 + 0x5010));
    }
    if ((param_4 != 0) &&
       (iVar2 < *(int *)(param_3 + 0x5010) + *(int *)(param_4 + 0x5010) + *(int *)(param_2 + 0x5010)
       )) {
      puVar4 = (undefined4 *)(param_4 + 0x38);
      do {
        *(undefined4 *)(*(int *)((int)this + 4) + iVar2 * 4) = *puVar4;
        iVar2 = iVar2 + 1;
        puVar4 = puVar4 + 0x14;
      } while (iVar2 < *(int *)(param_3 + 0x5010) + *(int *)(param_4 + 0x5010) +
                       *(int *)(param_2 + 0x5010));
    }
  }
  if (*(int *)this != iVar2) {
    FUN_004e0620("totalColors == i",".\\Particle.cpp",0x17b);
  }
  return;
}


// ==== FUN_0043ad20 @ 0043ad20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall
FUN_0043ad20(void *this,double param_2,double param_3,double param_4,double param_5,double param_6,
            double param_7,undefined4 param_8,undefined8 param_9)

{
  double dVar1;
  double dVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *local_6c;
  double local_68;
  undefined4 local_58 [6];
  double local_40;
  double local_38;
  double local_30;
  
  FUN_0043a8e0();
  *(undefined8 *)((int)this + 0x5018) = param_9;
  local_68 = _DAT_00506690;
  *(undefined4 *)((int)this + 0x5010) = param_8;
  dVar2 = _DAT_00506690;
  local_6c = (undefined4 *)((int)this + 0x58);
  *(undefined ***)this = &PTR_LAB_00506688;
  *(undefined4 *)((int)this + 0x500c) = 0xffffffff;
  *(undefined4 *)((int)this + 0x5008) = 0xffffffff;
  iVar4 = 2;
  do {
    puVar5 = local_6c + -0x14;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + -2;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + -2;
      dVar2 = dVar1;
    }
    puVar5 = local_6c;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + -1;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + -1;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 0x14;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 0x28;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + 1;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + 1;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 0x3c;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + 2;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + 2;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 0x50;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + 3;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + 3;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 100;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + 4;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + 4;
      dVar2 = dVar1;
    }
    puVar5 = local_6c + 0x78;
    puVar6 = local_58;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    dVar1 = (local_30 - param_4) * (local_30 - param_4) +
            (local_38 - param_3) * (local_38 - param_3) +
            (local_40 - param_2) * (local_40 - param_2);
    if (dVar1 < local_68) {
      *(int *)((int)this + 0x5008) = iVar4 + 5;
      local_68 = dVar1;
    }
    dVar1 = (local_30 - param_7) * (local_30 - param_7) +
            (local_38 - param_6) * (local_38 - param_6) +
            (local_40 - param_5) * (local_40 - param_5);
    if (dVar1 < dVar2) {
      *(int *)((int)this + 0x500c) = iVar4 + 5;
      dVar2 = dVar1;
    }
    local_6c = local_6c + 0xa0;
    iVar3 = iVar4 + 6;
    iVar4 = iVar4 + 8;
  } while (iVar3 < 0x100);
  if ((((*(int *)((int)this + 0x5008) < 0) || (0xff < *(int *)((int)this + 0x5008))) ||
      (*(int *)((int)this + 0x500c) < 0)) || (0xff < *(int *)((int)this + 0x500c))) {
    FUN_004e0620("0 <= p && p < 256 && 0 <= q && q < 256",".\\Colorsort.cpp",0x121);
  }
  FUN_0043abe0(this);
  return this;
}


// ==== FUN_004437f0 @ 004437f0

void __cdecl FUN_004437f0(void)

{
  return;
}


// ==== FUN_00424030 @ 00424030

undefined4 __cdecl FUN_00424030(undefined4 param_1)

{
  void *pvVar1;
  undefined4 uVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f962b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  pvVar1 = operator_new(0x90);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    uVar2 = FUN_00423b20(pvVar1,param_1,0);
    ExceptionList = local_c;
    return uVar2;
  }
  ExceptionList = local_c;
  return 0;
}


// ==== FUN_00424090 @ 00424090

undefined4 __cdecl FUN_00424090(undefined4 param_1)

{
  void *pvVar1;
  undefined4 uVar2;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f962b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  pvVar1 = operator_new(0x90);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    uVar2 = FUN_00423b20(pvVar1,param_1,1);
    ExceptionList = local_c;
    return uVar2;
  }
  ExceptionList = local_c;
  return 0;
}


// ==== __chkstk @ 004e4a40

/* WARNING: This is an inlined function */
/* WARNING: Unable to track spacebase fully for stack */
/* Library Function - Single Match
    __chkstk
   
   Library: Visual Studio 2003 Release */

void __cdecl __chkstk(void)

{
  uint in_EAX;
  undefined1 *puVar1;
  undefined4 unaff_retaddr;
  
  if (in_EAX < 0x1000) {
    *(undefined4 *)(&stack0x00000000 + -in_EAX) = unaff_retaddr;
    return;
  }
  puVar1 = &stack0x00000004;
  do {
    puVar1 = puVar1 + -0x1000;
    in_EAX = in_EAX - 0x1000;
  } while (0xfff < in_EAX);
  *(undefined4 *)(puVar1 + (-4 - in_EAX)) = unaff_retaddr;
  return;
}


// ==== FUN_00424530 @ 00424530

/* WARNING: Removing unreachable block (ram,0x00424644) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00424530(void *this)

{
  int iVar1;
  float *pfVar2;
  int iVar3;
  float *pfVar4;
  float10 fVar5;
  float fVar6;
  undefined4 uVar7;
  float fVar8;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  undefined1 local_18 [8];
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  if (*(int *)((int)this + 0x24) != 0) {
    if (*(int *)((int)this + 0x54) == 0) {
      FUN_00418400((int)this + 0x30);
      FUN_00423d50(this);
      *(int *)((int)this + 0x54) = *(int *)((int)this + 0x54) + 1;
      return;
    }
    if ((DAT_005e4794 != 0) &&
       (iVar1 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1), iVar1 == 0)) {
      FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x11f);
    }
    iVar1 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
    if (iVar1 != 0) {
      iVar3 = *(int *)((int)this + 0x6c) * 0x32 + DAT_005c8464;
      iVar1 = FUN_004adc60(iVar3);
      if (iVar1 == 0) {
        FUN_004e0620("s.hasHitPoints()",".\\Burning.cpp",0x126);
      }
      local_28 = *(float *)(iVar3 + 0xe);
      local_24 = *(float *)(iVar3 + 0x12);
      local_20 = _DAT_0059a924 * local_28;
      local_1c = _DAT_0059a928 * local_24;
      *(int *)((int)this + 0x20) = *(char *)(iVar3 + 0x23) + -1;
      if (*(int *)((int)this + 0x54) == 1) {
        *(float *)((int)this + 0xc) = local_20;
        *(float *)((int)this + 0x10) = local_1c;
        *(float *)((int)this + 0x14) = local_20;
        *(float *)((int)this + 0x18) = local_1c;
        *(undefined4 *)((int)this + 0x80) = 1;
        *(undefined4 *)((int)this + 0x54) = 2;
        return;
      }
      local_10 = local_20;
      local_c = local_1c;
      local_8 = local_20;
      local_4 = local_1c;
      FUN_00423a70(local_18,*(undefined4 *)((int)this + 0xc),*(undefined4 *)((int)this + 0x10));
      FUN_00423a70(&local_28,*(undefined4 *)((int)this + 0x14),*(undefined4 *)((int)this + 0x18));
      iVar1 = FUN_00487800(local_18,local_28,local_24,-(DAT_00595130 != 0) & 100);
      if (iVar1 == 0) {
        if (*(int *)((int)this + 0x80) == 0) {
          FUN_004a97e0(*(undefined4 *)((int)this + 0x50));
        }
        *(undefined4 *)((int)this + 0x80) = 1;
        return;
      }
      iVar1 = 0;
      if (*(int *)((int)this + 0x80) != 0) {
        *(undefined4 *)((int)this + 0x30) = DAT_0055b4d0;
        *(undefined4 *)((int)this + 0x34) = DAT_0055b4d4;
        *(undefined4 *)((int)this + 0x80) = 0;
      }
      FUN_00418430((int)this + 0x30);
      FUN_00424110(this);
      pfVar4 = *(float **)((int)this + 0x2c);
      if (0 < *(int *)((int)this + 0x24)) {
        do {
          if ((float)_DAT_00500460 < pfVar4[4]) {
            if (pfVar4[5] == (float)((int)this + 0x74)) {
              fVar5 = (float10)FUN_00455930(0,0x3fb33333);
              if (fVar5 < (float10)*(float *)((int)this + 0x40)) {
                uVar7 = 0;
                local_28 = (local_20 - (float)*(int *)((int)this + 100)) + *pfVar4;
                fVar6 = local_1c - (float)*(int *)((int)this + 0x68);
                fVar8 = pfVar4[1];
                fVar5 = (float10)FUN_00455930(0x40800000,0x41100000,0);
                pfVar2 = (float *)FUN_00423e80(this,local_28,fVar6 + fVar8,0,0xc0000000,(float)fVar5
                                               ,uVar7);
                if (pfVar2 != (float *)0x0) {
                  fVar8 = pfVar2[1];
                  fVar6 = *pfVar2;
                  goto LAB_0042480c;
                }
              }
            }
            else {
              pfVar4[4] = pfVar4[4] - *(float *)((int)this + 0x40);
              fVar8 = _DAT_005e18a8 + pfVar4[2];
              pfVar4[2] = fVar8;
              *pfVar4 = fVar8 * *(float *)((int)this + 0x40) + *pfVar4;
              fVar6 = *pfVar4;
              fVar8 = pfVar4[3] * *(float *)((int)this + 0x40) + pfVar4[1];
              pfVar4[1] = fVar8;
              local_28 = fVar8;
LAB_0042480c:
              FUN_004239f0(&local_10,fVar6,fVar8);
            }
          }
          iVar1 = iVar1 + 1;
          pfVar4 = pfVar4 + 6;
        } while (iVar1 < *(int *)((int)this + 0x24));
      }
      local_8 = local_8 + _DAT_00501444;
      local_4 = local_4 + _DAT_005012d0;
      if (local_8 - _DAT_005037e0 <= local_10) {
        local_10 = local_8 - _DAT_005037e0;
      }
      local_c = local_c - _DAT_00501444;
      FUN_00487ba0((int)this + 0xc,local_10,local_c,local_8,local_4);
    }
  }
  return;
}


// ==== FUN_00424110 @ 00424110

float * __thiscall FUN_00424110(void *this)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float *pfVar10;
  float *pfVar11;
  int iVar12;
  float *local_28;
  int local_24;
  int local_20;
  float *local_1c;
  float *local_18;
  
  if ((DAT_005e4794 != 0) && (iVar2 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1), iVar2 == 0)
     ) {
    FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x91);
  }
  iVar2 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
  if (iVar2 == 0) {
    return (float *)0x0;
  }
  iVar12 = *(int *)((int)this + 0x6c) * 0x32 + DAT_005c8464;
  iVar2 = FUN_004adc60(iVar12);
  if (iVar2 == 0) {
    FUN_004e0620("burningVictim.hasHitPoints()",".\\Burning.cpp",0x98);
  }
  iVar2 = *(int *)(iVar12 + 0x1a);
  uVar3 = FUN_004ac550(iVar12);
  iVar4 = FUN_0049fce0(uVar3);
  local_28 = (float *)(6 - (iVar2 * 5) /
                           (iVar4 - ((int)(iVar4 * 2 + (iVar4 * 2 >> 0x1f & 3U)) >> 2)));
  pfVar11 = (float *)((*(int *)((int)this + 0x60) + *(int *)((int)this + 0x5c)) / 5);
  pfVar10 = local_28;
  if ((int)pfVar11 <= (int)local_28) {
    pfVar10 = pfVar11;
  }
  if ((int)pfVar10 < 1) {
    pfVar11 = (float *)0x1;
  }
  else if ((int)local_28 < (int)pfVar11) goto LAB_004241f7;
  local_28 = pfVar11;
LAB_004241f7:
  uVar5 = FUN_004ac200(iVar12);
  if ((uVar5 & 0x400) != 0) {
    local_28 = (float *)0x1;
  }
  pfVar11 = *(float **)((int)this + 0x7c);
  if (pfVar11 != local_28) {
    if (*(int *)((int)this + 0x60) < 3) {
      *(float **)((int)this + 0x7c) = local_28;
      return pfVar11;
    }
    if ((int)pfVar11 < (int)local_28) {
      iVar2 = *(int *)((int)this + 0x5c) + -7;
      iVar12 = 8;
      if (7 < iVar2) {
        iVar12 = iVar2;
      }
      local_24 = *(int *)((int)this + 0x5c) + -1;
      if ((iVar12 <= local_24) && (local_24 = 8, 7 < iVar2)) {
        local_24 = iVar2;
      }
      iVar2 = *(int *)((int)this + 0x60) + -1;
      pfVar11 = (float *)((local_24 + -2 + iVar2) / 5);
      if ((int)pfVar11 <= (int)local_28) {
        local_28 = pfVar11;
      }
      iVar12 = 300;
      if (*(int *)((int)this + 0x7c) < (int)local_28) {
        do {
          iVar4 = iVar12 + -1;
          pfVar11 = (float *)0x0;
          if (iVar12 == 0) break;
          iVar6 = FUN_004558f0(0,local_24);
          iVar7 = FUN_004558f0(2,iVar2);
          pfVar11 = (float *)((iVar7 + iVar6) * 0x66666667);
          iVar12 = (iVar7 + iVar6) / 5;
          if ((*(char *)((iVar7 + 1) * *(int *)((int)this + 0x5c) + iVar6 +
                        *(int *)((int)this + 0x58)) != -1) &&
             (pfVar11 = *(float **)((int)this + 0x78), *(char *)((int)pfVar11 + iVar12) == '\0')) {
            iVar8 = FUN_004558f0(6,10);
            pfVar11 = (float *)0x0;
            local_20 = 0;
            local_1c = (float *)0x0;
            if (0 < iVar8) {
              do {
                iVar1 = ((uint)local_1c >> 1) + iVar6;
                if (((iVar1 < *(int *)((int)this + 0x5c) + -1) &&
                    (iVar9 = iVar7 - (int)local_1c / 2, -1 < iVar9)) &&
                   (*(char *)(iVar9 * *(int *)((int)this + 0x5c) + ((uint)local_1c >> 1) + iVar6 +
                             *(int *)((int)this + 0x58)) != -1)) {
                  FUN_00423e80(this,(float)iVar1,(float)iVar9,0,0,0x461c3c00,(int)this + 0x74);
                  local_20 = local_20 + 1;
                  *(undefined1 *)(*(int *)((int)this + 0x78) + iVar12) = 1;
                }
                pfVar11 = (float *)((int)local_1c + 1);
                local_1c = pfVar11;
              } while ((int)pfVar11 < iVar8);
              if (3 < local_20) {
                *(int *)((int)this + 0x7c) = *(int *)((int)this + 0x7c) + 1;
              }
            }
          }
          iVar12 = iVar4;
        } while (*(int *)((int)this + 0x7c) < (int)local_28);
        if (iVar4 < 1) {
          iVar2 = 7;
          local_1c = (float *)0x0;
          while ((int)local_1c < *(int *)((int)this + 0x60) + -1) {
            for (local_18 = (float *)0x0;
                (iVar2 != 0 &&
                (pfVar11 = *(float **)((int)this + 0x5c), (int)local_18 < (int)pfVar11));
                local_18 = (float *)((int)local_18 + 1)) {
              pfVar11 = (float *)((int)pfVar11 * (int)local_1c + *(int *)((int)this + 0x58));
              if (*(char *)((int)pfVar11 + (int)local_18) != -1) {
                pfVar11 = (float *)FUN_00487b40((int)this + 0x24);
                if (pfVar11 != (float *)0x0) {
                  pfVar11[2] = 0.0;
                  pfVar11[3] = 0.0;
                  *pfVar11 = (float)(int)local_18;
                  pfVar11[4] = 9999.0;
                  pfVar11[5] = (float)((int)this + 0x74);
                  pfVar11[1] = (float)(int)local_1c;
                }
                iVar2 = iVar2 + -1;
              }
            }
            local_1c = (float *)((int)local_1c + 1);
            if (iVar2 == 0) {
              return pfVar11;
            }
          }
        }
      }
    }
    else {
      iVar2 = *(int *)((int)this + 0x2c);
      pfVar11 = (float *)0x0;
      if (0 < *(int *)((int)this + 0x28)) {
        do {
          if (*(int *)(iVar2 + 0x14) == (int)this + 0x74) break;
          iVar2 = iVar2 + 0x18;
          pfVar11 = (float *)((int)pfVar11 + 1);
        } while ((int)pfVar11 < *(int *)((int)this + 0x28));
      }
      if (((int)pfVar11 < *(int *)((int)this + 0x28)) &&
         (pfVar11 = (float *)((int)this + 0x74), *(float **)(iVar2 + 0x14) == pfVar11)) {
        iVar2 = FUN_004e49c0();
        iVar12 = FUN_004e49c0();
        iVar2 = (iVar2 + iVar12) / 5;
        if (*(char *)(iVar2 + *(int *)((int)this + 0x78)) != '\x01') {
          FUN_004e0620("rowTaken[killRow] == 1",".\\Burning.cpp",0xf7);
        }
        *(undefined1 *)(iVar2 + *(int *)((int)this + 0x78)) = 0;
        pfVar11 = *(float **)((int)this + 0x28);
        iVar12 = *(int *)((int)this + 0x2c);
        local_18 = (float *)0x0;
        if (0 < (int)pfVar11) {
          do {
            if (*(int *)(iVar12 + 0x14) == (int)this + 0x74) {
              iVar4 = FUN_004e49c0();
              iVar6 = FUN_004e49c0();
              if ((iVar4 + iVar6) / 5 == iVar2) {
                *(undefined4 *)(iVar12 + 0x14) = 0;
                *(undefined4 *)(iVar12 + 0x10) = 0;
              }
            }
            pfVar11 = (float *)((int)local_18 + 1);
            iVar12 = iVar12 + 0x18;
            local_18 = pfVar11;
          } while ((int)pfVar11 < *(int *)((int)this + 0x28));
        }
      }
    }
  }
  return pfVar11;
}


// ==== FUN_00423d50 @ 00423d50

void __thiscall FUN_00423d50(void *this)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  
  if (*(void **)((int)this + 0x58) != (void *)0x0) {
    _free(*(void **)((int)this + 0x58));
  }
  if (*(void **)((int)this + 0x78) != (void *)0x0) {
    _free(*(void **)((int)this + 0x78));
  }
  FUN_00487ab0((int)this + 0x24);
  if (DAT_005e4794 != 0) {
    iVar3 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
    if (iVar3 == 0) {
      FUN_004e0620("isSidRobust(burningSid)",".\\Burning.cpp",0x6f);
    }
  }
  iVar3 = FUN_004ac140(*(undefined4 *)((int)this + 0x6c),1);
  if (iVar3 != 0) {
    iVar3 = *(int *)((int)this + 0x6c) * 0x32;
    iVar8 = iVar3 + DAT_005c8464;
    if (*(int *)(iVar3 + DAT_005c8464) == 0) {
      FUN_004e0620("*((long**)&s)!=0",".\\Burning.cpp",0x77);
    }
    iVar3 = FUN_004adc60(iVar8);
    if (iVar3 == 0) {
      FUN_004e0620("s.hasHitPoints()",".\\Burning.cpp",0x7f);
    }
    piVar1 = (int *)((int)this + 0x60);
    piVar2 = (int *)((int)this + 0x5c);
    uVar4 = FUN_004a7db0(*(undefined4 *)((int)this + 0x6c),piVar2,piVar1,(int)this + 100,
                         (int)this + 0x68);
    *(undefined4 *)((int)this + 0x58) = uVar4;
    puVar5 = operator_new((*piVar1 + *piVar2) / 5);
    uVar7 = (*piVar1 + *piVar2) / 5;
    *(undefined4 **)((int)this + 0x78) = puVar5;
    for (uVar6 = uVar7 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined1 *)puVar5 = 0;
      puVar5 = (undefined4 *)((int)puVar5 + 1);
    }
    *(undefined4 *)((int)this + 0x7c) = 0;
  }
  return;
}


// ==== FUN_00423e80 @ 00423e80

void __thiscall
FUN_00423e80(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_00487b40((int)this + 0x24);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = param_2;
    puVar1[1] = param_3;
    puVar1[2] = param_4;
    puVar1[3] = param_5;
    puVar1[4] = param_6;
    puVar1[5] = param_7;
  }
  return;
}


// ==== FUN_00423ec0 @ 00423ec0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00423ec0(void *this)

{
  undefined1 uVar1;
  int iVar2;
  int iVar3;
  undefined1 *puVar4;
  int iVar5;
  int local_10;
  
  if (*(int *)((int)this + 0x24) != 0) {
    if (*(int *)((int)this + 0x24) < *(int *)((int)this + 0x28)) {
      FUN_004e0620("totalParticles <= maxParticles",".\\Burning.cpp",0x17e);
    }
    iVar5 = *(int *)((int)this + 0x2c);
    local_10 = 0;
    if (0 < *(int *)((int)this + 0x28)) {
      do {
        if (((float)_DAT_00500460 < *(float *)(iVar5 + 0x10)) &&
           (*(int *)(iVar5 + 0x14) != (int)this + 0x74)) {
          FUN_004e49c0();
          FUN_004e49c0();
          uVar1 = FUN_00487e50(DAT_005453d0,*(undefined4 *)(iVar5 + 0x10),0x41100000);
          iVar2 = FUN_004e49c0();
          iVar3 = FUN_004e49c0();
          if ((DAT_005c7a44 <= iVar2) &&
             (((iVar2 < DAT_005c7a4c && (DAT_005c7a48 <= iVar3)) && (iVar3 < DAT_005c7a50)))) {
            if ((((DAT_005c7a44 <= iVar2 + 1) && (iVar2 + 1 < DAT_005c7a4c)) &&
                (DAT_005c7a48 <= iVar3 + -2)) && (iVar3 + -2 < DAT_005c7a50)) {
              iVar3 = DAT_005c78ec * iVar3 + DAT_005c78f8;
              *(undefined1 *)(iVar3 + iVar2) = uVar1;
              iVar3 = iVar3 + iVar2;
              *(undefined1 *)(iVar3 + 1) = uVar1;
              puVar4 = (undefined1 *)((iVar3 + 1) - DAT_005c78ec);
              *puVar4 = uVar1;
              puVar4 = puVar4 + -1;
              *puVar4 = uVar1;
              puVar4[-DAT_005c78ec] = uVar1;
            }
          }
        }
        local_10 = local_10 + 1;
        iVar5 = iVar5 + 0x18;
      } while (local_10 < *(int *)((int)this + 0x28));
    }
  }
  return;
}


// ==== FUN_00487b40 @ 00487b40

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_00487b40(void *this)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  
  iVar1 = *(int *)((int)this + 4);
  if (iVar1 < *(int *)this) {
    *(int *)((int)this + 4) = iVar1 + 1;
    return *(int *)((int)this + 8) + iVar1 * 0x18;
  }
  iVar2 = 0;
  if (0 < iVar1) {
    pfVar3 = (float *)(*(int *)((int)this + 8) + 0x10);
    do {
      if (*pfVar3 < (float)_DAT_00500460 != (*pfVar3 == (float)_DAT_00500460)) {
        return *(int *)((int)this + 8) + iVar2 * 0x18;
      }
      iVar2 = iVar2 + 1;
      pfVar3 = pfVar3 + 6;
    } while (iVar2 < *(int *)((int)this + 4));
    return 0;
  }
  return 0;
}


