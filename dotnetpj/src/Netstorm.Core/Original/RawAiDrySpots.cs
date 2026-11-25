// AI의 에너지 부족 배치 좌표 목록을 같은 0xb08 본체/12바이트 기록에서 처리한다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

// AI 부족 좌표 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.

// 실제 Mana 판정과 원본 로그 장치를 빌리며 임의 배치 성공으로 대신하지 않는다.
public sealed class AiDrySpotHooks
{
    /// <summary>004734d0의 초기 빈 문자열/좌표/TYPE/소유자와 원래 DWORD 반환이다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    public Func<StringBuilder, float, float, uint, uint, uint>? Energy { get; set; }

    /// <summary>00417c70의 Killed [Fake] Dry Spot과 현재 실제 로그다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    public Action<uint, bool>? Log { get; set; }
}

// 별도 좌표 사본 없이 원본 adc..aec와 같은 소유 backing을 사용한다.
// 근거: cpppj o/RawAiDrySpots.h/.cpp의 RawAiDrySpots다. 2026-11-25 R4-7 ... R4-5b 부족 좌표 이식에서 추가됐다.
public sealed class RawAiDrySpots
{
    /// <summary>원본 내장 DrySpot 목록의 실제 필드다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int CapacityField = 0xadc;

    /// <summary>원본 내장 DrySpot 목록의 실제 개수다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int CountField = 0xae0;

    /// <summary>원본 내장 DrySpot 목록의 실제 포인터다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int PointerField = 0xae4;

    /// <summary>원본 내장 DrySpot 목록의 실제 cursor다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int CursorField = 0xae8;

    /// <summary>원본 내장 DrySpot 목록의 실제 owner다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int OwnerField = 0xaec;

    /// <summary>한 기록의 x/y float 비트와 TYPE DWORD 크기다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private const int RecordBytes = 12;

    /// <summary>같은 PATCH 풀/본체 등록부를 빌린다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>같은 월드 정체성을 유지한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>선택된 실제 에너지/로그 효과다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private readonly AiDrySpotHooks _hooks;

    /// <summary>생성시 게임판단/목록할당을 수행하지 않는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 실제 소유 본체/목록 등록부다.</param>
    /// <param name="pool">같은 판본/월드 정체성이다.</param>
    /// <param name="hooks">선택된 실제 에너지/로그 효과다.</param>
    public RawAiDrySpots(RawAiBrainMemory memory, RawSidPool pool, AiDrySpotHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI DrySpot은 같은 PATCH풀만 지원한다", nameof(pool));
        }

        _memory = memory;
        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>해제 토큰/작은 가짜 본체를 거부한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI DrySpot 실제0xb08본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>하위 효과 뒤 현재 본체를 다시 빌린다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">읽을 DWORD 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan(offset, 4));

    /// <summary>선택 필드만 쓴다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">쓸 DWORD 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Field(uint token, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Body(token).AsSpan(offset, 4), value);

    /// <summary>포인터를 미리 복사한 별도 목록으로 대체하지 않는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 ae4가 가리키는 동일 backing이다.</returns>
    private byte[] Records(uint token) => _memory.Bytes(Field(token, PointerField));

    /// <summary>원본 정상 범위 밖 메모리 접근을 다른 항목/자동 clamp로 바꾸지 않는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">현재 backing 바이트다.</param>
    /// <param name="index">조회할 기록 번호다.</param>
    /// <returns>기록 시작 위치다.</returns>
    private static int Offset(ReadOnlySpan<byte> raw, int index)
    {
        if (index < 0 || (uint)index > (uint)raw.Length / RecordBytes || (uint)raw.Length - (uint)index * RecordBytes < RecordBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI DrySpot 기록 물리 범위");
        }

        return index * RecordBytes;
    }

