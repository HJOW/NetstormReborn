// ==== FUN_00460730 @ 00460730

void __cdecl FUN_00460730(void)

{
  char cVar1;
  char *pcVar2;
  char *pcVar3;
  uint uVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  char *pcVar8;
  undefined4 *puVar9;
  char *pcVar10;
  undefined2 *puVar11;
  bool bVar12;
  char local_108;
  undefined4 uStack_107;
  
  local_108 = '\0';
  puVar9 = &uStack_107;
  for (iVar6 = 0x3f; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  *(undefined2 *)puVar9 = 0;
  *(undefined1 *)((int)puVar9 + 2) = 0;
  if (DAT_0055b490 != (char *)0x0) {
    iVar6 = 1;
    bVar12 = true;
    pcVar3 = DAT_0055b490;
    pcVar2 = "";
    do {
      if (iVar6 == 0) break;
      iVar6 = iVar6 + -1;
      bVar12 = *pcVar3 == *pcVar2;
      pcVar3 = pcVar3 + 1;
      pcVar2 = pcVar2 + 1;
    } while (bVar12);
    if (!bVar12) {
      if (DAT_005c85a4 == 0) {
        if (DAT_00594fa4 == 0) {
          if (((&DAT_005c89f0)[DAT_00540cac * 5] != 0) && ((&DAT_005c89f8)[DAT_00540cac * 5] != 0))
          {
            pcVar2 = (char *)FUN_004410f0("readonly",0,0,0);
            pcVar3 = pcVar2;
            do {
              cVar1 = *pcVar3;
              pcVar3 = pcVar3 + 1;
            } while (cVar1 != '\0');
            pcVar10 = &stack0xfffffef7;
            do {
              pcVar8 = pcVar10 + 1;
              pcVar10 = pcVar10 + 1;
            } while (*pcVar8 != '\0');
            pcVar8 = pcVar2;
            for (uVar4 = (uint)((int)pcVar3 - (int)pcVar2) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
              *(undefined4 *)pcVar10 = *(undefined4 *)pcVar8;
              pcVar8 = pcVar8 + 4;
              pcVar10 = pcVar10 + 4;
            }
            for (uVar4 = (int)pcVar3 - (int)pcVar2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
              *pcVar10 = *pcVar8;
              pcVar8 = pcVar8 + 1;
              pcVar10 = pcVar10 + 1;
            }
            puVar11 = (undefined2 *)&stack0xfffffef7;
            do {
              pcVar3 = (char *)((int)puVar11 + 1);
              puVar11 = (undefined2 *)((int)puVar11 + 1);
            } while (*pcVar3 != '\0');
            *puVar11 = DAT_00504ad0;
          }
          pcVar2 = (char *)FUN_004410f0("windowName",DAT_0055b490,0,0);
          pcVar3 = pcVar2;
          do {
            cVar1 = *pcVar3;
            pcVar3 = pcVar3 + 1;
          } while (cVar1 != '\0');
          uVar4 = (int)pcVar3 - (int)pcVar2;
          pcVar3 = &stack0xfffffef7;
          do {
            pcVar10 = pcVar3 + 1;
            pcVar3 = pcVar3 + 1;
          } while (*pcVar10 != '\0');
        }
        else {
          iVar6 = FUN_00463a20(&DAT_00562ca0);
          if (iVar6 == 0) {
            iVar6 = FUN_004639e0(&DAT_00562ca0);
            if (iVar6 == 0) {
              uVar5 = FUN_00463a60(0,0);
              pcVar2 = (char *)FUN_004410f0("missionWindowName",uVar5);
              pcVar3 = pcVar2;
              do {
                cVar1 = *pcVar3;
                pcVar3 = pcVar3 + 1;
              } while (cVar1 != '\0');
              uVar4 = (int)pcVar3 - (int)pcVar2;
              pcVar3 = &stack0xfffffef7;
              do {
                pcVar10 = pcVar3 + 1;
                pcVar3 = pcVar3 + 1;
              } while (*pcVar10 != '\0');
            }
            else {
              uVar5 = FUN_00463a60(0,0);
              pcVar2 = (char *)FUN_004410f0("testBattleWindowName",uVar5);
              pcVar3 = pcVar2;
              do {
                cVar1 = *pcVar3;
                pcVar3 = pcVar3 + 1;
              } while (cVar1 != '\0');
              uVar4 = (int)pcVar3 - (int)pcVar2;
              pcVar3 = &stack0xfffffef7;
              do {
                pcVar10 = pcVar3 + 1;
                pcVar3 = pcVar3 + 1;
              } while (*pcVar10 != '\0');
            }
          }
          else {
            uVar5 = FUN_00463a60(0,0);
            pcVar2 = (char *)FUN_004410f0("demoWindowName",uVar5);
            pcVar3 = pcVar2;
            do {
              cVar1 = *pcVar3;
              pcVar3 = pcVar3 + 1;
            } while (cVar1 != '\0');
            uVar4 = (int)pcVar3 - (int)pcVar2;
            pcVar3 = &stack0xfffffef7;
            do {
              pcVar10 = pcVar3 + 1;
              pcVar3 = pcVar3 + 1;
            } while (*pcVar10 != '\0');
          }
        }
      }
      else {
        pcVar2 = (char *)FUN_004410f0("EditorWindowName",DAT_0055b490,0,0);
        pcVar3 = pcVar2;
        do {
          cVar1 = *pcVar3;
          pcVar3 = pcVar3 + 1;
        } while (cVar1 != '\0');
        uVar4 = (int)pcVar3 - (int)pcVar2;
        pcVar3 = &stack0xfffffef7;
        do {
          pcVar10 = pcVar3 + 1;
          pcVar3 = pcVar3 + 1;
        } while (*pcVar10 != '\0');
      }
      goto LAB_00460958;
    }
  }
  pcVar2 = (char *)FUN_004410f0("blankWindowName",0,0,0);
  pcVar3 = pcVar2;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  uVar4 = (int)pcVar3 - (int)pcVar2;
  pcVar3 = &stack0xfffffef7;
  do {
    pcVar10 = pcVar3 + 1;
    pcVar3 = pcVar3 + 1;
  } while (*pcVar10 != '\0');
LAB_00460958:
  for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar3 = *(undefined4 *)pcVar2;
    pcVar2 = pcVar2 + 4;
    pcVar3 = pcVar3 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar3 = *pcVar2;
    pcVar2 = pcVar2 + 1;
    pcVar3 = pcVar3 + 1;
  }
  FUN_00435c60(&local_108);
  return;
}


