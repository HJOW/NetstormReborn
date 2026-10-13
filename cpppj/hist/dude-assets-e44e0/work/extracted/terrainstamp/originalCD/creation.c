// ==== FUN_0049cf20 @ 0049cf20

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_0049cf20(void *this,int param_2,int param_3)

{
  char cVar1;
  char cVar2;
  char cVar3;
  bool bVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  int iVar12;
  int iVar13;
  uint uVar14;
  undefined4 local_8;
  
  iVar6 = (int)this + param_3 + param_2 * 0x100;
  cVar1 = *(char *)(iVar6 + 4);
  iVar12 = DAT_0051c350 + param_3;
  cVar2 = *(char *)(iVar6 + 0x10004);
  iVar13 = DAT_0051c330 + param_2;
  cVar3 = *(char *)((int)this + cVar1 * 4 + 0x20404);
  uVar5 = CONCAT31((int3)this,cVar3);
  iVar7 = FUN_0049ed70(this,iVar13,iVar12);
  if (((iVar7 == 0) ||
      (*(int *)((int)this + *(char *)((int)this + iVar12 + iVar13 * 0x100 + 4) * 4 + 0x20404) !=
       (int)cVar3)) ||
     (bVar4 = true, *(char *)((int)this + iVar12 + iVar13 * 0x100 + 0x10004) != cVar2)) {
    bVar4 = false;
  }
  uVar14 = 0;
  if (bVar4) {
    uVar14 = DAT_0051c3d0;
  }
  iVar7 = FUN_0049edb0(this,DAT_0051c338 + param_2,DAT_0051c358 + param_3,uVar5,cVar2);
  uVar10 = 0;
  if (iVar7 != 0) {
    uVar10 = DAT_0051c3d8;
  }
  iVar7 = FUN_0049edb0(this,DAT_0051c340 + param_2,DAT_0051c360 + param_3,uVar5,cVar2);
  uVar11 = 0;
  if (iVar7 != 0) {
    uVar11 = DAT_0051c3e0;
  }
  iVar7 = FUN_0049edb0(this,DAT_0051c348 + param_2,DAT_0051c368 + param_3,uVar5,cVar2);
  if (iVar7 == 0) {
    uVar14 = uVar11 | uVar10 | uVar14;
  }
  else {
    uVar14 = uVar11 | uVar10 | uVar14 | _DAT_0051c3e8;
  }
  cVar2 = *(char *)(iVar6 + 0x10004);
  local_8._1_3_ = (undefined3)((uint)iVar6 >> 8);
  local_8 = CONCAT31(local_8._1_3_,cVar2);
  iVar6 = FUN_0049edb0(this,DAT_0051c34c + param_2,DAT_0051c36c + param_3,uVar5,local_8);
  uVar10 = 0;
  if (iVar6 != 0) {
    uVar10 = DAT_0051c3ec;
  }
  iVar6 = DAT_0051c354 + param_3;
  iVar12 = DAT_0051c334 + param_2;
  iVar7 = FUN_0049ed70(this,iVar12,iVar6);
  if (((iVar7 == 0) ||
      (iVar6 = iVar6 + iVar12 * 0x100,
      *(int *)((int)this + *(char *)((int)this + iVar6 + 4) * 4 + 0x20404) != (int)cVar3)) ||
     (bVar4 = true, *(char *)((int)this + iVar6 + 0x10004) != cVar2)) {
    bVar4 = false;
  }
  uVar11 = 0;
  if (bVar4) {
    uVar11 = DAT_0051c3d4;
  }
  iVar6 = FUN_0049edb0(this,DAT_0051c33c + param_2,DAT_0051c35c + param_3,uVar5,local_8);
  uVar8 = 0;
  if (iVar6 != 0) {
    uVar8 = DAT_0051c3dc;
  }
  iVar6 = FUN_0049edb0(this,DAT_0051c344 + param_2,DAT_0051c364 + param_3,uVar5,local_8);
  if (iVar6 == 0) {
    uVar11 = uVar10 | uVar8 | uVar11;
  }
  else {
    uVar11 = uVar10 | uVar8 | uVar11 | _DAT_0051c3e4;
  }
  cVar2 = (&DAT_0051c3f0)[uVar14];
  cVar3 = (&DAT_0051c3f0)[uVar11];
  local_8 = CONCAT31(local_8._1_3_,cVar2);
  iVar7 = DAT_0051c960 + *(int *)((int)this + cVar1 * 4 + 0x20004) * 0x1d4;
  iVar6 = DAT_00537190;
  if (DAT_00537190 == 0) {
    iVar12 = 0;
    DAT_00537190 = FUN_004f1650(0x18c);
    FUN_0048ef00();
    do {
      uVar14 = _rand();
      iVar6 = DAT_00537190;
      *(uint *)(DAT_00537190 + iVar12) = uVar14;
      if (0 < iVar12) {
        uVar10 = *(uint *)(iVar6 + -4 + iVar12);
        uVar8 = (int)uVar10 >> 0x1f;
        uVar9 = (int)uVar14 >> 0x1f;
        if (((uVar10 ^ uVar8) - uVar8 & 3 ^ uVar8) - uVar8 ==
            ((uVar14 ^ uVar9) - uVar9 & 3 ^ uVar9) - uVar9) {
          *(uint *)(iVar6 + iVar12) = uVar14 + 1;
        }
      }
      iVar12 = iVar12 + 4;
    } while (iVar12 < 0x18c);
  }
  if (((cVar1 == 'x') && (cVar2 == 'A')) && (cVar3 == 'A')) {
    uVar14 = *(int *)(iVar6 + ((*(int *)(iVar6 + ((param_2 / 3) % 99) * 4) +
                               *(int *)(iVar6 + ((param_3 / 3) % 99) * 4)) % 99) * 4) + param_3 / 3;
    uVar10 = (int)uVar14 >> 0x1f;
    iVar6 = ((((uVar14 ^ uVar10) - uVar10 & 3 ^ uVar10) - uVar10) * 3 + param_3 % 3) * 3 +
            param_2 % 3 + 1 + *(int *)(iVar7 + 0x154);
  }
  else {
    iVar6 = FUN_004444c0(iVar7,local_8,CONCAT31((int3)(uVar11 >> 8),cVar3),0xffffffff);
  }
  if (iVar6 == -1) {
    iVar6 = 0;
  }
  return iVar6;
}


