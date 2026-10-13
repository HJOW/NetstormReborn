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


// ==== FUN_0049e620 @ 0049e620

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl
FUN_0049e620(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  int local_20;
  int local_1c;
  undefined2 local_18;
  undefined1 uStack_16;
  undefined1 uStack_15;
  undefined1 local_14;
  undefined1 uStack_13;
  undefined1 uStack_12;
  undefined1 uStack_11;
  ushort local_10;
  undefined2 uStack_e;
  code *pcStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  pcStack_c = FUN_0049e8f9;
  local_10 = (ushort)*unaff_FS_OFFSET;
  uStack_e = (undefined2)((uint)*unaff_FS_OFFSET >> 0x10);
  *unaff_FS_OFFSET = &local_10;
  iVar3 = FUN_0049e5e0();
  iVar6 = 0;
  local_1c = FUN_004f1650(0x2083c);
  local_8 = 0;
  if (local_1c != 0) {
    iVar6 = FUN_0049cdd0(local_1c,param_1,param_7,param_2,param_3,param_4,param_5,param_6);
  }
  iVar4 = DAT_00537188;
  local_8 = 0xffffffff;
  if (iVar3 == 0) {
    FUN_0049e160(iVar6);
    FUN_0049e4c0(iVar6);
  }
  else {
    *(undefined4 *)(iVar6 + 0x20818) = 1;
    FUN_0049cd50(iVar6,(*(int *)(iVar6 + 0x2082c) - iVar4) + 1,
                 (*(int *)(iVar6 + 0x20830) - iVar4) + 1,*(int *)(iVar6 + 0x20834) + iVar4,
                 *(int *)(iVar6 + 0x20838) + iVar4);
    iVar3 = DAT_00537188 * 2 + 2;
    FUN_0049e4e0(iVar6,(iVar3 * iVar3 * 0x55) / 0x19);
    local_1c = 0;
    local_1c = FUN_0049de40(iVar6);
    local_1c = local_1c * 0x1bb;
    iVar3 = (*(int *)(iVar6 + 0x20838) - *(int *)(iVar6 + 0x20830)) *
            (*(int *)(iVar6 + 0x20834) - *(int *)(iVar6 + 0x2082c)) * 0x100;
    if (_DAT_0053718c == 0) {
      iVar4 = (iVar3 * DAT_00537180) / 200;
      local_14 = (undefined1)iVar4;
      uStack_13 = (undefined1)((uint)iVar4 >> 8);
      uStack_12 = (undefined1)((uint)iVar4 >> 0x10);
      uStack_11 = (undefined1)((uint)iVar4 >> 0x18);
      iVar5 = (DAT_0053717c * iVar3) / 200;
      local_20 = FUN_004a59f0(iVar5,iVar4);
      iVar4 = FUN_004a59f0(iVar5,CONCAT13(uStack_11,CONCAT12(uStack_12,CONCAT11(uStack_13,local_14))
                                         ));
      iVar4 = local_20 + iVar4;
    }
    else {
      iVar4 = (_DAT_0053718c * iVar3) / 100;
    }
    FUN_0048ca70(0x1d,s_Archipelago_Percentage___d_005371b8,(iVar4 * 100) / iVar3);
    local_24 = *(undefined4 *)(iVar6 + 0x20834);
    local_20 = *(int *)(iVar6 + 0x20838);
    uVar1 = *(undefined4 *)(iVar6 + 0x2082c);
    local_18 = (undefined2)uVar1;
    uStack_16 = (undefined1)((uint)uVar1 >> 0x10);
    uStack_15 = (undefined1)((uint)uVar1 >> 0x18);
    uVar2 = *(undefined4 *)(iVar6 + 0x20830);
    local_14 = (undefined1)uVar2;
    uStack_13 = (undefined1)((uint)uVar2 >> 8);
    uStack_12 = (undefined1)((uint)uVar2 >> 0x10);
    uStack_11 = (undefined1)((uint)uVar2 >> 0x18);
    iVar5 = 1;
    FUN_0049d690(iVar6,0,uVar1,uVar2,local_24,local_20,iVar4,&local_1c);
    iVar3 = DAT_0050f834;
    for (iVar4 = 0xac; iVar4 < 0x60c; iVar4 = iVar4 + 0xac) {
      if (*(int *)(iVar4 + 0x2c + iVar3) != 0) {
        local_14 = 0;
        uStack_13 = 0;
        uStack_12 = 0;
        uStack_11 = 0;
        uStack_16 = 7;
        uStack_15 = 1;
        FUN_00403e10(&local_2c,iVar5,0x107,(uint)local_10 << 0x10,&DAT_005484c0);
        FUN_0049e030(iVar6,local_2c,local_28,local_24,local_20);
        iVar3 = DAT_0050f834;
      }
      iVar5 = iVar5 + 1;
    }
    FUN_0049db10(iVar6,1);
    FUN_0049dbb0(iVar6);
    FUN_0049e4c0(iVar6);
    DAT_00537184 = 1;
  }
  FUN_004f1a50(iVar6);
  *unaff_FS_OFFSET = CONCAT22(uStack_e,local_10);
  return;
}


