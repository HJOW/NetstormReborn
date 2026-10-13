// ==== FUN_004c0390 @ 004c0390

void * __thiscall
FUN_004c0390(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7,undefined4 param_8)

{
  undefined4 uVar1;
  int iVar2;
  undefined2 uVar3;
  undefined4 *puVar4;
  
  *(undefined4 *)((int)this + 0x2080c) = 0;
  *(undefined4 *)((int)this + 0x20810) = 0;
  *(undefined4 *)((int)this + 0x20808) = 1;
  *(undefined4 *)((int)this + 0x20814) = param_8;
  *(undefined4 *)((int)this + 4) = 1;
  puVar4 = (undefined4 *)((int)this + 0x20008);
  for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = (undefined4 *)((int)this + 0x20408);
  for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined4 *)((int)this + 0x2081c) = 0;
  *(undefined4 *)((int)this + 0x20818) = param_3;
  if (DAT_00594fbc != 0) {
    *(undefined4 *)((int)this + 0x20810) = 0x10;
  }
  if (param_2 == 0) {
    *(undefined4 *)((int)this + 0x20810) = 0x18;
  }
  *(undefined4 *)((int)this + 0x200c0) = 0;
  *(undefined4 *)((int)this + 0x201e8) = DAT_005411dc;
  *(undefined4 *)((int)this + 0x205e8) = 0x78;
  uVar1 = DAT_005411e0;
  *(undefined4 *)((int)this + 0x20568) = 0x78;
  *(undefined4 *)((int)this + 0x205c4) = 0x78;
  *(undefined4 *)((int)this + 0x20168) = uVar1;
  *(undefined4 *)((int)this + 0x201c4) = 0;
  uVar1 = DAT_005411dc;
  *(undefined4 *)((int)this + 0x204fc) = 0x78;
  *(undefined4 *)((int)this + 0x200fc) = uVar1;
  puVar4 = (undefined4 *)((int)this + 8);
  for (iVar2 = 0x4000; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0x2e2e2e2e;
    puVar4 = puVar4 + 1;
  }
  uVar3 = CONCAT11((undefined1)DAT_0050c038,(undefined1)DAT_0050c038);
  puVar4 = (undefined4 *)((int)this + 0x10008);
  for (iVar2 = 0x4000; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = CONCAT22(uVar3,uVar3);
    puVar4 = puVar4 + 1;
  }
  *(int *)this = param_2;
  FUN_004c02d0(this,param_4,param_5,param_6,param_7);
  return this;
}


// ==== FUN_004c02d0 @ 004c02d0

void __thiscall FUN_004c02d0(void *this,int param_2,int param_3,int param_4,int param_5)

{
  *(int *)((int)this + 0x20830) = param_2;
  *(int *)((int)this + 0x20834) = param_3;
  *(int *)((int)this + 0x20820) = param_2 << 4;
  *(int *)((int)this + 0x20824) = param_3 << 4;
  *(int *)((int)this + 0x2083c) = param_5;
  *(int *)((int)this + 0x20838) = param_4;
  *(int *)((int)this + 0x20828) = param_4 << 4;
  *(int *)((int)this + 0x2082c) = param_5 << 4;
  if (0x10 < param_4) {
    FUN_004e0620("cex <= MAP_CHUNK_XLEN","\\Ns\\O\\Terrainbuilder.cpp",0xe2);
  }
  if (0x10 < *(int *)((int)this + 0x2083c)) {
    FUN_004e0620("cey <= MAP_CHUNK_YLEN","\\Ns\\O\\Terrainbuilder.cpp",0xe3);
  }
  if (0x10 < *(int *)((int)this + 0x20838)) {
    *(undefined4 *)((int)this + 0x20838) = 0x10;
  }
  if (0x10 < *(int *)((int)this + 0x2083c)) {
    *(undefined4 *)((int)this + 0x2083c) = 0x10;
  }
  return;
}


// ==== FUN_004c09a0 @ 004c09a0