// ==== FUN_0049d2c0 @ 0049d2c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0049d2c0(void *this,char param_2)

{
  undefined1 *puVar1;
  float fVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  int local_24;
  char *local_20;
  undefined1 local_18;
  
  local_24 = *(int *)((int)this + 0x20820);
  if (local_24 < *(int *)((int)this + 0x20828)) {
    do {
      iVar9 = *(int *)((int)this + 0x2081c);
      if (iVar9 < *(int *)((int)this + 0x20824)) {
        local_20 = (char *)(iVar9 * 0x100 + local_24 + 4 + (int)this);
        do {
          if ((((*(int *)((int)this + 0x2081c) <= iVar9) &&
               (*(int *)((int)this + 0x20820) <= local_24)) &&
              (iVar9 <= *(int *)((int)this + 0x20824))) &&
             ((local_24 <= *(int *)((int)this + 0x20828) &&
              (*(int *)((int)this + *local_20 * 4 + 0x20404) == (int)param_2)))) {
            FUN_0049cf00(this,DAT_0051cac8,DAT_0051cacc);
            iVar6 = *(int *)((int)this + *local_20 * 4 + 0x20004);
            if (iVar6 != 0) {
              iVar4 = FUN_0049cf20(this,iVar9,local_24);
              iVar10 = (int)local_20[0x10000];
              piVar5 = (int *)FUN_004ab390(iVar6,*(undefined4 *)((int)this + 0x20804));
              *(int *)((int)this + 0x20808) = ((int)piVar5 - DAT_005395dc) / 0x24;
              (**(code **)(*piVar5 + 0x74))(0);
              local_18 = SUB41((float)local_24,0);
              *(undefined1 *)((int)piVar5 + 0x22) = local_18;
              FUN_004abf50(piVar5);
              (**(code **)(*piVar5 + 0x90))(local_20,(float)iVar9,0x2000061);
              if ((*(byte *)(iVar9 + 0xec) & 2) != 0) {
                if (iVar10 == DAT_00505eb0) {
                  iVar6 = __ftol();
                  iVar10 = __ftol();
                  cVar3 = FUN_00404350(&DAT_005484c0,(int)(iVar6 + (iVar6 >> 0x1f & 0xfU)) >> 4,
                                       (int)(iVar10 + (iVar10 >> 0x1f & 0xfU)) >> 4);
                  iVar10 = (int)cVar3;
                }
                FUN_004aca80(piVar5,iVar10);
                puVar1 = (undefined1 *)(*(int *)(iVar9 + 0x124) + iVar4 * 4);
                if ((puVar1[3] & 1) != 0) {
                  uVar7 = DAT_0051cadc * 0x1d4 + DAT_0051c960;
                  uVar8 = uVar7;
                  do {
                    iVar6 = FUN_004444c0(uVar7,*puVar1,CONCAT31((int3)(uVar8 >> 8),puVar1[1]),
                                         0xffffffff);
                    uVar8 = (uint)*(char *)(*(int *)(DAT_0051cadc * 0x1d4 + 0x124 + DAT_0051c960) +
                                            3 + iVar6 * 4);
                  } while ((*(uint *)((int)this + 0x2080c) & uVar8) != 0);
                  if (iVar6 != -1) {
                    if (*(int *)((int)this + 0x20804) == 0) {
                      FUN_00490040(s_makePredictable_005371d8,s__ns_o_terrainBuilder_cpp_005371e8,
                                   0x5a);
                    }
                    piVar5 = (int *)FUN_004ab390(DAT_0051cadc,*(undefined4 *)((int)this + 0x20804));
                    fVar2 = (float)local_24 + _DAT_0050522c;
                    *(char *)((int)piVar5 + 0x22) = (char)iVar6;
                    (**(code **)(*piVar5 + 0x90))((float)iVar9,fVar2,0x2000061);
                  }
                }
              }
            }
          }
          local_20 = local_20 + 0x100;
          iVar9 = iVar9 + 1;
        } while (iVar9 < *(int *)((int)this + 0x20824));
      }
      local_24 = local_24 + 1;
    } while (local_24 < *(int *)((int)this + 0x20828));
  }
  return;
}


