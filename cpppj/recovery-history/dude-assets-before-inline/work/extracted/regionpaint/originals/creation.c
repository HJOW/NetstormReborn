// ==== FUN_00470900 @ 00470900

void __thiscall
FUN_00470900(void *this,int param_2,int param_3,undefined4 param_4,undefined4 param_5)

{
  uint uVar1;
  
  FUN_0046fd20(&DAT_0055a6f8,0,0,0);
  *(undefined4 *)((int)this + 4) = 0x43808000;
  *(undefined4 *)((int)this + 8) = 0x43808000;
  *(undefined4 *)((int)this + 0xc) = 0xbf800000;
  *(undefined4 *)((int)this + 0x10) = 0xbf800000;
  if (((((-1 < param_2) && (-1 < param_3)) && (param_2 < 0x100)) &&
      ((param_3 < 0x100 &&
       (uVar1 = (uint)*(ushort *)(DAT_005c84bc + (param_3 * 0x100 + param_2) * 2), uVar1 != 0)))) &&
     (*(byte *)(uVar1 * 0x32 + 10 + DAT_005c8464) == DAT_005411dc)) {
    *(undefined4 *)((int)this + 0x14) = param_4;
    *(undefined4 *)((int)this + 0x18) = param_5;
    FUN_0046fe40(this,param_2,param_3);
  }
  for (uVar1 = 0; (DAT_0055a6fc != 0 && (uVar1 < (uint)(DAT_0055a700 - DAT_0055a6fc >> 3)));
      uVar1 = uVar1 + 1) {
    FUN_004703b0(this,*(undefined4 *)(DAT_0055a6fc + uVar1 * 8),
                 *(undefined4 *)(DAT_0055a6fc + 4 + uVar1 * 8));
  }
  DAT_0059a8b0 = DAT_0059a8b0 + 1;
  return;
}


// ==== FUN_0046fe40 @ 0046fe40

void __thiscall FUN_0046fe40(void *this,int param_2,int param_3)

