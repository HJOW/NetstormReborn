namespace Netstorm.Core.Original;

/// <summary>
/// 게임 보조 영역의 실제 전역 값을 나누며 목록·호스트가 참조한다
/// (cpppj <c>o/RawGameAuxState.h</c>의 <c>RawGameAuxState</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 같은 주소를 고정하는 게임 데이터 owner 주소이며 세션 조립(F3) 때 연결한다.</remarks>
public sealed class RawGameAuxState
{
    /// <summary>005950ac의 값이다. 시작이 많은 간헐천 부분을 읽는다.</summary>
    public uint OrdinaryGeysers { get; set; }

    /// <summary>005950b0의 값이다. 486bc0의 미복원 프레임 입력이다.</summary>
    public uint GeyserFrameFlag { get; set; }

    /// <summary>00595090의 값이다. 저장 상태 읽기의 현재 출력 주소 소스다.</summary>
    public uint SavedGeyserCount { get; set; }

    /// <summary>005950b8의 게임 시각 별도 8바이트 기한이다.</summary>
    public double GeyserDeadline { get; set; }

    /// <summary>005950c0의 원문 정리 작업 별도 8바이트다.</summary>
    public double CleanupDeadline { get; set; }

    /// <summary>005950c8의 b8과 별개인 게임 프레임 기한이다.</summary>
    public double SecondaryGeyserDeadline { get; set; }

    /// <summary>00595358의 그리기·받침 부모가 읽는 별도 게임 기한이다.</summary>
    public double ScheduledDrawDeadline { get; set; }

    /// <summary>0059508c의 실제 Pop·소멸 집계 부호 있는 비용 DWORD다.</summary>
    public int TotalCost { get; set; }

    /// <summary>00595088의 보상·로컬 간헐천이 직접 공유하는 부호 있는 물리 값이다.</summary>
    public int ResourcePool { get; set; }

    /// <summary>로그인·폭탄의 부호 없는 조회에 쓰는 명시 비트 변환이다.</summary>
    /// <returns>같은 DWORD의 비트다.</returns>
    public uint ResourcePoolBits()
    {
        return unchecked((uint)ResourcePool);
    }

    /// <summary>다른 원문의 참조와 별칭 없이 같은 DWORD를 쓴다.</summary>
    /// <param name="value">쓸 비트다.</param>
    public void StoreResourcePoolBits(uint value)
    {
        ResourcePool = unchecked((int)value);
    }

    /// <summary>005424bc의 보상·로그인·전투 옵션이 공유하는 초기 25 DWORD다.</summary>
    public uint RefundPercent { get; set; } = 25u;

    /// <summary>005453e4의 줍기 뒤 관문이 읽는 같은 DWORD의 raw 값이다.</summary>
    public int AllowAnyCapture { get; set; }

    /// <summary>005c8bb0의 실제 FarmCluster 모드이며 로그인의 bb4와 다르다.</summary>
    public uint Archipelago { get; set; }

    /// <summary>005c848c의 공통 소멸이 따로 기록하는 최근 소실 X다.</summary>
    public float LostX { get; set; }

    /// <summary>005c8490의 최근 소실 Y이며 정리가 X보다 먼저 둔다.</summary>
    public float LostY { get; set; }

    /// <summary>00540bdc의 초기 2000 부호 있는 정수이며 타입 비용과는 float로 변환해 둔다.</summary>
    public int DefaultSp { get; set; } = 2000;

    /// <summary>00541240의 실제 현재 간헐천 타입이며 호스트 pollType이 같은 주소를 고정한다. 주소 빌림은 같은 상자를 쓴다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 상자 공유로 바꿨다.</summary>
    public uint GeyserType
    {
        get => _geyserTypeBox.Value;
        set => _geyserTypeBox.Value = value;
    }

    /// <summary>같은 간헐천 타입 DWORD의 공유 owner다. 호스트 pollType이 이 주소를 고정한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox GeyserTypeBox => _geyserTypeBox;

    /// <summary>같은 간헐천 타입 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _geyserTypeBox = new() { Value = 122u };

    /// <summary>두 보호 PE의 최초 ProcessForm 번호이며 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint ProcessFormType
    {
        get => _processFormTypeBox.Value;
        set => _processFormTypeBox.Value = value;
    }

    /// <summary>같은 ProcessForm 번호 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox ProcessFormTypeBox => _processFormTypeBox;

    /// <summary>같은 ProcessForm 번호 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _processFormTypeBox = new() { Value = 2u };

