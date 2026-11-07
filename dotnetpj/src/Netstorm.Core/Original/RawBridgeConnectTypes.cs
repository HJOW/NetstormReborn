// 다리·섬 연결이 읽는 원본 전역과 외부 효과 경계만 복원한다.
namespace Netstorm.Core.Original;

// 다리·섬 연결 형식을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// 연결 객체와 종유석을 만들 때 생성 함수에 넘기는 값이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public static class BridgeConnectValues
{
    /// <summary>원본 004af530(type, 2)의 생성 플래그다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public const uint ConnectCreateFlags = 2u;
}

// 연결 함수들이 읽는 원본 전역이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class BridgeConnectState
{
    /// <summary>연결 객체 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint ConnectorType { get; set; }

    /// <summary>섬 받침 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint IslandType { get; set; }

    /// <summary>섬 아래 종유석 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint StalagType { get; set; }

    /// <summary>섬 받침 위의 표면 칸 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint NoIslandType { get; set; }

    /// <summary>단어 쓰기가 표면 알림을 부르는 다리 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint BridgeType { get; set; }

    /// <summary>전투 모드다. 꺼져 있으면 색 프레임은 항상 중립이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool Battle { get; set; }

    /// <summary>미션 중이다. 중립 프레임의 섬은 소유자가 바뀌어도 프레임을 그대로 둔다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool Mission { get; set; }

    /// <summary>소유자 색 표 9칸이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public int[] OwnerColors { get; } = [0, 1, 2, 3, 4, 5, 6, 7, 8];
}

// 연결 함수들이 부르는 외부 효과다. 호출 순서는 원본과 같다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class BridgeConnectHooks
{
    /// <summary>새 객체 생성이다. 타입 바이트와 가상 표가 채워진 할당 번호를 돌려준다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>가상 소유자 지정이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>가상 Pop이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>프레임 지정이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }

    /// <summary>연결 객체의 타입이 다리 타입 전역과 같을 때만 부르는 표면 알림이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int>? NotifySurface { get; set; }
}
