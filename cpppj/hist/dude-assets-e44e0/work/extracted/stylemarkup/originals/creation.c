// ==== FUN_004bb430 @ 004bb430

void __cdecl
FUN_004bb430(char *param_1,uint param_2,int param_3,long param_4,undefined4 param_5,
            undefined4 param_6,long param_7,long param_8,long param_9,undefined4 param_10)

{
  char cVar1;
  bool bVar2;
  char *pcVar3;
  void *pvVar4;
  long lVar5;
  char *pcVar6;
  long lVar7;
  undefined4 uVar8;
  int iVar9;
  int iVar10;
  uint uVar11;
  uint uVar12;
  int iVar13;
  long *plVar14;
  undefined4 *puVar15;
  char *pcVar16;
  long *plVar17;
  char **ppcVar18;
  char *apcStack_c8 [8];
  int local_84;
  long local_7c [8];
  char local_5c [80];
  void *local_c;
  code *pcStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb738;
  local_c = ExceptionList;
  plVar14 = (long *)&param_2;
  plVar17 = local_7c;
  for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
    *plVar17 = *plVar14;
    plVar14 = plVar14 + 1;
    plVar17 = plVar17 + 1;
  }
  apcStack_c8[7] = (char *)0x4bb47c;
  ExceptionList = &local_c;
  pcVar3 = _strchr(param_1,0x7e);
  do {
    if (pcVar3 == (char *)0x0) {
      pvVar4 = operator_new(0x30);
      local_4 = 3;
      if (pvVar4 != (void *)0x0) {
        puVar15 = &param_2;
        ppcVar18 = apcStack_c8;
        for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
          *ppcVar18 = (char *)*puVar15;
          puVar15 = puVar15 + 1;
          ppcVar18 = ppcVar18 + 1;
        }
        FUN_004b93c0(pvVar4,param_1);
      }
      local_4 = 0xffffffff;
      FUN_004756e0(param_10);
      ExceptionList = local_c;
      return;
    }
    iVar9 = (int)pcVar3 - (int)param_1;
    local_84 = iVar9 + 1;
    if (0 < iVar9) {
      pvVar4 = operator_new(0x30);
      local_4 = 0;
      if (pvVar4 != (void *)0x0) {
        puVar15 = &param_2;
        ppcVar18 = apcStack_c8;
        for (iVar10 = 8; iVar10 != 0; iVar10 = iVar10 + -1) {
          *ppcVar18 = (char *)*puVar15;
          puVar15 = puVar15 + 1;
          ppcVar18 = ppcVar18 + 1;
        }
        FUN_004b93c0(pvVar4,param_1);
      }
      local_4 = 0xffffffff;
      FUN_004756e0(param_10);
    }
    switch((int)param_1[local_84]) {
    case 0x2a:
      param_2 = 0;
      break;
    default:
      local_84 = iVar9;
      break;
    case 0x2e:
      plVar14 = local_7c;
      plVar17 = (long *)&param_2;
      for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
        *plVar17 = *plVar14;
        plVar14 = plVar14 + 1;
        plVar17 = plVar17 + 1;
      }
      break;
    case 0x30:
    case 0x31:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
      param_3 = param_1[local_84] + -0x30;
      break;
    case 0x42:
      param_5 = 2;
      break;
    case 0x45:
      param_8 = 1;
      param_9 = 1;
      param_2 = param_2 | 2;
      param_7 = DAT_005a4058;
      break;
    case 0x49:
      param_5 = 1;
      break;
    case 0x4e:
      param_2 = param_2 & 0xfffffffd;
      param_5 = 0;
      break;
    case 0x53:
      param_5 = 3;
      break;
    case 0x55:
      param_5 = 4;
      break;
    case 0x5b:
      apcStack_c8[7] = (char *)0x4bb533;
      pcVar3 = _strchr(param_1 + iVar9 + 2,0x5d);
      if (pcVar3 != (char *)0x0) {
        iVar10 = (int)pcVar3 - (int)param_1;
        uVar12 = iVar10 - local_84;
        pcVar3 = operator_new(uVar12);
        if (pcVar3 == (char *)0x0) {
          apcStack_c8[7] = "buffer != NULL";
          apcStack_c8[6] = (char *)0x4bb573;
          FUN_004e0620();
        }
        pcVar6 = param_1 + iVar9 + 2;
        pcVar16 = pcVar3;
        for (uVar11 = uVar12 - 1 >> 2; uVar11 != 0; uVar11 = uVar11 - 1) {
          *(undefined4 *)pcVar16 = *(undefined4 *)pcVar6;
          pcVar6 = pcVar6 + 4;
          pcVar16 = pcVar16 + 4;
        }
        for (uVar11 = uVar12 - 1 & 3; uVar11 != 0; uVar11 = uVar11 - 1) {
          *pcVar16 = *pcVar6;
          pcVar6 = pcVar6 + 1;
          pcVar16 = pcVar16 + 1;
        }
        pcVar3[uVar12 - 1] = '\0';
        iVar9 = FUN_004964e0(&DAT_005c8968);
        local_84 = iVar10;
        if (iVar9 == 0) {
          iVar9 = FUN_004964e0(&DAT_005c898c);
          if (iVar9 == 0) {
            iVar9 = FUN_004964e0(&DAT_005c8950);
            if (iVar9 == 0) {
              iVar9 = FUN_004964e0(&DAT_005c8998);
              if (iVar9 == 0) {
                iVar9 = FUN_004964e0(&DAT_005c895c);
                if (iVar9 == 0) {
                  iVar9 = FUN_004964e0(&DAT_005c8974);
                  if ((iVar9 != 0) && (DAT_0053187c != 0)) {
                    apcStack_c8[7] = (char *)0x0;
                    apcStack_c8[6] = (char *)0x1;
                    apcStack_c8[5] = (char *)0x4bb8e0;
                    pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                    lVar5 = _atol(pcVar3);
                    if (lVar5 < 5) {
                      apcStack_c8[7] = (char *)0x0;
                      apcStack_c8[6] = (char *)0x1;
                      apcStack_c8[5] = (char *)0x4bb900;
                      pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                      lVar5 = _atol(pcVar3);
                      if (lVar5 < -4) goto LAB_004bb90e;
LAB_004bb956:
                      apcStack_c8[7] = (char *)0x0;
                      apcStack_c8[6] = (char *)0x2;
                      apcStack_c8[5] = (char *)0x4bb968;
                      pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                      lVar5 = _atol(pcVar3);
                      apcStack_c8[7] = (char *)0x0;
                      apcStack_c8[6] = (char *)0x1;
                      apcStack_c8[5] = (char *)0x4bb985;
                      pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                      param_8 = _atol(pcVar3);
                      param_2 = param_2 | 2;
                      param_7 = DAT_005a4058;
                      param_9 = lVar5;
                    }
                    else {
LAB_004bb90e:
                      apcStack_c8[7] = (char *)0x0;
                      apcStack_c8[6] = (char *)0x2;
                      apcStack_c8[5] = (char *)0x4bb920;
                      pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                      lVar5 = _atol(pcVar3);
                      if (lVar5 < 5) {
                        apcStack_c8[7] = (char *)0x0;
                        apcStack_c8[6] = (char *)0x2;
                        apcStack_c8[5] = (char *)0x4bb944;
                        pcVar3 = (char *)FUN_00495a10(&DAT_005c8974);
                        lVar5 = _atol(pcVar3);
                        if (-5 < lVar5) goto LAB_004bb956;
                      }
                    }
                  }
                }
                else if (DAT_00531870 != 0) {
                  apcStack_c8[7] = (char *)0x0;
                  apcStack_c8[6] = (char *)0x1;
                  apcStack_c8[5] = (char *)0x4bb87e;
                  pcVar3 = (char *)FUN_00495a10(&DAT_005c895c);
                  _atol(pcVar3);
                  apcStack_c8[7] = (char *)0x0;
                  apcStack_c8[6] = (char *)0x1;
                  apcStack_c8[5] = (char *)0x4bb899;
                  pcVar3 = (char *)FUN_00495a10(&DAT_005c895c);
                  param_7 = _atol(pcVar3);
                }
              }
              else if (DAT_00531874 != 0) {
                apcStack_c8[7] = (char *)0x0;
                apcStack_c8[6] = (char *)0x1;
                apcStack_c8[5] = (char *)0x4bb820;
                pcVar3 = (char *)FUN_00495a10(&DAT_005c8998);
                _atol(pcVar3);
                apcStack_c8[7] = (char *)0x0;
                apcStack_c8[6] = (char *)0x1;
                apcStack_c8[5] = (char *)0x4bb83b;
                pcVar3 = (char *)FUN_00495a10(&DAT_005c8998);
                param_4 = _atol(pcVar3);
              }
            }
            else {
              apcStack_c8[7] = (char *)0x0;
              apcStack_c8[6] = (char *)0x1;
              apcStack_c8[5] = (char *)0x4bb7e0;
              pcVar3 = (char *)FUN_00495a10(&DAT_005c8950);
              _atol(pcVar3);
            }
          }
          else if (DAT_0053186c != 0) {
            apcStack_c8[7] = (char *)0x0;
            apcStack_c8[6] = (char *)0x0;
            apcStack_c8[5] = (char *)0x0;
            apcStack_c8[4] = (char *)0x0;
            apcStack_c8[3] = (char *)0x50;
            apcStack_c8[2] = (char *)0x0;
            apcStack_c8[1] = (char *)0x1;
            apcStack_c8[0] = (char *)0x4bb7b1;
            apcStack_c8[4] = (char *)FUN_00495a10(&DAT_005c898c);
            apcStack_c8[3] = (char *)0x4bb7b7;
            FUN_004a9cb0();
          }
        }
        else {
          apcStack_c8[7] = (char *)0x0;
          apcStack_c8[6] = (char *)0x1;
          apcStack_c8[5] = (char *)0x4bb5bb;
          pcVar3 = (char *)FUN_00495a10(&DAT_005c8968);
          lVar5 = _atol(pcVar3);
          apcStack_c8[7] = (char *)0x0;
          apcStack_c8[6] = (char *)0x2;
          apcStack_c8[5] = (char *)0x4bb5d8;
          pcVar6 = (char *)FUN_00495a10(&DAT_005c8968);
          pcVar3 = local_5c;
          do {
            cVar1 = *pcVar6;
            pcVar6 = pcVar6 + 1;
            *pcVar3 = cVar1;
            pcVar3 = pcVar3 + 1;
          } while (cVar1 != '\0');
          if (lVar5 == 0) {
            apcStack_c8[6] = (char *)0x1;
            apcStack_c8[5] = (char *)0x4bb5fe;
            apcStack_c8[7] = (char *)lVar5;
            FUN_00495a10(&DAT_005c8968);
            lVar5 = FUN_0049a860();
          }
          iVar9 = 0;
          if (lVar5 != -1) {
            bVar2 = false;
            pcVar3 = local_5c;
            if (local_5c[0] == '*') {
LAB_004bb675:
              if ((local_5c[0] == '#') || (local_5c[0] == '&')) {
LAB_004bb67d:
                bVar2 = true;
              }
              iVar9 = FUN_0049a840();
              if (*(int *)(iVar9 + 0x11c) == -1) {
                iVar9 = FUN_0049a840();
                iVar9 = *(int *)(iVar9 + 0x118);
              }
              else {
                iVar9 = FUN_0049a840();
                iVar9 = *(int *)(iVar9 + 0x11c);
              }
            }
            else {
              if (local_5c[0] == '#') goto LAB_004bb67d;
              if (local_5c[0] == '&') goto LAB_004bb675;
              if ('@' < local_5c[0]) {
                iVar13 = (int)local_5c[0];
                iVar10 = FUN_0049a840();
                if (iVar13 < *(int *)(iVar10 + 300) + 0x41) {
                  iVar9 = FUN_0049a840();
                  iVar9 = *(int *)(iVar9 + 0x2c + iVar13 * 4);
                  pcVar3 = local_5c + 1;
                }
              }
              bVar2 = false;
              lVar7 = _atol(pcVar3);
              iVar9 = iVar9 + lVar7;
            }
            iVar13 = FUN_0049a840();
            iVar10 = iVar9;
            if (*(int *)(iVar13 + 0x114) <= iVar9) {
              iVar10 = FUN_0049a840();
              iVar10 = *(int *)(iVar10 + 0x114);
            }
            if ((-1 < iVar10) && (iVar10 = FUN_0049a840(), *(int *)(iVar10 + 0x114) <= iVar9)) {
              FUN_0049a840();
            }
            pvVar4 = operator_new(0x1c);
            local_4 = 1;
            if (pvVar4 == (void *)0x0) {
              uVar8 = 0;
            }
            else {
              apcStack_c8[6] = (char *)0x4bb736;
              apcStack_c8[7] = (char *)lVar5;
              uVar8 = FUN_00475810(pvVar4);
            }
            local_4 = 0xffffffff;
            FUN_004758e0(uVar8);
            if (bVar2) {
              apcStack_c8[7] = (char *)0x4bb767;
              FUN_004758f0(uVar8);
            }
            FUN_004756e0(param_10);
          }
        }
      }
      break;
    case 0x61:
      param_4 = DAT_005b5dcc;
      break;
    case 0x62:
      param_4 = DAT_005ad128;
      break;
    case 100:
      param_4 = DAT_0059af80;
      break;
    case 0x67:
      param_4 = DAT_005acd20;
      break;
    case 0x69:
      param_4 = DAT_005bec30;
      break;
    case 0x6a:
      param_4 = DAT_005a4064;
      break;
    case 0x6b:
      param_4 = DAT_005a4058;
      break;
    case 0x6c:
      param_4 = DAT_005a4060;
      break;
    case 0x6d:
      param_4 = DAT_005b5da0;
      break;
    case 0x6e:
      param_4 = local_7c[2];
      break;
    case 0x6f:
      param_4 = DAT_005bec34;
      break;
    case 0x70:
      param_4 = DAT_005b5e6c;
      break;
    case 0x71:
      param_4 = DAT_005b5e5c;
      break;
    case 0x72:
      param_4 = DAT_005b5e58;
      break;
    case 0x73:
      param_4 = DAT_005b5e54;
      break;
    case 0x74:
      param_4 = DAT_005b5dc8;
      break;
    case 0x75:
      param_4 = DAT_0059afd4;
      break;
    case 0x77:
      param_4 = DAT_005bec2c;
      break;
    case 0x78:
      param_4 = DAT_005c78d0;
      break;
    case 0x79:
      param_4 = DAT_0059afd8;
      break;
    case 0x7e:
      pvVar4 = operator_new(0x30);
      local_4 = 2;
      if (pvVar4 != (void *)0x0) {
        puVar15 = &param_2;
        ppcVar18 = apcStack_c8;
        for (iVar9 = 8; iVar9 != 0; iVar9 = iVar9 + -1) {
          *ppcVar18 = (char *)*puVar15;
          puVar15 = puVar15 + 1;
          ppcVar18 = ppcVar18 + 1;
        }
        FUN_004b93c0(pvVar4,&DAT_00504f1c);
      }
      local_4 = 0xffffffff;
      FUN_004756e0(param_10);
    }
    param_1 = param_1 + local_84 + 1;
    apcStack_c8[7] = (char *)0x4bbc43;
    pcVar3 = _strchr(param_1,0x7e);
  } while( true );
}