{
  void *pvVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int *piVar11;
  void *pvVar12;
  void *pvStack_70;
  undefined4 uStack_6c;
  uint uStack_68;
  char *pcStack_64;
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  undefined4 uStack_54;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined4 uStack_40;
  undefined4 uStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  while( true ) {
    if ((((param_2 < 0) || (param_3 < 0)) || (0xff < param_2)) ||
       ((0xff < param_3 ||
        (uVar9 = (uint)*(ushort *)(DAT_005c84bc + (param_3 * 0x100 + param_2) * 2), uVar9 == 0)))) {
      FUN_0046fd90(param_2,param_3);
      return;
    }
    iVar10 = uVar9 * 0x32;
    piVar11 = (int *)(iVar10 + DAT_005c8464);
    if (*(byte *)(iVar10 + 10 + DAT_005c8464) != DAT_005411dc) {
      return;
    }
    if ((uint)*(byte *)((int)piVar11 + 0x22) == *(uint *)((int)this + 0x14)) break;
    (**(code **)(*piVar11 + 0x74))(*(uint *)((int)this + 0x14));
    iVar10 = FUN_004ac550(piVar11);
    pvVar12 = DAT_00568b2c;
    pcStack_64 = (char *)(*(int *)this + piVar11[9] * 4);
    pvVar1 = *(void **)(&DAT_0052f80c + *(char *)(*(int *)this + piVar11[9] * 4) * 4);
    pvStack_70 = pvVar1;
    if (DAT_00568b2c == (void *)0x0) {
      DAT_00568b2c = operator_new(0x18c);
      FUN_004437f0();
      pvStack_70 = pvVar12;
      do {
        uStack_6c = 0;
        uStack_68 = FUN_004bca00(&uStack_6c);
        pvVar12 = DAT_00568b2c;
        *(uint *)((int)pvStack_70 + (int)DAT_00568b2c) = uStack_68;
        if (0 < (int)pvStack_70) {
          uVar9 = *(uint *)(((int)pvStack_70 - 4U) + (int)pvVar12) & 0x80000003;
          if ((int)uVar9 < 0) {
            uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
          }
          uVar5 = uStack_68 & 0x80000003;
          if ((int)uVar5 < 0) {
            uVar5 = (uVar5 - 1 | 0xfffffffc) + 1;
          }
          if (uVar5 == uVar9) {
            *(uint *)((int)pvStack_70 + (int)pvVar12) = uStack_68 + 1;
          }
        }
        pvStack_70 = (void *)((int)pvStack_70 + 4);
      } while ((int)pvStack_70 < 0x18c);
    }
    if ((*pcStack_64 == 'A') && (pcStack_64[1] == 'A')) {
      iVar7 = *(int *)((int)this + 0x18);
      if ((iVar7 == 0) || (iVar7 == -1)) {
        uStack_18 = *(undefined4 *)((int)piVar11 + 0xe);
        uStack_14 = *(undefined4 *)((int)piVar11 + 0x12);
        iVar7 = *(int *)((int)pvVar12 + ((param_2 / 3) % 99) * 4);
        iVar8 = *(int *)((int)pvVar12 + ((param_3 / 3) % 99) * 4);
        uStack_10 = uStack_18;
        uStack_c = uStack_14;
        uStack_8 = uStack_18;
        uStack_4 = uStack_14;
        iVar6 = FUN_004e49c0();
        uVar9 = iVar6 / 3 + *(int *)((int)pvVar12 + ((iVar7 + iVar8) % 99) * 4) & 0x80000003;
        if ((int)uVar9 < 0) {
          uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
        }
        iVar7 = FUN_004e49c0();
        iVar8 = FUN_004e49c0();
        iVar10 = (uVar9 * 3 + iVar7 % 3) * 3 + iVar8 % 3 + 1 + *(int *)(iVar10 + 0x154);
      }
      else {
        uVar2 = *(undefined4 *)((int)piVar11 + 0xe);
        if (iVar7 == 1) {
          uStack_5c = *(undefined4 *)((int)piVar11 + 0x12);
          iVar7 = *(int *)((int)pvVar12 + ((param_2 / 3) % 99) * 4);
          iVar8 = *(int *)((int)pvVar12 + ((param_3 / 3) % 99) * 4);
          uStack_60 = uVar2;
          uStack_58 = uVar2;
          uStack_54 = uStack_5c;
          uStack_50 = uVar2;
          uStack_4c = uStack_5c;
          iVar6 = FUN_004e49c0();
          uVar9 = iVar6 / 3 + *(int *)((int)pvVar12 + ((iVar7 + iVar8) % 99) * 4) & 0x80000003;
          if ((int)uVar9 < 0) {
            uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
          }
          iVar7 = FUN_004e49c0();
          iVar8 = FUN_004e49c0();
          iVar10 = (uVar9 * 3 + iVar7 % 3) * 3 + iVar8 % 3 + 0x25 + *(int *)(iVar10 + 0x154);
        }
        else {
          uVar3 = *(undefined4 *)((int)piVar11 + 0x12);
          uVar4 = *(undefined4 *)((int)piVar11 + 0xe);
          if (iVar7 == 2) {
            iVar7 = *(int *)((int)pvVar12 + ((param_2 / 3) % 99) * 4);
            iVar8 = *(int *)((int)pvVar12 + ((param_3 / 3) % 99) * 4);
            uStack_48 = uVar2;
            uStack_44 = uVar3;
            uStack_40 = uVar4;
            uStack_3c = uVar3;
            uStack_38 = uVar4;
            uStack_34 = uVar3;
            iVar6 = FUN_004e49c0();
            uVar9 = iVar6 / 3 + *(int *)((int)pvVar12 + ((iVar7 + iVar8) % 99) * 4) & 0x80000003;
            if ((int)uVar9 < 0) {
              uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
            }
            iVar7 = FUN_004e49c0();
            iVar8 = FUN_004e49c0();
            iVar10 = (uVar9 * 3 + iVar7 % 3) * 3 + iVar8 % 3 + 0x49 + *(int *)(iVar10 + 0x154);
          }
          else {
            uStack_20 = *(undefined4 *)((int)piVar11 + 0xe);
            uStack_24 = *(undefined4 *)((int)piVar11 + 0x12);
            iVar7 = *(int *)((int)pvVar12 + ((param_2 / 3) % 99) * 4);
            iVar8 = *(int *)((int)pvVar12 + ((param_3 / 3) % 99) * 4);
            uStack_30 = uVar2;
            uStack_2c = uVar3;
            uStack_28 = uVar4;
            uStack_1c = uStack_24;
            iVar6 = FUN_004e49c0();
            uVar9 = iVar6 / 3 + *(int *)((int)pvVar12 + ((iVar7 + iVar8) % 99) * 4) & 0x80000003;
            if ((int)uVar9 < 0) {
              uVar9 = (uVar9 - 1 | 0xfffffffc) + 1;
            }
            iVar7 = FUN_004e49c0();
            iVar8 = FUN_004e49c0();
            iVar10 = (uVar9 * 3 + iVar7 % 3) * 3 + iVar8 % 3 + 0x6d + *(int *)(iVar10 + 0x154);
          }
        }
      }
    }
    else {
      iVar10 = FUN_0049aa90(iVar10,CONCAT31((int3)((uint)pvStack_70 >> 8),*pcStack_64),
                            (int)pcStack_64[1],0xffffffff,*(undefined4 *)((int)this + 0x18));
    }
    piVar11[9] = iVar10;
    if (((uint)pvVar1 & 1) != 0) {
      FUN_0046fe40(this,param_2,param_3 + -1);
    }
    if (((uint)pvVar1 & 2) != 0) {
      FUN_0046fe40(this,param_2 + 1,param_3);
    }
    if (((uint)pvVar1 & 4) != 0) {
      FUN_0046fe40(this,param_2,param_3 + 1);
    }
    if (((uint)pvVar1 & 8) == 0) {
      return;
    }
    param_2 = param_2 + -1;
  }
  return;
}