void __thiscall FUN_004c09a0(void *this,int param_2,char param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  int iVar5;
  int iVar6;
  
  iVar2 = *(int *)((int)this + 0x20824);
  if (iVar2 < *(int *)((int)this + 0x2082c)) {
    do {
      iVar6 = *(int *)((int)this + 0x20820);
      if (iVar6 < *(int *)((int)this + 0x20828)) {
        pcVar4 = (char *)(iVar6 * 0x100 + iVar2 + 0x10008 + (int)this);
        do {
          if (pcVar4[-0x10000] == param_2) {
            iVar5 = 0;
            do {
              iVar3 = *(int *)((int)&DAT_0052f85c + iVar5) + iVar2;
              iVar1 = *(int *)((int)&DAT_0052f83c + iVar5) + iVar6;
              if ((((iVar1 < *(int *)((int)this + 0x20820)) ||
                   (iVar3 < *(int *)((int)this + 0x20824))) ||
                  (*(int *)((int)this + 0x20828) < iVar1)) ||
                 (((*(int *)((int)this + 0x2082c) < iVar3 ||
                   (iVar3 = iVar1 * 0x100 + iVar3,
                   *(int *)((int)this + *(char *)(iVar3 + 8 + (int)this) * 4 + 0x20408) !=
                   (int)pcVar4[-0x10000])) || (*(char *)((int)this + iVar3 + 0x10008) != *pcVar4))))
              {
                pcVar4[-0x10000] = param_3;
                *pcVar4 = *pcVar4;
                break;
              }
              iVar5 = iVar5 + 4;
            } while (iVar5 < 0x20);
          }
          iVar6 = iVar6 + 1;
          pcVar4 = pcVar4 + 0x100;
        } while (iVar6 < *(int *)((int)this + 0x20828));
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < *(int *)((int)this + 0x2082c));
  }
  return;
}


// ==== FUN_004c0ad0 @ 004c0ad0

void __thiscall FUN_004c0ad0(void *this)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = *(int *)((int)this + 0x20824);
  if (iVar4 < *(int *)((int)this + 0x2082c)) {
    do {
      iVar3 = *(int *)((int)this + 0x20820);
      if (iVar3 < *(int *)((int)this + 0x20828)) {
        pcVar2 = (char *)(iVar3 * 0x100 + iVar4 + 8 + (int)this);
        do {
          cVar1 = *pcVar2;
          if (((cVar1 == '=') || (cVar1 == 'o')) || (cVar1 == 'X')) {
            *pcVar2 = 'x';
          }
          iVar3 = iVar3 + 1;
          pcVar2 = pcVar2 + 0x100;
        } while (iVar3 < *(int *)((int)this + 0x20828));
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < *(int *)((int)this + 0x2082c));
  }
  return;
}


// ==== FUN_004c1510 @ 004c1510

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_004c1510(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  bool bVar1;
  int iVar2;
  void *_Memory;
  int iVar3;
  int iVar4;
  uint uVar5;
  int local_28;
  void *local_24;
  undefined2 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  uint local_14;
  undefined4 local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb8eb;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  if (((DAT_00540a2c != 0) && (ExceptionList = &local_c, DAT_00594fbc != 0)) &&
     (ExceptionList = &local_c, DAT_00594fa0 == 0)) {
    ExceptionList = &local_c;
    iVar2 = FUN_0046ee50(&DAT_00568b60);
    if (iVar2 == 1) {
      bVar1 = true;
      goto LAB_004c155c;
    }
  }
  bVar1 = false;
LAB_004c155c:
  local_24 = operator_new(0x20840);
  local_4 = 0;
  if (local_24 == (void *)0x0) {
    _Memory = (void *)0x0;
  }
  else {
    _Memory = (void *)FUN_004c0390(local_24,param_1,param_7,param_2,param_3,param_4,param_5,param_6)
    ;
  }
  local_4 = 0xffffffff;
  if ((DAT_0052f7e5 == '\0') && (bVar1)) {
    iVar2 = FUN_0041ca10();
    iVar2 = iVar2 + -2;
    iVar3 = FUN_0041ca10();
    if (iVar3 == 3) {
      DAT_00542654 = 1;
    }
    else {
      DAT_00542654 = 2;
      if (iVar3 != 4) {
        DAT_00542654 = 3;
      }
    }
    *(undefined4 *)((int)_Memory + 0x2081c) = 1;
    FUN_004c02d0(_Memory,(*(int *)((int)_Memory + 0x20830) - iVar2) + 1,
                 (*(int *)((int)_Memory + 0x20834) - iVar2) + 1,
                 *(int *)((int)_Memory + 0x20838) + iVar2,*(int *)((int)_Memory + 0x2083c) + iVar2);
    iVar2 = DAT_00542654 * 2 + 2;
    FUN_004c13f0(_Memory,(iVar2 * iVar2 * 0x55) / 0x19);
    local_28 = 0;
    local_28 = FUN_004c0e20(_Memory);
    local_28 = local_28 * 0x1bb;
    iVar2 = (*(int *)((int)_Memory + 0x2083c) - *(int *)((int)_Memory + 0x20834)) *
            (*(int *)((int)_Memory + 0x20838) - *(int *)((int)_Memory + 0x20830)) * 0x100;
    if (_DAT_005c8bb4 == 0) {
      iVar4 = (DAT_00542650 * iVar2) / 200;
      local_24 = (void *)((DAT_0054264c * iVar2) / 200);
      iVar3 = FUN_004bca60(&local_28,local_24,iVar4);
      iVar4 = FUN_004bca60(&local_28,local_24,iVar4);
      uVar5 = iVar3 + iVar4;
    }
    else {
      uVar5 = (_DAT_005c8bb4 * iVar2) / 100;
    }
    FUN_004c2630(0x1d,"\nArchipelago Percentage: %d\n",(int)(uVar5 * 100) / iVar2);
    FUN_004c0800(_Memory,0,*(undefined4 *)((int)_Memory + 0x20830),
                 *(undefined4 *)((int)_Memory + 0x20834),*(undefined4 *)((int)_Memory + 0x20838),
                 *(undefined4 *)((int)_Memory + 0x2083c),uVar5,&local_28);
    iVar3 = 1;
    iVar2 = 0xb4;
    do {
      if (*(int *)(DAT_0059534c + 0x2c + iVar2) != 0) {
        local_24 = (void *)0x107;
        local_20 = 0;
        FUN_00432050(&local_1c,iVar3,0x107,uVar5 & 0xffff0000,&DAT_0054c8a0);
        uVar5 = local_14;
        FUN_004c0fe0(_Memory,local_1c,local_18,local_14,local_10);
      }
      iVar2 = iVar2 + 0xb4;
      iVar3 = iVar3 + 1;
    } while (iVar2 < 0x654);
    FUN_004c0cf0(_Memory,1);
    FUN_004c0db0(_Memory);
    FUN_004c09a0(_Memory,0x78,0x3d);
    FUN_004c0640(0x78);
    FUN_004c0ad0(_Memory);
    DAT_005c8bb0 = 1;
  }
  else {
    FUN_004c10a0(_Memory);
    FUN_004c09a0(_Memory,0x78,0x3d);
    FUN_004c0640(0x78);
    FUN_004c0ad0(_Memory);
  }
  _free(_Memory);
  ExceptionList = local_c;
  return;
}


// ==== FUN_0046da70 @ 0046da70

void __thiscall FUN_0046da70(void *this,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined1 local_28 [8];
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  if (*(undefined **)this != &DAT_0054c8a0) {
    FUN_004e0620("inWorld()","\\Ns\\O\\Islandbuilder.cpp",0x42);
  }
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_004e0620("cRect.isValid()","\\Ns\\O\\Islandbuilder.cpp",0x43);
  }
  if (*(int *)this == 0) {
    FUN_004e0620(&DAT_0050be98,"\\Ns\\O\\Islandbuilder.cpp",0x46);
  }
  FUN_004c1510(*(undefined4 *)((int)this + 0x14),*(undefined4 *)((int)this + 4),
               *(undefined4 *)((int)this + 8),*(undefined4 *)((int)this + 0xc),
               *(undefined4 *)((int)this + 0x10),*(undefined4 *)this,param_2);
  iVar3 = 0;
  iVar1 = FUN_0046ee50(&DAT_00568b60);
  if (0 < iVar1) {
    do {
      if ((param_2 == DAT_0050c038) || (param_2 == iVar3)) {
        FUN_00432430(*(undefined4 *)this,local_28,iVar3);
        puVar4 = &local_20;
        uVar2 = FUN_00432210(*(undefined4 *)this,local_10,iVar3);
        FUN_0040d900(uVar2,puVar4);
        uVar2 = FUN_004bfde0(local_28);
        FUN_0040e560(local_20,local_1c,local_18,local_14,iVar3,uVar2);
      }
      iVar3 = iVar3 + 1;
      iVar1 = FUN_0046ee50(&DAT_00568b60);
    } while (iVar3 < iVar1);
  }
  return;
}


