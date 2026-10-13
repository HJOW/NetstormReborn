// ==== FUN_0046edf0 @ 0046edf0

void __thiscall FUN_0046edf0(void *this)

{
  undefined4 *puVar1;
  int iVar2;
  
  puVar1 = (undefined4 *)((int)this + 0x18);
  iVar2 = 100;
  do {
    puVar1[-5] = 0;
    puVar1[-4] = 0;
    puVar1[-3] = 0;
    *(undefined1 *)(puVar1 + -2) = 0xff;
    *(undefined1 *)((int)puVar1 + -7) = 0;
    *puVar1 = 0xc61c4000;
    puVar1[-1] = 0xc61c4000;
    puVar1[2] = 0xc61c4000;
    puVar1[1] = 0xc61c4000;
    puVar1 = puVar1 + 8;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined4 *)this = 0;
  return;
}


// ==== FUN_0046ee30 @ 0046ee30

undefined4 __thiscall FUN_0046ee30(void *this,int param_2)

{
  if ((-1 < param_2) && (param_2 < *(int *)this)) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_0046ee50 @ 0046ee50

undefined4 __thiscall FUN_0046ee50(void *this)

{
  return *(undefined4 *)this;
}


// ==== FUN_0046ee60 @ 0046ee60

void __thiscall FUN_0046ee60(void *this,undefined4 param_2,undefined1 param_3)

{
  int iVar1;
  
  iVar1 = *(int *)this;
  *(int *)this = iVar1 + 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 0xc) = param_2;
  *(char *)((int)this + iVar1 * 0x20 + 0x11) = (char)param_2;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 8) = 1;
  *(undefined1 *)((int)this + iVar1 * 0x20 + 0x10) = param_3;
  return;
}


// ==== FUN_0046ee90 @ 0046ee90

void __thiscall FUN_0046ee90(void *this,int param_2)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x285);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (param_2 = param_2 * 0x20, *(int *)(param_2 + 8 + (int)this) != 0)) {
    *(undefined4 *)((int)this + param_2 + 4) = 0;
    *(undefined4 *)((int)this + param_2 + 8) = 0;
    *(undefined4 *)((int)this + param_2 + 0xc) = 0;
    *(undefined1 *)((int)this + param_2 + 0x10) = 0xff;
    *(undefined1 *)((int)this + param_2 + 0x11) = 0;
    *(undefined4 *)((int)this + param_2 + 0x18) = 0xc61c4000;
    *(undefined4 *)((int)this + param_2 + 0x14) = 0xc61c4000;
    *(undefined4 *)((int)this + param_2 + 0x20) = 0xc61c4000;
    *(undefined4 *)((int)this + param_2 + 0x1c) = 0xc61c4000;
  }
  return;
}


// ==== FUN_0046ef10 @ 0046ef10

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0046ef10(void *this,int param_2,int param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  float local_8;
  float local_4;
  
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x28f);
  }
  fVar3 = _DAT_00500fc4;
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     ((*(int *)((int)this + param_2 * 0x20 + 8) != 0 && ((DAT_00594fb8 == 0 || (DAT_005c85a4 != 0)))
      ))) {
    *(int *)((int)this + param_2 * 0x20 + 0xc) = param_3;
    DAT_0059a8b0 = DAT_0059a8b0 + 1;
    DAT_005949d4 = 1;
    fVar1 = *(float *)((int)this + param_2 * 0x20 + 0x14);
    fVar2 = *(float *)((int)this + param_2 * 0x20 + 0x18);
    local_8 = *(float *)((int)this + param_2 * 0x20 + 0x1c);
    local_4 = *(float *)((int)this + param_2 * 0x20 + 0x20);
    if ((fVar1 <= fVar3) ||
       ((((((float)DAT_00531928 <= fVar1 || (fVar2 <= _DAT_00500fc4)) ||
          ((float)DAT_00531928 <= fVar2)) ||
         ((iVar5 = FUN_0040e800(&local_8), iVar5 == 0 || (fVar1 < local_8 == (fVar1 == local_8)))))
        || (fVar2 < local_4 == (fVar2 == local_4))))) {
      FUN_004e0620("bcr.isValid()","\\Ns\\O\\Islandlist.cpp",0x2a4);
    }
    iVar7 = 0;
    iVar5 = DAT_005c8464;
    if (0 < DAT_0059a9e0) {
      do {
        iVar8 = *(int *)(DAT_0059a9d8 + iVar7 * 4) * 0x32;
        piVar9 = (int *)(iVar8 + iVar5);
        if (*(byte *)(iVar8 + 10 + iVar5) == DAT_005411bc) {
          fVar3 = *(float *)((int)piVar9 + 0xe);
          fVar4 = *(float *)((int)piVar9 + 0x12);
          if (((fVar1 < fVar3 != (fVar1 == fVar3)) && (fVar3 < local_8 != (fVar3 == local_8))) &&
             ((fVar2 < fVar4 != (fVar2 == fVar4) && (fVar4 < local_4 != (fVar4 == local_4))))) {
            iVar8 = *piVar9;
            iVar6 = FUN_0040eaf0(fVar3,fVar4);
            (**(code **)(iVar8 + 0x74))(*(undefined1 *)(iVar6 * 0x32 + 0x22 + iVar5));
            iVar5 = DAT_005c8464;
          }
        }
        iVar7 = iVar7 + 1;
      } while (iVar7 < DAT_0059a9e0);
    }
    if ((param_3 != 0) && (*(int *)(param_3 * 0xb4 + 0x78 + DAT_0059534c) == 0x7f)) {
      *(int *)(param_3 * 0xb4 + 0x78 + DAT_0059534c) = param_2;
    }
  }
  return;
}


