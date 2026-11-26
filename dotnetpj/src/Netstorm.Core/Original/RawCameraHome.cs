// 원문 초기 카메라 선택/중심 이동을 동일 raw 풀의 현재 슬롯과 화면에 연결한다.
using System.Buffers.Binary;
using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>원문 resetCamera와 camera.setCenter가 읽는 현재 모드·화면·갱신 전역이다.</summary>
public sealed class CameraHomeState
{
    /// <summary>PATCH540c70/CD50f6c8의 전체 DWORD이며 소유자 BYTE로 축소하지 않는다.</summary>
    public uint LocalPlayer { get; set; } = 1;
    /// <summary>원본 요새 모드의 현재 값이다.</summary>
    public bool Fort { get; set; }
    /// <summary>원본 전투 모드의 현재 값이다.</summary>
    public bool Battle { get; set; } = true;
    /// <summary>원본 Challenge 모드의 현재 값이다.</summary>
    public bool Challenge { get; set; }
    /// <summary>Challenge에서 찾는 현재 타입 번호다. 일반 캠페인은 읽지 않는다.</summary>
    public uint ChallengeType { get; set; }
    /// <summary>중심 이동 때 읽는 실제 화면 가로 크기다.</summary>
    public int Width { get; set; } = 640;
    /// <summary>중심 이동 때 읽는 실제 화면 세로 크기다.</summary>
    public int Height { get; set; } = 480;
    /// <summary>Challenge 빈 목록의 카메라 월드 가로 픽셀 크기다.</summary>
    public int WorldWidth { get; set; } = 4096;
    /// <summary>Challenge 빈 목록의 카메라 월드 세로 픽셀 크기다.</summary>
    public int WorldHeight { get; set; } = 2816;
    /// <summary>실제 억제 깊이를 읽고 쓴다.</summary>
    public uint SuppressionDepth { get; set; }
    /// <summary>Session의 요청 번호를 읽고 쓴다.</summary>
    public uint Requests { get; set; }
    /// <summary>Session의 변환 번호를 읽고 쓴다.</summary>
    public uint Revision { get; set; }
}

/// <summary>세션은 005c89b8을 실제 object loadingDepth와 공유하고 Carrier preview를 같은 Kernel에 예약한다.</summary>
public sealed class CameraHomeHooks
{
    /// <summary>실제 로딩/extra/타입 관문과 Regular491 생성이다.</summary>
    public Action<int>? ContentPreview { get; set; }
    /// <summary>현재 실제 005c89b8을 읽는다. 미지정이면 state의 독립 저장소를 사용한다.</summary>
    public Func<uint>? SuppressionDepth { get; set; }
    /// <summary>같은 실제 로딩 전역과 종속 모듈을 함께 갱신한다. getter와 쌍으로 제공한다.</summary>
    public Action<uint>? SetSuppressionDepth { get; set; }
}

/// <summary>대본 &lt;$ViewSpot,(XxY)&gt;가 고른 world 좌표다. CenterPoint와 같은 칸 단위 float이다.</summary>
/// <param name="X">가로 칸 좌표다.</param>
/// <param name="Y">세로 칸 좌표다.</param>
public readonly record struct ViewSpotPoint(float X, float Y);

/// <summary>원문 PATCH4340d0/CD488f50의 초기 대상과 같은 중심 이동 몸체다.</summary>
public sealed class RawCameraHome
{
    /// <summary>resetCamera가 선택하는 사제 genus다.</summary>
    private const uint PriestGenus = 0x200000;
    /// <summary>TYPE.flags2에서 일반/벌룬 Carrier를 함께 검사하는 genus다.</summary>
    private const uint CarrierGenus = 0x30000;
    /// <summary>원본 슬롯의 free 상태 비트다.</summary>
    private const byte Free = 1;
    /// <summary>원본 슬롯의 dead 상태 비트다.</summary>
    private const byte Dead = 2;
    /// <summary>보호 00542a58의 명령이며 이름 표 00543250의 ViewSpot이다. RunForIt [Init]이 입장 카메라에 쓴다.</summary>
    public const uint ViewSpotCommand = 0x020000d9U;

