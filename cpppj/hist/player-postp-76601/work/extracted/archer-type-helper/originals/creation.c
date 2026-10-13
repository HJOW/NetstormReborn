// ==== FUN_004a87c0 @ 004a87c0

int __cdecl FUN_004a87c0(void)

{
  int *in_EAX;
  int iVar1;
  char *unaff_EBX;
  int iVar2;
  
  iVar1 = *in_EAX;
  iVar2 = 0;
  while( true ) {
    if (iVar1 == 0) {
      return -1;
    }
    iVar1 = __stricmp((char *)*in_EAX,unaff_EBX);
    if (iVar1 == 0) break;
    iVar1 = in_EAX[1];
    in_EAX = in_EAX + 1;
    iVar2 = iVar2 + 1;
  }
  return iVar2;
}