    /// <summary>비정렬 DWORD를 물리 범위 안에서 읽는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        if ((uint)offset > (uint)raw.Length || (uint)raw.Length - (uint)offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI DrySpot DWORD 읽기 범위");
        }

        // 실제 네 BYTE만 낮은 자리부터 합친다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));
    }

    /// <summary>지정 네 BYTE만 쓰고 주변/미사용 꼬리를 보존한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        if ((uint)offset > (uint)raw.Length || (uint)raw.Length - (uint)offset < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI DrySpot DWORD 쓰기 범위");
        }

        // 원본 little endian 표현을 기록한다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);
    }

    /// <summary>DWORD의 signed 비교/나머지에서 구현 의존 변환을 피한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>실제 하위가 없으면 앞선 원문 쓰기를 유지한 채 거부한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI DrySpot 실제 하위 미연결");

    /// <summary>용량이 찼으면 포인터/중복 검색을 읽지 않고 나간다. 현재 TYPE가 달라도 같은 좌표면 중복이다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="x">추가할 X 좌표다.</param>
    /// <param name="y">추가할 Y 좌표다.</param>
    /// <param name="type">추가할 TYPE DWORD다.</param>
    /// <returns>추가했으면 참이다.</returns>
    public bool Add(uint token, float x, float y, uint type)
    {
        int count = Signed(Field(token, CountField));
        if (count >= Signed(Field(token, CapacityField)))
        {
            return false;
        }

        // float 동등 비교로 signed 0을 같게 보고 NaN 좌표는 중복으로 제거하지 않는 반복문이다.
        for (int index = 0; index < Signed(Field(token, CountField)); index++)
        {
            byte[] raw = Records(token);
            int at = Offset(raw, index);
            if (x == BitConverter.UInt32BitsToSingle(Read(raw, at)) && y == BitConverter.UInt32BitsToSingle(Read(raw, at + 4)))
            {
                return false;
            }
        }

        byte[] target = Records(token);
        int position = Offset(target, count);
        Put(target, position, BitConverter.SingleToUInt32Bits(x));
        Put(target, position + 4, BitConverter.SingleToUInt32Bits(y));
        Put(target, position + 8, type);
        Field(token, CountField, Field(token, CountField) + 1u);
        return true;
    }

    /// <summary>원문은 index가 끝 이상이면 복사 없이 count만 감소하며 cursor/미사용 꼬리를 초기화하지 않는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="index">지울 기록 번호다.</param>
    public void Remove(uint token, int index)
    {
        // 각 기록을 x→y→TYPE 순으로 옮기는 반복문이다.
        while (index < Signed(Field(token, CountField) - 1u))
        {
            byte[] raw = Records(token);
            int target = Offset(raw, index);
            int source = Offset(raw, Signed(unchecked((uint)index) + 1u));
            Put(raw, target, Read(raw, source));
            Put(raw, target + 4, Read(raw, source + 4));
            Put(raw, target + 8, Read(raw, source + 8));
            index = Signed(unchecked((uint)index) + 1u);
        }

        Field(token, CountField, Field(token, CountField) - 1u);
    }

    /// <summary>한 번에 한 기록만 검사하며 제거 뒤 원본 cursor를 보정하지 않는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>충족한 현재 기록 TYPE이며 없으면 0이다.</returns>
    public uint Scan(uint token)
    {
        int count = Signed(Field(token, CountField));
        if (count == 0)
        {
            return 0;
        }

        int numerator = Signed(Field(token, CursorField) + 1u);
        if (count == -1 && numerator == int.MinValue)
        {
            throw new OverflowException("AI DrySpot signed 나눗셈 넘침");
        }

        int index = numerator % count;
        uint pointer = Field(token, PointerField);
        uint owner = Field(token, OwnerField);
        Field(token, CursorField, unchecked((uint)index));
        byte[] raw = _memory.Bytes(pointer);
        int at = Offset(raw, index);
        var text = new StringBuilder();
        if (Need(_hooks.Energy)(text, BitConverter.UInt32BitsToSingle(Read(raw, at)), BitConverter.UInt32BitsToSingle(Read(raw, at + 4)), Read(raw, at + 8), owner) == 0)
        {
            return 0;
        }

        int current = Signed(Field(token, CursorField));
        byte[] records = Records(token);
        uint type = Read(records, Offset(records, current) + 8);
        Remove(token, current);
        return type;
    }

    /// <summary>반환 TYPE가 0이면 로그를 내지 않고 현재 fake 종류와 디버그 값을 읽는다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>제거한 현재 기록 TYPE이며 없으면 0이다.</returns>
    public uint Maintain(uint token)
    {
        uint type = Scan(token);
        if (type != 0)
        {
            bool fake = type == Field(token, 0x280);
            if (Field(token, 0xb00) != 0)
            {
                Need(_hooks.Log)(token, fake);
            }
        }

        return type;
    }

    /// <summary>전체 합/곱 뒤 네 float를 저장하고, 난수 한 번 뒤 원본 순서로 두 좌표를 기록한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="random">같은 공용 난수 스트림이다.</param>
    public void Seed(uint token, GameRandom random)
    {
        ArgumentNullException.ThrowIfNull(random);
        if ((Field(token, 0x248) & 0x20u) == 0)
        {
            return;
        }

        // 이 조회는 현재 본체 float를 넓혀 읽고 계산 중간의 임의 float 반올림을 피한다.
        double Number(int at) => BitConverter.UInt32BitsToSingle(Field(token, at));
        float combinedX = (float)(((Number(0x2c0) + Number(0x2b8)) + Number(0x278) + Number(0x270)) * 0.25);
        float combinedY = (float)(((Number(0x2c4) + Number(0x2bc)) + Number(0x27c) + Number(0x274)) * 0.25);
        float ownX = (float)((Number(0x278) + Number(0x270)) * 0.5);
        float ownY = (float)((Number(0x27c) + Number(0x274)) * 0.5);
        switch (random.Next(8u))
        {
            case 0: Field(token, 0xaf4, Field(token, 0x274)); Field(token, 0xaf0, BitConverter.SingleToUInt32Bits(combinedX)); break;
            case 1: Field(token, 0xaf4, Field(token, 0x27c)); Field(token, 0xaf0, BitConverter.SingleToUInt32Bits(combinedX)); break;
            case 2: Field(token, 0xaf0, Field(token, 0x270)); Field(token, 0xaf4, BitConverter.SingleToUInt32Bits(combinedY)); break;
            case 3: Field(token, 0xaf4, BitConverter.SingleToUInt32Bits(combinedY)); Field(token, 0xaf0, Field(token, 0x278)); break;
            case 4: Field(token, 0xaf4, Field(token, 0x274)); Field(token, 0xaf0, BitConverter.SingleToUInt32Bits(ownX)); break;
            case 5: Field(token, 0xaf4, Field(token, 0x27c)); Field(token, 0xaf0, BitConverter.SingleToUInt32Bits(ownX)); break;
            case 6: Field(token, 0xaf0, Field(token, 0x270)); Field(token, 0xaf4, BitConverter.SingleToUInt32Bits(ownY)); break;
            default: Field(token, 0xaf4, BitConverter.SingleToUInt32Bits(ownY)); Field(token, 0xaf0, Field(token, 0x278)); break;
        }

        uint type = Field(token, 0x280);
        uint y = Field(token, 0xaf4);
        uint x = Field(token, 0xaf0);
        Add(token, BitConverter.UInt32BitsToSingle(x), BitConverter.UInt32BitsToSingle(y), type);
    }

    /// <summary>같은 실제 월드의 풀인지 확인한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
