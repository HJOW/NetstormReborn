// 같은 원문 권한/역할/방향별 동맹과 선택된 큰 발사 효과만 빌린다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawGunProcess.h의 현재 게임 상태와 독립 fixture 입력이다.</summary>
public sealed class GunProcessState
{
    /// <summary>현재 프레임이 공급한 게임 시각이다.</summary>
    public double Now { get; set; }
    /// <summary>공유 권한이 없는 독립 입력의 boss 값이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>공유 권한이 없는 독립 입력의 server 값이다.</summary>
    public bool Server { get; set; } = true;
    /// <summary>요새 모드의 자동 검색 관문이다.</summary>
    public bool FortMode { get; set; }
    /// <summary>고정 모드이면 Frame/발사를 중지한다.</summary>
    public bool FixedMode { get; set; }
    /// <summary>현재 총 프레임의 비활성 관문이다.</summary>
    public bool Disabled { get; set; }
    /// <summary>현재 친군 사격 DWORD의 비영 뷰다.</summary>
    public RawCheckingFlag FriendlyFire { get; set; } = new();
    /// <summary>현재 checking DWORD의 별도 비영 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>독립 생성 입력의 최초34 프로세스 TYPE다.</summary>
    public uint ProcessType { get; set; } = 34;
    /// <summary>독립 회전 조건의 최초140 윈드 아처 TYPE다.</summary>
    public uint WindArcherType { get; set; } = 140;
    /// <summary>독립 회전 조건의 최초131 썬더 캐논 TYPE다.</summary>
    public uint ThunderCannonType { get; set; } = 131;
    /// <summary>독립 회전 조건의 최초129 아이스 캐논 TYPE다.</summary>
    public uint RainCannonType { get; set; } = 129;
    /// <summary>실제 owner가 없을 때만 읽는 독립9×9 동맹 표다.</summary>
    public bool[,] Allies { get; } = new bool[9, 9];
    /// <summary>실제 방향별81DWORD의 현재 한 칸을 반환한다.</summary>
    public Func<uint, uint, uint>? AllianceRead { get; set; }
    /// <summary>같은 실제 Pool/Host의 두 권한 owner다.</summary>
    public RawAuthorityState? SharedAuthority { get; set; }
    /// <summary>실제00541288 역할 주소다.</summary>
    public UIntBox? SharedWindArcherType { get; set; }
    /// <summary>실제00541264 역할 주소다.</summary>
    public UIntBox? SharedThunderCannonType { get; set; }
    /// <summary>실제0054125c 역할 주소다.</summary>
    public UIntBox? SharedRainCannonType { get; set; }
    /// <summary>같은 현재 실제 boss를 소비 위치마다 읽는다.</summary>
    public bool IsBoss() => SharedAuthority?.Boss ?? Boss;
    /// <summary>같은 현재 실제 server를 소비 위치마다 읽는다.</summary>
    public bool IsServer() => SharedAuthority?.Server ?? Server;
    /// <summary>회전 조건에서 현재 윈드 아처 역할을 읽는다.</summary>
    public uint ReadWindArcherType() => SharedWindArcherType?.Value ?? WindArcherType;
    /// <summary>앞선 역할 조회 뒤 현재 썬더 캐논 역할을 읽는다.</summary>
    public uint ReadThunderCannonType() => SharedThunderCannonType?.Value ?? ThunderCannonType;
    /// <summary>현재 아이스 캐논 역할을 읽는다.</summary>
    public uint ReadRainCannonType() => SharedRainCannonType?.Value ?? RainCannonType;
    /// <summary>원문 방향만 읽으며 역방향이나 초기 bool 표로 현재 DWORD를 대체하지 않는다.</summary>
    public bool IsAlly(uint owner, uint target)
    {
        if (owner >= 9 || target >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "Gun 현재 방향별 동맹 범위 오류");
        return AllianceRead is { } read ? read(owner, target) != 0 : Allies[owner, target];
    }
}

/// <summary>RawGunProcess가 원문 소비 위치에서만 요구하는 실제 가상/검색/발사/애니메이션 장치다.</summary>
public sealed class GunProcessHooks
{
    /// <summary>현재 gun Form이 표적에 만든 감시 쌍을 제거한다.</summary>
    public Action<ushort, ushort>? RemoveDependency { get; set; }
    /// <summary>현재 표적과 같은 inner 객체에 감시 쌍을 만든다.</summary>
    public Action<ushort, GunProcessFields>? AddDependency { get; set; }
    /// <summary>현재 Form Unpop/flags0 Attach와 실제 전송이다.</summary>
    public Action<GunProcessFields>? RepopForm { get; set; }
    /// <summary>현재 부모 체인의 fire/rotate AnimationForm 첫 일치다.</summary>
    public Func<ushort, ushort>? AnimationForm { get; set; }
    /// <summary>현재 PID의 애니메이션을 flags40으로 종료한다.</summary>
    public Action<ushort, uint>? KillAnimation { get; set; }
    /// <summary>현재 부모의 사거리 표시를 갱신한다.</summary>
    public Action<ushort>? RefreshRange { get; set; }
    /// <summary>현재 부모/소유자의 공격 모드를 읽는다.</summary>
    public Func<ushort, byte, uint>? TargetingMode { get; set; }
    /// <summary>현재 x/y/TYPE/모드/소유자로 실제 표적을 검색한다.</summary>
    public Func<float, float, uint, uint, byte, ushort>? FindTarget { get; set; }
    /// <summary>현재 게임 난수의 x87 구간 반환값이다.</summary>
    public Func<float, float, double>? RandomBetween { get; set; }
    /// <summary>단계·gun Form·표적의 실제 애니메이션 전체 생성이다.</summary>
    public Action<uint, ushort, ushort>? StartAnimation { get; set; }
    /// <summary>PATCH checking의 실제 발자국 소유자 진단이다.</summary>
    public Action<ushort>? ValidateIslandOwners { get; set; }
    /// <summary>발사 앞뒤의 실제 부모 Form 체인 진단이다.</summary>
    public Action<ushort>? CheckForms { get; set; }
    /// <summary>발사 직전 현재 부모의 차단 비트를 읽는다.</summary>
    public Func<ushort, bool>? ParentBlocked { get; set; }
    /// <summary>현재 좌표/중심/TYPE/소유자/frame의8DWORD descriptor다.</summary>
    public Func<ushort, uint[]>? ShotContext { get; set; }
    /// <summary>실제 각도 판정/투사체 생성의 수락 여부다.</summary>
    public Func<uint[], ushort, bool>? Shoot { get; set; }
    /// <summary>현재 TYPE의 실제 float 발사 간격을 double로 반환한다.</summary>
    public Func<ushort, double>? FiringInterval { get; set; }
    /// <summary>실제 inner flags0 부착과 Kernel/전송이다.</summary>
    public Action<GunProcessFields, ushort>? Attach { get; set; }
    /// <summary>원문 식/파일/줄의 진단이며 정상 복귀 뒤 현재 필드를 다시 읽는다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>함수 슬롯을 값 복사하며 현재 효과를 미리 실행하지 않는다.</summary>
    internal GunProcessHooks Copy() => (GunProcessHooks)MemberwiseClone();
}