// ==== FUN_004fed30 @ 004fed30

void FUN_004fed30(void)

{
  FUN_00496c90(&DAT_005c8968,"I *([^.\r\n ]+) *. *([^.\r\n ]+) *");
  _atexit(FUN_004ff2b0);
  return;
}


// ==== FUN_004fed50 @ 004fed50

void FUN_004fed50(void)

{
  FUN_00496c90(&DAT_005c898c,"S *. *([^.\r\n ]+) *");
  _atexit(FUN_004ff2c0);
  return;
}


// ==== FUN_004fed70 @ 004fed70

void FUN_004fed70(void)

{
  FUN_00496c90(&DAT_005c8998,"C *. *([^.\r\n ]+) *");
  _atexit(FUN_004ff2d0);
  return;
}


// ==== FUN_004fed90 @ 004fed90

void FUN_004fed90(void)

{
  FUN_00496c90(&DAT_005c895c,"B *. *([^.\r\n ]+) *");
  _atexit(FUN_004ff2e0);
  return;
}


// ==== FUN_004fedb0 @ 004fedb0

void FUN_004fedb0(void)

{
  FUN_00496c90(&DAT_005c8974,"E *. *([^.\r\n ]+) *. *([^.\r\n ]+) *");
  _atexit(FUN_004ff2f0);
  return;
}


