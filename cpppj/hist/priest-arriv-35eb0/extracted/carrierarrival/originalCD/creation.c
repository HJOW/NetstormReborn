// ==== FUN_0040c950 @ 0040c950

undefined4 __fastcall FUN_0040c950(int *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 unaff_EBX;
  
  iVar3 = DAT_00540a20;
  uVar4 = *(undefined4 *)((int)param_1 + 0xe);
  if (DAT_00540a20 != 0) {
    iVar1 = __ftol();
    iVar2 = __ftol();
    if ((*(byte *)(iVar1 * 0x100 + iVar2 + DAT_0052fe48) & 6) == 0) {
      if (DAT_00540a2c != 0) {
        (**(code **)(*param_1 + 0x48))(0);
        FUN_00440a80(&stack0xfffffff4);
        (**(code **)(*param_1 + 0x90))(unaff_EBX,uVar4,0);
      }
      return 0xb;
    }
    if (iVar3 != 0) {
      iVar3 = FUN_0040c3b0(param_1);
      if ((iVar3 != 0) && (DAT_00540a2c != 0)) {
        (**(code **)(*param_1 + 0x48))(0);
        FUN_00440a80(&stack0xfffffff4);
        (**(code **)(*param_1 + 0x90))(unaff_EBX,uVar4,0);
        return 0xb;
      }
    }
  }
  uVar4 = thunk_FUN_004e45c0(param_1);
  return uVar4;
}


// ==== FUN_0040d200 @ 0040d200

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_0040d200(void *this,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  float10 fVar4;
  
  if (DAT_00540a2c != 0) {
    iVar1 = FUN_004aed30();
    if (*(short *)((int)this + 0x1a) < iVar1) {
      iVar1 = FUN_0040c3d0(this);
      if (iVar1 != 0) {
        iVar1 = FUN_004bd9f0(&DAT_00583188,*(undefined4 *)((int)this + 0xe),
                             *(undefined4 *)((int)this + 0x12));
        iVar3 = DAT_005395dc + iVar1 * 0x24;
        if ((iVar1 != 0) && (*(char *)(iVar3 + 0x20) == *(char *)((int)this + 0x20))) {
          fVar4 = (float10)FUN_00440b20(*(undefined4 *)((int)this + 0xe),
                                        *(undefined4 *)((int)this + 0x12),iVar1);
          if (fVar4 <= (float10)_DAT_00500398) {
            FUN_004ac4d0(iVar3,*(undefined1 *)((int)this + 10),*(undefined2 *)((int)this + 0xc),0x20
                         ,0,0);
            (**(code **)(*(int *)this + 0x10))(0);
            return 0x3e9;
          }
        }
      }
    }
  }
  iVar1 = FUN_0040c400(this);
  if (iVar1 != 0) {
    FUN_0040d030(this);
    return 0x3e9;
  }
  uVar2 = FUN_004e66a0(this,param_2,param_3);
  return uVar2;
}


// ==== FUN_004e66a0 @ 004e66a0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_004e66a0(void *this,int param_2,int param_3)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  uint uVar9;
  int iVar10;
  undefined ***pppuVar11;
  uint *puVar12;
  undefined4 *puVar13;
  char *pcVar14;
  uint local_b4 [4];
  undefined **local_a4;
  float local_a0;
  int local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined **local_64;
  float local_60;
  undefined **local_5c;
  float local_58;
  undefined **local_54;
  undefined **local_50;
  undefined **local_4c;
  int local_48;
  int local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  uVar2 = FUN_004abae0();
  if (((uVar2 & 0x30000) != 0) &&
     ((((uVar2 = FUN_004abae0(this), (uVar2 & 0x200000) == 0 ||
        (iVar3 = FUN_004e7610(this), iVar3 == 0)) || (*(int *)(iVar3 + 0x2f14) == 0)) &&
      (*(char *)((int)this + 0x20) != '\0')))) {
    local_b4[2] = 0;
    local_b4[0] = FUN_004ac560(this,0x20,DAT_0051cbac,0);
    puVar13 = (undefined4 *)((int)this + 0xe);
    iVar3 = FUN_004eb5c0(*puVar13,*(undefined4 *)((int)this + 0x12),0x10000000);
    FUN_004eb5c0(*puVar13,*(undefined4 *)((int)this + 0x12),0x2000);
    local_64 = (undefined **)FUN_004e3d70(*puVar13,*(undefined4 *)((int)this + 0x12),1);
    local_40[0] = (undefined **)FUN_004ac560(this,0x20,DAT_0051cbbc,0);
    if (local_40[0] != (undefined **)0x0) {
      iVar3 = 0;
      local_64 = (undefined **)0x0;
      local_b4[0] = 0;
    }
    iVar4 = FUN_004eb7b0(*puVar13,*(undefined4 *)((int)this + 0x12),1,DAT_0051cbd8);
    if ((iVar4 != 0) &&
       ((iVar10 = iVar4 * 0x24 + DAT_005395dc, (*(byte *)(iVar10 + 0x23) & 1) != 0 ||
        (iVar10 = FUN_004830c0(iVar10,*puVar13,*(undefined4 *)((int)this + 0x12)), iVar10 == 0)))) {
      iVar4 = 0;
    }
    FUN_004e4420(this,&local_48,*puVar13,*(undefined4 *)((int)this + 0x12));
    iVar10 = FUN_004ac560(this,0x20,0,0);
    FUN_004ac560(this,1,0,0);
    iVar5 = FUN_004e6680(this);
    if (iVar5 == 0) {
      uVar2 = FUN_004abac0(this);
      if ((((uVar2 & 0x1000000) == 0) && (local_48 != 0)) && ((local_44 != 0 || (iVar10 == 0)))) {
        FUN_004e5800(this);
      }
      else if (iVar4 == 0) {
        if ((local_64 == (undefined **)0x0) || (local_b4[0] == 0)) {
          if (iVar3 != 0) {
            iVar4 = FUN_0045b7a0(((int)this - DAT_005395dc) / 0x24);
            iVar10 = FUN_0045b7a0(iVar3);
            if (iVar4 == iVar10) {
              if ((DAT_00540a2c != 0) && (iVar4 = FUN_004d71a0(this), iVar4 == 0)) {
                if ((param_3 == 0) || (iVar4 = FUN_004e5980(this,param_3), iVar4 == 0)) {
                  param_3 = FUN_004e5a90(this);
                }
                FUN_004e3860(((int)this - DAT_005395dc) / 0x24,iVar3,param_3);
                if (param_3 != 0) {
                  local_b4[2] = 1;
                  FUN_004e62d0(this,param_3);
                }
              }
              if (((DAT_00540a2c != 0) && (local_b4[0] == 0)) &&
                 (*(short *)(DAT_005395dc + iVar3 * 0x24 + 0xc) != 0)) {
                FUN_004ac4d0(this,DAT_0051cbac,0,0x20,0,0);
                FUN_0040ac20(&local_a4);
                local_a0 = (float)CONCAT22(local_a0._2_2_,(short)iVar3);
                FUN_004c0420(&local_a4);
                FUN_004e41e0(0,0,&local_a4);
              }
            }
          }
        }
        else {
          iVar3 = (int)local_64 * 0x24 + DAT_005395dc;
          if (*(char *)(iVar3 + 0x20) == *(char *)((int)this + 0x20)) {
            iVar4 = FUN_0045b7a0(((int)this - DAT_005395dc) / 0x24);
            iVar10 = FUN_0045b7a0(local_64);
            if (iVar4 == iVar10) {
              uVar7 = __ftol();
              FUN_004d8cc0(*(undefined1 *)((int)this + 0x20),uVar7);
              FUN_0040abd0(local_40);
              FUN_004c0380(*(undefined1 *)((int)this + 0x20),local_40);
              uVar7 = 0;
              pcVar14 = s_guy_wav_00541778;
              puVar6 = (undefined4 *)FUN_004aef30(iVar3,&local_a4);
              FUN_00438c00(*puVar6,puVar6[1],pcVar14,uVar7);
              if (((DAT_00540a2c != 0) &&
                  ((**(code **)(*(int *)(local_b4[0] * 0x24 + DAT_005395dc) + 0x10))(0),
                  DAT_00540a2c != 0)) && (iVar3 = FUN_004d71a0(this), iVar3 == 0)) {
                if ((param_2 == 0) || (iVar3 = FUN_004e5a10(this,param_2), iVar3 == 0)) {
                  param_2 = FUN_004e60d0(this);
                  FUN_004e3860(((int)this - DAT_005395dc) / 0x24,param_2,0);
                }
                if (param_2 != 0) {
                  local_b4[2] = 1;
                  FUN_004e6220(this,param_2);
                }
              }
            }
          }
        }
      }
      else if (DAT_00540a2c != 0) {
        if (local_40[0] != (undefined **)0x0) {
          piVar1 = (int *)((int)local_40[0] * 0x24 + DAT_005395dc);
          iVar3 = iVar4 * 0x24 + DAT_005395dc;
          if (((&DAT_0050f6e0)
               [(uint)*(byte *)(iVar3 + 0x20) + (uint)*(byte *)((int)this + 0x20) * 9] != 0) &&
             (iVar4 = FUN_004ac560(iVar3,0x20,DAT_0051cbbc,0), iVar4 == 0)) {
            FUN_004ac4d0(iVar3,DAT_0051cbbc,(short)piVar1[3],0x20,0,0);
            (**(code **)(*piVar1 + 0x10))(0);
          }
        }
        local_b4[2] = 1;
        FUN_004e5c10(this,0x40a00000,0);
      }
    }
    if (local_b4[2] != 0) {
      return 0x3e9;
    }
    if ((DAT_00540a2c != 0) &&
       ((uVar2 = FUN_004abae0(this), (uVar2 & 0x200000) == 0 ||
        (iVar3 = FUN_004eb7b0(*puVar13,*(undefined4 *)((int)this + 0x12),1,DAT_0051cbd8), iVar3 == 0
        )))) {
      local_a4 = (undefined **)*puVar13;
      local_a0 = *(float *)((int)this + 0x12);
      local_40[0] = &PTR_FUN_005003d0;
      local_8 = 1;
      local_4 = 0;
      FUN_00440a50(&local_a4);
      uVar7 = __ftol();
      uVar8 = __ftol();
      FUN_004eae20(local_40,uVar8,uVar7,uVar8,uVar7);
      while (local_c != 0) {
        uVar2 = FUN_004abae0(local_c * 0x24 + DAT_005395dc);
        if ((((uVar2 & 0x30000) != 0) && (((int)this - DAT_005395dc) / 0x24 != local_c)) &&
           (local_48 != local_c)) {
          uVar7 = FUN_004e5c10(this,0x3f800000,0xffffffff);
          return uVar7;
        }
        if ((*(byte *)(local_c * 0x24 + 10 + DAT_005395dc) == DAT_0051ca94) &&
           (iVar3 = FUN_004d71a0(this), iVar3 == 0)) {
          uVar7 = FUN_004e5c10(this,0x40000000,0x1da1a);
          return uVar7;
        }
        FUN_004eafe0(local_40);
      }
      iVar3 = FUN_004d71a0(this);
      if ((iVar3 == 0) && (uVar2 = FUN_004abae0(this), (uVar2 & 0x20000) == 0)) {
        iVar3 = 1;
        pppuVar11 = &local_64;
        do {
          FUN_0041f9d0(pppuVar11);
          iVar3 = iVar3 + -1;
          pppuVar11 = pppuVar11 + 2;
        } while (-1 < iVar3);
        local_64 = (undefined **)*puVar13;
        local_a0 = *(float *)((int)this + 0x12) + _DAT_00506d90;
        local_a4 = local_64;
        local_b4[2] = 0x3f800000;
        local_60 = *(float *)((int)this + 0x12);
        puVar12 = local_b4;
        local_5c = local_64;
        pppuVar11 = &local_64;
        local_58 = local_a0;
        local_b4[3] = 0x40000000;
        local_b4[0] = 0x4200;
        local_b4[1] = 0x200;
        local_54 = (undefined **)0x0;
        do {
          local_50 = *pppuVar11;
          local_4c = pppuVar11[1];
          local_a4 = &PTR_FUN_005003d0;
          local_6c = 1;
          local_68 = 0;
          FUN_00440a50(&local_50);
          uVar7 = __ftol();
          uVar8 = __ftol();
          FUN_004eae20(&local_a4,uVar8,uVar7,uVar8,uVar7);
          if (local_70 != 0) {
            uVar2 = *puVar12;
            do {
              uVar9 = FUN_004abae0(local_70 * 0x24 + DAT_005395dc);
              if ((uVar2 & uVar9) != 0) {
                uVar7 = FUN_004e5c10(this,local_b4[(int)local_54 + 2],0x1da1a);
                return uVar7;
              }
              FUN_004eafe0(&local_a4);
            } while (local_70 != 0);
          }
          puVar12 = puVar12 + 1;
          pppuVar11 = pppuVar11 + 2;
          local_54 = (undefined **)((int)local_54 + 1);
        } while (pppuVar11 < &local_54);
      }
    }
  }
  return 1000;
}