    /// <summary>원본 번호순 조회와 free/dead/좌표를 공유하는 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 TYPE의 genus와 중심 이동용 발자국 필드다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>매 조회의 모드/소유자/화면 크기와 변경 카운터를 읽고 쓴다.</summary>
    private readonly CameraHomeState _state;
    /// <summary>공유 카운터/목표/실제 497960을 수행하는 world float 부모다.</summary>
    private readonly Action<float, float> _center;
    /// <summary>현재 실제 로딩 전역과 같은 Carrier preview의 외부 경계다.</summary>
    private readonly CameraHomeHooks _hooks;

    /// <summary>풀/타입/상태는 호출 동안 살아 있어야 한다. center는 world float를 받는 433aa0 전체 부모이며 픽셀 setter로 대체하지 않는다.</summary>
    /// <param name="pool">실제 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="state">현재 상태다.</param>
    /// <param name="center">중심 이동 부모다.</param>
    /// <param name="hooks">로딩 전역·Carrier preview 갈고리다.</param>
    public RawCameraHome(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CameraHomeState state, Action<float, float> center, CameraHomeHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(center);
        if (types.Count == 0 || state.Width <= 0 || state.Height <= 0 || pool.Capacity > 65536 || (hooks?.SuppressionDepth is null) != (hooks?.SetSuppressionDepth is null))
        {
            throw new ArgumentException("초기 카메라의 실제 풀/타입/화면 소비자 범위 오류");
        }
        _pool = pool;
        _types = types;
        _state = state;
        _center = center;
        _hooks = hooks ?? new();
    }

    /// <summary>원문433ad0의 인자 해석이다. 맨 앞 '(' 한 글자만 건너뛰고 'x'와 ')'로 나눈 두 토큰을 atol한다. 숫자가 아닌 필드는 atol처럼 0이다.</summary>
    /// <param name="text">대본 인자다.</param>
    /// <returns>칸 단위 world 좌표다.</returns>
    public static ViewSpotPoint ParseViewSpot(string text)
    {
        text ??= "";
        RawTextTokenResult first = RawTextToken.Read(text, text.Length != 0 && text[0] == '(' ? 1 : 0, "x");
        RawTextTokenResult second = RawTextToken.Read(text, first.Next, ")");
        return new((float)ConfigNumbers.ParseLong(first.Text), (float)ConfigNumbers.ParseLong(second.Text));
    }

    /// <summary>원본 슬롯의 정렬되지 않은 좌표 DWORD를 직접 읽는다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>단정도 좌표다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadSingleLittleEndian(raw[offset..]);

    /// <summary>외부 화면 효과가 로딩 깊이를 바꿨으면 예전 사본이 아니라 현재 실제 값을 읽는다.</summary>
    /// <returns>현재 억제 깊이다.</returns>
    private uint SuppressionDepth()
    {
        if (_hooks.SuppressionDepth is not null)
        {
            _state.SuppressionDepth = _hooks.SuppressionDepth();
        }
        return _state.SuppressionDepth;
    }

    /// <summary>실제 소유자 갱신을 먼저 끝내고 현재 state에도 원문의 깊이를 반영한다.</summary>
    /// <param name="depth">쓸 깊이다.</param>
    private void SetSuppressionDepth(uint depth)
    {
        _hooks.SetSuppressionDepth?.Invoke(depth);
        _state.SuppressionDepth = depth;
    }

    /// <summary>선택한 world float를 433aa0에 그대로 전달한다. 미니맵의 직접 요청과 달리 현재 카메라도 즉시 적용한다.</summary>
    /// <param name="x">가로 칸 좌표다.</param>
    /// <param name="y">세로 칸 좌표다.</param>
    public void CenterPoint(float x, float y) => _center(x, y);

