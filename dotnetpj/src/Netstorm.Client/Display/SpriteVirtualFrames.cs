using System.Buffers.Binary;
using Netstorm.Core.Original;

// 현재 객체 가상표의 +78을 실제 TYPE과 프레임 자료로 분배한다.
namespace Netstorm.Client.Display;

/// <summary>10.78의 기본·다리·공장·훨리베이스·썬더 캐논 프레임 조회다.</summary>
public sealed class SpriteVirtualFrames
{
    /// <summary>현재 객체를 읽는 실제 SID 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>파일 자산의 최초 TYPE 정의다.</summary>
    private readonly IReadOnlyList<RiftTypeDefinition> _definitions;
    /// <summary>TYPE 번호로 조회하는 실제 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>같은 풀과 프레임 표를 사용하는 공장 프레임 본체다.</summary>
    private readonly RawFactoryDraw _factory;

    /// <summary>같은 풀과 실제 TYPE·프레임 자료를 빌려 가상 조회기를 만든다.</summary>
    /// <param name="pool">10.78 객체 풀이다.</param>
    /// <param name="definitions">TYPE 70부터의 파일 자산 정의다.</param>
    /// <param name="frames">내장 TYPE을 포함하는 프레임 표다.</param>
    public SpriteVirtualFrames(RawSidPool pool, IReadOnlyList<RiftTypeDefinition> definitions, IReadOnlyList<RiftTypeFrames> frames)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(definitions);
        ArgumentNullException.ThrowIfNull(frames);
        _factory = new RawFactoryDraw(pool, frames);
        _pool = pool;
        _definitions = definitions;
        _frames = frames;
    }

    /// <summary>현재 슬롯의 실제 +78 함수에 따라 기본 프레임을 조회한다.</summary>
    /// <param name="sid">표시할 실제 객체의 SID다.</param>
    /// <returns>프레임 번호이며 -1은 호출자가 TYPE 기본 프레임으로 대체한다.</returns>
    /// <remarks>cpppj RawCampaignFactoryDisplay의 HitFrame과 10.78 파생 본체를 따른다. 미확인 가상표나 타입을 일반 프레임으로 대체하지 않는다.</remarks>
    public int Read(int sid)
    {
        uint address = SpriteVtableLookup.Read(_pool, sid).Frame;
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        int type = raw[10];
        switch (address)
        {
            case 0x004acfe0:
                int asset = type - 70;
                if (asset < 0 || asset >= _definitions.Count) throw new ArgumentOutOfRangeException(nameof(sid), "객체 기본 프레임 TYPE 범위 오류");
                return _definitions[asset].SpecialFrames.BaseFrame;
            case 0x004222f0:
                return BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
            case 0x00454cd0:
                return _factory.MainFrame(sid);
            case 0x0045b530:
                if (type != 72) throw new ArgumentException("훨리베이스 프레임 TYPE 불일치", nameof(sid));
                int current = BinaryPrimitives.ReadInt32LittleEndian(raw[36..]);
                if ((raw[40] & 9) != 0) return current;
                if (current is >= 20 and <= 26) return 10;
                return current is >= 11 and <= 19 ? current - 9 : -1;
            case 0x0046ae10:
                if (type != 131) throw new ArgumentException("썬더 캐논 프레임 TYPE 불일치", nameof(sid));
                uint frame = BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]);
                if (frame >= _frames[type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(sid), "썬더 캐논 현재 프레임 범위 오류");
                return _frames[type].Run(_frames[type].Codes[(int)frame].Side).First;
            default:
                throw new InvalidOperationException($"객체 가상 프레임 미복원: 0x{address:X8}");
        }
    }
}
