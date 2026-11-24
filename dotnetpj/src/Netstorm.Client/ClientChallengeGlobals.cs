using System.Buffers.Binary;
using Netstorm.Core.Original;

// 실제 현재 CS 슬롯과 살아 있는 할당을 Client 수명으로 유지한다.
namespace Netstorm.Client;

/// <summary>ClientChallengeGlobals.cpp의 CS 생성/현재 기록/7bh 주소 수신 부분이다.</summary>
public sealed partial class ClientChallengeGlobals
{
    /// <summary>005467d8의 최초BSS0인 실제 현재 슬롯이다. 조회나 Session 교체로 다시 초기화하지 않는다.</summary>
    public uint Current { get; set; }
    /// <summary>실제 호스트 포인터를 자르지 않는 다음 비영 할당 토큰이다.</summary>
    private uint _nextToken = 1;
    /// <summary>현재 슬롯이 바뀌어도 Client 수명 동안 유지하는 실제21328BYTE 할당들이다.</summary>
    private readonly Dictionary<uint, byte[]> _allocations = [];
    /// <summary>CS 재사용/교체에서도 유지하는 같은 최초 배치 좌표다.</summary>
    public ChallengeGeometry Geometry { get; } = new();
    /// <summary>005467d4의 현재 DWORD이며 0은 고정 22링/비영은 공통 CS 배치 반경을 선택한다.</summary>
    /// <remarks>2026-11-24 0.954에서 R4-6 0x26 메타 geometry 수신 결선용으로 추가했다. 근거: cpppj client/ClientChallengeGlobals.h GeometryMode.</remarks>
    public UIntBox GeometryMode { get; } = new();
    /// <summary>현재 등록된 실제 CS 물리를 빌린다.0이나 없는 토큰을 자동 생성하지 않는다.</summary>
    public Memory<byte> Bytes => Current != 0 && _allocations.TryGetValue(Current, out byte[]? bytes)
        ? bytes : throw new InvalidOperationException("현재 CS의 실제 할당 물리 미등록");

    /// <summary>원문 40개의 현재 구독자 DWORD를 배열 순서로 비교하며 처음 일치를 반환한다. 유효성 비트나 구독자 0은 검색 조건이 아니다. 0.955에서 추가됐다.</summary>
    /// <param name="subscriber">찾을 구독자 번호다.</param>
    /// <returns>처음 일치한 참가자 번호이며 없으면 null이다.</returns>
    public uint? FindParticipantBySubscriber(uint subscriber)
    {
        // 처음 일치한 실제 0번도 반환한다.
        for (uint index = 0; index < RawChallengeState.ParticipantCount; index++)
        {
            if (BinaryPrimitives.ReadUInt32LittleEndian(Participant(index).Span.Slice(4, 4)) == subscriber)
            {
                return index;
            }
        }

        return null;
    }

    /// <summary>실제 호스트 포인터를 자르지 않는 다음 비영 가변 배열 토큰이다.</summary>
    private uint _nextArrayToken = 1;

    /// <summary>현재 CS가 소유하는 실제 가변 배열 할당이다.</summary>
    private readonly Dictionary<uint, byte[]> _arrays = [];

    /// <summary>실제 할당 성공 뒤만 토큰을 발행한다. 0.957에서 추가됐다.</summary>
    /// <param name="bytes">확보할 바이트 수다.</param>
    /// <returns>비영 배열 토큰이며 할당 실패이면 0이다.</returns>
    public uint AllocateArray(int bytes)
    {
        // 토큰이 소진되면 원문처럼 넘침을 표시한다.
        if (_nextArrayToken == 0)
        {
            throw new OverflowException("CS 가변 배열 토큰 소진");
        }

        if (bytes < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "CS 가변 배열의 실제 크기 부족");
        }