// ==== FUN_004fedd0 @ 004fedd0

void FUN_004fedd0(void)

{
  FUN_00496c90(&DAT_005c8950,"T *([0-9]+) *");
  _atexit(FUN_004ff300);
  return;
}


// ==== FUN_00496c90 @ 00496c90

void * __thiscall FUN_00496c90(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00496b20(param_2);
  *(undefined4 *)this = uVar1;
  return this;
}


// ==== FUN_004964e0 @ 004964e0

undefined4 __thiscall FUN_004964e0(void *this,undefined4 param_2)

{
  undefined4 uVar1;
  
  *(undefined4 *)((int)this + 8) = param_2;
  *(undefined4 *)((int)this + 4) = 0;
  if (*(int *)this != 0) {
    uVar1 = FUN_004963d0(*(int *)this,param_2);
    *(undefined4 *)((int)this + 4) = uVar1;
  }
  return *(undefined4 *)((int)this + 4);
}


// ==== FUN_004963d0 @ 004963d0

undefined4 __cdecl FUN_004963d0(int param_1,char *param_2)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  undefined4 uVar4;
  
  if (((param_1 == 0) || (param_2 == (char *)0x0)) || (*(char *)(param_1 + 0x5c) != -100)) {
    FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Regexp.cpp",0x6e);
    return 0;
  }
  if (*(char **)(param_1 + 0x54) != (char *)0x0) {
    pcVar2 = _strchr(param_2,(int)**(char **)(param_1 + 0x54));
    if (pcVar2 == (char *)0x0) {
      return 0;
    }
    while (iVar3 = _strncmp(pcVar2,*(char **)(param_1 + 0x54),*(size_t *)(param_1 + 0x58)),
          iVar3 != 0) {
      pcVar2 = _strchr(pcVar2 + 1,(int)**(char **)(param_1 + 0x54));
      if (pcVar2 == (char *)0x0) {
        return 0;
      }
    }
    if (pcVar2 == (char *)0x0) {
      return 0;
    }
  }
  DAT_0059a824 = param_2;
  if (*(char *)(param_1 + 0x51) != '\0') {
    uVar4 = FUN_00496270();
    return uVar4;
  }
  if (*(char *)(param_1 + 0x50) == '\0') {
    while (iVar3 = FUN_00496270(), iVar3 == 0) {
      cVar1 = *param_2;
      param_2 = param_2 + 1;
      if (cVar1 == '\0') {
        return 0;
      }
    }
  }
  else {
    pcVar2 = _strchr(param_2,(int)*(char *)(param_1 + 0x50));
    if (pcVar2 == (char *)0x0) {
      return 0;
    }
    while (iVar3 = FUN_00496270(), iVar3 == 0) {
      pcVar2 = _strchr(pcVar2 + 1,(int)*(char *)(param_1 + 0x50));
      if (pcVar2 == (char *)0x0) {
        return 0;
      }
    }
  }
  return 1;
}


// ==== FUN_00496270 @ 00496270

undefined4 __cdecl FUN_00496270(void)

{
  int iVar1;
  undefined4 unaff_EBX;
  undefined4 *unaff_ESI;
  
  *unaff_ESI = 0;
  unaff_ESI[10] = 0;
  unaff_ESI[1] = 0;
  unaff_ESI[0xb] = 0;
  unaff_ESI[2] = 0;
  unaff_ESI[0xc] = 0;
  unaff_ESI[3] = 0;
  unaff_ESI[0xd] = 0;
  unaff_ESI[4] = 0;
  unaff_ESI[0xe] = 0;
  unaff_ESI[5] = 0;
  unaff_ESI[0xf] = 0;
  unaff_ESI[6] = 0;
  unaff_ESI[0x10] = 0;
  unaff_ESI[7] = 0;
  unaff_ESI[0x11] = 0;
  unaff_ESI[8] = 0;
  unaff_ESI[0x12] = 0;
  unaff_ESI[9] = 0;
  unaff_ESI[0x13] = 0;
  DAT_0059a828 = unaff_ESI;
  DAT_0059a884 = unaff_ESI + 10;
  DAT_0059a888 = unaff_EBX;
  iVar1 = FUN_00495d60((int)unaff_ESI + 0x5d);
  if (iVar1 != 0) {
    unaff_ESI[10] = DAT_0059a888;
    *unaff_ESI = unaff_EBX;
    return 1;
  }
  return 0;
}


// ==== FUN_00495a10 @ 00495a10

undefined4 * __thiscall
FUN_00495a10(void *this,int param_2,undefined4 *param_3,uint param_4,int param_5)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  
  if (param_3 == (undefined4 *)0x0) {
    param_3 = (undefined4 *)&DAT_0059a830;
  }
  iVar1 = *(int *)this;
  if ((iVar1 != 0) && (*(int *)((int)this + 4) != 0)) {
    iVar2 = *(int *)(iVar1 + param_2 * 4);
    if ((iVar2 != 0) && (iVar1 = *(int *)(iVar1 + 0x28 + param_2 * 4), iVar1 != 0)) {
      uVar3 = (iVar1 - iVar2) - param_5;
      if ((int)param_4 < (int)uVar3) {
        uVar3 = param_4;
      }
      puVar5 = (undefined4 *)(iVar2 + param_5);
      puVar6 = param_3;
      for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      for (uVar4 = uVar3 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar6 = *(undefined1 *)puVar5;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
        puVar6 = (undefined4 *)((int)puVar6 + 1);
      }
      *(undefined1 *)(uVar3 + (int)param_3) = 0;
      return param_3;
    }
  }
  *(undefined1 *)param_3 = 0;
  return (undefined4 *)0x0;
}


// ==== FUN_0049a840 @ 0049a840

int __cdecl FUN_0049a840(int param_1)

{
  return param_1 * 500 + DAT_0059ab20;
}


// ==== FUN_0049a860 @ 0049a860

int __cdecl FUN_0049a860(char *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 == (char *)0x0) {
    FUN_004e0620("_name != NULL","\\Ns\\O\\Rifttype.cpp",0x75);
  }
  if (DAT_0059ab20 == 0) {
    return -1;
  }
  iVar3 = 0;
  if (0 < DAT_00541348) {
    iVar2 = 0;
    do {
      iVar1 = __stricmp((char *)(iVar2 + 4 + DAT_0059ab20),param_1);
      if (iVar1 == 0) {
        return iVar3;
      }
      iVar3 = iVar3 + 1;
      iVar2 = iVar2 + 500;
    } while (iVar3 < DAT_00541348);
  }
  return -1;
}


// ==== _atol @ 004e66de

long __cdecl _atol(char *_Str)

{
  _ptiddata p_Var1;
  pthreadlocinfo ptVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  byte *pbVar7;
  
  p_Var1 = __getptd();
  ptVar2 = (pthreadlocinfo)p_Var1->_tfpecode;
  if (ptVar2 != (pthreadlocinfo)PTR_DAT_00543efc) {
    ptVar2 = ___updatetlocinfo();
  }
  while( true ) {
    if ((int)ptVar2->lc_category[1].refcount < 2) {
      uVar3 = *(byte *)((int)ptVar2->lc_category[3].refcount + (uint)(byte)*_Str * 2) & 8;
    }
    else {
      uVar3 = ___isctype_mt(ptVar2,(uint)(byte)*_Str,8);
    }
    if (uVar3 == 0) break;
    _Str = _Str + 1;
  }
  uVar3 = (uint)(byte)*_Str;
  pbVar7 = (byte *)(_Str + 1);
  if ((uVar3 == 0x2d) || (uVar5 = uVar3, uVar3 == 0x2b)) {
    uVar5 = (uint)*pbVar7;
    pbVar7 = (byte *)(_Str + 2);
  }
  iVar4 = 0;
  while( true ) {
    if ((uVar5 < 0x30) || (0x39 < uVar5)) {
      iVar6 = -1;
    }
    else {
      iVar6 = uVar5 - 0x30;
    }
    if (iVar6 == -1) break;
    iVar4 = iVar6 + iVar4 * 10;
    uVar5 = (uint)*pbVar7;
    pbVar7 = pbVar7 + 1;
  }
  if (uVar3 == 0x2d) {
    iVar4 = -iVar4;
  }
  return iVar4;
}


