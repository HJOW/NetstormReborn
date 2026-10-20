namespace Netstorm.Core.Original;

/// <summary>
/// 원본 타입 표의 한 줄 가운데 원본 월드 코어가 쓰는 필드. 모듈이 늘면 필요한 필드를 여기에 더한다.
/// 게임이 쓰는 타입 정의는 Assets 의 TypeCatalog 이며, 이 레코드는 코어 검사와 연결 어댑터가 같은 값을 주고받는 얇은 모양이다.
/// 근거: cpppj/src/o/RiftType.h 의 RiftTypeRecord (W02 첫 조각, 2026-10-16).
/// </summary>
public sealed class OriginalTypeRecord
{
    /// <summary>플래그1의 "hit points 가 있음" 비트(typeflags maxHitPoints). 서버 생성 때 HP 를 채울지 정한다.</summary>
    public const uint HasHitPoints = 0x10;

    /// <summary>플래그2의 받침(dais) 비트. 켜진 타입은 HP 필요 여부를 타입이 아니라 객체의 abstract 비트가 정한다.</summary>
    public const uint Dais = 0x400000;

    /// <summary>플래그1의 표면(surface) 비트. 켜진 타입은 발자국이 건물군이 아니어도 공간 해제가 점유(spot)를 지운다. 기존 TypeFlagBits.Surface 와 같은 값이다.</summary>
    public const uint Surface = 0x800;

    /// <summary>플래그2에서 안쪽 비트와 옆면 처리를 받는 건물군 마스크(원본 005424b8). 신전·워크샾·포대류·받침·간헐천·거주 구역이 해당한다.</summary>
    public const uint BuildingMask = 0x50444200;

    /// <summary>플래그2에서 점유(spot)를 쓰는 타입의 마스크: 건물군과 섬·다리 등 하위 바이트(0x400ff)의 합집합.</summary>
    public const uint SpotWriteMask = BuildingMask | 0x400FF;

    /// <summary>타입 플래그 1(원본 타입 구조체 +0xE8).</summary>
    public uint Flags1 { get; set; }

    /// <summary>타입 플래그 2(원본 타입 구조체 +0xEC).</summary>
    public uint Flags2 { get; set; }

    /// <summary>최대 HP 속성(원본 타입 구조체 +0). 서버가 오브젝트를 만들 때 초기 HP 로 쓴다.</summary>
    public int MaxHitPoints { get; set; }

    /// <summary>비용(원본 타입 구조체 +0xc4, float). 공통 PostPop 이 전역 비용 집계에 더하는 값이다. 추가: W02 PostPop 조각(2026-10-20).</summary>
    public float Cost { get; set; }

    /// <summary>그리기 깊이 zOrder(원본 타입 구조체 +0x104). 기본 Squid 에는 하위 8비트만 복사한다.</summary>
    public int ZOrder { get; set; }

    /// <summary>생성자 원본 주소(원본 타입 구조체 +0x1D0). 0 이면 기본 생성이다. OriginalConstructors 의 표에 있어야 지원한다.</summary>
    public uint ConstructorAddress { get; set; }

    /// <summary>발자국 가로 칸 수(1~256). 공간 해제가 점유(spot)를 지울 칸의 폭이다. 기본 1 은 가장 작은 유효 발자국이다. 추가: W02 Unpop 조각(2026-10-16).</summary>
    public int FootX { get; set; } = 1;

    /// <summary>발자국 세로 칸 수(1~256). 기본 1. 추가: W02 Unpop 조각(2026-10-16).</summary>
    public int FootY { get; set; } = 1;
}
