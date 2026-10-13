// ==== FUN_004b7770 @ 004b7770

void __cdecl
FUN_004b7770(int param_1,uint param_2,int param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9,
            undefined4 param_10)

{
  char cVar1;
  char *pcVar2;
  char *pcVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  undefined4 *puVar10;
  char *pcVar11;
  undefined4 *puVar12;
  undefined4 *unaff_FS_OFFSET;
  undefined4 auStack_cc [5];
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  char *pcStack_b0;
  char *pcStack_ac;
  undefined4 local_98 [8];
  char local_78;
  char local_77 [79];
  int local_28;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  char *local_14;
  undefined4 local_10;
  code *pcStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  pcStack_c = FUN_004b7cd7;
  local_10 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = &local_10;
  puVar10 = &param_2;
  puVar12 = local_98;
  for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar12 = *puVar10;
    puVar10 = puVar10 + 1;
    puVar12 = puVar12 + 1;
  }
  local_20 = 0;
  local_1c = param_1;
  do {
    pcStack_ac = (char *)local_1c;
    pcStack_b0 = (char *)0x4b77b9;
    iVar5 = FUN_004f1bb0();
    if (iVar5 == 0) {
      pcStack_ac = (char *)0x4b7c7f;
      iVar5 = FUN_004f1650();
      local_14 = (char *)iVar5;
      local_8 = 3;
      if (iVar5 != 0) {
        pcStack_ac = (char *)0xffffffff;
        puVar10 = &param_2;
        puVar12 = auStack_cc;
        for (iVar6 = 8; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar12 = *puVar10;
          puVar10 = puVar10 + 1;
          puVar12 = puVar12 + 1;
        }
        FUN_004b4de0(iVar5,local_1c);
      }
      local_8 = 0xffffffff;
      pcStack_ac = (char *)0x4b7cc7;
      FUN_004505c0(param_10);
      *unaff_FS_OFFSET = local_10;
      return;
    }
    iVar5 = iVar5 - local_1c;
    local_24 = iVar5 + 1;
    if (0 < iVar5) {
      pcStack_ac = (char *)0x4b77e0;
      pcVar2 = (char *)FUN_004f1650();
      local_14 = pcVar2;
      local_8 = 0;
      if (pcVar2 != (char *)0x0) {
        puVar10 = &param_2;
        puVar12 = auStack_cc;
        pcStack_ac = (char *)iVar5;
        for (iVar6 = 8; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar12 = *puVar10;
          puVar10 = puVar10 + 1;
          puVar12 = puVar12 + 1;
        }
        FUN_004b4de0(pcVar2,local_1c);
      }
      local_8 = 0xffffffff;
      pcStack_ac = (char *)0x4b7827;
      FUN_004505c0(param_10);
    }
    iVar5 = (int)*(char *)(local_24 + local_1c);
    switch(iVar5) {
    case 0x2a:
      param_2 = 0;
      break;
    default:
      local_24 = local_24 + -1;
      break;
    case 0x2e:
      puVar10 = local_98;
      puVar12 = &param_2;
      for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar12 = *puVar10;
        puVar10 = puVar10 + 1;
        puVar12 = puVar12 + 1;
      }
      break;
    case 0x30:
    case 0x31:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
      param_3 = iVar5 + -0x30;
      break;
    case 0x42:
      param_5 = 2;
      break;
    case 0x45:
      param_2 = param_2 | 2;
      param_8 = 1;
      param_9 = 1;
      param_7 = DAT_0055baa8;
      break;
    case 0x49:
      param_5 = 1;
      break;
    case 0x4e:
      param_5 = 0;
      param_2 = param_2 & 0xfffffffd;
      break;
    case 0x5b:
      pcVar2 = (char *)(local_24 + local_1c) + 1;
      pcStack_b0 = (char *)0x4b78cc;
      pcStack_ac = pcVar2;
      iVar5 = FUN_004f1bb0();
      if (iVar5 != 0) {
        local_18 = iVar5 - local_1c;
        iVar5 = local_18 - local_24;
        pcStack_ac = (char *)0x4b78e8;
        local_14 = (char *)FUN_004f1650();
        if (local_14 == (char *)0x0) {
          pcStack_ac = s_StyleText_cpp_0053c9e4;
          pcStack_b0 = s_buffer____NULL_0053c9f4;
          uStack_b4 = 0x4b7906;
          FUN_00490040();
        }
        uVar8 = iVar5 - 1;
        pcVar3 = local_14;
        for (uVar7 = uVar8 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
          *(undefined4 *)pcVar3 = *(undefined4 *)pcVar2;
          pcVar2 = pcVar2 + 4;
          pcVar3 = pcVar3 + 4;
        }
        for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
          *pcVar3 = *pcVar2;
          pcVar2 = pcVar2 + 1;
          pcVar3 = pcVar3 + 1;
        }
        local_24 = local_18;
        local_14[iVar5 + -1] = '\0';
        pcStack_ac = (char *)0x4b7936;
        iVar5 = FUN_00463680(&DAT_00575a90);
        if (iVar5 == 0) {
          pcStack_ac = (char *)0x4b7b0c;
          iVar5 = FUN_00463680(&DAT_00575aa0);
          if (iVar5 != 0) {
            pcStack_ac = (char *)0x50;
            pcStack_b0 = (char *)0x0;
            uStack_b4 = 1;
            uStack_b8 = 0x4b7b22;
            pcVar2 = (char *)FUN_004636b0(&DAT_00575aa0);
            pcStack_ac = (char *)0x4b7b28;
            local_20 = _atoi(pcVar2);
          }
        }
        else {
          pcStack_ac = (char *)0x50;
          pcStack_b0 = (char *)0x0;
          uStack_b4 = 1;
          uStack_b8 = 0x4b7950;
          pcVar2 = (char *)FUN_004636b0(&DAT_00575a90);
          pcStack_ac = (char *)0x4b7956;
          pcVar3 = (char *)_atoi(pcVar2);
          pcStack_ac = (char *)0x50;
          pcStack_b0 = (char *)0x0;
          uStack_b4 = 2;
          uStack_b8 = 0x4b796d;
          pcVar2 = (char *)FUN_004636b0(&DAT_00575a90);
          uVar8 = 0xffffffff;
          do {
            pcVar11 = pcVar2;
            if (uVar8 == 0) break;
            uVar8 = uVar8 - 1;
            pcVar11 = pcVar2 + 1;
            cVar1 = *pcVar2;
            pcVar2 = pcVar11;
          } while (cVar1 != '\0');
          uVar8 = ~uVar8;
          pcVar2 = pcVar11 + -uVar8;
          pcVar11 = &local_78;
          for (uVar7 = uVar8 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
            *(undefined4 *)pcVar11 = *(undefined4 *)pcVar2;
            pcVar2 = pcVar2 + 4;
            pcVar11 = pcVar11 + 4;
          }
          for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
            *pcVar11 = *pcVar2;
            pcVar2 = pcVar2 + 1;
            pcVar11 = pcVar11 + 1;
          }
          if (pcVar3 == (char *)0x0) {
            pcStack_ac = (char *)0x50;
            pcStack_b0 = (char *)0x0;
            uStack_b4 = 1;
            uStack_b8 = 0x4b79ab;
            FUN_004636b0(&DAT_00575a90);
            pcStack_ac = (char *)0x4b79b1;
            pcVar3 = (char *)FUN_00443410();
          }
          iVar5 = 0;
          if (pcVar3 != (char *)0xffffffff) {
            pcVar2 = &local_78;
            local_28 = 0;
            if (local_78 == '*') {
LAB_004b7a2f:
              if ((local_78 == '#') || (local_78 == '&')) {
LAB_004b7a44:
                local_28 = 1;
              }
              else {
                local_28 = 0;
              }
              iVar6 = (int)pcVar3 * 0x1d4 + DAT_0051c960;
              iVar5 = *(int *)(iVar6 + 0x11c);
              if (iVar5 == -1) {
                iVar5 = *(int *)(iVar6 + 0x118);
              }
            }
            else {
              if (local_78 == '#') goto LAB_004b7a44;
              if (local_78 == '&') goto LAB_004b7a2f;
              if (('@' < local_78) &&
                 (local_18 = (int)local_78,
                 local_18 < *(int *)((int)pcVar3 * 0x1d4 + 300 + DAT_0051c960) + 0x41)) {
                pcVar2 = local_77;
                iVar5 = *(int *)(DAT_0051c960 + 0x2c + ((int)pcVar3 * 0x75 + local_18) * 4);
              }
              pcStack_ac = (char *)0x4b7a22;
              iVar6 = _atoi(pcVar2);
              iVar5 = iVar5 + iVar6;
            }
            iVar6 = *(int *)((int)pcVar3 * 0x1d4 + 0x114 + DAT_0051c960);
            iVar9 = iVar5;
            if (iVar6 <= iVar5) {
              iVar9 = iVar6;
            }
            if (iVar9 < 0) {
              iVar5 = 0;
            }
            else if (iVar6 <= iVar5) {
              iVar5 = iVar6;
            }
            pcStack_ac = (char *)0x4b7aa5;
            local_18 = FUN_004f1650();
            local_8 = 1;
            if (local_18 == 0) {
              uVar4 = 0;
            }
            else {
              uStack_b4 = 0x4b7ac4;
              pcStack_b0 = pcVar3;
              pcStack_ac = (char *)iVar5;
              uVar4 = FUN_004506f0(local_18);
            }
            local_8 = 0xffffffff;
            pcStack_ac = (char *)0x4b7ad9;
            FUN_00450800(uVar4);
            if (local_28 != 0) {
              pcStack_ac = (char *)DAT_005119c4;
              pcStack_b0 = (char *)0x4b7af3;
              FUN_00450810(uVar4);
            }
            pcStack_ac = (char *)0x4b7afc;
            FUN_004505c0(param_10);
          }
        }
        pcStack_ac = (char *)0x4b7b37;
        FUN_004f1a50();
      }
      break;
    case 0x62:
      param_4 = DAT_00564bc8;
      break;
    case 0x67:
      param_4 = DAT_00552998;
      break;
    case 0x69:
      param_4 = DAT_0055ba6c;
      break;
    case 0x6a:
      param_4 = DAT_00552890;
      break;
    case 0x6b:
      param_4 = DAT_0055baa8;
      break;
    case 0x6c:
      param_4 = DAT_0055bf54;
      break;
    case 0x6d:
      param_4 = DAT_00552de8;
      break;
    case 0x6e:
      param_4 = local_98[2];
      break;
    case 0x6f:
      param_4 = DAT_0055ba68;
      break;
    case 0x71:
      param_4 = DAT_00552878;
      break;
    case 0x72:
      param_4 = DAT_0055bac8;
      break;
    case 0x74:
      param_4 = DAT_00549b50;
      break;
    case 0x77:
      param_4 = DAT_0055bad0;
      break;
    case 0x78:
      param_4 = DAT_0055bad4;
      break;
    case 0x79:
      param_4 = DAT_0055bf50;
      break;
    case 0x7e:
      pcStack_ac = (char *)0x4b7bf6;
      pcVar2 = (char *)FUN_004f1650();
      local_14 = pcVar2;
      local_8 = 2;
      if (pcVar2 != (char *)0x0) {
        pcStack_ac = (char *)0x1;
        puVar10 = &param_2;
        puVar12 = auStack_cc;
        for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar12 = *puVar10;
          puVar10 = puVar10 + 1;
          puVar12 = puVar12 + 1;
        }
        FUN_004b4de0(pcVar2,&DAT_0053ca04);
      }
      local_8 = 0xffffffff;
      pcStack_ac = (char *)0x4b7c3f;
      FUN_004505c0(param_10);
    }
    local_1c = local_24 + 1 + local_1c;
  } while( true );
}


