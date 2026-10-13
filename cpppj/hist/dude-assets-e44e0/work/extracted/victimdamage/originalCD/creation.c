// ==== FUN_004aeda0 @ 004aeda0

undefined4 __thiscall FUN_004aeda0(void *this,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int unaff_retaddr;
  
  if ((*(byte *)((int)this + 0xb) & 3) != 0) {
    return 1;
  }
  if (DAT_00537e9c != 0) {
    param_2 = 0;
  }
  if ((*(byte *)((int)this + 0x23) & 9) != 0) {
    return 1;
  }
  iVar1 = (**(code **)(*(int *)this + 0x9c))(param_4);
  if (iVar1 != 0) {
    return 1;
  }
  FUN_00462280(&stack0x00000000,param_2,*(undefined1 *)((int)this + 0x20));
  iVar1 = *(short *)((int)this + 0x1a) - unaff_retaddr;
  if (iVar1 < 1) {
    if (DAT_00540a28 != 0) {
      FUN_004aaa20(((int)this - DAT_005395dc) / 0x24,param_2);
      uVar2 = FUN_004aaa40(param_2);
      uVar4 = uVar2 | 0x100000;
      if ((param_4 != 0) &&
         (uVar3 = FUN_004abae0(param_4 * 0x24 + DAT_005395dc), (uVar3 & 0x100) != 0)) {
        uVar4 = uVar2 | 0x4100000;
      }
      (**(code **)(*(int *)this + 0x10))(uVar4);
      return 0;
    }
    iVar1 = 1;
  }
  FUN_004aed20(this,iVar1);
  FUN_004d89b0(*(undefined1 *)((int)this + 0x20),((int)this - DAT_005395dc) / 0x24,param_4);
  return 1;
}


// ==== FUN_00462280 @ 00462280

void __cdecl FUN_00462280(int *param_1,int param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  if (((DAT_0052e170 == 0) && (param_2 != 0)) && (param_3 != 0)) {
    uVar3 = (uint)*(byte *)(param_2 * 0xac + 0x68 + DAT_0050f834);
    uVar2 = (uint)*(byte *)(param_3 * 0xac + 0x68 + DAT_0050f834);
    if (uVar2 < uVar3) {
      iVar1 = ((uVar3 - uVar2) + 8) * *param_1;
      *param_1 = (int)(iVar1 + (iVar1 >> 0x1f & 7U)) >> 3;
      return;
    }
    if (uVar3 < uVar2) {
      *param_1 = (*param_1 << 3) / (int)((uVar2 - uVar3) + 8);
    }
  }
  return;
}


// ==== FUN_00462480 @ 00462480

int __thiscall FUN_00462480(void *this)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  
  if ((*(byte *)((int)this + 0xb) & 1) == 0) {
    iVar2 = FUN_004abf50();
    if (*(int *)(iVar2 + 0xd4) != 0) {
      sVar1 = *(short *)((int)this + 0x1a);
      uVar3 = FUN_004abf50(this);
      iVar4 = FUN_00444720(uVar3);
      iVar5 = ((int)this - DAT_005395dc) / 0x24;
      iVar2 = FUN_004eb310(iVar5,DAT_0051c998,0,0);
      if ((int)sVar1 < (int)(iVar4 * 2 + (iVar4 * 2 >> 0x1f & 3U)) >> 2) {
        if (iVar2 == 0) {
          if (*(byte *)((int)this + 10) == DAT_0051cb60) {
            FUN_00438c00(*(undefined4 *)((int)this + 0xe),*(undefined4 *)((int)this + 0x12),
                         s_bulfHurt_wav_00521094,0);
          }
          iVar4 = FUN_004a6720(iVar5);
          if (iVar4 != 0) {
            iVar2 = *(int *)(iVar4 + 4);
          }
        }
      }
      else if (iVar2 != 0) {
        (**(code **)(*(int *)(iVar2 * 0x24 + DAT_005395dc) + 0x10))(0);
        return 0;
      }
      return iVar2;
    }
  }
  return 0;
}