// ==== FUN_00490530 @ 00490530

void __thiscall FUN_00490530(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  undefined4 uVar6;
  int iVar7;
  undefined1 local_18 [8];
  undefined1 local_10 [16];
  
  iVar4 = FUN_00490520();
  if (iVar4 == 0) {
    FUN_00490040(s_inWorld___00535afc,s__ns_o_islandbuilder_cpp_00535ae4,0x41);
  }
  if ((((*(int *)((int)this + 4) == -1) || (*(int *)((int)this + 8) == -1)) ||
      (*(int *)((int)this + 0xc) == -1)) || (*(int *)((int)this + 0x10) == -1)) {
    FUN_00490040(s_cRect_isValid___00535b20,s__ns_o_islandbuilder_cpp_00535b08,0x42);
  }
  if (*(int *)this == 0) {
    FUN_00490040(&DAT_00535b48,s__ns_o_islandbuilder_cpp_00535b30,0x45);
  }
  iVar7 = 0;
  FUN_0049e620(*(undefined4 *)((int)this + 0x14),*(int *)((int)this + 4),
               *(undefined4 *)((int)this + 8),*(undefined4 *)((int)this + 0xc),
               *(undefined4 *)((int)this + 0x10),*(undefined4 *)this,param_2);
  iVar4 = FUN_004bd4f0(&DAT_00583188);
  if (0 < iVar4) {
    do {
      if ((DAT_00505eb0 == param_2) || (iVar7 == param_2)) {
        FUN_004047c0(*(undefined4 *)this,local_18,iVar7);
        piVar5 = (int *)FUN_004040d0(*(undefined4 *)this,local_10,iVar7);
        iVar4 = *piVar5;
        iVar1 = piVar5[1];
        iVar2 = piVar5[2];
        iVar3 = piVar5[3];
        uVar6 = FUN_0040b9d0(local_18);
        FUN_0042cd80((float)(iVar4 << 4),(float)(iVar1 << 4),(float)(iVar2 << 4),(float)(iVar3 << 4)
                     ,iVar7,uVar6);
      }
      iVar7 = iVar7 + 1;
      iVar4 = FUN_004bd4f0(&DAT_00583188);
    } while (iVar7 < iVar4);
  }
  return;
}


// ==== FUN_0042cd80 @ 0042cd80