// ==== FUN_004703b0 @ 004703b0

void __thiscall FUN_004703b0(void *this,int param_2,int param_3)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  int *piVar9;
  void *pvVar10;
  int unaff_retaddr;
  void *pvStack_24;
  int iStack_20;
  undefined4 uStack_18;
  char *pcStack_14;
  float fStack_10;
  char *pcStack_c;
  float local_8;
  float local_4;
  
  if ((((param_2 < 0) || (param_3 < 0)) || (0xff < param_2)) ||
     ((0xff < param_3 ||
      (uVar8 = (uint)*(ushort *)(DAT_005c84bc + (param_3 * 0x100 + param_2) * 2), uVar8 == 0)))) {
    local_8 = (float)param_2;
    local_4 = (float)param_3;
    uVar8 = FUN_004b1fa0(local_8,local_4,DAT_0054123c);
    if (uVar8 == 0) {
      return;
    }
  }
  local_8 = (float)param_2;
  local_4 = (float)param_3;
  uVar1 = FUN_0046f530(local_8,local_4);
  FUN_00455300(param_2,param_3,uVar1);
  uVar2 = (uint)*(byte *)(uVar8 * 0x32 + 10 + DAT_005c8464);
  piVar9 = (int *)(uVar8 * 0x32 + DAT_005c8464);
  if (uVar2 == DAT_005411dc) {
    if ((uint)*(byte *)((int)piVar9 + 0x22) == *(uint *)((int)this + 0x14)) {
      return;
    }
    if (uVar2 == DAT_005411dc) goto LAB_0047048b;
  }
  if (uVar2 != DAT_0054123c) {
    return;
  }
