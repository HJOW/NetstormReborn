// ==== FUN_004bdaa0 @ 004bdaa0

int __thiscall FUN_004bdaa0(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar2 = 0;
  if (0 < *(int *)this) {
    piVar3 = (int *)((int)this + 0xc);
    do {
      iVar1 = FUN_004bd4c0(this,iVar2);
      if (iVar1 != 0) {
        if (DAT_00540a1c == 0) {
          if (*(char *)((int)piVar3 + 5) == param_2) {
LAB_004bdada:
            if (*piVar3 == param_2) goto LAB_004bdadf;
          }
        }
        else {
          if (*piVar3 != 0) goto LAB_004bdada;
LAB_004bdadf:
          if ((char)piVar3[1] == param_3) {
            return iVar2;
          }
        }
      }
      piVar3 = piVar3 + 8;
      iVar2 = iVar2 + 1;
    } while (iVar2 < *(int *)this);
  }
  return 0x7f;
}


// ==== FUN_004bdb10 @ 004bdb10

int __thiscall FUN_004bdb10(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  char *pcVar3;
  
  iVar2 = 0;
  if (0 < *(int *)this) {
    pcVar3 = (char *)((int)this + 0x10);
    do {
      iVar1 = FUN_004bd4c0(this,iVar2);
      if ((iVar1 != 0) && (*pcVar3 == param_2)) {
        return iVar2;
      }
      pcVar3 = pcVar3 + 0x20;
      iVar2 = iVar2 + 1;
    } while (iVar2 < *(int *)this);
  }
  return 0x7f;
}


// ==== FUN_004bdb60 @ 004bdb60

int __thiscall FUN_004bdb60(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = 0;
  iVar3 = 0;
  if (0 < *(int *)this) {
    do {
      iVar1 = FUN_004bd4c0(this,iVar3);
      if ((iVar1 != 0) && (iVar1 = FUN_004bda70(this,iVar3), iVar1 == param_2)) {
        iVar2 = iVar2 + 1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < *(int *)this);
  }
  return iVar2;
}


// ==== FUN_004bd4f0 @ 004bd4f0

undefined4 __thiscall FUN_004bd4f0(void *this)

{
  return *(undefined4 *)this;
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


