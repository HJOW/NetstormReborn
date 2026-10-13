// ==== FUN_0049e160 @ 0049e160

void __thiscall FUN_0049e160(void *this)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int *piVar10;
  int local_5b4;
  int local_5b0;
  int local_5ac;
  int local_5a8;
  int local_5a4;
  int local_5a0;
  int local_59c;
  int *local_594;
  int local_590 [100];
  int local_400 [256];
  
  local_400[0] = 0;
  piVar10 = local_400;
  for (iVar8 = 0xff; piVar10 = piVar10 + 1, iVar8 != 0; iVar8 = iVar8 + -1) {
    *piVar10 = 0;
  }
  piVar10 = local_590;
  local_590[0] = 0;
  for (iVar8 = 99; piVar10 = piVar10 + 1, iVar8 != 0; iVar8 = iVar8 + -1) {
    *piVar10 = 0;
  }
  local_5b4 = *(int *)((int)this + 0x20830);
  if (local_5b4 < *(int *)((int)this + 0x20838)) {
    local_5ac = local_5b4 << 4;
    do {
      local_5b0 = *(int *)((int)this + 0x2082c);
      if (local_5b0 < *(int *)((int)this + 0x20834)) {
        iVar8 = local_5b0 * 0x10;
        local_594 = local_400 + local_5b4 + iVar8;
        do {
          iVar2 = FUN_00404370(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4);
          if (iVar2 != 0) {
            iVar2 = *(int *)((int)this + 0x20814);
            if (iVar2 != DAT_00505eb0) {
              cVar1 = FUN_00404350(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4);
              if (cVar1 != iVar2) goto LAB_0049e45b;
            }
            pcVar3 = (char *)FUN_00404090(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4);
            local_590[*pcVar3] = local_590[*pcVar3] + 1;
            iVar2 = (pcVar3[5] & 7) + 4 + iVar8;
            uVar4 = 9999 - (byte)pcVar3[5];
            uVar9 = (int)uVar4 >> 0x1f;
            local_5a8 = local_5ac + 4 + (((uVar4 ^ uVar9) - uVar9 & 7 ^ uVar9) - uVar9);
            local_5a4 = local_5a8 + 2;
            FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,0);
            FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,4);
            FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,2);
            FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,6);
            local_59c = FUN_0049d810(this,iVar2 + -1,local_5a8 + -1,iVar2 + 3,local_5a8 + 3,0x78,
                                     *pcVar3);
            iVar5 = FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,0);
            if (iVar5 != 0) {
              local_5a8 = local_5ac;
            }
            iVar5 = FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,4);
            if (iVar5 != 0) {
              local_5a4 = local_5ac + 0x10;
            }
            iVar6 = FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,6);
            iVar5 = iVar2;
            if (iVar6 != 0) {
              iVar5 = iVar8;
            }
            iVar7 = FUN_004043a0(*(undefined4 *)((int)this + 0x20810),local_5b0,local_5b4,2);
            iVar6 = 0;
            local_5a0 = iVar2 + 2;
            if (iVar7 != 0) {
              iVar6 = iVar8 + 0x10;
              local_5a0 = iVar6;
            }
            iVar2 = FUN_0049d810(this,iVar5,local_5a8,local_5a0,local_5a4,0x78,
                                 CONCAT31((int3)((uint)iVar6 >> 8),*pcVar3));
            local_59c = local_59c + iVar2;
            if (local_590[*pcVar3] == 2) {
              iVar2 = FUN_0049d810(this,iVar8 + 3,local_5ac + 3,iVar8 + 0xd,local_5ac + 0xd,0x78,
                                   *pcVar3);
              local_59c = local_59c + iVar2;
            }
            iVar2 = FUN_004a59d0(0x1e);
            iVar2 = ((int)(local_59c * -100 + (local_59c * -100 >> 0x1f & 0xffU)) >> 8) +
                    iVar2 + 0x32;
            if (iVar2 < 0x15) {
              iVar2 = 0x14;
            }
            *local_594 = (iVar2 << 8) / 100;
          }