// ==== FUN_0046f130 @ 0046f130

void __thiscall FUN_0046f130(void *this,undefined4 *param_2,int param_3)

{
  undefined4 *puVar1;
  
  if (((param_3 < 0) || (*(int *)this <= param_3)) ||
     (*(int *)(param_3 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2ba);
  }
  puVar1 = (undefined4 *)(param_3 * 0x20 + 0x14 + (int)this);
  *param_2 = *puVar1;
  param_2[1] = puVar1[1];
  param_2[2] = puVar1[2];
  param_2[3] = puVar1[3];
  return;
}


// ==== FUN_0046f190 @ 0046f190

void __thiscall
FUN_0046f190(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2c0);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (param_2 = param_2 * 0x20, *(int *)(param_2 + 8 + (int)this) != 0)) {
    *(undefined4 *)((int)this + param_2 + 0x14) = param_3;
    *(undefined4 *)((int)this + param_2 + 0x18) = param_4;
    *(undefined4 *)((int)this + param_2 + 0x1c) = param_5;
    *(undefined4 *)((int)this + param_2 + 0x20) = param_6;
  }
  return;
}


// ==== FUN_0046f200 @ 0046f200

undefined4 __thiscall FUN_0046f200(void *this,int param_2,int param_3,int param_4)

{
  char cVar1;
  
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (*(int *)((int)this + param_2 * 0x20 + 8) != 0)) {
    if ((DAT_005c85a4 == 0) && (DAT_00594fa4 == 0)) {
      if (*(char *)((int)this + param_2 * 0x20 + 0x11) != param_3) {
        return 0;
      }
      cVar1 = *(char *)((int)this + param_2 * 0x20 + 0x10);
    }
    else {
      if (*(int *)((int)this + param_2 * 0x20 + 0xc) == 0) {
        return 0;
      }
      cVar1 = *(char *)((int)this + param_2 * 0x20 + 0x10);
    }
    if (cVar1 == param_4) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_0046f260 @ 0046f260

undefined4 __thiscall FUN_0046f260(void *this,int param_2)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2dd);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (*(int *)(param_2 * 0x20 + 8 + (int)this) != 0)) {
    return *(undefined4 *)((int)this + param_2 * 0x20 + 4);
  }
  return 0;
}


// ==== FUN_0046f2c0 @ 0046f2c0

void __thiscall FUN_0046f2c0(void *this,int param_2,undefined4 param_3)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2f2);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (*(int *)(param_2 * 0x20 + 8 + (int)this) != 0)) {
    *(undefined4 *)((int)this + param_2 * 0x20 + 4) = param_3;
  }
  return;
}


// ==== FUN_0046f320 @ 0046f320

undefined4 __thiscall FUN_0046f320(void *this,int param_2)

{
  if (((param_2 < 0) || (*(int *)this <= param_2)) ||
     (*(int *)(param_2 * 0x20 + 8 + (int)this) == 0)) {
    FUN_004e0620("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2fb);
  }
  if (((-1 < param_2) && (param_2 < *(int *)this)) &&
     (*(int *)(param_2 * 0x20 + 8 + (int)this) != 0)) {
    return *(undefined4 *)((int)this + param_2 * 0x20 + 0xc);
  }
  return 0;
}


