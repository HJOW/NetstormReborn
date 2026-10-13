// ==== FUN_004bd3f0 @ 004bd3f0

void __thiscall FUN_004bd3f0(void *this)

{
  *(undefined1 *)((int)this + 0xc) = 0xff;
  *(undefined4 *)this = 0;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined1 *)((int)this + 0xd) = 0;
  *(undefined4 *)((int)this + 0x14) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x10) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x1c) = 0xc61c4000;
  *(undefined4 *)((int)this + 0x18) = 0xc61c4000;
  return;
}


// ==== FUN_004bd450 @ 004bd450

void __thiscall FUN_004bd450(void *this)

{
  int iVar1;
  int iVar2;
  
  iVar1 = 100;
  iVar2 = (int)this + 4;
  do {
    FUN_004bd3f0(iVar2);
    iVar1 = iVar1 + -1;
    iVar2 = iVar2 + 0x20;
  } while (iVar1 != 0);
  *(undefined4 *)this = 0;
  return;
}


// ==== FUN_004bd480 @ 004bd480

undefined4 __thiscall FUN_004bd480(int *param_1,int param_2)

{
  if ((-1 < param_2) && (param_2 < *param_1)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004bd4c0 @ 004bd4c0

undefined4 __thiscall FUN_004bd4c0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd480(param_2);
  if (iVar1 != 0) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_004bd4f0 @ 004bd4f0

undefined4 __thiscall FUN_004bd4f0(void *this)

{
  return *(undefined4 *)this;
}


// ==== FUN_004bd500 @ 004bd500

void __thiscall FUN_004bd500(void *this,undefined4 param_2,undefined1 param_3)

{
  int iVar1;
  
  iVar1 = FUN_004bd4f0();
  *(int *)this = *(int *)this + 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 8) = 1;
  *(undefined4 *)((int)this + iVar1 * 0x20 + 0xc) = param_2;
  *(char *)((int)this + iVar1 * 0x20 + 0x11) = (char)param_2;
  *(undefined1 *)((int)this + iVar1 * 0x20 + 0x10) = param_3;
  return;
}


// ==== FUN_004bd640 @ 004bd640

void __thiscall FUN_004bd640(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 != 0) {
    FUN_004bd3f0(param_2 * 0x20 + 4 + (int)this);
  }
  return;
}


// ==== FUN_004bd670 @ 004bd670

void __thiscall FUN_004bd670(void *this,int param_2,int param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int *piVar10;
  int iVar11;
  
  iVar6 = FUN_004bd4c0(this,param_2);
  if ((iVar6 != 0) && ((DAT_00540a1c == 0 || (DAT_00518904 != 0)))) {
    iVar6 = param_2 * 0x20;
    DAT_0052039c = DAT_0052039c + 1;
    *(int *)(iVar6 + 0xc + (int)this) = param_3;
    fVar1 = *(float *)((int)this + iVar6 + 0x14);
    fVar2 = *(float *)((int)this + iVar6 + 0x18);
    fVar3 = *(float *)((int)this + iVar6 + 0x1c);
    fVar4 = *(float *)((int)this + iVar6 + 0x20);
    if ((((int)fVar1 < 1) ||
        (((fVar5 = (float)DAT_0052e9a8, fVar5 <= fVar1 || ((int)fVar2 < 1)) || (fVar5 <= fVar2))))
       || (((((int)fVar3 < 1 || (fVar5 <= fVar3)) ||
            (((int)fVar4 < 1 || ((fVar5 <= fVar4 || (fVar3 < fVar1)))))) || (fVar4 < fVar2)))) {
      FUN_00490040(s_bcr_isValid___0053d304,s__ns_o_islandList_cpp_0053d2ec,0x119);
    }
    iVar11 = 0;
    for (iVar6 = 0; iVar6 < DAT_00565ae8; iVar6 = iVar6 + 1) {
      piVar10 = (int *)(DAT_005395dc + *(int *)(DAT_00565ae0 + iVar11) * 0x24);
      if (*(byte *)((int)piVar10 + 10) == DAT_0051caa8) {
        if ((((fVar1 <= *(float *)((int)piVar10 + 0xe)) && (*(float *)((int)piVar10 + 0xe) <= fVar3)
             ) && (fVar2 <= *(float *)((int)piVar10 + 0x12))) &&
           (*(float *)((int)piVar10 + 0x12) <= fVar4)) {
          iVar7 = __ftol();
          iVar8 = __ftol();
          if (((iVar7 < 0) || (iVar8 < 0)) || ((0xff < iVar7 || (0xff < iVar8)))) {
            uVar9 = 0;
          }
          else {
            uVar9 = (uint)*(ushort *)(DAT_005670cc + (iVar8 * 0x100 + iVar7) * 2);
          }
          (**(code **)(*piVar10 + 0x74))(*(undefined1 *)(uVar9 * 0x24 + 0x20 + DAT_005395dc));
        }
      }
      iVar11 = iVar11 + 4;
    }
    if ((param_3 != 0) &&
       (piVar10 = (int *)(param_3 * 0xac + 0x70 + DAT_0050f834), *piVar10 == 0x7f)) {
      *piVar10 = param_2;
    }
  }
  return;
}