// ==== thunk_FUN_004e45c0 @ 004e7910

undefined4 __thiscall thunk_FUN_004e45c0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined **appuStack_40 [13];
  int iStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar1 = (**(code **)(*(int *)this + 0xcc))();
  if (iVar1 != 0) {
    return 0xc;
  }
  uStack_44 = *(undefined4 *)((int)this + 0x12);
  appuStack_40[0] = &PTR_FUN_005003d0;
  uStack_48 = *(undefined4 *)((int)this + 0xe);
  uStack_8 = 0;
  uStack_4 = 0;
  FUN_00440a50(&uStack_48);
  uVar2 = __ftol();
  uVar3 = __ftol();
  FUN_004eae20(appuStack_40,uVar3,uVar2,uVar3,uVar2);
  while( true ) {
    if (iStack_c == 0) {
      uVar2 = FUN_004ac460();
      return uVar2;
    }
    if (((*(byte *)(iStack_c * 0x24 + 10 + DAT_005395dc) == DAT_0051cbb4) &&
        (((int)this - DAT_005395dc) / 0x24 != iStack_c)) &&
       (iVar1 = FUN_004e7980(iStack_c), iVar1 != 0)) break;
    FUN_004eafe0(appuStack_40);
  }
  return 0xc;
}


// ==== FUN_004e45c0 @ 004e45c0

undefined4 __thiscall FUN_004e45c0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined **appuStack_40 [13];
  int iStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  iVar1 = (**(code **)(*(int *)this + 0xcc))();
  if (iVar1 != 0) {
    return 0xc;
  }
  uStack_44 = *(undefined4 *)((int)this + 0x12);
  appuStack_40[0] = &PTR_FUN_005003d0;
  uStack_48 = *(undefined4 *)((int)this + 0xe);
  uStack_8 = 0;
  uStack_4 = 0;
  FUN_00440a50(&uStack_48);
  uVar2 = __ftol();
  uVar3 = __ftol();
  FUN_004eae20(appuStack_40,uVar3,uVar2,uVar3,uVar2);
  while( true ) {
    if (iStack_c == 0) {
      uVar2 = FUN_004ac460();
      return uVar2;
    }
    if (((*(byte *)(iStack_c * 0x24 + 10 + DAT_005395dc) == DAT_0051cbb4) &&
        (((int)this - DAT_005395dc) / 0x24 != iStack_c)) &&
       (iVar1 = FUN_004e7980(iStack_c), iVar1 != 0)) break;
    FUN_004eafe0(appuStack_40);
  }
  return 0xc;
}


// ==== FUN_004e6680 @ 004e6680

undefined4 __thiscall FUN_004e6680(void)

