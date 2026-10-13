// ==== FUN_004c04b0 @ 004c04b0

int __thiscall FUN_004c04b0(void *this,int param_2,int param_3)

{
  char cVar1;
  undefined1 uVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  uint uVar6;
  void *pvVar7;
  uint uVar8;
  int iVar9;
  uint uVar10;
  
  iVar5 = param_2 * 0x100 + param_3;
  cVar1 = *(char *)(iVar5 + 8 + (int)this);
  iVar9 = (int)*(char *)((int)this + cVar1 * 4 + 0x20408);
  uVar2 = *(undefined1 *)((int)this + iVar5 + 0x10008);
  iVar5 = FUN_004c00f0(this,param_2,param_3,iVar9,uVar2);
  cVar3 = (&DAT_0052f8fc)[iVar5];
  iVar5 = FUN_004c01e0(this,param_2,param_3,iVar9,uVar2);
  cVar4 = (&DAT_0052f8fc)[iVar5];
  iVar5 = FUN_0049a840(*(undefined4 *)((int)this + cVar1 * 4 + 0x20008));
  pvVar7 = DAT_005c8be4;
  if (DAT_005c8be4 == (void *)0x0) {
    DAT_005c8be4 = operator_new(0x18c);
    FUN_004437f0();
    iVar9 = 0;
    do {
      uVar6 = _rand();
      pvVar7 = DAT_005c8be4;
      *(uint *)(iVar9 + (int)DAT_005c8be4) = uVar6;
      if (0 < iVar9) {
        uVar8 = *(uint *)(iVar9 + -4 + (int)pvVar7) & 0x80000003;
        if ((int)uVar8 < 0) {
          uVar8 = (uVar8 - 1 | 0xfffffffc) + 1;
        }
        uVar10 = uVar6 & 0x80000003;
        if ((int)uVar10 < 0) {
          uVar10 = (uVar10 - 1 | 0xfffffffc) + 1;
        }
        if (uVar10 == uVar8) {
          *(uint *)(iVar9 + (int)pvVar7) = uVar6 + 1;
        }
      }
      iVar9 = iVar9 + 4;
    } while (iVar9 < 0x18c);
  }
  if (((cVar1 == 'x') && (cVar3 == 'A')) && (cVar4 == 'A')) {
    uVar6 = *(int *)((int)pvVar7 +
                    ((*(int *)((int)pvVar7 + ((param_3 / 3) % 99) * 4) +
                     *(int *)((int)pvVar7 + ((param_2 / 3) % 99) * 4)) % 99) * 4) + param_3 / 3 &
            0x80000003;
    if ((int)uVar6 < 0) {
      uVar6 = (uVar6 - 1 | 0xfffffffc) + 1;
    }
    iVar5 = (param_3 % 3 + uVar6 * 3) * 3 + *(int *)(iVar5 + 0x154) + 1 + param_2 % 3;
  }
  else {
    iVar5 = FUN_0049aa90(iVar5,cVar3,cVar4,0xffffffff,0);
  }
  if (iVar5 == -1) {
    iVar5 = 0;
  }
  return iVar5;
}


// ==== FUN_004c0640 @ 004c0640

