using System.Buffers.Binary;

// 같은 현재 슬롯과 TYPE 프레임으로 공장 파생의 가상 캡처를 실행한다.
namespace Netstorm.Core.Original;

/// <summary>10.78 Factory/Outpost의 기본 그림00454cd0과 추가 그림00454cf0이다. 근거: cpppj/src/o/RawFactoryDraw.cpp/.h.</summary>
public sealed class RawFactoryDraw
{
    /// <summary>매 호출의 가상 표·상태·현재 프레임을 읽는 같은 저장소다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>TYPE 번호로 직접 읽는 현재188개 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>슬롯을 읽기 전에10.78 판본과 실제 프레임 표 폭을 확인한다.</summary>
    public RawFactoryDraw(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(frames);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188)
            throw new ArgumentException("Factory 그림의10.78 실제 풀/프레임 표 필요");
        _pool = pool; _frames = frames;
    }

    /// <summary>타입 이름 대신 복원된 두 파생의 실제 가상 표로 분배한다.</summary>
    public bool Handles(int sid)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid));
        return table is 0x509148 or 0x509210;
    }

    /// <summary>가상 표·TYPE를 검사한다. 원본 몸체가 읽는 free/dead/void 조건을 선행 필터로 바꾸지 않는다.</summary>
    private ReadOnlySpan<byte> Raw(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (!Handles(sid) || raw[10] >= _frames.Count)
            throw new InvalidOperationException($"Factory 그림의 실제 가상 표/TYPE 미복원 SID={sid} TYPE={raw[10]} VT={BinaryPrimitives.ReadUInt32LittleEndian(raw)}");
        return raw;
    }

    /// <summary>상태 상위3비트가 고른A~H 구간의 첫 프레임을 반환한다. 없는 레벨은-1이다.</summary>
    public int MainFrame(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        return _frames[raw[10]].Run((byte)('A' + (raw[11] >> 5))).First;
    }

    /// <summary>추가 억제 비트와 dead를 검사한 뒤 기본과 다른 현재 그림만 실제 소비자에 보낸다.</summary>
    public void Additional(int sid, int x, int y, Action<int, int, uint, int>? draw)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        if ((raw[40] & 9) != 0 || (raw[11] & 2) != 0) return;
        int baseFrame = MainFrame(sid);
        int current = BinaryPrimitives.ReadInt32LittleEndian(Raw(sid).Slice(36, 4));
        if (current == baseFrame) return;
        if (draw is null) throw new InvalidOperationException("Factory 추가 그림0044c180 실제 소비자 미연결");
        draw(x, y, Raw(sid)[10], current);
    }
}
