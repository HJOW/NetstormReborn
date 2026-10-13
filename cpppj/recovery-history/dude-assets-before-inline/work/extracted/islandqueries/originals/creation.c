// ==== FUN_0046f380 @ 0046f380

int __thiscall FUN_0046f380(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar1 = *(int *)this;
  iVar2 = 0;
  if (0 < iVar1) {
    piVar3 = (int *)((int)this + 0xc);
    do {
      if (((-1 < iVar2) && (iVar2 < iVar1)) && (piVar3[-1] != 0)) {
        if (DAT_00594fb8 == 0) {
          if (*(char *)((int)piVar3 + 5) == param_2) {
LAB_0046f3c2:
            if (*piVar3 == param_2) goto LAB_0046f3c6;
          }
        }
        else {
          if (*piVar3 != 0) goto LAB_0046f3c2;
LAB_0046f3c6:
          if ((char)piVar3[1] == param_3) {
            return iVar2;
          }
        }
      }
      iVar2 = iVar2 + 1;
      piVar3 = piVar3 + 8;
    } while (iVar2 < iVar1);
  }
  return 0x7f;
}


// ==== FUN_0046f3f0 @ 0046f3f0

int __thiscall FUN_0046f3f0(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  char *pcVar3;
  
  iVar1 = *(int *)this;
  iVar2 = 0;
  if (0 < iVar1) {
    pcVar3 = (char *)((int)this + 0x10);
    do {
      if ((((-1 < iVar2) && (iVar2 < iVar1)) && (*(int *)(pcVar3 + -8) != 0)) &&
         (*pcVar3 == param_2)) {
        return iVar2;
      }
      iVar2 = iVar2 + 1;
      pcVar3 = pcVar3 + 0x20;
    } while (iVar2 < iVar1);
  }
  return 0x7f;
}


// ==== FUN_0046f430 @ 0046f430

int __thiscall FUN_0046f430(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  
  iVar2 = *(int *)this;
  iVar3 = 0;
  iVar4 = 0;
  if (0 < iVar2) {
    piVar5 = (int *)((int)this + 8);
    do {
      if (((-1 < iVar4) && (iVar4 < iVar2)) && (*piVar5 != 0)) {
        iVar2 = *(int *)this;
        if ((iVar4 < iVar2) && (*piVar5 != 0)) {
          iVar1 = piVar5[1];
        }
        else {
          iVar1 = 0;
        }
        if (iVar1 == param_2) {
          iVar3 = iVar3 + 1;
        }
      }
      iVar4 = iVar4 + 1;
      piVar5 = piVar5 + 8;
    } while (iVar4 < iVar2);
  }
  return iVar3;
}


// ==== FUN_0046ee50 @ 0046ee50

undefined4 __thiscall FUN_0046ee50(void *this)

{
  return *(undefined4 *)this;
}


// ==== FUN_0046ee30 @ 0046ee30

undefined4 __thiscall FUN_0046ee30(void *this,int param_2)

{
  if ((-1 < param_2) && (param_2 < *(int *)this)) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
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