// ==== FUN_0040e560 @ 0040e560

void __cdecl
FUN_0040e560(float param_1,float param_2,float param_3,float param_4,int param_5,int param_6)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int local_58;
  int local_54;
  int local_50;
  undefined4 local_4c;
  int local_48 [2];
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  void *local_28;
  undefined4 local_24;
  undefined4 local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004f8fb8;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  iVar2 = FUN_00432430(&DAT_0054c8a0,local_48,param_5);
  local_58 = (uint)*(byte *)(iVar2 + 3) * 0x4d;
  local_28 = operator_new(0x4b0);
  uVar5 = 0;
  local_24 = 300;
  local_20 = 0;
  iVar2 = param_6 * 0x14;
  local_4 = 0;
  local_54 = 30000;
  iVar6 = 1;
  if ((DAT_005c8bb0 != 0) && (param_5 == 0)) {
    local_50 = DAT_00542654 * 0x10;
    param_1 = param_1 - (float)(local_50 + -0x10);
    param_2 = param_2 - (float)(local_50 + -0x10);
    iVar6 = DAT_00542654 * 2 + 2;
    param_3 = param_3 + (float)local_50;
    param_4 = (float)local_50 + param_4;
    uVar5 = 1;
    iVar2 = (iVar2 / 9) * iVar6 * iVar6;
    iVar6 = 2;
  }
  local_1c = param_1;
  local_10 = param_4;
  local_18 = param_2;
  local_14 = param_3;
  local_50 = FUN_004e49c0();
  local_4c = FUN_004e49c0();
  do {
    iVar3 = FUN_004bca20(&local_58,local_50);
    fVar1 = (float)iVar3 + local_18;
    iVar3 = FUN_004bca20(&local_58,local_4c);
    local_40 = (float)iVar3 + local_1c;
    local_3c = fVar1;
    local_38 = local_40;
    local_34 = fVar1;
    iVar3 = FUN_0040db90(&local_58,DAT_0054123c,local_40,fVar1,param_5,1,uVar5);
    if (iVar3 != 0) {
      param_6 = -iVar6;
      iVar2 = iVar2 + -1;
      local_54 = 30000;
      local_48[0] = param_6;
      if (-iVar6 == iVar6 || param_6 < iVar6) {
        do {
          iVar3 = local_48[0];
          if (iVar2 == 0) goto LAB_0040e7ce;
          while ((iVar3 <= iVar6 && (iVar2 != 0))) {
            local_2c = (float)iVar3 + local_34;
            local_30 = (float)param_6 + local_38;
            iVar4 = FUN_0040db90(&local_58,DAT_0054123c,local_30,local_2c,param_5,1,uVar5);
            if (iVar4 != 0) {
              iVar2 = iVar2 + -1;
            }
            iVar3 = iVar3 + 1;
          }
          param_6 = param_6 + 1;
        } while (param_6 <= iVar6);
      }
    }
  } while ((iVar2 != 0) && (local_54 = local_54 + -1, local_54 != 0));