        byte[] value = new byte[bytes];
        uint token = _nextArrayToken;
        _arrays.Add(token, value);
        _nextArrayToken = unchecked(_nextArrayToken + 1);
        return token;
    }

    /// <summary>같은 원문 포인터를 조회하며 없는 할당을 빈 배열로 합성하지 않는다. 0.957에서 추가됐다.</summary>
    /// <param name="token">조회할 배열 토큰이다.</param>
    /// <returns>같은 실제 할당 바이트다.</returns>
    public Memory<byte> ArrayBytes(uint token) =>
        token != 0 && _arrays.TryGetValue(token, out byte[]? value)
            ? value
            : throw new InvalidOperationException("CS 가변 배열의 실제 할당 미등록");

    /// <summary>두 포인터를 순서대로 free하고 네 DWORD를 0·4·12·8 순서로 쓴다. 0.957에서 추가됐다.</summary>
    /// <param name="header">실제 16BYTE 헤더다.</param>
    public void ClearArray(Memory<byte> header)
    {
        if (header.Length < 16)
        {
            throw new ArgumentOutOfRangeException(nameof(header), "CS 가변 배열의 실제 16BYTE 헤더 부족");
        }

        // 두 포인터는 순서대로 다시 읽으며 NULL일 때만 free를 생략한다.
        foreach (int at in new[] { 0, 4 })
        {
            uint token = BinaryPrimitives.ReadUInt32LittleEndian(header.Span.Slice(at, 4));
            if (token != 0 && !_arrays.Remove(token))
            {
                throw new InvalidOperationException("CS 가변 배열 free의 실제 할당 미등록");
            }
        }

        // 원문은 두 free가 끝난 뒤에만 네 DWORD를 이 순서로 쓴다.
        foreach (int at in new[] { 0, 4, 12, 8 })
        {
            BinaryPrimitives.WriteUInt32LittleEndian(header.Span.Slice(at, 4), 0);
        }
    }

    /// <summary>같은 현재 여덟 슬롯을 세며 WORD 상한 후 하한 조회와 최종 두 쓰기 순서를 유지한다. 0.957에서 추가됐다.</summary>
    /// <param name="cs">같은 실제 캡처 CS 물리다.</param>
    /// <param name="battle">집계할 전투 번호다.</param>
    /// <param name="assertion">실제 진단 장치다.</param>
    public static void UpdateBattleVersionBounds(Memory<byte> cs, int battle, Action<string, string, uint>? assertion)
    {
        // 원문 진단은 currentCS 물리 조회보다 앞서며 복귀해도 잘못된 배열 주소를 사용하지 않는다.
        if (battle < 0 || battle >= RawChallengeState.BattleCount)
        {
            (assertion ?? throw new InvalidOperationException("CS 버전 집계의 실제 진단 미연결"))("bId >= 0 && bId < CL_NUM_CHAL_BATTLES", ".\\Ns\\O\\Ch.cpp", 0x45a);
            throw new ArgumentOutOfRangeException(nameof(battle), "CS 버전 집계의 실제 전투10칸 범위");
        }

        if (cs.Length != RawChallengeState.Bytes)
        {
            throw new ArgumentOutOfRangeException(nameof(cs), "CS 버전 집계의 실제 캡처 기록 범위");
        }

        Memory<byte> record = cs.Slice(RawChallengeState.BattleOffset + battle * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
        if ((record.Span[0] & 1u) == 0)
        {
            (assertion ?? throw new InvalidOperationException("CS 버전 집계의 실제 진단 미연결"))("b->isValid()", ".\\Ns\\O\\Ch.cpp", 0x45d);
        }

        uint maximum = 0xffffffffu;
        uint minimum = 0x100u;
        // 원문의 두 묶음 네 슬롯을 동일한 현재 순서로 순회한다.
        for (uint slot = 0; slot < 8; slot++)
        {
            uint participant = BinaryPrimitives.ReadUInt32LittleEndian(record.Span.Slice(0x38 + unchecked((int)slot) * 4, 4));
            if (participant == 0)
            {
                continue;
            }

            if (participant >= RawChallengeState.ParticipantCount)
            {
                throw new ArgumentOutOfRangeException(nameof(cs), "CS 버전 집계의 실제 참가자40칸 범위");
            }

            Memory<byte> peer = cs.Slice(RawChallengeState.ParticipantOffset + unchecked((int)participant) * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
            uint upper = BinaryPrimitives.ReadUInt16LittleEndian(peer.Span.Slice(0xb4, 2));
            if (unchecked((int)maximum) <= unchecked((int)upper))
            {
                maximum = upper;
            }

            uint lower = BinaryPrimitives.ReadUInt16LittleEndian(peer.Span.Slice(0xb2, 2));
            if (lower <= minimum)
            {
                minimum = lower;
            }
        }

        BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x1c, 4), minimum);
        BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(0x18, 4), maximum);
    }

    /// <summary>새 생성은 Construct 완료 뒤 공개한다. 기존 객체는 선택 진단 뒤 현재 기록만 Reset한다.</summary>
    public bool Prepare(Action? uninitialized = null)
    {
        if (Current != 0)
        {
            if (Geometry.Initialized == 0)
                (uninitialized ?? throw new InvalidOperationException("기존 CS listsInitialized의 실제 진단 장치 미연결"))();
            RawChallengeState.Reset(Bytes.Span);
            return true;
        }
        if (_nextToken == 0) throw new OverflowException("CS 실제 할당 토큰 소진");
        byte[] memory;
        try { memory = new byte[RawChallengeState.Bytes]; }
        catch (OutOfMemoryException) { Current = 0; return false; }
        RawChallengeState.Construct(memory, Geometry);
        uint token = _nextToken;
        _allocations.Add(token, memory);
        _nextToken = unchecked(_nextToken + 1);
        Current = token;
        return true;
    }

    /// <summary>원문 유효성/참가 조건 없이 실제10칸의 현재 전투 기록을 빌린다.</summary>
    public Memory<byte> Battle(uint index)
    {
        if (index >= RawChallengeState.BattleCount) throw new ArgumentOutOfRangeException(nameof(index), "CS 전투10기록 물리 범위");
        return Bytes.Slice(RawChallengeState.BattleOffset + (int)index * RawChallengeState.BattleBytes, RawChallengeState.BattleBytes);
    }

    /// <summary>원문 active 조건 없이 실제40칸의 현재 참가자 기록을 빌린다.</summary>
    public Memory<byte> Participant(uint index)
    {
        if (index >= RawChallengeState.ParticipantCount) throw new ArgumentOutOfRangeException(nameof(index), "CS 참가자40기록 물리 범위");
        return Bytes.Slice(RawChallengeState.ParticipantOffset + (int)index * RawChallengeState.ParticipantBytes, RawChallengeState.ParticipantBytes);
    }

    /// <summary>현재 선택한BYTE/DWORD만 읽고 뒤쪽 입력 부족 전에 이전 저장을 취소하지 않는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(bytes), "CS 조회의 실제 필드 읽기 범위");
        return width == 1 ? bytes[offset] : BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(offset, 4));
    }

    /// <summary>CS0은 버퍼도 읽지 않는다. 같은 subscriber면 현재 참가자 주소 꼬리에NUL까지 BYTE별 저장한다.</summary>
    public void ReceivePeerAddress(ReadOnlyMemory<byte> packet)
    {
        if (Current == 0) return;
        ReadOnlySpan<byte> input = packet.Span;
        uint participant = Read(input, 4, 1);
        uint subscriber = Read(Participant(participant).Span, 4, 4);
        if (subscriber != Read(input, 5, 4)) return;
        int source = 10;
        Span<byte> address = Participant(Read(input, 4, 1)).Span[0x1a0..];
        int destination = 0;
        // strcpy의 입력 선조회/순차 BYTE 저장을 유지하며 겹친 입력을 사본으로 바꾸지 않는다.
        while (true)
        {
            byte value = (byte)Read(input, source, 1);
            source++;
            if (destination >= address.Length) throw new ArgumentOutOfRangeException(nameof(packet), "참가자 주소의 실제44BYTE 꼬리 쓰기 범위");
            address[destination++] = value;
            if (value == 0) return;
        }
    }
}