void __cdecl
FUN_0042cd80(float param_1,float param_2,float param_3,float param_4,int param_5,int param_6)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_50 [8];
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  float local_3c;
  float local_38;
  int local_34;
  float local_30;
  float local_2c;
  int local_28;
  undefined4 local_24;
  int local_20;
  int local_1c;
  undefined1 local_18 [2];
  int local_16;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0042cf71;
  *unaff_FS_OFFSET = &local_10;
  iVar2 = FUN_004047c0(&DAT_005484c0,local_18,param_5);
  local_20 = (uint)*(byte *)(iVar2 + 3) * 0x4d;
  local_48 = FUN_004f1650(0x4b0);
  local_40 = 0;
  local_24 = 0;
  local_44 = 300;
  param_6 = param_6 * 0x14;
  local_16 = 1;
  local_34 = 30000;
  local_8 = 1;
  if ((DAT_00537184 != 0) && (param_5 == 0)) {
    fVar1 = (float)((1 - DAT_00537188) * 0x10);
    param_1 = param_1 + fVar1;
    param_2 = fVar1 + param_2;
    local_16 = 2;
    param_3 = param_3 + (float)(DAT_00537188 << 4);
    local_24 = 1;
    iVar2 = DAT_00537188 * 2 + 2;
    param_4 = (float)(DAT_00537188 << 4) + param_4;
    param_6 = (param_6 / 9) * iVar2 * iVar2;
  }
  do {
    pfVar3 = (float *)FUN_0042c010(local_50,&local_20,param_1,param_2,param_3,param_4);
    local_30 = *pfVar3;
    local_2c = pfVar3[1];
    iVar2 = FUN_0042c3d0(&local_20,DAT_0051cb28,local_30,local_2c,param_5,1,local_24);
    if (iVar2 != 0) {
      param_6 = param_6 + -1;
      iVar2 = -local_16;
      local_34 = 30000;
      uVar5 = DAT_0051cb28;
      local_1c = iVar2;
      if (iVar2 <= local_16) {
        do {
          if (param_6 == 0) goto LAB_0042cf4d;
          iVar6 = local_1c;
          if (local_1c <= local_16) {
            do {
              if (param_6 == 0) break;
              local_3c = (float)iVar2 + local_30;
              local_38 = (float)iVar6 + local_2c;
              local_28 = iVar6;
              iVar4 = FUN_0042c3d0(&local_20,uVar5,local_3c,local_38,param_5,1,local_24);
              if (iVar4 != 0) {
                param_6 = param_6 + -1;
              }
              iVar6 = iVar6 + 1;
              uVar5 = DAT_0051cb28;
            } while (iVar6 <= local_16);
          }
          iVar2 = iVar2 + 1;
        } while (iVar2 <= local_16);
      }
    }
    if (param_6 == 0) break;
    local_34 = local_34 + -1;
  } while (local_34 != 0);
LAB_0042cf4d:
  local_8 = 0xffffffff;
  FUN_0042cf7b();
  *unaff_FS_OFFSET = local_10;
  return;
}


// ==== FUN_0040b9d0 @ 0040b9d0

int __thiscall FUN_0040b9d0(void *this)

{
  int iVar1;
  undefined4 local_54;
  undefined4 local_50;
  int local_40;
  
  local_54 = 0;
  local_50 = 0;
  iVar1 = 0;
  FUN_0041fcf0(&local_54,DAT_0051caf0,*(byte *)this & 0x3f,(*(byte *)this >> 6) * '\x02',0,0,0);
  while (local_40 != 0) {
    iVar1 = iVar1 + 1;
    FUN_0041feb0(&local_54);
  }
  return iVar1;
}


// ==== FUN_0042c3d0 @ 0042c3d0