// ==== FUN_004b4cc0 @ 004b4cc0

void __cdecl FUN_004b4cc0(void)

{
  FUN_00463650(&DAT_00575a90,s_I__________________________0053c3e8);
  return;
}


// ==== FUN_004b4d40 @ 004b4d40

void __cdecl FUN_004b4d40(void)

{
  FUN_00463650(&DAT_00575aa0,s_T____0_9______0053c428);
  return;
}


// ==== FUN_00463650 @ 00463650

void * __thiscall FUN_00463650(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  
  uVar1 = FUN_004637a0(param_2);
  *(undefined4 *)this = uVar1;
  return this;
}


// ==== FUN_00463680 @ 00463680

undefined4 __thiscall FUN_00463680(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  
  *(undefined4 *)((int)this + 8) = param_2;
  *(undefined4 *)((int)this + 4) = 0;
  if (*(int *)this != 0) {
    uVar1 = FUN_00464340(*(int *)this,param_2);
    *(undefined4 *)((int)this + 4) = uVar1;
  }
  return *(undefined4 *)((int)this + 4);
}


// ==== FUN_00464340 @ 00464340

undefined4 __cdecl FUN_00464340(int param_1,char *param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  
  if ((param_1 == 0) || (param_2 == (char *)0x0)) {
    FUN_00463780(s_NULL_parameter_005242fc);
    return 0;
  }
  if (*(char *)(param_1 + 0x5c) != -100) {
    FUN_00463780(s_corrupted_program_0052430c);
    return 0;
  }
  pcVar2 = param_2;
  if (*(int *)(param_1 + 0x54) != 0) {
    while( true ) {
      pcVar2 = (char *)FUN_004f1bb0(pcVar2,(int)**(char **)(param_1 + 0x54));
      if (pcVar2 == (char *)0x0) {
        return 0;
      }
      iVar3 = _strncmp(pcVar2,*(char **)(param_1 + 0x54),*(size_t *)(param_1 + 0x58));
      if (iVar3 == 0) break;
      pcVar2 = pcVar2 + 1;
    }
    if (pcVar2 == (char *)0x0) {
      return 0;
    }
  }
  DAT_00567014 = param_2;
  if (*(char *)(param_1 + 0x51) != '\0') {
    uVar4 = FUN_00464440(param_1,param_2);
    return uVar4;
  }
  if (*(char *)(param_1 + 0x50) == '\0') {
    do {
      iVar3 = FUN_00464440(param_1,param_2);
      if (iVar3 != 0) {
        return 1;
      }
      cVar1 = *param_2;
      param_2 = param_2 + 1;
    } while (cVar1 != '\0');
  }
  else {
    while (iVar3 = FUN_004f1bb0(param_2,(int)*(char *)(param_1 + 0x50)), iVar3 != 0) {
      iVar5 = FUN_00464440(param_1,iVar3);
      if (iVar5 != 0) {
        return 1;
      }
      param_2 = (char *)(iVar3 + 1);
    }
  }
  return 0;
}


// ==== FUN_004636b0 @ 004636b0

undefined4 * __thiscall
FUN_004636b0(void *this,int param_2,undefined4 *param_3,uint param_4,int param_5)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  
  if (param_3 == (undefined4 *)0x0) {
    param_3 = (undefined4 *)&DAT_00567020;
  }
  if ((*(int *)this != 0) && (*(int *)((int)this + 4) != 0)) {
    piVar1 = (int *)(*(int *)this + param_2 * 4);
    iVar2 = *piVar1;
    if ((iVar2 != 0) && (iVar3 = piVar1[10], iVar3 != 0)) {
      uVar5 = (iVar3 - iVar2) - param_5;
      if ((int)param_4 <= (int)uVar5) {
        uVar5 = param_4;
      }
      puVar6 = (undefined4 *)(iVar2 + param_5);
      puVar7 = param_3;
      for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar7 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      *(undefined1 *)(uVar5 + (int)param_3) = 0;
      return param_3;
    }
  }
  *(undefined1 *)param_3 = 0;
  return (undefined4 *)0x0;
}


// ==== FUN_00443410 @ 00443410

int __cdecl FUN_00443410(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 == 0) {
    FUN_00490040(s__name____NULL_0051d72c,s__ns_o_RiftType_cpp_0051d718,0x6d);
  }
  if (DAT_0051c960 == 0) {
    return -1;
  }
  iVar3 = 0;
  if (0 < DAT_0051cbf0) {
    iVar2 = 0;
    do {
      iVar1 = FUN_004f2980(iVar2 + DAT_0051c960 + 4,param_1);
      if (iVar1 == 0) {
        return iVar3;
      }
      iVar2 = iVar2 + 0x1d4;
      iVar3 = iVar3 + 1;
    } while (iVar3 < DAT_0051cbf0);
  }
  return -1;
}


// ==== _atoi @ 004f1ea0

/* Library Function - Single Match
    _atoi
   
   Library: Visual Studio 1998 Release */

int __cdecl _atoi(char *_Str)

{
  long lVar1;
  
  lVar1 = _atol(_Str);
  return lVar1;
}


// ==== FUN_004460f0 @ 004460f0

void __cdecl FUN_004460f0(undefined4 param_1,int *param_2,int *param_3,int *param_4)

