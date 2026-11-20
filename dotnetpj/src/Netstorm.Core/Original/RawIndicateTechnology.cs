using System.Buffers.Binary;

// 현재 Player 기술 부모와 실제 내용물 검색/생성으로 기술 표시를 추가한다.
namespace Netstorm.Core.Original;

/// <summary>004929e0의 현재 기술 부모→kind WORD 조회→실제 내용물 생성 사슬이다.</summary>
/// <remarks>근거: cpppj/src/o/RawProductionDeck.cpp/.h의 RawIndicateTechnology다.</remarks>
public sealed class RawIndicateTechnology
{
    /// <summary>현재 PlayerKnown·battleOwner와 기술 부모를 제공한다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>같은 실제 풀의 원문 종류/mask 검색기다.</summary>
    private readonly RawCarrierPreDestroy _carrier;
    /// <summary>같은 실제 공장과 내용물 부착기를 사용하는 생성기다.</summary>
    private readonly RawContainedCreate _create;
    /// <summary>세 실제 장치가 같은 풀을 빌리는지 확인하며 게임 효과는 실행하지 않는다.</summary>
    public RawIndicateTechnology(RawPlayerWorld player, RawCarrierPreDestroy carrier, RawContainedCreate create)
    {
        ArgumentNullException.ThrowIfNull(player); ArgumentNullException.ThrowIfNull(carrier); ArgumentNullException.ThrowIfNull(create);
        if (!ReferenceEquals(player.Pool(), carrier.Pool) || !ReferenceEquals(player.Pool(), create.Pool))
            throw new ArgumentException("기술 표시의 실제 Player/Carrier/생성 풀이 서로 다릅니다");
        _player = player; _carrier = carrier; _create = create;
    }
    /// <summary>같은 월드 조립을 확인하는 실제 풀이다.</summary>
    public RawSidPool Pool => _player.Pool();
    /// <summary>실제 getter의 전체 DWORD SID를 WORD로 미리 자르지 않고 물리 범위만 확인한다.</summary>
    private int Parent()
    {
        uint parent = _player.CurrentTechnology();
        return parent > ushort.MaxValue || parent >= Pool.Capacity
            ? throw new ArgumentOutOfRangeException(nameof(parent), "기술 표시의 현재 Technology SID 범위 오류") : (int)parent;
    }
    /// <summary>검색 결과의 저장 kind WORD가0이면 현재 부모를 다시 읽고 기술 내용물을 실제 생성한다.</summary>
    public void Indicate(uint type)
    {
        int found = _carrier.FindContained(Parent(), 4, type, 0);
        uint kind = found != 0 ? BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(found)[18..]) : 0u;
        if (kind == 0) _ = _create.Create(Parent(), type, 0, 4, 0x10, 0);
    }
}