LAB_0040e7ce:
  local_4 = 0xffffffff;
  _free(local_28);
  ExceptionList = local_c;
  return;
}


// ==== FUN_004bfde0 @ 004bfde0

int __thiscall FUN_004bfde0(void *this)

{
  int iVar1;
  undefined1 local_54 [20];
  int local_40;
  
  FUN_00425c20(local_54,DAT_00541204,*(byte *)this & 0x3f,(uint)(*(byte *)this >> 6) << 1,0,0,0);
  iVar1 = 0;
  while (local_40 != 0) {
    iVar1 = iVar1 + 1;
    FUN_00425860(local_54);
  }
  return iVar1;
}


// ==== FUN_0040db90 @ 0040db90

int __cdecl
FUN_0040db90(float param_1,float param_2,float param_3,float param_4,int param_5,int param_6,
            int param_7)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  char *pcVar4;
  int *piVar5;
  undefined4 unaff_retaddr;
  longlong lVar6;
  ulonglong uVar7;
  undefined8 uVar8;
  
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((*(byte *)(iVar2 + iVar1 * 0x100 + DAT_005c7c44) & 0x3f) == 0) {
    if (DAT_00544ce4 != (FILE *)0x0) {
      _fprintf(DAT_00544ce4,"failed spot flags\n");
      _fflush(DAT_00544ce4);
    }
    return 0;
  }
  iVar1 = FUN_004b1fa0(param_3,param_4,DAT_0054123c);
  if (iVar1 != 0) {
    if (DAT_00544ce4 != (FILE *)0x0) {
      _fprintf(DAT_00544ce4,"failed spot edgefarm\n");
      _fflush(DAT_00544ce4);
    }
    return 0;
  }
  uVar8 = 0;
  uVar7 = (ulonglong)(uint)param_4;
  lVar6 = (ulonglong)(uint)param_3 << 0x20;
  uVar3 = FUN_0049a840(param_2,0,param_3,param_4,0,0,0);
  iVar1 = FUN_0049b510(uVar3,lVar6,uVar7,uVar8);
  if ((iVar1 == 0) ||
     ((param_7 == 0 &&
      (pcVar4 = (char *)FUN_0040d980(&DAT_0054c8a0,param_3,param_4), *pcVar4 != param_5)))) {
    if (DAT_00544ce4 != (FILE *)0x0) {
      _fprintf(DAT_00544ce4,"failed !legaldrop %f %f\n",(double)param_3,(double)param_4);
      _fflush(DAT_00544ce4);
    }
    return 0;
  }
  iVar2 = FUN_0049a840();
  iVar1 = 0;
  if ((*(uint *)(iVar2 + 0xe8) & 0x200000) == 0) {
LAB_0040dda3:
    if ((*(uint *)(iVar2 + 0xe8) & 0x100000) != 0) {
      iVar1 = FUN_0049aa90(iVar2,0x50,0x50,0xffffffff,0xffffffff);
    }
    if (iVar1 == -1) {
      FUN_004e0620();
    }
    piVar5 = (int *)FUN_004af530(param_2,0);
    piVar5[9] = iVar1;
    iVar1 = FUN_004adc60(piVar5);
    if (iVar1 != 0) {
      uVar3 = FUN_004ac550(piVar5);
      FUN_0049fce0(uVar3);
      FUN_004adca0(piVar5);
    }
    if (param_7 == 0) {
      FUN_0046f320(&DAT_00568b60);
    }
    (**(code **)(*piVar5 + 0x74))();
    (**(code **)(*piVar5 + 0x90))(param_2,param_3,0);
    if (DAT_00544ce4 == (FILE *)0x0) goto LAB_0040dd85;
    uVar3 = FUN_0049a840(unaff_retaddr,(double)param_1,(double)param_2);
    pcVar4 = "Made %s at (%f,%f)\n";
  }
  else {
    iVar1 = FUN_004b2070();
    if (iVar1 == 0) {
      FUN_004e0620();
    }
    piVar5 = (int *)(iVar1 * 0x32 + DAT_005c8464);
    if (param_6 == 0) {
      iVar1 = piVar5[9];
      goto LAB_0040dda3;
    }
    FUN_004e49c0();
    uVar3 = FUN_004e49c0();
    iVar1 = FUN_0040dab0(uVar3);
    if (iVar1 == 0) {
      return 1;
    }
    (**(code **)(*piVar5 + 0x48))();
    FUN_004b1620(piVar5,DAT_0054123c);
    iVar1 = FUN_004adc60(piVar5);
    if (iVar1 != 0) {
      uVar3 = FUN_004ac550(piVar5);
      uVar3 = FUN_0049fce0(uVar3);
      FUN_004adca0(piVar5,uVar3);
    }
    (**(code **)(*piVar5 + 0x4c))(0x40);
    if (DAT_00544ce4 == (FILE *)0x0) goto LAB_0040dd85;
    uVar3 = FUN_0049a840(unaff_retaddr,(double)param_1,(double)param_2);
    pcVar4 = "Isle Made %s at (%f,%f)\n";
  }
  _fprintf(DAT_00544ce4,pcVar4,uVar3);
  _fflush(DAT_00544ce4);