void __thiscall FUN_004c0640(undefined4 *param_1,char param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  char *pcVar6;
  
  iVar4 = param_1[0x8209];
  if (iVar4 < (int)param_1[0x820b]) {
    do {
      iVar5 = param_1[0x8208];
      if (iVar5 < (int)param_1[0x820a]) {
        pcVar6 = (char *)(iVar5 * 0x100 + iVar4 + 8 + (int)param_1);
        do {
          uVar3 = DAT_005411e0;
          uVar2 = DAT_005411dc;
          if (((((int)param_1[0x8208] <= iVar5) && ((int)param_1[0x8209] <= iVar4)) &&
              (iVar5 <= (int)param_1[0x820a])) &&
             ((iVar4 <= (int)param_1[0x820b] && (param_1[*pcVar6 + 0x8102] == (int)param_2)))) {
            param_1[0x807a] = DAT_005411dc;
            param_1[0x805a] = uVar3;
            param_1[0x803f] = uVar2;
            iVar1 = param_1[*pcVar6 + 0x8002];
            if (iVar1 != 0) {
              uVar2 = *param_1;
              uVar3 = FUN_004c04b0(param_1,iVar5,iVar4);
              FUN_004bfe50(param_1 + 0x8202,(float)iVar5,(float)iVar4,iVar1,uVar3,uVar2,
                           (int)pcVar6[0x10000]);
            }
          }
          iVar5 = iVar5 + 1;
          pcVar6 = pcVar6 + 0x100;
        } while (iVar5 < (int)param_1[0x820a]);
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < (int)param_1[0x820b]);
  }
  return;
}


// ==== FUN_004bfe50 @ 004bfe50

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_004bfe50(void *this,undefined4 param_2,int param_3,int param_4)

{
  undefined1 *puVar1;
  float fVar2;
  char cVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  int *piVar9;
  undefined4 unaff_EBX;
  int unaff_retaddr;
  float fStack_8;
  
  piVar4 = (int *)FUN_004af530(param_4,*(undefined4 *)this);
  *(int *)((int)this + 4) = ((int)piVar4 - DAT_005c8464) / 0x32;
  (**(code **)(*piVar4 + 0x74))(0);
  piVar4[9] = param_4;
  iVar5 = FUN_004ac550(piVar4);
  (**(code **)(*piVar4 + 0x90))(unaff_retaddr,param_2,0x2000061);
  if ((*(byte *)(iVar5 + 0xec) & 2) != 0) {
    if (param_3 == DAT_0050c038) {
      FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Terrainbuilder.cpp",0x4a);
      iVar6 = FUN_004e49c0();
      iVar7 = FUN_004e49c0();
      cVar3 = FUN_004322e0(&DAT_0054c8a0,(int)(iVar6 + (iVar6 >> 0x1f & 0xfU)) >> 4,
                           (int)(iVar7 + (iVar7 >> 0x1f & 0xfU)) >> 4);
      param_3 = (int)cVar3;
    }
    FUN_004acd20(piVar4,param_3);
    puVar1 = (undefined1 *)(*(int *)(iVar5 + 0x124) + unaff_retaddr * 4);
    if ((puVar1[3] & 1) != 0) {
      uVar8 = FUN_0049a840(DAT_005411f0);
      do {
        iVar5 = FUN_0049aa90(uVar8,*puVar1,puVar1[1],0xffffffff,0xffffffff);
        iVar6 = FUN_0049a840(DAT_005411f0);
      } while ((*(uint *)((int)this + 8) & (int)*(char *)(*(int *)(iVar6 + 0x124) + 3 + iVar5 * 4))
               != 0);
      if (iVar5 != -1) {
        if (*(int *)this == 0) {
          FUN_004e0620("makePredictable","\\Ns\\O\\Terrainbuilder.cpp",0x5b);
        }
        piVar9 = (int *)FUN_004af530(DAT_005411f0,*(undefined4 *)this);
        fVar2 = fStack_8 + _DAT_00502314;
        piVar9[9] = iVar5;
        (**(code **)(*piVar9 + 0x90))(unaff_EBX,fVar2,0x2000061);
      }
    }
  }
  return ((int)piVar4 - DAT_005c8464) / 0x32;
}


// ==== FUN_004c00f0 @ 004c00f0

uint __thiscall
FUN_004c00f0(void *this,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  iVar1 = FUN_004c0090(this,DAT_0052f83c + param_2,DAT_0052f85c + param_3,param_4,param_5);
  uVar3 = -(uint)(iVar1 != 0) & DAT_0052f8dc;
  iVar1 = FUN_004c0090(this,DAT_0052f844 + param_2,DAT_0052f864 + param_3,param_4,param_5);
  uVar4 = -(uint)(iVar1 != 0) & DAT_0052f8e4;
  iVar1 = FUN_004c0090(this,DAT_0052f84c + param_2,DAT_0052f86c + param_3,param_4,param_5);
  uVar2 = -(uint)(iVar1 != 0) & DAT_0052f8ec;
  iVar1 = FUN_004c0090(this,DAT_0052f854 + param_2,DAT_0052f874 + param_3,param_4,param_5);
  return -(uint)(iVar1 != 0) & DAT_0052f8f4 | uVar2 | uVar4 | uVar3;
}


// ==== FUN_004c01e0 @ 004c01e0

uint __thiscall
FUN_004c01e0(void *this,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  iVar1 = FUN_004c0090(this,DAT_0052f858 + param_2,DAT_0052f878 + param_3,param_4,param_5);
  uVar3 = -(uint)(iVar1 != 0) & DAT_0052f8f8;
  iVar1 = FUN_004c0090(this,DAT_0052f840 + param_2,DAT_0052f860 + param_3,param_4,param_5);
  uVar4 = -(uint)(iVar1 != 0) & DAT_0052f8e0;
  iVar1 = FUN_004c0090(this,DAT_0052f848 + param_2,DAT_0052f868 + param_3,param_4,param_5);
  uVar2 = -(uint)(iVar1 != 0) & DAT_0052f8e8;
  iVar1 = FUN_004c0090(this,DAT_0052f850 + param_2,DAT_0052f870 + param_3,param_4,param_5);
  return -(uint)(iVar1 != 0) & DAT_0052f8f0 | uVar2 | uVar4 | uVar3;
}


// ==== FUN_004c0090 @ 004c0090

undefined4 __thiscall FUN_004c0090(void *this,int param_2,int param_3,char param_4,char param_5)

{
  if ((((*(int *)((int)this + 0x20820) <= param_2) && (*(int *)((int)this + 0x20824) <= param_3)) &&
      (param_2 <= *(int *)((int)this + 0x20828))) &&
     (((param_3 <= *(int *)((int)this + 0x2082c) &&
       (param_3 = param_2 * 0x100 + param_3,
       *(int *)((int)this + *(char *)(param_3 + 8 + (int)this) * 4 + 0x20408) == (int)param_4)) &&
      (*(char *)((int)this + param_3 + 0x10008) == param_5)))) {
    return 1;
  }
  return 0;
}


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


// ==== FUN_0049aa90 @ 0049aa90

undefined4 __thiscall FUN_0049aa90(void *this,char param_2,int param_3,uint param_4,int param_5)

{
  void *_Memory;
  char *pcVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb388;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  _Memory = operator_new(0x400);
  iVar3 = 0;
  iVar2 = 0;
  pcVar1 = *(char **)((int)this + 0x124);
  local_4 = 0;
  if (0 < *(int *)((int)this + 0x114)) {
    do {
      if ((((*pcVar1 == param_2) && (pcVar1[1] == (char)param_3)) &&
          ((param_4 == (int)pcVar1[3] ||
           ((param_4 == 0xffffffff || ((param_4 & (int)pcVar1[3]) != 0)))))) && (iVar3 < 0x100)) {
        *(int *)((int)_Memory + iVar3 * 4) = iVar2;
        iVar3 = iVar3 + 1;
      }
      iVar2 = iVar2 + 1;
      pcVar1 = pcVar1 + 4;
    } while (iVar2 < *(int *)((int)this + 0x114));
    if (iVar3 != 0) goto LAB_0049ac5a;
  }
  if (param_4 == 0xffffffff) {
    pcVar1 = *(char **)((int)this + 0x124);
    FUN_0041cd20((int)param_2,(int)(char)param_3,&param_3,&param_4);
    iVar2 = 0;
    if (0 < *(int *)((int)this + 0x114)) {
      do {
        if (((*pcVar1 == param_3) && ((int)pcVar1[1] == param_4)) && (iVar3 < 0x100)) {
          *(int *)((int)_Memory + iVar3 * 4) = iVar2;
          iVar3 = iVar3 + 1;
        }
        iVar2 = iVar2 + 1;
        pcVar1 = pcVar1 + 4;
      } while (iVar2 < *(int *)((int)this + 0x114));
      if (iVar3 != 0) goto LAB_0049ac5a;
    }
    pcVar1 = *(char **)((int)this + 0x124);
    iVar2 = 0;
    if (0 < *(int *)((int)this + 0x114)) {
      do {
        if (((*pcVar1 == param_2) && (pcVar1[1] == 'A')) && (iVar3 < 0x100)) {
          *(int *)((int)_Memory + iVar3 * 4) = iVar2;
          iVar3 = iVar3 + 1;
        }
        iVar2 = iVar2 + 1;
        pcVar1 = pcVar1 + 4;
      } while (iVar2 < *(int *)((int)this + 0x114));
      if (iVar3 != 0) goto LAB_0049ac5a;
    }
    pcVar1 = *(char **)((int)this + 0x124);
    iVar2 = 0;
    if (0 < *(int *)((int)this + 0x114)) {
      do {
        if ((*pcVar1 == param_2) && (iVar3 < 0x100)) {
          *(int *)((int)_Memory + iVar3 * 4) = iVar2;
          iVar3 = iVar3 + 1;
        }
        iVar2 = iVar2 + 1;
        pcVar1 = pcVar1 + 4;
      } while (iVar2 < *(int *)((int)this + 0x114));
      if (iVar3 != 0) {
LAB_0049ac5a:
        if (param_5 == -1) {
          if (iVar3 == 0) {
            local_4 = 0xffffffff;
            _free(_Memory);
            uVar4 = 0;
          }
          else {
            iVar2 = FUN_004558c0(iVar3);
            uVar4 = *(undefined4 *)((int)_Memory + iVar2 * 4);
            local_4 = 0xffffffff;
            _free(_Memory);
          }
        }
        else {
          iVar2 = (int)(iVar3 + (iVar3 >> 0x1f & 3U)) >> 2;
          iVar2 = FUN_004558f0(iVar2 * param_5 + 1,(param_5 + 1) * iVar2);
          uVar4 = *(undefined4 *)((int)_Memory + iVar2 * 4);
          local_4 = 0xffffffff;
          _free(_Memory);
        }
        ExceptionList = local_c;
        return uVar4;
      }
    }
  }
  local_4 = 0xffffffff;
  _free(_Memory);
  ExceptionList = local_c;
  return 0xffffffff;
}


// ==== FUN_004437f0 @ 004437f0

void __cdecl FUN_004437f0(void)

{
  return;
}


// ==== _rand @ 004e64ab

/* Library Function - Single Match
    _rand
   
   Libraries: Visual Studio 2003 Release, Visual Studio 2005 Release, Visual Studio 2008 Release */

int __cdecl _rand(void)

{
  _ptiddata p_Var1;
  uint uVar2;
  
  p_Var1 = __getptd();
  uVar2 = p_Var1->_holdrand * 0x343fd + 0x269ec3;
  p_Var1->_holdrand = uVar2;
  return uVar2 >> 0x10 & 0x7fff;
}


// ==== FUN_004e649e @ 004e649e

void __cdecl FUN_004e649e(ulong param_1)

{
  _ptiddata p_Var1;
  
  p_Var1 = __getptd();
  p_Var1->_holdrand = param_1;
  return;
}