LAB_0049e45b:
          iVar8 = iVar8 + 0x10;
          local_5b0 = local_5b0 + 1;
          local_594 = local_594 + 0x10;
        } while (local_5b0 < *(int *)((int)this + 0x20834));
      }
      local_5ac = local_5ac + 0x10;
      local_5b4 = local_5b4 + 1;
    } while (local_5b4 < *(int *)((int)this + 0x20838));
  }
  FUN_0049ded0(this,local_400,local_590);
  FUN_0049db10(this,2);
  return;
}


// ==== FUN_0049ded0 @ 0049ded0

void __thiscall FUN_0049ded0(void *this,int param_2,int param_3)

{
  int *piVar1;
  undefined4 *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  bool bVar6;
  int local_28;
  undefined4 *local_24;
  int local_10;
  undefined4 local_c;
  uint local_8;
  undefined4 local_4;
  
  local_28 = *(int *)((int)this + 0x20830);
  if (local_28 < *(int *)((int)this + 0x20838)) {
    do {
      iVar5 = *(int *)((int)this + 0x2082c);
      if (iVar5 < *(int *)((int)this + 0x20834)) {
        local_24 = (undefined4 *)(param_2 + (iVar5 * 0x10 + local_28) * 4);
        do {
          puVar2 = (undefined4 *)FUN_00404090(*(undefined4 *)((int)this + 0x20810),iVar5,local_28);
          local_c = *puVar2;
          local_8 = puVar2[1];
          local_4 = puVar2[2];
          iVar4 = (int)(char)local_c;
          bVar6 = iVar4 != DAT_00505eb0;
          if (*(int *)((int)this + 0x20818) == 0) {
LAB_0049df80:
            if (bVar6) {
              iVar3 = 0;
              if (param_3 != 0) {
                iVar3 = *(int *)(param_3 + iVar4 * 4);
              }
              local_10 = (local_8 >> 8 & 0xff) + iVar3 * 0x10e3;
              if (param_3 != 0) {
                piVar1 = (int *)(param_3 + iVar4 * 4);
                *piVar1 = *piVar1 + 1;
              }
              if (param_2 == 0) {
                FUN_00490040(&DAT_005371b4,s__ns_o_terrainBuilder_cpp_00537198,0x279);
              }
              FUN_0049d7c0(this,iVar4,iVar5,local_28,*local_24,&local_10);
            }
          }
          else {
            if ((iVar4 == DAT_00505eb0) || (bVar6 = false, iVar4 == 0)) {
              bVar6 = true;
            }
            if (bVar6) {
              iVar4 = 0;
              bVar6 = true;
              goto LAB_0049df80;
            }
          }
          local_24 = local_24 + 0x10;
          iVar5 = iVar5 + 1;
        } while (iVar5 < *(int *)((int)this + 0x20834));
      }
      local_28 = local_28 + 1;
    } while (local_28 < *(int *)((int)this + 0x20838));
  }
  return;
}


// ==== FUN_0049d690 @ 0049d690

void __thiscall
FUN_0049d690(void *this,char param_2,int param_3,int param_4,int param_5,undefined4 param_6,
            int param_7)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_18;
  
  local_18 = 1000;
  iVar3 = (param_5 - param_3) * 0x10 + -1;
  while ((0 < param_7 && (iVar8 = local_18 + -1, local_18 != 0))) {
    iVar4 = FUN_004a59d0(iVar3);
    iVar4 = iVar4 + param_3 * 0x10;
    iVar5 = FUN_004a59d0(iVar3);
    iVar5 = iVar5 + param_4 * 0x10;
    FUN_004a59d0(1);
    iVar6 = iVar4 * 0x100 + iVar5;
    local_18 = iVar8;
    if ((*(char *)((int)this + iVar6 + 4) == 'x') &&
       ((*(int *)((int)this + 0x20818) != 0 || (*(char *)((int)this + iVar6 + 0x10004) == param_2)))
       ) {
      iVar6 = iVar5 + -1;
      iVar8 = param_7;
      if (iVar6 <= iVar5 + 1) {
        iVar2 = iVar4 + -1;
        do {
          for (; iVar2 <= iVar4 + 1; iVar2 = iVar2 + 1) {
            iVar7 = FUN_0049d5b0(this,iVar2,iVar6,0x78,param_2);
            if (iVar7 != 0) {
              iVar8 = iVar8 + -1;
            }
          }
          iVar6 = iVar6 + 1;
          iVar2 = iVar4 + -1;
        } while (iVar6 <= iVar5 + 1);
      }
      bVar1 = iVar8 < param_7;
      param_7 = iVar8;
      if (bVar1) {
        local_18 = 1000;
      }
    }
  }
  return;
}