{
  char cVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined1 *puVar5;
  undefined4 *puVar6;
  uint uVar7;
  uint uVar8;
  undefined **ppuVar9;
  int *piVar10;
  char *pcVar11;
  int *piVar12;
  undefined4 *unaff_FS_OFFSET;
  char *pcVar13;
  undefined1 local_58 [4];
  int local_54;
  uint local_40;
  int *local_38;
  int local_34;
  int *local_28;
  int local_24;
  int *local_1c;
  char *local_18;
  int *local_14;
  undefined4 local_10;
  code *pcStack_c;
  uint local_8;
  
  local_10 = *unaff_FS_OFFSET;
  local_8 = 0xffffffff;
  pcStack_c = FUN_0044832c;
  *unaff_FS_OFFSET = &local_10;
  pcVar11 = *(char **)(&DAT_0051c6f4 + DAT_0051cc08 * 4);
  uVar7 = 0xffffffff;
  local_18 = (char *)(DAT_0051cc08 * 0x1d4 + 0x7e24 + DAT_0051c960);
  local_18[0x1b4] = '\x01';
  local_18[0x1b5] = '\0';
  local_18[0x1b6] = '\0';
  local_18[0x1b7] = '\0';
  local_18[0x1b8] = '\x01';
  local_18[0x1b9] = '\0';
  local_18[0x1ba] = '\0';
  local_18[0x1bb] = '\0';
  do {
    pcVar13 = pcVar11;
    if (uVar7 == 0) break;
    uVar7 = uVar7 - 1;
    pcVar13 = pcVar11 + 1;
    cVar1 = *pcVar11;
    pcVar11 = pcVar13;
  } while (cVar1 != '\0');
  uVar7 = ~uVar7;
  pcVar13 = pcVar13 + -uVar7;
  pcVar11 = local_18;
  for (uVar8 = uVar7 >> 2; pcVar11 = pcVar11 + 4, uVar8 != 0; uVar8 = uVar8 - 1) {
    *(undefined4 *)pcVar11 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
  }
  for (uVar7 = uVar7 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
    *pcVar11 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar11 = pcVar11 + 1;
  }
  local_18[0x11c] = -1;
  local_18[0x11d] = -1;
  local_18[0x11e] = -1;
  local_18[0x11f] = -1;
  local_18[0xe8] = '\0';
  local_18[0xe9] = '\0';
  local_18[0xea] = '\0';
  local_18[0xeb] = '\0';
  local_18[0xec] = '\0';
  local_18[0xed] = '\0';
  local_18[0xee] = '\0';
  local_18[0xef] = '\0';
  if (param_2 != (int *)0x0) {
    local_14 = param_2;
    if ((*param_2 != 1) && (param_2[1] != 1)) {
      FUN_00490040(s_flag_>btype____BT_IDENTIFIER____f_0051dadc,s__ns_o_RiftType_cpp_0051dac8,0x558)
      ;
    }
    iVar3 = local_14[1];
    piVar10 = local_14;
    while (piVar10 != (int *)0x0) {
      if ((*piVar10 == 0) && (piVar10[1] == iVar3)) {
        piVar12 = (int *)piVar10[3];
        local_14 = (int *)piVar10[2];
      }
      else {
        piVar12 = (int *)0x0;
        local_14 = piVar10;
      }
      if (*local_14 != 1) {
        FUN_00490040(s_flag_>btype____BT_IDENTIFIER_0051db30,s__ns_o_RiftType_cpp_0051db1c,0x55d);
      }
      iVar2 = FUN_004f2980(local_14[2],s_dontdrawdefault_0051db50);
      if (iVar2 == 0) {
        FUN_00490040(s_false_0051db74,s__ns_o_RiftType_cpp_0051db60,0x563);
      }
      iVar2 = FUN_004f2980(local_14[2],s_carribleInVehicle_0051db7c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x10000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_container_0051db90);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_default_hotspot_0051db9c);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_defaulthotspot_0051dbac), iVar2 == 0))
      {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 1;
      }
      iVar2 = FUN_004f2980(local_14[2],s_shadow_0051dbbc);
      if (iVar2 == 0) {
        if ((local_18[0xea] & 0x40U) != 0) {
          FUN_00490040(s___ts_>typeFlags___TYPE_FLAG_FLYE_0051dbd8,s__ns_o_RiftType_cpp_0051dbc4,
                       0x574);
        }
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x40000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_flyershadow_0051dc04);
      if (iVar2 == 0) {
        if ((local_18[0xea] & 4U) != 0) {
          FUN_00490040(s___ts_>typeFlags___TYPE_FLAG_SHAD_0051dc24,s__ns_o_RiftType_cpp_0051dc10,
                       0x579);
        }
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x400000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_not_selectable_0051dc48);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_notselectable_0051dc58), iVar2 == 0))
      {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x8000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_not_real_0051dc68);
      if ((iVar2 == 0) || (iVar2 = FUN_004f2980(local_14[2],s_notreal_0051dc74), iVar2 == 0)) {
        *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x2000000;
        uVar7 = *(uint *)(local_18 + 0xe8);
        *(uint *)(local_18 + 0xe8) = uVar7 | 0x8000000;
        *(uint *)(local_18 + 0xe8) = uVar7 | 0x28000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_createsisland_0051dc7c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x400;
      }
      iVar2 = FUN_004f2980(local_14[2],s_mayDropOnRim_0051dc8c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 4;
      }
      iVar2 = FUN_004f2980(local_14[2],s_mayDropOnIsle_0051dc9c);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 2;
      }
      iVar2 = FUN_004f2980(local_14[2],s_surface_0051dcac);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x800;
      }
      iVar2 = FUN_004f2980(local_14[2],s_randframe_0051dcb4);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x100000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_matchframe_0051dcc0);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x200000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_opaqueCollide_0051dccc);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x800000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_dontSave_0051dcdc);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20000000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveFrame_0051dce8);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x20;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveQA_0051dcf4);
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x1000;
      }
      iVar2 = FUN_004f2980(local_14[2],s_saveQB_0051dcfc);
      piVar10 = piVar12;
      if (iVar2 == 0) {
        *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x2000;
      }
      else {
        iVar2 = FUN_004f2980(local_14[2],s_dropblocking_0051dd04);
        if (iVar2 == 0) {
          *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x10;
        }
        else {
          iVar2 = FUN_004f2980(local_14[2],s_blocking_0051dd14);
          if (iVar2 == 0) {
            uVar7 = *(uint *)(local_18 + 0xec);
            *(uint *)(local_18 + 0xec) = uVar7 | 0x10;
            *(uint *)(local_18 + 0xec) = uVar7 | 0x18;
          }
          else {
            iVar2 = FUN_004f2980(local_14[2],s_shotblocking_0051dd20);
            if (iVar2 == 0) {
              *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20;
            }
            else {
              iVar2 = FUN_004f2980(local_14[2],s_fencePost_0051dd30);
              if (iVar2 == 0) {
                *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 1;
              }
              else {
                iVar2 = FUN_004f2980(local_14[2],s_fence_0051dd3c);
                if (iVar2 == 0) {
                  *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20000000;
                }
                else {
                  iVar2 = FUN_004f2980(local_14[2],s_buried_0051dd44);
                  if (iVar2 == 0) {
                    uVar7 = 0xffffffff;
                    pcVar11 = (char *)local_14[2];
                    do {
                      if (uVar7 == 0) break;
                      uVar7 = uVar7 - 1;
                      cVar1 = *pcVar11;
                      pcVar11 = pcVar11 + 1;
                    } while (cVar1 != '\0');
                    uVar8 = 8;
                    if (~uVar7 < 8) {
                      uVar8 = 0xffffffff;
                      pcVar11 = (char *)local_14[2];
                      do {
                        if (uVar8 == 0) break;
                        uVar8 = uVar8 - 1;
                        cVar1 = *pcVar11;
                        pcVar11 = pcVar11 + 1;
                      } while (cVar1 != '\0');
                      uVar8 = ~uVar8;
                    }
                    FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                    *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x2000;
                  }
                  else {
                    iVar2 = FUN_004f2980(local_14[2],s_nugget_0051dd4c);
                    if (iVar2 == 0) {
                      uVar7 = 0xffffffff;
                      pcVar11 = (char *)local_14[2];
                      do {
                        if (uVar7 == 0) break;
                        uVar7 = uVar7 - 1;
                        cVar1 = *pcVar11;
                        pcVar11 = pcVar11 + 1;
                      } while (cVar1 != '\0');
                      uVar8 = 8;
                      if (~uVar7 < 8) {
                        uVar8 = 0xffffffff;
                        pcVar11 = (char *)local_14[2];
                        do {
                          if (uVar8 == 0) break;
                          uVar8 = uVar8 - 1;
                          cVar1 = *pcVar11;
                          pcVar11 = pcVar11 + 1;
                        } while (cVar1 != '\0');
                        uVar8 = ~uVar8;
                      }
                      FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                      *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x8000;
                    }
                    else {
                      iVar2 = FUN_004f2980(local_14[2],&DAT_0051dd54);
                      if (iVar2 == 0) {
                        uVar7 = 0xffffffff;
                        pcVar11 = (char *)local_14[2];
                        do {
                          if (uVar7 == 0) break;
                          uVar7 = uVar7 - 1;
                          cVar1 = *pcVar11;
                          pcVar11 = pcVar11 + 1;
                        } while (cVar1 != '\0');
                        uVar8 = 8;
                        if (~uVar7 < 8) {
                          uVar8 = 0xffffffff;
                          pcVar11 = (char *)local_14[2];
                          do {
                            if (uVar8 == 0) break;
                            uVar8 = uVar8 - 1;
                            cVar1 = *pcVar11;
                            pcVar11 = pcVar11 + 1;
                          } while (cVar1 != '\0');
                          uVar8 = ~uVar8;
                        }
                        FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                        *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x100;
                      }
                      else {
                        iVar2 = FUN_004f2980(local_14[2],s_emplacement_0051dd5c);
                        if (iVar2 == 0) {
                          *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x40000;
                        }
                        else {
                          iVar2 = FUN_004f2980(local_14[2],s_bridge_0051dd68);
                          if (iVar2 == 0) {
                            uVar7 = 0xffffffff;
                            pcVar11 = (char *)local_14[2];
                            do {
                              if (uVar7 == 0) break;
                              uVar7 = uVar7 - 1;
                              cVar1 = *pcVar11;
                              pcVar11 = pcVar11 + 1;
                            } while (cVar1 != '\0');
                            uVar8 = 8;
                            if (~uVar7 < 8) {
                              uVar8 = 0xffffffff;
                              pcVar11 = (char *)local_14[2];
                              do {
                                if (uVar8 == 0) break;
                                uVar8 = uVar8 - 1;
                                cVar1 = *pcVar11;
                                pcVar11 = pcVar11 + 1;
                              } while (cVar1 != '\0');
                              uVar8 = ~uVar8;
                            }
                            FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                            *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 4;
                          }
                          else {
                            iVar2 = FUN_004f2980(local_14[2],s_walker_0051dd70);
                            if (iVar2 == 0) {
                              uVar7 = 0xffffffff;
                              pcVar11 = (char *)local_14[2];
                              do {
                                if (uVar7 == 0) break;
                                uVar7 = uVar7 - 1;
                                cVar1 = *pcVar11;
                                pcVar11 = pcVar11 + 1;
                              } while (cVar1 != '\0');
                              uVar8 = 8;
                              if (~uVar7 < 8) {
                                uVar8 = 0xffffffff;
                                pcVar11 = (char *)local_14[2];
                                do {
                                  if (uVar8 == 0) break;
                                  uVar8 = uVar8 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                uVar8 = ~uVar8;
                              }
                              FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                              *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x10000;
                            }
                            else {
                              iVar2 = FUN_004f2980(local_14[2],s_balloon_0051dd78);
                              if (iVar2 == 0) {
                                uVar7 = 0xffffffff;
                                pcVar11 = (char *)local_14[2];
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                uVar8 = 8;
                                if (~uVar7 < 8) {
                                  uVar8 = 0xffffffff;
                                  pcVar11 = (char *)local_14[2];
                                  do {
                                    if (uVar8 == 0) break;
                                    uVar8 = uVar8 - 1;
                                    cVar1 = *pcVar11;
                                    pcVar11 = pcVar11 + 1;
                                  } while (cVar1 != '\0');
                                  uVar8 = ~uVar8;
                                }
                                FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 0x20000;
                              }
                              else {
                                iVar2 = FUN_004f2980(local_14[2],s_island_0051dd80);
                                if (iVar2 == 0) {
                                  uVar7 = 0xffffffff;
                                  pcVar11 = (char *)local_14[2];
                                  do {
                                    if (uVar7 == 0) break;
                                    uVar7 = uVar7 - 1;
                                    cVar1 = *pcVar11;
                                    pcVar11 = pcVar11 + 1;
                                  } while (cVar1 != '\0');
                                  uVar8 = 8;
                                  if (~uVar7 < 8) {
                                    uVar8 = 0xffffffff;
                                    pcVar11 = (char *)local_14[2];
                                    do {
                                      if (uVar8 == 0) break;
                                      uVar8 = uVar8 - 1;
                                      cVar1 = *pcVar11;
                                      pcVar11 = pcVar11 + 1;
                                    } while (cVar1 != '\0');
                                    uVar8 = ~uVar8;
                                  }
                                  FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                  *(uint *)(local_18 + 0xec) = *(uint *)(local_18 + 0xec) | 2;
                                }
                                else {
                                  iVar2 = FUN_004f2980(local_14[2],s_islandThreeByThree_0051dd88);
                                  if (iVar2 == 0) {
                                    *(uint *)(local_18 + 0xec) =
                                         *(uint *)(local_18 + 0xec) | 0x1000000;
                                  }
                                  else {
                                    iVar2 = FUN_004f2980(local_14[2],s_factory_0051dd9c);
                                    if (iVar2 == 0) {
                                      uVar7 = 0xffffffff;
                                      pcVar11 = (char *)local_14[2];
                                      do {
                                        if (uVar7 == 0) break;
                                        uVar7 = uVar7 - 1;
                                        cVar1 = *pcVar11;
                                        pcVar11 = pcVar11 + 1;
                                      } while (cVar1 != '\0');
                                      uVar8 = 8;
                                      if (~uVar7 < 8) {
                                        uVar8 = 0xffffffff;
                                        pcVar11 = (char *)local_14[2];
                                        do {
                                          if (uVar8 == 0) break;
                                          uVar8 = uVar8 - 1;
                                          cVar1 = *pcVar11;
                                          pcVar11 = pcVar11 + 1;
                                        } while (cVar1 != '\0');
                                        uVar8 = ~uVar8;
                                      }
                                      FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                      *(uint *)(local_18 + 0xec) =
                                           *(uint *)(local_18 + 0xec) | 0x4000;
                                    }
                                    else {
                                      iVar2 = FUN_004f2980(local_14[2],s_vortex_0051dda4);
                                      if (iVar2 == 0) {
                                        uVar7 = 0xffffffff;
                                        pcVar11 = (char *)local_14[2];
                                        do {
                                          if (uVar7 == 0) break;
                                          uVar7 = uVar7 - 1;
                                          cVar1 = *pcVar11;
                                          pcVar11 = pcVar11 + 1;
                                        } while (cVar1 != '\0');
                                        uVar8 = 8;
                                        if (~uVar7 < 8) {
                                          uVar8 = 0xffffffff;
                                          pcVar11 = (char *)local_14[2];
                                          do {
                                            if (uVar8 == 0) break;
                                            uVar8 = uVar8 - 1;
                                            cVar1 = *pcVar11;
                                            pcVar11 = pcVar11 + 1;
                                          } while (cVar1 != '\0');
                                          uVar8 = ~uVar8;
                                        }
                                        FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                        *(uint *)(local_18 + 0xec) =
                                             *(uint *)(local_18 + 0xec) | 0x200;
                                      }
                                      else {
                                        iVar2 = FUN_004f2980(local_14[2],s_edgefarm_0051ddac);
                                        if (iVar2 == 0) {
                                          uVar7 = 0xffffffff;
                                          pcVar11 = (char *)local_14[2];
                                          do {
                                            if (uVar7 == 0) break;
                                            uVar7 = uVar7 - 1;
                                            cVar1 = *pcVar11;
                                            pcVar11 = pcVar11 + 1;
                                          } while (cVar1 != '\0');
                                          uVar8 = 8;
                                          if (~uVar7 < 8) {
                                            uVar8 = 0xffffffff;
                                            pcVar11 = (char *)local_14[2];
                                            do {
                                              if (uVar8 == 0) break;
                                              uVar8 = uVar8 - 1;
                                              cVar1 = *pcVar11;
                                              pcVar11 = pcVar11 + 1;
                                            } while (cVar1 != '\0');
                                            uVar8 = ~uVar8;
                                          }
                                          FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                          *(uint *)(local_18 + 0xec) =
                                               *(uint *)(local_18 + 0xec) | 0x80000;
                                        }
                                        else {
                                          iVar2 = FUN_004f2980(local_14[2],s_focus_0051ddb8);
                                          if (iVar2 == 0) {
                                            uVar7 = 0xffffffff;
                                            pcVar11 = (char *)local_14[2];
                                            do {
                                              if (uVar7 == 0) break;
                                              uVar7 = uVar7 - 1;
                                              cVar1 = *pcVar11;
                                              pcVar11 = pcVar11 + 1;
                                            } while (cVar1 != '\0');
                                            uVar8 = 8;
                                            if (~uVar7 < 8) {
                                              uVar8 = 0xffffffff;
                                              pcVar11 = (char *)local_14[2];
                                              do {
                                                if (uVar8 == 0) break;
                                                uVar8 = uVar8 - 1;
                                                cVar1 = *pcVar11;
                                                pcVar11 = pcVar11 + 1;
                                              } while (cVar1 != '\0');
                                              uVar8 = ~uVar8;
                                            }
                                            FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                          }
                                          else {
                                            iVar2 = FUN_004f2980(local_14[2],&DAT_0051ddc0);
                                            if (iVar2 == 0) {
                                              uVar7 = 0xffffffff;
                                              pcVar11 = (char *)local_14[2];
                                              do {
                                                if (uVar7 == 0) break;
                                                uVar7 = uVar7 - 1;
                                                cVar1 = *pcVar11;
                                                pcVar11 = pcVar11 + 1;
                                              } while (cVar1 != '\0');
                                              uVar8 = 8;
                                              if (~uVar7 < 8) {
                                                uVar8 = 0xffffffff;
                                                pcVar11 = (char *)local_14[2];
                                                do {
                                                  if (uVar8 == 0) break;
                                                  uVar8 = uVar8 - 1;
                                                  cVar1 = *pcVar11;
                                                  pcVar11 = pcVar11 + 1;
                                                } while (cVar1 != '\0');
                                                uVar8 = ~uVar8;
                                              }
                                              FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                              *(uint *)(local_18 + 0xec) =
                                                   *(uint *)(local_18 + 0xec) | 0x400;
                                            }
                                            else {
                                              iVar2 = FUN_004f2980(local_14[2],s_geyser_0051ddc4);
                                              if (iVar2 == 0) {
                                                uVar7 = 0xffffffff;
                                                pcVar11 = (char *)local_14[2];
                                                do {
                                                  if (uVar7 == 0) break;
                                                  uVar7 = uVar7 - 1;
                                                  cVar1 = *pcVar11;
                                                  pcVar11 = pcVar11 + 1;
                                                } while (cVar1 != '\0');
                                                uVar8 = 8;
                                                if (~uVar7 < 8) {
                                                  uVar8 = 0xffffffff;
                                                  pcVar11 = (char *)local_14[2];
                                                  do {
                                                    if (uVar8 == 0) break;
                                                    uVar8 = uVar8 - 1;
                                                    cVar1 = *pcVar11;
                                                    pcVar11 = pcVar11 + 1;
                                                  } while (cVar1 != '\0');
                                                  uVar8 = ~uVar8;
                                                }
                                                FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                                *(uint *)(local_18 + 0xec) =
                                                     *(uint *)(local_18 + 0xec) | 0x10000000;
                                              }
                                              else {
                                                iVar2 = FUN_004f2980(local_14[2],s_flyer_0051ddcc);
                                                if (iVar2 == 0) {
                                                  *(uint *)(local_18 + 0xec) =
                                                       *(uint *)(local_18 + 0xec) | 0x100000;
                                                }
                                                else {
                                                  iVar2 = FUN_004f2980(local_14[2],
                                                                       s_residence_0051ddd4);
                                                  if (iVar2 == 0) {
                                                    uVar7 = 0xffffffff;
                                                    pcVar11 = (char *)local_14[2];
                                                    do {
                                                      if (uVar7 == 0) break;
                                                      uVar7 = uVar7 - 1;
                                                      cVar1 = *pcVar11;
                                                      pcVar11 = pcVar11 + 1;
                                                    } while (cVar1 != '\0');
                                                    uVar8 = 8;
                                                    if (~uVar7 < 8) {
                                                      uVar8 = 0xffffffff;
                                                      pcVar11 = (char *)local_14[2];
                                                      do {
                                                        if (uVar8 == 0) break;
                                                        uVar8 = uVar8 - 1;
                                                        cVar1 = *pcVar11;
                                                        pcVar11 = pcVar11 + 1;
                                                      } while (cVar1 != '\0');
                                                      uVar8 = ~uVar8;
                                                    }
                                                    FUN_004f2440(local_18 + 0xf0,local_14[2],uVar8);
                                                    *(uint *)(local_18 + 0xec) =
                                                         *(uint *)(local_18 + 0xec) | 0x40000000;
                                                  }
                                                  else {
                                                    iVar2 = FUN_004f2980(local_14[2],
                                                                         s_fringe_0051dde0);
                                                    if (iVar2 == 0) {
                                                      *(uint *)(local_18 + 0xec) =
                                                           *(uint *)(local_18 + 0xec) | 0x1000;
                                                    }
                                                    else {
                                                      iVar2 = FUN_004f2980(local_14[2],
                                                                           s_priest_0051dde8);
                                                      if (iVar2 == 0) {
                                                        *(uint *)(local_18 + 0xec) =
                                                             *(uint *)(local_18 + 0xec) | 0x200000;
                                                      }
                                                      else {
                                                        iVar2 = FUN_004f2980(local_14[2],
                                                                             &DAT_0051ddf0);
                                                        if (iVar2 == 0) {
                                                          *(uint *)(local_18 + 0xec) =
                                                               *(uint *)(local_18 + 0xec) | 0x400000
                                                          ;
                                                        }
                                                        else {
                                                          iVar2 = FUN_004f2980(local_14[2],
                                                                               s_altar_0051ddf8);
                                                          if (iVar2 == 0) {
                                                            *(uint *)(local_18 + 0xec) =
                                                                 *(uint *)(local_18 + 0xec) |
                                                                 0x80000000;
                                                          }
                                                          else {
                                                            iVar2 = FUN_004f2980(local_14[2],
                                                                                                                                                                  
                                                  s_predictable_0051de00);
                                                  if (iVar2 == 0) {
                                                    *(uint *)(local_18 + 0x1c8) =
                                                         *(uint *)(local_18 + 0x1c8) | 99;
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  if (param_3 == (int *)0x0) {
LAB_00448264:
    local_1c = param_4;
    if ((param_4[1] != 2) && (*param_4 != 3)) {
      FUN_00490040(s_frameBud_>utype____UT_FRAME_LIST_0051eea0,s__ns_o_RiftType_cpp_0051ee8c,0x7b3);
    }
    iVar2 = -1;
    iVar3 = 0;
    local_28 = local_1c;
    local_24 = local_1c[1];
    for (piVar10 = local_1c;
        ((piVar10 != (int *)0x0 && (*piVar10 == 0)) && (piVar10[1] == local_24));
        piVar10 = (int *)piVar10[3]) {
      iVar3 = iVar3 + 1;
    }
    iVar3 = (uint)(piVar10 != (int *)0x0) + iVar3;
    *(int *)(local_18 + 0x114) = iVar3;
    puVar6 = (undefined4 *)FUN_004f1650(iVar3 * 4);
    local_40 = 0;
    *(undefined4 **)(local_18 + 0x124) = puVar6;
    for (uVar7 = *(uint *)(local_18 + 0x114) & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    while (local_28 != (int *)0x0) {
      iVar2 = iVar2 + 1;
      local_1c = local_28;
      if ((*local_28 == 0) && (local_28[1] == local_24)) {
        local_1c = (int *)local_28[2];
        local_28 = (int *)local_28[3];
      }
      else {
        local_28 = (int *)0x0;
      }
      pcVar11 = (char *)(iVar2 * 4 + *(int *)(local_18 + 0x124));
      if (*local_1c != 3) {
        FUN_00490040(s_frameBud_>btype____BT_FRAME_0051eef4,s__ns_o_RiftType_cpp_0051eee0,0x7bc);
      }
      piVar10 = (int *)local_1c[2];
      if (*piVar10 != 4) {
        FUN_00490040(s_orientVariant_>btype____BT_ORIEN_0051ef24,s__ns_o_RiftType_cpp_0051ef10,0x7c0
                    );
      }
      cVar1 = **(char **)(piVar10[2] + 8);
      *pcVar11 = cVar1;
      if ((cVar1 < 'A') || ('Z' < cVar1)) {
        FUN_00490040(s__A__<__frame_>sideOrientation____0051ef64,s__ns_o_RiftType_cpp_0051ef50,0x7c3
                    );
      }
      iVar3 = piVar10[2];
      pcVar11[1] = 'P';
      cVar1 = *(char *)(*(int *)(iVar3 + 8) + 1);
      if (cVar1 != '\0') {
        pcVar11[1] = cVar1;
      }
      cVar1 = __ftol();
      pcVar11[3] = '\0';
      pcVar11[2] = cVar1;
      piVar10 = (int *)local_1c[3];
      if (piVar10 != (int *)0x0) {
        local_54 = piVar10[1];
        do {
          if ((*piVar10 == 0) && (piVar10[1] == local_54)) {
            piVar12 = (int *)piVar10[3];
            local_14 = (int *)piVar10[2];
          }
          else {
            piVar12 = (int *)0x0;
            local_14 = piVar10;
          }
          if (*local_14 != 1) {
            FUN_00490040(s_frameFlagBud_>btype____BT_IDENTI_0051efb8,s__ns_o_RiftType_cpp_0051efa4,
                         0x7d3);
          }
          iVar3 = FUN_004f2980(local_14[2],s_fringe_0051efe0);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 1;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051efe8);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 2;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051eff0);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 4;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051eff4);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 8;
          }
          iVar3 = FUN_004f2980(local_14[2],s_unlit_0051eff8);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x10;
          }
          iVar3 = FUN_004f2980(local_14[2],s_cracked_0051f000);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x20;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051f008);
          if (iVar3 == 0) {
            pcVar11[3] = pcVar11[3] | 0x40;
          }
          iVar3 = FUN_004f2980(local_14[2],s_baseframe_0051f010);
          if (iVar3 == 0) {
            *(int *)(local_18 + 0x128) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],s_gumpframe_0051f01c);
          if (iVar3 == 0) {
            *(int *)(local_18 + 0x11c) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],s_default_0051f028);
          if ((iVar3 == 0) && (*(int *)(local_18 + 0x118) = iVar2, local_40 == 0)) {
            *(int *)(local_18 + 0x120) = iVar2;
          }
          iVar3 = FUN_004f2980(local_14[2],&DAT_0051f030);
          if (iVar3 == 0) {
            local_40 = 1;
            *(int *)(local_18 + 0x120) = iVar2;
          }
          piVar10 = piVar12;
        } while (piVar12 != (int *)0x0);
      }
    }
    if (DAT_0051cc20 == (int *)0x0) {
      DAT_0051cc20 = DAT_0051cbf4;
    }
    iVar3 = *DAT_0051cc20;
    for (iVar2 = 0; (iVar3 != 0x30312e31 && (iVar2 < 1000)); iVar2 = iVar2 + 1) {
      DAT_0051cc20 = DAT_0051cc20 + 1;
      iVar3 = *DAT_0051cc20;
    }
    if (999 < iVar2) {
      FUN_00490040(s_i_<_1000_0051f04c,s__ns_o_RiftType_cpp_0051f038,0x80a);
    }
    *(int **)(local_18 + 0xdc) = DAT_0051cc20;
    DAT_0051cc20 = DAT_0051cc20 + 1;
    FUN_00445ff0();
    *unaff_FS_OFFSET = local_10;
    return;
  }
  local_14 = param_3;
  if ((*param_3 != 9) && (param_3[1] != 7)) {
    FUN_00490040(s_attributeValue_>btype____BT_ATTR_0051de20,s__ns_o_RiftType_cpp_0051de0c,0x63c);
  }
  local_34 = local_14[1];
  piVar10 = local_14;
joined_r0x00446dcc:
  local_38 = piVar10;
  if (piVar10 == (int *)0x0) goto LAB_00448264;
  if ((*piVar10 == 0) && (piVar10[1] == local_34)) {
    local_38 = (int *)piVar10[3];
    local_14 = (int *)piVar10[2];
  }
  else {
    local_38 = (int *)0x0;
    local_14 = piVar10;
  }
  if (*local_14 != 9) {
    FUN_00490040(s_attributeValue_>btype____BT_ATTR_0051de94,s__ns_o_RiftType_cpp_0051de80,0x641);
  }
  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxHitPoints_0051dec0);
  if (iVar3 == 0) {
    *(uint *)(local_18 + 0xe8) = *(uint *)(local_18 + 0xe8) | 0x10;
    if (*(int *)local_14[3] != 2) {
      FUN_00490040(s_attributeValue_>p2_>btype____BT__0051dee4,s__ns_o_RiftType_cpp_0051ded0,0x647);
    }
    uVar4 = __ftol();
    FUN_00444750(local_18,uVar4);
    piVar10 = local_38;
    goto joined_r0x00446dcc;
  }
  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeConst_0051df0c);
  if (iVar3 == 0) {
    uVar4 = 0x64d;
    pcVar13 = s__ns_o_RiftType_cpp_0051df18;
    pcVar11 = s_false_0051df2c;
  }
  else {
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_helpText_0051df34);
    if (iVar3 == 0) {
      if (*(int *)local_14[3] != 10) {
        FUN_00490040(s_attributeValue_>p2_>btype____BT__0051df54,s__ns_o_RiftType_cpp_0051df40,0x651
                    );
      }
      uVar4 = FUN_004f26f0(*(undefined4 *)(local_14[3] + 8));
      *(undefined4 *)(local_18 + 0x40) = uVar4;
      for (puVar5 = (undefined1 *)FUN_004f2b70(uVar4,&DAT_0051df84); piVar10 = local_38,
          puVar5 != (undefined1 *)0x0; puVar5 = (undefined1 *)FUN_004f2b70(puVar5,&DAT_0051df88)) {
        *puVar5 = 0x20;
        puVar5[1] = 10;
      }
      goto joined_r0x00446dcc;
    }
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeVar_0051df8c);
    if (iVar3 == 0) {
      uVar4 = 0x65e;
      pcVar13 = s__ns_o_RiftType_cpp_0051df98;
      pcVar11 = &DAT_0051dfac;
      goto LAB_00448252;
    }
    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueConst_0051dfb0);
    if (iVar3 == 0) {
      uVar4 = 0x662;
      pcVar13 = s__ns_o_RiftType_cpp_0051dfc4;
      pcVar11 = &DAT_0051dfd8;
    }
    else {
      iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueVar_0051dfdc);
      if (iVar3 == 0) {
        uVar4 = 0x666;
        pcVar13 = s__ns_o_RiftType_cpp_0051dfec;
        pcVar11 = &DAT_0051e000;
      }
      else {
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_minUsage_0051e004);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e024,s__ns_o_RiftType_cpp_0051e010,
                         0x66a);
          }
          FUN_00444820(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxUsage_0051e04c);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e06c,s__ns_o_RiftType_cpp_0051e058,
                         0x66f);
          }
          FUN_00444870(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxRainBattleUsage_0051e094);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e0bc,s__ns_o_RiftType_cpp_0051e0a8,
                         0x674);
          }
          FUN_00444870(local_18,0,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxThunderBattleUsage_0051e0e4);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e110,s__ns_o_RiftType_cpp_0051e0fc,
                         0x679);
          }
          FUN_00444870(local_18,1,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_maxWindBattleUsage_0051e138);
        if (iVar3 == 0) {
          if (*(int *)local_14[3] != 2) {
            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e160,s__ns_o_RiftType_cpp_0051e14c,
                         0x67e);
          }
          FUN_00444870(local_18,2,*(undefined4 *)(local_14[3] + 8));
          piVar10 = local_38;
          goto joined_r0x00446dcc;
        }
        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_zorder_0051e188);
        if ((iVar3 == 0) ||
           (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_cursorzorder_0051e190),
           iVar3 == 0)) {
          piVar10 = (int *)(local_18 + 0x104);
          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_cursorzorder_0051e1a0);
          if (iVar3 == 0) {
            local_18[0x108] = '\x01';
            local_18[0x109] = '\0';
            local_18[0x10a] = '\0';
            local_18[0x10b] = '\0';
            piVar10 = (int *)(local_18 + 0x10c);
          }
          iVar3 = *(int *)local_14[3];
          if (iVar3 == 2) {
            iVar3 = __ftol();
            *piVar10 = iVar3;
            piVar10 = local_38;
            goto joined_r0x00446dcc;
          }
          if (iVar3 == 10) {
            iVar3 = ((int *)local_14[3])[2];
            FUN_00463650(local_58,s___A_Za_z0_9_______t___0051e1b0);
            local_8 = 0;
            iVar2 = FUN_00463680(local_58,*(undefined4 *)(local_14[3] + 8));
            if (iVar2 != 0) {
              iVar3 = FUN_004636b0(local_58,1,0,0x50,0);
            }
            local_40 = 0;
            ppuVar9 = &PTR_s_zoNONE_0051c630;
            do {
              iVar2 = FUN_004f2b70(*ppuVar9,iVar3);
              if (iVar2 != 0) {
                FUN_00463650(&local_28,s____t____A_Za_z0_9_______t________0051e1c8);
                local_8 = CONCAT31(local_8._1_3_,1);
                iVar3 = FUN_00463680(&local_28,*(undefined4 *)(local_14[3] + 8));
                if (iVar3 == 0) {
                  *piVar10 = *(int *)(&DAT_0051c634 + local_40 * 8);
                }
                else {
                  pcVar11 = (char *)FUN_004636b0(&local_28,2,0,0x50,0);
                  if (*pcVar11 == '+') {
                    pcVar11 = (char *)FUN_004636b0(&local_28,3,0,0x50,0);
                    iVar3 = _atoi(pcVar11);
                    *piVar10 = *(int *)(&DAT_0051c634 + local_40 * 8) + iVar3;
                  }
                  else {
                    pcVar11 = (char *)FUN_004636b0(&local_28,2,0,0x50,0);
                    if (*pcVar11 == '-') {
                      iVar3 = *(int *)(&DAT_0051c634 + local_40 * 8);
                      pcVar11 = (char *)FUN_004636b0(&local_28,3,0,0x50,0);
                      iVar2 = _atoi(pcVar11);
                      *piVar10 = iVar3 - iVar2;
                    }
                  }
                }
                local_8 = local_8 & 0xffffff00;
                FUN_00448324();
                break;
              }
              ppuVar9 = ppuVar9 + 2;
              local_40 = local_40 + 1;
            } while (ppuVar9 < &DAT_0051c6e8);
            if (0x16 < local_40) {
              FUN_00490040(s_zo<COUNTOF(zOrderDefs)_0051e20c,s__ns_o_RiftType_cpp_0051e1f8,0x6ab);
            }
            local_8 = 0xffffffff;
            FUN_00448336();
            piVar10 = local_38;
            goto joined_r0x00446dcc;
          }
          uVar4 = 0x6ae;
          pcVar13 = s__ns_o_RiftType_cpp_0051e224;
          pcVar11 = &DAT_0051e238;
        }
        else {
          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_foot_x_0051e23c);
          if ((iVar3 == 0) ||
             (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_footx_0051e244), iVar3 == 0))
          {
            if (*(int *)local_14[3] != 2) {
              FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e260,s__ns_o_RiftType_cpp_0051e24c
                           ,0x6ba);
            }
            iVar2 = __ftol();
            iVar3 = DAT_0052e9bc;
            *(int *)(local_18 + 0x1b4) = iVar2;
            piVar10 = local_38;
            if (iVar2 <= iVar3) goto joined_r0x00446dcc;
            uVar4 = 0x6bc;
            pcVar13 = s__ns_o_RiftType_cpp_0051e288;
            pcVar11 = s_ts_>footX_<__maxFootpadInTilesX_0051e29c;
          }
          else {
            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_foot_y_0051e2bc);
            if ((iVar3 == 0) ||
               (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_footy_0051e2c4), iVar3 == 0)
               ) {
              if (*(int *)local_14[3] != 2) {
                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e2e0,
                             s__ns_o_RiftType_cpp_0051e2cc,0x6c1);
              }
              iVar2 = __ftol();
              iVar3 = DAT_0052e9c0;
              *(int *)(local_18 + 0x1b8) = iVar2;
              piVar10 = local_38;
              if (iVar2 <= iVar3) goto joined_r0x00446dcc;
              uVar4 = 0x6c3;
              pcVar13 = s__ns_o_RiftType_cpp_0051e308;
              pcVar11 = s_ts_>footY_<__maxFootpadInTilesY_0051e31c;
            }
            else {
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_height_0051e33c);
              if (iVar3 == 0) {
                uVar4 = __ftol();
                *(undefined4 *)(local_18 + 0x1c4) = uVar4;
                piVar10 = local_38;
                goto joined_r0x00446dcc;
              }
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hpPerSec_0051e344);
              if (iVar3 == 0) {
                if (*(int *)local_14[3] != 2) {
                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e364,
                               s__ns_o_RiftType_cpp_0051e350,0x6cb);
                }
                *(undefined4 *)(local_18 + 0x48) = *(undefined4 *)(local_14[3] + 8);
                piVar10 = local_38;
                goto joined_r0x00446dcc;
              }
              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeVar_0051e38c);
              if ((iVar3 == 0) ||
                 (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rangeConst_0051e398),
                 iVar3 == 0)) {
                uVar4 = 0x6d1;
                pcVar13 = s__ns_o_RiftType_cpp_0051e3a4;
                pcVar11 = s_false_0051e3b8;
              }
              else {
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_range_0051e3c0);
                if (iVar3 == 0) {
                  *(undefined4 *)(local_18 + 0x44) = *(undefined4 *)(local_14[3] + 8);
                  piVar10 = local_38;
                  goto joined_r0x00446dcc;
                }
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_delayBetweenShots_0051e3c8);
                if (iVar3 == 0) {
                  *(undefined4 *)(local_18 + 0x4c) = *(undefined4 *)(local_14[3] + 8);
                  piVar10 = local_38;
                  goto joined_r0x00446dcc;
                }
                iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueConst_0051e3dc);
                if (iVar3 == 0) {
                  uVar4 = 0x6dd;
                  pcVar13 = s__ns_o_RiftType_cpp_0051e3f0;
                  pcVar11 = s_false_0051e404;
                }
                else {
                  iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_attackValueVar_0051e40c);
                  if (iVar3 == 0) {
                    uVar4 = 0x6e1;
                    pcVar13 = s__ns_o_RiftType_cpp_0051e41c;
                    pcVar11 = s_false_0051e430;
                  }
                  else {
                    iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rateConst_0051e438);
                    if (iVar3 == 0) {
                      uVar4 = 0x6e5;
                      pcVar13 = s__ns_o_RiftType_cpp_0051e444;
                      pcVar11 = s_false_0051e458;
                    }
                    else {
                      iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_rateVar_0051e460);
                      if (iVar3 == 0) {
                        uVar4 = 0x6e9;
                        pcVar13 = s__ns_o_RiftType_cpp_0051e468;
                        pcVar11 = s_false_0051e47c;
                      }
                      else {
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                             s_hotFootRatioX_0051e484);
                        if (iVar3 == 0) {
                          if (*(int *)local_14[3] != 2) {
                            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e4a8,
                                         s__ns_o_RiftType_cpp_0051e494,0x6ed);
                          }
                          uVar4 = __ftol();
                          *(undefined4 *)(local_18 + 0x1bc) = uVar4;
                          piVar10 = local_38;
                          goto joined_r0x00446dcc;
                        }
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                             s_hotFootRatioY_0051e4d0);
                        if (iVar3 == 0) {
                          if (*(int *)local_14[3] != 2) {
                            FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e4f4,
                                         s__ns_o_RiftType_cpp_0051e4e0,0x6f2);
                          }
                          uVar4 = __ftol();
                          *(undefined4 *)(local_18 + 0x1c0) = uVar4;
                          piVar10 = local_38;
                          goto joined_r0x00446dcc;
                        }
                        iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hotFootX_0051e51c);
                        if (iVar3 == 0) {
                          uVar4 = 0x6f7;
                          pcVar13 = s__ns_o_RiftType_cpp_0051e528;
                          pcVar11 = s_false_0051e53c;
                        }
                        else {
                          iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_hotFootY_0051e544)
                          ;
                          if (iVar3 == 0) {
                            uVar4 = 0x6fb;
                            pcVar13 = s__ns_o_RiftType_cpp_0051e550;
                            pcVar11 = s_false_0051e564;
                          }
                          else {
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_priority_0051e56c);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e58c,
                                             s__ns_o_RiftType_cpp_0051e578,0x6ff);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0xd8) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_constructionRate_0051e5b4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e5dc,
                                             s__ns_o_RiftType_cpp_0051e5c8,0x704);
                              }
                              *(undefined4 *)(local_18 + 0x50) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_orientations_0051e604);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e628,
                                             s__ns_o_RiftType_cpp_0051e614,0x709);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0x110) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_researchDepend_0051e650);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e674,
                                             s__ns_o_RiftType_cpp_0051e660,0x70f);
                              }
                              FUN_00444d20(local_18,*(undefined4 *)(local_14[3] + 8));
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_activeSound_0051e6a4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e6c4,
                                             s__ns_o_RiftType_cpp_0051e6b0,0x716);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x6c) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_fireSound_0051e6f4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e714,
                                             s__ns_o_RiftType_cpp_0051e700,0x71e);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x70) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_impactSound_0051e744);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e764,
                                             s__ns_o_RiftType_cpp_0051e750,0x725);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x74) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_moveSound_0051e794);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e7b4,
                                             s__ns_o_RiftType_cpp_0051e7a0,0x72b);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x78) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_pickupSound_0051e7e4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e804,
                                             s__ns_o_RiftType_cpp_0051e7f0,0x732);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x7c) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_dropSound_0051e834);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e854,
                                             s__ns_o_RiftType_cpp_0051e840,0x739);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x80) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_fallSound_0051e884);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e8a4,
                                             s__ns_o_RiftType_cpp_0051e890,0x740);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x84) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_miscSound0_0051e8d4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e8f4,
                                             s__ns_o_RiftType_cpp_0051e8e0,0x747);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x88) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_miscSound1_0051e924);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e944,
                                             s__ns_o_RiftType_cpp_0051e930,0x74e);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x88) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_buildDoneSound_0051e974);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e998,
                                             s__ns_o_RiftType_cpp_0051e984,0x755);
                              }
                              uVar4 = FUN_00437d20(*(undefined4 *)(local_14[3] + 8));
                              *(undefined4 *)(local_18 + 0x90) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_damageEffect_0051e9c8);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051e9ec,
                                             s__ns_o_RiftType_cpp_0051e9d8,0x75c);
                              }
                              uVar4 = __ftol();
                              *(undefined4 *)(local_18 + 0xd4) = uVar4;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_speed_0051ea14);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ea30,
                                             s__ns_o_RiftType_cpp_0051ea1c,0x761);
                              }
                              *(undefined4 *)(local_18 + 0xe0) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                 s_turningSpeed_0051ea58);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ea7c,
                                             s__ns_o_RiftType_cpp_0051ea68,0x766);
                              }
                              *(undefined4 *)(local_18 + 0xe4) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_theme_0051eaa4);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eac0,
                                             s__ns_o_RiftType_cpp_0051eaac,0x76b);
                              }
                              *(undefined4 *)(local_18 + 0x98) =
                                   (&DAT_00530278)[**(char **)(local_14[3] + 8)];
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_level_0051eaf0);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb0c,
                                             s__ns_o_RiftType_cpp_0051eaf8,0x770);
                              }
                              iVar3 = __ftol();
                              *(int *)(local_18 + 0x94) = iVar3 + -1;
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051eb34);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 2) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb50,
                                             s__ns_o_RiftType_cpp_0051eb3c,0x776);
                              }
                              *(undefined4 *)(local_18 + 0xc4) = *(undefined4 *)(local_14[3] + 8);
                              piVar10 = local_38;
                              goto joined_r0x00446dcc;
                            }
                            iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_group_0051eb78);
                            if (iVar3 == 0) {
                              if (*(int *)local_14[3] != 10) {
                                FUN_00490040(s_attributeValue_>p2_>btype____BT__0051eb94,
                                             s__ns_o_RiftType_cpp_0051eb80,0x77b);
                              }
                              iVar3 = FUN_00420500(local_14[3],*(undefined4 *)(local_14[3] + 8));
                              *(int *)(local_18 + 0x9c) = iVar3;
                              piVar10 = local_38;
                              if (iVar3 != 10) goto joined_r0x00446dcc;
                              uVar4 = 0x77d;
                              pcVar13 = s__ns_o_RiftType_cpp_0051ebc4;
                              pcVar11 = s_ts_>group____NO_GROUP_0051ebd8;
                            }
                            else {
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051ebf0);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ec0c,
                                               s__ns_o_RiftType_cpp_0051ebf8,0x781);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                if (9 < ~uVar7 - 1) {
                                  FUN_00490040(s_strlen_attributeValue_>p2_>strin_0051ec50,
                                               s__ns_o_RiftType_cpp_0051ec3c,0x782);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  pcVar13 = pcVar11;
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  pcVar13 = pcVar11 + 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar13;
                                } while (cVar1 != '\0');
                                uVar7 = ~uVar7;
                                pcVar11 = pcVar13 + -uVar7;
                                pcVar13 = local_18 + 0xa0;
                                for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
                                  *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
                                  pcVar11 = pcVar11 + 4;
                                  pcVar13 = pcVar13 + 4;
                                }
                                for (uVar7 = uVar7 & 3; piVar10 = local_38, uVar7 != 0;
                                    uVar7 = uVar7 - 1) {
                                  *pcVar13 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                  pcVar13 = pcVar13 + 1;
                                }
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),s_class_0051ec78
                                                  );
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ec94,
                                               s__ns_o_RiftType_cpp_0051ec80,0x787);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                } while (cVar1 != '\0');
                                if (0x17 < ~uVar7 - 1) {
                                  FUN_00490040(s_strlen_attributeValue_>p2_>strin_0051ecd8,
                                               s__ns_o_RiftType_cpp_0051ecc4,0x788);
                                }
                                uVar7 = 0xffffffff;
                                pcVar11 = *(char **)(local_14[3] + 8);
                                do {
                                  pcVar13 = pcVar11;
                                  if (uVar7 == 0) break;
                                  uVar7 = uVar7 - 1;
                                  pcVar13 = pcVar11 + 1;
                                  cVar1 = *pcVar11;
                                  pcVar11 = pcVar13;
                                } while (cVar1 != '\0');
                                uVar7 = ~uVar7;
                                pcVar11 = pcVar13 + -uVar7;
                                pcVar13 = local_18 + 0xaa;
                                for (uVar8 = uVar7 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
                                  *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
                                  pcVar11 = pcVar11 + 4;
                                  pcVar13 = pcVar13 + 4;
                                }
                                for (uVar7 = uVar7 & 3; piVar10 = local_38, uVar7 != 0;
                                    uVar7 = uVar7 - 1) {
                                  *pcVar13 = *pcVar11;
                                  pcVar11 = pcVar11 + 1;
                                  pcVar13 = pcVar13 + 1;
                                }
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_artifactChanceClass_0051ed14);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ed3c,
                                               s__ns_o_RiftType_cpp_0051ed28,0x78d);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0xcc) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_artifactFrame_0051ed64);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ed88,
                                               s__ns_o_RiftType_cpp_0051ed74,0x792);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0xd0) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_techBit_0051edb0);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 2) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051edcc,
                                               s__ns_o_RiftType_cpp_0051edb8,0x797);
                                }
                                uVar4 = __ftol();
                                *(undefined4 *)(local_18 + 0x1cc) = uVar4;
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                   s_description_0051edf4);
                              if (iVar3 == 0) {
                                if (*(int *)local_14[3] != 10) {
                                  FUN_00490040(s_attributeValue_>p2_>btype____BT__0051ee14,
                                               s__ns_o_RiftType_cpp_0051ee00,0x79c);
                                }
                                FUN_004f2440(local_18 + 0x18,*(undefined4 *)(local_14[3] + 8),0x28);
                                piVar10 = local_38;
                                goto joined_r0x00446dcc;
                              }
                              iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),&DAT_0051ee44);
                              piVar10 = local_38;
                              if ((((iVar3 == 0) ||
                                   (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                         s_basecrew_0051ee4c), piVar10 = local_38,
                                   iVar3 == 0)) ||
                                  (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                        s_joyfactor_0051ee58), piVar10 = local_38,
                                  iVar3 == 0)) ||
                                 (iVar3 = FUN_004f2980(*(undefined4 *)(local_14[2] + 8),
                                                       s_foodproduced_0051ee64), piVar10 = local_38,
                                 iVar3 == 0)) goto joined_r0x00446dcc;
                              uVar4 = 0x7aa;
                              pcVar13 = s__ns_o_RiftType_cpp_0051ee74;
                              pcVar11 = &DAT_0051ee88;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
LAB_00448252:
  FUN_00490040(pcVar11,pcVar13,uVar4);
  piVar10 = local_38;
  goto joined_r0x00446dcc;
}