LAB_0047048b:
  (**(code **)(*piVar9 + 0x74))(*(undefined4 *)((int)this + 0x14));
  iVar3 = FUN_004ac550(piVar9);
  pvVar10 = DAT_00568b30;
  pvStack_24 = (void *)(int)*(char *)(*(int *)this + piVar9[9] * 4);
  pcStack_c = (char *)(*(int *)this + piVar9[9] * 4);
  uVar8 = *(uint *)(&DAT_0052f80c + (int)pvStack_24 * 4);
  if (DAT_00568b30 == (void *)0x0) {
    DAT_00568b30 = operator_new(0x18c);
    FUN_004437f0();
    pvStack_24 = pvVar10;
    do {
      uStack_18 = 0;
      pcStack_14 = (char *)FUN_004bca00(&uStack_18);
      pvVar10 = DAT_00568b30;
      *(char **)((int)pvStack_24 + (int)DAT_00568b30) = pcStack_14;
      if (0 < (int)pvStack_24) {
        uVar2 = *(uint *)((int)pvStack_24 + -4 + (int)pvVar10) & 0x80000003;
        if ((int)uVar2 < 0) {
          uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
        }
        uVar4 = (uint)pcStack_14 & 0x80000003;
        if ((int)uVar4 < 0) {
          uVar4 = (uVar4 - 1 | 0xfffffffc) + 1;
        }
        if (uVar4 == uVar2) {
          *(char **)((int)pvStack_24 + (int)pvVar10) = pcStack_14 + 1;
        }
      }
      pvStack_24 = (void *)((int)pvStack_24 + 4);
    } while ((int)pvStack_24 < 0x18c);
  }
  if ((*pcStack_c == 'A') && (pcStack_c[1] == 'A')) {
    iVar6 = *(int *)(iStack_20 + 0x18);
    if ((iVar6 == 0) || (iVar6 == -1)) {
      pcStack_14 = *(char **)((int)piVar9 + 0xe);
      fStack_10 = *(float *)((int)piVar9 + 0x12);
      iVar6 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
      iVar7 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
      pcStack_c = pcStack_14;
      local_8 = fStack_10;
      iVar5 = FUN_004e49c0();
      uVar2 = iVar5 / 3 + *(int *)((int)pvVar10 + ((iVar6 + iVar7) % 99) * 4) & 0x80000003;
      if ((int)uVar2 < 0) {
        uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
      }
      iVar6 = FUN_004e49c0();
      iVar7 = FUN_004e49c0();
      iVar3 = (uVar2 * 3 + iVar6 % 3) * 3 + iVar7 % 3 + 1 + *(int *)(iVar3 + 0x154);
    }
    else {
      pcStack_c = *(char **)((int)piVar9 + 0xe);
      if (iVar6 == 1) {
        fStack_10 = *(float *)((int)piVar9 + 0x12);
        iVar6 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
        iVar7 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
        pcStack_14 = pcStack_c;
        local_8 = fStack_10;
        iVar5 = FUN_004e49c0();
        uVar2 = iVar5 / 3 + *(int *)((int)pvVar10 + ((iVar6 + iVar7) % 99) * 4) & 0x80000003;
        if ((int)uVar2 < 0) {
          uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
        }
        iVar6 = FUN_004e49c0();
        iVar7 = FUN_004e49c0();
        iVar3 = (uVar2 * 3 + iVar6 % 3) * 3 + iVar7 % 3 + 0x25 + *(int *)(iVar3 + 0x154);
      }
      else {
        local_8 = *(float *)((int)piVar9 + 0x12);
        pcStack_14 = *(char **)((int)piVar9 + 0xe);
        fStack_10 = *(float *)((int)piVar9 + 0x12);
        if (iVar6 == 2) {
          iVar6 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
          iVar7 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
          iVar5 = FUN_004e49c0();
          uVar2 = iVar5 / 3 + *(int *)((int)pvVar10 + ((iVar6 + iVar7) % 99) * 4) & 0x80000003;
          if ((int)uVar2 < 0) {
            uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
          }
          iVar6 = FUN_004e49c0();
          iVar7 = FUN_004e49c0();
          iVar3 = (uVar2 * 3 + iVar6 % 3) * 3 + iVar7 % 3 + 0x49 + *(int *)(iVar3 + 0x154);
        }
        else {
          iVar6 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
          iVar7 = *(int *)((int)pvVar10 + ((param_2 / 3) % 99) * 4);
          iVar5 = FUN_004e49c0();
          uVar2 = iVar5 / 3 + *(int *)((int)pvVar10 + ((iVar6 + iVar7) % 99) * 4) & 0x80000003;
          if ((int)uVar2 < 0) {
            uVar2 = (uVar2 - 1 | 0xfffffffc) + 1;
          }
          iVar6 = FUN_004e49c0();
          iVar7 = FUN_004e49c0();
          iVar3 = (uVar2 * 3 + iVar6 % 3) * 3 + iVar7 % 3 + 0x6d + *(int *)(iVar3 + 0x154);
        }
      }
    }
  }
  else {
    iVar3 = FUN_0049aa90(iVar3,CONCAT31((int3)((uint)pvStack_24 >> 8),*pcStack_c),(int)pcStack_c[1],
                         0xffffffff,*(undefined4 *)(iStack_20 + 0x18));
    unaff_retaddr = param_2;
  }
  piVar9[9] = iVar3;
  if ((uVar8 & 1) != 0) {
    FUN_0046fd90(unaff_retaddr,param_2 + -1);
  }
  if ((uVar8 & 2) != 0) {
    FUN_0046fd90(unaff_retaddr + 1,param_2);
  }
  if ((uVar8 & 4) != 0) {
    FUN_0046fd90(unaff_retaddr,param_2 + 1);
  }
  if ((uVar8 & 8) != 0) {
    FUN_0046fd90(unaff_retaddr + -1,param_2);
  }
  return;
}