// ==== FUN_0049d7c0 @ 0049d7c0

void __thiscall
FUN_0049d7c0(void *this,undefined1 param_2,int param_3,int param_4,undefined4 param_5,
            undefined4 param_6)

{
  FUN_0049d690(this,param_2,param_3,param_4,param_3 + 1,param_4 + 1,param_5,param_6);
  return;
}


// ==== FUN_0049d5b0 @ 0049d5b0

undefined4 __thiscall FUN_0049d5b0(void *this,int param_2,int param_3,char param_4,char param_5)

{
  char cVar1;
  int iVar2;
  int iVar3;
  
  if ((((*(int *)((int)this + 0x2081c) <= param_2) && (*(int *)((int)this + 0x20820) <= param_3)) &&
      (param_2 <= *(int *)((int)this + 0x20824))) &&
     ((param_3 <= *(int *)((int)this + 0x20828) &&
      (*(char *)((int)this + param_3 + param_2 * 0x100 + 4) != param_4)))) {
    if (*(int *)((int)this + 0x20818) == 0) {
      iVar2 = __ftol();
      iVar3 = __ftol();
      cVar1 = FUN_00404350(*(undefined4 *)((int)this + 0x20810),
                           (int)(iVar2 + (iVar2 >> 0x1f & 0xfU)) >> 4,
                           (int)(iVar3 + (iVar3 >> 0x1f & 0xfU)) >> 4);
      if (cVar1 != param_5) {
        return 0;
      }
    }
    FUN_0049d580(this,param_2,param_3,param_4,param_5);
    return 1;
  }
  return 0;
}


// ==== FUN_0049d810 @ 0049d810

int __thiscall
FUN_0049d810(void *this,int param_2,int param_3,int param_4,int param_5,char param_6,
            undefined4 param_7)

{
  int iVar1;
  int iVar2;
  int local_4;
  
  local_4 = 0;
  if (param_2 < param_4) {
    iVar2 = param_2 << 8;
    iVar1 = param_3;
    do {
      for (; iVar1 < param_5; iVar1 = iVar1 + 1) {
        if (*(char *)((int)this + iVar2 + iVar1 + 4) != param_6) {
          FUN_0049d580(this,param_2,iVar1,param_6,param_7);
          local_4 = local_4 + 1;
        }
      }
      iVar2 = iVar2 + 0x100;
      param_2 = param_2 + 1;
      iVar1 = param_3;
    } while (param_2 < param_4);
  }
  return local_4;
}


// ==== FUN_0049db10 @ 0049db10

void __thiscall FUN_0049db10(void *this,int param_2)

{
  int iVar1;
  char *pcVar2;
  int iVar3;
  int local_4;
  
  if (0 < param_2) {
    local_4 = param_2;
    do {
      iVar3 = *(int *)((int)this + 0x20820) + 1;
      if (iVar3 < *(int *)((int)this + 0x20828)) {
        do {
          iVar1 = *(int *)((int)this + 0x2081c) + 1;
          if (iVar1 < *(int *)((int)this + 0x20824) + -1) {
            pcVar2 = (char *)((int)this + iVar3 + iVar1 * 0x100 + 4);
            do {
              if ((*pcVar2 == '.') &&
                 ((((pcVar2[-0x100] == 'x' || (pcVar2[0x100] == 'x')) || (pcVar2[-1] == 'x')) ||
                  (pcVar2[1] == 'x')))) {
                FUN_0049da90(this,iVar1,iVar3);
              }
              pcVar2 = pcVar2 + 0x100;
              iVar1 = iVar1 + 1;
            } while (iVar1 < *(int *)((int)this + 0x20824) + -1);
          }
          iVar3 = iVar3 + 1;
        } while (iVar3 < *(int *)((int)this + 0x20828));
      }
      local_4 = local_4 + -1;
    } while (local_4 != 0);
  }
  return;
}