    /// <summary>실제 SID의 현재 좌표/TYPE footX/footY로 중심을 옮긴 뒤 현재 Carrier genus의 preview를 요청한다.</summary>
    /// <param name="sid">중심 SID다.</param>
    public void CenterOn(int sid)
    {
        if (sid == 0)
        {
            return;
        }
        // 선택된 같은 슬롯을 중심 이동 직전에 다시 읽는다
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if ((raw[11] & (Free | Dead)) != 0)
        {
            return;
        }
        if (raw[10] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "초기 카메라의 현재 TYPE 누락");
        }
        OriginalTypeRecord type = _types[raw[10]];
        // 원본은 전체 footX를 빼며 발자국 절반을 쓰지 않는다
        float x = (float)((double)Coordinate(raw, 14) - type.FootX);
        // 원본은 계산 뒤 float 저장을 거친다
        float y = (float)((double)Coordinate(raw, 18) - type.FootY);
        CenterPoint(x, y);
        // 화면 적용 뒤 원문이 읽는 현재 타입을 다시 조회한다
        int currentType = _pool.Slot(sid)[10];
        if (currentType >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "중심 이동 뒤 현재 TYPE 누락");
        }
        if ((_types[currentType].Flags2 & CarrierGenus) != 0)
        {
            if (_hooks.ContentPreview is not null)
            {
                _hooks.ContentPreview(sid);
            }
            else if (SuppressionDepth() == 0)
            {
                throw new InvalidOperationException("카메라 Carrier의 실제 내용물 표시 미연결");
            }
        }
    }

    /// <summary>free만 건너뛰며 마지막 내 사제를 고른다. dead 후보의 CenterOn 자연 반환도 유지한다.</summary>
    public void Reset()
    {
        int selected = 0;
        float sumX = 0, sumY = 0, count = 0;
        float challengeX = 0, challengeY = 0;
        bool foundChallenge = false;
        uint ownerOffset = _pool.Edition == OriginalEdition.Patch1078 ? 34u : 32u;
        // free/dead/void를 합쳐 걸러내지 않고 원문과 같은 SID 순서를 유지한다
        // 원본 반복자는 2부터 시작하며 free 비트만 보고 다음 현재 번호로 간다
        for (int number = 2; number < _pool.Capacity; number++)
        {
            ReadOnlySpan<byte> raw = _pool.Slot(number);
            if ((raw[11] & Free) != 0)
            {
                continue;
            }
            if (raw[10] >= _types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(_pool), "초기 카메라 순회의 TYPE 범위 오류");
            }
            if ((_state.Fort || _state.Battle) && raw[(int)ownerOffset] == _state.LocalPlayer && (_types[raw[10]].Flags2 & PriestGenus) != 0)
            {
                selected = number;
                sumX += Coordinate(raw, 14);
                sumY += Coordinate(raw, 18);
                count += 1.0f;
            }
            if (_state.Challenge && raw[10] == _state.ChallengeType)
            {
                selected = number;
                if (raw[(int)ownerOffset] == _state.LocalPlayer)
                {
                    challengeX = Coordinate(raw, 14);
                    challengeY = Coordinate(raw, 18);
                    foundChallenge = true;
                    break;
                }
            }
        }
        // 이 원문 분기는 좌표를 쓰지 않은 스택을 읽는다. 정의된 미션 초기화 경로와 구별해 실행을 거부한다.
        if (_state.Challenge && selected != 0 && !foundChallenge)
        {
            throw new InvalidOperationException("원본 Challenge 초기 카메라의 미초기화 좌표 경로");
        }
        // 실제 object loadingDepth와 같은 현재 전역을 증가 직전에 읽는다
        uint depth = SuppressionDepth();
        if (depth == uint.MaxValue)
        {
            throw new OverflowException("카메라 억제 깊이 감김의 외부 표시 생성 경로 미연결");
        }
        SetSuppressionDepth(depth + 1);
        try
        {
            if (_state.Challenge)
            {
                if (selected == 0)
                {
                    challengeX = (float)((double)_state.WorldWidth * 0.5 * 0.0625f);
                    challengeY = (float)((double)_state.WorldHeight * 0.5 * 0.09090909361839294f);
                }
                CenterPoint(challengeX, challengeY);
            }
            else if (selected != 0)
            {
                CenterOn(selected);
            }
            else if (count > 1.0f)
            {
                CenterPoint(sumX * (1.0f / count), sumY * (1.0f / count));
            }
            else
            {
                CenterPoint(128.0f, 128.0f);
            }
        }
        catch
        {
            SetSuppressionDepth(SuppressionDepth() - 1u);
            throw;
        }
        SetSuppressionDepth(SuppressionDepth() - 1u);
    }
}