// ==== FUN_0049c3b0 @ 0049c3b0

void __cdecl FUN_0049c3b0(undefined4 param_1,int *param_2,int *param_3,int *param_4)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  char *pcVar4;
  long lVar5;
  undefined4 *puVar6;
  char *pcVar7;
  int *piVar8;
  int *piVar9;
  int iVar10;
  uint uVar11;
  int *piVar12;
  undefined4 uVar13;
  int *local_40;
  undefined1 local_38 [12];
  undefined1 local_2c [4];
  int *local_28;
  int local_24;
  int local_20;
  int *local_18;
  int local_14;
  int local_10;
  void *local_c;
  code *pcStack_8;
  uint local_4;
  
  iVar3 = DAT_0059ab38;
  local_4 = 0xffffffff;
  pcStack_8 = FUN_004fb3b0;
  local_c = ExceptionList;
  piVar9 = (int *)((DAT_0059ab38 + 0x45) * 500 + DAT_0059ab20);
  ExceptionList = &local_c;
  piVar9[0x75] = 1;
  piVar9[0x76] = 1;
  pcVar7 = *(char **)(&DAT_00540dcc + iVar3 * 4);
  piVar8 = piVar9 + 1;
  do {
    cVar2 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    *(char *)piVar8 = cVar2;
    piVar8 = (int *)((int)piVar8 + 1);
  } while (cVar2 != '\0');
  piVar9[0x47] = -1;
  piVar9[0x3a] = 0;
  piVar9[0x3b] = 0;
  if (param_2 != (int *)0x0) {
    if ((*param_2 != 1) && (param_2[1] != 1)) {
      FUN_004e0620("flag->btype == BT_IDENTIFIER || flag->utype == UT_TYPEFLAG_LIST",
                   "\\Ns\\O\\Rifttype.cpp",0x673);
    }
    local_28 = param_2;
    local_24 = param_2[1];
    local_20 = -1;
    while (piVar8 = local_28, local_28 != (int *)0x0) {
      local_20 = local_20 + 1;
      if ((*local_28 == 0) && (local_28[1] == local_24)) {
        piVar8 = (int *)local_28[2];
        local_28 = (int *)local_28[3];
      }
      else {
        local_28 = (int *)0x0;
      }
      if (*piVar8 != 1) {
        FUN_004e0620("flag->btype == BT_IDENTIFIER","\\Ns\\O\\Rifttype.cpp",0x678);
      }
      iVar3 = __stricmp((char *)piVar8[2],"dontdrawdefault");
      if (iVar3 == 0) {
        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",0x67e);
      }
      iVar3 = __stricmp((char *)piVar8[2],"carribleInVehicle");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x10000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"container");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"default_hotspot");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"defaulthotspot"), iVar3 == 0)) {
        piVar9[0x3a] = piVar9[0x3a] | 1;
      }
      iVar3 = __stricmp((char *)piVar8[2],"shadow");
      if (iVar3 == 0) {
        if ((piVar9[0x3a] & 0x400000U) != 0) {
          FUN_004e0620("!(ts->typeFlags & TYPE_FLAG_FLYER_SHADOW)","\\Ns\\O\\Rifttype.cpp",0x68f);
        }
        piVar9[0x3a] = piVar9[0x3a] | 0x40000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"flyershadow");
      if (iVar3 == 0) {
        if ((piVar9[0x3a] & 0x40000U) != 0) {
          FUN_004e0620("!(ts->typeFlags & TYPE_FLAG_SHADOW)","\\Ns\\O\\Rifttype.cpp",0x694);
        }
        piVar9[0x3a] = piVar9[0x3a] | 0x400000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"not_selectable");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"notselectable"), iVar3 == 0)) {
        piVar9[0x3a] = piVar9[0x3a] | 0x8000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"not_real");
      if ((iVar3 == 0) || (iVar3 = __stricmp((char *)piVar8[2],"notreal"), iVar3 == 0)) {
        piVar9[0x3b] = piVar9[0x3b] | 0x2000000;
        piVar9[0x3a] = piVar9[0x3a] | 0x28000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"createsisland");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x400;
      }
      iVar3 = __stricmp((char *)piVar8[2],"mayDropOnRim");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 4;
      }
      iVar3 = __stricmp((char *)piVar8[2],"mayDropOnIsle");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 2;
      }
      iVar3 = __stricmp((char *)piVar8[2],"surface");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x800;
      }
      iVar3 = __stricmp((char *)piVar8[2],"randframe");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x100000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"matchframe");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x200000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"opaqueCollide");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x800000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"dontSave");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20000000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveFrame");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x20;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveQA");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x1000;
      }
      iVar3 = __stricmp((char *)piVar8[2],"saveQB");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x2000;
      }
      else {
        iVar3 = __stricmp((char *)piVar8[2],"dropblocking");
        if (iVar3 == 0) {
          piVar9[0x3b] = piVar9[0x3b] | 0x10;
        }
        else {
          iVar3 = __stricmp((char *)piVar8[2],"blocking");
          if (iVar3 == 0) {
            piVar9[0x3b] = piVar9[0x3b] | 0x18;
          }
          else {
            iVar3 = __stricmp((char *)piVar8[2],"shotblocking");
            if (iVar3 == 0) {
              piVar9[0x3b] = piVar9[0x3b] | 0x20;
            }
            else {
              iVar3 = __stricmp((char *)piVar8[2],"fencePost");
              if (iVar3 == 0) {
                piVar9[0x3b] = piVar9[0x3b] | 1;
              }
              else {
                iVar3 = __stricmp((char *)piVar8[2],"fence");
                if (iVar3 == 0) {
                  piVar9[0x3b] = piVar9[0x3b] | 0x20000000;
                }
                else {
                  iVar3 = __stricmp((char *)piVar8[2],"buried");
                  if (iVar3 == 0) {
                    pcVar7 = (char *)piVar8[2];
                    pcVar4 = pcVar7;
                    do {
                      cVar2 = *pcVar4;
                      pcVar4 = pcVar4 + 1;
                    } while (cVar2 != '\0');
                    if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                      pcVar4 = pcVar7;
                      do {
                        cVar2 = *pcVar4;
                        pcVar4 = pcVar4 + 1;
                      } while (cVar2 != '\0');
                      pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                    }
                    else {
                      pcVar4 = (char *)0x8;
                    }
                    _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                    piVar9[0x3b] = piVar9[0x3b] | 0x2000;
                  }
                  else {
                    iVar3 = __stricmp((char *)piVar8[2],"nugget");
                    if (iVar3 == 0) {
                      pcVar7 = (char *)piVar8[2];
                      pcVar4 = pcVar7;
                      do {
                        cVar2 = *pcVar4;
                        pcVar4 = pcVar4 + 1;
                      } while (cVar2 != '\0');
                      if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                        pcVar4 = pcVar7;
                        do {
                          cVar2 = *pcVar4;
                          pcVar4 = pcVar4 + 1;
                        } while (cVar2 != '\0');
                        pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                      }
                      else {
                        pcVar4 = (char *)0x8;
                      }
                      _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                      piVar9[0x3b] = piVar9[0x3b] | 0x8000;
                    }
                    else {
                      iVar3 = __stricmp((char *)piVar8[2],"bomb");
                      if (iVar3 == 0) {
                        pcVar7 = (char *)piVar8[2];
                        pcVar4 = pcVar7;
                        do {
                          cVar2 = *pcVar4;
                          pcVar4 = pcVar4 + 1;
                        } while (cVar2 != '\0');
                        if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                          pcVar4 = pcVar7;
                          do {
                            cVar2 = *pcVar4;
                            pcVar4 = pcVar4 + 1;
                          } while (cVar2 != '\0');
                          pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                        }
                        else {
                          pcVar4 = (char *)0x8;
                        }
                        _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                        piVar9[0x3b] = piVar9[0x3b] | 0x100;
                      }
                      else {
                        iVar3 = __stricmp((char *)piVar8[2],"emplacement");
                        if (iVar3 == 0) {
                          piVar9[0x3b] = piVar9[0x3b] | 0x40000;
                        }
                        else {
                          iVar3 = __stricmp((char *)piVar8[2],"bridge");
                          if (iVar3 == 0) {
                            pcVar7 = (char *)piVar8[2];
                            pcVar4 = pcVar7;
                            do {
                              cVar2 = *pcVar4;
                              pcVar4 = pcVar4 + 1;
                            } while (cVar2 != '\0');
                            if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                              pcVar4 = pcVar7;
                              do {
                                cVar2 = *pcVar4;
                                pcVar4 = pcVar4 + 1;
                              } while (cVar2 != '\0');
                              pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                            }
                            else {
                              pcVar4 = (char *)0x8;
                            }
                            _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                            piVar9[0x3b] = piVar9[0x3b] | 4;
                          }
                          else {
                            iVar3 = __stricmp((char *)piVar8[2],"walker");
                            if (iVar3 == 0) {
                              pcVar7 = (char *)piVar8[2];
                              pcVar4 = pcVar7;
                              do {
                                cVar2 = *pcVar4;
                                pcVar4 = pcVar4 + 1;
                              } while (cVar2 != '\0');
                              if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                pcVar4 = pcVar7;
                                do {
                                  cVar2 = *pcVar4;
                                  pcVar4 = pcVar4 + 1;
                                } while (cVar2 != '\0');
                                pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                              }
                              else {
                                pcVar4 = (char *)0x8;
                              }
                              _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                              piVar9[0x3b] = piVar9[0x3b] | 0x10000;
                            }
                            else {
                              iVar3 = __stricmp((char *)piVar8[2],"balloon");
                              if (iVar3 == 0) {
                                pcVar7 = (char *)piVar8[2];
                                pcVar4 = pcVar7;
                                do {
                                  cVar2 = *pcVar4;
                                  pcVar4 = pcVar4 + 1;
                                } while (cVar2 != '\0');
                                if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                  pcVar4 = pcVar7;
                                  do {
                                    cVar2 = *pcVar4;
                                    pcVar4 = pcVar4 + 1;
                                  } while (cVar2 != '\0');
                                  pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                }
                                else {
                                  pcVar4 = (char *)0x8;
                                }
                                _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                piVar9[0x3b] = piVar9[0x3b] | 0x20000;
                              }
                              else {
                                iVar3 = __stricmp((char *)piVar8[2],"island");
                                if (iVar3 == 0) {
                                  pcVar7 = (char *)piVar8[2];
                                  pcVar4 = pcVar7;
                                  do {
                                    cVar2 = *pcVar4;
                                    pcVar4 = pcVar4 + 1;
                                  } while (cVar2 != '\0');
                                  if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                    pcVar4 = pcVar7;
                                    do {
                                      cVar2 = *pcVar4;
                                      pcVar4 = pcVar4 + 1;
                                    } while (cVar2 != '\0');
                                    pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                  }
                                  else {
                                    pcVar4 = (char *)0x8;
                                  }
                                  _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                  piVar9[0x3b] = piVar9[0x3b] | 2;
                                }
                                else {
                                  iVar3 = __stricmp((char *)piVar8[2],"islandThreeByThree");
                                  if (iVar3 == 0) {
                                    piVar9[0x3b] = piVar9[0x3b] | 0x1000000;
                                  }
                                  else {
                                    iVar3 = __stricmp((char *)piVar8[2],"factory");
                                    if (iVar3 == 0) {
                                      pcVar7 = (char *)piVar8[2];
                                      pcVar4 = pcVar7;
                                      do {
                                        cVar2 = *pcVar4;
                                        pcVar4 = pcVar4 + 1;
                                      } while (cVar2 != '\0');
                                      if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                        pcVar4 = pcVar7;
                                        do {
                                          cVar2 = *pcVar4;
                                          pcVar4 = pcVar4 + 1;
                                        } while (cVar2 != '\0');
                                        pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                      }
                                      else {
                                        pcVar4 = (char *)0x8;
                                      }
                                      _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                      piVar9[0x3b] = piVar9[0x3b] | 0x4000;
                                    }
                                    else {
                                      iVar3 = __stricmp((char *)piVar8[2],"vortex");
                                      if (iVar3 == 0) {
                                        pcVar7 = (char *)piVar8[2];
                                        pcVar4 = pcVar7;
                                        do {
                                          cVar2 = *pcVar4;
                                          pcVar4 = pcVar4 + 1;
                                        } while (cVar2 != '\0');
                                        if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                          pcVar4 = pcVar7;
                                          do {
                                            cVar2 = *pcVar4;
                                            pcVar4 = pcVar4 + 1;
                                          } while (cVar2 != '\0');
                                          pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                        }
                                        else {
                                          pcVar4 = (char *)0x8;
                                        }
                                        _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                        piVar9[0x3b] = piVar9[0x3b] | 0x200;
                                      }
                                      else {
                                        iVar3 = __stricmp((char *)piVar8[2],"edgefarm");
                                        if (iVar3 == 0) {
                                          pcVar7 = (char *)piVar8[2];
                                          pcVar4 = pcVar7;
                                          do {
                                            cVar2 = *pcVar4;
                                            pcVar4 = pcVar4 + 1;
                                          } while (cVar2 != '\0');
                                          if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                            pcVar4 = pcVar7;
                                            do {
                                              cVar2 = *pcVar4;
                                              pcVar4 = pcVar4 + 1;
                                            } while (cVar2 != '\0');
                                            pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                          }
                                          else {
                                            pcVar4 = (char *)0x8;
                                          }
                                          _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                          piVar9[0x3b] = piVar9[0x3b] | 0x80000;
                                        }
                                        else {
                                          iVar3 = __stricmp((char *)piVar8[2],"focus");
                                          if (iVar3 == 0) {
                                            pcVar7 = (char *)piVar8[2];
                                            pcVar4 = pcVar7;
                                            do {
                                              cVar2 = *pcVar4;
                                              pcVar4 = pcVar4 + 1;
                                            } while (cVar2 != '\0');
                                            if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                              pcVar4 = pcVar7;
                                              do {
                                                cVar2 = *pcVar4;
                                                pcVar4 = pcVar4 + 1;
                                              } while (cVar2 != '\0');
                                              pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                            }
                                            else {
                                              pcVar4 = (char *)0x8;
                                            }
                                            _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4);
                                          }
                                          else {
                                            iVar3 = __stricmp((char *)piVar8[2],"guy");
                                            if (iVar3 == 0) {
                                              pcVar7 = (char *)piVar8[2];
                                              pcVar4 = pcVar7;
                                              do {
                                                cVar2 = *pcVar4;
                                                pcVar4 = pcVar4 + 1;
                                              } while (cVar2 != '\0');
                                              if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8) {
                                                pcVar4 = pcVar7;
                                                do {
                                                  cVar2 = *pcVar4;
                                                  pcVar4 = pcVar4 + 1;
                                                } while (cVar2 != '\0');
                                                pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                              }
                                              else {
                                                pcVar4 = (char *)0x8;
                                              }
                                              _strncpy((char *)(piVar9 + 0x3c),pcVar7,(size_t)pcVar4
                                                      );
                                              piVar9[0x3b] = piVar9[0x3b] | 0x400;
                                            }
                                            else {
                                              iVar3 = __stricmp((char *)piVar8[2],"geyser");
                                              if (iVar3 == 0) {
                                                pcVar7 = (char *)piVar8[2];
                                                pcVar4 = pcVar7;
                                                do {
                                                  cVar2 = *pcVar4;
                                                  pcVar4 = pcVar4 + 1;
                                                } while (cVar2 != '\0');
                                                if (pcVar4 + (1 - (int)(pcVar7 + 1)) < (char *)0x8)
                                                {
                                                  pcVar4 = pcVar7;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                                }
                                                else {
                                                  pcVar4 = (char *)0x8;
                                                }
                                                _strncpy((char *)(piVar9 + 0x3c),pcVar7,
                                                         (size_t)pcVar4);
                                                piVar9[0x3b] = piVar9[0x3b] | 0x10000000;
                                              }
                                              else {
                                                iVar3 = __stricmp((char *)piVar8[2],"flyer");
                                                if (iVar3 == 0) {
                                                  piVar9[0x3b] = piVar9[0x3b] | 0x100000;
                                                }
                                                else {
                                                  iVar3 = __stricmp((char *)piVar8[2],"residence");
                                                  if (iVar3 == 0) {
                                                    pcVar7 = (char *)piVar8[2];
                                                    pcVar4 = pcVar7;
                                                    do {
                                                      cVar2 = *pcVar4;
                                                      pcVar4 = pcVar4 + 1;
                                                    } while (cVar2 != '\0');
                                                    if (pcVar4 + (1 - (int)(pcVar7 + 1)) <
                                                        (char *)0x8) {
                                                      pcVar4 = pcVar7;
                                                      do {
                                                        cVar2 = *pcVar4;
                                                        pcVar4 = pcVar4 + 1;
                                                      } while (cVar2 != '\0');
                                                      pcVar4 = pcVar4 + (1 - (int)(pcVar7 + 1));
                                                    }
                                                    else {
                                                      pcVar4 = (char *)0x8;
                                                    }
                                                    _strncpy((char *)(piVar9 + 0x3c),pcVar7,
                                                             (size_t)pcVar4);
                                                    piVar9[0x3b] = piVar9[0x3b] | 0x40000000;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp((char *)piVar8[2],"fringe");
                                                    if (iVar3 == 0) {
                                                      piVar9[0x3b] = piVar9[0x3b] | 0x1000;
                                                    }
                                                    else {
                                                      iVar3 = __stricmp((char *)piVar8[2],"priest");
                                                      if (iVar3 == 0) {
                                                        piVar9[0x3b] = piVar9[0x3b] | 0x200000;
                                                      }
                                                      else {
                                                        iVar3 = __stricmp((char *)piVar8[2],"dais");
                                                        if (iVar3 == 0) {
                                                          piVar9[0x3b] = piVar9[0x3b] | 0x400000;
                                                        }
                                                        else {
                                                          iVar3 = __stricmp((char *)piVar8[2],
                                                                            "altar");
                                                          if (iVar3 == 0) {
                                                            piVar9[0x3b] = piVar9[0x3b] | 0x80000000
                                                            ;
                                                          }
                                                          else {
                                                            iVar3 = __stricmp((char *)piVar8[2],
                                                                              "predictable");
                                                            if (iVar3 == 0) {
                                                              piVar9[0x7a] = piVar9[0x7a] | 99;
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
  if (param_3 != (int *)0x0) {
    if ((*param_3 != 9) && (param_3[1] != 7)) {
      FUN_004e0620("attributeValue->btype == BT_ATTRIBUTE_VALUE || attributeValue->utype == UT_ATTRIBUTE_VALUE_LIST"
                   ,"\\Ns\\O\\Rifttype.cpp",0x757);
    }
    local_14 = param_3[1];
    local_10 = -1;
LAB_0049d090:
    piVar8 = param_3;
    if (piVar8 != (int *)0x0) {
      local_10 = local_10 + 1;
      if ((*piVar8 == 0) && (piVar8[1] == local_14)) {
        param_3 = (int *)piVar8[3];
        piVar8 = (int *)piVar8[2];
      }
      else {
        param_3 = (int *)0x0;
      }
      local_18 = param_3;
      if (*piVar8 != 9) {
        FUN_004e0620("attributeValue->btype == BT_ATTRIBUTE_VALUE","\\Ns\\O\\Rifttype.cpp",0x75c);
      }
      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxHitPoints");
      if (iVar3 == 0) {
        piVar9[0x3a] = piVar9[0x3a] | 0x10;
        if (*(int *)piVar8[3] != 2) {
          FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",0x762);
        }
        iVar3 = FUN_004e49c0();
        if ((*(byte *)(piVar9 + 0x3a) & 0x10) == 0) {
          FUN_004e0620("(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0","\\Ns\\O\\Rifttype.cpp",0x1ef);
        }
        if (*piVar9 != 0) {
          FUN_004e0620("maxHitPoints == 0","\\Ns\\O\\Rifttype.cpp",0x1f0);
        }
        *piVar9 = iVar3;
      }
      else {
        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeConst");
        if (iVar3 != 0) {
          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"helpText");
          if (iVar3 == 0) {
            if (*(int *)piVar8[3] != 10) {
              FUN_004e0620("attributeValue->p2->btype == BT_STRING_CONSTANT","\\Ns\\O\\Rifttype.cpp"
                           ,0x76c);
            }
            pcVar7 = __strdup(*(char **)(piVar8[3] + 8));
            piVar9[0x10] = (int)pcVar7;
            for (pcVar7 = _strstr(pcVar7,"\\n"); pcVar7 != (char *)0x0;
                pcVar7 = _strstr(pcVar7,"\\n")) {
              *pcVar7 = ' ';
              pcVar7[1] = '\n';
            }
            goto LAB_0049d090;
          }
          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeVar");
          if (iVar3 == 0) {
            uVar13 = 0x779;
          }
          else {
            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueConst");
            if (iVar3 == 0) {
              uVar13 = 0x77d;
            }
            else {
              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueVar");
              if (iVar3 == 0) {
                uVar13 = 0x781;
              }
              else {
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"minUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x785);
                  }
                  FUN_0049ad00(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x78a);
                  }
                  FUN_0049ad60(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxRainBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x78f);
                  }
                  FUN_0049ad60(piVar9,0,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxThunderBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x794);
                  }
                  FUN_0049ad60(piVar9,1,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"maxWindBattleUsage");
                if (iVar3 == 0) {
                  if (*(int *)piVar8[3] != 2) {
                    FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                 0x799);
                  }
                  FUN_0049ad60(piVar9,2,*(undefined4 *)(piVar8[3] + 8));
                  goto LAB_0049d090;
                }
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"zorder");
                if ((iVar3 != 0) &&
                   (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"cursorzorder"), iVar3 != 0)) {
                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"foot_x");
                  if ((iVar3 == 0) ||
                     (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"footx"), iVar3 == 0)) {
                    if (*(int *)piVar8[3] != 2) {
                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER","\\Ns\\O\\Rifttype.cpp",
                                   0x7d5);
                    }
                    iVar3 = FUN_004e49c0();
                    piVar9[0x75] = iVar3;
                    if (DAT_0054dbe8 < iVar3) {
                      FUN_004e0620("ts->footX <= maxFootpadInTilesX","\\Ns\\O\\Rifttype.cpp",0x7d7);
                    }
                  }
                  else {
                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"foot_y");
                    if ((iVar3 == 0) ||
                       (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"footy"), iVar3 == 0)) {
                      if (*(int *)piVar8[3] != 2) {
                        FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                     "\\Ns\\O\\Rifttype.cpp",0x7dc);
                      }
                      iVar3 = FUN_004e49c0();
                      piVar9[0x76] = iVar3;
                      if (DAT_0054dbec < iVar3) {
                        FUN_004e0620("ts->footY <= maxFootpadInTilesY","\\Ns\\O\\Rifttype.cpp",0x7de
                                    );
                      }
                    }
                    else {
                      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"height");
                      if (iVar3 != 0) {
                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hpPerSec");
                        if (iVar3 == 0) {
                          if (*(int *)piVar8[3] != 2) {
                            FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                         "\\Ns\\O\\Rifttype.cpp",0x7e6);
                          }
                          piVar9[0x12] = *(int *)(piVar8[3] + 8);
                          goto LAB_0049d090;
                        }
                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeVar");
                        if ((iVar3 == 0) ||
                           (iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rangeConst"), iVar3 == 0))
                        {
                          uVar13 = 0x7ec;
                        }
                        else {
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"range");
                          if (iVar3 == 0) {
                            piVar9[0x11] = *(int *)(piVar8[3] + 8);
                            goto LAB_0049d090;
                          }
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"delayBetweenShots");
                          if (iVar3 == 0) {
                            piVar9[0x13] = *(int *)(piVar8[3] + 8);
                            goto LAB_0049d090;
                          }
                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueConst");
                          if (iVar3 == 0) {
                            uVar13 = 0x7f8;
                          }
                          else {
                            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"attackValueVar");
                            if (iVar3 == 0) {
                              uVar13 = 0x7fc;
                            }
                            else {
                              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rateConst");
                              if (iVar3 == 0) {
                                uVar13 = 0x800;
                              }
                              else {
                                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"rateVar");
                                if (iVar3 == 0) {
                                  uVar13 = 0x804;
                                }
                                else {
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootRatioX");
                                  if (iVar3 == 0) {
                                    if (*(int *)piVar8[3] != 2) {
                                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                   "\\Ns\\O\\Rifttype.cpp",0x808);
                                    }
                                    iVar3 = FUN_004e49c0();
                                    piVar9[0x77] = iVar3;
                                    goto LAB_0049d090;
                                  }
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootRatioY");
                                  if (iVar3 == 0) {
                                    if (*(int *)piVar8[3] != 2) {
                                      FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                   "\\Ns\\O\\Rifttype.cpp",0x80d);
                                    }
                                    iVar3 = FUN_004e49c0();
                                    piVar9[0x78] = iVar3;
                                    goto LAB_0049d090;
                                  }
                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootX");
                                  if (iVar3 == 0) {
                                    uVar13 = 0x812;
                                  }
                                  else {
                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"hotFootY");
                                    if (iVar3 != 0) {
                                      iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"priority");
                                      if (iVar3 == 0) {
                                        if (*(int *)piVar8[3] != 2) {
                                          FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                       "\\Ns\\O\\Rifttype.cpp",0x81a);
                                        }
                                        iVar3 = FUN_004e49c0();
                                        piVar9[0x36] = iVar3;
                                      }
                                      else {
                                        iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                          "constructionRate");
                                        if (iVar3 == 0) {
                                          if (*(int *)piVar8[3] != 2) {
                                            FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                         "\\Ns\\O\\Rifttype.cpp",0x81f);
                                          }
                                          piVar9[0x14] = *(int *)(piVar8[3] + 8);
                                        }
                                        else {
                                          iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"orientations"
                                                           );
                                          if (iVar3 == 0) {
                                            if (*(int *)piVar8[3] != 2) {
                                              FUN_004e0620("attributeValue->p2->btype == BT_NUMBER",
                                                           "\\Ns\\O\\Rifttype.cpp",0x824);
                                            }
                                            iVar3 = FUN_004e49c0();
                                            piVar9[0x44] = iVar3;
                                          }
                                          else {
                                            iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                              "researchDepend");
                                            if (iVar3 == 0) {
                                              if (*(int *)piVar8[3] != 10) {
                                                FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x82a);
                                              }
                                              FUN_0049b000(piVar9,*(undefined4 *)(piVar8[3] + 8));
                                            }
                                            else {
                                              iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                "activeSound");
                                              if (iVar3 == 0) {
                                                if (*(int *)piVar8[3] != 10) {
                                                  FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x831);
                                                }
                                                iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                            (piVar8[3] + 8));
                                                piVar9[0x1b] = iVar3;
                                              }
                                              else {
                                                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                  "fireSound");
                                                if (iVar3 == 0) {
                                                  if (*(int *)piVar8[3] != 10) {
                                                    FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x839);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1c] = iVar3;
                                                }
                                                else {
                                                  iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                    "impactSound");
                                                  if (iVar3 == 0) {
                                                    if (*(int *)piVar8[3] != 10) {
                                                      FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x840);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1d] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "moveSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x846);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1e] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "pickupSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x84d);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x1f] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "dropSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x854);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x20] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "fallSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x85b);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x21] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "miscSound0");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x862);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x22] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "miscSound1");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x869);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x22] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "buildDoneSound");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x870);
                                                  }
                                                  iVar3 = thunk_FUN_004a8d50(*(undefined4 *)
                                                                              (piVar8[3] + 8));
                                                  piVar9[0x24] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "damageEffect");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x877);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x35] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "speed");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x87c);
                                                  }
                                                  piVar9[0x38] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "turningSpeed");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x881);
                                                  }
                                                  piVar9[0x39] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "theme");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x886);
                                                  }
                                                  piVar9[0x26] = (&DAT_00592990)
                                                                 [**(char **)(piVar8[3] + 8)];
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "level");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x88b);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x25] = iVar3 + -1;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "useairdamage");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x890);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6c] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "airdamage");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x894);
                                                  }
                                                  piVar9[0x6d] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "casttime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x898);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6e] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "threat");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x89c);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x70] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "praytime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a0);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x6f] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "manacost");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a4);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x71] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "spawns");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8a8);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x72] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "effecttime");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8ac);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x73] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "cost");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8b0);
                                                  }
                                                  piVar9[0x31] = *(int *)(piVar8[3] + 8);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "group");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8b5);
                                                  }
                                                  iVar3 = FUN_004a8960(*(undefined4 *)
                                                                        (piVar8[3] + 8));
                                                  piVar9[0x27] = iVar3;
                                                  if (iVar3 == 10) {
                                                    FUN_004e0620("ts->group != NO_GROUP",
                                                                 "\\Ns\\O\\Rifttype.cpp",0x8b7);
                                                  }
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "mana");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8bb);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = pcVar4 + 1;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  if (9 < (uint)((int)pcVar4 - (int)pcVar7)) {
                                                    FUN_004e0620(
                                                  "strlen(attributeValue->p2->string) <= 9",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8bc);
                                                  }
                                                  pcVar7 = *(char **)(piVar8[3] + 8);
                                                  piVar8 = piVar9 + 0x28;
                                                  do {
                                                    cVar2 = *pcVar7;
                                                    pcVar7 = pcVar7 + 1;
                                                    *(char *)piVar8 = cVar2;
                                                    piVar8 = (int *)((int)piVar8 + 1);
                                                  } while (cVar2 != '\0');
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "class");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8c1);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = pcVar4 + 1;
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                  } while (cVar2 != '\0');
                                                  if (0x17 < (uint)((int)pcVar4 - (int)pcVar7)) {
                                                    FUN_004e0620(
                                                  "strlen(attributeValue->p2->string) <= (RT_CLASSNAME_LEN-1)"
                                                  ,"\\Ns\\O\\Rifttype.cpp",0x8c2);
                                                  }
                                                  pcVar4 = *(char **)(piVar8[3] + 8);
                                                  pcVar7 = (char *)((int)piVar9 + 0xaa);
                                                  do {
                                                    cVar2 = *pcVar4;
                                                    pcVar4 = pcVar4 + 1;
                                                    *pcVar7 = cVar2;
                                                    pcVar7 = pcVar7 + 1;
                                                  } while (cVar2 != '\0');
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "artifactChanceClass");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8c7);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x33] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "artifactFrame");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8cc);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x34] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "techBit");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 2) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_NUMBER",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8d1);
                                                  }
                                                  iVar3 = FUN_004e49c0();
                                                  piVar9[0x7b] = iVar3;
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "description");
                                                    if (iVar3 == 0) {
                                                      if (*(int *)piVar8[3] != 10) {
                                                        FUN_004e0620(
                                                  "attributeValue->p2->btype == BT_STRING_CONSTANT",
                                                  "\\Ns\\O\\Rifttype.cpp",0x8d6);
                                                  }
                                                  _strncpy((char *)(piVar9 + 6),
                                                           *(char **)(piVar8[3] + 8),0x28);
                                                  }
                                                  else {
                                                    iVar3 = __stricmp(*(char **)(piVar8[2] + 8),
                                                                      "crew");
                                                    if ((((iVar3 != 0) &&
                                                         (iVar3 = __stricmp(*(char **)(piVar8[2] + 8
                                                                                      ),"basecrew"),
                                                         iVar3 != 0)) &&
                                                        (iVar3 = __stricmp(*(char **)(piVar8[2] + 8)
                                                                           ,"joyfactor"), iVar3 != 0
                                                        )) && (iVar3 = __stricmp(*(char **)(piVar8[2
                                                  ] + 8),"foodproduced"), iVar3 != 0)) {
                                                    uVar13 = 0x8e4;
                                                    goto LAB_0049e539;
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
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                      goto LAB_0049d090;
                                    }
                                    uVar13 = 0x816;
                                  }
                                }
                              }
                            }
                          }
                        }
                        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",uVar13);
                        goto LAB_0049d090;
                      }
                      iVar3 = FUN_004e49c0();
                      piVar9[0x79] = iVar3;
                    }
                  }
                  goto LAB_0049d090;
                }
                piVar12 = piVar9 + 0x41;
                iVar3 = __stricmp(*(char **)(piVar8[2] + 8),"cursorzorder");
                if (iVar3 == 0) {
                  piVar9[0x42] = 1;
                  piVar12 = piVar9 + 0x43;
                }
                if (*(int *)piVar8[3] == 2) {
                  iVar3 = FUN_004e49c0();
                  *piVar12 = iVar3;
                  goto LAB_0049d090;
                }
                if (*(int *)piVar8[3] == 10) {
                  pcVar7 = *(char **)(piVar8[3] + 8);
                  FUN_00496c90(local_2c,"([A-Za-z0-9_]+)[ \\t]*");
                  local_4 = 0;
                  iVar3 = FUN_004964e0(local_2c,*(undefined4 *)(piVar8[3] + 8));
                  if (iVar3 != 0) {
                    pcVar7 = (char *)FUN_00495a10(local_2c,1,0,0x50,0);
                  }
                  uVar11 = 0;
                  do {
                    pcVar4 = _strstr((&PTR_s_zoNONE_00540d10)[uVar11 * 2],pcVar7);
                    if (pcVar4 != (char *)0x0) {
                      FUN_00496c90(local_38,"[ \\t]*([A-Za-z0-9_]+)[ \\t]*(\\+|-)[ \\t]*([0-9]+)");
                      local_4 = CONCAT31(local_4._1_3_,1);
                      iVar3 = FUN_004964e0(local_38,*(undefined4 *)(piVar8[3] + 8));
                      if (iVar3 == 0) {
                        *piVar12 = (&DAT_00540d14)[uVar11 * 2];
                      }
                      else {
                        pcVar7 = (char *)FUN_00495a10(local_38,2,0,0x50,0);
                        if (*pcVar7 == '+') {
                          pcVar7 = (char *)FUN_00495a10(local_38,3,0,0x50,0);
                          lVar5 = _atol(pcVar7);
                          *piVar12 = lVar5 + (&DAT_00540d14)[uVar11 * 2];
                        }
                        else {
                          pcVar7 = (char *)FUN_00495a10(local_38,2,0,0x50,0);
                          if (*pcVar7 == '-') {
                            pcVar7 = (char *)FUN_00495a10(local_38,3,0,0x50,0);
                            lVar5 = _atol(pcVar7);
                            *piVar12 = (&DAT_00540d14)[uVar11 * 2] - lVar5;
                          }
                        }
                      }
                      local_4 = local_4 & 0xffffff00;
                      FUN_00495a00(local_38);
                      if (uVar11 < 0x17) goto LAB_0049e7b3;
                      break;
                    }
                    uVar11 = uVar11 + 1;
                  } while (uVar11 < 0x17);
                  FUN_004e0620("zo<COUNTOF(zOrderDefs)","\\Ns\\O\\Rifttype.cpp",0x7c6);
LAB_0049e7b3:
                  local_4 = 0xffffffff;
                  FUN_00495a00(local_2c);
                  param_3 = local_18;
                  goto LAB_0049d090;
                }
                uVar13 = 0x7c9;
              }
            }
          }