// ==== FUN_0049da90 @ 0049da90

void __thiscall FUN_0049da90(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  
  iVar2 = 0;
  uVar4 = 0;
  iVar3 = DAT_00505eb0;
  do {
    iVar1 = param_3 + ((&DAT_0051c330)[uVar4 & 7] + param_2) * 0x100 + (&DAT_0051c350)[uVar4 & 7];
    if (*(char *)((int)this + iVar1 + 4) == 'x') {
      if (iVar2 == 0) {
        iVar3 = (int)*(char *)((int)this + iVar1 + 0x10004);
      }
      if (*(char *)((int)this + iVar1 + 0x10004) != iVar3) goto LAB_0049daff;
      iVar2 = iVar2 + 1;
      if ((4 < iVar2) && ((uVar4 & 1) != 0)) {
        FUN_0049d580(this,param_2,param_3,0x78,iVar3);
        return;
      }
    }
    else {
LAB_0049daff:
      iVar2 = 0;
    }
    uVar4 = uVar4 + 1;
    if (0xc < (int)uVar4) {
      return;
    }
  } while( true );
}


// ==== FUN_0049d580 @ 0049d580

void __thiscall
FUN_0049d580(void *this,int param_2,int param_3,undefined1 param_4,undefined1 param_5)

{
  param_3 = param_3 + param_2 * 0x100;
  *(undefined1 *)((int)this + param_3 + 4) = param_4;
  *(undefined1 *)((int)this + param_3 + 0x10004) = param_5;
  return;
}


// ==== FUN_00404090 @ 00404090

int __thiscall FUN_00404090(void *this,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  
  iVar2 = *(int *)((int)this + 4) + -1;
  if (param_2 < 1) {
    param_2 = 0;
  }
  if (param_2 <= iVar2) {
    iVar2 = param_2;
  }
  iVar1 = *(int *)((int)this + 8) + -1;
  if (param_3 < 1) {
    param_3 = 0;
  }
  if (param_3 <= iVar1) {
    iVar1 = param_3;
  }
  return (int)this + (iVar2 * 0x10 + iVar1) * 0xc + 0xc;
}


// ==== FUN_00404370 @ 00404370

bool __thiscall FUN_00404370(void *this,undefined4 param_2,undefined4 param_3)

{
  char *pcVar1;
  
  pcVar1 = (char *)FUN_00404090(this,param_2,param_3);
  return *pcVar1 != DAT_00505eb0;
}


// ==== FUN_00404350 @ 00404350

undefined1 __thiscall FUN_00404350(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)FUN_00404090(this,param_2,param_3);
  return *puVar1;
}


// ==== FUN_004043a0 @ 004043a0

uint __thiscall FUN_004043a0(void *this,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  
  iVar1 = FUN_00404090(this,param_2,param_3);
  return *(uint *)(s_Error_in_command____s___code______0051c2dc + *(char *)(iVar1 + 1) * 4 + 0x20) &
         (&DAT_0051c3d0)[param_4];
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


// ==== FUN_004bd480 @ 004bd480

undefined4 __thiscall FUN_004bd480(int *param_1,int param_2)

{
  if ((-1 < param_2) && (param_2 < *param_1)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004a59d0 @ 004a59d0

int FUN_004a59d0(int param_1)

{
  int iVar1;
  
  if (param_1 < 1) {
    return 0;
  }
  iVar1 = FUN_004a59a0();
  return iVar1 % param_1;
}


