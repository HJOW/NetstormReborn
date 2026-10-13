// ==== FUN_004c10a0 @ 004c10a0

/* WARNING: Removing unreachable block (ram,0x004c11c0) */
/* WARNING: Type propagation algorithm not settling */

void __thiscall FUN_004c10a0(void *this)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  int *piVar10;
  int local_5bc;
  int local_5b8;
  int local_5b4;
  int local_5ac;
  int local_5a8;
  int *local_5a4;
  int local_5a0;
  int local_59c [2];
  int local_594 [99];
  int local_408 [257];
  
  iVar9 = *(int *)((int)this + 0x20834);
  piVar10 = local_408;
  local_408[0] = 0;
  for (iVar7 = 0xff; piVar10 = piVar10 + 1, iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar10 = 0;
  }
  local_59c[1] = 0;
  piVar10 = local_59c + 2;
  for (iVar7 = 99; iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar10 = 0;
    piVar10 = piVar10 + 1;
  }
  if (iVar9 < *(int *)((int)this + 0x2083c)) {
    local_5bc = (iVar9 + 1) * 0x10;
    do {
      iVar7 = *(int *)((int)this + 0x20830);
      if (iVar7 < *(int *)((int)this + 0x20838)) {
        local_5a4 = local_408 + iVar7 * 0x10 + iVar9;
        iVar6 = iVar7 * 0x10;
        do {
          local_5b8 = iVar6 + 0x10;
          iVar2 = FUN_00432300(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9);
          if ((iVar2 != 0) &&
             ((*(int *)((int)this + 0x20818) == DAT_0050c038 ||
              (cVar1 = FUN_004322e0(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9),
              (int)cVar1 == *(int *)((int)this + 0x20818))))) {
            pcVar3 = (char *)FUN_00432180(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9);
            local_59c[0] = local_59c[(int)*pcVar3 + 1] * 0x10e3 + (uint)(byte)pcVar3[5];
            local_59c[(int)*pcVar3 + 1] = local_59c[(int)*pcVar3 + 1] + 1;
            iVar2 = ((byte)pcVar3[5] & 0x80000007) + iVar6 + 4;
            uVar8 = 9999 - (byte)pcVar3[5] & 0x80000007;
            if ((int)uVar8 < 0) {
              uVar8 = (uVar8 - 1 | 0xfffffff8) + 1;
            }
            local_5a8 = uVar8 + local_5bc + -0xc;
            local_5a0 = local_5a8 + 2;
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,0);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,4);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,2);
            FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,6);
            local_5b4 = FUN_004c0930(this,iVar2 + -1,local_5a8 + -1,iVar2 + 3,local_5a8 + 3,0x78,
                                     *pcVar3);
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,0);
            if (iVar4 != 0) {
              local_5a8 = local_5bc + -0x10;
            }
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,4);
            if (iVar4 != 0) {
              local_5a0 = local_5bc;
            }
            iVar4 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,6);
            local_5ac = iVar2;
            if (iVar4 != 0) {
              local_5ac = iVar6;
            }
            iVar5 = FUN_00432330(*(undefined4 *)((int)this + 0x20814),iVar7,iVar9,2);
            iVar4 = local_5b8;
            if (iVar5 == 0) {
              iVar4 = iVar2 + 2;
            }
            iVar2 = FUN_004c0930(this,local_5ac,local_5a8,iVar4,local_5a0,0x78,*pcVar3);
            local_5b4 = local_5b4 + iVar2;
            if (local_59c[(int)*pcVar3 + 1] == 2) {
              iVar6 = FUN_004c0930(this,iVar6 + 3,local_5bc + -0xd,iVar6 + 0xd,local_5bc + -3,0x78,
                                   (int)*pcVar3);
              local_5b4 = local_5b4 + iVar6;
            }
            iVar6 = FUN_004bca20(local_59c,0x1e);
            iVar6 = (iVar6 + 0x32) -
                    ((int)(local_5b4 * 100 + (local_5b4 * 100 >> 0x1f & 0xffU)) >> 8);
            if (iVar6 < 0x14) {
              iVar6 = 0x14;
            }
            *local_5a4 = (iVar6 << 8) / 100;
          }
          iVar7 = iVar7 + 1;
          local_5a4 = local_5a4 + 0x10;
          iVar6 = local_5b8;
        } while (iVar7 < *(int *)((int)this + 0x20838));
      }
      iVar9 = iVar9 + 1;
      local_5bc = local_5bc + 0x10;
    } while (iVar9 < *(int *)((int)this + 0x2083c));
  }
  FUN_004c0ec0(this,local_408,local_59c + 1);
  FUN_004c0cf0(this,2);
  return;
}