int __cdecl
FUN_0042c3d0(undefined4 param_1,int param_2,float param_3,float param_4,int param_5,int param_6,
            int param_7)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  undefined4 uVar5;
  uint uVar6;
  int *piVar7;
  float unaff_retaddr;
  undefined8 local_8;
  float fStack_4;
  
  iVar1 = __ftol();
  iVar2 = __ftol();
  if ((*(byte *)(iVar1 * 0x100 + iVar2 + DAT_0052fe48) & 0x3f) == 0) {
    if (DAT_005187bc != (FILE *)0x0) {
      FID_conflict__fwprintf(DAT_005187bc,(wchar_t *)s_failed_spot_flags_005187c0);
      _fflush(DAT_005187bc);
    }
    return 0;
  }
  iVar1 = FUN_004eb4b0();
  if (iVar1 != 0) {
    if (DAT_005187bc != (FILE *)0x0) {
      FID_conflict__fwprintf(DAT_005187bc,(wchar_t *)s_failed_spot_edgefarm_005187d4);
      _fflush(DAT_005187bc);
    }
    return 0;
  }
  iVar1 = param_2 * 0x1d4;
  iVar2 = FUN_00445200(DAT_0051c960 + iVar1,0,param_3,param_4,0,0,0);
  if (iVar2 != 0) {
    if (param_7 == 0) {
      iVar2 = __ftol();
      iVar3 = __ftol();
      pcVar4 = (char *)FUN_00404090(&DAT_005484c0,(int)(iVar2 + (iVar2 >> 0x1f & 0xfU)) >> 4,
                                    (int)(iVar3 + (iVar3 >> 0x1f & 0xfU)) >> 4);
      if (*pcVar4 != param_5) goto LAB_0042c756;
    }
    uVar6 = 0;
    iVar2 = DAT_0051c960 + iVar1;
    if ((*(byte *)(iVar2 + 0xea) & 0x20) != 0) {
      iVar3 = FUN_004eb5c0();
      if (iVar3 == 0) {
        FUN_00490040();
      }
      piVar7 = (int *)(DAT_005395dc + iVar3 * 0x24);
      if (param_6 != 0) {
        __ftol();
        uVar5 = __ftol();
        iVar1 = FUN_0042c0d0(uVar5);
        if (iVar1 == 0) {
          return 1;
        }
        (**(code **)(*piVar7 + 0x48))();
        FUN_004ab6b0(piVar7,DAT_0051cb28);
        iVar1 = FUN_004aecf0(piVar7);
        if (iVar1 != 0) {
          uVar5 = FUN_004abf50(piVar7);
          uVar5 = FUN_00444720(uVar5);
          FUN_004aed20(piVar7,uVar5);
        }
        (**(code **)(*piVar7 + 0x4c))(0x40);
        if (DAT_005187bc != (FILE *)0x0) {
          FID_conflict__fwprintf(DAT_005187bc,(wchar_t *)s_Isle_Made__s_at___f__f__0051880c);
          _fflush(DAT_005187bc);
        }
        return ((int)piVar7 - DAT_005395dc) / 0x24;
      }
      uVar6 = (uint)*(byte *)((int)piVar7 + 0x22);
    }
    if ((*(byte *)(iVar2 + 0xea) & 0x10) != 0) {
      uVar6 = FUN_004444c0(iVar2);
    }
    if (uVar6 == 0xffffffff) {
      FUN_00490040();
    }
    piVar7 = (int *)FUN_004ab390(param_2,0);
    *(char *)((int)piVar7 + 0x22) = (char)uVar6;
    iVar2 = FUN_004aecf0(piVar7);
    if (iVar2 != 0) {
      uVar5 = FUN_004abf50(piVar7);
      FUN_00444720(uVar5);
      FUN_004aed20(piVar7);
    }
    if (param_7 == 0) {
      FUN_004bda70(&DAT_00583188);
    }
    (**(code **)(*piVar7 + 0x74))();
    (**(code **)(*piVar7 + 0x90))(param_2,param_3,0);
    if (DAT_005187bc != (FILE *)0x0) {
      FID_conflict__fwprintf
                (DAT_005187bc,(wchar_t *)s_Made__s_at___f__f__00518850,DAT_0051c960 + iVar1,
                 (double)fStack_4,(double)unaff_retaddr);
      _fflush(DAT_005187bc);
    }
    return ((int)piVar7 - DAT_005395dc) / 0x24;
  }
LAB_0042c756:
  if (DAT_005187bc != (FILE *)0x0) {
    FID_conflict__fwprintf
              (DAT_005187bc,(wchar_t *)s_failed__legaldrop__f__f_00518864,(double)param_3,
               (double)param_4);
    _fflush(DAT_005187bc);
  }
  return 0;
}


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


