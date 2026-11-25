// 사제 AI의 실제 574 배치 탐색 상태/행별 예산/가장 먼 합법 후보를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 배치 탐색 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.

// 원본 0040f660의 네 float 경계이며 별도 clamp나 inclusive 변환을 하지 않는다.
public struct AiBuildSearchBounds
{
    /// <summary>왼쪽 경계다. 현재 footprint를 더하고 절삭한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public float Left;

    /// <summary>위 경계다. 현재 footprint를 더하고 절삭한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public float Top;

    /// <summary>오른쪽 경계다. 그대로 절삭한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public float Right;

    /// <summary>아래 경계다. 그대로 절삭한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public float Bottom;

    /// <summary>네 float 경계를 묶는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="left">왼쪽 경계다.</param>
    /// <param name="top">위 경계다.</param>
    /// <param name="right">오른쪽 경계다.</param>
    /// <param name="bottom">아래 경계다.</param>
    public AiBuildSearchBounds(float left, float top, float right, float bottom)
    {
        Left = left;
        Top = top;
        Right = right;
        Bottom = bottom;
    }
}

// 게임 시간과 다른 실제 벽시각과 전체 배치 검사를 빌린다.
public sealed class AiBuildSearchHooks
{
    /// <summary>00460d70의 매 호출 현재 벽시각이다. 프레임 snapshot으로 대신하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public Func<double>? Wall { get; set; }

    /// <summary>0049b510 전체이며 cached TYPE/argument 0/flags 0/owner BYTE/mode 1이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }
}

// 별도 좌표 목록 없이 같은 0xb08의 574..5a7와 실제 65536 spot/현재 188 TYPE를 사용한다.
// 근거: cpppj o/RawAiBuildSearch.h/.cpp의 RawAiBuildSearch다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(footX/footY만 쓴다).
public sealed class RawAiBuildSearch
{
    /// <summary>실제 본체 안의 0040f660/0040f700 상태 시작이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private const int StateOffset = 0x574;

    /// <summary>0049a840의 실제 TYPE 레코드 포인터 산술 stride다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private const uint TypeStride = 500;

    /// <summary>PE00501230의 정확한 double 값 0.029999999329447746이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private static readonly double WorkBudget = (double)0.03f;

    /// <summary>0040f660이 쓰는 미정 x/y -10000.0f 비트다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private const uint Unset = 0xc61c4000u;

    /// <summary>다른 AI와 같은 본체/목록 저장소다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>재조회/캐시 포인터가 같은 실제 현재 표를 본다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>월드의 같은 65536 BYTE spot이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private readonly byte[] _spots;

    /// <summary>호스트 포인터 절단 없이 500 stride를 보존하는 opaque32 TYPE 기준이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private readonly uint _typeBase;

    /// <summary>선택된 실제 벽시각/전체 배치 효과다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private readonly AiBuildSearchHooks _hooks;

    /// <summary>생성은 시계나 배치 검사를 실행하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="memory">다른 AI와 같은 본체/목록 저장소다.</param>
    /// <param name="types">재조회/캐시 포인터가 같은 실제 현재 표를 본다.</param>
    /// <param name="spots">월드의 같은 65536 BYTE spot이다.</param>
    /// <param name="typeBase">호스트 포인터 절단 없이 500 stride를 보존하는 opaque32 TYPE 기준이다.</param>
    /// <param name="hooks">선택된 실제 벽시각/전체 배치 효과다.</param>
    public RawAiBuildSearch(RawAiBrainMemory memory, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, uint typeBase, AiBuildSearchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != 188 || spots.Length != 65536 || typeBase == 0 || typeBase > uint.MaxValue - 187u * TypeStride)
        {
            throw new ArgumentException("AI 배치 탐색 실제188TYPE/65536spot/TYPE핸들 누락");
        }