// ==== FUN_004c0ec0 @ 004c0ec0

void __thiscall FUN_004c0ec0(void *this,int param_2,int param_3)

{
  int *piVar1;
  undefined4 *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *local_14;
  int local_10 [3];
  undefined4 local_4;
  
  iVar4 = *(int *)((int)this + 0x20834);
  if (iVar4 < *(int *)((int)this + 0x2083c)) {
    do {
      iVar6 = *(int *)((int)this + 0x20830);
      if (iVar6 < *(int *)((int)this + 0x20838)) {
        local_14 = (undefined4 *)(param_2 + (iVar6 * 0x10 + iVar4) * 4);
        do {
          puVar2 = (undefined4 *)FUN_00432180(*(undefined4 *)((int)this + 0x20814),iVar6,iVar4);
          local_4 = puVar2[2];
          iVar5 = (int)(char)*puVar2;
          if (*(int *)((int)this + 0x2081c) == 0) {
            if (iVar5 != DAT_0050c038) goto LAB_004c0f45;
          }
          else if ((iVar5 == DAT_0050c038) || (iVar5 == 0)) {
            iVar5 = 0;
LAB_004c0f45:
            if (param_3 == 0) {
              iVar3 = 0;
            }
            else {
              iVar3 = *(int *)(param_3 + iVar5 * 4);
            }
            local_10[0] = iVar3 * 0x10e3 + ((uint)puVar2[1] >> 8 & 0xff);
            if (param_3 != 0) {
              piVar1 = (int *)(param_3 + iVar5 * 4);
              *piVar1 = *piVar1 + 1;
            }
            if (param_2 == 0) {
              FUN_004e0620(&DAT_00510e00,"\\Ns\\O\\Terrainbuilder.cpp",0x27f);
            }
            FUN_004c0800(this,iVar5,iVar6,iVar4,iVar6 + 1,iVar4 + 1,*local_14,local_10);
          }
          iVar6 = iVar6 + 1;
          local_14 = local_14 + 0x10;
        } while (iVar6 < *(int *)((int)this + 0x20838));
      }
      iVar4 = iVar4 + 1;
    } while (iVar4 < *(int *)((int)this + 0x2083c));
  }
  return;
}


// ==== FUN_004c0800 @ 004c0800

void __thiscall
FUN_004c0800(void *this,undefined4 param_2,int param_3,int param_4,int param_5,undefined4 param_6,
            int param_7,undefined4 param_8)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  
  iVar9 = param_3 * 0x10;
  iVar8 = (param_5 - param_3) * 0x10 + -1;
  param_3 = 1000;
  iVar2 = param_7;
  while ((0 < iVar2 && (iVar7 = param_3 + -1, param_3 != 0))) {
    iVar3 = FUN_004bca20(param_8,iVar8);
    iVar3 = iVar3 + iVar9;
    iVar4 = FUN_004bca20(param_8,iVar8);
    iVar4 = iVar4 + param_4 * 0x10;
    FUN_004bca20(param_8,1);
    iVar6 = iVar3 * 0x100 + iVar4;
    param_3 = iVar7;
    if ((*(char *)((int)this + iVar6 + 8) == 'x') &&
       ((*(int *)((int)this + 0x2081c) != 0 ||
        (*(char *)((int)this + iVar6 + 0x10008) == (char)param_2)))) {
      iVar7 = iVar4 + -1;
      if (iVar7 <= iVar4 + 1) {
        iVar6 = iVar3 + -1;
        param_7 = iVar2;
        do {
          for (; iVar6 <= iVar3 + 1; iVar6 = iVar6 + 1) {
            iVar5 = FUN_004c0780(this,iVar6,iVar7,0x78,param_2);
            if (iVar5 != 0) {
              param_7 = param_7 + -1;
            }
          }
          iVar7 = iVar7 + 1;
          iVar6 = iVar3 + -1;
        } while (iVar7 <= iVar4 + 1);
        bVar1 = param_7 < iVar2;
        iVar2 = param_7;
        if (bVar1) {
          param_3 = 1000;
        }
      }
    }
  }
  return;
}