LAB_0040dd85:
  return ((int)piVar5 - DAT_005c8464) / 0x32;
}


// ==== FUN_004c10a0 @ 004c10a0

/* WARNING: Removing unreachable block (ram,0x004c11c0) */
/* WARNING: Type propagation algorithm not settling */

void __thiscall FUN_004c10a0(void *this)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int *piVar10;
  int local_5bc;
  int local_5b8;
  int local_5b4;
  int local_5ac;
  int local_5a8;
  int *local_5a4;
  int local_5a0;
  int local_59c [2];
  int local_594 [99];
  int local_408 [257];
  
  iVar9 = *(int *)((int)this + 0x20834);
  piVar10 = local_408;
  local_408[0] = 0;
  for (iVar7 = 0xff; piVar10 = piVar10 + 1, iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar10 = 0;
  }
  local_59c[1] = 0;
  piVar10 = local_59c + 2;
  for (iVar7 = 99; iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar10 = 0;
    piVar10 = piVar10 + 1;
  }
  if (iVar9 < *(int *)((int)this + 0x2083c)) {
    local_5bc = (iVar9 + 1) * 0x10;
    do {
      iVar7 = *(int *)((int)this + 0x20830);
      if (iVar7 < *(int *)((int)this + 0x20838)) {
        local_5a4 = local_408 + iVar7 * 0x10 + iVar9;
        iVar6 = iVar7 * 0x10;
        do {
          local_5b8 = iVar6 + 0x10;
          iVar2 = FUN_00432300(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9);
          if ((iVar2 != 0) &&
             ((*(int *)((int)this + 0x20818) == DAT_0050c038 ||
              (cVar1 = FUN_004322e0(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9),
              (int)cVar1 == *(int *)((int)this + 0x20818))))) {
            pcVar3 = (char *)FUN_00432180(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9);
            local_59c[0] = local_59c[(int)*pcVar3 + 1] * 0x10e3 + (uint)(byte)pcVar3[5];
            local_59c[(int)*pcVar3 + 1] = local_59c[(int)*pcVar3 + 1] + 1;
            iVar2 = ((byte)pcVar3[5] & 0x80000007) + iVar6 + 4;
            uVar8 = 9999 - (byte)pcVar3[5] & 0x80000007;
            if ((int)uVar8 < 0) {
              uVar8 = (uVar8 - 1 | 0xfffffff8) + 1;
            }
            local_5a8 = uVar8 + local_5bc + -0xc;
            local_5a0 = local_5a8 + 2;
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,0);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,4);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,2);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,6);
            local_5b4 = FUN_004c0930(this,iVar2 + -1,local_5a8 + -1,iVar2 + 3,local_5a8 + 3,0x78,
                                     *pcVar3);
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,0);
            if (iVar4 != 0) {
              local_5a8 = local_5bc + -0x10;
            }
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,4);
            if (iVar4 != 0) {
              local_5a0 = local_5bc;
            }
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,6);
            local_5ac = iVar2;
            if (iVar4 != 0) {
              local_5ac = iVar6;
            }
            iVar5 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,2);
            iVar4 = local_5b8;
            if (iVar5 == 0) {
              iVar4 = iVar2 + 2;
            }
            iVar2 = FUN_004c0930(this,local_5ac,local_5a8,iVar4,local_5a0,0x78,*pcVar3);
            local_5b4 = local_5b4 + iVar2;
            if (local_59c[(int)*pcVar3 + 1] == 2) {
              iVar6 = FUN_004c0930(this,iVar6 + 3,local_5bc + -0xd,iVar6 + 0xd,local_5bc + -3,0x78,
                                   (int)*pcVar3);
              local_5b4 = local_5b4 + iVar6;
            }
            iVar6 = FUN_004bca20(local_59c,0x1e);
            iVar6 = (iVar6 + 0x32) -
                    ((int)(local_5b4 * 100 + (local_5b4 * 100 >> 0x1f & 0xffU)) >> 8);
            if (iVar6 < 0x14) {
              iVar6 = 0x14;
            }
            *local_5a4 = (iVar6 << 8) / 100;
          }
          iVar7 = iVar7 + 1;
          local_5a4 = local_5a4 + 0x10;
          iVar6 = local_5b8;
        } while (iVar7 < *(int *)((int)this + 0x20838));
      }
      iVar9 = iVar9 + 1;
      local_5bc = local_5bc + 0x10;
    } while (iVar9 < *(int *)((int)this + 0x2083c));
  }
  FUN_004c0ec0(this,local_408,local_59c + 1);
  FUN_004c0cf0(this,2);
  return;
}