// ==== FUN_0049e4c0 @ 0049e4c0

void __thiscall FUN_0049e4c0(void *this)

{
  FUN_0049d890(this,0x78,0x3d);
  FUN_0049d2c0(this,0x78);
  FUN_0049d9a0(this);
  return;
}


// ==== FUN_0049cf00 @ 0049cf00

void __thiscall FUN_0049cf00(void *this,undefined4 param_2,undefined4 param_3)

{
  *(undefined4 *)((int)this + 0x201e4) = param_2;
  *(undefined4 *)((int)this + 0x20164) = param_3;
  *(undefined4 *)((int)this + 0x200f8) = param_2;
  return;
}


// ==== FUN_0049cd50 @ 0049cd50

void __thiscall FUN_0049cd50(void *this,int param_2,int param_3,int param_4,int param_5)

{
  *(int *)((int)this + 0x2082c) = param_2;
  *(int *)((int)this + 0x20830) = param_3;
  *(int *)((int)this + 0x2081c) = param_2 << 4;
  *(int *)((int)this + 0x20820) = param_3 << 4;
  *(int *)((int)this + 0x20838) = param_5;
  *(int *)((int)this + 0x20834) = param_4;
  *(int *)((int)this + 0x20824) = param_4 << 4;
  *(int *)((int)this + 0x20828) = param_5 << 4;
  if (0x10 < param_4) {
    *(undefined4 *)((int)this + 0x20834) = 0x10;
  }
  if (0x10 < param_5) {
    *(undefined4 *)((int)this + 0x20838) = 0x10;
  }
  return;
}