{
  int iVar1;
  
  iVar1 = FUN_004e7610();
  if ((iVar1 != 0) && (*(int *)(iVar1 + 0x2f18) != 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0040abd0 @ 0040abd0

void * __thiscall FUN_0040abd0(void *this)

{
  undefined1 uVar1;
  
  uVar1 = DAT_00510094;
  *(undefined1 *)this = 0;
  *(undefined2 *)((int)this + 2) = 0;
  *(undefined1 *)((int)this + 1) = 0;
  *(undefined1 *)((int)this + 1) = uVar1;
  FUN_00419090(this,DAT_0051009c);
  *(byte *)this = DAT_00510098 & 0x7f;
  return this;
}


// ==== FUN_0040ac20 @ 0040ac20

void * __thiscall FUN_0040ac20(void *this)

{
  undefined1 uVar1;
  
  uVar1 = DAT_005100a0;
  *(undefined1 *)this = 0;
  *(undefined2 *)((int)this + 2) = 0;
  *(undefined1 *)((int)this + 1) = 0;
  *(undefined1 *)((int)this + 1) = uVar1;
  FUN_00419090(this,DAT_005100a8);
  *(byte *)this = DAT_005100a4 & 0x7f;
  return this;
}


// ==== FUN_0040c3b0 @ 0040c3b0

bool __thiscall FUN_0040c3b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004aed30();
  return (int)*(short *)((int)this + 0x1a) < iVar1 / 2;
}


// ==== FUN_0040c3d0 @ 0040c3d0

undefined4 __thiscall FUN_0040c3d0(void *this)

{
  if (DAT_005178d4 != 0) {
    return 0;
  }
  if ((*(char *)((int)this + 0x20) == '\0') && (DAT_0052e170 == 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_0040c400 @ 0040c400

undefined4 __thiscall FUN_0040c400(void *this)

{
  int iVar1;
  
  if (DAT_00540a2c != 0) {
    iVar1 = FUN_0040c3d0();
    if (iVar1 != 0) {
      iVar1 = FUN_0040c3b0(this);
      if (iVar1 == 0) {
        return 1;
      }
    }
  }
  return 0;
}


// ==== FUN_0040d030 @ 0040d030

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_0040d030(void *this)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined4 uVar5;
  bool bVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  bool bVar11;
  float10 fVar12;
  
  bVar11 = false;
  pfVar1 = (float *)((int)this + 0xe);
  iVar7 = FUN_004bd9f0(&DAT_00583188,*pfVar1,*(undefined4 *)((int)this + 0x12));
  if (iVar7 != 0) {
    iVar8 = FUN_004aed30(this);
    if ((*(short *)((int)this + 0x1a) < iVar8) &&
       (*(char *)(iVar7 * 0x24 + 0x20 + DAT_005395dc) == *(char *)((int)this + 0x20))) {
      iVar8 = FUN_004abf50();
      iVar9 = __ftol();
      iVar10 = (int)this - DAT_005395dc;
      *(float *)(iVar8 + 0xe0) = *(float *)(iVar8 + 0xe0) * _DAT_005003ac;
      iVar7 = FUN_0047d410(iVar10 / 0x24,iVar7,0,0);
      bVar11 = iVar7 != 0;
      *(float *)(iVar8 + 0xe0) = (float)iVar9;
    }
  }
  if (!bVar11) {
    bVar6 = false;
    iVar7 = FUN_004bdd80(&DAT_00583188,*pfVar1,*(undefined4 *)((int)this + 0x12));
    do {
      fVar2 = *pfVar1;
      fVar3 = *(float *)((int)this + 0x12);
      fVar12 = (float10)FUN_0048cc50(0xc1a00000,0x41a00000);
      fVar2 = (float)(fVar12 + (float10)fVar2);
      fVar12 = (float10)FUN_0048cc50(0xc1a00000,0x41a00000);
      fVar3 = (float)(fVar12 + (float10)fVar3);
      if (iVar7 == 0) {
LAB_0040d1a5:
        iVar8 = FUN_0047d150(((int)this - DAT_005395dc) / 0x24,fVar2,fVar3,0,0,0,0);
        bVar11 = iVar8 != 0;
        if (!bVar11) {
          bVar6 = true;
        }
      }
      else {
        iVar8 = FUN_004bdd80(&DAT_00583188,fVar2,fVar3);
        if (iVar8 != 0) {
          fVar4 = *pfVar1;
          uVar5 = *(undefined4 *)((int)this + 0x12);
          iVar8 = FUN_004bdc50(&DAT_00583188,fVar2,fVar3);
          iVar9 = FUN_004bdc50(&DAT_00583188,fVar4,uVar5);
          if (iVar8 == iVar9) goto LAB_0040d1a5;
        }
      }
    } while ((!bVar11) && (!bVar6));
  }
  return;
}


// ==== FUN_0041f9d0 @ 0041f9d0

void * __thiscall FUN_0041f9d0(void *this)

{
  return this;
}


// ==== FUN_00438c00 @ 00438c00

void __cdecl
FUN_00438c00(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00437b29(param_3,param_4,0);
  FUN_00438ae0(param_1,param_2,uVar1);
  return;
}


// ==== FUN_00440a50 @ 00440a50

void __thiscall FUN_00440a50(void *this)

{
  int iVar1;
  
  iVar1 = __ftol();
  *(float *)this = (float)iVar1;
  iVar1 = __ftol();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== FUN_00440a80 @ 00440a80

void __thiscall FUN_00440a80(void *this)

{
  int iVar1;
  
  iVar1 = __ftol();
  *(float *)this = (float)iVar1;
  iVar1 = __ftol();
  *(float *)((int)this + 4) = (float)iVar1;
  return;
}


// ==== FUN_00440b20 @ 00440b20

float10 __cdecl FUN_00440b20(float param_1,float param_2,int param_3)

{
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  FUN_004aeec0(param_3 * 0x24 + DAT_005395dc,&local_10,0);
  if (local_10 <= param_1) {
    local_10 = param_1 - local_8;
  }
  else {
    local_10 = local_10 - param_1;
  }
  if (local_c <= param_2) {
    local_c = param_2 - local_4;
  }
  else {
    local_c = local_c - param_2;
  }
  if (local_10 < local_c) {
    return (float10)local_c;
  }
  return (float10)local_10;
}


// ==== FUN_0045b7a0 @ 0045b7a0

int __cdecl FUN_0045b7a0(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_0045b6f0(param_1);
  if (iVar1 == 0xfe) {
    return 0;
  }
  return DAT_005207f0 + iVar1 * 6;
}


// ==== FUN_004830c0 @ 004830c0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_004830c0(void *this,float param_2,float param_3)

{
  if ((*(float *)((int)this + 0xe) - _DAT_00504058 == param_2) &&
     (*(float *)((int)this + 0x12) - _DAT_00504058 == param_3)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004abac0 @ 004abac0

undefined4 __thiscall FUN_004abac0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xe8 + DAT_0051c960);
}


// ==== FUN_004abae0 @ 004abae0

undefined4 __thiscall FUN_004abae0(void *this)

{
  return *(undefined4 *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0xec + DAT_0051c960);
}


// ==== FUN_004ac460 @ 004ac460

undefined4 __cdecl FUN_004ac460(void)

{
  return 10;
}


// ==== FUN_004ac4d0 @ 004ac4d0

int __thiscall
FUN_004ac4d0(void *this,undefined2 param_2,undefined4 param_3,int param_4,uint param_5,
            undefined1 param_6)

{
  undefined4 uVar1;
  int *piVar2;
  int iVar3;
  
  if (param_4 == 0) {
    param_4 = 0x20;
  }
  uVar1 = 0;
  if ((param_5 & 0x20) == 0) {
    if ((param_5 & 0x10) != 0) {
      uVar1 = 2;
    }
  }
  else {
    uVar1 = 1;
  }
  piVar2 = (int *)FUN_004ab390(DAT_0051c978,uVar1);
  *(undefined2 *)((int)piVar2 + 0x12) = param_2;
  FUN_004abb00(piVar2,param_3);
  *(short *)(piVar2 + 5) = (short)param_4;
  iVar3 = (int)this - DAT_005395dc;
  *(undefined1 *)((int)piVar2 + 0x1a) = param_6;
  (**(code **)(*piVar2 + 0x50))(iVar3 / 0x24,param_5);
  return ((int)piVar2 - DAT_005395dc) / 0x24;
}


// ==== FUN_004ac560 @ 004ac560

int __thiscall FUN_004ac560(void *this,uint param_2,uint param_3,int param_4)

{
  bool bVar1;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  local_10[0] = &PTR_FUN_005004b0;
  local_4 = 0;
  FUN_004eb8d0(local_10,((int)this - DAT_005395dc) / 0x24);
  do {
    if (local_8 == 0) {
      return 0;
    }
    bVar1 = true;
    if ((param_3 != 0) && (*(ushort *)(local_8 * 0x24 + 0x12 + DAT_005395dc) != param_3)) {
      bVar1 = false;
    }
    if ((param_2 != 0) && ((param_2 & *(ushort *)(local_8 * 0x24 + 0x14 + DAT_005395dc)) == 0)) {
      bVar1 = false;
    }
    if (bVar1) {
      if (param_4 == 0) {
        return (local_8 * 0x24) / 0x24;
      }
      param_4 = param_4 + -1;
    }
    FUN_004eb900(local_10);
  } while( true );
}


// ==== FUN_004aed30 @ 004aed30

undefined4 __thiscall FUN_004aed30(void *this)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x400000) == 0) {
    uVar2 = FUN_00444720((uint)*(byte *)((int)this + 10) * 0x1d4 + DAT_0051c960);
    return uVar2;
  }
  if ((*(byte *)((int)this + 0x23) & 1) == 0) {
    return 0;
  }
  uVar2 = FUN_00444720(DAT_0051cbd4 * 0x1d4 + DAT_0051c960);
  return uVar2;
}


// ==== FUN_004aef30 @ 004aef30

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_004aef30(void *this,float *param_2)

{
  float fVar1;
  undefined4 uVar2;
  int *piVar3;
  int *piVar4;
  undefined4 uVar5;
  int local_10 [3];
  int local_4;
  
  piVar4 = &local_4;
  uVar5 = 0;
  piVar3 = local_10;
  uVar2 = FUN_004abf50(this,piVar3,piVar4,0);
  FUN_004442b0(uVar2,piVar3,piVar4,uVar5);
  fVar1 = *(float *)((int)this + 0x12);
  *param_2 = (float)local_10[0] * _DAT_005058cc + *(float *)((int)this + 0xe);
  param_2[1] = (float)local_4 * _DAT_005058cc + fVar1;
  return;
}


// ==== FUN_004bd9f0 @ 004bd9f0

undefined4 __thiscall FUN_004bd9f0(void *this)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar1 = __ftol();
  uVar2 = __ftol(uVar1);
  iVar3 = FUN_004bdbf0(uVar2,uVar1);
  if (iVar3 == 0x7f) {
    return 0;
  }
  uVar1 = FUN_004bd9c0(this,iVar3);
  return uVar1;
}


// ==== FUN_004c0380 @ 004c0380

void __cdecl FUN_004c0380(int param_1,undefined4 param_2)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_004bf830(param_1);
  if ((((param_1 < 1) || (8 < param_1)) ||
      ((*(byte *)(param_1 * 0xac + 0x50 + DAT_0050f834) & 1) != 0)) && (piVar1 != (int *)0x0)) {
    (**(code **)(*piVar1 + 4))(param_2);
  }
  return;
}


// ==== FUN_004c0420 @ 004c0420

