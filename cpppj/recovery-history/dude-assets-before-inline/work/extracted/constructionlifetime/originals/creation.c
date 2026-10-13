// ==== FUN_00487fd0 @ 00487fd0

undefined4 * __thiscall FUN_00487fd0(undefined4 *param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_004fb048;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  param_1[2] = 0;
  param_1[1] = 0;
  param_1[6] = 0;
  param_1[5] = 0;
  param_1[4] = 0;
  param_1[3] = 0;
  *param_1 = &PTR_FUN_0050ddc4;
  local_4 = 0;
  puVar2 = DAT_00595138;
  do {
    DAT_00595138 = puVar2;
    iVar3 = DAT_0059513c;
    iVar1 = 0;
    if (0 < DAT_0059513c) {
      do {
        if (DAT_00595138[iVar1] == 0) {
          DAT_00595138[iVar1] = param_1;
          param_1[7] = iVar1;
          param_1[8] = (int)*(char *)(param_1[2] * 0x32 + 0x23 + DAT_005c8464);
          if (param_2 != 0) {
            FUN_0041bd20(param_3,param_2,0,0x10);
            param_1[8] = (int)*(char *)(param_2 * 0x32 + 0x23 + DAT_005c8464);
          }
          ExceptionList = local_c;
          return param_1;
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


// ==== FUN_00487880 @ 00487880

void __fastcall FUN_00487880(undefined4 *param_1)

{
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_004fb048;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *param_1 = &PTR_FUN_0050ddc4;
  local_4 = 0;
  if (((int)param_1[7] < 0) || (DAT_0059513c <= (int)param_1[7])) {
    FUN_004e0620("0 <= id && id < totalSystemsAllocated",".\\Particle.cpp",0x90);
  }
  if (*(undefined4 **)(DAT_00595138 + param_1[7] * 4) != param_1) {
    FUN_004e0620("systems[id] == this",".\\Particle.cpp",0x91);
  }
  *(undefined4 *)(DAT_00595138 + param_1[7] * 4) = 0;
  local_4 = 0xffffffff;
  FUN_0041bd00();
  ExceptionList = local_c;
  return;
}


// ==== FUN_0041bd00 @ 0041bd00

void __fastcall FUN_0041bd00(undefined4 *param_1)

{
  *param_1 = &PTR_FUN_005019ec;
  param_1[1] = 0xffffcfc7;
  param_1[2] = 0xffffcfc7;
  return;
}


// ==== FUN_00422d00 @ 00422d00

void __fastcall FUN_00422d00(undefined4 *param_1)

{
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  puStack_8 = &LAB_004f953f;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *param_1 = &PTR_FUN_00503610;
  local_4 = 1;
  if ((void *)param_1[0x13] != (void *)0x0) {
    _free((void *)param_1[0x13]);
  }
  if ((void *)param_1[0x14] != (void *)0x0) {
    _free((void *)param_1[0x14]);
  }
  local_4 = local_4 & 0xffffff00;
  if ((void *)param_1[0xb] != (void *)0x0) {
    _free((void *)param_1[0xb]);
  }
  local_4 = 0xffffffff;
  FUN_00487880();
  ExceptionList = local_c;
  return;
}


// ==== FUN_00423540 @ 00423540

void * __thiscall FUN_00423540(void *param_1,byte param_2)

{
  FUN_00422d00();
  if ((param_2 & 1) != 0) {
    _free(param_1);
  }
  return param_1;
}


// ==== thunk_FUN_00422d00 @ 00423810

void __fastcall thunk_FUN_00422d00(undefined4 *param_1)

{
  void *pvStack_c;
  undefined1 *puStack_8;
  uint uStack_4;
  
  puStack_8 = &LAB_004f953f;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  *param_1 = &PTR_FUN_00503610;
  uStack_4 = 1;
  if ((void *)param_1[0x13] != (void *)0x0) {
    _free((void *)param_1[0x13]);
  }
  if ((void *)param_1[0x14] != (void *)0x0) {
    _free((void *)param_1[0x14]);
  }
  uStack_4 = uStack_4 & 0xffffff00;
  if ((void *)param_1[0xb] != (void *)0x0) {
    _free((void *)param_1[0xb]);
  }
  uStack_4 = 0xffffffff;
  FUN_00487880();
  ExceptionList = pvStack_c;
  return;
}


// ==== FUN_00423820 @ 00423820

void * __thiscall FUN_00423820(void *param_1,byte param_2)

{
  thunk_FUN_00422d00();
  if ((param_2 & 1) != 0) {
    _free(param_1);
  }
  return param_1;
}


