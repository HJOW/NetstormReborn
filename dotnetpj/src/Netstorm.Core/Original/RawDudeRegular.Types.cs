// Dude 정기 분배의 현재 목록/순환/진단 전역과 실제 효과 경계를 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawDudeRegular.h의 현재 검사 모드·공급자 목록·보행 순환 상태다.</summary>
public sealed class DudeRegularState
{
    /// <summary>005e4794의 현재 진단 모드다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>00592f84/8c/90의 기존 에너지 공급자 목록이다.</summary>
    public RawPostPopList Providers { get; }
    /// <summary>0055a4fc의 같은 signed 순환 값이다. 몸체가 이동 때마다 갱신한다.</summary>
    public int ProviderIndex { get; set; }
    /// <summary>005326c0의 현재 사건이다.</summary>
    public uint PollEvent { get; set; } = 0x1a2b;
    /// <summary>005410d0의 현재 경로 종류다.</summary>
    public uint PathType { get; set; } = 24;
    /// <summary>실제 가상 진입은 TYPE70 선검사 없이 원문 위치에서 현재 필드를 읽는다.</summary>
    public bool Native { get; set; }
    /// <summary>같은 005326c0의 장기 주소다. 없으면 현재 사건 값을 쓴다.</summary>
    public UIntBox? SharedPollEvent { get; set; }
    /// <summary>같은 005410d0의 장기 주소다. 없으면 현재 경로 종류를 쓴다.</summary>
    public UIntBox? SharedPathType { get; set; }
    /// <summary>같은 검사 모드·목록·순환 값을 빌린다. 난수/프레임 표는 몸체가 받는다.</summary>
    public DudeRegularState(RawCheckingFlag checking, RawPostPopList providers, int providerIndex)
    {
        ArgumentNullException.ThrowIfNull(checking); ArgumentNullException.ThrowIfNull(providers);
        Checking = checking; Providers = providers; ProviderIndex = providerIndex;
    }
    /// <summary>첫 사건 비교 시 현재값을 읽는다.</summary>
    public uint CurrentPollEvent() => SharedPollEvent?.Value ?? PollEvent;
    /// <summary>지면 효과 뒤 현재 경로 종류를 읽는다.</summary>
    public uint CurrentPathType() => SharedPathType?.Value ?? PathType;
}

/// <summary>RawDudeRegular.h의 현재 몸체 효과 묶음이다. 도달하지 않은 효과는 빈 성공으로 대체하지 않는다.</summary>
public sealed class DudeRegularHooks
{
    /// <summary>450e80의 같은 지면/Fall/이동 제거 몸체다.</summary>
    public Func<ushort, bool>? CheckSpace { get; set; }
    /// <summary>4b1a90(parent,24,0,false)의 현재 종속 조회다.</summary>
    public Func<ushort, uint, ushort>? FindPath { get; set; }
    /// <summary>450670의 실제 Regular 재예약이다.</summary>
    public Action<ushort, uint, float>? Task { get; set; }
    /// <summary>48fd30의 현재 소유자/위치와 같은 거주 구역 목록이다.</summary>
    public Func<ushort, ushort>? NearestResidence { get; set; }
    /// <summary>4505a0의 현재 목표 발자국 진입점이다.</summary>
    public Func<ushort, RawPathPoint>? Entrance { get; set; }
    /// <summary>450760의 현재 속도·실제24 경로 시작이다.</summary>
    public Func<ushort, RawPathPoint, bool, bool>? Move { get; set; }
    /// <summary>4acee0의 실제 가상 공간/프레임 변경이다.</summary>
    public Action<ushort, int, uint>? SetFrame { get; set; }
    /// <summary>현재 부모의 실제+48이다.</summary>
    public Action<ushort, uint>? Unpop { get; set; }
    /// <summary>현재 부모의 실제+10이다.</summary>
    public Action<ushort, uint>? Destroy { get; set; }
    /// <summary>현재 부모+90의 좌표/flags다.</summary>
    public Action<ushort, RawPathPoint, uint>? Pop { get; set; }
    /// <summary>복귀하는 진단의 행 번호이며 세션은 예외로 진단한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>Native4ac220의 실제 WORD 쓰기 뒤 현재 Bridge 통지다.</summary>
    public Action<ushort, uint>? SetWord { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal DudeRegularHooks Copy() => (DudeRegularHooks)MemberwiseClone();
}