void __cdecl FUN_004c0420(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  
  iVar2 = 1;
  iVar4 = 0xac;
  piVar3 = &DAT_0053d614;
  iVar1 = DAT_0050f834;
  do {
    if ((((*(byte *)(iVar4 + 0x50 + iVar1) & 1) != 0) && ((int *)*piVar3 != (int *)0x0)) &&
       (DAT_0050f6c8 != iVar2)) {
      (**(code **)(*(int *)*piVar3 + 4))(param_1);
      iVar1 = DAT_0050f834;
    }
    piVar3 = piVar3 + 1;
    iVar4 = iVar4 + 0xac;
    iVar2 = iVar2 + 1;
  } while (iVar4 < 0x60c);
  return;
}


// ==== FUN_004d71a0 @ 004d71a0

undefined4 __thiscall FUN_004d71a0(void *this)

{
  if ((*(byte *)((int)this + 0x20) != 0) &&
     (*(int *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0xa8 + DAT_0050f834) != 0)) {
    return 1;
  }
  return 0;
}


// ==== FUN_004d8cc0 @ 004d8cc0

void __cdecl FUN_004d8cc0(undefined4 param_1,int param_2)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_004d81f0(param_1);
  if (iVar1 != 0) {
    *(int *)(iVar1 + 0x2d8) = *(int *)(iVar1 + 0x2d8) + param_2;
    iVar2 = FUN_004175a0();
    if (iVar2 != 0) {
      iVar2 = *(int *)(iVar1 + 0x2d8);
      if (iVar2 < 0x4a7) {
        iVar2 = 0x4a6;
      }
      *(int *)(iVar1 + 0x2d8) = iVar2;
    }
  }
  return;
}


// ==== FUN_004e3860 @ 004e3860

void __cdecl FUN_004e3860(int param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_0047d020(*(undefined1 *)(param_1 * 0x24 + 10 + DAT_005395dc),0,0);
  iVar2 = FUN_004eb310(param_1,uVar1);
  if (iVar2 != 0) {
    iVar2 = FUN_004af4e0(iVar2 * 0x24 + DAT_005395dc);
    *(undefined4 *)(iVar2 + 0x2f0c) = param_2;
    *(undefined4 *)(iVar2 + 0x2f10) = param_3;
  }
  return;
}


// ==== FUN_004e3d70 @ 004e3d70

int __cdecl FUN_004e3d70(undefined4 param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_4 = 0;
  local_c = 0;
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = 1;
  if (param_3 == 0) {
    uVar1 = __ftol();
    uVar2 = __ftol();
    uVar4 = uVar2;
    uVar3 = uVar1;
  }
  else {
    uVar3 = __ftol();
    uVar4 = __ftol(uVar3);
    uVar1 = __ftol(uVar4);
    uVar2 = __ftol(uVar1);
  }
  FUN_004eae20(local_40,uVar2,uVar1,uVar4,uVar3);
  if (local_c != 0) {
    do {
      iVar5 = local_c * 0x24 + DAT_005395dc;
      if (((*(byte *)(iVar5 + 0x23) & 1) == 0) && ((*(byte *)(iVar5 + 0xb) & 2) == 0)) {
        uVar4 = FUN_004abf50();
        iVar5 = FUN_004e3d40(uVar4);
        if (iVar5 != 0) {
          return local_c;
        }
      }
      FUN_004eafe0(local_40);
    } while (local_c != 0);
    return 0;
  }
  return 0;
}


// ==== FUN_004e41e0 @ 004e41e0

void __cdecl FUN_004e41e0(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  undefined4 uStack_104;
  uint local_100;
  undefined4 local_fc;
  
  piVar3 = (int *)(DAT_005395dc + (uint)*(ushort *)(param_3 + 4) * 0x24);
  if ((*(byte *)((int)piVar3 + 0xb) & 1) == 0) {
    iVar1 = FUN_0048cc10(1000,7000);
    uVar2 = FUN_00420560(&local_100,s_geyserMined_d_wav_00541628,iVar1 / 1000);
    FUN_00438c00(*(uint *)((int)piVar3 + 0xe),*(undefined4 *)((int)piVar3 + 0x12),uVar2,0);
    local_100 = (uint)*(ushort *)(piVar3 + 3);
    uVar2 = __ftol();
    FUN_004abb00(piVar3,uVar2);
    if ((DAT_00540a2c != 0) && ((short)piVar3[3] == 0)) {
      local_100 = *(uint *)((int)piVar3 + 0xe);
      local_fc = *(undefined4 *)((int)piVar3 + 0x12);
      (**(code **)(*piVar3 + 0x10))(0x1000);
      iVar1 = FUN_004bdd80(&DAT_00583188,uStack_104,local_100);
      if (iVar1 == 0) {
        piVar3 = (int *)FUN_004ab390(DAT_0051ca94,0);
        (**(code **)(*piVar3 + 0x90))(uStack_104,local_100,0);
      }
    }
  }
  return;
}


// ==== FUN_004e4420 @ 004e4420

void __thiscall FUN_004e4420(void *this,int *param_2,undefined4 param_3,undefined4 param_4)

{
  byte bVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  bool bVar7;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  FUN_00440a80(&param_3);
  local_48 = param_3;
  local_8 = 0;
  local_44 = param_4;
  local_4 = 0;
  local_40[0] = &PTR_FUN_005003d0;
  FUN_00440a50(&local_48);
  uVar2 = __ftol();
  uVar3 = __ftol();
  FUN_004eae20(local_40,uVar3,uVar2,uVar3,uVar2);
  do {
    if (local_c == 0) {
      *param_2 = 0;
      param_2[1] = 0;
      return;
    }
    uVar4 = FUN_004abae0(local_c * 0x24 + DAT_005395dc);
    if ((uVar4 & 0x20a000) != 0) {
      bVar7 = true;
      if ((uVar4 & 0x200000) != 0) {
        iVar5 = FUN_0040d7b0((local_c * 0x24) / 0x24);
        bVar7 = iVar5 != 0;
        if ((*(char *)(local_c * 0x24 + 0x20 + DAT_005395dc) == *(char *)((int)this + 0x20)) ||
           ((uVar6 = FUN_004abae0(this), (uVar6 & 0x200000) != 0 &&
            (iVar5 = FUN_004a9400(s_priestPickupPriest_0054164c), iVar5 == 0)))) {
          bVar7 = false;
        }
        else if (DAT_00541578 != 0) {
          bVar1 = *(byte *)(local_c * 0x24 + 0x20 + DAT_005395dc);
          if ((bVar1 != *(byte *)((int)this + 0x20)) &&
             ((&DAT_0050f6e0)[(uint)*(byte *)((int)this + 0x20) * 9 + (uint)bVar1] != 0)) {
            bVar7 = true;
          }
        }
      }
      if (bVar7) {
        *param_2 = local_c;
        param_2[1] = (uVar4 & 0x2000) >> 0xd;
        return;
      }
    }
    FUN_004eafe0(local_40);
  } while( true );
}


// ==== FUN_004e5800 @ 004e5800

void __thiscall FUN_004e5800(void *this)

{
  int iVar1;
  undefined4 local_14;
  undefined2 local_10;
  undefined2 local_e;
  undefined1 local_c;
  int local_8 [2];
  
  local_14 = *(undefined4 *)((int)this + 0xe);
  FUN_004e4420(this,local_8,local_14,*(undefined4 *)((int)this + 0x12));
  if (local_8[0] != 0) {
    iVar1 = DAT_005395dc + local_8[0] * 0x24;
    if (DAT_00540a2c != 0) {
      if (DAT_0052e170 == 0) {
        FUN_004abae0(iVar1);
      }
      if (DAT_00540a2c != 0) {
        FUN_0040a7f0(&local_14);
        local_10 = (undefined2)(((int)this - DAT_005395dc) / 0x24);
        local_e = (undefined2)local_8[0];
        local_c = *(undefined1 *)(iVar1 + 10);
        FUN_004c0380(9,&local_14);
      }
    }
  }
  return;
}


// ==== FUN_004e5980 @ 004e5980

