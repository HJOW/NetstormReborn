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

    /// <summary>고정 외형 프레임 번호(원본 타입 구조체 +0x130 부근, 간헐천 가상78이 읽는다). 표시 수집이 억제보다 먼저 현재 TYPE+128을 읽을 때 쓴다. 2026-11-07 F2d 표시 수집 이식에서 추가됐다.</summary>
    public int DefaultFrame { get; set; }

    /// <summary>비용(원본 타입 구조체 +0xc4, float). 공통 PostPop 이 전역 비용 집계에 더하는 값이다. 추가: W02 PostPop 조각(2026-10-20).</summary>
    public float Cost { get; set; }

    /// <summary>그리기 깊이 zOrder(원본 타입 구조체 +0x104). 기본 Squid 에는 하위 8비트만 복사한다.</summary>
    public int ZOrder { get; set; }

    /// <summary>피격 효과(원본 타입 구조체 +0xd4). 0 이면 타격/화상 효과가 없고 burning 판정도 건너뛴다. 추가: W03 삭제·수명 여덟째 조각(2026-10-21).</summary>
    public int DamageEffect { get; set; }

    /// <summary>생성자 원본 주소(원본 타입 구조체 +0x1D0). 0 이면 기본 생성이다. OriginalConstructors 의 표에 있어야 지원한다.</summary>
    public uint ConstructorAddress { get; set; }

    /// <summary>발자국 가로 칸 수(1~256). 공간 해제가 점유(spot)를 지울 칸의 폭이다. 기본 1 은 가장 작은 유효 발자국이다. 추가: W02 Unpop 조각(2026-10-16).</summary>
    public int FootX { get; set; } = 1;

    /// <summary>발자국 세로 칸 수(1~256). 기본 1. 추가: W02 Unpop 조각(2026-10-16).</summary>
    public int FootY { get; set; } = 1;

    /// <summary>타입 그룹 번호(원본 타입 구조체 +0x9c). 기본값은 "그룹 없음"(10)이고, 삭제 수명주기의 잃어버린 소리 판정이 이 값을 본다. 추가: W03 삭제·수명 셋째 조각(2026-10-21).</summary>
    public int Group { get; set; } = 10;

    /// <summary>기술 비트 번호(원본 타입 구조체 PATCH +0x1ec·CD +0x1cc). 실제 초기값-1 뒤 techBit 숫자 속성을 그대로 두며, 없으면 -1이다. 추가: R0-4 마지막 묶음(2026-10-28).</summary>
    public int TechBitIndex { get; set; } = -1;

    /// <summary>추가 기준점의 가로 오프셋(TYPE 정의 hotspotX). 캡처가 CapTypeHotspot 옵션 때 기준점에 더한다. 추가: R2d-2(2026-10-29).</summary>
    public int HotspotX { get; set; }

    /// <summary>추가 기준점의 세로 오프셋(TYPE 정의 hotspotY). 추가: R2d-2(2026-10-29).</summary>
    public int HotspotY { get; set; }

    /// <summary>
    /// +4(20바이트)를 C 문자열로 읽은 이름이다. 내장 프로세스 타입은 20글자로 잘린다. .type 타입은 파일 이름이지만 20글자 이상이면 종료 문자가 없어
    /// 바로 뒤의 설명 필드까지 이어 읽힌다(fakeThreeByThreeSurface). 문자 하나 = 바이트 하나로 다룬다. 추가: F1 타입 표(2026-11-02).
    /// </summary>
    public string Name { get; set; } = "";

    /// <summary>+0x18(40바이트): <c>description</c> 속성이다. 추가: F1 타입 표(2026-11-02).</summary>
    public string Description { get; set; } = "";

    /// <summary>.type 파일 이름(로딩 순서의 이름)이다. 표시·도구용이며 원본 구조체 필드가 아니다. 추가: F1 타입 표(2026-11-02).</summary>
    public string AssetName { get; set; } = "";

    /// <summary>원소별 최소 에너지 사용량(+0x54, minUsage 속성). 추가: F1 타입 표(2026-11-02).</summary>
    public float[] MinUsage { get; set; } = new float[3];

    /// <summary>원소별 최대 에너지 사용량(+0x60: maxUsage·maxRainBattleUsage, maxThunderBattleUsage, maxWindBattleUsage). 추가: F1 타입 표(2026-11-02).</summary>
    public float[] MaxUsage { get; set; } = new float[3];

    /// <summary>레벨(+0x94). <c>level</c> 속성 - 1이다. 추가: F1 타입 표(2026-11-02).</summary>
    public int Level { get; set; }

    /// <summary>테마(+0x98). <c>theme</c> 첫 글자의 조회값이며 실제 표 초기화가 패치 5 / CD 4를 넣는다. 추가: F1 타입 표(2026-11-02).</summary>
    public int Theme { get; set; }

    /// <summary>마지막으로 나온 종류 단어(+0xf0, 최대 8글자, 원본 strncpy 그대로)다. 추가: F1 타입 표(2026-11-02).</summary>
    public string Kind { get; set; } = "";

    /// <summary>이 타입이 그릇일 때 내용물의 목록 플래그(+0xfc, 0이면 파일에서 읽는다)다. 추가: F1 타입 표(2026-11-02).</summary>
    public uint ContainerListFlags { get; set; }

    /// <summary>이 타입이 내용물일 때의 목록 플래그(+0x100, 0이면 그릇 것을 쓴다)다. 추가: F1 타입 표(2026-11-02).</summary>
    public uint ContentListFlags { get; set; }

    /// <summary>.type 파일에서 읽은 타입인가. 추가: F1 타입 표(2026-11-02).</summary>
    public bool FromAsset { get; set; }

    /// <summary>현재 값의 복사본을 만든다. 삭제 수명주기가 타입 입력을 고정할 때 쓴다(추가: W03 삭제·수명 셋째 조각, 2026-10-21).</summary>
    public OriginalTypeRecord Clone()
    {
        // 원본 SquidDestroyLifecycle 가 타입 표를 값으로 복사해 장부 처리 중의 입력을 고정하는 것과 같다
        return new OriginalTypeRecord
        {
            Flags1 = Flags1,
            Flags2 = Flags2,
            MaxHitPoints = MaxHitPoints,
            DefaultFrame = DefaultFrame,
            Cost = Cost,
            ZOrder = ZOrder,
            DamageEffect = DamageEffect,
            ConstructorAddress = ConstructorAddress,
            FootX = FootX,
            FootY = FootY,
            Group = Group,
            TechBitIndex = TechBitIndex,
            HotspotX = HotspotX,
            HotspotY = HotspotY,
            Name = Name,
            Description = Description,
            AssetName = AssetName,
            MinUsage = (float[])MinUsage.Clone(),
            MaxUsage = (float[])MaxUsage.Clone(),
            Level = Level,
            Theme = Theme,
            Kind = Kind,
            ContainerListFlags = ContainerListFlags,
            ContentListFlags = ContentListFlags,
            FromAsset = FromAsset,
        };
    }
}