// ==== FUN_0046fd90 @ 0046fd90

void __cdecl FUN_0046fd90(int param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  int local_8;
  int local_4;
  
  iVar1 = DAT_0055a700;
  uVar2 = 0;
  while( true ) {
    if ((DAT_0055a6fc == 0) || ((uint)(DAT_0055a700 - DAT_0055a6fc >> 3) <= uVar2)) {
      local_8 = param_1;
      local_4 = param_2;
      if ((DAT_0055a6fc != 0) &&
         ((uint)(DAT_0055a700 - DAT_0055a6fc >> 3) < (uint)(DAT_0055a704 - DAT_0055a6fc >> 3))) {
        FUN_0046fa30(DAT_0055a700,1,&local_8,&DAT_0055a6f8,param_1);
        DAT_0055a700 = iVar1 + 8;
        return;
      }
      FUN_0046fad0(DAT_0055a700,1,&local_8);
      return;
    }
    if ((*(int *)(DAT_0055a6fc + uVar2 * 8) == param_1) &&
       (*(int *)(DAT_0055a6fc + 4 + uVar2 * 8) == param_2)) break;
    uVar2 = uVar2 + 1;
  }
  return;
}


// ==== FUN_00455300 @ 00455300

void __cdecl FUN_00455300(undefined4 param_1,undefined4 param_2,int param_3)

{
  int *piVar1;
  uint uVar2;
  undefined4 local_c;
  undefined4 local_8;
  int local_4;
  
  piVar1 = (int *)(DAT_0055a71c + 8);
  for (uVar2 = 0; (DAT_0055a71c != 0 && (uVar2 < (uint)((DAT_0055a720 - DAT_0055a71c) / 0xc)));
      uVar2 = uVar2 + 1) {
    if (*piVar1 == param_3) {
      return;
    }
    piVar1 = piVar1 + 3;
  }
  local_c = param_1;
  local_8 = param_2;
  local_4 = param_3;
  FUN_00455280(&DAT_0055a718,&local_c);
  return;
}


// ==== FUN_0046ebd0 @ 0046ebd0

void * __thiscall FUN_0046ebd0(void *this)

{
  int iVar1;
  
  *(undefined4 *)((int)this + 8) = 0xc61c4000;
  *(undefined4 *)((int)this + 4) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x10) = 0xc61c4000;
  *(undefined4 *)((int)this + 0xc) = 0xc61c4000;
  iVar1 = FUN_0049a840(DAT_005411dc);
  *(undefined4 *)this = *(undefined4 *)(iVar1 + 0x124);
  return this;
}


// ==== FUN_004bca00 @ 004bca00

uint __thiscall FUN_004bca00(void *this)