// ==== FUN_004c0780 @ 004c0780

undefined4 __thiscall FUN_004c0780(void *this,int param_2,int param_3,char param_4,char param_5)

{
  int iVar1;
  int iVar2;
  
  if ((((*(int *)((int)this + 0x20820) <= param_2) && (*(int *)((int)this + 0x20824) <= param_3)) &&
      (param_2 <= *(int *)((int)this + 0x20828))) &&
     ((param_3 <= *(int *)((int)this + 0x2082c) &&
      (iVar2 = param_2 * 0x100 + param_3, *(char *)((int)this + iVar2 + 8) != param_4)))) {
    if ((*(int *)((int)this + 0x2081c) == 0) &&
       (iVar1 = FUN_004c0020(this,param_2,param_3), iVar1 != param_5)) {
      return 0;
    }
    *(char *)((int)this + iVar2 + 0x10008) = param_5;
    *(char *)((int)this + iVar2 + 8) = param_4;
    return 1;
  }
  return 0;
}


// ==== FUN_004c0930 @ 004c0930

int __thiscall
FUN_004c0930(void *this,int param_2,int param_3,int param_4,int param_5,char param_6,
            undefined1 param_7)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  if (param_2 < param_4) {
    this = (void *)(param_2 * 0x100 + (int)this);
    param_4 = param_4 - param_2;
    iVar1 = param_3;
    do {
      for (; iVar1 < param_5; iVar1 = iVar1 + 1) {
        if (*(char *)((int)this + iVar1 + 8) != param_6) {
          *(char *)((int)this + iVar1 + 8) = param_6;
          *(undefined1 *)((int)this + iVar1 + 0x10008) = param_7;
          iVar2 = iVar2 + 1;
        }
      }
      this = (void *)((int)this + 0x100);
      param_4 = param_4 + -1;
      iVar1 = param_3;
    } while (param_4 != 0);
  }
  return iVar2;
}


// ==== FUN_004c0cf0 @ 004c0cf0

void __thiscall FUN_004c0cf0(void *this,int param_2)

{
  int iVar1;
  char *pcVar2;
  int iVar3;
  
  if (0 < param_2) {
    do {
      iVar1 = *(int *)((int)this + 0x20824) + 1;
      if (iVar1 < *(int *)((int)this + 0x2082c)) {
        do {
          iVar3 = *(int *)((int)this + 0x20820) + 1;
          if (iVar3 < *(int *)((int)this + 0x20828) + -1) {
            pcVar2 = (char *)(iVar3 * 0x100 + iVar1 + -0xf8 + (int)this);
            do {
              if ((pcVar2[0x100] == '.') &&
                 ((((*pcVar2 == 'x' || (pcVar2[0x200] == 'x')) || (pcVar2[0xff] == 'x')) ||
                  (pcVar2[0x101] == 'x')))) {
                FUN_004c0c30(this,iVar3,iVar1);
              }
              iVar3 = iVar3 + 1;
              pcVar2 = pcVar2 + 0x100;
            } while (iVar3 < *(int *)((int)this + 0x20828) + -1);
          }
          iVar1 = iVar1 + 1;
        } while (iVar1 < *(int *)((int)this + 0x2082c));
      }
      param_2 = param_2 + -1;
    } while (param_2 != 0);
  }
  return;
}


// ==== FUN_004c0c30 @ 004c0c30