LAB_0049e539:
          FUN_004e0620(&DAT_0050161c,"\\Ns\\O\\Rifttype.cpp",uVar13);
          goto LAB_0049d090;
        }
        FUN_004e0620("false","\\Ns\\O\\Rifttype.cpp",0x768);
      }
      goto LAB_0049d090;
    }
  }
  if ((param_4[1] != 2) && (*param_4 != 3)) {
    FUN_004e0620("frameBud->utype == UT_FRAME_LIST || frameBud->btype == BT_FRAME",
                 "\\Ns\\O\\Rifttype.cpp",0x8ed);
  }
  local_14 = param_4[1];
  iVar10 = -1;
  local_18 = param_4;
  iVar3 = 0;
  do {
    if ((*param_4 != 0) || (param_4[1] != local_14)) break;
    param_4 = (int *)param_4[3];
    iVar3 = iVar3 + 1;
  } while (param_4 != (int *)0x0);
  iVar3 = (uint)(param_4 != (int *)0x0) + iVar3;
  piVar9[0x45] = iVar3;
  puVar6 = operator_new(iVar3 * 4);
  piVar9[0x49] = (int)puVar6;
  for (uVar11 = piVar9[0x45] & 0x3fffffff; uVar11 != 0; uVar11 = uVar11 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  bVar1 = false;
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  while (piVar8 = (int *)0x0, local_18 != (int *)0x0) {
    local_40 = local_18;
    iVar10 = iVar10 + 1;
    if ((*local_18 == 0) && (local_18[1] == local_14)) {
      piVar8 = (int *)local_18[3];
      local_40 = (int *)local_18[2];
    }
    pcVar7 = (char *)(piVar9[0x49] + iVar10 * 4);
    local_18 = piVar8;
    if (*local_40 != 3) {
      FUN_004e0620("frameBud->btype == BT_FRAME","\\Ns\\O\\Rifttype.cpp",0x8f6);
    }
    piVar8 = (int *)local_40[2];
    if (*piVar8 != 4) {
      FUN_004e0620("orientVariant->btype == BT_ORIENT_VARIANT","\\Ns\\O\\Rifttype.cpp",0x8fa);
    }
    cVar2 = **(char **)(piVar8[2] + 8);
    *pcVar7 = cVar2;
    if ((cVar2 < 'A') || ('Z' < cVar2)) {
      FUN_004e0620("\'A\' <= frame->sideOrientation && frame->sideOrientation <= \'Z\'",
                   "\\Ns\\O\\Rifttype.cpp",0x8fd);
    }
    cVar2 = *(char *)(*(int *)(piVar8[2] + 8) + 1);
    if (cVar2 == '\0') {
      pcVar7[1] = 'P';
    }
    else {
      pcVar7[1] = cVar2;
    }
    cVar2 = FUN_004e49c0();
    pcVar7[2] = cVar2;
    pcVar7[3] = '\0';
    piVar8 = (int *)local_40[3];
    if (piVar8 != (int *)0x0) {
      local_24 = piVar8[1];
      while (piVar8 != (int *)0x0) {
        if ((*piVar8 == 0) && (piVar8[1] == local_24)) {
          piVar12 = (int *)piVar8[3];
          piVar8 = (int *)piVar8[2];
        }
        else {
          piVar12 = (int *)0x0;
        }
        if (*piVar8 != 1) {
          FUN_004e0620("frameFlagBud->btype == BT_IDENTIFIER","\\Ns\\O\\Rifttype.cpp",0x90d);
        }
        iVar3 = __stricmp((char *)piVar8[2],"fringe");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 1;
        }
        iVar3 = __stricmp((char *)piVar8[2],"suck");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 2;
        }
        iVar3 = __stricmp((char *)piVar8[2],"rim");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 4;
        }
        iVar3 = __stricmp((char *)piVar8[2],"lit");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 8;
        }
        iVar3 = __stricmp((char *)piVar8[2],"unlit");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"cracked");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x20;
        }
        iVar3 = __stricmp((char *)piVar8[2],"hard");
        if (iVar3 == 0) {
          pcVar7[3] = pcVar7[3] | 0x40;
        }
        iVar3 = __stricmp((char *)piVar8[2],"baseframe");
        if (iVar3 == 0) {
          piVar9[0x4a] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"gumpframe");
        if (iVar3 == 0) {
          piVar9[0x47] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"default");
        if ((iVar3 == 0) && (piVar9[0x46] = iVar10, !bVar1)) {
          piVar9[0x48] = iVar10;
        }
        iVar3 = __stricmp((char *)piVar8[2],"help");
        piVar8 = piVar12;
        if (iVar3 == 0) {
          piVar9[0x48] = iVar10;
          bVar1 = true;
        }
      }
    }
  }
  if (DAT_0059af78 == (int *)0x0) {
    DAT_0059af78 = DAT_0059ab24;
  }
  if (*DAT_0059af78 != 0x30312e31) {
    do {
      if (999 < (int)piVar8) goto LAB_0049eb6b;
      piVar12 = DAT_0059af78 + 1;
      DAT_0059af78 = DAT_0059af78 + 1;
      piVar8 = (int *)((int)piVar8 + 1);
    } while (*piVar12 != 0x30312e31);
    if (999 < (int)piVar8) {
LAB_0049eb6b:
      FUN_004e0620("i < 1000","\\Ns\\O\\Rifttype.cpp",0x944);
    }
  }
  piVar9[0x37] = (int)DAT_0059af78;
  DAT_0059af78 = DAT_0059af78 + 1;
  FUN_0049c270();
  ExceptionList = local_c;
  return;
}