{
  int iVar1;
  
  if (*(int *)this == 0) {
    *(undefined4 *)this = 0xbad0bad;
  }
  iVar1 = *(int *)this * 0x10003 + 3;
  *(int *)this = iVar1;
  return iVar1 >> 0x10 & 0xffff;
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


// ==== FUN_0041cd20 @ 0041cd20

void __cdecl FUN_0041cd20(undefined4 param_1,int param_2,undefined4 *param_3,int *param_4)

{
  char cVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = FUN_0041cc70(param_1);
  cVar1 = (&DAT_0052f8fc)
          [*(uint *)(&DAT_0052f80c + iVar2 * 4) & *(uint *)(&DAT_0052f80c + param_2 * 4)];
  *param_4 = (int)cVar1;
  uVar3 = FUN_0041ccc0((int)cVar1);
  *param_3 = uVar3;
  return;
}


// ==== FUN_004e49c0 @ 004e49c0

ulonglong __cdecl FUN_004e49c0(void)

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


// ==== FUN_004ac550 @ 004ac550

void __thiscall FUN_004ac550(void *this)

{
  FUN_0049a840(*(undefined1 *)((int)this + 10));
  return;
}


// ==== FUN_0046f530 @ 0046f530

undefined4 FUN_0046f530(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  
  FUN_0041d7a0(&stack0x00000004);
  iVar1 = FUN_004e49c0();
  iVar2 = FUN_004e49c0();
  if ((((-1 < iVar2) && (-1 < iVar1)) && (iVar2 < 0x100)) &&
     ((iVar1 < 0x100 &&
      (uVar4 = (uint)*(ushort *)(DAT_005c84bc + (iVar1 * 0x100 + iVar2) * 2), uVar4 != 0)))) {
    uVar3 = FUN_004acda0(uVar4 * 0x32 + DAT_005c8464);
    return uVar3;
  }
  return 0x7f;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_004558c0 @ 004558c0

uint __cdecl FUN_004558c0(uint param_1)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % param_1;
}


// ==== FUN_004558f0 @ 004558f0

int __cdecl FUN_004558f0(int param_1,int param_2)

{
  if (DAT_00532710 == 0) {
    DAT_00532710 = 0xbad0bad;
  }
  DAT_00532710 = DAT_00532710 * 0x10003 + 3;
  return (DAT_00532710 >> 0x10) % (uint)(param_2 - param_1) + param_1;
}


// ==== FUN_0041cc70 @ 0041cc70

int __cdecl FUN_0041cc70(int param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar4 = 0;
  uVar2 = 0;
  do {
    uVar1 = uVar2 + 2;
    uVar3 = uVar1 & 0x80000007;
    if ((int)uVar3 < 0) {
      uVar3 = (uVar3 - 1 | 0xfffffff8) + 1;
    }
    if ((((&DAT_0052f8dc)[uVar2] & *(uint *)(&DAT_0052f80c + param_1 * 4)) != 0) &&
       ((*(uint *)(&DAT_0052f80c + param_1 * 4) & (&DAT_0052f8dc)[uVar3]) != 0)) {
      uVar4 = uVar4 | (&DAT_0052f8dc)[uVar3];
    }
    uVar2 = uVar1;
  } while ((int)uVar1 < 7);
  return (int)(char)(&DAT_0052f8fc)[uVar4];
}


// ==== FUN_0041ccc0 @ 0041ccc0

int __cdecl FUN_0041ccc0(int param_1)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  
  uVar4 = 0;
  uVar1 = 2;
  puVar2 = &DAT_0052f8e0;
  iVar3 = 4;
  do {
    if ((*puVar2 & *(uint *)(&DAT_0052f80c + param_1 * 4)) != 0) {
      uVar4 = uVar4 | (&DAT_0052f8dc)[uVar1 - 2 & 7] | (&DAT_0052f8dc)[uVar1 & 7];
    }
    puVar2 = puVar2 + 2;
    uVar1 = uVar1 + 2;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return (int)(char)(&DAT_0052f8fc)[uVar4];
}


// ==== FUN_0041d7a0 @ 0041d7a0

void __thiscall FUN_0041d7a0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004e49c0();
  *(float *)this = (float)iVar1;
  iVar1 = FUN_004e49c0();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== FUN_004acda0 @ 004acda0

uint __thiscall FUN_004acda0(void *this)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = FUN_0049a840(*(undefined1 *)((int)this + 10));
  if ((*(uint *)(iVar1 + 0xec) & 0x1000006) == 0) {
    FUN_004e0620("genus & (gISLAND|gBRIDGE|gISLAND3x3)","\\Ns\\O\\Squid.cpp",0x4cf);
    uVar2 = DAT_0050c038;
  }
  else {
    uVar2 = DAT_0050c038;
    if ((*(uint *)(iVar1 + 0xec) & 4) == 0) {
      uVar2 = (uint)*(ushort *)((int)this + 8);
    }
  }
  return uVar2;
}