// ==== FUN_004bd8a0 @ 004bd8a0

void __thiscall FUN_004bd8a0(void *this,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = FUN_004bd4c0(this,param_3);
  if (iVar2 == 0) {
    FUN_00490040(s_exists__islandId___0053d32c,s__ns_o_islandList_cpp_0053d314,0x12f);
  }
  uVar1 = *(undefined4 *)((int)this + param_3 * 0x20 + 0x18);
  *param_2 = *(undefined4 *)((int)this + param_3 * 0x20 + 0x14);
  param_2[1] = uVar1;
  uVar1 = *(undefined4 *)((int)this + param_3 * 0x20 + 0x20);
  param_2[2] = *(undefined4 *)((int)this + param_3 * 0x20 + 0x1c);
  param_2[3] = uVar1;
  return;
}


// ==== FUN_004bd900 @ 004bd900

void __thiscall
FUN_004bd900(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 != 0) {
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x14) = param_3;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x18) = param_4;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x1c) = param_5;
    *(undefined4 *)((int)this + param_2 * 0x20 + 0x20) = param_6;
  }
  return;
}


// ==== FUN_004bd940 @ 004bd940

undefined4 __thiscall FUN_004bd940(void *this,int param_2,int param_3,int param_4)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 == 0) {
    return 0;
  }
  param_2 = param_2 * 0x20;
  if ((DAT_00518904 == 0) && (DAT_0052e170 == 0)) {
    if ((*(char *)((int)this + param_2 + 0x11) == param_3) &&
       (*(char *)((int)this + param_2 + 0x10) == param_4)) {
      return 1;
    }
    return 0;
  }
  if ((*(int *)((int)this + param_2 + 0xc) != 0) &&
     (*(char *)((int)this + param_2 + 0x10) == param_4)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004bd9c0 @ 004bd9c0

undefined4 __thiscall FUN_004bd9c0(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 == 0) {
    return 0;
  }
  return *(undefined4 *)(param_2 * 0x20 + 4 + (int)this);
}


// ==== FUN_004bda40 @ 004bda40

void __thiscall FUN_004bda40(void *this,int param_2,undefined4 param_3)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 != 0) {
    *(undefined4 *)(param_2 * 0x20 + 4 + (int)this) = param_3;
  }
  return;
}


// ==== FUN_004bda70 @ 004bda70

undefined4 __thiscall FUN_004bda70(void *this,int param_2)

{
  int iVar1;
  
  iVar1 = FUN_004bd4c0(this,param_2);
  if (iVar1 == 0) {
    return 0;
  }
  return *(undefined4 *)(param_2 * 0x20 + 0xc + (int)this);
}


