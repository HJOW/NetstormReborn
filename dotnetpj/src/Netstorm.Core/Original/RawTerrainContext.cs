// 실제 초기 지형 constructor의 지도·문자 대응·청크/칸 bounds와 도장 앞뒤 문자 변환을 보관한다(cpppj o/RawTerrainContext.h/.cpp).
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.
namespace Netstorm.Core.Original;

/// <summary>
/// constructor가 읽는 원본 mutable 타입/모드 전역이다
/// (cpppj <c>o/RawTerrainContext.h</c>의 <c>TerrainContextGlobals</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainContextGlobals
{
    /// <summary>x/=/X 문자의 초기 지면 타입 번호다.</summary>
    public uint IslandType { get; set; } = 97u;

    /// <summary>X 문자의 초기 경계 타입 번호다.</summary>
    public uint EdgeType { get; set; } = 98u;

    /// <summary>초기 지역 BYTE 배열을 채우는 현재 지역 없음 값이다.</summary>
    public int NoRegion { get; set; } = 127;

    /// <summary>fringe 제외 flags 0x10을 켜는 현재 전투 화면 상태다.</summary>
    public bool Battle { get; set; }
}

/// <summary>
/// 패치 bounds 저장 함수의 두 assertion 원인이다. CD는 같은 초과에서 보고 없이 청크 값만 줄인다
/// (cpppj <c>o/RawTerrainContext.h</c>의 <c>TerrainContextAssertion</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public enum TerrainContextAssertion
{
    /// <summary>입력 right가 최대 청크 16보다 크다.</summary>
    RightChunkLimit,

    /// <summary>현재 저장한 bottom이 최대 청크 16보다 크다.</summary>
    BottomChunkLimit,
}