undefined4 __thiscall FUN_004e5980(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = DAT_005395dc + param_2 * 0x24;
  if (((*(char *)((int)this + 0x20) == *(char *)(iVar4 + 0x20)) &&
      ((*(byte *)(iVar4 + 0x23) & 9) == 0)) && ((*(byte *)(iVar4 + 0xb) & 2) == 0)) {
    uVar2 = FUN_004abae0(this);
    iVar1 = DAT_005395dc;
    if ((uVar2 & 0x20000) == 0) {
      iVar3 = FUN_0045b7a0(((int)this - DAT_005395dc) / 0x24);
      iVar4 = FUN_0045b7a0((iVar4 - iVar1) / 0x24);
      if (iVar3 != iVar4) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


// ==== FUN_004e5a10 @ 004e5a10

undefined4 __thiscall FUN_004e5a10(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  
  iVar1 = DAT_005395dc + param_2 * 0x24;
  if ((((*(byte *)(iVar1 + 0x23) & 9) == 0) && ((*(byte *)(iVar1 + 0xb) & 2) == 0)) &&
     (*(byte *)(iVar1 + 10) == DAT_0051cb2c)) {
    uVar2 = FUN_004abae0(this);
    if ((uVar2 & 0x20000) == 0) {
      iVar1 = FUN_0045b7a0(((int)this - DAT_005395dc) / 0x24);
      iVar3 = FUN_0045b7a0(param_2);
      if (iVar1 != iVar3) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}


// ==== FUN_004e5a90 @ 004e5a90

undefined4 __thiscall FUN_004e5a90(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *unaff_FS_OFFSET;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004e5bb5;
  *unaff_FS_OFFSET = &local_10;
  local_20 = FUN_004f1650(0x50);
  local_18 = 0;
  local_1c = 0x14;
  local_8 = 1;
  iVar4 = *(int *)((uint)*(byte *)(local_14 + 0x20) * 0xac + 0x74 + DAT_0050f834);
  if (iVar4 != 0) {
    iVar1 = FUN_004e5980(local_14,iVar4);
    if ((iVar1 != 0) && (local_18 < local_1c)) {
      *(int *)(local_20 + local_18 * 4) = iVar4;
      local_18 = local_18 + 1;
    }
  }
  iVar4 = 0;
  iVar1 = 0;
  if (0 < DAT_005670e8) {
    do {
      if (DAT_005670e4 <= iVar4) break;
      if (DAT_005670e8 == 0) {
        iVar3 = 0;
      }
      else {
        iVar3 = *(int *)(DAT_005670e0 + (iVar4 % DAT_005670e8) * 4);
      }
      if (iVar3 != 0) {
        iVar2 = FUN_004e5980(local_14,iVar3);
        if ((iVar2 != 0) && (local_18 < local_1c)) {
          *(int *)(local_20 + local_18 * 4) = iVar3;
          local_18 = local_18 + 1;
        }
        iVar1 = iVar1 + 1;
      }
      iVar4 = iVar4 + 1;
    } while (iVar1 < DAT_005670e8);
  }
  iVar4 = (local_14 - DAT_005395dc) / 0x24;
  local_14 = FUN_004e7ca0(&local_20,iVar4,iVar4);
  local_8 = 0xffffffff;
  FUN_004e5bbf();
  *unaff_FS_OFFSET = local_10;
  return local_14;
}


// ==== FUN_004e5c10 @ 004e5c10

undefined4 __thiscall FUN_004e5c10(void)

{
  float fVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int extraout_ECX;
  uint uVar8;
  undefined4 *puVar9;
  undefined1 *puVar10;
  bool bVar11;
  float10 fVar12;
  float fVar13;
  float fStack00000004;
  float fStack00000024;
  undefined **in_stack_00000048;
  int in_stack_0000007c;
  undefined4 in_stack_00000080;
  undefined4 in_stack_00000084;
  float in_stack_00000088;
  float in_stack_0000008c;
  float in_stack_00002f74;
  int in_stack_00002f78;
  
  FUN_004f1bd0();
  iVar2 = FUN_004e7660();
  iVar2 = iVar2 + 1;
  if (iVar2 < 4) {
    if ((in_stack_00002f74 == 1.0) && (uVar4 = FUN_004abae0(extraout_ECX), (uVar4 & 0x20000) != 0))
    {
      uVar3 = 1000;
    }
    else {
      fVar13 = *(float *)(extraout_ECX + 0x12);
      uVar4 = 0;
      fVar1 = *(float *)(extraout_ECX + 0xe);
      bVar11 = in_stack_00002f78 == 0x1da1a;
      if (bVar11) {
        in_stack_00002f78 = -1;
        uVar4 = 10;
      }
      if (in_stack_00002f78 == -1) {
        iVar5 = FUN_0048cc10();
        iVar5 = iVar5 / 1000;
      }
      else {
        iVar5 = in_stack_00002f78 / 2;
      }
      if (uVar4 < 4) {
        do {
          in_stack_00000088 = (float)(int)(&DAT_0051c330)[iVar5 * 2] * in_stack_00002f74 + fVar1;
          in_stack_0000008c = (float)(int)(&DAT_0051c350)[iVar5 * 2] * in_stack_00002f74 + fVar13;
          iVar6 = __ftol();
          iVar7 = __ftol();
          if (((*(byte *)(iVar6 * 0x100 + iVar7 + DAT_0052fe48) & 0x10) == 0) &&
             (iVar6 = FUN_0047d150(), iVar6 != 0)) {
            *(int *)(iVar6 + 0x2f18) = iVar2;
            goto LAB_004e60bb;
          }
          uVar4 = uVar4 + 1;
          uVar8 = (int)(iVar5 + 1U) >> 0x1f;
          iVar5 = ((iVar5 + 1U ^ uVar8) - uVar8 & 3 ^ uVar8) - uVar8;
        } while ((int)uVar4 < 4);
      }
      if (bVar11) {
        in_stack_00000048 = &PTR_FUN_005003d0;
        in_stack_00000080 = 0;
        in_stack_00000084 = 0;
        in_stack_00000088 = fVar1;
        in_stack_0000008c = fVar13;
        FUN_00440a80(&stack0x00000088);
        __ftol();
        __ftol();
        FUN_004eae20(&stack0x00000048);
        fStack00000004 = -10000.0;
        fVar13 = -10000.0;
        fStack00000024 = 10000.0;
        while (in_stack_0000007c != 0) {
          uVar4 = FUN_004abac0(DAT_005395dc + in_stack_0000007c * 0x24);
          iVar5 = DAT_0052fe48;
          if ((uVar4 & 0x800) != 0) {
            iVar6 = __ftol();
            iVar7 = __ftol();
            if (((*(byte *)(iVar6 * 0x100 + iVar7 + iVar5) & 0x10) == 0) &&
               (iVar5 = FUN_004eb5c0(), iVar5 == 0)) {
              iVar5 = 599;
              puVar9 = &stack0x0000008c;
              do {
                FUN_0041f9d0(puVar9);
                iVar5 = iVar5 + -1;
                puVar9 = puVar9 + 2;
              } while (-1 < iVar5);
              iVar5 = 599;
              puVar10 = &stack0x00001350;
              do {
                FUN_00401590(puVar10);
                iVar5 = iVar5 + -1;
                puVar10 = puVar10 + 8;
              } while (-1 < iVar5);
              in_stack_00000088 = 0.0;
              FUN_0047cff0();
              FUN_00402100();
              if (in_stack_00000088 != 0.0) {
                FUN_004abf50(extraout_ECX);
                FUN_004abf50(extraout_ECX);
                FUN_00402590(&stack0x00000088);
                iVar5 = (int)in_stack_00000088 + -1;
                if ((int)in_stack_00000088 <= iVar5) {
                  FUN_00490040();
                }
                fVar12 = (float10)(double)CONCAT44(*(undefined4 *)(&stack0x00001354 + iVar5 * 8),
                                                   *(undefined4 *)(&stack0x00001350 + iVar5 * 8)) -
                         (float10)(double)CONCAT44(DAT_005484ac,DAT_005484a8);
                if (fVar12 < (float10)fStack00000024) {
                  iVar5 = in_stack_0000007c * 0x24 + DAT_005395dc;
                  fVar13 = *(float *)(iVar5 + 0xe);
                  fStack00000004 = *(float *)(iVar5 + 0x12);
                  fStack00000024 = (float)fVar12;
                }
              }
            }
          }
          FUN_004eafe0(&stack0x00000048);
        }
        if ((((0 < (int)fVar13) &&
             (in_stack_00000088 = (float)DAT_0052e9a8, fVar13 < in_stack_00000088)) &&
            (0 < (int)fStack00000004)) &&
           ((fStack00000004 < in_stack_00000088 && (iVar5 = FUN_0047d150(), iVar5 != 0)))) {
          *(int *)(iVar5 + 0x2f18) = iVar2;
        }
      }
LAB_004e60bb:
      uVar3 = 0x3e9;
    }
  }
  else {
    uVar3 = 1000;
  }
  return uVar3;
}


// ==== FUN_004e60d0 @ 004e60d0

undefined4 __thiscall FUN_004e60d0(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  puStack_c = &LAB_004e6203;
  *unaff_FS_OFFSET = &local_10;
  local_24 = 0;
  iVar2 = FUN_004e5a90();
  uVar1 = iVar2 * 0x24 + DAT_005395dc;
  if ((DAT_005395dc + 0x24U <= uVar1) && (uVar1 < (uint)(DAT_005395f4 * 0x24 + DAT_005395dc))) {
    FUN_0045b6f0((local_14 - DAT_005395dc) / 0x24);
    local_20 = FUN_004f1650(0x50);
    iVar5 = 0;
    local_1c = 0x14;
    local_18 = 0;
    local_8 = 1;
    if (0 < DAT_00549200) {
      do {
        if (DAT_00549200 == 0) {
          iVar4 = 0;
        }
        else {
          iVar4 = *(int *)(DAT_005491f8 + (iVar5 % DAT_00549200) * 4);
        }
        if (iVar4 != 0) {
          iVar3 = FUN_004e5a10(local_14,iVar4);
          if ((iVar3 != 0) && (local_18 < local_1c)) {
            *(int *)(local_20 + local_18 * 4) = iVar4;
            local_18 = local_18 + 1;
          }
        }
        iVar5 = iVar5 + 1;
      } while (iVar5 < DAT_00549200);
    }
    local_24 = FUN_004e7ca0(&local_20,iVar2,(local_14 - DAT_005395dc) / 0x24);
    local_8 = 0xffffffff;
    FUN_004e620d();
  }
  *unaff_FS_OFFSET = local_10;
  return local_24;
}


// ==== FUN_004e6220 @ 004e6220

undefined4 __thiscall FUN_004e6220(void *this,int param_2)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  int iVar6;
  undefined1 *puVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 local_18 [4];
  undefined1 local_8 [8];
  
  iVar6 = DAT_005395dc + param_2 * 0x24;
  iVar1 = FUN_004d71a0();
  if (iVar1 != 0) {
    return 0;
  }
  if ((*(byte *)(iVar6 + 0xb) & 1) == 0) {
    uVar2 = FUN_004abae0(iVar6);
    if ((uVar2 & 0x10000000) != 0) {
      uVar5 = *(undefined4 *)((int)this + 0x12);
      uVar13 = 0;
      uVar12 = 0;
      uVar8 = *(undefined4 *)((int)this + 0xe);
      uVar11 = 0;
      uVar10 = 0;
      uVar9 = 1;
      puVar7 = local_8;
      local_18[0] = uVar8;
      uVar3 = FUN_004aeec0(iVar6,local_18,0);
      puVar4 = (undefined4 *)FUN_00440790(uVar3,puVar7,uVar8,uVar5,uVar9);
      uVar5 = FUN_0047d150(((int)this - DAT_005395dc) / 0x24,*puVar4,puVar4[1],uVar10,uVar11,uVar12,
                           uVar13);
      return uVar5;
    }
  }
  return 0;
}


// ==== FUN_004e62d0 @ 004e62d0

undefined4 __thiscall FUN_004e62d0(void *this,int param_2)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  undefined1 *puVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_8 [8];
  
  iVar2 = FUN_004d71a0();
  iVar1 = DAT_005395dc;
  if (iVar2 != 0) {
    return 0;
  }
  uVar12 = 0;
  uVar11 = 0;
  uVar10 = 0;
  uVar5 = *(undefined4 *)((int)this + 0xe);
  uVar9 = 0;
  uVar7 = *(undefined4 *)((int)this + 0x12);
  uVar8 = 1;
  puVar6 = local_8;
  local_18 = uVar5;
  local_14 = uVar7;
  uVar3 = FUN_004aeec0(param_2 * 0x24 + DAT_005395dc,&local_18,0);
  puVar4 = (undefined4 *)FUN_00440790(uVar3,puVar6,uVar5,uVar7,uVar8);
  uVar5 = FUN_0047d150(((int)this - iVar1) / 0x24,*puVar4,puVar4[1],uVar9,uVar10,uVar11,uVar12);
  return uVar5;
}


// ==== FUN_004e7610 @ 004e7610

undefined4 __thiscall FUN_004e7610(void *this)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_0047d020(*(undefined1 *)((int)this + 10),0,0);
  iVar2 = FUN_004eb310(((int)this - DAT_005395dc) / 0x24,uVar1);
  if (iVar2 != 0) {
    uVar1 = FUN_004af4e0(iVar2 * 0x24 + DAT_005395dc);
    return uVar1;
  }
  return 0;
}


// ==== FUN_004e7980 @ 004e7980

undefined4 __cdecl FUN_004e7980(int param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  uVar1 = param_1 * 0x24 + DAT_005395dc;
  if (((uVar1 < DAT_005395dc + 0x24U) || ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar1)) ||
     (*(byte *)(uVar1 + 10) != DAT_0051cbb4)) {
    return 0;
  }
  if ((DAT_00540a2c != 0) && (iVar2 = FUN_004ac560(uVar1,0x20,0,0), iVar2 == 0)) {
    local_44 = *(undefined4 *)(uVar1 + 0x12);
    local_48 = *(undefined4 *)(uVar1 + 0xe);
    local_8 = 0;
    local_40[0] = &PTR_FUN_005003d0;
    local_4 = 0;
    FUN_00440a50(&local_48);
    uVar3 = __ftol();
    uVar4 = __ftol();
    FUN_004eae20(local_40,uVar4,uVar3,uVar4,uVar3);
    iVar2 = uVar1 - DAT_005395dc;
    if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
      do {
        if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_c * 0x24 + DAT_005395dc)) {
          return 0;
        }
        uVar5 = FUN_004abae0();
        if (((uVar5 & 0x210000) != 0) &&
           (iVar7 = local_c * 0x24 + DAT_005395dc,
           (&DAT_0050f6e0)[(uint)*(byte *)(iVar7 + 0x20) * 9 + (uint)*(byte *)(uVar1 + 0x20)] == 0))
        {
          iVar7 = FUN_004ac560(iVar7,0x20,0,0);
          iVar6 = DAT_005395dc + iVar7 * 0x24;
          if ((iVar7 != 0) &&
             ((*(ushort *)(iVar6 + 0x12) != DAT_0051cbbc ||
              ((*(ushort *)(iVar6 + 0xc) & 0x7f) != (ushort)*(byte *)(uVar1 + 0x20))))) {
            FUN_0040ad50(&local_48);
            local_44 = CONCAT22((short)iVar7,(short)(iVar2 / 0x24));
            FUN_004e4300(DAT_0050f6c8,0,&local_48);
            FUN_004c0420(&local_48);
            if ((*(byte *)((uint)*(ushort *)(iVar7 * 0x24 + 0x12 + DAT_005395dc) * 0x1d4 + 0xed +
                          DAT_0051c960) & 0x80) != 0) {
              FUN_004e5bd0(uVar1);
            }
            return 1;
          }
        }
        FUN_004eafe0(local_40);
      } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
    }
  }
  return 0;
}


