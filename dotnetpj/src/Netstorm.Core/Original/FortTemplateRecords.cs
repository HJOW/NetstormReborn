namespace Netstorm.Core.Original;

/// <summary>내용물 목록의 한 항목이다(원본 FUN_004bd130 ↔ CD 00426dd0, cpppj <c>FortContent</c>). 읽은 뒤 바꾸지 않는 자료다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortContent
{
    /// <summary>파일에 적힌 타입 번호다.</summary>
    public byte StoredType { get; init; }

    /// <summary><c>TypeNames</c>로 바꾼 현재 타입 번호다.</summary>
    public int Type { get; init; }

    /// <summary>수량이다: saveQA면 1바이트(부호 없음), saveQB면 부호 있는 16비트, 둘 다 아니면 0.</summary>
    public int Quantity { get; init; }

    /// <summary>목록 플래그다: 타입 +0x100 → 그릇 타입 +0xfc → 파일의 1바이트 순서로 정한다.</summary>
    public uint ListFlags { get; init; }

    /// <summary>목록 플래그를 파일에서 읽었는가.</summary>
    public bool ListFlagsStored { get; init; }

    /// <summary>container 타입이면 중첩 내용물이다.</summary>
    public IReadOnlyList<FortContent> Contents { get; init; } = [];

    /// <summary>중첩 목록이 타입 0에서 끊기지 않았는가.</summary>
    public bool ContentsComplete { get; init; } = true;
}

/// <summary>
/// 내용물 목록 전체다. <see cref="Complete"/>가 거짓이면 변환한 타입이 0인 항목에서 읽기가 끊긴 것이다(원본 반환값 0).
/// </summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortContentList
{
    /// <summary>읽거나 생성한 항목의 순서 있는 목록이다.</summary>
    public IReadOnlyList<FortContent> Items { get; init; } = [];

    /// <summary>읽기가 끊기지 않고 끝났는가.</summary>
    public bool Complete { get; init; } = true;
}

/// <summary>청크 안의 오브젝트 레코드다(원본 FUN_004bdc60 ↔ CD 00428320, cpppj <c>FortObject</c>). 읽은 뒤 바꾸지 않는 자료다.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortObject
{
    /// <summary>소유자 바이트의 유효 범위 끝(플레이어 1~8)이다.</summary>
    private const int MaxOwner = 8;

    /// <summary>청크 안 위치다: 상위 4비트 x, 하위 4비트 y.</summary>
    public byte Position { get; init; }

    /// <summary>파일에 적힌 타입 번호다. 0xf6 이상이면 다리 약식 표기다.</summary>
    public byte StoredType { get; init; }

    /// <summary>현재 타입 번호다. 0이면 빈 레코드다.</summary>
    public int Type { get; init; }

    /// <summary>다리 약식 표기의 소유자다(0xff - 타입 바이트).</summary>
    public int? ShorthandOwner { get; init; }

    /// <summary>saveFrame: 프레임.</summary>
    public byte? Frame { get; init; }

    /// <summary>saveQA: 수량의 낮은 1바이트.</summary>
    public byte? QuantityByte { get; init; }

    /// <summary>saveQB: 수량 16비트.</summary>
    public short? QuantityWord { get; init; }

    /// <summary>bridge: 다리 클러스터 번호(프레임 자리에 쓴다).</summary>
    public byte? BridgeFrame { get; init; }

    /// <summary>버전 0에서만 있는 상태 1바이트다.</summary>
    public byte? LegacyState { get; init; }

    /// <summary>버전 2 이상, factory: 작업장 상태.</summary>
    public byte? FactoryState { get; init; }

    /// <summary>버전 1 이상, 소유자를 저장하는 타입: 파일의 값 그대로.</summary>
    public byte? Owner { get; init; }

    /// <summary>island 타입 뒤의 건너뛰는 1바이트다.</summary>
    public byte? IslandByte { get; init; }

    /// <summary>container: 내용물 목록이다.</summary>
    public FortContentList? Contents { get; init; }

    /// <summary>청크 안의 칸 x 좌표다.</summary>
    public int CellX => Position >> 4;

    /// <summary>청크 안의 칸 y 좌표다.</summary>
    public int CellY => Position & 0xf;

    /// <summary>원본이 소유자 바이트를 쓰는 값이다: 0이거나 8보다 크면 1.</summary>
    public int? NormalizedOwner() => Owner is null ? null : Owner == 0 || Owner > MaxOwner ? 1 : Owner;
}

/// <summary>청크 레코드다: 'c' + 오브젝트 수 + 오브젝트(원본 FUN_004be160 ↔ CD 004288a0).</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortChunk
{
    /// <summary>청크의 오브젝트 목록이다. 빈 레코드(타입 0)도 개수에 들어 있으므로 그대로 보존한다.</summary>
    public IReadOnlyList<FortObject> Objects { get; init; } = [];
}

/// <summary><c>Chaff</c>·<c>TerrNN</c> 섹션이다: 버전 1바이트 + 청크 레코드들.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortChunkSection
{
    /// <summary>섹션에 버전 바이트 말고도 내용이 있는가.</summary>
    public bool Present { get; init; }

    /// <summary>섹션 버전이다. 원본 파일은 2(구버전 1도 있다).</summary>
    public byte Version { get; init; }

    /// <summary>청크 목록이다.</summary>
    public IReadOnlyList<FortChunk> Chunks { get; init; } = [];
}

/// <summary><c>Deck</c> 항목이다(원본 FUN_004bf190 ↔ CD 00429670).</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortDeckEntry
{
    /// <summary>덱 항목에 저장된 타입 바이트다.</summary>
    public byte StoredType { get; init; }

    /// <summary>변환한 현재 타입 번호다. 0이면 원본은 덱에 넣지 않는다.</summary>
    public int Type { get; init; }

    /// <summary>추첨에서 사용하는 선택 가중치다.</summary>
    public byte Chance { get; init; }

    /// <summary>덱 항목에 저장된 파워 값이다(부호 있는 8비트).</summary>
    public sbyte Power { get; init; }

    /// <summary>남은 수량 또는 남은 수명이다.</summary>
    public byte Remaining { get; init; }
}

/// <summary><c>Subscriber</c> 섹션이다(원본 FUN_004bd450): 소유자 ID와 요새 이름.</summary>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public sealed class FortSubscriber
{
    /// <summary>소유자를 구별하는 식별 번호다.</summary>
    public uint Id { get; init; }

    /// <summary>요새 이름이다. 원본 바이트 그대로(문자 하나 = 바이트 하나, Latin1 사상)이며 화면에 쓸 때 변환한다.</summary>
    public string Name { get; init; } = "";
}