/// <summary>
/// 패치 004c0390 / CD 0049cdd0의 실제 기본 배열·특수 mapping/alias와 bounds를 소유하는 지형 컨텍스트다
/// (cpppj <c>o/RawTerrainContext</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class RawTerrainContext
{
    /// <summary>필드 폭과 패치/CD assertion 차이를 선택하는 판본이다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>생성 요청의 현재 플레이어다.</summary>
    private readonly uint _owner;

    /// <summary>생성 요청의 대상 지역이다.</summary>
    private readonly int _region;

    /// <summary>제한 후 청크 범위다.</summary>
    private ChunkRect _chunks;

    /// <summary>일반 0 / 군도 비영인 context의 성장·청크 지역 검사 선택값이다.</summary>
    private uint _archipelago;

    /// <summary>원본 X열 우선 65,536 BYTE 문자 지도다.</summary>
    private readonly byte[] _codes = new byte[65536];

    /// <summary>원본 X열 우선 65,536 BYTE 지역 지도다.</summary>
    private readonly byte[] _regions = new byte[65536];

    /// <summary>원본 문자별 256 DWORD 타입 대응표다.</summary>
    private readonly int[] _mapping = new int[256];

    /// <summary>원본 문자별 256 DWORD 문자 대응표다.</summary>
    private readonly int[] _aliases = new int[256];

    /// <summary>실제 저장소를 초기 RawTerrainStamp와 공유하는 context다.</summary>
    private readonly TerrainStampMap _map;

    /// <summary>패치가 bounds 오류를 보고한 뒤 계속 진행할 때 호출하는 필수 경계다.</summary>
    private readonly Action<TerrainContextAssertion> _diagnostic;

    /// <summary>
    /// 패치 004c0390 / CD 0049cdd0의 실제 기본 배열·특수 mapping/alias와 bounds를 만든다.
    /// </summary>
    /// <param name="edition">판본이다.</param>
    /// <param name="owner">생성 요청 소유자다.</param>
    /// <param name="region">대상 지역이다.</param>
    /// <param name="bounds">청크 범위다.</param>
    /// <param name="globals">전역 설정이다.</param>
    /// <param name="diagnostic">경계 진단 콜백이다.</param>
    public RawTerrainContext(
        OriginalEdition edition,
        uint owner,
        int region,
        ChunkRect bounds,
        TerrainContextGlobals globals,
        Action<TerrainContextAssertion> diagnostic)
    {
        ArgumentNullException.ThrowIfNull(globals);
        if (diagnostic is null)
        {
            throw new ArgumentException("초기 지형 context의 bounds 진단 연결 누락", nameof(diagnostic));
        }

        _edition = edition;
        _owner = owner;
        _region = region;
        _diagnostic = diagnostic;

        Array.Fill(_codes, (byte)'.');
        Array.Fill(_regions, (byte)globals.NoRegion);
        _mapping['.'] = 0;
        _mapping['x'] = (int)globals.IslandType;
        _mapping['X'] = (int)globals.EdgeType;
        _mapping['o'] = 0;
        _mapping['='] = (int)globals.IslandType;
        _aliases['x'] = 'x';
        _aliases['X'] = 'x';
        _aliases['o'] = 'x';
        _aliases['='] = 'x';

        _map = new TerrainStampMap
        {
            Codes = _codes,
            Regions = _regions,
            TypeMapping = _mapping,
            Aliases = _aliases,
            Bounds = new int[4],
            MakePredictable = 1u,
            LastSid = 0,
            ExcludedFlags = globals.Battle ? 0x10u : 0u,
        };
        if (_owner == 0)
        {
            _map.ExcludedFlags = 0x18u;
        }
        SetBounds(bounds);
    }

    /// <summary>초기 지면 성장/초기 stamp가 동일한 배열/context를 사용하게 한다.</summary>
    public TerrainStampMap Map => _map;

    /// <summary>원본 성장 몸체가 쓰는 X열 우선 문자 배열이다.</summary>
    public byte[] Codes => _codes;

    /// <summary>원본 성장 몸체가 쓰는 X열 우선 지역 배열이다.</summary>
    public byte[] Regions => _regions;

    /// <summary>원본 초기화/편집 helper가 같은 문자 대응 DWORD를 갱신할 때 사용한다.</summary>
    public int[] Aliases => _aliases;

    /// <summary>칸 bounds와 별도로 보존하는 제한 후 청크 범위다.</summary>
    public ChunkRect Chunks => _chunks;

    /// <summary>원본 context의 생성 요청 소유자다.</summary>
    public uint Owner => _owner;

    /// <summary>원본 context의 대상 지역이다.</summary>
    public int Region => _region;

    /// <summary>청크 getter/첫 시드의 패치 진단과 CD의 무진단 경계를 선택한다.</summary>
    public OriginalEdition Edition => _edition;

    /// <summary>군도 root가 켜는 원본 DWORD다.</summary>
    public uint ArchipelagoMode
    {
        get => _archipelago;
        set => _archipelago = value;
    }

    /// <summary>
    /// 칸 bounds를 먼저 16배 저장한 뒤 패치 진단/CD 조용한 청크 right/bottom 제한을 적용한다.
    /// </summary>
    /// <param name="bounds">설정할 청크 사각형이다.</param>
    public void SetBounds(ChunkRect bounds)
    {
        _chunks = bounds;
        static int Cells(int value) => unchecked((int)((uint)value << 4));
        _map.Bounds[0] = Cells(bounds.Left);
        _map.Bounds[1] = Cells(bounds.Top);
        _map.Bounds[2] = Cells(bounds.Right);
        _map.Bounds[3] = Cells(bounds.Bottom);
        if (_edition == OriginalEdition.Patch1078)
        {
            if (bounds.Right > 16)
            {
                _diagnostic(TerrainContextAssertion.RightChunkLimit);
            }
            if (_chunks.Bottom > 16)
            {
                _diagnostic(TerrainContextAssertion.BottomChunkLimit);
            }
        }
        if (_chunks.Right > 16)
        {
            _chunks.Right = 16;
        }
        if (_chunks.Bottom > 16)
        {
            _chunks.Bottom = 16;
        }
    }

    /// <summary>
    /// 004c09a0 / CD 0049d890: 여덟 이웃의 bounds·alias·지역 중 하나라도 다르면 현재 code를 edge로 바꾼다.
    /// </summary>
    /// <param name="code">검사할 문자 코드다.</param>
    /// <param name="edge">대체할 경계 문자다.</param>
    public void MarkEdges(int code, byte edge)
    {
        // 원본의 Y 외부 / X 내부 순서로 앞에서 바뀐 문자도 alias를 통해 후행 판정에 반영한다.
        for (int y = _map.Bounds[1]; y < _map.Bounds[3]; y++)
        {
            // 각 행의 현재 X 범위를 읽는다.
            for (int x = _map.Bounds[0]; x < _map.Bounds[2]; x++)
            {
                int index = Index(x, y);
                if ((sbyte)_codes[index] != code)
                {
                    continue;
                }
                // 북부터 시계 방향 실제 여덟 이웃에서 첫 불일치만 처리한다.
                foreach ((int dx, int dy) in TerrainBuilder.NeighborCells)
                {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx < _map.Bounds[0] || ny < _map.Bounds[1] || nx > _map.Bounds[2] || ny > _map.Bounds[3])
                    {
                        _codes[index] = edge;
                        break;
                    }
                    int neighbor = Index(nx, ny);
                    if (_aliases[Character(_codes[neighbor])] != (sbyte)_codes[index]
                        || _regions[neighbor] != _regions[index])
                    {
                        _codes[index] = edge;
                        break;
                    }
                }
            }
        }
    }

    /// <summary>
    /// 004c0ad0 / CD 0049d9a0: 도장 뒤 현재 =/o/X 문자를 x로 바꾸며 지역 BYTE는 보존한다.
    /// </summary>
    public void NormalizeAfterStamp()
    {
        // 도장의 같은 반열린 Y 외부 / X 내부 bounds를 순회한다.
        for (int y = _map.Bounds[1]; y < _map.Bounds[3]; y++)
        {
            // 원본의 현재 X 범위를 각 행에서 다시 읽는다.
            for (int x = _map.Bounds[0]; x < _map.Bounds[2]; x++)
            {
                int idx = Index(x, y);
                byte code = _codes[idx];
                if (code == (byte)'=' || code == (byte)'o' || code == (byte)'X')
                {
                    _codes[idx] = (byte)'x';
                }
            }
        }
    }

    /// <summary>저장 범위를 넘으면 원본 자료 손상으로 진단한다.</summary>
    private static int Index(int x, int y)
    {
        if (x < 0 || x >= 256 || y < 0 || y >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "초기 지형 context 지도 범위 오류");
        }
        return x * 256 + y;
    }

    /// <summary>signed char 음수 주소는 현재 ASCII 대응표의 정상 입력이 아니다.</summary>
    private static int Character(byte value)
    {
        if ((sbyte)value < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "초기 지형 context signed 문자 주소 범위 오류");
        }
        return value;
    }
}