        _memory = memory;
        _types = types;
        _spots = spots;
        _typeBase = typeBase;
        _hooks = hooks;
    }

    /// <summary>해제/다른 크기 본체를 거부한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 배치 탐색 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 원문 DWORD를 물리 범위 안에서 읽는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, long at)
    {
        if (at < 0 || (ulong)at + 4 > (ulong)raw.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 탐색 DWORD 범위");
        }

        // 지정 네 BYTE만 합치는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan((int)at, 4));
    }

    /// <summary>지정 DWORD만 바꾸고 주변/미초기화 상태를 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] raw, long at, uint value)
    {
        if (at < 0 || (ulong)at + 4 > (ulong)raw.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 배치 탐색 쓰기 범위");
        }

        // 네 BYTE만 쓰는 반복문이다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan((int)at, 4), value);
    }

    /// <summary>원본 DWORD의 signed 비교 해석이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 DWORD 덧셈/뺄셈의 감김을 signed 값으로 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="first">첫 값이다.</param>
    /// <param name="second">더할 DWORD다.</param>
    /// <returns>감김 signed 합이다.</returns>
    private static int Add(int first, uint second) => Signed(unchecked((uint)first) + second);

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD이며 무효 FISTP의 하위 값은 0이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD다.</returns>
    private static uint Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)Math.Truncate(value));
    }

    /// <summary>선택되지 않은 하위는 요구하지 않고 선택된 누락만 명시 오류로 처리한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 배치 탐색 실제 하위 미연결");

    /// <summary>원본 상대 필드를 현재 본체에서 읽는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">상태 상대 위치다.</param>
    /// <returns>현재 상태 DWORD다.</returns>
    private uint Field(uint token, long at) => Read(Body(token), StateOffset + at);

    /// <summary>해당 상대 필드만 바꾼다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">상태 상대 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, long at, uint value) => Put(Body(token), StateOffset + at, value);

    /// <summary>TYPE를 별도 사본으로 캐시하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(uint type)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 배치 탐색 현재TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>실제 표와 500 stride의 안정된 opaque32 대응이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="type">바꿀 TYPE 번호다.</param>
    /// <returns>opaque32 TYPE 포인터다.</returns>
    private uint Handle(uint type)
    {
        _ = Type(type);
        return unchecked(_typeBase + type * TypeStride);
    }

    /// <summary>손상된 캐시를 원래 TYPE나 다른 기본값으로 보정하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="pointer">역해석할 opaque32 핸들이다.</param>
    /// <returns>현재 실제 표 index다.</returns>
    private uint TypeFromHandle(uint pointer)
    {
        if (pointer < _typeBase || (pointer - _typeBase) % TypeStride != 0)
        {
            throw new ArgumentOutOfRangeException(nameof(pointer), "AI 배치 탐색 cached TYPE핸들 범위");
        }

        uint type = (pointer - _typeBase) / TypeStride;
        _ = Type(type);
        return type;
    }

    /// <summary>원본 지도 좌표를 clamp하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="x">읽을 가로 칸이다.</param>
    /// <param name="y">읽을 세로 칸이다.</param>
    /// <returns>실제 row-major spot이다.</returns>
    private byte Spot(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "AI 배치 탐색 실제spot 주소 범위");
        }

        return _spots[(long)y * 256 + x];
    }

    /// <summary>원본 여덟 인자와 두 TYPE 조회/상태 쓰기 순서다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="type">탐색할 TYPE 번호다.</param>
    /// <param name="bounds">탐색할 네 float 경계다.</param>
    /// <param name="avoid">피할 좌표다.</param>
    /// <param name="owner">적을 소유자다.</param>
    public void Initialize(uint token, uint type, AiBuildSearchBounds bounds, RawPathPoint avoid, uint owner)
    {
        Field(token, 0, type);
        Field(token, 0x18, Handle(type));
        Field(token, 0x10, BitConverter.SingleToUInt32Bits(avoid.X));
        Field(token, 0x14, BitConverter.SingleToUInt32Bits(avoid.Y));
        Field(token, 0x1c, owner);
        Field(token, 4, 0);
        Field(token, 0xc, Unset);
        Field(token, 8, Unset);
        OriginalTypeRecord current = Type(Field(token, 0));
        int width = current.FootX;
        int height = current.FootY;
        Field(token, 0x20, Ftol((double)width + bounds.Left));
        uint top = Ftol((double)height + bounds.Top);
        Field(token, 0x24, top);
        Field(token, 0x28, Ftol(bounds.Right));
        uint bottom = Ftol(bounds.Bottom);
        Field(token, 0x30, top);
        Field(token, 0x2c, bottom);
    }

    /// <summary>원본은 x를 바깥쪽에 두며 0 이하 footprint는 칸에 접근하지 않고 성공할 수 있다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="type">검사할 TYPE 번호다.</param>
    /// <param name="x">검사할 가로 칸이다.</param>
    /// <param name="y">검사할 세로 칸이다.</param>
    /// <returns>footprint 전체가 비었으면 참이다.</returns>
    public bool FreeFootprint(uint type, int x, int y)
    {
        OriginalTypeRecord current = Type(type);
        int column = Add(x, unchecked(0u - (uint)current.FootX));
        int firstRow = Add(Add(y, unchecked(0u - (uint)current.FootY)), 1u);
        // 각 x 열에서 y 시작을 다시 사용하며 최초 실패 칸에서 즉시 종료하는 반복문이다.
        for (;;)
        {
            column = Add(column, 1u);
            if (column > x)
            {
                return true;
            }

            if (firstRow > y)
            {
                continue;
            }

            int row = firstRow;
            // 안쪽 반복문은 원본 inclusive y footprint를 순회한다.
            do
            {
                if ((Spot(column, row) & 0x3fu) != 2u)
                {
                    return false;
                }

                row = Add(row, 1u);
            }
            while (row <= y);
        }
    }

    /// <summary>각 호출의 deadline과 행 시작 clock만 사용하며 한 행은 예산을 넘어도 끝까지 처리한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="budgeted">0은 전체 완료, 비영은 행 시작의 벽시각이 deadline보다 크거나 NaN이면 중단한다.</param>
    public void Scan(uint token, uint budgeted)
    {
        if (Field(token, 0) == 0)
        {
            return;
        }

        double deadline = Need(_hooks.Wall)() + WorkBudget;
        if (Signed(Field(token, 0x30)) >= Signed(Field(token, 0x2c)))
        {
            return;
        }

        // 원본은 행마다 현재 cursor/maxY를 재조회하고 중단시 미처리 행 cursor를 보존하는 반복문이다.
        do
        {
            if (budgeted != 0 && !(Need(_hooks.Wall)() <= deadline))
            {
                return;
            }

            int x = Signed(Field(token, 0x20));
            int y = Signed(Field(token, 0x30));
            // x 시작과 y는 이행 snapshot이며 maxX만 각 후보 효과 뒤 현재값으로 다시 읽는 반복문이다.
            while (x < Signed(Field(token, 0x28)))
            {
                if ((Spot(x, y) & 0x3fu) == 2u && FreeFootprint(Field(token, 0), x, y))
                {
                    float pointX = x;
                    float pointY = y;
                    float avoidX = BitConverter.UInt32BitsToSingle(Field(token, 0x10));
                    float avoidY = BitConverter.UInt32BitsToSingle(Field(token, 0x14));
                    float dx = (float)((double)x - avoidX);
                    double dy = (double)y - avoidY;
                    double distance = (double)dx * dx + dy * dy;
                    float saved = (float)distance;
                    if (distance > BitConverter.UInt32BitsToSingle(Field(token, 4)))
                    {
                        var query = new CanonPlacementQuery(TypeFromHandle(Field(token, 0x18)), 0, pointX, pointY, 0, Field(token, 0x1c) & 0xffu, 1);
                        if (Need(_hooks.MayPlace)(query))
                        {
                            Field(token, 4, BitConverter.SingleToUInt32Bits(saved));
                            Field(token, 8, BitConverter.SingleToUInt32Bits(pointX));
                            Field(token, 0xc, BitConverter.SingleToUInt32Bits(pointY));
                        }
                    }
                }

                x = Add(x, 1u);
            }

            Field(token, 0x30, Field(token, 0x30) + 1u);
        }
        while (Signed(Field(token, 0x30)) < Signed(Field(token, 0x2c)));
    }
}