// ==== FUN_00462300 @ 00462300

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __thiscall FUN_00462300(void *this,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  double dVar1;
  int iVar2;
  int iVar3;
  double *pdVar4;
  uint uVar5;
  double local_c;
  float local_4;
  
  iVar2 = FUN_004aecf0();
  if (iVar2 == 0) {
    return 0;
  }
  _DAT_00566ff0 = (double)CONCAT44(_DAT_00566ff4,_DAT_00566ff0);
  if (*(byte *)((int)this + 0x20) == DAT_0050f6c8) {
    local_c = *(double *)((int)this + 0xe);
    iVar2 = __ftol();
    iVar3 = __ftol();
    if ((((iVar2 < 0) || (iVar3 < 0)) || (0xff < iVar2)) || (0xff < iVar3)) {
      uVar5 = 0;
    }
    else {
      uVar5 = (uint)*(ushort *)(DAT_005670cc + (iVar3 * 0x100 + iVar2) * 2);
    }
    iVar3 = DAT_005395dc + uVar5 * 0x24;
    iVar2 = FUN_004acad0(iVar3);
    _DAT_00566ff0 = (double)CONCAT44(_DAT_00566ff4,_DAT_00566ff0);
    if (iVar2 != 0) {
      iVar2 = *(int *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0x70 + DAT_0050f834);
      iVar3 = FUN_004acaa0(iVar3);
      _DAT_00566ff0 = (double)CONCAT44(_DAT_00566ff4,_DAT_00566ff0);
      if (iVar2 == iVar3) {
        pdVar4 = (double *)FUN_004011b0(&local_c);
        if ((double)CONCAT44(_DAT_00566ff4,_DAT_00566ff0) <= *pdVar4) {
          FUN_00438b50(s_islandUnderAttack_wav_0052107c,0,0,0,1,0);
        }
        local_4 = _DAT_00520fe4;
        pdVar4 = (double *)FUN_004011b0(&local_c);
        local_c = *pdVar4 + (double)local_4;
        _DAT_00566ff0 = local_c;
      }
    }
  }
  if ((*(byte *)((int)this + 0x23) & 4) != 0) {
    (**(code **)(*(int *)this + 0x88))();
  }
  iVar2 = FUN_004aeda0(this,param_2,param_3,param_4);
  dVar1 = _DAT_00566ff0;
  if (iVar2 != 0) {
    FUN_00462480(this);
    dVar1 = _DAT_00566ff0;
  }
  _DAT_00566ff4 = (undefined4)((ulonglong)dVar1 >> 0x20);
  _DAT_00566ff0 = SUB84(dVar1,0);
  return iVar2;
}


// ==== FUN_0040cb10 @ 0040cb10

undefined4 __thiscall FUN_0040cb10(void *this,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  undefined **local_10 [2];
  int local_8;
  undefined4 local_4;
  
  if ((*(byte *)((int)this + 0xb) & 2) != 0) {
    return 1;
  }
  if (DAT_00537e9c != 0) {
    param_2 = 0;
  }
  iVar1 = FUN_0040c5a0(this);
  if ((iVar1 != 0) || (iVar1 = FUN_0040c800(this), iVar1 != 0)) {
    FUN_0040c570(this,0x25c);
    FUN_0040c570(this,0x25d);
    uVar3 = 0;
    iVar1 = FUN_004abf50(this,0);
    FUN_004acbc0(this,*(undefined4 *)(iVar1 + 0x118),uVar3);
  }
  FUN_00462280(&param_2,param_3,*(undefined1 *)((int)this + 0x20));
  iVar1 = *(short *)((int)this + 0x1a) - param_2;
  if (iVar1 < 2) {
    iVar1 = 1;
  }
  FUN_0040ca40(this,iVar1);
  FUN_004d89b0(*(undefined1 *)((int)this + 0x20),((int)this - DAT_005395dc) / 0x24,param_4);
  FUN_00462480(this);
  if ((DAT_00540a2c != 0) && (*(short *)((int)this + 0x1a) == 1)) {
    local_4 = 0;
    local_10[0] = &PTR_LAB_005003d8;
    FUN_004eb960(local_10,((int)this - DAT_005395dc) / 0x24);
    while (local_8 != 0) {
      piVar2 = (int *)FUN_004eba30(local_10);
      (**(code **)(*piVar2 + 0x1c))(((int)this - DAT_005395dc) / 0x24);
      iVar1 = FUN_004eba30(local_10);
      if ((*(byte *)(iVar1 + 0xb) & 1) == 0) {
        uVar3 = FUN_004eba30(local_10,((int)this - DAT_005395dc) / 0x24,0);
        uVar3 = FUN_004af700(uVar3);
        FUN_004afa50(uVar3);
      }
      FUN_004eb990(local_10);
    }
    return 0;
  }
  return 1;
}


// ==== FUN_0040ca40 @ 0040ca40

void __thiscall FUN_0040ca40(void *this,undefined4 param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = FUN_0040c3b0();
  sVar1 = *(short *)((int)this + 0x1a);
  FUN_004aed20(this,param_2);
  iVar3 = FUN_0040c3b0(this);
  if (iVar3 != iVar2) {
    if (DAT_00540a2c != 0) {
      if ((iVar2 != 0) &&
         (*(int *)((uint)*(byte *)((int)this + 0x20) * 0xac + 0x74 + DAT_0050f834) == 0)) {
        FUN_004aed20(this,(int)sVar1);
        return;
      }
      (**(code **)(*(int *)this + 0x48))(0);
      iVar2 = FUN_0040c3b0(this);
      if (iVar2 != 0) {
        FUN_00440a80((int)this + 0xe);
      }
      (**(code **)(*(int *)this + 0x4c))(0);
      return;
    }
    if (iVar2 != 0) {
      iVar2 = FUN_004aed30(this);
      FUN_004aed20(this,iVar2 / 2 + -1);
      return;
    }
    iVar2 = FUN_004aed30(this);
    FUN_004aed20(this,iVar2 / 2);
  }
  return;
}