// ==== FUN_004eae20 @ 004eae20

void __thiscall FUN_004eae20(void *this,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int iVar6;
  int iVar7;
  
  iVar2 = DAT_0052e9a8;
  if (param_4 == -1) {
    param_4 = DAT_0052e9a8;
  }
  if (param_5 == -1) {
    param_5 = DAT_0052e9a8;
  }
  if (param_2 < 1) {
    param_2 = 0;
  }
  *(int *)((int)this + 4) = param_2;
  if (param_3 < 1) {
    param_3 = 0;
  }
  *(int *)((int)this + 8) = param_3;
  iVar1 = iVar2 + -1;
  if (iVar1 <= param_4) {
    param_4 = iVar1;
  }
  *(int *)((int)this + 0xc) = param_4;
  if (iVar1 <= param_5) {
    param_5 = iVar1;
  }
  *(int *)((int)this + 0x10) = param_5;
  *(undefined4 *)((int)this + 0x24) = 1;
  if (*(int *)((int)this + 0x38) == 0) {
    *(undefined4 *)((int)this + 0x24) = 0;
  }
  iVar1 = *(int *)((int)this + 0x24);
  iVar2 = iVar2 / (int)(&DAT_0052d598)[iVar1] + -1;
  iVar3 = param_2 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_2 / (int)(&DAT_0052d598)[iVar1]) {
    iVar3 = iVar2;
  }
  *(int *)((int)this + 0x14) = iVar3;
  iVar6 = param_3 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_3 / (int)(&DAT_0052d598)[iVar1]) {
    iVar6 = iVar2;
  }
  *(int *)((int)this + 0x18) = iVar6;
  iVar7 = param_4 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_4 / (int)(&DAT_0052d598)[iVar1]) {
    iVar7 = iVar2;
  }
  *(int *)((int)this + 0x1c) = iVar7;
  iVar4 = param_5 / (int)(&DAT_0052d598)[iVar1];
  if (iVar2 <= param_5 / (int)(&DAT_0052d598)[iVar1]) {
    iVar4 = iVar2;
  }
  *(int *)((int)this + 0x20) = iVar4;
  *(int *)((int)this + 0x1c) = iVar7 + 1;
  *(int *)((int)this + 0x20) = iVar4 + 1;
  if (iVar7 + 1 < 0) {
    *(undefined4 *)((int)this + 0x1c) = 0;
  }
  if (iVar4 + 1 < 0) {
    *(undefined4 *)((int)this + 0x20) = 0;
  }
  if (iVar2 < *(int *)((int)this + 0x1c)) {
    *(int *)((int)this + 0x1c) = iVar2;
  }
  if (iVar2 < *(int *)((int)this + 0x20)) {
    *(int *)((int)this + 0x20) = iVar2;
  }
  *(int *)((int)this + 0x28) = iVar3;
  *(int *)((int)this + 0x2c) = iVar6;
  *(undefined4 *)((int)this + 0x34) = 0;
  puVar5 = (ushort *)FUN_00479d90(&DAT_005670c0,iVar3,iVar6,*(undefined4 *)((int)this + 0x24));
  *(uint *)((int)this + 0x30) = (uint)*puVar5;
  (**(code **)(*(int *)this + 4))();
  return;
}


// ==== FUN_004eafe0 @ 004eafe0

void __thiscall FUN_004eafe0(void *this)

