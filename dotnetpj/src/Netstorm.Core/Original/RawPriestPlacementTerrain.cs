// 일반 사제 배치의 후보별 다리·섬·지역 배열과 모양 종료 효과를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 사제 배치 지형 모듈을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// 사제 배치 검사 지형 처리 중 조회·갱신하는 상태 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class PriestPlacementTerrainState
{
    /// <summary>원본 열 간격 12의 지역 배열이다. 후보 쓰기 범위는 8×8이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public byte[] Regions { get; } = new byte[144];

    /// <summary>현재 지역 빈 칸 값과 무인도 타입 전역이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint EmptyRegion { get; set; } = 127u;

    /// <summary>무인도 표면 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint NoIslandType { get; set; }

    /// <summary>다리 겹침 여부다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool BridgeOverlap { get; set; }

    /// <summary>무인도 전용 여부다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool NoIslandOnly { get; set; } = true;

    /// <summary>지면 완료 여부다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool GroundComplete { get; set; } = true;

    /// <summary>소유권·지역 관계에 따른 배치 허용 판정이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool Permission { get; set; } = true;

    /// <summary>가능 배치 지면 여부다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool CanPlaceGround { get; set; }
}

// 일반 사제 배치의 후보별 다리·섬·지역 배열과 모양 종료 효과를 복원한 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class RawPriestPlacementTerrain
{
    /// <summary>원본 지역 배열의 열 간격이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Pitch = 12;

    /// <summary>후보 쓰기의 제한 폭이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int PaintLimit = 8;

    /// <summary>같은 풀을 사용하는 모양·충돌 어댑터만 연결한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·발자국을 조회하는 타입 표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 프레임 자료다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType> _frames;

    /// <summary>현재 섬의 배치·소유권 자료다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _islands;

    /// <summary>처리 중 조회·갱신하는 상태다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly PriestPlacementTerrainState _state;

    /// <summary>좌표 계산의 기준 가로 원점이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private long _originX;

    /// <summary>좌표 계산의 기준 세로 원점이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private long _originY;

    /// <summary>검사할 지면 칸을 고르는 비트 마스크다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private uint _groundMask;

    /// <summary>활성 타입이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private uint _activeType;

    /// <summary>현재 입력·실행 대상이 활성 상태인지다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private bool _active;

    /// <summary>모양 준비 상태인지다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private bool _shape;

    /// <summary>
    /// 지형 번호 지도는 미리보기의 표면 번호 지도와 구분하여 256×256으로 제공한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="islands">섬 배치 지도 65,536칸이다.</param>
    /// <param name="state">처리 중 상태다.</param>
    /// <exception cref="ArgumentException">사제 지형 자료 크기 오류다.</exception>
    public RawPriestPlacementTerrain(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, IReadOnlyList<ushort> islands, PriestPlacementTerrainState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(state);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || frames.Count != types.Count || islands.Count != 65536)
        {
            throw new ArgumentException("사제 지형 자료 크기 오류");
        }

        _pool = pool;
        _types = types;
        _frames = frames;
        _islands = islands;
        _state = state;
    }

    /// <summary>연결 검증용 풀을 반환한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>이 모듈이 쓰는 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 일반 사제의 초기 권한과 모양 사이에 유지하는 누적 상태를 초기화한다. 지역 배열은 유지한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    public void Begin(PriestPlacementQuery query)
    {
        Placing(query);
        _active = true;
        _shape = false;
        _activeType = query.Type;
        _state.BridgeOverlap = false;
        _state.NoIslandOnly = true;
        _state.GroundComplete = true;
        _state.Permission = true;
        _state.CanPlaceGround = false;
    }

    /// <summary>
    /// 탐색 직전에 배열을 빈 칸 값으로 채우고 발자국 원점·지면 마스크를 잡는다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <param name="x">절삭할 가로 좌표다.</param>
    /// <param name="y">절삭할 세로 좌표다.</param>
    public void BeginShape(PriestPlacementQuery query, float x, float y)
    {
        Require(query, false);
        OriginalTypeRecord type = Placing(query);
        _originX = (long)Truncate(x) - type.FootX + 1;
        _originY = (long)Truncate(y) - type.FootY + 1;
        _groundMask = type.Flags1 & 6u;
        Array.Fill(_state.Regions, (byte)_state.EmptyRegion);
        _shape = true;
    }

    /// <summary>
    /// 충돌 검사를 통과한 모든 후보에 적용한다. 무시된 후보도 다리·섬 효과를 받는다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <param name="candidate">후보 번호다.</param>
    public void Candidate(PriestPlacementQuery query, int candidate)
    {
        Require(query, true);
        OriginalTypeRecord placing = Placing(query);
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        if (raw[10] >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "사제 지형 후보 타입 오류");
        }

        OriginalTypeRecord other = _types[raw[10]];
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if ((other.Flags2 & 4u) != 0u && (placing.Flags2 & 0x208100u) != 0u && (raw[patch ? 40 : 35] & 1u) == 0u)
        {
            if (placing.FootX != placing.FootY)
            {
                throw new ArgumentException("다리 위 사제 발자국 폭·높이 불일치");
            }

            _state.BridgeOverlap = true;
        }

        if ((other.Flags2 & 2u) == 0u)
        {
            return;
        }

        if (raw[10] != _state.NoIslandType)
        {
            _state.NoIslandOnly = false;
        }

        int frame = patch ? BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(36, 4)) : (int)raw[34];
        IReadOnlyList<RiftFrameCode> codes = _frames[raw[10]].Frames.Codes;
        if (frame < 0 || frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "사제 지형 후보 프레임 오류");
        }

        uint surface = (codes[frame].Flags & 4u) != 0u ? 4u : 2u;
        if ((_groundMask & surface) == 0u)
        {
            return;
        }

        int x = Truncate(BitConverter.UInt32BitsToSingle(Read(raw, 14)));
        int y = Truncate(BitConverter.UInt32BitsToSingle(Read(raw, 18)));
        byte region = (byte)RegionAt(x, y);
        // 배열 밖의 반복은 쓰기 효과가 없으므로 8×8 교집합만 순서대로 방문하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        long left = Math.Max(0L, (long)x - other.FootX - _originX + 1);
        long top = Math.Max(0L, (long)y - other.FootY - _originY + 1);
        long right = Math.Min(PaintLimit - 1L, (long)x - _originX);
        long bottom = Math.Min(PaintLimit - 1L, (long)y - _originY);
        // 원본 가로 바깥·세로 안쪽 순서와 열 간격 12를 보존하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (long cx = left; cx <= right; ++cx)
        {
            // 해당 열의 지역 바이트만 덮어쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (long cy = top; cy <= bottom; ++cy)
            {
                _state.Regions[(int)(cx * Pitch + cy)] = region;
            }
        }
    }

    /// <summary>
    /// 첫 부호 바이트와 빈 칸 값 비교 및 발자국 전체의 동일 지역 여부를 누적한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <returns>항상 참이다.</returns>
    public bool EndShape(PriestPlacementQuery query)
    {
        Require(query, true);
        OriginalTypeRecord type = Placing(query);
        byte first = _state.Regions[0];
        if (SignedByte(first) == _state.EmptyRegion)
        {
            _state.GroundComplete = false;
        }
        else
        {
            // 배치 발자국 전체를 검사하며 후보의 8칸 쓰기 제한과 구분하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int x = 0; x < type.FootX; ++x)
            {
                // 각 열의 바이트 지역 번호가 첫 번호와 같은지 검사하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
                for (int y = 0; y < type.FootY; ++y)
                {
                    if (_state.Regions[x * Pitch + y] != first)
                    {
                        _state.GroundComplete = false;
                    }
                }
            }
        }

        _shape = false;
        return true;
    }

    /// <summary>
    /// 일반 사제는 최종 지면·관계 조건의 거부를 우회하지만 해당 계산 효과는 보존한다.
    /// </summary>
    /// <param name="query">배치 조회다.</param>
    /// <returns>항상 참이다.</returns>
    public bool Finish(PriestPlacementQuery query)
    {
        Require(query, false);
        OriginalTypeRecord type = Placing(query);
        _state.CanPlaceGround = ((type.Flags1 & 0x400u) != 0u && _state.NoIslandOnly) || _state.GroundComplete || _state.BridgeOverlap;
        if ((type.Flags1 & 0x400u) != 0u && !_state.NoIslandOnly && !_state.GroundComplete)
        {
            _state.Permission = false;
        }

        _active = false;
        return true;
    }

    /// <summary>
    /// 실제 지역 얻기다. 지도 번호 0·범위 밖은 127, 다리는 현재 빈 칸 값이다.
    /// </summary>
    /// <param name="x">격자 가로 좌표다.</param>
    /// <param name="y">격자 세로 좌표다.</param>
    /// <returns>지역 번호다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">지역 지도 타입 오류다.</exception>
    /// <exception cref="ArgumentException">지역 지도 자산 분류 오류다.</exception>
    public uint RegionAt(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256)
        {
            return 127u;
        }

        int sid = _islands[y * 256 + x];
        if (sid == 0)
        {
            return 127u;
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "지역 지도 타입 오류");
        }

        uint genus = _types[raw[10]].Flags2;
        if ((genus & 0x1000006u) == 0u)
        {
            if (_pool.Edition == OriginalEdition.Patch1078)
            {
                throw new ArgumentException("지역 지도 자산 분류 오류");
            }

            return _state.EmptyRegion;
        }

        return (genus & 4u) != 0u ? _state.EmptyRegion : Read(raw, 8, 2);
    }

    /// <summary>다리·특수 지역 제외 타입 조합과 배열 밖 발자국은 지원 범위 밖으로 진단한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="query">배치 조회다.</param>
    /// <returns>배치 중인 타입 기록이다.</returns>
    /// <exception cref="ArgumentException">일반 사제 지형 타입 계약 오류다.</exception>
    /// <exception cref="ArgumentOutOfRangeException">사제 지형 타입·발자국 범위 오류다.</exception>
    private OriginalTypeRecord Placing(PriestPlacementQuery query)
    {
        if (query.Type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(query), "사제 지형 타입 오류");
        }

        OriginalTypeRecord type = _types[(int)query.Type];
        if ((type.Flags2 & 0x200000u) == 0u || (type.Flags2 & 4u) != 0u || (_pool.Edition == OriginalEdition.Patch1078 && (type.Flags2 & 0x200u) != 0u))
        {
            throw new ArgumentException("일반 사제 지형 타입 계약 오류");
        }

        if (type.FootX < 1 || type.FootX > Pitch || type.FootY < 1 || type.FootY > Pitch)
        {
            throw new ArgumentOutOfRangeException(nameof(query), "사제 지형 발자국 배열 범위 오류");
        }

        return type;
    }

    /// <summary>순회 시작 및 모양 시작 순서를 확인한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="query">배치 조회다.</param>
    /// <param name="shape">모양 단계면 참이다.</param>
    /// <exception cref="InvalidOperationException">사제 지형 순회 순서 오류다.</exception>
    private void Require(PriestPlacementQuery query, bool shape)
    {
        if (!_active || query.Type != _activeType || (shape && !_shape))
        {
            throw new InvalidOperationException("사제 지형 순회 순서 오류");
        }
    }

    /// <summary>부호 있는 CRT 절삭을 안전한 정수 범위에서 수행한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>절삭한 정수다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">사제 지형 좌표 절삭 오류다.</exception>
    private static int Truncate(float value)
    {
        double result = Math.Truncate((double)value);
        if (!double.IsFinite(result) || result < int.MinValue || result > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "사제 지형 좌표 절삭 오류");
        }

        return (int)result;
    }

    /// <summary>비정렬 바이트를 리틀 엔디언으로 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint result = 0u;
        // 지정한 폭의 낮은 바이트부터 합치는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            result |= (uint)raw[offset + i] << (8 * i);
        }

        return result;
    }

    /// <summary>원본 movsx 바이트와 DWORD 비교의 비트값을 보존한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="value">바이트 값이다.</param>
    /// <returns>부호 확장한 비트값이다.</returns>
    private static uint SignedByte(byte value) => unchecked((uint)(int)(sbyte)value);
}