void __thiscall FUN_004c0c30(void *this,int param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  int local_8;
  int local_4;
  
  uVar2 = 0;
  local_8 = 0;
  local_4 = DAT_0050c038;
  do {
    iVar1 = ((&DAT_0052f83c)[uVar2 & 7] + param_2) * 0x100 + (&DAT_0052f85c)[uVar2 & 7] + param_3;
    if (*(char *)(iVar1 + 8 + (int)this) == 'x') {
      if (local_8 == 0) {
        local_4 = (int)*(char *)((int)this + iVar1 + 0x10008);
      }
      if (local_4 != *(char *)((int)this + iVar1 + 0x10008)) goto LAB_004c0cc9;
      local_8 = local_8 + 1;
      if ((4 < local_8) && ((uVar2 & 1) != 0)) {
        param_3 = param_2 * 0x100 + param_3;
        *(char *)((int)this + param_3 + 0x10008) = (char)local_4;
        *(undefined1 *)((int)this + param_3 + 8) = 0x78;
        return;
      }
    }
    else {
LAB_004c0cc9:
      local_8 = 0;
    }
    uVar2 = uVar2 + 1;
    if (0xc < (int)uVar2) {
      return;
    }
  } while( true );
}


// ==== FUN_004c0020 @ 004c0020

int __thiscall FUN_004c0020(void *this)

{
  char cVar1;
  int iVar2;
  int iVar3;
  
  iVar2 = FUN_004e49c0();
  iVar3 = FUN_004e49c0();
  cVar1 = FUN_004322e0(*(undefined4 *)((int)this + 0x20814),
                       (int)(iVar2 + (iVar2 >> 0x1f & 0xfU)) >> 4,
                       (int)(iVar3 + (iVar3 >> 0x1f & 0xfU)) >> 4);
  return (int)cVar1;
}


// ==== FUN_00432180 @ 00432180

void * __thiscall FUN_00432180(void *this,uint param_2,uint param_3)

{
  uint uVar1;
  
  if (*(int *)this == 0) {
    FUN_004e0620("isValid()","\\Ns\\O\\Chunkmap.cpp",0xb7);
  }
  if (((((int)param_2 < 0) || ((int)param_3 < 0)) || (*(int *)((int)this + 4) <= (int)param_2)) ||
     (*(int *)((int)this + 8) <= (int)param_3)) {
    FUN_004e0620("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen","\\Ns\\O\\Chunkmap.cpp",0xb8);
  }
  param_2 = ((int)param_2 < 1) - 1 & param_2;
  uVar1 = *(int *)((int)this + 4) - 1;
  if ((int)uVar1 <= (int)param_2) {
    param_2 = uVar1;
  }
  param_3 = ((int)param_3 < 1) - 1 & param_3;
  uVar1 = *(int *)((int)this + 8) - 1;
  if ((int)uVar1 <= (int)param_3) {
    param_3 = uVar1;
  }
  return (void *)((int)this + (param_2 * 0x10 + 1 + param_3) * 0xc);
}


// ==== FUN_00432300 @ 00432300

bool __thiscall FUN_00432300(void *this,undefined4 param_2,undefined4 param_3)

{
  char *pcVar1;
  
  pcVar1 = (char *)FUN_00432180(this,param_2,param_3);
  return *pcVar1 != DAT_0050c038;
}


// ==== FUN_004322e0 @ 004322e0

undefined1 __thiscall FUN_004322e0(void *this,undefined4 param_2,undefined4 param_3)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)FUN_00432180(this,param_2,param_3);
  return *puVar1;
}


// ==== FUN_00432330 @ 00432330

uint __thiscall FUN_00432330(void *this,undefined4 param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  
  iVar1 = FUN_00432180(this,param_2,param_3);
  return *(uint *)(&DAT_0052f80c + *(char *)(iVar1 + 1) * 4) & (&DAT_0052f8dc)[param_4];
}


// ==== FUN_0046ee30 @ 0046ee30

undefined4 __thiscall FUN_0046ee30(void *this,int param_2)

{
  if ((-1 < param_2) && (param_2 < *(int *)this)) {
    return *(undefined4 *)(param_2 * 0x20 + 8 + (int)this);
  }
  return 0;
}


// ==== FUN_004bca20 @ 004bca20

undefined4 __thiscall FUN_004bca20(void *this,int param_2)

{
  int iVar1;
  
  if (param_2 < 1) {
    return 0;
  }
  if (*(int *)this == 0) {
    *(undefined4 *)this = 0xbad0bad;
  }
  iVar1 = *(int *)this * 0x10003 + 3;
  *(int *)this = iVar1;
  return (int)((longlong)(ulonglong)(iVar1 >> 0x10 & 0xffff) % (longlong)param_2);
}