{
  float *pfVar1;
  byte bVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  ushort *puVar9;
  int iVar10;
  uint uVar11;
  int local_8;
  int local_4;
  
  iVar5 = DAT_005395dc;
LAB_004eafef:
  do {
    uVar11 = *(uint *)((int)this + 0x30);
    while (uVar11 == 0) {
      iVar5 = *(int *)((int)this + 0x28) + 1;
      *(int *)((int)this + 0x28) = iVar5;
      if (*(int *)((int)this + 0x1c) < iVar5) {
        *(undefined4 *)((int)this + 0x28) = *(undefined4 *)((int)this + 0x14);
        iVar5 = *(int *)((int)this + 0x2c) + 1;
        *(int *)((int)this + 0x2c) = iVar5;
        if (*(int *)((int)this + 0x20) < iVar5) {
          iVar5 = *(int *)((int)this + 0x24) + 1;
          *(int *)((int)this + 0x24) = iVar5;
          if (3 < iVar5) {
            *(undefined4 *)((int)this + 0x34) = 0;
            return;
          }
          iVar10 = DAT_0052e9a8 / (int)(&DAT_0052d598)[iVar5] + -1;
          iVar6 = *(int *)((int)this + 4) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar6) {
            iVar6 = iVar10;
          }
          *(int *)((int)this + 0x14) = iVar6;
          iVar7 = *(int *)((int)this + 8) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar7) {
            iVar7 = iVar10;
          }
          *(int *)((int)this + 0x18) = iVar7;
          iVar8 = *(int *)((int)this + 0xc) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar8) {
            iVar8 = iVar10;
          }
          *(int *)((int)this + 0x1c) = iVar8;
          iVar5 = *(int *)((int)this + 0x10) / (int)(&DAT_0052d598)[iVar5];
          if (iVar10 <= iVar5) {
            iVar5 = iVar10;
          }
          *(int *)((int)this + 0x20) = iVar5;
          *(int *)((int)this + 0x1c) = iVar8 + 1;
          *(int *)((int)this + 0x20) = iVar5 + 1;
          if (iVar8 + 1 < 0) {
            *(undefined4 *)((int)this + 0x1c) = 0;
          }
          if (iVar5 + 1 < 0) {
            *(undefined4 *)((int)this + 0x20) = 0;
          }
          if (iVar10 < *(int *)((int)this + 0x1c)) {
            *(int *)((int)this + 0x1c) = iVar10;
          }
          if (iVar10 < *(int *)((int)this + 0x20)) {
            *(int *)((int)this + 0x20) = iVar10;
          }
          *(int *)((int)this + 0x2c) = iVar7;
          *(undefined4 *)((int)this + 0x34) = 0;
          *(int *)((int)this + 0x28) = iVar6;
        }
      }
      puVar9 = (ushort *)
               FUN_00479d90(&DAT_005670c0,*(undefined4 *)((int)this + 0x28),
                            *(undefined4 *)((int)this + 0x2c),*(undefined4 *)((int)this + 0x24));
      iVar5 = DAT_005395dc;
      uVar11 = (uint)*puVar9;
      *(uint *)((int)this + 0x30) = uVar11;
    }
    do {
      iVar10 = *(int *)((int)this + 0x30);
      *(int *)((int)this + 0x34) = iVar10;
      if (iVar10 == 0) goto LAB_004eafef;
      iVar10 = iVar10 * 0x24 + iVar5;
      bVar2 = *(byte *)(iVar10 + 0x23);
      *(uint *)((int)this + 0x30) = (uint)*(ushort *)(iVar10 + 4);
    } while ((bVar2 & 8) != 0);
    iVar10 = *(int *)((int)this + 0x34) * 0x24;
    pfVar1 = (float *)(iVar10 + 0xe + iVar5);
    fVar3 = *pfVar1;
    fVar4 = pfVar1[1];
    if (((((int)fVar3 < 1) || ((float)DAT_0052e9a8 <= fVar3)) || ((int)fVar4 < 1)) ||
       ((float)DAT_0052e9a8 <= fVar4)) {
      FUN_00490040(s_pos_isValid___00541890,s__ns_o_SquidFinder_cpp_00541878,0x89);
    }
    FUN_004442b0(DAT_0051c960 + (uint)*(byte *)(iVar10 + iVar5 + 10) * 0x1d4,&local_8,&local_4,
                 *(undefined4 *)((int)this + 0x3c));
    iVar10 = __ftol();
    iVar6 = __ftol();
    iVar8 = (iVar10 - local_8) + 1;
    iVar7 = (iVar6 - local_4) + 1;
    iVar5 = DAT_005395dc;
    if ((((((*(int *)((int)this + 4) <= iVar10) || (*(int *)((int)this + 4) <= iVar8)) &&
          ((iVar10 <= *(int *)((int)this + 0xc) || (iVar8 <= *(int *)((int)this + 0xc))))) &&
         ((*(int *)((int)this + 8) <= iVar6 || (*(int *)((int)this + 8) <= iVar7)))) &&
        ((iVar6 <= *(int *)((int)this + 0x10) || (iVar7 <= *(int *)((int)this + 0x10))))) &&
       (iVar10 = (*(code *)**(undefined4 **)this)(*(int *)((int)this + 0x34) * 0x24 + DAT_005395dc),
       iVar5 = DAT_005395dc, iVar10 != 0)) {
      return;
    }
  } while( true );
}


// ==== FUN_004eb5c0 @ 004eb5c0

int __cdecl FUN_004eb5c0(undefined4 param_1,undefined4 param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = (uint)((param_3 & 6) == 0);
  local_48 = param_1;
  local_44 = param_2;
  local_4 = 0;
  FUN_00440a50(&local_48);
  uVar1 = __ftol();
  uVar2 = __ftol();
  FUN_004eae20(local_40,uVar2,uVar1,uVar2,uVar1);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= (uint)(local_c * 0x24 + DAT_005395dc)) {
        return 0;
      }
      uVar3 = FUN_004abae0();
      if ((param_3 & uVar3) != 0) {
        return (local_c * 0x24) / 0x24;
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  return 0;
}


// ==== FUN_004eb7b0 @ 004eb7b0

int __cdecl FUN_004eb7b0(undefined4 param_1,undefined4 param_2,int param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 local_48;
  undefined4 local_44;
  undefined **local_40 [13];
  int local_c;
  uint local_8;
  undefined4 local_4;
  
  local_40[0] = &PTR_FUN_005003d0;
  local_8 = (uint)((*(uint *)(param_4 * 0x1d4 + 0xec + DAT_0051c960) & 6) == 0);
  local_48 = param_1;
  local_44 = param_2;
  local_4 = 0;
  FUN_00440a80(&local_48);
  iVar1 = __ftol();
  iVar2 = __ftol();
  FUN_004eae20(local_40,iVar2 - param_3,iVar1 - param_3,iVar2 + param_3,iVar1 + param_3);
  if (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc)) {
    do {
      uVar3 = local_c * 0x24 + DAT_005395dc;
      if ((uint)(DAT_005395f4 * 0x24 + DAT_005395dc) <= uVar3) {
        return 0;
      }
      if (*(byte *)(uVar3 + 10) == param_4) {
        return (local_c * 0x24) / 0x24;
      }
      FUN_004eafe0(local_40);
    } while (DAT_005395dc + 0x24U <= (uint)(local_c * 0x24 + DAT_005395dc));
  }
  return 0;
}


// ==== __ftol @ 004f161c

/* Library Function - Single Match
    __ftol
   
   Library: Visual Studio */

longlong __cdecl __ftol(void)

{
  float10 in_ST0;
  
  return (longlong)ROUND(in_ST0);
}


// ==== FUN_004abf50 @ 004abf50

int __thiscall FUN_004abf50(void *this)

{
  return DAT_0051c960 + (uint)*(byte *)((int)this + 10) * 0x1d4;
}


// ==== FUN_00444720 @ 00444720

int __thiscall FUN_00444720(void *this)

{
  int iVar1;
  
  if (DAT_00511c90 != 0) {
    iVar1 = FUN_004448e0();
    if (iVar1 == DAT_0051cbbc) {
      return (int)(*(int *)this + (*(int *)this >> 0x1f & 3U)) >> 2;
    }
  }
  return *(int *)this;
}


// ==== FUN_004ae3f0 @ 004ae3f0

int __thiscall FUN_004ae3f0(void *this)

{
  if (*(byte *)((int)this + 10) < 0x46) {
    FUN_00490040(s__isForm___0053a1cc,s__ns_o_Squid_cpp_0053a1bc,0x826);
  }
  return (int)*(char *)(*(int *)((uint)*(byte *)((int)this + 10) * 0x1d4 + 0x124 + DAT_0051c960) +
                       (uint)*(byte *)((int)this + 0x22) * 4);
}


// ==== FUN_004ae4b0 @ 004ae4b0

int __thiscall FUN_004ae4b0(void *this)

{
  int iVar1;
  
  iVar1 = FUN_004abf50();
  return *(char *)(*(int *)(iVar1 + 0x124) + (uint)*(byte *)((int)this + 0x22) * 4) + -0x41;
}


// ==== FUN_004af4e0 @ 004af4e0

void __thiscall FUN_004af4e0(void *this)

{
  FUN_004ed640(*(undefined4 *)((int)this + 0x12));
  return;
}