// ==== FUN_0049cdd0 @ 0049cdd0

void * __thiscall
FUN_0049cdd0(void *this,int param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7,undefined1 *param_8)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined1 *puVar6;
  undefined4 *puVar7;
  
  *(undefined4 *)((int)this + 0x20808) = 0;
  *(undefined4 *)((int)this + 0x2080c) = 0;
  *(undefined1 **)((int)this + 0x20810) = param_8;
  *(undefined4 *)((int)this + 0x20804) = 1;
  puVar6 = param_8;
  for (iVar5 = 0xc0c; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar6 = *param_8;
    param_8 = param_8 + 1;
    puVar6 = puVar6 + 1;
  }
  puVar7 = (undefined4 *)((int)this + 0x20004);
  for (iVar5 = 0x100; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  puVar7 = (undefined4 *)((int)this + 0x20404);
  for (iVar5 = 0x100; uVar2 = DAT_00505eb0, iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  *(undefined4 *)((int)this + 0x20814) = DAT_00505eb0;
  *(undefined4 *)((int)this + 0x20818) = 0;
  *(undefined4 *)((int)this + 0x20814) = param_3;
  if (DAT_00540a20 != 0) {
    *(undefined4 *)((int)this + 0x2080c) = 0x10;
  }
  if (param_2 == 0) {
    *(undefined4 *)((int)this + 0x2080c) = 0x18;
  }
  uVar4 = DAT_0051cacc;
  uVar3 = DAT_0051cac8;
  *(undefined4 *)((int)this + 0x201e4) = DAT_0051cac8;
  *(undefined4 *)((int)this + 0x205e4) = 0x78;
  *(undefined4 *)((int)this + 0x20164) = uVar4;
  *(undefined4 *)((int)this + 0x20564) = 0x78;
  *(undefined4 *)((int)this + 0x205c0) = 0x78;
  *(undefined4 *)((int)this + 0x200f8) = uVar3;
  *(undefined4 *)((int)this + 0x200bc) = 0;
  *(undefined4 *)((int)this + 0x201c0) = 0;
  *(undefined4 *)((int)this + 0x204f8) = 0x78;
  puVar7 = this;
  for (iVar5 = 0x4000; puVar7 = puVar7 + 1, iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = 0x2e2e2e2e;
  }
  uVar1 = CONCAT11((char)uVar2,(char)uVar2);
  puVar7 = (undefined4 *)((int)this + 0x10004);
  for (iVar5 = 0x4000; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar7 = CONCAT22(uVar1,uVar1);
    puVar7 = puVar7 + 1;
  }
  *(int *)this = param_2;
  FUN_0049cd50(this,param_4,param_5,param_6,param_7);
  return this;
}


// ==== FUN_0049ed70 @ 0049ed70

undefined4 __thiscall FUN_0049ed70(void *this,int param_2,int param_3)

{
  if ((((*(int *)((int)this + 0x2081c) <= param_2) && (*(int *)((int)this + 0x20820) <= param_3)) &&
      (param_2 <= *(int *)((int)this + 0x20824))) && (param_3 <= *(int *)((int)this + 0x20828))) {
    return 1;
  }
  return 0;
}


// ==== FUN_0049edb0 @ 0049edb0

undefined4 __thiscall FUN_0049edb0(void *this,int param_2,int param_3,char param_4,char param_5)

{
  if ((((*(int *)((int)this + 0x2081c) <= param_2) && (*(int *)((int)this + 0x20820) <= param_3)) &&
      (param_2 <= *(int *)((int)this + 0x20824))) && (param_3 <= *(int *)((int)this + 0x20828))) {
    param_3 = param_2 * 0x100 + param_3;
    if ((*(int *)((int)this + *(char *)(param_3 + 4 + (int)this) * 4 + 0x20404) == (int)param_4) &&
       (*(char *)((int)this + param_3 + 0x10004) == param_5)) {
      return 1;
    }
    return 0;
  }
  return 0;
}


// ==== FUN_0049d890 @ 0049d890

void __thiscall FUN_0049d890(void *this,int param_2,undefined4 param_3)

{
  int iVar1;
  bool bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  char *local_c;
  
  iVar6 = *(int *)((int)this + 0x20820);
  if (iVar6 < *(int *)((int)this + 0x20828)) {
    do {
      iVar4 = *(int *)((int)this + 0x2081c);
      local_c = (char *)(iVar4 * 0x100 + iVar6 + 4 + (int)this);
      for (; iVar4 < *(int *)((int)this + 0x20824); iVar4 = iVar4 + 1) {
        if (param_2 == *local_c) {
          iVar3 = 0;
          do {
            iVar5 = *(int *)((int)&DAT_0051c330 + iVar3) + iVar4;
            if (iVar5 < *(int *)((int)this + 0x2081c)) {
LAB_0049d950:
              bVar2 = false;
            }
            else {
              iVar1 = iVar6 + *(int *)((int)&DAT_0051c350 + iVar3);
              if ((((iVar1 < *(int *)((int)this + 0x20820)) ||
                   (*(int *)((int)this + 0x20824) < iVar5)) ||
                  (*(int *)((int)this + 0x20828) < iVar1)) ||
                 ((iVar5 = iVar5 * 0x100 + *(int *)((int)&DAT_0051c350 + iVar3) + iVar6,
                  *(int *)((int)this + *(char *)(iVar5 + 4 + (int)this) * 4 + 0x20404) !=
                  (int)*local_c || (*(char *)((int)this + iVar5 + 0x10004) != local_c[0x10000]))))
              goto LAB_0049d950;
              bVar2 = true;
            }
            if (!bVar2) {
              FUN_0049d580(this,iVar4,iVar6,param_3,(int)local_c[0x10000]);
              break;
            }
            iVar3 = iVar3 + 4;
          } while (iVar3 < 0x20);
        }
        local_c = local_c + 0x100;
      }
      iVar6 = iVar6 + 1;
    } while (iVar6 < *(int *)((int)this + 0x20828));
  }
  return;
}


// ==== FUN_0049d9a0 @ 0049d9a0

void __thiscall FUN_0049d9a0(void *this)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  int iVar4;
  
  iVar3 = *(int *)((int)this + 0x20820);
  if (iVar3 < *(int *)((int)this + 0x20828)) {
    do {
      iVar4 = *(int *)((int)this + 0x2081c);
      if (iVar4 < *(int *)((int)this + 0x20824)) {
        pcVar2 = (char *)(iVar4 * 0x100 + iVar3 + 4 + (int)this);
        do {
          cVar1 = *pcVar2;
          if (((cVar1 == '=') || (cVar1 == 'o')) || (cVar1 == 'X')) {
            *pcVar2 = 'x';
          }
          pcVar2 = pcVar2 + 0x100;
          iVar4 = iVar4 + 1;
        } while (iVar4 < *(int *)((int)this + 0x20824));
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < *(int *)((int)this + 0x20828));
  }
  return;
}


// ==== FUN_004444c0 @ 004444c0

int __thiscall FUN_004444c0(undefined4 param_1,char param_2,char param_3,uint param_4)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  undefined4 *unaff_FS_OFFSET;
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0044469e;
  *unaff_FS_OFFSET = &local_10;
  local_20 = FUN_004f1650(0x400);
  iVar3 = 0;
  local_18 = 0;
  local_8 = 0;
  local_1c = 0x100;
  pcVar4 = *(char **)(local_14 + 0x124);
  local_24 = *(int *)(local_14 + 0x114);
  pcVar1 = pcVar4;
  if (0 < local_24) {
    do {
      if ((((*pcVar1 == param_2) && (pcVar1[1] == param_3)) &&
          ((param_4 == (int)pcVar1[3] ||
           ((param_4 == 0xffffffff || ((param_4 & (int)pcVar1[3]) != 0)))))) && (local_18 < 0x100))
      {
        *(int *)(local_20 + local_18 * 4) = iVar3;
        local_18 = local_18 + 1;
      }
      iVar3 = iVar3 + 1;
      pcVar1 = pcVar1 + 4;
    } while (iVar3 < local_24);
  }
  if (local_18 == 0) {
    if (param_4 == 0xffffffff) {
      FUN_0043f400((int)param_2,(int)param_3,&local_28,&local_24);
      iVar2 = 0;
      iVar3 = *(int *)(local_14 + 0x114);
      if (0 < iVar3) {
        do {
          if (((*pcVar4 == local_28) && (pcVar4[1] == local_24)) && (local_18 < local_1c)) {
            *(int *)(local_20 + local_18 * 4) = iVar2;
            local_18 = local_18 + 1;
          }
          iVar2 = iVar2 + 1;
          pcVar4 = pcVar4 + 4;
        } while (iVar2 < iVar3);
      }
      goto LAB_004445e5;
    }
LAB_00444565:
    local_8 = 0xffffffff;
    FUN_004446a8();
    local_14 = 0xffffffff;
  }
  else {
LAB_004445e5:
    if (local_18 == 0) {
      iVar2 = 0;
      pcVar4 = *(char **)(local_14 + 0x124);
      iVar3 = *(int *)(local_14 + 0x114);
      if (0 < iVar3) {
        do {
          if (((*pcVar4 == param_2) && (pcVar4[1] == 'A')) && (local_18 < local_1c)) {
            *(int *)(local_20 + local_18 * 4) = iVar2;
            local_18 = local_18 + 1;
          }
          iVar2 = iVar2 + 1;
          pcVar4 = pcVar4 + 4;
        } while (iVar2 < iVar3);
      }
      if (local_18 == 0) {
        iVar2 = 0;
        pcVar4 = *(char **)(local_14 + 0x124);
        iVar3 = *(int *)(local_14 + 0x114);
        if (0 < iVar3) {
          do {
            if ((*pcVar4 == param_2) && (local_18 < local_1c)) {
              *(int *)(local_20 + local_18 * 4) = iVar2;
              local_18 = local_18 + 1;
            }
            iVar2 = iVar2 + 1;
            pcVar4 = pcVar4 + 4;
          } while (iVar2 < iVar3);
        }
        goto LAB_0044465b;
      }
      if (local_18 != 0) goto LAB_00444675;
      local_14 = 0;
    }
    else {
LAB_0044465b:
      if (local_18 == 0) goto LAB_00444565;
LAB_00444675:
      iVar3 = FUN_0048cbd0(local_18);
      local_14 = *(undefined4 *)(local_20 + iVar3 * 4);
    }
    local_8 = 0xffffffff;
    FUN_004446a8();
  }
  *unaff_FS_OFFSET = local_10;
  return local_14;
}


// ==== FUN_0048ef00 @ 0048ef00

void __cdecl FUN_0048ef00(void)

{
  return;
}


// ==== _rand @ 004f23b0

/* Library Function - Single Match
    _rand
   
   Library: Visual Studio 1998 Release */

int __cdecl _rand(void)

{
  uint uVar1;
  _ptiddata p_Var2;
  
  p_Var2 = __getptd();
  uVar1 = p_Var2->_holdrand * 0x343fd + 0x269ec3;
  p_Var2->_holdrand = uVar1;
  return (uVar1 & 0x7fff0000) >> 0x10;
}


