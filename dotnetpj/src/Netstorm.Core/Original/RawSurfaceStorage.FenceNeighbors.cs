// 썬 펜스의 실제 동맹 이웃 검색을 같은 장기 풀·해시·TYPE·사거리·동맹에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 BindFenceAlliances/FenceNeighbors 장기 본문을 이식한다.
// 0.588에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 이웃)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/Hash/TYPE으로 펜스 이웃을 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 Player의 실제 81 DWORD 읽기 함수다. 첫 이웃 본체 전에만 설치한다. 0.588에서 추가됐다.</summary>
    private Func<uint, uint, uint>? _fenceAlliances;

    /// <summary>같은 장기 사거리·동맹 getter를 공유하는 이웃 본체다. 0.588에서 추가됐다.</summary>
    private RawFenceNeighbors? _fenceNeighbors;

    /// <summary>첫 실제 이웃 검색 전에 실제 동맹 읽기 장치만 연결한다. 0.588에서 추가됐다.</summary>
    /// <param name="read">행 소유자에서 열 소유자로 읽는 실제 동맹 함수.</param>
    public void BindFenceAlliances(Func<uint, uint, uint> read)
    {
        ArgumentNullException.ThrowIfNull(read);
        // Native 전송 뒤, 첫 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _fenceNeighbors is not null || _fenceAlliances is not null)
        {
            throw new ArgumentException("장기 펜스 동맹의 실제 최초 장치 중복/누락", nameof(read));
        }

        _fenceAlliances = read;
    }

    /// <summary>같은 장기 사거리·동맹 getter를 사용하는 이웃 본체다. 0.588에서 추가됐다.</summary>
    public RawFenceNeighbors FenceNeighbors()
    {
        if (_fenceNeighbors is null)
        {
            // TYPE+44와 Player의 현재 81 DWORD를 원문 선택 위치에서만 읽는다.
            Func<uint, uint, uint> alliance = _fenceAlliances
                ?? throw new InvalidOperationException("장기 펜스의 선택된 실제 Player 동맹 미결합");
            _fenceNeighbors = new RawFenceNeighbors(
                Pool,
                Hash,
                Types,
                type =>
                {
                    if (type >= Types.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(type), "장기 펜스 현재 TYPE 사거리 범위 오류");
                    }

                    return Types[(int)type].Range;
                },
                (row, column) => alliance(row, column));
        }

        return _fenceNeighbors;
    }
}