// ==== FUN_0047d020 @ 0047d020

undefined4 __cdecl FUN_0047d020(int param_1)

{
  if (param_1 == DAT_0051cb58) {
    return DAT_0051ca00;
  }
  if ((param_1 == DAT_0051cb68) || (param_1 == DAT_0051cbb4)) {
    return DAT_0051c9f8;
  }
  if ((*(uint *)(param_1 * 0x1d4 + 0xec + DAT_0051c960) & 0x20000) != 0) {
    return DAT_0051ca08;
  }
  if (DAT_0051cb5c == param_1) {
    return DAT_0051c9bc;
  }
  if (DAT_0051cb60 == param_1) {
    return DAT_0051ca04;
  }
  if (DAT_0051ca5c == param_1) {
    return DAT_0051c9c0;
  }
  if (param_1 == DAT_0051cbbc) {
    return DAT_0051ca38;
  }
  if (param_1 == DAT_0051cae8) {
    return DAT_0051c9fc;
  }
  FUN_00490040(&DAT_0052dac8,s__Ns_o_pathprocess_cpp_0052dab0,0x79);
  return 0;
}


// ==== FUN_004eb310 @ 004eb310

uint __cdecl FUN_004eb310(int param_1,uint param_2,int param_3,int param_4)

{
  int *piVar1;
  ushort uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  
  uVar2 = *(ushort *)(param_1 * 0x24 + 6 + DAT_005395dc);
  iVar5 = DAT_005395dc;
  do {
    uVar3 = (uint)uVar2;
    if (uVar3 == 0) {
      return 0;
    }
    piVar1 = (int *)(uVar3 * 0x24 + iVar5);
    if ((*(byte *)(uVar3 * 0x24 + 10 + iVar5) == param_2) &&
       ((*(byte *)((int)piVar1 + 0xb) & 2) == 0)) {
      if (param_3 == 0) {
        if (param_4 == 0) {
          return uVar3;
        }
        if ((DAT_00540a28 != 0) ||
           ((iVar4 = ((int)piVar1 - iVar5) / 0x24, 4 < iVar4 && (iVar4 < 6000)))) {
          (**(code **)(*piVar1 + 0x10))(0);
          iVar5 = DAT_005395dc;
        }
      }
      param_3 = param_3 + -1;
    }
    uVar2 = *(ushort *)(piVar1 + 1);
  } while( true );
}


// ==== FUN_004e50d0 @ 004e50d0

undefined4 __thiscall FUN_004e50d0(void *this)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_004abae0();
  if ((uVar1 & 0x320000) != 0) {
    return 0;
  }
  iVar2 = __ftol();
  iVar3 = __ftol();
  if ((*(byte *)(iVar2 * 0x100 + iVar3 + DAT_0052fe48) & 6) == 0) {
    iVar2 = (**(code **)(*(int *)this + 200))();
    if (iVar2 != 0) {
      return 1;
    }
  }
  return 0;
}


// ==== FUN_00490040 @ 00490040

void __cdecl FUN_00490040(undefined4 param_1,undefined4 param_2)

{
  code *pcVar1;
  FILE *_File;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  undefined4 *unaff_FS_OFFSET;
  undefined1 local_206c [4095];
  char acStackY_106d [2];
  undefined4 local_106b;
  char local_86c [1024];
  char local_46c [1024];
  undefined4 local_6c [10];
  undefined4 uStack_3c;
  undefined4 uStack_38;
  undefined4 *puVar7;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_10 = *unaff_FS_OFFSET;
  puVar7 = (undefined4 *)&stack0xfffffffc;
  local_8 = 0xffffffff;
  pcStack_c = FUN_004903fb;
  *unaff_FS_OFFSET = &local_10;
  FUN_004f1bd0();
  if (DAT_00531554 == 0) {
    DAT_00531554 = 1;
    uStack_38 = 0x49009d;
    FID_conflict__fwprintf
              ((FILE *)&DAT_00546c10,(wchar_t *)s_ZAssertion_failed___s__at__s_lin_00531558);
    _File = FID_conflict___wfopen(s_assert_html_00531584,&DAT_00531580);
    FUN_004f3940();
    FID_conflict__fwprintf(_File,(wchar_t *)s_<html>_<title>Netstorm_Crash<_ti_00531590);
    uStack_38 = 0x4900e8;
    FID_conflict__fwprintf(_File,(wchar_t *)s_ZAssertion_failed___s__at__s_lin_005315d4);
    iVar2 = FUN_004a8950();
    if (iVar2 != 0) {
      local_206c[0] = 0;
      FUN_004a96e0();
      FID_conflict__fwprintf(_File,(wchar_t *)local_206c);
    }
    FID_conflict__fwprintf(_File,(wchar_t *)s_<_body><_html>_00531608);
    _fclose(_File);
    FUN_0048ca50();
    acStackY_106d[1] = 0;
    puVar3 = &local_106b;
    for (iVar2 = 0x1ff; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    *(undefined2 *)puVar3 = 0;
    *(undefined1 *)((int)puVar3 + 2) = 0;
    iVar2 = 0;
    puVar3 = local_6c;
    do {
      *puVar3 = puVar7[1];
      puVar7 = (undefined4 *)*puVar7;
      puVar3 = puVar3 + 1;
      iVar2 = iVar2 + 1;
    } while (puVar3 < &stack0xffffffe4);
    local_8 = 0xffffffff;
    uVar5 = 0;
    if (0 < iVar2) {
      do {
        pcVar6 = acStackY_106d;
        uVar4 = 0xffffffff;
        do {
          pcVar6 = pcVar6 + 1;
          if (uVar4 == 0) break;
          uVar4 = uVar4 - 1;
        } while (*pcVar6 != '\0');
        _sprintf(acStackY_106d + ~uVar4,s__d__08X_00531618);
        uVar4 = (int)uVar5 >> 0x1f;
        if (((uVar5 ^ uVar4) - uVar4 & 3 ^ uVar4) - uVar4 == 3) {
          pcVar6 = acStackY_106d;
          uVar4 = 0xffffffff;
          do {
            pcVar6 = pcVar6 + 1;
            if (uVar4 == 0) break;
            uVar4 = uVar4 - 1;
          } while (*pcVar6 != '\0');
          _sprintf(acStackY_106d + ~uVar4,&DAT_00531624);
        }
        uVar5 = uVar5 + 1;
      } while ((int)uVar5 < iVar2);
    }
    if (DAT_0056fa00 != 0) {
      pcVar6 = acStackY_106d;
      uVar5 = 0xffffffff;
      do {
        pcVar6 = pcVar6 + 1;
        if (uVar5 == 0) break;
        uVar5 = uVar5 - 1;
      } while (*pcVar6 != '\0');
      _sprintf(acStackY_106d + ~uVar5,s_lastRobust__X_sidType__d_00531628);
    }
    pcVar6 = acStackY_106d;
    uVar5 = 0xffffffff;
    do {
      pcVar6 = pcVar6 + 1;
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
    } while (*pcVar6 != '\0');
    _sprintf(acStackY_106d + ~uVar5,s_EBP____08X_00531644);
    local_8 = 2;
    uVar5 = 0;
    do {
      pcVar6 = acStackY_106d;
      uVar4 = 0xffffffff;
      do {
        pcVar6 = pcVar6 + 1;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
      } while (*pcVar6 != '\0');
      _sprintf(acStackY_106d + ~uVar4,s__08X_00531654);
      uVar4 = (int)uVar5 >> 0x1f;
      if (((uVar5 ^ uVar4) - uVar4 & 7 ^ uVar4) - uVar4 == 7) {
        pcVar6 = acStackY_106d;
        uVar4 = 0xffffffff;
        do {
          pcVar6 = pcVar6 + 1;
          if (uVar4 == 0) break;
          uVar4 = uVar4 - 1;
        } while (*pcVar6 != '\0');
        _sprintf(acStackY_106d + ~uVar4,&DAT_0053165c);
      }
      uVar5 = uVar5 + 1;
    } while ((int)uVar5 < 0x28);
    local_8 = 0xffffffff;
    uStack_38 = param_2;
    uStack_3c = param_1;
    local_6c[9] = 0x490325;
    _sprintf(local_86c,s_An_assertion_has_failed____s__in_00531664);
    if (DAT_0052e97c != 0) {
      FUN_004220b0();
    }
    uStack_38 = param_2;
    uStack_3c = param_1;
    local_6c[9] = 0x49036f;
    _sprintf(local_46c,s___s__in_file__s__line__d_version_0053170c);
    uStack_38 = 0x490393;
    DialogBoxParamA(DAT_0052e9cc,(LPCSTR)0x69,DAT_0052e9c8,FUN_0048ff20,(LPARAM)local_46c);
    DAT_00531554 = 0;
  }
  if (DAT_00531450 != 0) {
    if (DAT_0052e16c != 0) {
      FUN_00401230();
    }
    pcVar1 = (code *)swi(3);
    (*pcVar1)();
    return;
  }
  if (DAT_00531458 != 0) {
    FUN_004f1a50();
    DAT_00531458 = 0;
  }
  *unaff_FS_OFFSET = local_10;
  return;
}