    /// <summary>일반 절차 번호의 실제 초기 DWORD다. 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint RegularProcessType
    {
        get => _regularProcessTypeBox.Value;
        set => _regularProcessTypeBox.Value = value;
    }

    /// <summary>같은 일반 절차 번호 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox RegularProcessTypeBox => _regularProcessTypeBox;

    /// <summary>같은 일반 절차 번호 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _regularProcessTypeBox = new() { Value = 46u };

    /// <summary>공유 절차 번호의 실제 초기 DWORD다. 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint SharedRegularProcessType
    {
        get => _sharedRegularProcessTypeBox.Value;
        set => _sharedRegularProcessTypeBox.Value = value;
    }

    /// <summary>같은 공유 절차 번호 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox SharedRegularProcessTypeBox => _sharedRegularProcessTypeBox;

    /// <summary>같은 공유 절차 번호 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _sharedRegularProcessTypeBox = new() { Value = 61u };

    /// <summary>절차 상한의 실제 초기 DWORD다. 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint ProcessTypeEnd
    {
        get => _processTypeEndBox.Value;
        set => _processTypeEndBox.Value = value;
    }

    /// <summary>같은 절차 상한 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox ProcessTypeEndBox => _processTypeEndBox;

    /// <summary>같은 절차 상한 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _processTypeEndBox = new() { Value = 63u };

    /// <summary>발사 애니메이션 절차의 실제 초기 DWORD다. 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint FireAnimationProcessType
    {
        get => _fireAnimationProcessTypeBox.Value;
        set => _fireAnimationProcessTypeBox.Value = value;
    }

    /// <summary>같은 발사 번호 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox FireAnimationProcessTypeBox => _fireAnimationProcessTypeBox;

    /// <summary>같은 발사 번호 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _fireAnimationProcessTypeBox = new() { Value = 35u };

    /// <summary>회전 애니메이션 절차의 실제 초기 DWORD다. 일반 World 준비에서도 유지한다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public uint RotateAnimationProcessType
    {
        get => _rotateAnimationProcessTypeBox.Value;
        set => _rotateAnimationProcessTypeBox.Value = value;
    }

    /// <summary>같은 회전 번호 DWORD의 공유 owner다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    public UIntBox RotateAnimationProcessTypeBox => _rotateAnimationProcessTypeBox;

    /// <summary>같은 회전 번호 DWORD를 빌리는 상자다. 2026-11-07 F2d 프로세스 호스트 저장소 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _rotateAnimationProcessTypeBox = new() { Value = 36u };

    /// <summary>005411dc의 같은 현재 지형 타입이며 지역 조회·진단이 빌린다.</summary>
    public uint TerrainType { get; set; } = 97u;

    /// <summary>0054123c의 같은 현재 가장자리 농장 타입이다.</summary>
    public uint EdgeFarmType { get; set; } = 121u;

    /// <summary>00542654의 현재 공백·육지·분할 범위에 속한 생성 값이며 같은 주소를 둔다.</summary>
    public int WorldGeyserMargin { get; set; } = 2;

    /// <summary>005411e8·00541338·005411ec·0054133c의 현재 나무 둘·유적·나무 셋·기념물 순서다.</summary>
    public uint[] WorldDecorationTypes { get; } = [100u, 184u, 101u, 185u];

    /// <summary>005467bc의 구독자 별도 표시 DWORD다.</summary>
    public uint SubscriberMarker { get; set; }

    /// <summary>005ca8f0의 같은 현재 0·1이며 생성·HP·축소가 상태 정리를 공유한다.</summary>
    public bool PriestQuarterHp { get; set; }

    /// <summary>기존 지역 목록을 비어 있는 입력으로 되돌리며 실제 정리 전체가 아니다.</summary>
    public void ResetForGeneralWorld()
    {
        OrdinaryGeysers = 0;
        TotalCost = 0;
        ResourcePool = 0;
        RefundPercent = 25;
        AllowAnyCapture = 0;
        Archipelago = 0;
        LostX = 0.0f;
        LostY = 0.0f;
        GeyserType = 122;
        PriestQuarterHp = false;
    }

    /// <summary>실제 값 저장은 Y를 X보다 먼저 실행하며 일반 비움 0과 구별한다.</summary>
    public void ResetLostPositionForCleanup()
    {
        LostY = BitConverter.UInt32BitsToSingle(0xc61c4000u);
        LostX = BitConverter.UInt32BitsToSingle(0xc61c4000u);
    }
}
